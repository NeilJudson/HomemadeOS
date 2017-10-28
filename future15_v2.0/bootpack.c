#include <stdio.h>
#include "bootpack.h"
#include "naskfunc.h"
#include "fifo.h"
#include "graphic.h"
#include "dsctbl.h"
#include "int.h"
#include "keyboard.h"
#include "mouse.h"
#include "memory.h"
#include "sheet.h"
#include "timer.h"
#include "mtask.h"

#define KEYCMD_LED      0xed

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
void console_task(struct SHEET *sheet);

void HariMain(void)
{
	struct FIFO32       fifo, keycmd;
	int                 fifobuf[128], keycmd_buf[32];
	struct MOUSE_DEC    mdec;
	unsigned int        memtotal;
	struct SHTCTL       *shtctl;
	struct MEMMAN       *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct BOOTINFO     *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	struct TASK         *task_a, *task_cons;
	struct SHEET        *sht_back, *sht_mouse, *sht_win, *sht_cons;
	unsigned char       *buf_back, buf_mouse[256], *buf_win, *buf_cons;
	int                 cursor_x, cursor_c, mx, my, i;          // cursor_x: 记录光标显示位置
	struct TIMER        *timer;
	char                s[40];
	static char         keytable0[0x80] = {
	                          0,   0, '1',  '2', '3', '4', '5', '6',  '7', '8', '9',  '0', '-',  '=',   0,   0,
	                        'Q', 'W', 'E',  'R', 'T', 'Y', 'U', 'I',  'O', 'P', '[',  ']',   0,    0, 'A', 'S',
	                        'D', 'F', 'G',  'H', 'J', 'K', 'L', ';', '\'', '`',   0, '\\', 'Z',  'X', 'C', 'V',
	                        'B', 'N', 'M',  ',', '.', '/',   0, '*',    0, ' ',   0,    0,   0,    0,   0,   0,
	                          0,   0,   0,    0,   0,   0,   0, '7',  '8', '9', '-',  '4', '5',  '6', '+', '1',
	                        '2', '3', '0',  '.',   0,   0,   0,   0,    0,   0,   0,    0,   0,    0,   0,   0,
	                          0,   0,   0,    0,   0,   0,   0,   0,    0,   0,   0,    0,   0,    0,   0,   0,
	                          0,   0,   0, 0x5c,   0,   0,   0,   0,    0,   0,   0,    0,   0, 0x5c,   0,   0
	                    };
	static char         keytable1[0x80] = {
	                          0,   0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',   0,   0,
	                        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',   0,   0, 'A', 'S',
	                        'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',   0, '|', 'Z', 'X', 'C', 'V',
	                        'B', 'N', 'M', '<', '>', '?',   0, '*',   0, ' ',   0,   0,   0,   0,   0,   0,
	                          0,   0,   0,   0,   0,   0,   0, '7',  '8', '9', '-', '4', '5', '6', '+', '1',
	                        '2', '3', '0', '.',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	                          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	                          0,   0,   0, '_',   0,   0,   0,   0,   0,   0,   0,   0,   0, '|',   0,   0
	                    };
	int                 key_to = 0, key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1; // key_to为1时，console窗口有效

	init_gdtidt();
	init_pic();
	io_sti();                                                   /* IDT/PIC的初始化结束，开启CPU中断 */

	fifo32_init(&fifo, 128, fifobuf, 0);
	init_pit();
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	io_out8(PIC0_IMR, 0xf8);                                    /* 许可PIC1和键盘(11111000) */
	io_out8(PIC1_IMR, 0xef);                                    /* 许可鼠标(11101111) */
	fifo32_init(&keycmd, 32, keycmd_buf, 0);

	memtotal = memtest(0x00400000, 0xbfffffff);                 // 使用的内存空间，包含了0x00400000前已用的内存
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);                /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	
	task_a = task_init(memman);                                 // 为什么没给tss.eip、tss.esp赋值？task_a什么都不干。
	fifo.task = task_a;
	task_run(task_a, 1, 2);

	/* sht_back */
	sht_back = sheet_alloc(shtctl);
	buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* 没有透明色 */
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);

	/* sht_cons */
	sht_cons = sheet_alloc(shtctl);
	buf_cons = (unsigned char *) memman_alloc_4k(memman, 256 * 165);
	sheet_setbuf(sht_cons, buf_cons, 256, 165, -1);             /* 无透明色 */
	make_window8(buf_cons, 256, 165, "console", 0);             // act为1时，颜色不变；act为0时，标题栏变灰色。
	make_textbox8(sht_cons, 8, 28, 240, 128, COL8_000000);
	task_cons = task_alloc();
	task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
	task_cons->tss.eip = (int) &console_task;
	task_cons->tss.es = 1 * 8;
	task_cons->tss.cs = 2 * 8;                                  // 与bootpack.c相同的地址段，asmkfunc.nas中bootpack启动部分
	task_cons->tss.ss = 1 * 8;
	task_cons->tss.ds = 1 * 8;
	task_cons->tss.fs = 1 * 8;
	task_cons->tss.gs = 1 * 8;
	*((int *) (task_cons->tss.esp + 4)) = (int) sht_cons;       // 传参
	task_run(task_cons, 2, 2);                                  /* level=2, priority=2 */

	/* sht_win */
	sht_win = sheet_alloc(shtctl);
	buf_win = (unsigned char *) memman_alloc_4k(memman, 160 * 52);
	sheet_setbuf(sht_win, buf_win, 144, 52, -1);                /* 没有透明色 */
	make_window8(buf_win, 144, 52, "task_a", 1);                // act为1时，颜色不变；act为0时，标题栏变灰色。
	make_textbox8(sht_win, 8, 28, 128, 16, COL8_FFFFFF);
	cursor_x = 8;
	cursor_c = COL8_FFFFFF;
	timer = timer_alloc();
	timer_init(timer, &fifo, 1);
	timer_settime(timer, 50);

	/* sht_mouse */
	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);             // 透明色号99
	init_mouse_cursor8(buf_mouse, 99);
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;

	sheet_slide(sht_back ,  0,  0);
	sheet_slide(sht_cons , 32,  4);
	sheet_slide(sht_win  , 64, 56);
	sheet_slide(sht_mouse, mx, my);
	sheet_updown(sht_back, 0);
	sheet_updown(sht_cons, 1);
	sheet_updown(sht_win , 2);
	sheet_updown(sht_mouse, 3);
	sprintf(s, "(%3d, %3d)", mx, my);
	putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);

	sprintf(s, "memory %dMB   free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_asc_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_008484, s, 40);

	/* 为了避免和键盘当前状态冲突，在一开始先进行设置 */
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);

	for (;;) {
	    if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			/* 如果存在想键盘控制器发送的数据，则发送 */
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		io_cli();
		if (fifo32_status(&fifo) == 0) {
			/* J
			char ss[10];
			sprintf(ss, "%c", "J");
			putfonts8_asc(binfo->vram, binfo->scrnx, 100, 100, COL8_FFFFFF, ss);
			*/
			task_sleep(task_a);
			io_sti();
		} else {
			i = fifo32_get(&fifo);
			io_sti();
			if (256 <= i && i <= 511) {							/* 键盘数据 */
				sprintf(s, "%02X", i - 256);
				putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
				if (i < 0x80 + 256) {                           /* 将按键编码转换为字符编码 */
					if (key_shift == 0) {
						s[0] = keytable0[i - 256];
					} else {
						s[0] = keytable1[i - 256];
					}
				} else {
					s[0] = 0;
				}
	            if ('A' <= s[0] && s[0] <= 'Z') {               /* 当输入字符为应为字符时 */
	                if (((key_leds & 4) == 0 && key_shift == 0) || ((key_leds & 4) != 0 && key_shift != 0)) {
	                    s[0] += 0x20;                           /* 将大写字母转换为小写字母 */
	                }
	            }
				if (s[0] != 0) {                                /* 通常文字 */
					if (key_to == 0) {                          /* 发送给任务A */
						if (cursor_x < 128) {
							/* 显示1个字符就前移1次光标 */
							s[1] = 0;
							putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
							cursor_x += 8;
						}
					} else {                                    /* 发送给命令行窗口 */
						fifo32_put(&task_cons->fifo, s[0] + 256);
					}
				}
				if (i == 256 + 0x0e) {                          /* 退格键 */
					if (key_to == 0) {                          /* 发送给任务A */
						if (cursor_x > 8) {
							/* 用空格键把光标消去后，后移1次光标 */
							putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
							cursor_x -= 8;
						}
					} else {                                    /* 发送给命令行窗口 */
						fifo32_put(&task_cons->fifo, 8 + 256);
					}
				}
				if (i == 256 + 0x1c) {                          /* 回车键 */
					if (key_to != 0) {                          /* 发送至命令行窗口 */
						fifo32_put(&task_cons->fifo, 10 + 256);
					}
				}
				if (i == 256 + 0x0f) {                          /* Tab */
					if (key_to == 0) {
						key_to = 1;
						make_wtitle8(buf_win,  sht_win->bxsize,  "task_a",  0);
						make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
						cursor_c = -1;                          /* 不显示光标 */
						boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cursor_x, 28, cursor_x + 7, 43);
						fifo32_put(&task_cons->fifo, 2);        /* 命令行窗口光标ON */
					} else {
						key_to = 0;
						make_wtitle8(buf_win,  sht_win->bxsize,  "task_a",  1);
						make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
						cursor_c = COL8_000000;                 /* 显示光标 */
						fifo32_put(&task_cons->fifo, 3);        /* 命令行窗口光标OFF */
					}
					sheet_refresh(sht_win,  0, 0, sht_win->bxsize,  21);
					sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);
				}
				if (i == 256 + 0x2a) {                          /* 左Shift ON */
					key_shift |= 1;                             // 直接赋值呢？
				}
				if (i == 256 + 0x36) {                          /* 右Shift ON */
					key_shift |= 2;
				}
				if (i == 256 + 0xaa) {                          /* 左Shift OFF */
					key_shift &= ~1;
				}
				if (i == 256 + 0xb6) {                          /* 右Shift OFF */
					key_shift &= ~2;
				}
				if (i == 256 + 0x3a) {                          /* CapsLock */
					key_leds ^= 4;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x45) {                          /* NumLock */
					key_leds ^= 2;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x46) {                          /* ScrollLock */
					key_leds ^= 1;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0xfa) {                          /* 键盘成功接收到数据 */
					keycmd_wait = -1;
				}
				if (i == 256 + 0xfe) {                          /* 键盘没有成功接收到数据 */
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
				/* 重新显示光标 */
				if (cursor_c >= 0) {
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				}
				sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
			} else if (512 <= i && i <= 767) { /* マウスデータ */
				if (mouse_decode(&mdec, i - 512) != 0) {
					/* 鼠标的3个字节都齐了，显示出来 */
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0) {
						s[1] = 'L';
					}
					if ((mdec.btn & 0x02) != 0) {
						s[3] = 'R';
					}
					if ((mdec.btn & 0x04) != 0) {
						s[2] = 'C';
					}
					putfonts8_asc_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, s, 15);
					/* 移动鼠标 */
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}
					sprintf(s, "(%3d, %3d)", mx, my);
					putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
					sheet_slide(sht_mouse, mx, my);
					if ((mdec.btn & 0x01) != 0) {
						/* 按下左键、移动sht_win */
						sheet_slide(sht_win, mx - 80, my - 8);
					}
				}
			} else if (i <= 1) {								/* 光标用定时器 */
				if (i != 0) {
					timer_init(timer, &fifo, 0);				/* 下面设定0 */
					if (cursor_c >= 0) {
						cursor_c = COL8_000000;
					}
				} else {
					timer_init(timer, &fifo, 1);				/* 下面设定1 */
					if (cursor_c >= 0) {
						cursor_c = COL8_FFFFFF;
					}
				}
				timer_settime(timer, 50);
				if (cursor_c >= 0) {
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
					sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
				}
			}
		}
	}
}

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act)
{
	boxfill8(buf, xsize, COL8_C6C6C6,         0,         0, xsize - 1,         0);
	boxfill8(buf, xsize, COL8_FFFFFF,         1,         1, xsize - 2,         1);
	boxfill8(buf, xsize, COL8_C6C6C6,         0,         0,         0, ysize - 1);
	boxfill8(buf, xsize, COL8_FFFFFF,         1,         1,         1, ysize - 2);
	boxfill8(buf, xsize, COL8_848484, xsize - 2,         1, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, xsize - 1,         0, xsize - 1, ysize - 1);
	boxfill8(buf, xsize, COL8_C6C6C6,         2,         2, xsize - 3, ysize - 3);
	boxfill8(buf, xsize, COL8_848484,         1, ysize - 2, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000,         0, ysize - 1, xsize - 1, ysize - 1);
	make_wtitle8(buf, xsize, title, act);
	return;
}

