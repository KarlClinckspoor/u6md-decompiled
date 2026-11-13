;----------------------------------------
;OSI library (MARTIAN.EXE (Savage Empire's, addresses moved)): the mouse's variables (a module with data only)
;Which module of Origin's library held these is not known: their relocation (D_1918)
;sits between 0FE6's and 0FEA's in the .EXE relocation table, so they belong to 0FE6
;or to a module linked between 0FE6 and 0FEA (this one). D_18CA-1915 before them are
;0F2C's (events.asm).
;----------------------------------------
INCLUDE OSI.INC

DSegStart
PUBLIC D_1916,D_191A,D_191C,D_1936,D_1938,D_193A,D_193C,D_193E

D_1916	LABEL	DWORD	;D_1916 (not used by MARTIAN.EXE (Savage Empire's, addresses moved))
	FarNull
D_191A	dw 0	;D_191A struct tMouse *: the mouse
D_191C	LABEL	BYTE	;D_191C struct tMouse: the mouse events.asm initialises
	tMouse	<0,0,0,0,0,0,0,0,0,0,0,0>
D_1936	dw 0	;D_1936 (events.asm) how far the mouse moved: x
D_1938	dw 0	;D_1938 where the pointer was drawn: x
D_193A	dw 0	;D_193A   y
D_193C	dw 0	;D_193C (events.asm) how far the mouse moved: y
D_193E	db 0	;D_193E (events.asm) 1: the mouse moved
DSegEnd

;an empty code segment, as Origin's macros always gave a module: being WORD aligned, it
;puts one byte of padding after 0FE6 (0FEA's code starts at an even address)
CSegStart	MOUSDATA_TEXT
CSegEnd	MOUSDATA_TEXT

END
