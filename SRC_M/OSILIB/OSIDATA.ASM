;----------------------------------------
;OSI library (MARTIAN.EXE (Savage Empire's, addresses moved)): the library's shared variables (a module with data only)
;Which module of Origin's library held these is not known: they come after the shape
;routines (0E7C) and before TXTJUST (0E7E) in the data segment, and their relocations
;sit between those two modules' in the .EXE relocation table, so this module is linked
;between them. The far pointers start as DGROUP:0000.
;(DS:14F0-14F1, a zero word, is the end of drvdata.c's savedPalette: OSILIB/PALSAVE.C saves
;600h bytes from 0EF2 there)
;----------------------------------------
INCLUDE OSI.INC

DSegStart
PUBLIC D_14F2,_tmDrvCode,_pText,_drvCopyLine,_drvFillLine,_drvPutChar,clearLineY,clearLinesLeft,charFitSlack,crtStatusPort

D_14F0	dw 0	;D_14F0 (the last word of savedPalette: not this module's)
D_14F2	LABEL	DWORD	;D_14F2 (not used by MARTIAN.EXE (Savage Empire's, addresses moved))
	FarNull
_tmDrvCode	LABEL	DWORD	;D_14F6 TM.DRV in memory (LIB/tmdrv.c): while its first word is not 0 the mouse
;pointer is not redrawn
	FarNull
_pText	dw 0	;D_14FA struct tText *: the current text window
_drvCopyLine	LABEL	DWORD	;D_14FC TM.DRV +04: copy a line of pixels
;(ES: segment, SI: scanlines, BX: from y, DI: to y, DX: left, CX: right)
	FarNull
_drvFillLine	LABEL	DWORD	;D_1500 TM.DRV +06: fill a line of pixels
;(ES: segment, SI: scanlines, BX: y, DX: left, CX: right, AL: color)
	FarNull
_drvPutChar	LABEL	DWORD	;D_1504 TM.DRV +02: draw the character AX (BX: struct tText *, CX: height, DX: width, SI: 2*y)
	FarNull
clearLineY	dw 0	;D_1508 TXT_clearLine: the line of pixels being filled
clearLinesLeft	dw 0	;D_150A TXT_clearLine: lines of pixels left
charFitSlack	dw 0	;D_150C added to the cursor's x to test whether a character fits (never set: 0)
crtStatusPort	dw 0	;D_150E the CRT status port (3BAh or 3DAh), set by VID_initPort
DSegEnd

END
