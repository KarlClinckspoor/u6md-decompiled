;----------------------------------------
;OSI library: the graphics driver's mode call (saving the BIOS video mode that
;OSILIB/VIDRESET.ASM restores) and line clipping against the driver's
;clipping rectangle (DRVLOAD.ASM's D_2A42-D_2A48), Cohen-Sutherland outcodes
;with midpoint subdivision. New in Martian Dreams; nothing in GAME.EXE calls
;these routines directly (no relocation points at segment 264A).
;The helpers are labels inside FAR PROCs: their RET assembles to RETF even
;where the code reaches them with a near CALL, as in the original.
;----------------------------------------
INCLUDE OSI.INC

EXTRN D_2A28:WORD,D_2A2A:WORD,D_2A2C:WORD	;DRVLOAD.ASM: the driver's entry and parameter block
EXTRN D_2A42:WORD,D_2A44:WORD,D_2A46:WORD,D_2A48:WORD	;DRVLOAD.ASM: clipping rectangle left, right, top, bottom
EXTRN D_2A7A:WORD	;VIDRESET.ASM: the video mode at the start (-1: not saved yet)

DSegStart

D_2A6C	dw 0	;midpoint subdivision: x step
D_2A6E	dw 0	;y step
D_2A70	dw 0	;the line: x1
D_2A72	dw 0	;y1
D_2A74	dw 0	;x2
D_2A76	dw 0	;y2
D_2A78	dw 0	;not used

DSegEnd

CSegStart	SEG_264A_TEXT

;void C_264A_0000(int mode) (pascal): if a driver is loaded, call its
;function 3 with AX = mode and BX = the parameter block; the first time with a
;mode other than 0 the BIOS video mode is saved first (D_2A7A)
ProcStart	C_264A_0000	;264A:0000
	CMP	D_2A2A,0
	JZ	C_0033
	MOV	AX,[BP+6]
	OR	AX,AX
	JZ	C_0026
	CMP	D_2A7A,-1
	JNZ	C_0026
	PUSH	AX
	MOV	AH,0Fh		;get video mode
	INT	10h
	XOR	AH,AH
	MOV	D_2A7A,AX
	POP	AX
C_0026:
	MOV	BX,OFFSET DGROUP:D_2A2C
	MOV	D_2A28,3
	CALL	DWORD PTR D_2A28
C_0033:
ProcEndN	C_264A_0000,2

;(AX, BX) = point: CF clear if it is inside the clipping rectangle
PUBLIC C_264A_003A
C_264A_003A	PROC FAR	;264A:003A
	CMP	AX,D_2A42
	JL	C_0054
	CMP	AX,D_2A44
	JG	C_0054
	CMP	BX,D_2A46
	JL	C_0054
	CMP	BX,D_2A48
	JG	C_0054
	CLC
	RET
C_0054:
	STC
	RET
C_264A_003A	ENDP

;(AX, BX)-(CX, DX) = rectangle: order the corners (AX <= CX, BX <= DX) and
;clamp them to the screen (0..319, 0..199)
PUBLIC C_264A_0056
C_264A_0056	PROC FAR	;264A:0056
	CMP	AX,CX
	JLE	C_005B
	XCHG	CX,AX
C_005B:
	CMP	BX,DX
	JLE	C_0061
	XCHG	BX,DX
C_0061:
	CMP	AX,0
	JGE	C_0069
	MOV	AX,0
C_0069:
	CMP	AX,13Fh
	JLE	C_0071
	MOV	AX,13Fh
C_0071:
	CMP	BX,0
	JGE	C_0079
	MOV	BX,0
C_0079:
	CMP	BX,0C7h
	JLE	C_0082
	MOV	BX,0C7h
C_0082:
	CMP	CX,0
	JGE	C_008A
	MOV	CX,0
C_008A:
	CMP	CX,13Fh
	JLE	C_0093
	MOV	CX,13Fh
C_0093:
	CMP	DX,0
	JGE	C_009B
	MOV	DX,0
C_009B:
	CMP	DX,0C7h
	JLE	C_00A4
	MOV	DX,0C7h
C_00A4:
	RET
C_264A_0056	ENDP

;(AX, BX)-(CX, DX) = line: clip it to the clipping rectangle. CF set when it
;is wholly outside; otherwise CF clear and AX, BX, CX, DX the clipped line
PUBLIC C_264A_00A5
C_264A_00A5	PROC FAR	;264A:00A5
	CALL	C_0192
	OR	SI,SI
	JNZ	C_00B2
	OR	DI,DI
	JNZ	C_00B2
	CLC			;both ends inside
	RET
C_00B2:
	TEST	SI,DI
	JZ	C_00B8
	STC			;both ends outside on the same side
	RET
C_00B8:
	MOV	D_2A70,AX
	MOV	D_2A72,BX
	MOV	D_2A74,CX
	MOV	D_2A76,DX
	PUSH	DI
	OR	SI,SI
	JZ	C_00E1
	XCHG	CX,AX		;the first end is outside: move it in
	XCHG	BX,DX
	CALL	C_0101
	MOV	D_2A70,AX
	MOV	D_2A72,BX
	MOV	CX,D_2A74
	MOV	DX,D_2A76
C_00E1:
	POP	DI
	OR	DI,DI
	JZ	C_00F0
	CALL	C_0101		;the second end is outside: move it in
	MOV	D_2A74,AX
	MOV	D_2A76,BX
C_00F0:
	MOV	AX,D_2A70
	MOV	BX,D_2A72
	MOV	CX,D_2A74
	MOV	DX,D_2A76
	CLC
	RET

;(CX, DX) inside, (AX, BX) outside: halve the segment until (AX, BX) is a
;point on the rectangle's border
C_0101:
	MOV	SI,AX
	MOV	DI,BX
	ADD	AX,CX
	ADD	BX,DX
	TEST	AX,8000h
	JS	C_0112
	SHR	AX,1
	JMP	SHORT C_011C
C_0112:
	PUSH	CX
	PUSH	DX
	MOV	CX,2
	CWD
	IDIV	CX
	POP	DX
	POP	CX
C_011C:
	TEST	BX,8000h
	JS	C_0126
	SHR	BX,1
	JMP	SHORT C_0132
C_0126:
	PUSH	CX
	PUSH	DX
	MOV	CX,2
	XCHG	BX,AX
	CWD
	IDIV	CX
	POP	DX
	POP	CX
	XCHG	BX,AX
C_0132:
	SUB	SI,AX
	SUB	DI,BX
	MOV	D_2A6C,SI
	MOV	D_2A6E,DI
	CALL	C_0192
	OR	SI,SI
	JZ	C_0153
	MOV	CX,AX		;the midpoint is outside: go on from it
	MOV	DX,BX
	ADD	AX,D_2A6C
	ADD	BX,D_2A6E
	JMP	C_0101
C_0153:
	CMP	AX,D_2A42
	JZ	C_015F
	CMP	AX,D_2A44
	JNZ	C_016B
C_015F:
	CMP	BX,D_2A46
	JZ	C_0191
	CMP	BX,D_2A48
	JZ	C_0191
C_016B:
	MOV	SI,D_2A6C
	MOV	DI,D_2A6E
	TEST	SI,8000h
	JNS	C_017B
	NEG	SI
C_017B:
	TEST	DI,8000h
	JNS	C_0183
	NEG	DI
C_0183:
	CMP	SI,DI
	JGE	C_0189
	MOV	SI,DI
C_0189:
	CMP	SI,1
	JL	C_0191
	JMP	C_0101
C_0191:
	RET

;the outcodes of (AX, BX) in SI and of (CX, DX) in DI: 1 left, 2 right,
;4 below, 8 above the clipping rectangle
C_0192:
	XOR	SI,SI
	MOV	DI,SI
	CMP	AX,D_2A42
	JGE	C_019F
	db	83h,0CEh,01h	;OR SI,1 (TASM's sign-extended imm8; MASM 3.0 writes 81h)
C_019F:
	CMP	CX,D_2A42
	JGE	C_01A8
	db	83h,0CFh,01h	;OR DI,1 (TASM's sign-extended imm8; MASM 3.0 writes 81h)
C_01A8:
	CMP	AX,D_2A44
	JLE	C_01B1
	db	83h,0CEh,02h	;OR SI,2 (TASM's sign-extended imm8; MASM 3.0 writes 81h)
C_01B1:
	CMP	CX,D_2A44
	JLE	C_01BA
	db	83h,0CFh,02h	;OR DI,2 (TASM's sign-extended imm8; MASM 3.0 writes 81h)
C_01BA:
	CMP	BX,D_2A48
	JLE	C_01C3
	db	83h,0CEh,04h	;OR SI,4 (TASM's sign-extended imm8; MASM 3.0 writes 81h)
C_01C3:
	CMP	DX,D_2A48
	JLE	C_01CC
	db	83h,0CFh,04h	;OR DI,4 (TASM's sign-extended imm8; MASM 3.0 writes 81h)
C_01CC:
	CMP	BX,D_2A46
	JGE	C_01D5
	db	83h,0CEh,08h	;OR SI,8 (TASM's sign-extended imm8; MASM 3.0 writes 81h)
C_01D5:
	CMP	DX,D_2A46
	JGE	C_01DE
	db	83h,0CFh,08h	;OR DI,8 (TASM's sign-extended imm8; MASM 3.0 writes 81h)
C_01DE:
	RET
C_264A_00A5	ENDP

CSegEnd	SEG_264A_TEXT

END
