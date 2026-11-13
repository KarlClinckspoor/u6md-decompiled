;----------------------------------------
;OSI library (SAVAGE.EXE): the CRT status port
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_150E:WORD	;the CRT status port
DSegEnd

CSegStart	VIDPORT_TEXT

;D_150E = the display's input status register 1 (3BAh or 3DAh)
FProc	_VID_initPort	;0F29:000A
	PUSH	ES
	MOV	AX,40h
	MOV	ES,AX
	MOV	DX,ES:[63h]	;0040:0063 video controller base I/O port address
	ADD	DL,6	;input status register 1
	MOV	D_150E,DX
	POP	ES
	RET
_VID_initPort	ENDP

CSegEnd	VIDPORT_TEXT

END
