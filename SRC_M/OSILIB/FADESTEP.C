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
/*44D6*/extern int fadeSteps;/*the largest distance: the number of steps*/
/*44D2*/extern int *fadeDist;/*the distance of each value*/
/*44D8*/extern int fadeLeft;/*the steps left*/
/*44D4*/extern int *fadeDir;/*the direction of each value (+1/-1)*/
/*44D0*/extern int *fadeErr;/*the error term of each value*/

/*move each of the n values of cur one step towards target (all of them arrive in the
  same number of steps, as the points of Bresenham's lines): 0 when they have arrived
  (or there is no memory)*/
/*C_0FF0_000B*/int pascal FADE_step(int *cur, int *target, int n) {
	register int i;
	register int d;

	if(fadeStart) {
		fadeErr = malloc(n * 2);
		fadeDist = malloc(n * 2);
		fadeDir = malloc(n * 2);
		if(!fadeErr || !fadeDist || !fadeDir) {
			if(fadeErr)
				free(fadeErr);
			if(fadeDist)
				free(fadeDist);
			if(fadeDir)
				free(fadeDir);
			return 0;
		}
		fadeSteps = 0;
		for(i = 0; i < n; i++) {
			d = cur[i] - target[i];
			if(d < 0) {
				d = -d;
				fadeDir[i] = 1;
			} else {
				fadeDir[i] = -1;
			}
			fadeDist[i] = d;
			if(d > fadeSteps)
				fadeSteps = d;
		}
		d = fadeSteps / 2;
		for(i = 0; i < n; i++)
			fadeErr[i] = d;
		fadeLeft = fadeSteps;
		fadeStart = 0;
	}
	if(fadeLeft-- == 0) {
		free(fadeErr);
		free(fadeDist);
		free(fadeDir);
		fadeStart = 1;
		return 0;
	}
	for(i = 0; i < n; i++) {
		if((fadeErr[i] += fadeDist[i]) > fadeSteps) {
			fadeErr[i] -= fadeSteps;
			cur[i] += fadeDir[i];
		}
	}
	return 1;
}
