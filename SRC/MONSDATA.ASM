;----------------------------------------
;The monster tables (far data segment 2DE5, after msgs): Savage Empire's and Ultima
;VI's monsdata.c, Martian Dreams'. A monster class is the index of a creature's object
;type in MonsterTypes (GetMonsterClass C_1649_000A); the other tables are indexed
;by class too. Assembly, not C: Turbo C gives each far variable a paragraph-aligned
;segment of its own, while here one segment holds all the tables; and the module has an
;empty WORD-aligned code segment (1649:A, see MSGS.ASM).
;----------------------------------------

MONSDATA_TEXT	SEGMENT WORD PUBLIC 'CODE'
MONSDATA_TEXT	ENDS

MONSDATA_DATA	SEGMENT PARA PUBLIC 'FAR_DATA'

PUBLIC	MonsterTypes
MonsterTypes	LABEL	BYTE	;2DE5:0000 the object type of each monster class (class 0 unused)
	dw	0,157h,158h,156h,159h,15Ah,15Bh,15Ch,15Dh,15Eh
	dw	15Fh,160h,161h,162h,163h,164h,165h,166h,171h,167h
	dw	168h,169h,174h,179h,17Ah,17Ch,175h,16Ah,17Fh,16Bh
	dw	16Ch,91h,180h,181h,16Dh,176h,178h,17Bh,17Dh,182h
	dw	177h,16Eh,16Fh,183h,170h,18Eh,187h,198h,1A8h,193h
	dw	0
PUBLIC	MonsterStr
MonsterStr	LABEL	BYTE	;2DE5:0066
	db	0,25,25,22,15,18,20,25,25,25,25,25,25,25,25,25
	db	25,25,15,20,25,15,20,20,10,15,30,20,20,15,20,20
	db	15,10,17,15,29,20,30,25,10,15,20,10,15,20,12,25
	db	25,20
PUBLIC	MonsterDex
MonsterDex	LABEL	BYTE	;2DE5:0098
	db	0,25,25,22,15,18,25,25,25,25,25,25,25,25,25,25
	db	25,15,15,20,12,22,15,20,20,15,10,20,12,15,19,19
	db	18,22,12,15,18,20,30,25,25,15,15,25,22,18,20,18
	db	18,20
PUBLIC	MonsterInt
MonsterInt	LABEL	BYTE	;2DE5:00CA
	db	0,25,25,22,25,22,18,25,25,25,25,25,25,25,25,25
	db	25,10,20,10,11,9,10,20,15,8,9,9,3,3,10,10
	db	3,10,8,3,10,15,30,10,12,15,10,10,12,8,15,5
	db	5,10
PUBLIC	MonsterArmor
MonsterArmor	LABEL	BYTE	;2DE5:00FC
	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	db	0,4,0,4,8,1,8,3,0,4,3,2,4,10,2,2
	db	4,1,3,1,10,5,99,4,1,2,10,2,4,2,9,3
	db	3,99
PUBLIC	MonsterDamage
MonsterDamage	LABEL	BYTE	;2DE5:012E
	db	0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6
	db	10,12,6,35,20,12,25,30,6,20,30,20,25,12,10,12
	db	12,4,20,16,20,25,60,18,10,15,20,12,20,15,1,12
	db	12,12
PUBLIC	MonsterHP
MonsterHP	LABEL	BYTE	;2DE5:0160
	db	0,90,90,60,30,60,60,90,90,90,90,90,90,90,90,90
	db	90,40,30,45,35,20,40,80,10,30,100,10,30,20,20,20
	db	25,10,20,20,30,30,255,40,20,30,30,20,20,90,255,90
	db	90,255
PUBLIC	MonsterAlignment
MonsterAlignment	LABEL	BYTE	;2DE5:0192 NPCStatus bits
	db	0,64,64,32,64,64,64,64,64,64,64,64,64,64,64,64
	db	64,64,0,96,0,0,96,0,32,32,96,96,96,96,32,32
	db	96,96,96,96,96,96,0,96,96,32,96,32,96,0,96,0
	db	0,32,0,16
PUBLIC	MonsterFlags
MonsterFlags	LABEL	BYTE	;2DE5:01C6
	dw	6000h,6000h,6000h,6000h,6000h,6000h,6000h,6000h,6000h,6000h
	dw	6000h,6000h,6000h,6000h,6000h,0,0,2080h,2080h,20A0h
	dw	2080h,6080h,5400h,980h,3080h,0E080h,0E0A0h,3080h,2080h,20A0h
	dw	0A0A0h,3080h,3080h,2080h,3080h,20h,1280h,9000h,0B084h,7080h
	dw	6080h,20h,3080h,2080h,7020h,5020h,8080h,8080h,8000h

MONSDATA_DATA	ENDS

END
