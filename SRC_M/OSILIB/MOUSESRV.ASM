;----------------------------------------
;OSI library (SAVAGE.EXE): mouse, following its moves
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN _tmDrvCode:DWORD	;int far *: while not 0 the pointer is left alone
EXTRN _superTmDrvCode:DWORD	;int far *: the same
EXTRN evtMouseX:WORD	;(events.asm) the mouse state at the last update: x
EXTRN evtMouseY:WORD	;  y
EXTRN evtButtonL:BYTE	;  buttons (left: bit 0)
EXTRN evtButtonR:BYTE	;  (right: bit 1)
EXTRN evtButtonM:BYTE	;  (middle: bit 2)
EXTRN curMouse:WORD	;struct tMouse *: the mouse
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
	MOV	BX,curMouse
	CMP	[BX].installed,AL
	JNZ	C_001E
	JMPN	C_00A0
C_001E:
	MOV	AX,3	;get the position and the buttons
	INT	33h
	SHR	CX,1
	MOV	AL,evtButtonL
	XOR	AL,BL
	AND	AL,1
	JNZ	C_004C
	MOV	AL,evtButtonR
	XOR	AL,BL
	AND	AL,2
	JNZ	C_004C
	MOV	AL,evtButtonM
	XOR	AL,BL
	AND	AL,4
	JNZ	C_004C
	CMP	evtMouseX,CX
	JNZ	C_004C
	CMP	evtMouseY,DX
	JZ	C_00A0
C_004C:
	MOV	evtMouseX,CX
	MOV	evtMouseY,DX
	MOV	evtButtonL,BL
	MOV	evtButtonR,BL
	MOV	evtButtonM,BL
	MOV	SI,curMouse
	MOV	[SI].posX,CX
	MOV	[SI].posY,DX
	MOV	[SI].buttonM,AL
	MOV	[SI].buttonL,BL
	AND	[SI].buttonL,1
	MOV	[SI].buttonR,BL
	AND	[SI].buttonR,2
	LES	DI,_tmDrvCode
	MOV	AX,ES:[DI]
	OR	AX,AX
	JNZ	C_009D
	LES	DI,_superTmDrvCode
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
