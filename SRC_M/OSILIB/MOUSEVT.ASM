;----------------------------------------
;OSI library (SAVAGE.EXE): events, taking the next one from the queue
;(the queue and its variables are events.asm's)
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN evtCount:WORD	;the number of events in the queue
EXTRN evtHead:WORD	;where the next one is read
EXTRN evtBuffer:WORD	;the queue's first entry
EXTRN evtBufferLast:WORD	;its last entry
DSegEnd

EXTRN EvtCheckAltX:FAR	;(events.asm) update the queue
EXTRN EvtGetMousePos:FAR	;pascal (int *p): the mouse position to p[0], p[1]

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
	CALL	EvtCheckAltX
	PUSH	DS
	POP	ES
	MOV	AX,evtCount
	OR	AX,AX
	JNZ	C_002E
	MOV	BX,[BP+06]
	LEA	AX,[BX+0Ch]
	PUSH	AX
	CALL	EvtGetMousePos	;the mouse position at e+12
	XOR	AX,AX
	MOV	DI,[BP+06]
	MOV	ES:[DI],AX
	JMP	SHORT C_0053
C_002E:
	MOV	SI,evtHead
	MOV	AX,[SI]
	MOV	DI,[BP+06]
	MOV	CX,10h
	REP	MOVSB
	CMP	SI,evtBufferLast
	JLE	C_0046
	MOV	SI,evtBuffer
C_0046:
	MOV	evtHead,SI
	DEC	evtCount
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
