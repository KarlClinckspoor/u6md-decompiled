INCLUDE OSI.INC

tSurface	STRUC
	segment_	dw ?
	pScanlines	dd (?)
	right	dw ?	;f_06
	left	dw ?	;f_08
	top	dw ?	;f_0a
	bottom	dw ?	;f_0c
tSurface	ENDS

tMouse	STRUC
	buttonL	db ?	;button1
	buttonR	db ?	;button2
	posX	dw ?	;x
	posY	dw ?	;y
	unused06	dw ?	;
	shapes	dd ?	;graphic data?
	shapeIndex	dw ?	;pointer index?
	surface	dw ?	;graphic structure?[struct tSurface *]
	saveBuf	dd ?	;work memory?0x400 bytes
	onMove	dd ?	;a callback
	installed	db ?	;"is installed" flag?
tMouse	ENDS

;--a far call within the segment: TASM made it PUSH CS / near CALL / NOP
CALL_FAR_PTR	MACRO a
	PUSH	CS
	CALL	NEAR PTR a
	NOP
ENDM
;----

DSegStart

EXTRN GrDriver:DWORD	;the graphic driver's entry point (uninitialised data, BSS.ASM)

_pMouse	dw 0	;D_1412
_nMouseSem	dw 0	;D_1414
__2FB8	db 0,0	;??
mousePosX	dw 0	;D_1418 pos x
mousePosY	dw 0	;D_141A pos y
mouseButtonL	dw 0	;D_141C L button
mouseButtonR	dw 0	;D_141E R button

DSegEnd

CSegStart	MOUSE_TEXT

;----------------------------------------
;
;----------------------------------------
;init mouse?
ProcStart	_MOUSE_INIT	;16D7:000A
	MOV	BX,[BP+06]
	MOV	_pMouse,BX
	;%%
	MOV	[BX].tMouse.buttonL,0
	MOV	[BX].tMouse.buttonR,0
	MOV	[BX].tMouse.posX,0
	MOV	[BX].tMouse.posY,0
	MOV	WORD PTR [BX].tMouse.onMove,OFFSET MOUSE_TEXT:_MOUSE_0055
	MOV	WORD PTR [BX].tMouse.onMove+2,MOUSE_TEXT
	MOV	[BX].tMouse.installed,0
	;--
	MOV	AX,0	;mouse reset/get mouse installed flag
	INT	33h
	CMP	AX,0
	JZ	C_0050
	;-- is installed
	PUSH	BX	;push # of buttons
	;--
	MOV	BX,_pMouse
	MOV	[BX].tMouse.installed,1
	;--
	MOV	_nMouseSem,-1
	CALL_FAR_PTR _MOUSE_0124
	CALL_FAR_PTR _MOUSE_SERVE
	;-- returns # of buttons
	POP	AX
C_0050:	;--
ProcEnd	_MOUSE_INIT

;dummy callback
PUBLIC _MOUSE_0055	;16D7:005F
_MOUSE_0055	PROC FAR
	RET
_MOUSE_0055	ENDP

ProcStart	_MOUSE_POINTER	;16D7:0060	;MOUSE_0056
	LES	DI,[BP+06]
	MOV	AX,[BP+0Ah]
	;--
	MOV	BX,_pMouse
	CMP	[BX].tMouse.installed,0
	JZ	C_009A
	;-- compare bp0a to f_0c and bp06 to f_08
	CMP	AX,[BX].tMouse.shapeIndex
	JNZ	C_007D
	MOV	AX,ES
	CMP	AX,WORD PTR [BX].tMouse.shapes+2
	JNZ	C_007D
	CMP	DI,WORD PTR [BX].tMouse.shapes
	JZ	C_009A
C_007D:	;--
	CALL_FAR_PTR _MOUSE_HIDE
	;--
	LES	DI,[BP+06]
	MOV	AX,WORD PTR [BP].tMouse.shapes+2
	MOV	BX,_pMouse
	MOV	[BX].tMouse.shapeIndex,AX
	MOV	WORD PTR [BX].tMouse.shapes+2,ES
	MOV	WORD PTR [BX].tMouse.shapes,DI
	;--
	CALL_FAR_PTR _MOUSE_SHOW
