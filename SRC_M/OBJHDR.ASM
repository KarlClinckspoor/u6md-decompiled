;----------------------------------------
;MARTIAN.EXE: the game state block that ends savegame\objlist (a module with data only):
;GAME.EXE's gamestat.c from IsOnQuest (62h of its 64h bytes: up to WakeYear), with its
;initial values (8:00, 6 January 1895, MapX 198h, MapY 270h ...). CreateCharacter (main.c)
;reads the file's first 1CF1h bytes into objList, the next 62h over this block, and writes
;both back; 0BF6 is PlayerIsFemale, 0BF7 AvatarPortrait (nonzero: a character exists).
;Linked after main.c (its _DATA follow main's at DS:0BC0); which module of Origin's held
;it is not known.
;(The name OBJHDR is older than the finding that the block is at the file's end.)
;----------------------------------------
_DATA	SEGMENT PUBLIC WORD 'DATA'
_DATA	ENDS
DGROUP	GROUP	_DATA

_DATA	SEGMENT PUBLIC WORD 'DATA'
PUBLIC	_gameState,_PlayerIsFemale,_AvatarPortrait
_gameState	LABEL	BYTE	;D_0BC0 the block (62h bytes)
	db	000h,000h,000h,008h,006h,001h,067h,007h,000h,007h,098h,001h
	db	070h,002h,000h,000h,000h,000h,000h,000h,000h,001h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,001h,0FFh,000h,0FFh
_PlayerIsFemale	db	000h	;D_0BF6 the avatar is a woman
_AvatarPortrait	db	000h	;D_0BF7 nonzero: a character exists (GAME.EXE: its portrait)
	db	000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h
D_0C22	dw	0	;D_0C22 (not used; GAME.EXE's DreamStage would be here, but only 62h bytes are read)
_DATA	ENDS

END
