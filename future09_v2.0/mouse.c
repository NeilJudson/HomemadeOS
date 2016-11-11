#include "bootpack.h"
#include "fifo.h"
#include "int.h"

struct FIFO8 mousefifo;

/* PS/2鼠标的中断 */
void inthandler2c(int *esp)
{
	unsigned char data;
	io_out8(PIC1_OCW2, 0x64);							/* 通知PIC IRQ-12已经受理完毕 */
	io_out8(PIC0_OCW2, 0x62);							/* 通知PIC IRQ-02已经受理完毕 */
	data = io_in8(PORT_KEYDAT);
	fifo8_put(&mousefifo, data);
	return;
}

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

/* 激活鼠标 */
void enable_mouse(struct MOUSE_DEC *mdec)
{
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	mdec->phase = 0;									/* 进入到等待鼠标的0xfa的状态 */
	return;												/* 顺利的话，键盘控制器会返送回ACK(0xfa) */
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
	if (mdec->phase == 0) {
		/* 等待鼠标的0xfa的状态 */
		if (dat == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	}
	if (mdec->phase == 1) {
		/* 等待鼠标的第1个字节 */
		if ((dat & 0xc8) == 0x08) {						// 11001000
			/* 如果第一字节正确 */
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
		return 0;
	}
	if (mdec->phase == 2) {
		/* 等待鼠标的第2个字节 */
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if (mdec->phase == 3) {
		/* 等待鼠标的第3个字节 */
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;				// button
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if ((mdec->buf[0] & 0x10) != 0) {				// 为负，向左
			mdec->x |= 0xffffff00;
		}
		if ((mdec->buf[0] & 0x20) != 0) {				// 为负，向下
			mdec->y |= 0xffffff00;
		}
		mdec->y = - mdec->y;							/* 反转方向 */
		return 1;
	}
	return -1;											/* 应该不会到这 */
}
