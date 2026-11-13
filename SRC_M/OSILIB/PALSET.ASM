;----------------------------------------
;OSI library (SAVAGE.EXE): setting the whole VGA palette
;(this version: the setting is a routine of its own, PAL_setRaw)
;----------------------------------------
INCLUDE OSI.INC

CSegStart	PALSET_TEXT

;pascal PAL_setAll(unsigned char far *pal): pal (256 colors, 8 bits a component) is
;turned into 6 bits a component, in place, and set
FProc	PAL_setAll	;100B:000A
	PUSH	BP
	MOV	BP,SP
	PUSH	DS
	PUSH	SI
	LDS	SI,[BP+06]
	MOV	CX,300h
C_0015:
	SHR	BYTE PTR [SI],1
	SHR	BYTE PTR [SI],1
	INC	SI
	LOOP	C_0015
	PUSH	WORD PTR [BP+08]
	PUSH	WORD PTR [BP+06]
	PUSH	CS
	CALL	NEAR PTR PAL_setRaw
	POP	SI
	POP	DS
	POP	BP
	RET	4
PAL_setAll	ENDP

;pascal PAL_setRaw(unsigned char far *pal): set the 256 DAC registers from pal
;(6 bits a component; int 10h AX 1012h)
FProc	PAL_setRaw	;100B:002C
	PUSH	BP
	MOV	BP,SP
	XOR	BX,BX
	MOV	CX,100h
	LES	DX,[BP+06]
	MOV	AX,1012h	;set a block of DAC registers
	INT	10h
	POP	BP
	RET	4
PAL_setRaw	ENDP

CSegEnd	PALSET_TEXT

END
