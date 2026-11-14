;----------------------------------------
;OSI library: reading a file (pascal) into memory that may be EMS (a far
;pointer with segment C000h-FFFFh is mapped in first, EMS_map).
;Savage Empire's OSIREAD.ASM (GAME.EXE 2E7E), changed: it saves BX and CX,
;returns the number of bytes read (a long, kept in osiBytesRead) and reads
;nothing for a size of 0. Assembled by TASM with JUMPS: a conditional jump
;that turned out short is followed by three NOPs, written out here.
;----------------------------------------

DGROUP	GROUP _DATA
_DATA	SEGMENT WORD PUBLIC 'DATA'
osiBytesRead	dd 0		;D_2EA8 bytes read by the current call
_DATA	ENDS

EXTRN _osiError:WORD,_osiErrorHandler:DWORD
EXTRN _EMS_map:FAR

OSIREAD_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:OSIREAD_TEXT,DS:DGROUP

;long pascal DOS_read(int handle, long pos, long size, void far *buf):
;seek to pos (unless pos's high word is -1), read size bytes to buf;
;returns the number of bytes read (0 with CF clear for a size of 0), or 0
;with CF set on an error not retried (osiError has the error)
PUBLIC DOS_read
DOS_read	PROC FAR	;2A5E:0004
	PUSH	BP
	MOV	BP,SP
	SUB	SP,4
	PUSH	SI
	PUSH	DI
	PUSH	BX
	PUSH	CX
	MOV	_osiError,0
	MOV	AX,[BP+0Ah]
	OR	AX,[BP+0Ch]
	CMP	AX,0
	JNZ	L_0024
	NOP
	NOP
	NOP
	CLC			;nothing to read
	XOR	DX,DX
	JMP	L_00E6
L_0024:
	XOR	AX,AX
	MOV	WORD PTR osiBytesRead+2,AX
	MOV	WORD PTR osiBytesRead,AX
	CMP	WORD PTR [BP+08],0
	JNZ	L_0035
	JMP	L_00BD
L_0035:
	PUSH	WORD PTR [BP+08]
	PUSH	WORD PTR [BP+06]
	CALL	_EMS_map		;map an EMS pointer
	ADD	SP,4
	MOV	[BP-02],DX
	MOV	[BP-04],AX
	MOV	BX,[BP+12h]
	MOV	DX,[BP+0Eh]
	MOV	CX,[BP+10h]
	CMP	CX,-1
	JZ	L_0064
	NOP
	NOP
	NOP
	MOV	AX,4200h	;move file pointer
	INT	21h
	JB	L_00BD
	NOP
	NOP
	NOP
L_0064:
	MOV	DI,[BP+0Ch]
	MOV	SI,[BP+0Ah]
L_006A:
	OR	DI,DI
	JNZ	L_0079
	NOP
	NOP
	NOP
	CMP	SI,-10h
	JBE	L_00AD
	NOP
	NOP
	NOP
L_0079:
	MOV	CX,0FFF0h
	PUSH	DS
	LDS	DX,[BP-04]
	MOV	AH,3Fh		;read file
	INT	21h
	POP	DS
	JB	L_00BD
	NOP
	NOP
	NOP
	ADD	WORD PTR osiBytesRead,AX
	ADC	WORD PTR osiBytesRead+2,0
	CMP	AX,0
	JZ	L_00D6
	NOP
	NOP
	NOP
	SUB	SI,AX
	SBB	DI,0
	SHR	AX,1
	SHR	AX,1
	SHR	AX,1
	SHR	AX,1
	ADD	[BP-02],AX
	JMP	SHORT L_006A
L_00AD:
	MOV	CX,SI
	PUSH	DS
	LDS	DX,[BP-04]
	MOV	AH,3Fh		;read file
	INT	21h
	POP	DS
	JNB	L_00D6
	NOP
	NOP
	NOP
L_00BD:
	MOV	_osiError,AX
	CALL	_osiErrorHandler
	TEST	_osiError,0FFFFh
	JNZ	L_00CF
	JMP	L_0024
L_00CF:
	XOR	AX,AX
	XOR	DX,DX
	STC
	JMP	SHORT L_00E6
L_00D6:
	ADD	WORD PTR osiBytesRead,AX
	ADC	WORD PTR osiBytesRead+2,0
	MOV	AX,WORD PTR osiBytesRead
	MOV	DX,WORD PTR osiBytesRead+2
L_00E6:
	POP	CX
	POP	BX
	POP	DI
	POP	SI
	MOV	SP,BP
	POP	BP
	RET	0Eh
DOS_read	ENDP

OSIREAD_TEXT	ENDS
END
