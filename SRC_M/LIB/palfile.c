/* flags: -mm -c -O -G */ /* compiler: tcpp101 (Turbo C++ 1.01) */
/*
 * Loading a VGA palette from a file: the 768 bytes at offset 0x30 (256 colors, 8 bits a
 * component), set with PAL_setAll. A library module of MARTIAN.EXE's (not in SAVAGE.EXE),
 * compiled with Turbo C++ 1.01 (1.00 tests AX, not SI, after `pal = malloc(...)`).
 */
#include <stdio.h>
#include <stdlib.h>

extern pascal PAL_setAll(unsigned char far *pal);/*OSILIB/PALSET.ASM*/

/*sets the palette held in the file name; 0: no memory or no file*/
/*C_102E_000E*/int pascal PAL_loadFile(char *name) {
	unsigned char *pal;
	FILE *f;

	pal = malloc(0x300);
	if(pal == 0)
		return 0;
	f = fopen(name, "rb");
	if(f != 0) {
		fseek(f, 0x30L, SEEK_SET);
		fread(pal, 0x300, 1, f);
		PAL_setAll(pal);
		fclose(f);
		free(pal);
		return 1;
	}
	free(pal);
	return 0;
}
