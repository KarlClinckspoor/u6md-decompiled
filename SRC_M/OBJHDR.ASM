;----------------------------------------
;MARTIAN.EXE: the header of savegame\objlist (a module with data only)
;CreateCharacter (main.c) reads the file's first 62h bytes over it and writes them back;
;0BF6 is the avatar's gender, 0BF7 1 while a character exists. Linked after main.c (its
;_DATA follow main's at DS:0BC0); which module of Origin's held it is not known.
;----------------------------------------
_DATA	SEGMENT PUBLIC WORD 'DATA'
_DATA	ENDS
DGROUP	GROUP	_DATA

_DATA	SEGMENT PUBLIC WORD 'DATA'
PUBLIC	_D_0BC0,_D_0BF6,_D_0BF7
_D_0BC0	LABEL	BYTE	;D_0BC0 the header (62h bytes)
	db	000h,000h,000h,008h,006h,001h,067h,007h,000h,007h,098h,001h
	db	070h,002h,000h,000h,000h,000h,000h,000h,000h,001h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,001h,0FFh,000h,0FFh
_D_0BF6	db	000h	;D_0BF6 the avatar's gender
_D_0BF7	db	000h	;D_0BF7 1: a character exists
	db	000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h
D_0C22	dw	0	;D_0C22 (not used)
_DATA	ENDS

END
