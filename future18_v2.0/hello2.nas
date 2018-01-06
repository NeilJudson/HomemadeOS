[INSTRSET "i486p"]                              ; 要使用486的指令
[BITS 32]
		MOV		EDX,2
		MOV		EBX,msg
		INT		0x40
		RETF
msg:
		DB      "hello",0
