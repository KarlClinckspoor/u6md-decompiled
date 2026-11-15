/* flags: -mm -c -O -G */
/*
 * MARTIAN.EXE segment 0F09: the drawing surfaces of the screen and of the back buffer,
 * for each video mode (vidMode: 4 CGA, 7 Hercules, 9 Tandy, 13 EGA, 0x13 VGA/MCGA).
 * Savage Empire's LIB/screen.c (SAVAGE.EXE 0D28) with its variables defined here, in _DATA,
 * and a list of surfaces (surfaceList) that starts with the screen.
 * (inline assembly: compiled through the assembler. The line tables are filled with an
 * unsigned index, (unsigned)(i - 2), which keeps the subtraction out of the address.)
 */

/*a drawing surface (see surface.c)*/
struct tSurface {
	unsigned seg;/*+00 segment of the pixels*/
	unsigned *lines;/*+02 offset of each line (from line top)*/
	int left;/*+04 clipping window (inclusive)*/
	int top;/*+06*/
	int right;/*+08*/
	int bottom;/*+0A*/
	char huge *pixels;/*+0C the memory allocated for it*/
};

/*an entry of the list of surfaces (only the screen's is set here)*/
struct tSurfaceLink {
	int id;/*+00*/
	struct tSurface *surface;/*+02*/
	int x;/*+04 (not used here)*/
};

extern int vidMode;/*the video mode*/
extern void (far *drvClearSurface)(struct tSurface *, int);/*a TM.DRV routine (header +18)*/

extern int pascal AllocSurface(struct tSurface *s, int x, int n);

/*172E*/struct tSurface *screenSurf = 0;/*the screen*/
/*1730*/struct tSurface *bufferSurf = 0;/*the back buffer*/
/*1732*/struct tSurfaceLink screenLink = {0};/*the screen's entry*/
/*1738*/struct tSurfaceLink *surfaceList = 0;/*the list of surfaces*/
/*173A*/unsigned screenLines[200] = {0};/*the screen's line offsets*/

/*make s the screen (EGA: its second page); x: handed to the driver (*drvClearSurface) unless -1
  (always -1 but in VGA/MCGA); 0: out of memory*/
/*C_0F09_000F*/int pascal ScreenSurface(register struct tSurface *s, register int x) {
	int i;

	screenSurf = s;
	surfaceList = &screenLink;
	screenLink.id = 0;
	screenLink.surface = s;
	s->left = 0;
	s->top = 0;
	s->right = 319;
	s->bottom = 199;
	switch(vidMode) {
		case 13:
			if(AllocSurface(s, x, 0) == 0)
				return 0;
			i = s->seg;
			/*the CRT controller's start address (high byte): the surface*/
			asm mov dx,3d4h
			asm mov ax,i
			asm shl ax,1
			asm shl ax,1
			asm shl ax,1
			asm shl ax,1
			asm mov al,0ch
			asm out dx,ax
		break;
		case 4:
			s->seg = 0xb800;
			s->lines = screenLines;
			screenLines[0] = 0;
			screenLines[1] = 0x2000;
			for(i = 2; i < 200; i ++)
				screenLines[i] = screenLines[(unsigned)(i - 2)] + 80;
		break;
		case 7:
			s->seg = 0xb000;
			s->lines = screenLines;
			screenLines[0] = 0x032f;
			screenLines[1] = 0x232f;
			screenLines[2] = 0x632f;
			screenLines[3] = 0x0389;
			screenLines[4] = 0x4389;
			screenLines[5] = 0x6389;
			screenLines[6] = 0x23e3;
			screenLines[7] = 0x43e3;
			for(i = 8; i < 200; i ++)
				screenLines[i] = screenLines[(unsigned)(i - 8)] + 270;
		break;
		case 9:
			s->seg = 0xb800;
			s->lines = screenLines;
			screenLines[0] = 0;
			screenLines[1] = 0x2000;
			screenLines[2] = 0x4000;
			screenLines[3] = 0x6000;
			for(i = 4; i < 200; i ++)
				screenLines[i] = screenLines[(unsigned)(i - 4)] + 160;
		break;
		default:
			if(vidMode != 0x13)
				x = -1;
			s->seg = 0xa000;
			s->lines = screenLines;
			for(i = 0; i < 200; i ++)
				screenLines[i] = i * 320;
	}
	if(x != -1)
		(*drvClearSurface)(s, x);
	return 1;
}

/*make s the back buffer, 320x200, allocated*/
/*C_0F09_01DA*/int pascal BufferSurface(struct tSurface *s, int x) {
	bufferSurf = s;
	s->left = 0;
	s->right = 319;
	s->top = 0;
	s->bottom = 199;
	return AllocSurface(s, x, 0);
}

