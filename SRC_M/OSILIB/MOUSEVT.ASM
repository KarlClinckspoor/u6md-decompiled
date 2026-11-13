;----------------------------------------
;OSI library (SAVAGE.EXE): events, taking the next one from the queue
;(the queue and its variables are 07D9's)
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_18DC:WORD	;the number of events in the queue
EXTRN D_190E:WORD	;where the next one is read
EXTRN D_1912:WORD	;the queue's first entry
EXTRN D_1914:WORD	;its last entry
DSegEnd

EXTRN C_0F2C_07AD:FAR	;(07D9) update the queue
EXTRN C_0F2C_08E9:FAR	;pascal (int *p): the mouse position to p[0], p[1]

CSegStart	MOUSEVT_TEXT

;pascal EVT_get(char *e): copy the next event (16 bytes, its first word its type) to e
;and take it from the queue; events of type -1 are skipped; with none, e's type is 0
;and the mouse position goes to e+12 (this version)
FProc	EVT_get	;0FBD:0004
	PUSH	BP
	MOV	BP,SP
	PUSH	ES
	PUSH	SI
	PUSH	DI
C_000A:
	CALL	C_0F2C_07AD
	PUSH	DS
	POP	ES
	MOV	AX,D_18DC
	OR	AX,AX
	JNZ	C_002E
	MOV	BX,[BP+06]
	LEA	AX,[BX+0Ch]
	PUSH	AX
	CALL	C_0F2C_08E9	;the mouse position at e+12
	XOR	AX,AX
	MOV	DI,[BP+06]
	MOV	ES:[DI],AX
	JMP	SHORT C_0053
C_002E:
	MOV	SI,D_190E
	MOV	AX,[SI]
	MOV	DI,[BP+06]
	MOV	CX,10h
	REP	MOVSB
	CMP	SI,D_1914
	JLE	C_0046
	MOV	SI,D_1912
C_0046:
	MOV	D_190E,SI
	DEC	D_18DC
	CMP	AX,0FFFFh
	JZ	C_000A
C_0053:
	POP	DI
	POP	SI
	POP	ES
	POP	BP
	RET	2
EVT_get	ENDP

CSegEnd	MOUSEVT_TEXT

END
