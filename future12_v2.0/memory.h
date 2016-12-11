#ifndef MEMORY_H
#define MEMORY_H

#define MEMMAN_FREES	4090							/* 大约32KB */
#define MEMMAN_ADDR		0x003c0000

struct FREEINFO {										/* 可用信息 */
	unsigned int addr, size;
};
struct MEMMAN {											/* 内存管理 */
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

#endif
