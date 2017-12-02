/* ����̨��� */

#include <stdio.h>
#include <string.h>
#include "console.h"
#include "sheet.h"
#include "mtask.h"
#include "timer.h"
#include "memory.h"
#include "bootpack.h"
#include "naskfunc.h"
#include "fifo.h"
#include "graphic.h"
#include "window.h"
#include "file.h"
#include "dsctbl.h"

void console_task(struct SHEET *sheet, unsigned int memtotal)
{
	int                 fifobuf[128];
	struct TASK         *task = task_now();
	struct TIMER        *timer;
	struct MEMMAN       *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int                 *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	struct CONSOLE      cons = {sheet, 8, 28, -1};
	int                 i;
	char                cmdline[30];

	fifo32_init(&task->fifo, 128, fifobuf, task);
	
	timer = timer_alloc();
	timer_init(timer, &task->fifo, 1);
	timer_settime(timer, 50);
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200)); // ��FATչ����fat��

	/* ��ʾ��ʾ�� */
	cons_putchar(&cons, '>', 1);

	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1) {                                       /* ����ö�ʱ�� */
				if (i != 0) {
					timer_init(timer, &task->fifo, 0);          /* �´���0 */
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_FFFFFF;
					}
				} else {
					timer_init(timer, &task->fifo, 1);          /* �´���1 */
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_000000;
					}
				}
				timer_settime(timer, 50);
			}
			// ���ON��OFF�������ڴ����л�ʱ
			if (i == 2) {                                       /* ���ON */
				cons.cur_c = COL8_FFFFFF;
			}
			if (i == 3) {                                       /* ���OFF */
				boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				cons.cur_c = -1;
			}
			if (256 <= i && i <= 511) {                         /* �������ݣ�ͨ������A�� */
				if (i == 8 + 256) {
					/* �˸�� */
					if (cons.cur_x > 16) {
						/* �ÿհײ������󽫹��ǰ��һλ */
						cons_putchar(&cons, ' ', 0);
						cons.cur_x -= 8;
					}
				} else if (i == 10 + 256) {
					/* �س��� */
					/* ������ÿո�������� */
					cons_putchar(&cons, ' ', 0);
					cmdline[cons.cur_x / 8 - 2] = 0;
					cons_newline(&cons);
					cons_runcmd(cmdline, &cons, fat, memtotal); /* �������� */
					/* ��ʾ��ʾ�� */
					cons_putchar(&cons, '>', 1);
				} else {
					/* һ���ַ� */
					if (cons.cur_x < 240) {
						/* ��ʾһ���ַ�֮�󽫹�����һλ */
						cmdline[cons.cur_x / 8 - 2] = i - 256;
						cons_putchar(&cons, i - 256, 1);
					}
				}
			}
			/* ������ʾ��� */
			if (cons.cur_c >= 0) {
				boxfill8(sheet->buf, sheet->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
			}
			sheet_refresh(sheet, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
		}
	}
}

void cons_putchar(struct CONSOLE *cons, char chr, int move)
{
	/* ������� */
	char s[2];
	s[0] = chr;
	s[1] = 0;
	if (s[0] == 0x09) {                                         /* �Ʊ��� */
		for (;;) {
			putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 240) {
				cons_newline(cons);
			}
			if (((cons->cur_x - 8) & 0x1f) == 0) {
				break;                                          /* ��32������break */
			}
		}
	} else if (s[0] == 0x0a) {                                  /* ���� */
		cons_newline(cons);
	} else if (s[0] == 0x0d) {                                  /* �س� */
		/* �������Ҳ������κβ��� */
	} else {                                                    /* һ���ַ� */
		putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
		if (move != 0) {
			/* moveΪ0ʱ��겻���� */
			cons->cur_x += 8;                                   // ����һλ
			if (cons->cur_x == 8 + 240) {                       // �������Ҷ˺���
				cons_newline(cons);
			}
		}
	}
	return;
}

