;----------------------------------------
;OSI library (MARTIAN.EXE): text window, printing a character (Savage Empire's
;TXTCHAR.ASM, unchanged: U.EXE's TXTCHAR, newer: no variables, far calls within the
;module made PUSH CS / CALL near). The segment is WORD aligned: the byte before it
;(0EA9:0009) is the linker's pad after TXTGOTO.
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN _pText:WORD	;struct tText *: the current text window
EXTRN _drvPutChar:DWORD	;driver: draw a character
EXTRN charFitSlack:WORD	;added to the cursor's x to see whether a character fits
DSegEnd

EXTRN TXT_scrollUp1:FAR	;TXT_scrollUp1

CSegStart	TXTCHAR_TEXT

;pascal TXT_putc(int c): print a character
FProc	TXT_putc	;0EA9:000A
	PUSH	BP
	MOV	BP,SP
	MOV	AX,[BP+06]
	PUSH	SI
	PUSH	DI
	PUSH	DS
	PUSH	CS
	CALL	NEAR PTR TXT_putcAL
	POP	DS
	POP	DI
	POP	SI
	POP	BP
	RET	2
TXT_putc	ENDP

;print the character in AL at the cursor (13: carriage return, 10: new line), going to
;a new line when it does not fit
FProc	TXT_putcAL	;0EA9:001E
	XOR	AH,AH
	CMP	AL,0Dh
	JNZ	C_0026
	JMP	SHORT C_0074
C_0026:
	CMP	AL,0Ah
	JNZ	C_002C
	JMP	SHORT C_0081
C_002C:
	PUSH	AX
	MOV	BX,_pText
	LES	SI,[BX].font
	MOV	BP,AX
	MOV	CX,ES:[SI]	;height
	MOV	DL,ES:[BP+SI+04]	;width
	XOR	DH,DH
	CLD
C_0040:
	MOV	BX,_pText
	LES	SI,[BX].font
	MOV	AX,[BX].curX
	ADD	AX,charFitSlack
	DEC	AX
	MOV	SI,[BX].surface
	CMP	AX,[SI].right
	JLE	C_005C
	PUSH	CS
	CALL	NEAR PTR TXT_newLine
	JMP	SHORT C_0040
C_005C:
	MOV	AX,[BX].curY
	ADD	AX,CX
	CMP	AX,[SI].bottom
	JG	C_0072
	SUB	AX,CX
	SHL	AX,1
	MOV	SI,AX
	POP	AX
	CALL	_drvPutChar
	PUSH	AX
C_0072:
	POP	AX
	RET
TXT_putcAL	ENDP

;carriage return
FProc	TXT_cr	;0EA9:0074
C_0074:
	MOV	BX,_pText
	MOV	SI,[BX].surface
	MOV	AX,[SI].left
	MOV	[BX].curX,AX
	RET
TXT_cr	ENDP

;new line, scrolling the window up at its bottom
FProc	TXT_newLine	;0EA9:0081
C_0081:
	PUSH	ES
	PUSH	DI
	PUSH	SI
	PUSH	BX
	PUSH	AX
	PUSH	CS
	CALL	NEAR PTR TXT_cr
	LES	DI,[BX].font
	MOV	AX,[BX].curY
	ADD	AX,ES:[DI]
	CMP	AX,[SI].bottom
	JL	C_009F
	CALL	TXT_scrollUp1
	JMP	SHORT C_00A2
C_009F:
	MOV	[BX].curY,AX
C_00A2:
	POP	AX
	POP	BX
	POP	SI
	POP	DI
	POP	ES
	RET
TXT_newLine	ENDP

CSegEnd	TXTCHAR_TEXT

END