void make_wtitle8(unsigned char *buf, int xsize, char *title, char act)
{
	/*
	* act: 为1时，颜色不变；为0时，标题栏变灰色。
	*/
	static char closebtn[14][16] = {
	    "OOOOOOOOOOOOOOO@",
	    "OQQQQQQQQQQQQQ$@",
	    "OQQQQQQQQQQQQQ$@",
	    "OQQQ@@QQQQ@@QQ$@",
	    "OQQQQ@@QQ@@QQQ$@",
	    "OQQQQQ@@@@QQQQ$@",
	    "OQQQQQQ@@QQQQQ$@",
	    "OQQQQQ@@@@QQQQ$@",
	    "OQQQQ@@QQ@@QQQ$@",
	    "OQQQ@@QQQQ@@QQ$@",
	    "OQQQQQQQQQQQQQ$@",
	    "OQQQQQQQQQQQQQ$@",
	    "O$$$$$$$$$$$$$$@",
	    "@@@@@@@@@@@@@@@@"
	};
	int x, y;
	char c, tc, tbc;
	if (act != 0) {                                             // act为1时，颜色不变；act为0时，标题栏变灰色
		tc = COL8_FFFFFF;
		tbc = COL8_000084;
	} else {
		tc = COL8_C6C6C6;
		tbc = COL8_848484;
	}
	boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
	putfonts8_asc(buf, xsize, 24, 4, tc, title);
	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			c = closebtn[y][x];
			if (c == '@') {
				c = COL8_000000;
			} else if (c == '$') {
				c = COL8_848484;
			} else if (c == 'Q') {
				c = COL8_C6C6C6;
			} else {
				c = COL8_FFFFFF;
			}
			buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
		}
	}
	return;
}

