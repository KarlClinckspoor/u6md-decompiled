/* flags: -mm -c -O */
/*
 * MARTIAN.EXE: data only. The video modes, the names of the graphics drivers, and the
 * drivers' entry points (far pointers that LIB/tmdrv.c and LIB/supertm.c fill in from
 * each driver's header). Savage Empire's drvdata.c (SRC_S) has the first three and keeps
 * the entry points in uninitialised data; here they are initialised (to 0), followed by
 * the buffer of OSILIB/PALSAVE.C (savedPalette: 256 struct tRGB of three ints, 600h bytes;
 * its last word is OSIDATA.ASM's 14F0). Linked after the shape routines, before OSIDATA.ASM
 * (the data lie between RANDOM's and OSIDATA's).
 */

typedef void (far *tDrvFunc)();

/*0E6E*/unsigned char videoModes[] = {0x13, 0x0D, 4, 9, 7};/*MCGA/VGA, EGA, CGA, Tandy, Hercules*/
/*0E73*/char tmDrvName[] = "TM.DRV";
/*0E7A*/char superTmDrvName[] = "SUPERTM.DRV";
/*0E86*/unsigned far *superTmDrvCode = 0;/*SUPERTM.DRV in memory: its header*/
/*0E8A*/void far *D_0E8A = 0;/*not used*/
/*TM.DRV's entry points, by header offset (+02, +04, +06: OSIDATA.ASM); tmEntryXX: not called*/
/*0E8E*/tDrvFunc drvColorsUsed = 0;/*+08 used[c] = 1 for each colour c in a surface's window*/
/*0E92*/tDrvFunc drvGetPalette = 0;/*+0A get a palette entry*/
/*0E96*/tDrvFunc drvSetPalette = 0;/*+0C set a palette entry*/
/*0E9A*/tDrvFunc drvDrawShape = 0;/*+0E draw a shape (surface, x, y, shapes, n)*/
/*0E9E*/tDrvFunc drvSaveBG = 0;/*+10 save the background under a shape (the mouse pointer)*/
/*0EA2*/tDrvFunc drvRestoreBG = 0;/*+12 restore it*/
/*0EA6*/tDrvFunc tmEntry14 = 0;/*+14*/
/*0EAA*/tDrvFunc drvCopySurface = 0;/*+16 copy a surface's window onto another (from, to)*/
/*0EAE*/tDrvFunc drvClearSurface = 0;/*+18 clear a surface (surface, colour)*/
/*0EB2*/tDrvFunc drvPutPixel = 0;/*+1A set a pixel (surface, x, y, colour)*/
/*0EB6*/tDrvFunc drvGetPixel = 0;/*+1C the colour of a pixel (surface, x, y)*/
/*0EBA*/tDrvFunc tmEntry1E = 0;/*+1E*/
/*0EBE*/tDrvFunc tmEntry20 = 0;/*+20*/
/*0EC2*/tDrvFunc tmEntry22 = 0;/*+22*/
/*0EC6*/tDrvFunc drvFillRect = 0;/*+24 fill a rectangle (surface, x1, y1, x2, y2, colour)*/
/*0ECA*/tDrvFunc tmEntry26 = 0;/*+26*/
/*0ECE*/tDrvFunc tmEntry28 = 0;/*+28*/
/*0ED2*/tDrvFunc tmEntry2A = 0;/*+2A called once (PlayIntro: surface, x, y, 1, 1, colour), what it draws not known*/
/*0ED6*/tDrvFunc tmEntry32 = 0;/*+32*/
/*0EDA*/tDrvFunc drvDrawScaled = 0;/*SUPERTM.DRV +02: draw a shape rotated and scaled (surface, x, y, shapes, n, angle, scale, flags)*/
/*0EDE*/tDrvFunc superTmEntry04 = 0;/*SUPERTM.DRV +04*/
/*0EE2*/tDrvFunc drvDrawScaledRect = 0;/*SUPERTM.DRV +06: the same, reporting the rectangle drawn*/
/*0EE6*/tDrvFunc superTmEntry08 = 0;/*SUPERTM.DRV +08*/
/*0EEA*/tDrvFunc tmEntry2C = 0;/*TM.DRV +2C*/
/*0EEE*/tDrvFunc superTmEntry0A = 0;/*SUPERTM.DRV +0A*/
/*0EF2*/tDrvFunc savedPalette = 0;/*OSILIB/PALSAVE.C's saved palette (600h bytes, through D_0EF6)*/
/*0EF6*/char D_0EF6[0x5FA] = {0};/*the rest of savedPalette (not referred to by name)*/
