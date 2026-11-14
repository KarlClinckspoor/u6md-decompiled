;----------------------------------------
;OSI library: creating a file (pascal). New in Martian Dreams, in the manner of
;OSIOPEN.ASM's DOS_open: on a DOS error the code goes to osiError and
;osiErrorHandler is called; if it clears osiError, the call is retried.
;----------------------------------------

DGROUP	GROUP _DATA
_DATA	SEGMENT WORD PUBLIC 'DATA'
_DATA	ENDS

EXTRN _osiError:WORD,_osiErrorHandler:DWORD

OSICREAT_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:OSICREAT_TEXT,DS:DGROUP

;int pascal DOS_create(char far *name): create (or truncate) the file, with
;no attributes; returns its handle (-1, CF set, on an error not retried)
PUBLIC DOS_create
DOS_create	PROC FAR	;2A5B:0008
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI
L_0005:
	PUSH	DS
	LDS	DX,[BP+06]
	XOR	CX,CX
	MOV	AH,3Ch		;create file
	INT	21h
	POP	DS
	JNB	L_0025
	MOV	_osiError,AX
	CALL	_osiErrorHandler
	TEST	_osiError,0FFFFh
	JZ	L_0005
	MOV	AX,0FFFFh
	STC
L_0025:
	POP	DI
	POP	SI
	POP	BP
	RET	4
DOS_create	ENDP

OSICREAT_TEXT	ENDS
END