C_009A:	;--
ProcEnd	_MOUSE_POINTER
;----------------------------------------
;
;----------------------------------------MOUSE_009F
;update mouse state
ProcStart _MOUSE_SERVE	;16D7:00A9
	MOV	BX,_pMouse
	CMP	[BX].tMouse.installed,0
	JZ	C_00D4
	;--
	MOV	AX,3	;get mouse position and buttons status
	INT	33h
	SHR	CX,1	;horizontal pos / 2
	;--
	MOV	AX,mouseButtonL
	XOR	AL,BL
	AND	AL,1
	JNZ	C_00D9
	;--
	MOV	AX,mouseButtonR
	XOR	AL,BL
	AND	AL,2
	JNZ	C_00D9
	;--
	CMP	mousePosX,CX
	JNZ	C_00D9
	;--
	CMP	mousePosY,DX
	JNZ	C_00D9
C_00D4:	;-- mouse state didn't change
	MOV	AX,0
	JMP	SHORT C_011F
C_00D9:	;--
if 0
	MOV	AX,_nMouseSem
	PUSH	AX
endif
	PUSH	BX
	PUSH	CX
	PUSH	DX
if 0
	OR	AX,AX
	JNZ	C_dummy
endif	
	CALL_FAR_PTR _MOUSE_HIDE
