;----------------------------------------
;OSI library (SAVAGE.EXE): text window, copying a line of pixels
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN _pText:WORD	;struct tText *: the current text window
EXTRN _drvCopyLine:DWORD	;driver: copy a line of pixels
DSegEnd

CSegStart	TXTCOPY_TEXT

;copy line of pixels AX of the window to line BX (from its left to its right)
FProc	TXT_copyLine	;0EB9:0000
	PUSH	DX
	PUSH	ES
	PUSH	DI
	CLD
	MOV	SI,_pText
	MOV	DI,[SI].surface
	MOV	ES,[DI].segment_
	MOV	CX,[DI].right
	MOV	DX,[DI].left
	MOV	SI,[DI].scanlines
	MOV	DI,BX
	MOV	BX,AX
	CALL	_drvCopyLine
	POP	DI
	POP	ES
	POP	DX
	RET
TXT_copyLine	ENDP

CSegEnd	TXTCOPY_TEXT

END
