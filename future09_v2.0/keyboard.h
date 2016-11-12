#ifndef KEYBOARDH
#define KEYBOARDH
#define PORT_KEYDAT		0x0060
#define PORT_KEYCMD		0x0064

extern struct FIFO8 keyfifo;

void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(void);
#endif