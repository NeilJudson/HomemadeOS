#include "timer.h"
#include "memory.h"

#ifndef MTASK_H
#define MTASK_H

#define MAX_TASKS		1000									/* 最大任务数 */
#define TASK_GDT0		3										/* 定义从GDT的几号开始分配给TSS */

struct TSS32 {
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;			// 与任务设置相关的信息
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
};
struct TASK {
	/*
	* sel：		用来存放GDT的编号
	* flags：	表示任务状态；2为活动中；1为正在使用，但处于休眠状态；0为未使用
	*/
	int sel, flags;
	struct TSS32 tss;
};
struct TASKCTL {
	int running;												/* 正在运行的任务数量 */
	int now;													/* 这个变量用来记录当前正在运行的是哪个任务 */
	struct TASK *tasks[MAX_TASKS];
	struct TASK tasks0[MAX_TASKS];
};

extern struct TIMER *task_timer;

struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *task);
void task_switch(void);
void task_sleep(struct TASK *task);

#endif
