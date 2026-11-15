/* flags: -mm -c -O */
/*
 * MARTIAN.EXE segment 0DBF: the sparkles of the title's last scene (8 frames each, with a
 * sound panned by where they are). Savage Empire's SRC_S/sprites.c (SAVAGE.EXE 071C), the
 * addresses moved.
 */

/*a sparkle (12 bytes)*/
struct tSprite {
	int frame;/*+00 -1: free*/
	int x;/*+02*/
	int y;/*+04*/
	int kind;/*+06*/
	void far *sound;/*+08*/
};

extern int (*drvDrawShape)();/*the graphics driver: draws a shape*/
/*4364*/extern void far *sparkleShapes;/*the shapes of the sparkles*/
/*4368*/extern struct tSprite sparkleList[30];

extern StopSound(void far *, int);/*stops a sound*/
extern void far *PlaySound(void far *snd, int, int, int pan, int, int);/*plays a sound*/

/*0DF4*/unsigned char sparkleSound[] = {0x00, 0x80, 0x40, 0x40, 0x3c, 0x00};/*the sound of a sparkle*/
/*0DFA*/int D_0DFA = 0;/*not used: a zero word after the sound (in SAVAGE.EXE it follows the same data, kept there in OSIDATA.ASM)*/

/*C_0DBF_0000*/void ClearSprites() {
	int i;

	for(i = 0; i < 30; i++)
		sparkleList[i].frame = -1;
}

/*draws and moves the first n sparkles; returns how many of them are free*/
/*C_0DBF_0020*/DrawSprites(int surf, int n) {
	int i;
	int nFree = 0;

	nFree = 0;
	for(i = n - 1; i >= 0; i--) {
		if(sparkleList[i].frame == -1) {
			nFree++;
			continue;
		}
		(*drvDrawShape)(surf, sparkleList[i].x, sparkleList[i].y, sparkleShapes, sparkleList[i].frame + sparkleList[i].kind * 8);
		if(sparkleList[i].frame++ == 7) {
			sparkleList[i].frame = -1;
			StopSound(sparkleList[i].sound, i);
		} else if(sparkleList[i].frame == 1) {
			sparkleList[i].sound = PlaySound(sparkleSound, 0, 0x7f, (char)(0x7f - sparkleList[i].x * 0x7fL / 0x13f), i, 1);
		}
	}
	return nFree;
}
