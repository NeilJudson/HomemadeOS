#ifndef MEMORYH
#include "memory.h"
#endif

#ifndef SHEETH
#define SHEETH

#define MAX_SHEETS		256

struct SHEET {
	unsigned char *buf;
	int bxsize, bysize, vx0, vy0, col_inv, height, flags; 		/* col_inv：透明色色号，height：图层高度，flags：图层设定信息 */
	struct SHTCTL *ctl;
};
struct SHTCTL {
	unsigned char *vram, *map;
	int xsize, ysize, top;										/* top：最上面图层的高度 */
	struct SHEET *sheets[MAX_SHEETS];							/* 记忆地址变量 */
	struct SHEET sheets0[MAX_SHEETS];							/* 存放准备的256个图层的信息 */
};

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);

#endif
