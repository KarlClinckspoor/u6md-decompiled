;----------------------------------------
;EMS memory: the block after a block of the EMS heap (Martian Dreams only;
;Savage Empire's EMS heap has no such routine). See EMSINIT.ASM and
;EMSALLOC.ASM for the heap's layout.
;----------------------------------------

DGROUP	GROUP _DATA
_DATA	SEGMENT WORD PUBLIC 'DATA'
_DATA	ENDS

EXTRN _emsHeapPages:WORD
EXTRN _EMS_map:FAR

EMSNEXT_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:EMSNEXT_TEXT,DS:DGROUP

;void far * pascal EMS_next(void far *p): the virtual pointer of the
;block after block p (p + its length + 4, carried into the page number);
;NULL past the end of the EMS heap
PUBLIC EMS_next
EMS_next	PROC FAR	;291E:0006
	PUSH	BP
	MOV	BP,SP
	SUB	SP,4
	PUSH	ES
	PUSH	SI
	PUSH	DI
	PUSH	BX
	PUSH	CX
	PUSH	WORD PTR [BP+08]
	PUSH	WORD PTR [BP+06]
	CALL	_EMS_map
	ADD	SP,4
	MOV	ES,DX
	MOV	BX,AX
	MOV	AX,ES:[BX-04]		;the block's length
	MOV	DX,ES:[BX-02]
	AND	AL,0FEh			;without the in-use bit
	ADD	AX,4
	ADC	DX,0
	ADD	AX,[BP+06]
	ADC	DX,0
	MOV	BH,AH			;offset bits 14-15 go to the page
	AND	AH,3Fh
	SHL	BH,1
	RCL	DX,1
	SHL	BH,1
	RCL	DX,1
	ADD	DX,[BP+08]
	MOV	BX,DX
	AND	BH,3Fh
	CMP	BX,_emsHeapPages
	JB	L_005E
	XOR	DX,DX
	XOR	AX,AX
L_005E:
	POP	CX
	POP	BX
	POP	DI
	POP	SI
	POP	ES
	MOV	SP,BP
	POP	BP
	RET	4
EMS_next	ENDP

EMSNEXT_TEXT	ENDS
END
