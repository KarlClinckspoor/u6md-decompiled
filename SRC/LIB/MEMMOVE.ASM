;----------------------------------------
;GAME.EXE segment 2706 (MARTIAN.EXE's module): copying between far buffers that may overlap (Savage Empire's MEMMOVE.ASM,
;SAVAGE.EXE 0CAC; this one does nothing for n = 0 and tests n < 1 unsigned). The original
;was assembled by an assembler that pads each forward conditional jump to 5 bytes (TASM's
;JUMPS): the NOPs after the jumps are written out.
;----------------------------------------

MEMMOVE_TEXT	SEGMENT PUBLIC WORD 'CODE'
	ASSUME CS:MEMMOVE_TEXT

backward	DB	0	;2706:0000	1 while copying downwards

;padding of a forward conditional jump
JPAD	MACRO
	db	90h,90h,90h
ENDM

;void pascal MemMove(void far *dst, void far *src, int n): copy n bytes from src to dst,
;downwards when dst is above src (the pointers are normalised first)
PUBLIC MemMove
MemMove	PROC FAR	;2706:0001
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI
	PUSH	DS
	PUSHF
	db	83h,7Eh,06h,00h	;CMP WORD PTR [BP+6],0 (n)
	JNZ	C_0017
	db	0E9h		;JMP C_00AD
	dw	C_00AD-$-2
C_0017:
	MOV	CS:backward,0
	LDS	SI,[BP+8]	;src
	MOV	AX,SI
	MOV	BX,DS
	MOV	CL,4
	SHR	AX,CL
	ADD	BX,AX
	MOV	DS,BX
	db	83h,0E6h,0Fh	;AND SI,0Fh (MASM 3.0 would not sign-extend)
	MOV	DX,BX
	LES	DI,[BP+0Ch]	;dst
	MOV	AX,DI
	MOV	BX,ES
	MOV	CL,4
	SHR	AX,CL
	ADD	BX,AX
	MOV	ES,BX
	db	83h,0E7h,0Fh	;AND DI,0Fh
	CLD
	MOV	CX,[BP+6]	;n
	CMP	CX,1
	JZ	C_00A1
	JPAD
	JB	C_00AD
	JPAD
	CMP	DX,BX
	JG	C_008B
	JPAD
	JL	C_007B
	JPAD
	MOV	AX,SI
	SUB	AX,DI
	CMP	AX,-1
	JZ	C_00A4
	JPAD
	CMP	AX,1
	JZ	C_00AB
	JPAD
	CMP	SI,DI
	JG	C_008B
	JPAD
C_007B:	;dst above src: downwards, by words from the last one
	ADD	SI,CX
	ADD	DI,CX
	SUB	SI,2
	SUB	DI,2
	STD
	INC	CS:backward
C_008B:
	SHR	CX,1
	REP	MOVSW
	JNB	C_00AD
	JPAD
	CMP	CS:backward,0
	JZ	C_00A1
	JPAD
	INC	SI
	INC	DI
C_00A1:
	MOVSB
	JMP	SHORT C_00AD
C_00A4:	;dst one byte above src: downwards, by bytes
	STD
	ADD	SI,CX
	ADD	DI,CX
	DEC	SI
	DEC	DI
C_00AB:	;dst one byte below src: upwards, by bytes
	REP	MOVSB
C_00AD:
	POPF
	POP	DS
	POP	DI
	POP	SI
	POP	BP
	RET	0Ah
MemMove	ENDP

MEMMOVE_TEXT	ENDS

END
