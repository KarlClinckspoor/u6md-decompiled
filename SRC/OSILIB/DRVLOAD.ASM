;----------------------------------------
;OSI library: loading the graphics driver (CGA/EGA/T1K/HER/MCG.DRV), and the
;driver's far entry point and parameter block (the block's address goes to the
;driver in BX, module 264A). New in Martian Dreams; it took the place of
;Savage Empire's OSI_selectDrive (LOW.ASM's first code segment) and reports
;an error the same way (CriticalError, DriveErrorCallback).
;----------------------------------------
INCLUDE OSI.INC

EXTRN _drvName:WORD	;the drivers' file names (near pointers) by driver type (data-only module)
EXTRN CriticalError:WORD,DriveErrorCallback:DWORD	;LOW.ASM
EXTRN my_alloc:FAR,LOAD_file:FAR	;LOW.ASM: allocate BX paragraphs; load a file whole

DSegStart

PUBLIC GrDrvEntryOfs,GrDrvEntrySeg,GrDrvParams,GrDrvType,GrClipLeft,GrClipRight,GrClipTop,GrClipBottom
GrDrvEntryOfs	dw 0	;D_2A28 the driver's entry: offset (the function number, set before a call)
GrDrvEntrySeg	dw 0	;D_2A2A and segment: where the driver was loaded
;the driver's parameter block (2A2C-2A6C)
GrDrvParams	dw 0	;D_2A2C CGA wanted
	dw 8,8,10h,10h,0,0
GrDrvType	dw 0	;D_2A3A the driver type: 0 CGA, 1 EGA, 2 Tandy 1000, 3 Hercules, 5 MCGA; -1 none
	dw 0,0,0
GrClipLeft	dw 0	;D_2A42 the clipping rectangle: left
GrClipRight	dw 13Fh	;D_2A44 right
GrClipTop	dw 0	;D_2A46 top
GrClipBottom	dw 0C7h	;D_2A48 bottom
	dw 0,0,0
	db 0,1,2,3,4,5,6,7,38h,39h,3Ah,3Bh,3Ch,3Dh,3Eh,3Fh	;D_2A50 EGA palette registers
	db 0
GrWantEGA	dw 0	;D_2A61 EGA wanted
GrWantTandy	dw 0	;D_2A63 Tandy 1000 wanted
GrWantHercules	dw 0	;D_2A65 Hercules wanted
	dw 0
GrWantMCGA	dw 0	;D_2A69 MCGA wanted
	db 90h

DSegEnd

CSegStart	SEG_263D_TEXT

;int OSI_loadDriver(void): unless the type is -1 (no driver), choose it from
;the flags (CGA, EGA, Hercules, Tandy 1000, MCGA, the first set; a Tandy 1000
;without the BIOS's 7000h service also gets 16K, its segment at 0000:0510),
;load its file and keep the segment; returns 0 after DriveErrorCallback on an
;error, -1 when the loader returned no error code
ProcStart	OSI_loadDriver	;263D:000E
	CMP	GrDrvType,-1
	JNZ	C_001E
	JMP	C_00CA
C_001E:
	CMP	GrDrvParams,0
	JZ	C_002D
	MOV	GrDrvType,0
	JMP	SHORT C_0069
C_002D:
	CMP	GrWantEGA,0
	JZ	C_003C
	MOV	GrDrvType,1
	JMP	SHORT C_0069
C_003C:
	CMP	GrWantHercules,0
	JZ	C_004B
	MOV	GrDrvType,3
	JMP	SHORT C_0069
C_004B:
	CMP	GrWantTandy,0
	JZ	C_005A
	MOV	GrDrvType,2
	JMP	SHORT C_0069
C_005A:
	CMP	GrWantMCGA,0
	JZ	C_0069
	MOV	GrDrvType,5
	JMP	SHORT C_0069
C_0069:
	MOV	BX,GrDrvType
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
	CALL	LOAD_file	;load the driver
	JB	C_00B8
	MOV	GrDrvEntrySeg,BX
	JNB	C_00CA
C_00B8:
	OR	BX,BX
	JZ	C_00C7
	MOV	CriticalError,AX
	CALL	DriveErrorCallback
	XOR	AX,AX
	JMP	SHORT C_00CA
C_00C7:
	MOV	AX,-1
C_00CA:
ProcEnd	OSI_loadDriver

CSegEnd	SEG_263D_TEXT

END
