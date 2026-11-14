/* flags: -mm -c -O -Z -d */
/* GAME.EXE: data only, the graphics drivers' file names by driver type (OSILIB/DRVLOAD.ASM
   loads them). Its code segment is empty: the FBOV segment table lists it at 263D:E. -d
   merges the two "EGA.DRV". */
/*29F4*/char *drvName[] = {"CGA.DRV", "EGA.DRV", "T1K.DRV", "HER.DRV", "EGA.DRV", "MCG.DRV"};
