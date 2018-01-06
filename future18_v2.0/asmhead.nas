; future-os boot asm
; TAB=4

[INSTRSET "i486p"]								; “想要使用486指令”的叙述，是为了能够使用386以后的LGDT，EAX，CR0等关键字。

VBEMODE	EQU		0x105							; 1024x768x8bit彩色
; 画面模式一览
; 0x100 :  640 x  400 x 8bit彩色
; 0x101 :  640 x  480 x 8bit彩色
; 0x103 :  800 x  600 x 8bit彩色
; 0x105 : 1024 x  768 x 8bit彩色
; 0x107 : 1280 x 1024 x 8bit彩色

BOTPAK	EQU		0x00280000						; bootpack
DSKCAC	EQU		0x00100000						; 磁盘缓存的地方
DSKCAC0	EQU		0x00008000						; 磁盘缓存的地方（真实模式）

; BOOT_INFO相关信息在内存中的存储地址
CYLS	EQU		0x0ff0							; 存放写入的柱面个数
LEDS	EQU		0x0ff1							; 存放键盘上各种LED指示灯的状态
VMODE	EQU		0x0ff2							; 存放颜色数目的信息，颜色的位数。
SCRNX	EQU		0x0ff4							; 存放分辨率的X
SCRNY	EQU		0x0ff6							; 存放分辨率的Y
VRAM	EQU		0x0ff8							; 存放图像缓冲区的开始地址

		ORG		0xc200							; 指明程序的装载地址

;===============================================================
; 设置显示模式
;===============================================================
; 确认VBE是否存在
		MOV		AX,0x9000
		MOV		ES,AX
		MOV		DI,0
		MOV		AX,0x4f00						; 此显卡能利用的VBE信息要写入到内存中以ES:DI开始的512字节中
		INT		0x10							; 调用0x10中断，AX=0x4f00功能
		CMP		AX,0x004f
		JNE		scrn320
; 检查VBE的版本
		MOV		AX,[ES:DI+4]
		CMP		AX,0x0200
		JB		scrn320							; if (AX < 0x0200) goto scrn320，如果VBE版本不是2.0以上，就不能使用高分辨率
; 取得画面模式信息
		MOV		CX,VBEMODE
		MOV		AX,0x4f01
		INT		0x10							; 调用0x10中断，AX=0x4f01功能
		CMP		AX,0x004f
		JNE		scrn320
; 画面模式信息的确认
		CMP		BYTE [ES:DI+0x19],8 			; 颜色数，必须为8
		JNE		scrn320
		CMP		BYTE [ES:DI+0x1b],4 			; 颜色的指定方式，必须为4（调色板模式）
		JNE		scrn320
		MOV		AX,[ES:DI+0x00]					; 模式属性，bit7要是1
		AND		AX,0x0080
		JZ		scrn320							; 模式属性的bit7是0，所以放弃
; 画面模式切换
		MOV		BX,VBEMODE+0x4000
		MOV		AX,0x4f02
		INT		0x10							; 调用0x10中断，AX=0x4f02功能
		MOV		BYTE [VMODE],8					; 记录画面模式
		MOV		AX,[ES:DI+0x12]					; X的分辨率
		MOV		[SCRNX],AX
		MOV		AX,[ES:DI+0x14]					; Y的分辨率
		MOV		[SCRNY],AX
		MOV		EAX,[ES:DI+0x28]				; VRAM的地址
		MOV		[VRAM],EAX
		JMP		keystatus
; 320x200x8bit彩色模式
scrn320:
		MOV		AX,0x0013						; VGA图、320x200x8bit彩色
		INT		0x10							; 调用0x10中断，AX=0x0013功能
		MOV		BYTE [VMODE],8					; 记录画面模式
		MOV		WORD [SCRNX],320
		MOV		WORD [SCRNY],200
		MOV		DWORD [VRAM],0x000a0000

;===============================================================
; 
;===============================================================
; 用BIOS取得键盘上各种LED指示灯的状态
keystatus:
		MOV		AH,0x02
		INT		0x16 							; keyboard BIOS
		MOV		[LEDS],AL