if 0
C_dummy:
endif
	POP	DX
	POP	CX
	POP	BX
	;--
	MOV	mousePosX,CX	;store pos x
	MOV	mousePosY,DX	;store pos y
	MOV	mouseButtonL,BX
	db	83h,26h	;AND D_141C,1 (TASM's sign-extended imm8; MASM 3.0 writes 81h)
	dw	OFFSET DGROUP:mouseButtonL
	db	1	;store L button status
	MOV	mouseButtonR,BX
	db	83h,26h	;AND D_141E,2 (TASM's sign-extended imm8; MASM 3.0 writes 81h)
	dw	OFFSET DGROUP:mouseButtonR
	db	2	;store R button status
	;--
	MOV	SI,_pMouse
	MOV	[SI].tMouse.posX,CX
	MOV	[SI].tMouse.posY,DX
	MOV	[SI].tMouse.buttonL,BL
	AND	[SI].tMouse.buttonL,1
	MOV	[SI].tMouse.buttonR,BL
	AND	[SI].tMouse.buttonR,2
	;--
	CALL	DWORD PTR [SI].tMouse.onMove
if 0
	POP	AX
	OR	AX,AX
	JNZ	C_dummy2
endif
	CALL_FAR_PTR _MOUSE_SHOW
if 0
C_dummy2:
endif
	MOV	AX,1
C_011F:	;--
ProcEnd	_MOUSE_SERVE


;force next refresh/reset refresh
ProcStart	_MOUSE_0124	;16D7:012E
	MOV	mousePosX,8001h
ProcEnd	_MOUSE_0124
;----------------------------------------
;
;----------------------------------------MOUSE_0135
PUBLIC _MOUSE_SHOW	;16D7:013F
_MOUSE_SHOW	PROC FAR
	INC	_nMouseSem
	JNZ	C_0145
	;--
	CALL_FAR_PTR _MOUSE_SAVE_BG	;call graphic function 0Ch
	CALL_FAR_PTR _MOUSE_DRAW	;call graphic function 30h
C_0145:	;--
	RET
_MOUSE_SHOW	ENDP
;----------------------------------------MOUSE_0146(HIDE_MOUSE?)
PUBLIC _MOUSE_HIDE	;16D7:0150
_MOUSE_HIDE	PROC FAR
	DEC	_nMouseSem
	CMP	_nMouseSem,-1
	JNZ	C_015B
	;--
	CALL_FAR_PTR _MOUSE_01DD	;wait on some port?
	CALL_FAR_PTR _MOUSE_01B0	;call graphic function 0Fh
C_015B:	;--
	RET
_MOUSE_HIDE	ENDP
;----------------------------------------
;call graphic function 30h
PUBLIC _MOUSE_DRAW	;16D7:0166
_MOUSE_DRAW	PROC FAR	;MOUSE_015C
	MOV	BX,_pMouse
	CMP	[BX].tMouse.installed,0
	JZ	C_0182
	;--
	PUSH	[BX].tMouse.surface
	PUSH	[BX].tMouse.posX
	PUSH	[BX].tMouse.posY
	PUSH	WORD PTR [BX].tMouse.shapes+2
	PUSH	WORD PTR [BX].tMouse.shapes
	PUSH	[BX].tMouse.shapeIndex
	MOV	WORD PTR [GrDriver],30h
	CALL	DWORD PTR [GrDriver]
C_0182:	;--
	RET
_MOUSE_DRAW	ENDP	;MOUSE_015C
;----------------------------------------
;call graphic function 0Ch
PUBLIC _MOUSE_SAVE_BG	;16D7:018D
_MOUSE_SAVE_BG	PROC FAR	;MOUSE_0183
	MOV	BX,_pMouse
	CMP	[BX].tMouse.installed,0
	JZ	C_01AF
	;--
	PUSH	[BX].tMouse.surface
	PUSH	WORD PTR [BX].tMouse.saveBuf+2
	PUSH	WORD PTR [BX].tMouse.saveBuf
	PUSH	[BX].tMouse.posX
	PUSH	[BX].tMouse.posY
	PUSH	WORD PTR [BX].tMouse.shapes+2
	PUSH	WORD PTR [BX].tMouse.shapes
	PUSH	[BX].tMouse.shapeIndex
	MOV	WORD PTR [GrDriver],0Ch
	CALL	DWORD PTR [GrDriver]
C_01AF:	;--
	RET
_MOUSE_SAVE_BG	ENDP	;MOUSE_0183
;----------------------------------------
;call graphic function 0Fh
PUBLIC _MOUSE_01B0	;16D7:01BA
_MOUSE_01B0	PROC FAR
	MOV	BX,_pMouse
	CMP	[BX].tMouse.installed,0
	JZ	C_01DC
	;--
	PUSH	[BX].tMouse.surface
	PUSH	WORD PTR [BX].tMouse.saveBuf+2
	PUSH	WORD PTR [BX].tMouse.saveBuf
	PUSH	[BX].tMouse.posX
	PUSH	[BX].tMouse.posY
	PUSH	WORD PTR [BX].tMouse.shapes+2
	PUSH	WORD PTR [BX].tMouse.shapes
	PUSH	[BX].tMouse.shapeIndex
	MOV	WORD PTR [GrDriver],0Fh
	CALL	DWORD PTR [GrDriver]
C_01DC:	;--
	RET
_MOUSE_01B0	ENDP
;----------------------------------------
;wait on some port?
PUBLIC _MOUSE_01DD	;16D7:01E7
_MOUSE_01DD	PROC FAR
	MOV	AX,40h
	MOV	ES,AX
	MOV	DX,ES:[63h]	;0040:0063 Video controller base I/O port address
	ADD	DL,6
C_01EA:
	IN	AL,DX
	TEST	AL,8
	JZ	C_01EA
	;--
	RET
_MOUSE_01DD	ENDP
;----------------------------------------
;
;----------------------------------------MOUSE_01F0
ProcStart	_MOUSE_IN_AREA	;16D7:01FA
	PUSH	ES
	;--
	MOV	SI,_pMouse
	MOV	AH,0
	MOV	AL,[SI].tMouse.installed
	CMP	AX,0
	JNZ	C_0208
	JMP	C_0291
C_0208:	;--
	MOV	ES,WORD PTR [SI].tMouse.shapes+2
	MOV	DI,WORD PTR [SI].tMouse.shapes
	MOV	BX,[SI].tMouse.shapeIndex
	;-- ES:DI += offset
	MOV	AX,ES
	MOV	DX,0
	SHL	AX,1
	RCL	DX,1
	SHL	AX,1
	RCL	DX,1
	SHL	AX,1
	RCL	DX,1
	SHL	AX,1
	RCL	DX,1
	ADD	AX,DI
	ADC	DX,0
	SHL	BX,1
	SHL	BX,1
	ADD	BX,4
	ADD	AX,ES:[BX+DI]
	ADC	DX,ES:[BX+DI+02]
	MOV	DI,AX
	db	83h,0E7h,0Fh	;AND DI,0Fh (TASM's sign-extended imm8; MASM 3.0 writes 81h)
	SHR	DX,1
	RCR	AX,1
	SHR	DX,1
	RCR	AX,1
	SHR	DX,1
	RCR	AX,1
	SHR	DX,1
	RCR	AX,1
	MOV	ES,AX
	;--
	MOV	BX,[BP+06]
	MOV	AX,0
	MOV	CX,_nMouseSem
	CMP	CX,0
	JL	C_0291
	;--
	MOV	CX,[SI].tMouse.posX
	SUB	CX,ES:[DI+02]
	CMP	CX,[BX].tSurface.right
	JG	C_0291
	;--
	MOV	CX,[SI].tMouse.posX
	ADD	CX,ES:[DI]
	CMP	CX,[BX].tSurface.left
	JL	C_0291
	;--
	MOV	CX,[SI].tMouse.posY
	SUB	CX,ES:[DI+04]
	CMP	CX,[BX].tSurface.bottom
	JG	C_0291
	;--
	MOV	CX,[SI].tMouse.posY
	ADD	CX,ES:[DI+06]
	CMP	CX,[BX].tSurface.top
	JL	C_0291
	;--
	MOV	AX,1
C_0291:	;--
	POP	ES
ProcEnd	_MOUSE_IN_AREA



ProcStart	_MOUSE_0297	;16D7:02A1
	MOV	BX,[BP+06]
	;--
	MOV	SI,_pMouse
	MOV	AX,0
	CMP	BYTE PTR [SI].tMouse.installed,0
	JZ	C_02CA
	;--
	MOV	DX,[SI].tMouse.posX
	CMP	DX,[BX].tSurface.right
	JG	C_02CA
	CMP	DX,[BX].tSurface.left
	JL	C_02CA
	
	MOV	DX,[SI].tMouse.posY
	CMP	DX,[BX].tSurface.top
	JL	C_02CA
	CMP	DX,[BX].tSurface.bottom
	JG	C_02CA
	;--
	MOV	AX,1
C_02CA:	;--
ProcEnd	_MOUSE_0297


ProcStart	_MOUSE_02CF	;16D7:02D9
	CLD
	LES	DI,[BP+08]
	MOV	BX,[BP+0Ch]
	;-- ES:DI += offset
	MOV	AX,ES
	MOV	DX,0
	SHL	AX,1
	RCL	DX,1
	SHL	AX,1
	RCL	DX,1
	SHL	AX,1
	RCL	DX,1
	SHL	AX,1
	RCL	DX,1
	ADD	AX,DI
	ADC	DX,0
	SHL	BX,1
	SHL	BX,1
	ADD	BX,4
	ADD	AX,ES:[BX+DI]
	ADC	DX,ES:[BX+DI+02]
	MOV	DI,AX
	db	83h,0E7h,0Fh	;AND DI,0Fh (TASM's sign-extended imm8; MASM 3.0 writes 81h)
	SHR	DX,1
	RCR	AX,1
	SHR	DX,1
	RCR	AX,1
	SHR	DX,1
	RCR	AX,1
	SHR	DX,1
	RCR	AX,1
	MOV	ES,AX
	;--
	MOV	BX,[BP+06]
	MOV	AX,0
	MOV	CX,[BP+0Eh]
	SUB	CX,ES:[DI+02]
	CMP	CX,[BX+08]
	JL	C_0353
	;--
	MOV	CX,[BP+0Eh]
	ADD	CX,ES:[DI]
	CMP	CX,[BX+06]
	JG	C_0353
	;--
	MOV	CX,[BP+10h]
	SUB	CX,ES:[DI+04]
	CMP	CX,[BX+08+2]
	JL	C_0353
	;--
	MOV	CX,[BP+10h]
	ADD	CX,ES:[DI+06]
	CMP	CX,[BX+0Ch]
	JG	C_0353
	;--
	MOV	AX,1
C_0353:	;--
ProcEnd	_MOUSE_02CF

CSegEnd	MOUSE_TEXT

END
