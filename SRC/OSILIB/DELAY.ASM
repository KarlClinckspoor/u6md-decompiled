INCLUDE OSI.INC

EXTRN CpuSpeed:WORD	;computer speed

DSegStart	
	
delayTicks	dw 0	;D_2B12
delayWanted	dw 0	;D_2B14
delayOldInt1C	dw 0,0	;D_2B16 original int 1Ch

DSegEnd


CSegStart	_DELAY_TEXT

ProcStart	OSI_delay	;26C7:0002
	MOV	AX,[BP+06]
	MOV	delayWanted,AX
C_000E:	;--
	MOV	AX,delayTicks
	CMP	AX,delayWanted
	JB	C_000E
	;--
	MOV	delayTicks,0
ProcEndN	OSI_delay,2


NEW_INT_1C:	;26C7:0024
	STI
	PUSH	DS
	PUSH	AX
	PUSH	SI
	;--
	MOV	AX,DGROUP
	MOV	DS,AX
	MOV	SI,OFFSET DGROUP:delayTicks
	INC	WORD PTR [SI]
	;--
	POP	SI
	POP	AX
	POP	DS
	IRET

PUBLIC OSI_delay_init	;26C7:0036
OSI_delay_init	PROC FAR
	PUSH	SI
	PUSH	DI
	PUSH	DS
	;--
	MOV	delayTicks,0
	;--
	PUSH	ES
	MOV	AL,1Ch
	MOV	AH,35h
	INT	21h
	MOV	AX,ES
	POP	ES
	MOV	delayOldInt1C,BX
	MOV	delayOldInt1C+2,AX
	;--
	PUSH	DS
	PUSH	CS
	POP	DS
	LEA	DX,NEW_INT_1C
	MOV	AL,1Ch
	MOV	AH,25h
	INT	21h
	POP	DS
	;--
	POP	DS
	POP	DI
	POP	SI
	RET
OSI_delay_init	ENDP

PUBLIC OSI_delay_clean	;26C7:0062
OSI_delay_clean	PROC FAR
	PUSH	SI
	PUSH	DI
	PUSH	DS
	;--
	MOV	AX,delayOldInt1C+2
	MOV	DX,delayOldInt1C
	PUSH	DS
	MOV	DS,AX
	MOV	AL,1Ch
	MOV	AH,25h
	INT	21h
	POP	DS
	;--
	POP	DS
	POP	DI
	POP	SI
	RET
OSI_delay_clean	ENDP

CSegEnd	_DELAY_TEXT

END
