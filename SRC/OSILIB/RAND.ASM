INCLUDE OSI.INC

DSegStart
randSeed1	dw 1	;D_2B1A random seed 1
randSeed2	dw 0	;D_2B1C random seed 2
DSegEnd

CSegStart	SEG_2B57_TEXT
	
RandNext	PROC FAR
	MOV	CX,randSeed1
	MOV	BX,CX
	MOV	AX,randSeed2
	ADD	CX,CX
	RCL	AX,1
	ADD	CX,BX
	ADC	AX,randSeed2
	ADD	AX,BX
	MOV	randSeed1,CX
	MOV	randSeed2,AX
	RET
RandNext	ENDP

PUBLIC OSI_rand
OSI_rand	PROC FAR	;26CE:0027
	PUSH	BP
	MOV	BP,SP
	;--
	CALL	RandNext
	;--
	MOV	BX,[BP+06]
	MOV	CX,[BP+08]
	CMP	BX,CX
	JNB	C_003D
	XCHG	BX,CX
C_003D:
	XOR	DX,DX
	SUB	BX,CX
	INC	BX
	JZ	C_0046
	DIV	BX
C_0046:
	ADD	DX,CX
	MOV	AX,DX
	POP	BP
	RET	4
OSI_rand	ENDP

PUBLIC OSI_randomize	;26CE:004C
OSI_randomize	PROC FAR	;26CE:004C
	PUSH	BP
	MOV	BP,SP
	;--
	MOV	AX,[BP+06]
	OR	AX,1
	MOV	randSeed1,AX
	;--
	POP	BP
	RET	2
OSI_randomize	ENDP

CSegEnd	SEG_2B57_TEXT


;(Ultima VI has OSI_timeSeed here, in a second code segment: Savage Empire left it out)

END
