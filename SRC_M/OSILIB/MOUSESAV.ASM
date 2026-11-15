;----------------------------------------
;OSI library (SAVAGE.EXE): mouse, saving the background under the pointer
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN curMouse:WORD	;struct tMouse *: the mouse
EXTRN mouseDrawX:WORD	;where the pointer is drawn: x
EXTRN mouseDrawY:WORD	;  y
EXTRN _drvSaveBG:DWORD	;TM.DRV +10: save the background (s, void far *buf, x, y, void far *shapes, n)
DSegEnd

CSegStart	MOUSESAV_TEXT

;save what the pointer will cover (at the mouse position, kept in the surface)
FProc	MOUSE_saveBG	;0FDF:0000
	PUSH	CS
	CALL	NEAR PTR MOUSE_clamp
	MOV	BX,curMouse
	PUSH	[BX].shapeIndex
	PUSH	WORD PTR [BX].shapes+2
	PUSH	WORD PTR [BX].shapes
	MOV	AX,[BX].posY
	MOV	mouseDrawY,AX
	PUSH	AX
	MOV	AX,[BX].posX
	MOV	mouseDrawX,AX
	PUSH	AX
	PUSH	WORD PTR [BX].saveBuf+2
	PUSH	WORD PTR [BX].saveBuf
	PUSH	[BX].mSurface
	CALL	_drvSaveBG
	ADD	SP,10h
	RET
MOUSE_saveBG	ENDP

;keep the mouse position inside its surface's window (and tell the driver)
FProc	MOUSE_clamp	;0FDF:002F
	PUSH	BX
	PUSH	CX
	PUSH	DX
	PUSH	AX
	MOV	BX,curMouse
	MOV	CX,[BX].posX
	MOV	DX,[BX].posY
	PUSH	BX
	MOV	BX,[BX].mSurface
	CMP	CX,[BX].left
	JGE	C_0056
	MOV	CX,[BX].left
	JMP	SHORT C_005E
C_0056:
	CMP	CX,[BX].right
	JLE	C_005E
	MOV	CX,[BX].right
C_005E:
	CMP	DX,[BX].top
	JGE	C_0068
	MOV	DX,[BX].top
	JMP	SHORT C_0070
C_0068:
	CMP	DX,[BX].bottom
	JLE	C_0070
	MOV	DX,[BX].bottom
C_0070:
	POP	BX
	MOV	[BX].posX,CX
	MOV	[BX].posY,DX
	CMP	[BX].installed,0
	JZ	C_0083
	MOV	AX,4	;set the position
	SHL	CX,1
	INT	33h
C_0083:
	POP	AX
	POP	DX
	POP	CX
	POP	BX
	RET
MOUSE_clamp	ENDP

CSegEnd	MOUSESAV_TEXT

END
