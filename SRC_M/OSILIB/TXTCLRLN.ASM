;----------------------------------------
;OSI library (SAVAGE.EXE): text window, clearing a line of text
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN _pText:WORD	;struct tText *: the current text window
EXTRN _D_1500:DWORD	;driver: fill a line of pixels
EXTRN D_1508:WORD	;the line of pixels being filled
EXTRN D_150A:WORD	;lines of pixels left
DSegEnd

CSegStart	TXTCLRLN_TEXT

;clear a line of text (font height lines of pixels) from (DX,AX) to the right of the
;window, in the background color; not below the window's bottom
FProc	TXT_clearLine	;1036:000A
	PUSH	BP
	PUSH	BX
	CLD
	PUSH	DX
	MOV	D_1508,AX
	MOV	SI,_pText
	MOV	DI,[SI].surface
	MOV	CX,[DI].right
	MOV	BP,CX
	MOV	AL,[SI].bgColor
	LES	BX,[SI].font
	MOV	DX,ES:[BX]
	MOV	D_150A,DX
	ADD	DX,D_1508
	CMP	DX,[DI].bottom
	JLE	C_0039
	SUB	DX,[DI].bottom	;no lines below the window (this version)
	SUB	D_150A,DX
C_0039:
	MOV	SI,[DI].scanlines
	MOV	ES,[DI].segment_
	POP	DX
C_003F:
	MOV	CX,BP
	MOV	BX,D_1508
	CALL	_D_1500
	INC	D_1508
	DEC	D_150A
	JNZ	C_003F
	POP	BX
	POP	BP
	RET
TXT_clearLine	ENDP

CSegEnd	TXTCLRLN_TEXT

END
