#include "fifo.h"

#ifndef KEYBOARD_H
#define KEYBOARD_H

#define PORT_KEYDAT		0x0060
#define PORT_KEYCMD		0x0064


void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *fifo, int data0);

#endif
