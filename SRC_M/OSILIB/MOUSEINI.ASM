;----------------------------------------
;OSI library (SAVAGE.EXE): mouse, initialisation
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN _timerRate:WORD	;(saved and given back to C_0CED_00A5)
EXTRN D_18D2:WORD	;(07D9) the mouse's x at the last update
EXTRN D_191A:WORD	;struct tMouse *: the mouse
DSegEnd

EXTRN _SetTimerRate:FAR	;cdecl (int): sets D_2458
EXTRN C_0F2C_0030:FAR	;(07D9) the default onMove: does nothing
EXTRN MOUSE_saveBG:FAR	;MOUSE_saveBG
EXTRN _VID_initPort:FAR	;VID_initPort
EXTRN MOUSE_serve:FAR	;MOUSE_serve

CSegStart	MOUSEINI_TEXT

;pascal int MOUSE_init(struct tMouse *m, int serve): initialise the mouse (position 0,0);
;serve 1: update it at once. Returns the number of buttons (garbage without a mouse)
FProc	MOUSE_init	;0FC2:000A
	PUSH	BP
	MOV	BP,SP
	SUB	SP,2
	PUSH	SI
	PUSH	DI
	MOV	AX,_timerRate
	MOV	[BP-02],AX
	XOR	AX,AX
	PUSH	AX
	CALL	_SetTimerRate
	POP	AX
	MOV	BX,[BP+08]
	XOR	AX,AX
	MOV	[BX].buttonL,AL
	MOV	[BX].buttonR,AL
	MOV	[BX].buttonM,AL
	MOV	[BX].posX,AX
	MOV	[BX].posY,AX
	MOV	[BX].installed,AL
	MOV	WORD PTR [BX].onMove,OFFSET C_0F2C_0030
	MOV	WORD PTR [BX].onMove+2,SEG C_0F2C_0030
	MOV	CX,BX
	MOV	D_191A,BX
	INT	33h	;AX 0: reset, get the number of buttons
	OR	AX,AX
	JZ	C_007B
	MOV	DX,BX
	MOV	BX,CX
	MOV	[BX].status,AX
	MOV	[BX].installed,DL
	PUSH	DX
	MOV	CX,0
	MOV	DX,0
	MOV	AX,4	;set the position
	INT	33h
	CALL	MOUSE_saveBG
	CALL	_VID_initPort
	CMP	WORD PTR [BP+06],1
	JNZ	C_007A
	MOV	D_18D2,0FFFFh
	CALL	MOUSE_serve
C_007A:
	POP	AX
C_007B:
	MOV	AX,[BP-02]
	PUSH	AX
	CALL	_SetTimerRate
	POP	AX
	POP	DI
	POP	SI
	MOV	SP,BP
	POP	BP
	RET	4
MOUSE_init	ENDP

CSegEnd	MOUSEINI_TEXT

END
