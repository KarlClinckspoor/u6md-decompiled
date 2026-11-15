/* flags: -mm -c -O -G */
/*
 * MARTIAN.EXE segment 0E62, the OSI library: save and restore the whole palette.
 * Savage Empire's SRC_S/OSILIB/PALSAVE.C (SAVAGE.EXE 0737); here an entry is addressed as
 * (int)savedPalette + i * 6. savedPalette (600h bytes) is defined in LIB/drvdata.c.
 */
struct tRGB { int r, g, b; };

extern void (pascal *drvGetPalette)(int index, struct tRGB *c);/*driver: get a palette entry*/
extern void (pascal *drvSetPalette)(int index, struct tRGB *c);/*driver: set a palette entry*/
extern struct tRGB savedPalette[256];/*the saved palette*/
extern void far VID_waitRetrace(void);

/*save the 256 colors (waiting for the retrace every 16)*/
/*C_0E62_0008*/PAL_save() {
	register int i;

	for(i = 0; i < 256; i++) {
		if(!(i & 15))
			VID_waitRetrace();
		(*drvGetPalette)(i, (struct tRGB *)((int)savedPalette + i * sizeof(struct tRGB)));
	}
}

/*and restore them*/
/*C_0E62_0037*/PAL_restore() {
	register int i;

	for(i = 0; i < 256; i++) {
		if(!(i & 15))
			VID_waitRetrace();
		(*drvSetPalette)(i, (struct tRGB *)((int)savedPalette + i * sizeof(struct tRGB)));
	}
}
