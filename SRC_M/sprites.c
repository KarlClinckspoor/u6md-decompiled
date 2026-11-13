/* flags: -mm -c -O */
/*
 * Worlds of Ultima: The Savage Empire - SAVAGE.EXE
 * segment 071C: the sparkles of the title's last scene (8 frames each, with a sound
 * panned by where they are)
 */

/*a sparkle (12 bytes)*/
struct tSprite {
	int frame;/*+00 -1: free*/
	int x;/*+02*/
	int y;/*+04*/
	int kind;/*+06*/
	void far *sound;/*+08*/
};

extern int (*D_0E9A)();/*the graphics driver: draws a shape*/
extern void far *D_4364;/*the shapes of the sparkles*/
extern struct tSprite D_4368[30];

extern StopSound(void far *, int);/*stops a sound*/
extern void far *PlaySound(void far *snd, int, int, int pan, int, int);/*plays a sound*/

/*0DF4*/unsigned char D_0DF4[] = {0x00, 0x80, 0x40, 0x40, 0x3c, 0x00};/*the sound of a sparkle*/
/*0DFA*/int D_0DFA = 0;/*not used: a zero word after the sound (in SAVAGE.EXE it follows the same data, kept there in OSIDATA.ASM)*/

/*C_0DBF_0000*/void ClearSprites() {
	int i;

	for(i = 0; i < 30; i++)
		D_4368[i].frame = -1;
}

/*draws and moves the first n sparkles; returns how many of them are free*/
/*C_0DBF_0020*/DrawSprites(int surf, int n) {
	int i;
	int nFree = 0;

	nFree = 0;
	for(i = n - 1; i >= 0; i--) {
		if(D_4368[i].frame == -1) {
			nFree++;
			continue;
		}
		(*D_0E9A)(surf, D_4368[i].x, D_4368[i].y, D_4364, D_4368[i].frame + D_4368[i].kind * 8);
		if(D_4368[i].frame++ == 7) {
			D_4368[i].frame = -1;
			StopSound(D_4368[i].sound, i);
		} else if(D_4368[i].frame == 1) {
			D_4368[i].sound = PlaySound(D_0DF4, 0, 0x7f, (char)(0x7f - D_4368[i].x * 0x7fL / 0x13f), i, 1);
		}
	}
	return nFree;
}
