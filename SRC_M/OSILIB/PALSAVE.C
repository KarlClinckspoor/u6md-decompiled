/* flags: -mm -c -O -G */
/*
 * Worlds of Ultima: The Savage Empire - SAVAGE.EXE
 * the OSI library: save and restore the whole palette
 */
struct tRGB { int r, g, b; };

extern void (pascal *D_0E92)(int index, struct tRGB *c);/*driver: get a palette entry*/
extern void (pascal *D_0E96)(int index, struct tRGB *c);/*driver: set a palette entry*/
extern struct tRGB D_0EF2[256];/*the saved palette*/
extern void far VID_waitRetrace(void);/*VID_waitRetrace*/

/*save the 256 colors (waiting for the retrace every 16)*/
/*C_0E62_0008*/PAL_save() {
	register int i;

	for(i = 0; i < 256; i++) {
		if(!(i & 15))
			VID_waitRetrace();
		(*D_0E92)(i, (struct tRGB *)((int)D_0EF2 + i * sizeof(struct tRGB)));
	}
}

/*and restore them*/
/*C_0E62_0037*/PAL_restore() {
	register int i;

	for(i = 0; i < 256; i++) {
		if(!(i & 15))
			VID_waitRetrace();
		(*D_0E96)(i, (struct tRGB *)((int)D_0EF2 + i * sizeof(struct tRGB)));
	}
}
