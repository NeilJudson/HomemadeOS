/* GDT、IDT、descriptor table等相关 */

#include "bootpack.h"

void init_gdtidt(void)
{
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) ADR_IDT;
	int i;

	/* GDT初始化 */
	for (i = 0; i <= LIMIT_GDT / 8; i++) {						// LIMIT_GDT为存放段信息的内存总Byte数，一个段信息需要8Byte
		set_segmdesc(gdt + i, 0, 0, 0);
	}
	set_segmdesc(gdt + 1, 0xffffffff,   0x00000000, AR_DATA32_RW); // 1号段，4GB，表示CPU所能管理的全部内存本身
	set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER); // 2号段，512KB，地址0x280000，正好涵盖整个bootpack.hrb
	load_gdtr(LIMIT_GDT, ADR_GDT);

	/* IDT初始化 */
	for (i = 0; i <= LIMIT_IDT / 8; i++) {
		set_gatedesc(idt + i, 0, 0, 0);
	}
	load_idtr(LIMIT_IDT, ADR_IDT);

	/* IDT的设定 */
	set_gatedesc(idt + 0x20, (int) asm_inthandler20, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32); // 表示asm_inthandler21属于那一段，即段号是2，低3位有别的作用，必须为0
	set_gatedesc(idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32); // AR_INTGATE32 = 0x008e，表示这是用于中断处理的有效设定
	set_gatedesc(idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x40, (int)      asm_fex_api, 2 * 8, AR_INTGATE32); // 0x30-0xff都是空闲的

	return;
}

void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar)
{
	/*
	* sd:
	* limit:	段的字节数-1
	* base:		基址
	* ar:		访问权限
	*/
	if (limit > 0xfffff) {
		ar |= 0x8000;											// Gbit标志位为1，limit的单位为页，1页指4KB
		limit /= 0x1000;
	}
	sd->limit_low    = limit & 0xffff;
	sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0); // 段属性高4位写入limit_high的高4位
	sd->base_low     = base & 0xffff;
	sd->base_mid     = (base >> 16) & 0xff;
	sd->base_high    = (base >> 24) & 0xff;
	sd->access_right = ar & 0xff;
	return;
}

void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar)
{
	gd->offset_low   = offset & 0xffff;
	gd->offset_high  = (offset >> 16) & 0xffff;
	gd->selector     = selector;
	gd->dw_count     = (ar >> 8) & 0xff;
	gd->access_right = ar & 0xff;
	return;
}