void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l)
{
	/*
	* 重绘sht图层[(x,y),(x+l*8,y+16))区域
	* b: 背景颜色
	* c: 字符颜色
	* s: 字符串
	* l: 字符串长度
	*/
	boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15); // 涂背景色
	putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);			// 写上字符
	sheet_refresh(sht, x, y, x + l * 8, y + 16);				// 刷新
	return;
}

void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c)
{
	int x1 = x0 + sx, y1 = y0 + sy;
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
	boxfill8(sht->buf, sht->bxsize,           c, x0 - 1, y0 - 1, x1 + 0, y1 + 0);
	return;
}

void console_task(struct SHEET *sheet)
{
	struct TIMER *timer;
	struct TASK *task = task_now();
	int i, fifobuf[128], cursor_x = 16, cursor_y = 28, cursor_c = -1;
	char s[2];
	int x, y;

	fifo32_init(&task->fifo, 128, fifobuf, task);
	
	timer = timer_alloc();
	timer_init(timer, &task->fifo, 1);
	timer_settime(timer, 50);

	/* 显示提示符 */
	putfonts8_asc_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">", 1);

	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1) {                                       /* 光标用定时器 */
				if (i != 0) {
					timer_init(timer, &task->fifo, 0);          /* 下次置0 */
					if (cursor_c >= 0) {
						cursor_c = COL8_FFFFFF;
					}
				} else {
					timer_init(timer, &task->fifo, 1);          /* 下次置1 */
					if (cursor_c >= 0) {
						cursor_c = COL8_000000;
					}
				}
				timer_settime(timer, 50);
			}
			// 光标ON、OFF功能用于窗口切换时
			if (i == 2) {                                       /* 光标ON */
				cursor_c = COL8_FFFFFF;
			}
			if (i == 3) {                                       /* 光标OFF */
				boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
				cursor_c = -1;
			}
			if (256 <= i && i <= 511) {                         /* 键盘数据（通过任务A） */
				if (i == 8 + 256) {
					/* 退格键 */
					if (cursor_x > 16) {
						/* 用空白擦除光标后将光标前移一位 */
						putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
						cursor_x -= 8;
					}
				} else if (i == 10 + 256) {
					/* 回车键 */
					/* 用空格将光标擦除 */
					putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
					if (cursor_y < 28 + 112) {
						cursor_y += 16;                         // 换行
					} else {
						/* 滚动 */
						for (y = 28; y < 28 + 112; y++) {       // 上移一行
							for (x = 8; x < 8 + 240; x++) {
								sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
							}
						}
						for (y = 28 + 112; y < 28 + 128; y++) { // 涂黑最后一行
							for (x = 8; x < 8 + 240; x++) {
								sheet->buf[x + y * sheet->bxsize] = COL8_000000;
							}
						}
						sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
					}
					/* 显示提示符 */
					putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, ">", 1);
					cursor_x = 16;
				} else {
					/* 一般字符 */
					if (cursor_x < 240) {
						/* 显示一个字符之后将光标后移一位 */
						s[0] = i - 256;
						s[1] = 0;
						putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
						cursor_x += 8;                          // 后移一位
					}
				}
			}
			/* 重新显示光标 */
			if (cursor_c >= 0) {
				boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
			}
			sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
		}
	}
}
