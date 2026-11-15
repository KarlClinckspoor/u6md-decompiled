;----------------------------------------
;OSI library (SAVAGE.EXE): mouse, drawing the pointer
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN curMouse:WORD	;struct tMouse *: the mouse
EXTRN mouseDrawX:WORD	;where the pointer is drawn: x
EXTRN mouseDrawY:WORD	;  y
EXTRN _drvDrawShape:DWORD	;driver: draw a shape (struct tSurface *s, int x, int y, void far *shapes, int index)
DSegEnd

EXTRN MOUSE_clamp:FAR	;MOUSE_clamp

CSegStart	MOUSEDRW_TEXT

;draw the pointer at the mouse position (kept in the surface)
FProc	MOUSE_draw	;0FE6:000C
	CALL	MOUSE_clamp
	MOV	BX,curMouse
	MOV	CX,[BX].posX
	MOV	DX,[BX].posY
	CMP	[BX].installed,0
	JZ	C_0025
	MOV	AX,4	;set the position
	SHL	CX,1
	INT	33h
	SHR	CX,1
C_0025:
	PUSH	[BX].shapeIndex
	PUSH	WORD PTR [BX].shapes+2
	PUSH	WORD PTR [BX].shapes
	MOV	mouseDrawY,DX
	PUSH	DX
	MOV	mouseDrawX,CX
	PUSH	CX
	PUSH	[BX].mSurface
	CALL	_drvDrawShape
	ADD	SP,0Ch
	RET
MOUSE_draw	ENDP

CSegEnd	MOUSEDRW_TEXT

END
