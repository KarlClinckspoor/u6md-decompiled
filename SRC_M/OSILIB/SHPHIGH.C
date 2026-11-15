/* flags: -mm -c -O -G */
/* compiler: Turbo C++ 1.01 */
/*
 * Martian Dreams - MARTIAN.EXE
 * the OSI library: shape files, the height of a file's shapes (Savage Empire's
 * SHPHIGH.C, unchanged; compiled by 1.01, which tests h from DI)
 */
extern int pascal SHP_count(char far *shapes);
extern int pascal SHP_rect(int far *rect, int x, int y, char far *shapes, int index);

/*meant to be the tallest of the shapes' heights; returns the last shape's height*/
/*C_0E5D_0003*/int pascal SHP_height(char far *shapes) {
	int n;
	int max;
	int rect[4];
	register int i;
	register int h;

	max = 0;
	n = SHP_count(shapes);
	for(i = 0; i < n; i++) {
		SHP_rect((int *)(unsigned)rect, 0, 0, shapes, i);
		h = rect[3] - rect[1] + 1;
		if(h > max)
			max = h;
	}
	return h;
}
