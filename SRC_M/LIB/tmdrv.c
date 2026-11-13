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
extern unsigned far *D_14F6;/*TM.DRV: its header (an offset per entry point)*/
extern tDrvFunc D_0E8E;/*+08*/
extern tDrvFunc D_0E92;/*+0A*/
extern tDrvFunc D_0E96;/*+0C*/
extern tDrvFunc D_0E9A;/*+0E*/
extern tDrvFunc D_0E9E;/*+10*/
extern tDrvFunc D_0EA2;/*+12*/
extern tDrvFunc D_0EA6;/*+14*/
extern tDrvFunc D_0EAA;/*+16*/
extern tDrvFunc D_0EAE;/*+18*/
extern tDrvFunc D_0EB2;/*+1A*/
extern tDrvFunc D_0EB6;/*+1C*/
extern tDrvFunc D_0EBA;/*+1E*/
extern tDrvFunc D_0EBE;/*+20*/
extern tDrvFunc D_0EC2;/*+22*/
extern tDrvFunc D_0EC6;/*+24*/
extern tDrvFunc D_0ECA;/*+26*/
extern tDrvFunc D_0ECE;/*+28*/
extern tDrvFunc D_0ED2;/*+2A*/
extern tDrvFunc D_0ED6;/*+32*/
extern tDrvFunc D_0EEA;/*+2C*/
extern tDrvFunc D_14FC;/*+04*/
extern tDrvFunc D_1500;/*+06*/
extern tDrvFunc drvPutChar;/*+02*/

/*load TM.DRV (kind 4 loads kind 2's) and take its entry points; 0: not loaded*/
/*C_100F_0000*/int LoadTM(int kind) {
	register int k;
	register int seg;

	if(kind == 4)
		k = 2;
	else
		k = kind;
	D_14F6 = LZC_load(tmDrvName, k, 0L, 0x12, 0L);
	if(D_14F6) {
		seg = FP_SEG(D_14F6);
		drvPutChar = ENTRY(seg, D_14F6[0x02 / 2]);
		D_14FC = ENTRY(seg, D_14F6[0x04 / 2]);
		D_1500 = ENTRY(seg, D_14F6[0x06 / 2]);
		D_0E8E = ENTRY(seg, D_14F6[0x08 / 2]);
		D_0E92 = ENTRY(seg, D_14F6[0x0A / 2]);
		D_0E96 = ENTRY(seg, D_14F6[0x0C / 2]);
		D_0E9A = ENTRY(seg, D_14F6[0x0E / 2]);
		D_0E9E = ENTRY(seg, D_14F6[0x10 / 2]);
		D_0EA2 = ENTRY(seg, D_14F6[0x12 / 2]);
		D_0EA6 = ENTRY(seg, D_14F6[0x14 / 2]);
		D_0EAA = ENTRY(seg, D_14F6[0x16 / 2]);
		D_0EAE = ENTRY(seg, D_14F6[0x18 / 2]);
		D_0EB2 = ENTRY(seg, D_14F6[0x1A / 2]);
		D_0EB6 = ENTRY(seg, D_14F6[0x1C / 2]);
		D_0EBA = ENTRY(seg, D_14F6[0x1E / 2]);
		D_0EBE = ENTRY(seg, D_14F6[0x20 / 2]);
		D_0EC2 = ENTRY(seg, D_14F6[0x22 / 2]);
		D_0EC6 = ENTRY(seg, D_14F6[0x24 / 2]);
		D_0ECA = ENTRY(seg, D_14F6[0x26 / 2]);
		D_0ECE = ENTRY(seg, D_14F6[0x28 / 2]);
		D_0ED2 = ENTRY(seg, D_14F6[0x2A / 2]);
		D_0EEA = ENTRY(seg, D_14F6[0x2C / 2]);
		D_0ED6 = ENTRY(seg, D_14F6[0x32 / 2]);
		*(tDrvFunc far *)(D_14F6 + 0x2E / 2) = (tDrvFunc)EMS_map;
		return 1;
	}
	return 0;
}
