;----------------------------------------
;OSI library (SAVAGE.EXE): mouse, showing the pointer
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_18D9:WORD	;(07D9) the pointer is shown when 0, hidden when below
EXTRN _D_14F6:DWORD	;int far *: while not 0 the pointer is left alone
EXTRN _D_0E86:DWORD	;int far *: the same
DSegEnd

EXTRN MOUSE_saveBG:FAR	;MOUSE_saveBG
EXTRN MOUSE_draw:FAR	;MOUSE_draw

CSegStart	MOUSESHO_TEXT

busy	db 0	;0FD4:0008 1 while drawing

;show the pointer (once as many times as it was hidden)
FProc	MOUSE_show	;0FD4:0009
	PUSH	DI
	PUSH	ES
	INC	D_18D9
	JNZ	C_003F
	CMP	busy,0
	JNZ	C_003F
	INC	busy
	LES	DI,_D_14F6
	MOV	AX,ES:[DI]
	OR	AX,AX
	JNZ	C_003A
	LES	DI,_D_0E86
	MOV	AX,ES:[DI]
	OR	AX,AX
	JNZ	C_003A
	CALL	MOUSE_saveBG
	CALL	MOUSE_draw
C_003A:
	DEC	busy
C_003F:
	POP	ES
	POP	DI
	RET
MOUSE_show	ENDP

CSegEnd	MOUSESHO_TEXT

END
