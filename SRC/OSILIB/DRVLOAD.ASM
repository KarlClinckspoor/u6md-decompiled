;----------------------------------------
;OSI library: loading the graphics driver (CGA/EGA/T1K/HER/MCG.DRV), and the
;driver's far entry point and parameter block (the block's address goes to the
;driver in BX, module 264A). New in Martian Dreams; it took the place of
;Savage Empire's OSI_selectDrive (LOW.ASM's first code segment) and reports
;an error the same way (D_2A84, DriveErrorCallback).
;----------------------------------------
INCLUDE OSI.INC

EXTRN _drvName:WORD	;the drivers' file names (near pointers) by driver type (data-only module)
EXTRN D_2A84:WORD,DriveErrorCallback:DWORD	;LOW.ASM
EXTRN my_alloc:FAR,C_2674_04D9:FAR	;LOW.ASM: allocate BX paragraphs; load a file whole

DSegStart

PUBLIC D_2A28,D_2A2A,D_2A2C,D_2A3A,D_2A42,D_2A44,D_2A46,D_2A48
D_2A28	dw 0	;the driver's entry: offset (the function number, set before a call)
D_2A2A	dw 0	;and segment: where the driver was loaded
;the driver's parameter block (2A2C-2A6C)
D_2A2C	dw 0	;CGA wanted
	dw 8,8,10h,10h,0,0
D_2A3A	dw 0	;the driver type: 0 CGA, 1 EGA, 2 Tandy 1000, 3 Hercules, 5 MCGA; -1 none
	dw 0,0,0
D_2A42	dw 0	;the clipping rectangle: left
D_2A44	dw 13Fh	;right
D_2A46	dw 0	;top
D_2A48	dw 0C7h	;bottom
	dw 0,0,0
	db 0,1,2,3,4,5,6,7,38h,39h,3Ah,3Bh,3Ch,3Dh,3Eh,3Fh	;D_2A50 EGA palette registers
	db 0
D_2A61	dw 0	;EGA wanted
D_2A63	dw 0	;Tandy 1000 wanted
D_2A65	dw 0	;Hercules wanted
	dw 0
D_2A69	dw 0	;MCGA wanted
	db 90h

DSegEnd

CSegStart	SEG_263D_TEXT

;int OSI_loadDriver(void): unless the type is -1 (no driver), choose it from
;the flags (CGA, EGA, Hercules, Tandy 1000, MCGA, the first set; a Tandy 1000
;without the BIOS's 7000h service also gets 16K, its segment at 0000:0510),
;load its file and keep the segment; returns 0 after DriveErrorCallback on an
;error, -1 when the loader returned no error code
ProcStart	OSI_loadDriver	;263D:000E
	CMP	D_2A3A,-1
	JNZ	C_001E
	JMP	C_00CA
C_001E:
	CMP	D_2A2C,0
	JZ	C_002D
	MOV	D_2A3A,0
	JMP	SHORT C_0069
C_002D:
	CMP	D_2A61,0
	JZ	C_003C
	MOV	D_2A3A,1
	JMP	SHORT C_0069
C_003C:
	CMP	D_2A65,0
	JZ	C_004B
	MOV	D_2A3A,3
	JMP	SHORT C_0069
C_004B:
	CMP	D_2A63,0
	JZ	C_005A
	MOV	D_2A3A,2
	JMP	SHORT C_0069
C_005A:
	CMP	D_2A69,0
	JZ	C_0069
	MOV	D_2A3A,5
	JMP	SHORT C_0069
C_0069:
	MOV	BX,D_2A3A
	CMP	BX,2
	JNZ	C_00A5
	;-- Tandy 1000
	MOV	AH,0C0h		;get system configuration
	INT	15h
	JB	C_0089
	MOV	AX,7000h
	MOV	BX,1
	INT	15h
	MOV	BX,2
	TEST	DX,300h
	JNZ	C_00A5
C_0089:
	MOV	BX,400h
	CALL	my_alloc
	JNB	C_0097
	MOV	AH,4Ch		;exit
	INT	21h
C_0097:
	MOV	BX,50h
	MOV	ES,BX
	MOV	BX,10h
	MOV	ES:[BX],AX
	MOV	BX,2
C_00A5:
	SHL	BX,1
	MOV	DX,_drvName[BX]
	CALL	C_2674_04D9	;load the driver
	JB	C_00B8
	MOV	D_2A2A,BX
	JNB	C_00CA
C_00B8:
	OR	BX,BX
	JZ	C_00C7
	MOV	D_2A84,AX
	CALL	DriveErrorCallback
	XOR	AX,AX
	JMP	SHORT C_00CA
C_00C7:
	MOV	AX,-1
C_00CA:
ProcEnd	OSI_loadDriver

CSegEnd	SEG_263D_TEXT

END