; PIC关闭一切中断
;	根据AT兼容机的规格，如果要初始化PIC，必须在CLI之前进行，否则有时会挂起。
;	随后进行PIC的初始化
		MOV		AL,0xff
		OUT		0x21,AL							; RIC0_IMR，禁止主PIC的全部中断
		NOP										; 如果连续执行OUT指令，有些机种会无法正常运行
		OUT		0xa1,AL							; RIC1_IMR，禁止从PIC的全部中断
		CLI										; 禁止CPU级别的中断

; 为了让CPU能够访问1MB以上的内存空间，设定A20GATE
		CALL	waitkbdout
		MOV		AL,0xd1
		OUT		0x64,AL
		CALL	waitkbdout
		MOV		AL,0xdf							; enable A20
		OUT		0x60,AL
		CALL	waitkbdout

; 切换到保护模式
		LGDT	[GDTR0]							; 设定临时GDT，LGDT作用是给GDTR赋值
		MOV		EAX,CR0
		AND		EAX,0x7fffffff					; 设bit31为0，为了禁止分页
		OR		EAX,0x00000001					; 设bit0为1，为了切换到保护模式
		MOV		CR0,EAX
		JMP		pipelineflush					; CPU为了加快指令的执行速度而使用管道机制，前一条指令还在执行时，就开始解释后续指令。因为模式变了，就要重新解释一遍，所以加入JMP指令。
pipelineflush:
		MOV		AX,1*8							; 可读写的段 32bit
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

;===============================================================
; 
;===============================================================
; bootpack的传送
		MOV		ESI,bootpack					; 转送源
		MOV		EDI,BOTPAK						; 转送目的地
		MOV		ECX,512*1024/4
		CALL	memcpy
; 磁盘数据最终转送到它本来的位置去
; 将启动扇区复制到1MB后的内存区
		MOV		ESI,0x7c00						; 转送源
		MOV		EDI,DSKCAC						; 转送目的地0x100000
		MOV		ECX,512/4
		CALL	memcpy
; 所有剩下的
		MOV		ESI,DSKCAC0+512					; 转送源0x8200
		MOV		EDI,DSKCAC+512					; 转送目的地0x100200
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4					; 从柱面数变换为字节数/4
		SUB		ECX,512/4						; 减去IPL
		CALL	memcpy

; 必须由asmhead来完成的工作，至此全部完毕，以后就交由bootpack来完成

; bootpack的启动
; 将bootpack.hrb第0x10c8字节开始的0x11a8字节复制到0x00310000号地址去
		MOV		EBX,BOTPAK						; 0x280000
		MOV		ECX,[EBX+16]
		ADD		ECX,3							; ECX += 3;
		SHR		ECX,2							; ECX /= 4;
		JZ		skip							; 没有要转送的东西时
		MOV		ESI,[EBX+20]					; 转送源
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]					; 转送目的地
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]					; 栈初始值
		JMP		DWORD 2*8:0x0000001b			; J: 这条指令在向EIP存入0x1b的同时，将CS置为2*8（=16）。像这样在JMP目标地址中带冒号（:）的，就是far模式的JMP指令。0x1b其实就是.fex文件中的HariMain的地址。

;===============================================================
; 函数定义
;===============================================================
waitkbdout:
		IN		AL,0x64
		AND		AL,0x02
		IN		AL,0x60							; 空读，为了清空数据接收缓冲中的垃圾数据
		JNZ		waitkbdout						; AND结果如果不是0，就跳转到waitkbdout
		RET

memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy							; 减法运算结果如果不为0就跳转到memcpy
		RET
; memcpyはアドレスサイズプリフィクスを入れ忘れなければ、ストリング命令でも書ける

		ALIGNB	16								; 告诉汇编程序，本伪指令下面的内存变量必须从下一个能被num整除的地址开始分配。
GDT0:											; 这里还不太懂
		RESB	8								; NULL sector，8个Byte
		DW		0xffff,0x0000,0x9200,0x00cf		; 可以读写的段（segment）32bit
		DW		0xffff,0x0000,0x9a28,0x0047		; 可以执行的段（segment）32bit（bootpack用）

		DW		0
GDTR0:											; 这里还不太懂
		DW		8*3-1
		DD		GDT0

		ALIGNB	16

;===============================================================
; bootpack
;===============================================================
bootpack:
