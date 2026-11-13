;----------------------------------------
;OSI library (SAVAGE.EXE): mouse, drawing the pointer
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_191A:WORD	;struct tMouse *: the mouse
EXTRN D_1938:WORD	;where the pointer is drawn: x
EXTRN D_193A:WORD	;  y
EXTRN _D_0E9A:DWORD	;driver: draw a shape (struct tSurface *s, int x, int y, void far *shapes, int index)
DSegEnd

EXTRN MOUSE_clamp:FAR	;MOUSE_clamp

CSegStart	MOUSEDRW_TEXT

;draw the pointer at the mouse position (kept in the surface)
FProc	MOUSE_draw	;0FE6:000C
	CALL	MOUSE_clamp
	MOV	BX,D_191A
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
	MOV	D_193A,DX
	PUSH	DX
	MOV	D_1938,CX
	PUSH	CX
	PUSH	[BX].mSurface
	CALL	_D_0E9A
	ADD	SP,0Ch
	RET
MOUSE_draw	ENDP

CSegEnd	MOUSEDRW_TEXT

END
