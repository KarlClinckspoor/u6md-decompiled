;----------------------------------------
;OSI library (MARTIAN.EXE): text window, choosing the current one
;(not in SAVAGE.EXE, where the programs set pText themselves)
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN _pText:WORD	;struct tText *: the current text window
DSegEnd

CSegStart	TXTSEL_TEXT

;pascal TXT_select(struct tText *t): make t the current text window
FProc	TXT_select	;0EA7:0004
	PUSH	BP
	MOV	BP,SP
	MOV	AX,[BP+06]
	MOV	_pText,AX
	POP	BP
	RET	2
TXT_select	ENDP

CSegEnd	TXTSEL_TEXT

END
