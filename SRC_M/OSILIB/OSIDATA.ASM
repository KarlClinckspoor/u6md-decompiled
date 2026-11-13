;----------------------------------------
;OSI library (MARTIAN.EXE (Savage Empire's, addresses moved)): the library's shared variables (a module with data only)
;Which module of Origin's library held these is not known: they come after the shape
;routines (0E7C) and before TXTJUST (0E7E) in the data segment, and their relocations
;sit between those two modules' in the .EXE relocation table, so this module is linked
;between them. The far pointers start as DGROUP:0000.
;(DS:14F0-14F1, a zero word, may belong to the module before)
;----------------------------------------
INCLUDE OSI.INC

DSegStart
PUBLIC D_14F2,_D_14F6,_pText,_D_14FC,_D_1500,_drvPutChar,D_1508,D_150A,D_150C,D_150E

D_14F0	dw 0	;D_14F0 (not used by MARTIAN.EXE (Savage Empire's, addresses moved); it could as well be another module's)
D_14F2	LABEL	DWORD	;D_14F2 (not used by MARTIAN.EXE (Savage Empire's, addresses moved))
	FarNull
_D_14F6	LABEL	DWORD	;D_14F6 int far *: while *D_14F6 is not 0 the mouse pointer is not redrawn
	FarNull
_pText	dw 0	;D_14FA struct tText *: the current text window
_D_14FC	LABEL	DWORD	;D_14FC driver: copy a line of pixels (ES: segment, SI: scanlines, BX: from y, DI: to y, DX: left, CX: right)
	FarNull
_D_1500	LABEL	DWORD	;D_1500 driver: fill a line of pixels (ES: segment, SI: scanlines, BX: y, DX: left, CX: right, AL: color)
	FarNull
_drvPutChar	LABEL	DWORD	;D_1504 driver: draw the character AX (BX: struct tText *, CX: height, DX: width, SI: 2*y)
	FarNull
D_1508	dw 0	;D_1508 TXT_clearLine: the line of pixels being filled
D_150A	dw 0	;D_150A TXT_clearLine: lines of pixels left
D_150C	dw 0	;D_150C added to the cursor's x to test whether a character fits (never set: 0)
D_150E	dw 0	;D_150E the CRT status port (3BAh or 3DAh), set by VID_initPort
DSegEnd

END
