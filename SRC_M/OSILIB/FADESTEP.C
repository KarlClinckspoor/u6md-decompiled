/* flags: -mm -c -O -G */
/* compiler: Turbo C++ 1.01 */
/*
 * Martian Dreams - MARTIAN.EXE
 * the OSI library: palette fades, a step (Savage Empire's FADESTEP.C, U.EXE's FADE_step
 * made pascal; it gives up when it has no memory)
 */
#include <stdlib.h>

/*1940*/int fadeStart = 1;/*1: the next FADE_step starts a fade*/
/*this module's _BSS (uninitialised: declared only; each 2 bytes)*/
extern int D_44D6;/*fadeSteps: the largest distance: the number of steps*/
extern int *D_44D2;/*fadeDist: the distance of each value*/
extern int D_44D8;/*fadeLeft: the steps left*/
extern int *D_44D4;/*fadeDir: the direction of each value (+1/-1)*/
extern int *D_44D0;/*fadeErr: the error term of each value*/

/*move each of the n values of cur one step towards target (all of them arrive in the
  same number of steps, as the points of Bresenham's lines): 0 when they have arrived
  (or there is no memory)*/
/*C_0FF0_000B*/int pascal FADE_step(int *cur, int *target, int n) {
	register int i;
	register int d;

	if(fadeStart) {
		D_44D0 = malloc(n * 2);
		D_44D2 = malloc(n * 2);
		D_44D4 = malloc(n * 2);
		if(!D_44D0 || !D_44D2 || !D_44D4) {
			if(D_44D0)
				free(D_44D0);
			if(D_44D2)
				free(D_44D2);
			if(D_44D4)
				free(D_44D4);
			return 0;
		}
		D_44D6 = 0;
		for(i = 0; i < n; i++) {
			d = cur[i] - target[i];
			if(d < 0) {
				d = -d;
				D_44D4[i] = 1;
			} else {
				D_44D4[i] = -1;
			}
			D_44D2[i] = d;
			if(d > D_44D6)
				D_44D6 = d;
		}
		d = D_44D6 / 2;
		for(i = 0; i < n; i++)
			D_44D0[i] = d;
		D_44D8 = D_44D6;
		fadeStart = 0;
	}
	if(D_44D8-- == 0) {
		free(D_44D0);
		free(D_44D2);
		free(D_44D4);
		fadeStart = 1;
		return 0;
	}
	for(i = 0; i < n; i++) {
		if((D_44D0[i] += D_44D2[i]) > D_44D6) {
			D_44D0[i] -= D_44D6;
			cur[i] += D_44D4[i];
		}
	}
	return 1;
}
