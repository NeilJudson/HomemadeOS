; naskfunc
; TAB=4

[FORMAT "WCOFF"]				; 制作目标文件的模式
[INSTRSET "i486p"]				; 这个程序是给486用的
[BITS 32]						; 制作32位模式用的机器语言
[FILE "naskfunc.nas"]			; 源文件名信息

		; 程序中包含的函数名
		GLOBAL	_io_hlt

[SECTION .text]	; 目标文件中写了这些之后再写程序

_io_hlt:	; void io_hlt(void);
		HLT
		RET
