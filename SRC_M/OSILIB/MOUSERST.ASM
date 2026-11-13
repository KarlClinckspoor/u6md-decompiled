;----------------------------------------
;OSI library (SAVAGE.EXE): mouse, restoring the background under the pointer
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_191A:WORD	;struct tMouse *: the mouse
EXTRN D_1938:WORD	;where the pointer was drawn: x
EXTRN D_193A:WORD	;  y
EXTRN _D_0EA2:DWORD	;driver: restore the background (struct tSurface *s, void far *buf, int x, int y, void far *shapes, int index)
DSegEnd

CSegStart	MOUSERST_TEXT

;put back what the pointer covered
FProc	MOUSE_restoreBG	;0FDC:000A
	MOV	BX,D_191A
	PUSH	[BX].shapeIndex
	PUSH	WORD PTR [BX].shapes+2
	PUSH	WORD PTR [BX].shapes
	PUSH	D_193A
	PUSH	D_1938
	PUSH	WORD PTR [BX].saveBuf+2
	PUSH	WORD PTR [BX].saveBuf
	PUSH	[BX].mSurface
	CALL	_D_0EA2
	ADD	SP,10h
	RET
MOUSE_restoreBG	ENDP

CSegEnd	MOUSERST_TEXT

END
