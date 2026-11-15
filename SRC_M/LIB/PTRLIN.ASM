;----------------------------------------
;SAVAGE.EXE (and SE's GAME.EXE 2F2B): a far pointer to its linear address
;----------------------------------------

PTRLIN_TEXT	SEGMENT PUBLIC WORD 'CODE'
	ASSUME CS:PTRLIN_TEXT

;long pascal PtrToLinear(void far *p): segment*16 + offset (20 bits, in DX:AX)
PUBLIC PtrToLinear
PtrToLinear	PROC FAR	;0E40:000A
	PUSH	BP
	MOV	BP,SP
	MOV	AX,[BP+8]	;segment
	XOR	DX,DX
	SHL	AX,1
	RCL	DL,1
	SHL	AX,1
	RCL	DL,1
	SHL	AX,1
	RCL	DL,1
	SHL	AX,1
	RCL	DL,1
	ADD	AX,[BP+6]	;offset
	ADC	DL,DH
	POP	BP
	RET	4
PtrToLinear	ENDP

PTRLIN_TEXT	ENDS

END
