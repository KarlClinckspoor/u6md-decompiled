;----------------------------------------
;OSI library (MARTIAN.EXE (Savage Empire's, addresses moved)): the mouse's variables (a module with data only)
;Which module of Origin's library held these is not known: their relocation (D_1918)
;sits between 0FE6's and 0FEA's in the .EXE relocation table, so they belong to 0FE6
;or to a module linked between 0FE6 and 0FEA (this one). D_18CA-1915 before them are
;0F2C's (events.asm).
;----------------------------------------
INCLUDE OSI.INC

DSegStart
PUBLIC D_1916,curMouse,mouseState,mouseDX,mouseDrawX,mouseDrawY,mouseDY,mouseMoved

D_1916	LABEL	DWORD	;D_1916 (not used by MARTIAN.EXE (Savage Empire's, addresses moved))
	FarNull
curMouse	dw 0	;D_191A struct tMouse *: the mouse
mouseState	LABEL	BYTE	;D_191C struct tMouse: the mouse events.asm initialises
	tMouse	<0,0,0,0,0,0,0,0,0,0,0,0>
mouseDX	dw 0	;D_1936 (events.asm) how far the mouse moved: x
mouseDrawX	dw 0	;D_1938 where the pointer was drawn: x
mouseDrawY	dw 0	;D_193A   y
mouseDY	dw 0	;D_193C (events.asm) how far the mouse moved: y
mouseMoved	db 0	;D_193E (events.asm) 1: the mouse moved
DSegEnd

;an empty code segment, as Origin's macros always gave a module: being WORD aligned, it
;puts one byte of padding after 0FE6 (0FEA's code starts at an even address)
CSegStart	MOUSDATA_TEXT
CSegEnd	MOUSDATA_TEXT

END
