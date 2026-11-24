;----------------------------------------
;OSI library, low level: critical-error and Ctrl-Break handlers, the computer
;speed, memory allocation, and loading a file whole (LZW-compressed or not).
;Savage Empire's OSILIB/LOW.ASM (SE's GAME.EXE; here 2674) without its first two code segments
;(OSI_selectDrive, OSI_getDrive) and their two bytes of data: its third code
;segment, identical. In GAME.EXE the driver loader (OSILIB/DRVLOAD.ASM, 263D),
;264A, the video-mode reset (OSILIB/VIDRESET.ASM, 2668) and OSIWRITE (2669)
;sit between this module and where SE had its other segments.
;----------------------------------------
INCLUDE OSI.INC

EXTRN OSI_DELAY_CLEAN:FAR

EXTRN CompressedFileList:BYTE	;the compressed files' names (SE: CompressedFileList, main.c)

DSegStart

segment_0	dw 0	;D_2A7C
PUBLIC CpuSpeed
CpuSpeed	dw 0	;D_2A7E computer speed (loops in a timer tick, scaled)
MemorySizeKB	dw 0	;D_2A80 conventional memory, in KB (int 12h)
SpeedTicks	dw 0	;D_2A82 timer ticks, while the speed is measured
PUBLIC CriticalError
CriticalError	dw 0	;D_2A84 set by the critical-error handler (DRVLOAD: the error code)
LowTextWindows	db 8 dup(0)	;D_2A86 text windows (Ultima VI's console; set up, not used)
	db 8 dup(0)
	db 8 dup(0)
	db 8 dup(0)
	db 8 dup(0)
D_2AAE	dw 0
D_2AB0	db 4 dup(0)
PUBLIC OSI_Started
OSI_Started	dw 0	;D_2AB4
PUBLIC TextIdle
TextIdle	dw 1	;D_2AB6
D_2AB8	dw 5
D_2ABA	dw 4
PUBLIC DriveErrorCallback
DriveErrorCallback	dw OFFSET SEG_2674_TEXT:OSI_nullCallback,SEG_2674_TEXT	;D_2ABC
D_2AC0	db 2 dup(0)
LowCurTextWindow	dw 0	;D_2AC2 the current one
D_2AC4	db 4 dup(0)
D_2AC8	db 4 dup(0)
D_2ACC	dw 0
D_2ACE	dw 0
D_2AD0	dw 0
D_2AD2	db 2 dup(0)

DSegEnd

;########################################

DSegStart
masks	dw 1FFh,3FFh,7FFh,0FFFh
lDestSz	dd ?
input_handle	dw 0
output_segment	dw 0
pDest	dw 0
input_segment	dw 0
hash_seg	dw 0
output_offset	dw 0
bit_offset	dw 0
cur_code	dw 0
old_code	dw 0
in_code	dw 0
free_code	dw 0
stack_count	dw 0
max_code	dw 0
nbits	dw 0
fin_char	db 0
k	db 0

OutOfMemoryStr	db "Out of Memory...",13,10,"$"	;D_2AFE
D_2B11	db 0

DSegEnd


CSegStart	SEG_2674_TEXT

;init OSI low?
ProcStart	OSI_lowInit	;2674:0002
	;-- set new int 24h & 23h
	PUSH	DS
	PUSH	CS
	POP	DS
	MOV	DX,OFFSET CritErrorInt
	MOV	AL,24h
	MOV	AH,25h
	INT	21h
	PUSH	CS
	POP	DS
	MOV	DX,OFFSET CtrlBreakInt
	MOV	AL,23h
	MOV	AH,25h
	INT	21h
	POP	DS
	;--
	CALL	initTextWindows
	CALL	clearSpareVars
	CALL	measureCpuSpeed
ProcEnd	OSI_lowInit

;set up the 4 text windows at LowTextWindows, each {0,0,39,24,0,0,0Fh,0} (40x25,
;color 0Fh; Ultima VI's console), LowCurTextWindow the first and D_2AAE 0;
;nothing else in GAME.EXE reads them
initTextWindows	PROC NEAR	;C_002E (2674:002E)
	MOV	SI,OFFSET DGROUP:LowTextWindows
	MOV	LowCurTextWindow,SI
	;--
	XOR	AX,AX
	MOV	CX,4
C_003A:
	MOV	[SI],AL
	MOV	[SI+01],AL
	;--
	MOV	BYTE PTR [SI+02],39
	MOV	BYTE PTR [SI+03],24
	MOV	[SI+04],AL
	MOV	[SI+05],AL
	MOV	BYTE PTR [SI+06],0Fh
	MOV	[SI+07],AL
	;--
	ADD	SI,8
	LOOP	C_003A
	;--
	CBW
	MOV	D_2AAE,AX
	;--
	RET
initTextWindows	ENDP	;C_002E (2674:002E)

;measure the computer's speed: count loop iterations during one timer tick
;(int 1Ch), CpuSpeed = count * 18 / 750; also store the memory size (int 12h)
measureCpuSpeed	PROC NEAR	;C_005E (2674:005E)
	MOV	CpuSpeed,0
	MOV	SpeedTicks,0
	;-- push vector 0x1c
	MOV	AL,1Ch
	MOV	AH,35h
	INT	21h
	PUSH	ES
	PUSH	BX
	;-- set new vector 0x1c
	PUSH	DS
	MOV	AX,CS
	MOV	DS,AX
	LEA	DX,[INT_00BE]
	MOV	AL,1Ch
	MOV	AH,25h
	INT	21h
	POP	DS
C_0082:	;--
	CMP	SpeedTicks,0
	JZ	C_0082
	MOV	SpeedTicks,0
C_008F:	;--
	INC	CpuSpeed
	CMP	SpeedTicks,0
	JZ	C_008F
	;-- restore original vector 0x1c
	POP	DX
	POP	AX
	PUSH	DS
	MOV	DS,AX
	MOV	AL,1Ch
	MOV	AH,25h
	INT	21h
	POP	DS
	;--
	MOV	AX,CpuSpeed
	XOR	DX,DX
	MOV	CX,12h
	MUL	CX
	MOV	CX,2EEh
	DIV	CX
	MOV	CpuSpeed,AX
	;--
	INT	12h
	MOV	MemorySizeKB,AX
	;--
	RET
measureCpuSpeed	ENDP	;C_005E (2674:005E)
;----------------------------------------
;new int 1Ch [determine computer speed]
INT_00BE:
	STI
	PUSH	DS
	PUSH	SI
	PUSH	AX
	;--
	MOV	AX,DGROUP
	MOV	DS,AX
	MOV	SI,OFFSET DGROUP:SpeedTicks
	INC	WORD PTR [SI]
	;--
	POP	AX
	POP	SI
	POP	DS
	IRET
;----------------------------------------
;clear the words D_2ACC, D_2ACE, D_2AD0 (nothing else in GAME.EXE uses them)
clearSpareVars	PROC NEAR	;C_00D0 (2674:00D0)
	XOR	AX,AX
	MOV	D_2ACC,AX
	MOV	D_2ACE,AX
	MOV	D_2AD0,AX
	RET
clearSpareVars	ENDP	;C_00D0 (2674:00D0)
;----------------------------------------
;a callback?
OSI_nullCallback	PROC FAR	;2674:00DC
	RET
OSI_nullCallback	ENDP	;2674:00DC
;----------------------------------------
;new int 24h
CritErrorInt:
	STI
	PUSH	DS
	PUSH	BX
	;--
	MOV	BX,DGROUP
	MOV	DS,BX
	;--
	MOV	BX,OFFSET DGROUP:CriticalError
	MOV	WORD PTR [BX],1
	;--
	XOR	AL,AL
	;--
	POP	BX
	POP	DS
	IRET
;----------------------------------------
;new int 23h
CtrlBreakInt:
	STI
	CLC
	IRET
;----------------------------------------
;unused inflate module
;----------------------------------------
;"out of memory"
C_00F4:
	MOV	AX,3
	INT	10h
	MOV	DX,OFFSET DGROUP:OutOfMemoryStr	;"Out of Memory...",13,10,"$"
	MOV	AH,9
	INT	21h
	CALL	OSI_delay_clean
	MOV	AX,(4Ch SHL 8) OR 1
	INT	21h
	JMP	$
;----------------------------------------
;get file size?
LOAD_size	PROC FAR	;2674:010C
	MOV	AL,2
	MOV	AH,3Dh
	INT	21h
	MOV	BX,AX
	JB	C_012A
	XOR	CX,CX
	MOV	DX,CX
	PUSH	BX
	MOV	AL,2
	MOV	AH,42h
	INT	21h
	POP	BX
	PUSH	AX
	PUSH	DX
	MOV	AH,3Eh
	INT	21h
	POP	DX
	POP	AX
C_012A:
	RET
LOAD_size	ENDP
;----------------------------------------
LOAD_toSeg	PROC FAR	;2674:012B
	PUSH	DS
	PUSH	AX
	PUSH	BX
	MOV	AL,2
	MOV	AH,3Dh
	INT	21h
	MOV	BX,AX
	JB	C_016D
	POP	CX
	POP	DS
	XOR	DX,DX
	OR	CX,CX
	JNZ	C_0141
	DEC	CX
C_0141:
	PUSH	CX
	CALL	LOAD_read
	POP	CX
	JB	C_015C
	CMP	AX,CX
	JB	C_015B
	CMP	CX,-01
	JNZ	C_015B
	PUSH	CX
	MOV	DX,CX
	MOV	CX,1
	CALL	LOAD_read
	POP	AX
C_015B:
	CLC
C_015C:
	PUSH	AX
	PUSHF
	MOV	AH,3Eh
	INT	21h
	POPF
	POP	AX
	POP	DS
	RET
;----
;DOS read (int 21h AH=3Fh), keeping BX
LOAD_read	PROC NEAR	;C_0166 (2674:0166)
	PUSH	BX
	MOV	AH,3Fh	;read file
	INT	21h
	POP	BX
	RET
LOAD_read	ENDP	;C_0166 (2674:0166)
;----
C_016D:
	POP	BX
	POP	BX
	POP	DS
	RET
LOAD_toSeg	ENDP
;----------------------------------------
EVEN
;----------------------------------------
PUBLIC my_alloc
my_alloc	PROC FAR	;2674:0172
	PUSH	BX
	MOV	AH,30h	;dos version
	INT	21h
	CMP	AL,3
	JZ	C_019E
	;-- --
	MOV	BX,0FFFFh
	MOV	AH,48h	;allocate memory
	INT	21h
	POP	AX
	SUB	BX,AX
	DEC	BX
	PUSH	AX
	MOV	AH,48h	;allocate memory
	INT	21h
	POP	BX
	PUSH	AX
	MOV	AH,48h	;allocate memory
	INT	21h
	POP	BX
	PUSH	AX
	PUSHF
	PUSH	ES
	MOV	ES,BX
	MOV	AH,49h	;free memory
	INT	21h
	POP	ES
	;-- --
	JMP	SHORT C_01B7
C_019E:
	;-- DOS 3.xx --
	MOV	AL,1
	MOV	BX,2
	MOV	AH,58h
	INT	21h
	POP	BX
	MOV	AH,48h	;allocate memory
	INT	21h
	PUSH	AX
	PUSHF
	MOV	AL,1
	MOV	BX,0
	MOV	AH,58h
	INT	21h
	;-- --
C_01B7:
	POPF
	POP	AX
	RET
my_alloc	ENDP
;----------------------------------------
CLEAR      EQU 256
EOF        EQU 257
FIRST_FREE EQU 258

read_code	MACRO
	LOCAL rd0,rd1,rd2
	PUSH	BP

	MOV	AX,bit_offset
	ADD	AX,nbits
	XCHG	AX,bit_offset
	MOV	CX,8
	XOR	DX,DX
	DIV	CX
	CMP	AX,1021
	JL	rd0
	PUSH	DX
	ADD	DX,nbits
	MOV	bit_offset,DX
	MOV	CX,1024
	MOV	BP,AX
	SUB	CX,AX
	PUSH	DS
	MOV	BX,input_handle
	MOV	DS,input_segment
	PUSH	DS
	POP	ES
	XOR	SI,SI
	MOV	DI,SI
	ADD	SI,AX
	REPZ	MOVSB
	MOV	CX,BP
	MOV	DX,DI
	MOV	AH,3Fh	;read file
	INT	21h
	POP	DS
	XOR	AX,AX
	POP	DX
rd0:
	PUSH	DS
	MOV	DS,input_segment
	MOV	SI,AX
	LODSW
	MOV	BX,AX
	LODSB
	MOV	CX,DX
	JCXZ	rd2
rd1:
	SHR	AL,1
	RCR	BX,1
	LOOP	rd1
rd2:
	POP	DS
	MOV	AX,BX
	MOV	BX,nbits
	SUB	BX,9
	SHL	BX,1
	AND	AX,masks[BX]
	POP	BP
ENDM

init_tab	MACRO
	MOV	nbits,9
	MOV	max_code,512
	MOV	free_code,FIRST_FREE
ENDM

write_char	MACRO
	LOCAL wd0
	PUSH	ES
	MOV	ES,output_segment
	MOV	DI,output_offset
	STOSB
	POP	ES
	INC	output_offset
	CMP	DI,10h
	JNZ	wd0
	INC	output_segment
	MOV	output_offset,0
wd0:
ENDM

index	MACRO
	PUSH	AX
	MOV	AX,BX
	SHL	BX,1
	ADD	BX,AX
	POP	AX
ENDM

add_code	MACRO
	MOV	BX,free_code
	index
	PUSH	ES
	MOV	ES,hash_seg
	MOV	AL,k
	MOV	ES:2[BX],AL
	MOV	AX,old_code
	MOV	ES:[BX],AX
	POP	ES
	INC	free_code
ENDM
;----------------------------------------
decompress	PROC FAR	;2674:01BA
	MOV	nbits,9
if 1
	MOV	output_offset,0
endif
	MOV	bit_offset,0
	MOV	free_code,FIRST_FREE
	MOV	max_code,512

	MOV	AX,segment_0
	MOV	hash_seg,AX

	MOV	BX,input_handle
	PUSH	DS
	MOV	DS,input_segment
	XOR	DX,DX
	MOV	CX,1024
	MOV	AH,3Fh	;read file
	INT	21h
	POP	DS

L1:	read_code
	CMP	AX,EOF
	JNE	L2
	;--
	MOV	CX,1800h
	XOR	AX,AX
	MOV	ES,hash_seg
	MOV	DI,AX
	REPZ	STOSW
	;--
	RET

L2:	CMP	AX,CLEAR
	JZ	C_0279
	JMP	L7
C_0279:

	init_tab
	read_code
	MOV	cur_code,AX
	MOV	old_code,AX
	MOV	k,AL
	MOV	fin_char,AL
	write_char
	JMP	L1

L7:	MOV	cur_code,AX
	MOV	in_code,AX
	MOV	ES,hash_seg
	CMP	AX,free_code
	JL	L11
	MOV	AX,old_code
	MOV	cur_code,AX
	MOV	AL,fin_char
	PUSH	AX
	INC	stack_count
L11:	CMP	cur_code,255
	JLE	L15

	MOV	BX,cur_code
	index
	MOV	AL,ES:2[BX]
	PUSH	AX
	INC	stack_count
	MOV	AX,ES:[BX]
	MOV	cur_code,AX
	JMP	L11

L15:
	PUSH	DS
	POP	ES
	MOV	AX,cur_code
	MOV	fin_char,AL
	MOV	k,AL
	PUSH	AX
	INC	stack_count
	MOV	CX,stack_count
	JCXZ	L18
L17:	POP	AX
	write_char
	LOOP	L17
L18:	MOV	stack_count,CX
	add_code
	MOV	AX,in_code
	MOV	old_code,AX
	MOV	BX,free_code
	CMP	BX,max_code
	JL	L23
	CMP	nbits,12
	JE	L23
	INC	nbits
	SHL	max_code,1
L23:	JMP	L1
decompress	ENDP

;load & inflate?
LZW_loadFile	PROC FAR	;2674:03E8
	MOV	output_segment,0
	MOV	pDest,0
	MOV	input_segment,0
	MOV	input_handle,0

	MOV	AL,2
	MOV	AH,3Dh
	INT	21h
	MOV	BX,AX
	JB	C_0463
	MOV	input_handle,BX
	;-- read Z-file header
	MOV	CX,4
	MOV	DX,OFFSET DGROUP:lDestSz
	MOV	AH,3Fh	;read file
	INT	21h
	JB	C_0463
	;--
	MOV	BX,WORD PTR lDestSz
	MOV	DX,WORD PTR lDestSz+2
	MOV	CL,4
C_0424:
	SHR	DX,1
	RCR	BX,1
	LOOP	C_0424
	INC	BX
	CALL	my_alloc
	JNB	C_043A

	MOV	output_segment,-1
	JMP	SHORT C_0463
C_043A:
	MOV	output_segment,AX
	MOV	pDest,AX
	MOV	BX,40h
	CALL	my_alloc
	JNB	C_0452
	MOV	output_segment,-1
	JMP	SHORT C_0463
C_0452:
	MOV	input_segment,AX
	CALL	decompress
	CLC
	MOV	AX,WORD PTR lDestSz
	OR	AX,AX
	JNZ	C_0463
	DEC	AX
C_0463:
	PUSHF
	PUSH	AX
if 1
	MOV	BX,input_segment
	OR	BX,BX
	JZ	C_0475
	PUSH	ES
	MOV	ES,BX
	MOV	AH,49h	;free memory
	INT	21h
	POP	ES
C_0475:
endif
	MOV	BX,input_handle
	JZ	C_047F
	MOV	AH,3Eh
	INT	21h
C_047F:
	MOV	BX,output_segment
	CMP	BX,-1
	JNZ	C_048E
	INC	BX
	POP	AX
	POPF
	JMP	C_00F4	;"out of memory"
C_048E:
	POP	AX
	POPF
	JNB	C_0497

	MOV	BX,1
	STC
	RET
C_0497:
	MOV	BX,pDest
	RET
LZW_loadFile	ENDP
;----------------------------------------
;CF set when the file name DS:DX (any case) is in CompressedFileList (main.c;
;empty: its only entry is "")
LOAD_isZFile	PROC NEAR	;C_049C (2674:049C)
	PUSH	BX
	PUSH	SI
	PUSH	DI
	;--
	MOV	BX,OFFSET DGROUP:CompressedFileList
C_04A2:	;--
	MOV	SI,[BX]
	CMP	BYTE PTR [SI],0
	JZ	C_04D5
	ADD	BX,2
	MOV	DI,DX
C_04AE:
	LODSB
	MOV	AH,[DI]
	CMP	AH,'a'
	JL	C_04BE
	CMP	AH,'z'
	JG	C_04BE
	SUB	AH,'a' - 'A'
C_04BE:
	CMP	AL,'a'
	JL	C_04C8
	CMP	AL,'z'
	JG	C_04C8
	SUB	AL,'a' - 'A'
C_04C8:
	CMP	AH,AL
	JNZ	C_04A2
	;--
	CMP	BYTE PTR [SI],0
	JZ	C_04D4
	INC	DI
	JMP	C_04AE
C_04D4:
	STC
C_04D5:
	POP	DI
	POP	SI
	POP	BX
	RET
LOAD_isZFile	ENDP	;C_049C (2674:049C)
;----------------------------------------
;load a file whole (DX: its name) into memory it allocates: BX the segment,
;CF set on an error (a compressed file in the list D_00AC is decompressed);
;called by the driver loader (DRVLOAD.ASM)
PUBLIC LOAD_file
LOAD_file	PROC FAR	;2674:04D9
	CALL	LOAD_isZFile	;is file name in list?
	JNB	C_04E3
	JMP	LZW_loadFile	;load & inflate?
C_04E3:	;--
	PUSH	DX
	CALL	LOAD_size	;get file size?
	JB	C_052C
	PUSH	AX
	OR	AX,AX
	JNZ	C_04F5
	OR	DX,DX
	JZ	C_052C
	DEC	AX
C_04F5:
	MOV	BX,AX
	OR	AX,AX
	JZ	C_0501
	DEC	BX
	MOV	CL,04
	SHR	BX,CL
	INC	BX
C_0501:
	CALL	my_alloc
	POP	BX
	JB	C_0525
	POP	DX
	PUSH	BX
	PUSH	AX
	CALL	LOAD_toSeg
	POP	BX
	POP	CX
	JNB	C_0521
	PUSH	AX
	PUSH	ES
	MOV	ES,BX
	MOV	AH,49h	;free memory
	INT	21h
	POP	ES
	POP	AX
	JMP	SHORT C_052D
C_0521:
	MOV	AX,CX
	CLC
	RET
C_0525:
	POP	BX
	XOR	BX,BX
	STC
	JMP	C_00F4	;"out of memory"
C_052C:
	POP	BX
C_052D:
	MOV	BX,1
	STC
	RET
LOAD_file	ENDP

CSegEnd	SEG_2674_TEXT

END
