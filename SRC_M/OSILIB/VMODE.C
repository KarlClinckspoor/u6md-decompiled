/* flags: -mm -c -O -G */
/* compiler: Turbo C++ 1.01 */
/*
 * Martian Dreams - MARTIAN.EXE
 * the OSI library: setting the video mode, and the allocation of the palette's colors
 * (Savage Empire's VMODE.C; the allocation table is initialised data here, and PAL_dump
 * prints a char)
 */
#pragma inline
#include <stdio.h>

extern void far VID_initPort(void);/*VID_initPort: the CRT status port*/

/*1526*/int vidMode = -1;/*the video mode last set (a graphics mode)*/
/*1528*/int palAlloc[256] = {0};/*colors allocated: at each color of a run, the run's length (0: free)*/

/*set a video mode, through the BIOS (the BIOS equipment word says monochrome for mode 7,
  color otherwise); a graphics mode frees all the palette's colors*/
/*C_0EBB_0001*/void pascal VID_setMode(int mode) {
	register int i;

	if(mode > 3 && mode != 7) {
		vidMode = mode;
		for(i = 0; i < 256; i++)
			palAlloc[i] = 0;
	}
	asm mov ax,mode
	asm push ds
	asm mov cx,40h
	asm mov ds,cx
	asm mov bl,20h
	asm mov ah,al
	asm and ah,7
	asm cmp ah,7
	asm jnz color
	asm mov bl,30h
color:
	asm and byte ptr ds:[10h],0CFh
	asm or ds:[10h],bl
	asm xor ah,ah
	asm push bp
	asm int 10h
	asm pop bp
	asm pop ds
	VID_initPort();
}

/*allocate a run of n free colors: its first color, or -1*/
/*C_0EBB_005D*/int pascal PAL_alloc(register int n) {
	register int i;

	i = _CX = _DX = 0;
	for(; i < 256; i++) {
		if(palAlloc[i]) {
			_CX = 0;
			_DX = i + 1;
		} else
			_CX++;
		if(_CX == n) {
			for(i = 0; i < n; i++)
				palAlloc[i + _DX] = n;
			return _DX;
		}
	}
	return -1;
}

/*free the run of colors that starts at start*/
/*C_0EBB_00AF*/void pascal PAL_free(int start) {
	register int i;
	register int n;

	n = palAlloc[_AX = start];
	for(i = 0; i < n; i++)
		palAlloc[i + _AX] = 0;
}

/*print the colors allocated ('.') and free ('_'), 64 a line*/
/*C_0EBB_00DA*/PAL_dump() {
	int row;
	register int col;
	register int k;

	for(row = k = 0; row < 4; row++) {
		for(col = 0; col < 64; col++)
			printf("%c", palAlloc[k++] ? (char)'.' : (char)'_');
		printf("\n");
	}
}
