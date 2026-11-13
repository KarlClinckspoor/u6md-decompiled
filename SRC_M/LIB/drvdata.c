/* flags: -mm -c -O */
/*
 * MARTIAN.EXE: data only. The video modes, the names of the graphics drivers, and the
 * drivers' entry points (far pointers that LIB/tmdrv.c and LIB/supertm.c fill in from
 * each driver's header). Savage Empire's drvdata.c (SRC_S) has the first three and keeps
 * the entry points in uninitialised data; here they are initialised (to 0), followed by
 * 5FAh zero bytes nothing refers to. Linked after the shape routines, before OSIDATA.ASM
 * (the data lie between RANDOM's and OSIDATA's).
 */

typedef void (far *tDrvFunc)();

/*0E6E*/unsigned char videoModes[] = {0x13, 0x0D, 4, 9, 7};/*MCGA/VGA, EGA, CGA, Tandy, Hercules*/
/*0E73*/char tmDrvName[] = "TM.DRV";
/*0E7A*/char superTmDrvName[] = "SUPERTM.DRV";
/*0E86*/unsigned far *D_0E86 = 0;/*SUPERTM.DRV: its header*/
/*0E8A*/void far *D_0E8A = 0;/*not used*/
/*0E8E*/tDrvFunc D_0E8E = 0;
/*0E92*/tDrvFunc D_0E92 = 0;
/*0E96*/tDrvFunc D_0E96 = 0;
/*0E9A*/tDrvFunc D_0E9A = 0;
/*0E9E*/tDrvFunc D_0E9E = 0;
/*0EA2*/tDrvFunc D_0EA2 = 0;
/*0EA6*/tDrvFunc D_0EA6 = 0;
/*0EAA*/tDrvFunc D_0EAA = 0;
/*0EAE*/tDrvFunc D_0EAE = 0;
/*0EB2*/tDrvFunc D_0EB2 = 0;
/*0EB6*/tDrvFunc D_0EB6 = 0;
/*0EBA*/tDrvFunc D_0EBA = 0;
/*0EBE*/tDrvFunc D_0EBE = 0;
/*0EC2*/tDrvFunc D_0EC2 = 0;
/*0EC6*/tDrvFunc D_0EC6 = 0;
/*0ECA*/tDrvFunc D_0ECA = 0;
/*0ECE*/tDrvFunc D_0ECE = 0;
/*0ED2*/tDrvFunc D_0ED2 = 0;
/*0ED6*/tDrvFunc D_0ED6 = 0;
/*0EDA*/tDrvFunc D_0EDA = 0;
/*0EDE*/tDrvFunc D_0EDE = 0;
/*0EE2*/tDrvFunc D_0EE2 = 0;
/*0EE6*/tDrvFunc D_0EE6 = 0;
/*0EEA*/tDrvFunc D_0EEA = 0;
/*0EEE*/tDrvFunc D_0EEE = 0;
/*0EF2*/tDrvFunc D_0EF2 = 0;
/*0EF6*/char D_0EF6[0x5FA] = {0};/*not used*/
