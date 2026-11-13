/* flags: -mm -c -O -G */
/* compiler: Turbo C++ 1.01 */
/*
 * Martian Dreams - MARTIAN.EXE
 * the OSI library: the colors a surface uses (Savage Empire's PALUSED.C; the driver entry
 * is TM.DRV's, and the index of the store goes through AX)
 */
#include <stdlib.h>

struct tSurface;
extern void (far pascal *D_0E8E)(struct tSurface *s, char *used);/*TM.DRV +08: used[c] = 1 for each color c in s's window*/

/*list in out the colors (of n) used in s's window: their number (0 without memory)*/
/*C_0FEA_0008*/int pascal PAL_colorsUsed(struct tSurface *s, unsigned char *out, int n) {
	register int count;
	register char *used;
	int i;

	count = 0;
	used = calloc(n, 1);
	if(used == 0)
		return 0;
	(*D_0E8E)(s, used);
	for(i = 0; i < n; i++)
		if(used[i]) {
			_AX = count++;
			out[_AX] = i;
		}
	free(used);
	return count;
}
