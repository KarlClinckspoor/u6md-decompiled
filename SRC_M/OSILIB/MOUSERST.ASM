;----------------------------------------
;OSI library (SAVAGE.EXE): mouse, restoring the background under the pointer
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN curMouse:WORD	;struct tMouse *: the mouse
EXTRN mouseDrawX:WORD	;where the pointer was drawn: x
EXTRN mouseDrawY:WORD	;  y
EXTRN _drvRestoreBG:DWORD	;TM.DRV +12: restore the background (s, void far *buf, x, y, void far *shapes, n)
DSegEnd

CSegStart	MOUSERST_TEXT

;put back what the pointer covered
FProc	MOUSE_restoreBG	;0FDC:000A
	MOV	BX,curMouse
	PUSH	[BX].shapeIndex
	PUSH	WORD PTR [BX].shapes+2
	PUSH	WORD PTR [BX].shapes
	PUSH	mouseDrawY
	PUSH	mouseDrawX
	PUSH	WORD PTR [BX].saveBuf+2
	PUSH	WORD PTR [BX].saveBuf
	PUSH	[BX].mSurface
	CALL	_drvRestoreBG
	ADD	SP,10h
	RET
MOUSE_restoreBG	ENDP

CSegEnd	MOUSERST_TEXT

END
