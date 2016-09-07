; future-os boot asm
; TAB=4

BOTPAK	EQU		0x00280000		; bootpack
DSKCAC	EQU		0x00100000		; 甘徒産贋議仇圭
DSKCAC0	EQU		0x00008000		; 甘徒産贋議仇圭��寔糞庁塀��

; BOOT_INFO�犢慚渡�壓坪贋嶄議贋刈仇峽
CYLS	EQU		0x0ff0			; 贋慧亟秘議庠中倖方
LEDS	EQU		0x0ff1			; 贋慧囚徒貧光嶽LED峺幣菊議彜蓑
VMODE	EQU		0x0ff2			; 贋慧冲弼方朕議佚連��冲弼議了方。
SCRNX	EQU		0x0ff4			; 贋慧蛍掩楕議X
SCRNY	EQU		0x0ff6			; 贋慧蛍掩楕議Y
VRAM	EQU		0x0ff8			; 贋慧夕�饂些綰�議蝕兵仇峽

		ORG		0xc200			; 峺苧殻會議廾墮仇峽

; 譜協鮫中庁塀

		MOV		AL,0x13			; VGA�埒┌�320x200x8bit科弼
		MOV		AH,0x00
		INT		0x10
		MOV		BYTE [VMODE],8	; 芝村鮫中庁塀
		MOV		WORD [SCRNX],320
		MOV		WORD [SCRNY],200
		MOV		DWORD [VRAM],0x000a0000

; 喘BIOS函誼囚徒貧光嶽LED峺幣菊議彜蓑

		MOV		AH,0x02
		INT		0x16 			; keyboard BIOS
		MOV		[LEDS],AL

; PIC購液匯俳嶄僅
;	功象AT惹否字議号鯉��泌惚勣兜兵晒PIC��
;	駅倬壓CLI岻念序佩��倦夸嗤扮氏航軟。
;	昧朔序佩PIC議兜兵晒

		MOV		AL,0xff
		OUT		0x21,AL
		NOP						; 泌惚銭偬峇佩OUT峺綜��嗤乂字嶽氏涙隈屎械塰佩
		OUT		0xa1,AL

		CLI						; 鋤峭CPU雫艶議嶄僅

; CPUから1MB參貧のメモリにアクセスできるように、A20GATEを�O協

		CALL	waitkbdout
		MOV		AL,0xd1
		OUT		0x64,AL
		CALL	waitkbdout
		MOV		AL,0xdf			; enable A20
		OUT		0x60,AL
		CALL	waitkbdout

; プロテクトモ�`ド卞佩

[INSTRSET "i486p"]				; 486の凋綜まで聞いたいという��峰

		LGDT	[GDTR0]			; �唆�GDTを�O協
		MOV		EAX,CR0
		AND		EAX,0x7fffffff	; bit31を0にする�┘擧`ジング鋤峭のため��
		OR		EAX,0x00000001	; bit0を1にする�┘廛蹈謄�トモ�`ド卞佩のため��
		MOV		CR0,EAX
		JMP		pipelineflush
pipelineflush:
		MOV		AX,1*8			;  �iみ��き辛嬬セグメント32bit
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; bootpackの��僕

		MOV		ESI,bootpack	; ��僕圷
		MOV		EDI,BOTPAK		; ��僕枠
		MOV		ECX,512*1024/4
		CALL	memcpy

; ついでにディスクデ�`タも云栖の了崔へ��僕

; まずはブ�`トセクタから

		MOV		ESI,0x7c00		; ��僕圷
		MOV		EDI,DSKCAC		; ��僕枠
		MOV		ECX,512/4
		CALL	memcpy

; 火り畠何

		MOV		ESI,DSKCAC0+512	; ��僕圷
		MOV		EDI,DSKCAC+512	; ��僕枠
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; シリンダ方からバイト方/4に���Q
		SUB		ECX,512/4		; IPLの蛍だけ餓し哈く
		CALL	memcpy

; asmheadでしなければいけないことは畠何し�Kわったので、
;	あとはbootpackに販せる

; bootpackの軟��

		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; ��僕するべきものがない
		MOV		ESI,[EBX+20]	; ��僕圷
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; ��僕枠
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; スタック兜豚��
		JMP		DWORD 2*8:0x0000001b

waitkbdout:
		IN		 AL,0x64
		AND		 AL,0x02
		JNZ		waitkbdout		; ANDの�Y惚が0でなければwaitkbdoutへ
		RET

memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; 哈き麻した�Y惚が0でなければmemcpyへ
		RET
; memcpyはアドレスサイズプリフィクスを秘れ梨れなければ、ストリング凋綜でも��ける

		ALIGNB	16
GDT0:
		RESB	8				; ヌルセレクタ
		DW		0xffff,0x0000,0x9200,0x00cf	; �iみ��き辛嬬セグメント32bit
		DW		0xffff,0x0000,0x9a28,0x0047	; �g佩辛嬬セグメント32bit��bootpack喘��

		DW		0
GDTR0:
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
bootpack:
