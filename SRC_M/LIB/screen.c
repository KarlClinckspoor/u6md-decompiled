/* flags: -mm -c -O -G */
/*
 * MARTIAN.EXE segment 0F09: the drawing surfaces of the screen and of the back buffer,
 * for each video mode (vidMode: 4 CGA, 7 Hercules, 9 Tandy, 13 EGA, 0x13 VGA/MCGA).
 * Savage Empire's LIB/screen.c (SAVAGE.EXE 0D28) with its variables defined here, in _DATA,
 * and a list of surfaces (D_1738) that starts with the screen.
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

extern int vidMode;/*vidMode: the video mode*/
extern void (far *D_0EAE)(struct tSurface *, int);/*a TM.DRV routine (header +18)*/

extern int pascal AllocSurface(struct tSurface *s, int x, int n);/*AllocSurface*/

/*172E*/struct tSurface *D_172E = 0;/*the screen*/
/*1730*/struct tSurface *D_1730 = 0;/*the back buffer*/
/*1732*/struct tSurfaceLink D_1732 = {0};/*the screen's entry*/
/*1738*/struct tSurfaceLink *D_1738 = 0;/*the list of surfaces*/
/*173A*/unsigned D_173A[200] = {0};/*the screen's line offsets*/

/*make s the screen (EGA: its second page); x: handed to the driver (*D_0EAE) unless -1
  (always -1 but in VGA/MCGA); 0: out of memory*/
/*C_0F09_000F*/int pascal ScreenSurface(register struct tSurface *s, register int x) {
	int i;

	D_172E = s;
	D_1738 = &D_1732;
	D_1732.id = 0;
	D_1732.surface = s;
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
			s->lines = D_173A;
			D_173A[0] = 0;
			D_173A[1] = 0x2000;
			for(i = 2; i < 200; i ++)
				D_173A[i] = D_173A[(unsigned)(i - 2)] + 80;
		break;
		case 7:
			s->seg = 0xb000;
			s->lines = D_173A;
			D_173A[0] = 0x032f;
			D_173A[1] = 0x232f;
			D_173A[2] = 0x632f;
			D_173A[3] = 0x0389;
			D_173A[4] = 0x4389;
			D_173A[5] = 0x6389;
			D_173A[6] = 0x23e3;
			D_173A[7] = 0x43e3;
			for(i = 8; i < 200; i ++)
				D_173A[i] = D_173A[(unsigned)(i - 8)] + 270;
		break;
		case 9:
			s->seg = 0xb800;
			s->lines = D_173A;
			D_173A[0] = 0;
			D_173A[1] = 0x2000;
			D_173A[2] = 0x4000;
			D_173A[3] = 0x6000;
			for(i = 4; i < 200; i ++)
				D_173A[i] = D_173A[(unsigned)(i - 4)] + 160;
		break;
		default:
			if(vidMode != 0x13)
				x = -1;
			s->seg = 0xa000;
			s->lines = D_173A;
			for(i = 0; i < 200; i ++)
				D_173A[i] = i * 320;
	}
	if(x != -1)
		(*D_0EAE)(s, x);
	return 1;
}

/*make s the back buffer, 320x200, allocated*/
/*C_0F09_01DA*/int pascal BufferSurface(struct tSurface *s, int x) {
	D_1730 = s;
	s->left = 0;
	s->right = 319;
	s->top = 0;
	s->bottom = 199;
	return AllocSurface(s, x, 0);
}

