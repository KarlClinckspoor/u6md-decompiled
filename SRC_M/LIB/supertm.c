/* flags: -mm -c -O -G */ /* compiler: tcpp101 (Turbo C++ 1.01) */
/*
 * Loading the driver SUPERTM.DRV (after TM.DRV) and taking its entry points. Savage
 * Empire's SRC_S/LIB/tmdrv.c, second half (LoadSuperTM): a module of its own here; the
 * entry points are made with MK_FP. Compiled with Turbo C++ 1.01, like LIB/tmdrv.c.
 */

#include <dos.h>

typedef void (far *tDrvFunc)();

/*an entry point: driver segment and an offset of its header*/
#define ENTRY(seg, off) ((tDrvFunc)MK_FP(seg, off))

/*LZC_load: load a file (into buf, or allocated when buf is 0)*/
extern void far * pascal LZC_load(char far *name, int kind, void far *buf, int flags, long x);
extern void far *EMS_map(void far *p);/*map an EMS pointer: handed to the drivers (header +2E / +0C)*/

extern char superTmDrvName[];/*"SUPERTM.DRV"*/
extern unsigned far *superTmDrvCode;/*SUPERTM.DRV in memory: its header*/
extern tDrvFunc drvDrawScaled;/*+02*/
extern tDrvFunc superTmEntry04;/*+04*/
extern tDrvFunc drvDrawScaledRect;/*+06*/
extern tDrvFunc superTmEntry08;/*+08*/
extern tDrvFunc superTmEntry0A;/*+0A*/

extern int LoadTM(int kind);/*LIB/tmdrv.c*/

/*load TM.DRV, then SUPERTM.DRV, and take its entry points; 0: not loaded*/
/*C_1024_000B*/int LoadSuperTM(int kind) {
	register int k;
	register int seg;

	if(LoadTM(kind)) {
		if(kind == 4)
			k = 2;
		else
			k = kind;
		superTmDrvCode = LZC_load(superTmDrvName, k, 0L, 0x12, 0L);
		if(superTmDrvCode) {
			seg = FP_SEG(superTmDrvCode);
			drvDrawScaled = ENTRY(seg, superTmDrvCode[0x02 / 2]);
			superTmEntry04 = ENTRY(seg, superTmDrvCode[0x04 / 2]);
			drvDrawScaledRect = ENTRY(seg, superTmDrvCode[0x06 / 2]);
			superTmEntry08 = ENTRY(seg, superTmDrvCode[0x08 / 2]);
			superTmEntry0A = ENTRY(seg, superTmDrvCode[0x0A / 2]);
			*(tDrvFunc far *)(superTmDrvCode + 0x0C / 2) = (tDrvFunc)EMS_map;
			return 1;
		}
	}
	return 0;
}
