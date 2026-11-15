/* flags: -mm -c -O -G */ /* compiler: tcpp101 (Turbo C++ 1.01) */
/*
 * Loading the driver TM.DRV and taking its entry points (far pointers made of the
 * driver's segment and the offsets in its header). Savage Empire's SRC_S/LIB/tmdrv.c, first
 * half: MARTIAN.EXE has LoadSuperTM in a module of its own (LIB/supertm.c). The entry
 * points are made with MK_FP here. Compiled with Turbo C++ 1.01 (1.00 reuses DX for the
 * test of the loaded pointer).
 */

#include <dos.h>

typedef void (far *tDrvFunc)();

/*an entry point: driver segment and an offset of its header*/
#define ENTRY(seg, off) ((tDrvFunc)MK_FP(seg, off))

/*LZC_load: load a file (into buf, or allocated when buf is 0)*/
extern void far * pascal LZC_load(char far *name, int kind, void far *buf, int flags, long x);
extern void far *EMS_map(void far *p);/*map an EMS pointer: handed to the drivers (header +2E / +0C)*/

extern char tmDrvName[];/*"TM.DRV"*/
extern unsigned far *tmDrvCode;/*TM.DRV in memory: its header (an offset per entry point)*/
extern tDrvFunc drvColorsUsed;/*+08*/
extern tDrvFunc drvGetPalette;/*+0A*/
extern tDrvFunc drvSetPalette;/*+0C*/
extern tDrvFunc drvDrawShape;/*+0E*/
extern tDrvFunc drvSaveBG;/*+10*/
extern tDrvFunc drvRestoreBG;/*+12*/
extern tDrvFunc tmEntry14;/*+14*/
extern tDrvFunc drvCopySurface;/*+16*/
extern tDrvFunc drvClearSurface;/*+18*/
extern tDrvFunc drvPutPixel;/*+1A*/
extern tDrvFunc drvGetPixel;/*+1C*/
extern tDrvFunc tmEntry1E;/*+1E*/
extern tDrvFunc tmEntry20;/*+20*/
extern tDrvFunc tmEntry22;/*+22*/
extern tDrvFunc drvFillRect;/*+24*/
extern tDrvFunc tmEntry26;/*+26*/
extern tDrvFunc tmEntry28;/*+28*/
extern tDrvFunc tmEntry2A;/*+2A*/
extern tDrvFunc tmEntry32;/*+32*/
extern tDrvFunc tmEntry2C;/*+2C*/
extern tDrvFunc drvCopyLine;/*+04*/
extern tDrvFunc drvFillLine;/*+06*/
extern tDrvFunc drvPutChar;/*+02*/

/*load TM.DRV (kind 4 loads kind 2's) and take its entry points; 0: not loaded*/
/*C_100F_0000*/int LoadTM(int kind) {
	register int k;
	register int seg;

	if(kind == 4)
		k = 2;
	else
		k = kind;
	tmDrvCode = LZC_load(tmDrvName, k, 0L, 0x12, 0L);
	if(tmDrvCode) {
		seg = FP_SEG(tmDrvCode);
		drvPutChar = ENTRY(seg, tmDrvCode[0x02 / 2]);
		drvCopyLine = ENTRY(seg, tmDrvCode[0x04 / 2]);
		drvFillLine = ENTRY(seg, tmDrvCode[0x06 / 2]);
		drvColorsUsed = ENTRY(seg, tmDrvCode[0x08 / 2]);
		drvGetPalette = ENTRY(seg, tmDrvCode[0x0A / 2]);
		drvSetPalette = ENTRY(seg, tmDrvCode[0x0C / 2]);
		drvDrawShape = ENTRY(seg, tmDrvCode[0x0E / 2]);
		drvSaveBG = ENTRY(seg, tmDrvCode[0x10 / 2]);
		drvRestoreBG = ENTRY(seg, tmDrvCode[0x12 / 2]);
		tmEntry14 = ENTRY(seg, tmDrvCode[0x14 / 2]);
		drvCopySurface = ENTRY(seg, tmDrvCode[0x16 / 2]);
		drvClearSurface = ENTRY(seg, tmDrvCode[0x18 / 2]);
		drvPutPixel = ENTRY(seg, tmDrvCode[0x1A / 2]);
		drvGetPixel = ENTRY(seg, tmDrvCode[0x1C / 2]);
		tmEntry1E = ENTRY(seg, tmDrvCode[0x1E / 2]);
		tmEntry20 = ENTRY(seg, tmDrvCode[0x20 / 2]);
		tmEntry22 = ENTRY(seg, tmDrvCode[0x22 / 2]);
		drvFillRect = ENTRY(seg, tmDrvCode[0x24 / 2]);
		tmEntry26 = ENTRY(seg, tmDrvCode[0x26 / 2]);
		tmEntry28 = ENTRY(seg, tmDrvCode[0x28 / 2]);
		tmEntry2A = ENTRY(seg, tmDrvCode[0x2A / 2]);
		tmEntry2C = ENTRY(seg, tmDrvCode[0x2C / 2]);
		tmEntry32 = ENTRY(seg, tmDrvCode[0x32 / 2]);
		*(tDrvFunc far *)(tmDrvCode + 0x2E / 2) = (tDrvFunc)EMS_map;
		return 1;
	}
	return 0;
}