void cons_newline(struct CONSOLE *cons)
{
	int x, y;
	struct SHEET *sheet = cons->sht;
	if (cons->cur_y < 28 + 112) {
		cons->cur_y += 16;                                      // ����
	} else {
		/* ���� */
		for (y = 28; y < 28 + 112; y++) {                       // ����һ��
			for (x = 8; x < 8 + 240; x++) {
				sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
			}
		}
		for (y = 28 + 112; y < 28 + 128; y++) {                 // Ϳ�����һ��
			for (x = 8; x < 8 + 240; x++) {
				sheet->buf[x + y * sheet->bxsize] = COL8_000000;
			}
		}
		sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
	}
	cons->cur_x = 8;
	return;
}

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal)
{
	/* ִ������ */
	if (strcmp(cmdline, "mem") == 0) {
		/* mem���� */
		cmd_mem(cons, memtotal);
	} else if (strcmp(cmdline, "cls") == 0) {
		/* cls���� */
		cmd_cls(cons);
	} else if (strcmp(cmdline, "dir") == 0) {
		/* dir���� */
		cmd_dir(cons);
	} else if (strncmp(cmdline, "type ", 5) == 0) {
		/* type���� */
		cmd_type(cons, fat, cmdline);
	} else if (strcmp(cmdline, "hlt") == 0) {
		/* ����Ӧ�ó���hlt.fex */
		cmd_hlt(cons, fat);
	} else if (cmdline[0] != 0) {
		/* �������Ҳ���ǿ��� */
		putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, "Bad command.", 12);
		cons_newline(cons);
		cons_newline(cons);
	}
	return;
}

void cmd_mem(struct CONSOLE *cons, unsigned int memtotal)
{
	/* mem���� */
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char s[30];
	sprintf(s, "total   %dMB", memtotal / (1024 * 1024));
	putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 30);
	cons_newline(cons);
	sprintf(s, "free %dKB", memman_total(memman) / 1024);
	putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 30);
	cons_newline(cons);
	cons_newline(cons);
	return;
}

void cmd_cls(struct CONSOLE *cons)
{
	/* cls���� */
	int x, y;
	struct SHEET *sheet = cons->sht;
	for (y = 28; y < 28 + 128; y++) {
		for (x = 8; x < 8 + 240; x++) {
			sheet->buf[x + y * sheet->bxsize] = COL8_000000;
		}
	}
	sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
	cons->cur_y = 28;
	return;
}

void cmd_dir(struct CONSOLE *cons)
{
	/* dir���� */
	struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	int i, j;
	char s[30];
	for (i = 0; i < 224; i++) {
		if (finfo[i].name[0] == 0x00) {                         // �������κ��ļ�����Ϣ
			break;
		}
		if (finfo[i].name[0] != 0xe5) {                         // û�б�ɾ��
			if ((finfo[i].type & 0x18) == 0) {
				sprintf(s, "filename.ext   %7d", finfo[i].size);
				for (j = 0; j < 8; j++) {
					s[j] = finfo[i].name[j];
				}
				s[ 9] = finfo[i].ext[0];
				s[10] = finfo[i].ext[1];
				s[11] = finfo[i].ext[2];
				putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 30);
				cons_newline(cons);
			}
		}
	}
	cons_newline(cons);
	return;
}

void cmd_type(struct CONSOLE *cons, int *fat, char *cmdline)
{
	/* type���� */
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo = file_search(cmdline + 5, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	char *p;
	int i;
	if (finfo != 0) {
		/* �ҵ��ļ������ */
		p = (char *) memman_alloc_4k(memman, finfo->size);
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
		for (i = 0; i < finfo->size; i++) {
			cons_putchar(cons, p[i], 1);
		}
		memman_free_4k(memman, (int) p, finfo->size);
	} else {
		/* û���ҵ��ļ������ */
		putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
		cons_newline(cons);
	}
	cons_newline(cons);
	return;
}

void cmd_hlt(struct CONSOLE *cons, int *fat)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo = file_search("HLT.FEX", (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	char *p;
	if (finfo != 0) {
		/* �ҵ��ļ������ */
		p = (char *) memman_alloc_4k(memman, finfo->size);
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
		set_segmdesc(gdt + 1003, finfo->size - 1, (int) p, AR_CODE32_ER);
		farjmp(0, 1003 * 8);
		memman_free_4k(memman, (int) p, finfo->size);
	} else {
		/* û���ҵ��ļ������ */
		putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
		cons_newline(cons);
	}
	cons_newline(cons);
	return;
}