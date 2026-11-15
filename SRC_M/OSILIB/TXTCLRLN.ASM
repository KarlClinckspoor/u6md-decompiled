;----------------------------------------
;OSI library (SAVAGE.EXE): text window, clearing a line of text
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN _pText:WORD	;struct tText *: the current text window
EXTRN _drvFillLine:DWORD	;driver: fill a line of pixels
EXTRN clearLineY:WORD	;the line of pixels being filled
EXTRN clearLinesLeft:WORD	;lines of pixels left
DSegEnd

CSegStart	TXTCLRLN_TEXT

;clear a line of text (font height lines of pixels) from (DX,AX) to the right of the
;window, in the background color; not below the window's bottom
FProc	TXT_clearLine	;1036:000A
	PUSH	BP
	PUSH	BX
	CLD
	PUSH	DX
	MOV	clearLineY,AX
	MOV	SI,_pText
	MOV	DI,[SI].surface
	MOV	CX,[DI].right
	MOV	BP,CX
	MOV	AL,[SI].bgColor
	LES	BX,[SI].font
	MOV	DX,ES:[BX]
	MOV	clearLinesLeft,DX
	ADD	DX,clearLineY
	CMP	DX,[DI].bottom
	JLE	C_0039
	SUB	DX,[DI].bottom	;no lines below the window (this version)
	SUB	clearLinesLeft,DX
C_0039:
	MOV	SI,[DI].scanlines
	MOV	ES,[DI].segment_
	POP	DX
C_003F:
	MOV	CX,BP
	MOV	BX,clearLineY
	CALL	_drvFillLine
	INC	clearLineY
	DEC	clearLinesLeft
	JNZ	C_003F
	POP	BX
	POP	BP
	RET
TXT_clearLine	ENDP

CSegEnd	TXTCLRLN_TEXT

END
