#include "bootpack.h"

#define PIT_CTRL	0x0043
#define PIT_CNT0	0x0040

struct TIMERCTL timerctl;								// 这声明似乎是不必要的，但似乎因为编译器、连接器的原因，此处不声明会报link error

void init_pit(void)
{
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);							// 中断周期的低8位
	io_out8(PIT_CNT0, 0x2e);							// 中断周期的高8位
	timerctl.count = 0;
	return;
}

void inthandler20(int *esp)
{
	io_out8(PIC0_OCW2, 0x60);							/* 把IRQ-00信号接收完了的信息通知给PIC */
	timerctl.count++;
	return;
}
