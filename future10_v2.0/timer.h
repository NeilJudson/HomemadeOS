#include "fifo.h"

#ifndef TIMERH
#define TIMERH

#define MAX_TIMER		500

struct TIMER {
	unsigned int timeout, flags;								// timeout: 结束时刻
	struct FIFO32 *fifo;
	int data;
};
struct TIMERCTL {
	unsigned int count, next, using;							// next: 记录下一个时刻
	struct TIMER *timers[MAX_TIMER];
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
