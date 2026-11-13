;----------------------------------------
;OSI library (SAVAGE.EXE): mouse, following its moves
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN _D_14F6:DWORD	;int far *: while not 0 the pointer is left alone
EXTRN _D_0E86:DWORD	;int far *: the same
EXTRN D_18D2:WORD	;(07D9) the mouse state at the last update: x
EXTRN D_18D4:WORD	;  y
EXTRN D_18D6:BYTE	;  buttons (left: bit 0)
EXTRN D_18D7:BYTE	;  (right: bit 1)
EXTRN D_18D8:BYTE	;  (middle: bit 2)
EXTRN D_191A:WORD	;struct tMouse *: the mouse
DSegEnd

EXTRN MOUSE_hide:FAR	;MOUSE_hide
EXTRN MOUSE_show:FAR	;MOUSE_show

CSegStart	MOUSESRV_TEXT

;update the mouse state, and call onMove (the pointer hidden) when it changed;
;returns 1 when it changed
FProc	MOUSE_serve	;0FCB:0002
	PUSH	SI
	PUSH	DI
	XOR	AX,AX
	MOV	BX,D_191A
	CMP	[BX].installed,AL
	JNZ	C_001E
	JMPN	C_00A0
C_001E:
	MOV	AX,3	;get the position and the buttons
	INT	33h
	SHR	CX,1
	MOV	AL,D_18D6
	XOR	AL,BL
	AND	AL,1
	JNZ	C_004C
	MOV	AL,D_18D7
	XOR	AL,BL
	AND	AL,2
	JNZ	C_004C
	MOV	AL,D_18D8
	XOR	AL,BL
	AND	AL,4
	JNZ	C_004C
	CMP	D_18D2,CX
	JNZ	C_004C
	CMP	D_18D4,DX
	JZ	C_00A0
C_004C:
	MOV	D_18D2,CX
	MOV	D_18D4,DX
	MOV	D_18D6,BL
	MOV	D_18D7,BL
	MOV	D_18D8,BL
	MOV	SI,D_191A
	MOV	[SI].posX,CX
	MOV	[SI].posY,DX
	MOV	[SI].buttonM,AL
	MOV	[SI].buttonL,BL
	AND	[SI].buttonL,1
	MOV	[SI].buttonR,BL
	AND	[SI].buttonR,2
	LES	DI,_D_14F6
	MOV	AX,ES:[DI]
	OR	AX,AX
	JNZ	C_009D
	LES	DI,_D_0E86
	MOV	AX,ES:[DI]
	OR	AX,AX
	JNZ	C_009D
	CALL	MOUSE_hide
	CALL	[SI].onMove
	CALL	MOUSE_show
C_009D:
	MOV	AX,1
C_00A0:
	POP	DI
	POP	SI
	RET
MOUSE_serve	ENDP

CSegEnd	MOUSESRV_TEXT

END
