;----------------------------------------
;OSI library: going back to the video mode the program started in (the BIOS
;mode that module 264A saves here before it sets the first graphics mode).
;New in Martian Dreams, in the place of Savage Empire's OSI_getDrive (LOW.ASM's
;second code segment).
;----------------------------------------
INCLUDE OSI.INC

DSegStart

PUBLIC D_2A7A
D_2A7A	dw -1	;the video mode at the start (-1: not saved yet)

DSegEnd

CSegStart	SEG_2668_TEXT

;void OSI_resetVideo(void): set the saved video mode, if one was saved
ProcStart	OSI_resetVideo	;2668:0000
	MOV	BX,D_2A7A
	CMP	BX,-1
	JZ	C_0015
	MOV	AH,0		;set video mode
	MOV	AL,BL
	INT	10h
C_0015:
ProcEnd	OSI_resetVideo

CSegEnd	SEG_2668_TEXT

;OSIWRITE.ASM's code segment, named here first: TLINK puts a segment where the link
;first names it, so OSIWRITE's code, linked after OSIREAD.ASM (its relocation follows
;OSIREAD's in the .EXE's table), comes right after this module's, at 2669:A. How
;Origin's link got it here is not known.
OSIWRITE_TEXT	SEGMENT WORD PUBLIC 'CODE'
OSIWRITE_TEXT	ENDS

END
