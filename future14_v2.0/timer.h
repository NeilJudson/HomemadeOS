#include "fifo.h"

#ifndef TIMER_H
#define TIMER_H

#define MAX_TIMER		500

struct TIMER {
	struct TIMER *nextTimer;
	unsigned int timeout, flag;                                 // timeout: 结束时刻
	struct FIFO32 *fifo;
	int data;
};
struct TIMERCTL {
	unsigned int count, nextTime;                               // nextTime: 记录下一个时刻
	struct TIMER *t0;
	struct TIMER timers0[MAX_TIMER];
};

extern struct TIMERCTL timerctl;

void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
void inthandler20(int *esp);

#endif
