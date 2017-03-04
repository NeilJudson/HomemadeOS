#ifndef FIFO_H
#define FIFO_H

struct FIFO32 {
	int *buf;
	int p, q, size, free, flag;                                 // p：下一个数据写入位置；q：下一个数据读出位置；task：有数据写入时需要唤醒的任务
	struct TASK *task;
};

void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);
int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);

#endif
