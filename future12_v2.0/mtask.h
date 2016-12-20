#include "timer.h"

#ifndef MTASK_H
#define MTASK_H
extern struct TIMER *mt_timer;

void mt_init(void);
void mt_taskswitch(void);

#endif
