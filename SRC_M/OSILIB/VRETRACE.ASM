;----------------------------------------
;OSI library (SAVAGE.EXE): waiting until the display is in its vertical retrace
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN crtStatusPort:WORD	;the CRT status port
DSegEnd

CSegStart	VRETRACE_TEXT

;wait until the display is in its vertical retrace
FProc	VID_inRetrace	;0F2B:000E
	MOV	DX,crtStatusPort
C_000C:
	IN	AL,DX
	TEST	AL,8
	JZ	C_000C
	RET
VID_inRetrace	ENDP

CSegEnd	VRETRACE_TEXT

END
