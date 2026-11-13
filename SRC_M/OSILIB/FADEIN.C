/* flags: -mm -c -O -G */
/*
 * Martian Dreams - MARTIAN.EXE
 * the OSI library: palette fades, fading a picture in and fading colors out (Savage
 * Empire's FADEIN.C and FADEOUT.C in one module, through TM.DRV's entry points; here
 * without the test of the video mode, with a static color list, and they return 0 when
 * they have no memory)
 */
#include <stdlib.h>

struct tSurface {
	unsigned segment;/*+00 segment of the pixels*/
	unsigned *scanlines;/*+02 offset of each line*/
	int left, top, right, bottom;/*+04 the clipping window (inclusive)*/
};
struct tRGB { int r, g, b; };

extern unsigned char D_4230[256];/*the colors a picture uses (uninitialised, 256 bytes)*/
extern int (far *D_0EB6)(struct tSurface *s, int x, int y);/*TM.DRV +1C: the color of a pixel*/
extern void (far *D_0EAA)(struct tSurface *src, struct tSurface *dst);/*TM.DRV +16: copy a surface's window*/
extern void (far pascal *D_0E92)(int index, struct tRGB *c);/*TM.DRV +0A: get a palette entry*/
extern void (far pascal *D_0E96)(int index, struct tRGB *c);/*TM.DRV +0C: set a palette entry*/
extern void far VID_waitRetrace(void);/*VID_waitRetrace*/
extern int pascal PAL_colorsUsed(struct tSurface *s, unsigned char *out, int n);/*PAL_colorsUsed*/
extern int pascal FADE_step(int *cur, int *target, int n);/*FADE_step*/

/*show the picture src on dst, its colors fading in from the color of dst's top left
  pixel: 0 without memory, otherwise 1*/
/*C_0CB7_0001*/int pascal FADE_in(struct tSurface *src, struct tSurface *dst) {
	int count;
	struct tRGB *cur;
	struct tRGB *target;
	struct tRGB rgb;
	int i;

	count = PAL_colorsUsed(src, D_4230, 256);
	cur = calloc(count, sizeof(struct tRGB));
	target = calloc(count, sizeof(struct tRGB));
	if(!cur || !target)
		return 0;
	(*D_0E92)((*D_0EB6)(dst, dst->left, dst->top), &rgb);
	VID_waitRetrace();
	for(i = 0; i < count; i++) {
		(*D_0E92)(D_4230[i], &cur[i]);
		(*D_0E96)(D_4230[i], &rgb);
		target[i] = rgb;
	}
	VID_waitRetrace();
	(*D_0EAA)(src, dst);
	while(FADE_step((int *)target, (int *)cur, count * 3)) {
		VID_waitRetrace();
		for(i = 0; i < count; i++)
			(*D_0E96)(D_4230[i], &target[i]);
	}
	free(target);
	free(cur);
	return 1;
}

/*fade the colors used in s's window to the palette's color "color", waiting delay
  retraces between the steps: 0 without memory, otherwise 1*/
/*C_0CB7_0125*/int pascal FADE_out(struct tSurface *s, int color, int delay) {
	int count;
	struct tRGB *cur;
	struct tRGB *target;
	struct tRGB rgb;
	int i;

	count = PAL_colorsUsed(s, D_4230, 256);
	cur = calloc(count, sizeof(struct tRGB));
	target = calloc(count, sizeof(struct tRGB));
	if(!cur || !target)
		return 0;
	(*D_0E92)(color, &rgb);
	for(i = 0; i < count; i++) {
		(*D_0E92)(D_4230[i], &cur[i]);
		target[i] = rgb;
	}
	while(FADE_step((int *)cur, (int *)target, count * 3)) {
		if(delay)
			for(i = delay; i--; )
				VID_waitRetrace();
		for(i = 0; i < count; i++)
			(*D_0E96)(D_4230[i], &cur[i]);
	}
	free(target);
	free(cur);
	return 1;
}
