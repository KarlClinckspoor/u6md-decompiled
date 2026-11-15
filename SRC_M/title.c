/* flags: -mm -c -O -y */
/*
 * MARTIAN.EXE segment 0CD9: the title (TITLE.LZC): the letters of the title change and fly
 * apart, the sun sets behind the mountains, sparkles. Savage Empire's SRC_S/title.c (the
 * same pictures code); here PlayTitle gets the speed from its caller.
 */

/*a drawing surface (16 bytes; the fields this module uses)*/
struct tSurface {
	int w00;
	int w02;
	int w04;
	int x0;/*+06*/
	int w08;
	int y0;/*+0A*/
	int w0C;
	int w0E;
};

/*a sparkle of the last scene (12 bytes; see sprites.c)*/
struct tSprite {
	int frame;/*+00 -1: free*/
	int x;/*+02*/
	int y;/*+04*/
	int kind;/*+06*/
	void far *sound;/*+08*/
};

/*how a letter of the title moves*/
struct tLetter {
	int x;
	int y;
	int dx;
	int dy;
};

extern unsigned char far escapePressed;/*nonzero when a key was pressed (the event module's code segment)*/
extern int soundCard;/*the sound card? (1, 2: sounds are played; 3)*/
extern int musicMarker;/*how far the music has got?*/
extern int (*drvDrawScaledRect)();/*the graphics driver: draws a shape, reports its rectangle*/
extern void (pascal *drvNote)();/*the sound driver (music.c): a note (channel, note, velocity; 0: off)*/
extern int (*drvCopySurface)();/*the graphics driver: shows a surface*/
extern int (*drvDrawShape)();/*the graphics driver: draws a shape*/
extern int (*drvDrawScaled)();/*the graphics driver: draws a shape, scaled*/
extern int (*drvClearSurface)();
extern int musicOn;/*nonzero: music?*/
/*4330*/extern void far *titleShapes[13];/*the shapes of TITLE.LZC*/
extern void far *sparkleShapes;
extern struct tSprite sparkleList[30];

extern void far *LoadLzc(char *name, int n);/*item n of an LZC file, loaded*/
extern PAL_restore();
extern pascal FADE_out(struct tSurface *, int, int);
extern pascal FADE_in(struct tSurface *, struct tSurface *);
extern StopSound(void far *, int);/*stops a sound*/
extern FadeOutMusic(int);
extern PlaySong(void far *);
extern StopAllSounds();
extern ClearSprites();/*frees all the sparkles*/
extern DrawSprites(struct tSurface *, int n);/*draws the first n sparkles; returns how many are free*/
extern VID_waitRetrace();/*waits for the vertical retrace*/
extern MEM_free(void far *);/*frees*/
extern int DrvCalibrate();
extern int pascal Random(int lo, int hi);/*random number from lo to hi*/
extern int kbhit(void);

/*0C24*/char sunFrames[] = "opoqopoqopoqopoqqrstrq";/*frames of the sun: p, q play a sound*/
/*0C3B*/char *letterFrames[10] = {/*frames of each letter of the title*/
	"abcdefghijkaakkkkaaaalllllllmmll",
	"aaaaaaaabcddeeddccffgghhgghhiiih",
	"aaaaaaaaaabbccbbaaccccccccccaccc",
	"aaaabbbcccddeeddccffggaaggffcaaa",
	"aabbbaacccddddddccddeeeeeeddccca",
	"aaaaaabbbaaabbaaaabbbbaabbbbaccc",
	"aabbcdbbbeaaaaaaeeaaaaaaaaaabaaa",
	"abbbaacccdeeaaeeddaaaaaaaaaadacc",
	"aaaaaaaaabbbccbbbbccddddddccaaaa",
	"abccdefgghiijjiihhkkllmmllkkhnnn"
};
/*0C4F*/struct tLetter letterMoves[10] = {
	{0x3a, 0x5e, -1, 1}, {0xba, 0x5e, 0, 1}, {0x116, 0x5e, 1, 1},
	{0x3a, 0x66, -3, 2}, {0xba, 0x66, 0, 2}, {0x116, 0x66, 3, 2},
	{0x3a, 0x6e, -5, 3}, {0xba, 0x6e, 0, 3}, {0x116, 0x6e, 5, 3},
	{0x9e, 0x4a, 0, 4}
};

/*C_0CD9_0004*/void Show(struct tSurface *surf, struct tSurface *screen) {
	VID_waitRetrace();
	(*drvCopySurface)(surf, screen);
}

/*C_0CD9_001A*/void DrawBackground(struct tSurface *surf, int y) {
	(*drvDrawShape)(surf, 0, 0, titleShapes[0], 0);
	(*drvDrawShape)(surf, 0xa0, y, titleShapes[12], 0);
}

/*C_0CD9_0052*/void TitleFadeIn(struct tSurface *surf, struct tSurface *screen) {
	int i;

	DrawBackground(surf, 0x18);
	for(i = 0; i < 10; i++)
		(*drvDrawShape)(surf, letterMoves[i].x, letterMoves[i].y, titleShapes[i + 1], 0);
	FADE_in(surf, screen);
}

/*the letters of the title change*/
/*C_0CD9_00AD*/void LettersChange(struct tSurface *surf, struct tSurface *screen, int speed) {
	int from;
	int count;
	int t;
	int i;
	int f;

	from = 0;
	count = 0;
	t = 1;
	do {
		for(f = from; f < 0x20; f++) {
			if(--t < 1) {
				t = speed;
				DrawBackground(surf, 0x18);
				for(i = 0; i < 10; i++)
					(*drvDrawShape)(surf, letterMoves[i].x, letterMoves[i].y, titleShapes[i + 1], letterFrames[i][f] - 'a');
				Show(surf, screen);
			}
			if(kbhit())
				escapePressed = 0xff;
			if(musicMarker >= 1)
				return;
			if(escapePressed)
				return;
			count++;
		}
		from = Random(0, 0xd) + 9;
		for(f = 0x1f; f >= from; f--) {
			if(--t < 1) {
				t = speed;
				DrawBackground(surf, 0x18);
				for(i = 0; i < 10; i++)
					(*drvDrawShape)(surf, letterMoves[i].x, letterMoves[i].y, titleShapes[i + 1], letterFrames[i][f] - 'a');
				Show(surf, screen);
			}
			if(kbhit())
				escapePressed = 0xff;
			if(musicMarker >= 1)
				return;
			if(escapePressed)
				return;
			count++;
		}
	} while((musicOn && soundCard != 3) || count <= 30);
}

/*the sun sets*/
/*C_0CD9_0256*/void PlaySunFrames(struct tSurface *surf, struct tSurface *screen, int speed) {
	int i;
	int t;
	int f;

	t = 1;
	for(f = 0; f < 0x14; f++) {
		if(sunFrames[f] == 'p') {
			if(soundCard == 2) {
				t = 1;
				(*drvNote)(0x12, 0x30, 0x60);
			} else if(soundCard == 1) {
				t = 1;
				(*drvNote)(0xa, 0x25, 0x60);
			}
		} else if(sunFrames[f] == 'q') {
			if(soundCard == 2) {
				(*drvNote)(0x12, 0x30, 0);
				t = 1;
			} else if(soundCard == 1) {
				(*drvNote)(0xa, 0x25, 0);
				t = 1;
			}
		}
		if(--t < 1) {
			t = speed;
			DrawBackground(surf, 0x18);
			for(i = 0; i < 9; i++)
				(*drvDrawShape)(surf, letterMoves[i].x, letterMoves[i].y, titleShapes[i + 1], letterFrames[i][0x1f] - 'a');
			(*drvDrawShape)(surf, letterMoves[9].x, letterMoves[9].y, titleShapes[10], sunFrames[f] - 'a');
			Show(surf, screen);
		}
		if(kbhit())
			escapePressed = 0xff;
		if(escapePressed)
			return;
	}
	DrawBackground(surf, 0x18);
	(*drvDrawShape)(surf, letterMoves[0].x, letterMoves[0].y, titleShapes[1], letterFrames[0][0x1f] - 'a');
	(*drvDrawShape)(surf, letterMoves[1].x, letterMoves[1].y, titleShapes[2], letterFrames[1][0x1f] - 'a');
	(*drvDrawShape)(surf, letterMoves[2].x, letterMoves[2].y, titleShapes[3], letterFrames[2][0x1f] - 'a');
	(*drvDrawShape)(surf, letterMoves[3].x, letterMoves[3].y, titleShapes[4], letterFrames[3][0x1f] - 'a');
	(*drvDrawShape)(surf, letterMoves[4].x, letterMoves[4].y, titleShapes[5], letterFrames[4][0x1f] - 'a');
	(*drvDrawShape)(surf, letterMoves[5].x, letterMoves[5].y, titleShapes[6], letterFrames[5][0x1f] - 'a');
	(*drvDrawShape)(surf, letterMoves[6].x, letterMoves[6].y, titleShapes[7], letterFrames[6][0x1f] - 'a');
	(*drvDrawShape)(surf, letterMoves[7].x, letterMoves[7].y, titleShapes[8], letterFrames[7][0x1f] - 'a');
	(*drvDrawShape)(surf, letterMoves[8].x, letterMoves[8].y, titleShapes[9], letterFrames[8][0x1f] - 'a');
	(*drvDrawShape)(surf, letterMoves[9].x, letterMoves[9].y, titleShapes[10], sunFrames[20] - 'a');
	if(musicOn && (soundCard == 2 || soundCard == 1)) {
		do {
			if(kbhit())
				escapePressed = 0xff;
		} while(musicMarker < 2 && escapePressed == 0);
		Show(surf, screen);
	}
}

/*draws the mountains in front of the sun at x, y*/
/*C_0CD9_0565*/void DrawMountains(struct tSurface *surf, int x, int y) {
	int r[4];

	(*drvDrawScaledRect)(surf, 0xa2, x, titleShapes[0], 2, 0, y, 0, r);
	(*drvDrawScaled)(surf, r[0], x, titleShapes[0], 1, 0, y, 0);
	(*drvDrawScaled)(surf, 0xa2, x, titleShapes[0], 2, 0, y, 0);
	(*drvDrawScaled)(surf, r[2], x, titleShapes[0], 3, 0, y, 0);
}

/*the letters fly apart*/
/*C_0CD9_05FB*/void LettersFlyApart(struct tSurface *surf, struct tSurface *screen, int speed) {
	int x;
	int y;
	int t;
	int d;
	int i;

	t = 1;
	for(d = 1; d < 0x78; d = d + d / 4 + 1) {
		if(--t < 1) {
			t = speed;
			DrawBackground(surf, 0x18);
			for(i = 0; i < 9; i++) {
				x = letterMoves[i].x + letterMoves[i].dx * d;
				y = letterMoves[i].y + letterMoves[i].dy * d;
				(*drvDrawScaled)(surf, x, y, titleShapes[i + 1], letterFrames[i][0x1f] - 'a', 0, d * 4 * letterMoves[i].dy + 0x100, 0);
			}
			x = letterMoves[9].x + d * letterMoves[9].dx;
			y = letterMoves[9].y + d * letterMoves[9].dy;
			(*drvDrawScaled)(surf, x, y, titleShapes[10], sunFrames[21] - 'a', 0, d * 4 * letterMoves[9].dy + 0x100, 0);
			Show(surf, screen);
		}
		if(kbhit())
			escapePressed = 0xff;
		if(escapePressed)
			break;
	}
}

/*the mountains rise and the sun sets; returns where the mountains are*/
/*C_0CD9_0765*/MountainsRise(struct tSurface *surf, struct tSurface *screen, int speed) {
	int unused;
	int y;
	int sunY;
	int n;
	int c1;
	int c2;
	int moved;
	int behind;
	int r[4];
	int t;
	int x;

	unused = 0;
	behind = 0;
	t = 1;
	x = 0x3b;
	for(n = 5000, c1 = 0, c2 = 0; n >= 1000; n--, c1--, c2--) {
		moved = 0;
		if(c1 == 0) {
			moved = 1;
			sunY = 120000L / n;
			c1 = 200;
		}
		if(c2 == 0) {
			moved = 1;
			y = 256000L / n;
			x = n > 3000 ? x - 2 : x + 2;
			c2 = 100;
		}
		if(moved) {
			if(--t < 1) {
				t = speed;
				(*drvDrawShape)(surf, 0, 0, titleShapes[0], 0);
				(*drvDrawScaledRect)(surf, 0xa2, x, titleShapes[0], 2, 0, y, 0, r);
				if(r[3] < sunY)
					behind = 1;
				if(!behind) {
					DrawMountains(surf, x, y);
					(*drvDrawShape)(surf, 0xa0, sunY, titleShapes[12], 0);
				} else {
					(*drvDrawShape)(surf, 0xa0, sunY, titleShapes[12], 0);
					DrawMountains(surf, x, y);
				}
				Show(surf, screen);
			}
		}
		if(kbhit())
			escapePressed = 0xff;
		if(escapePressed)
			return;
	}
	(*drvDrawShape)(surf, 0, 0, titleShapes[0], 0);
	DrawMountains(surf, x, y);
	Show(surf, screen);
	return x;
}

/*sparkles over the mountains*/
/*C_0CD9_0921*/void TitleSparkles(struct tSurface *surf, struct tSurface *screen, int x) {
	int j;
	int nFree;
	int done;
	int add;
	int count;
	int i;

	done = 0;
	count = 0;
	StopAllSounds();
	ClearSprites();
	do {
		if(!done) {
			/*DIFFERS: the original does not merge the end of "add = Random(0, 5) == 0"
			  (0636:0977-0993) with the same code of the last case; this merges it (9 bytes
			  shorter)*/
			if(musicOn && (soundCard == 2 || soundCard == 1)) {
				if(musicMarker >= 4)
					add = Random(0, 0) == 0;
				else
					add = Random(0, 5) == 0;
			} else
				add = Random(0, 0) == 0;
		}
		if(add && !done) {
			for(i = 0; i < 5; i++) {
				if(sparkleList[i].frame == -1) {
					sparkleList[i].frame = 0;
					sparkleList[i].x = Random(0, 0x13f);
					sparkleList[i].y = Random(0, 0x7f);
					sparkleList[i].kind = Random(0, 2);
					break;
				}
			}
		}
		(*drvDrawShape)(surf, 0, 0, titleShapes[0], 0);
		nFree = DrawSprites(surf, 5);
		DrawMountains(surf, x, 0x100);
		Show(surf, screen);
		count++;
		if(done == 1 && nFree == 5)
			goto all;
		/*DIFFERS: the original keeps both "done = 1" (0636:0A9A, 0AA7); this merges them
		  (2 bytes shorter)*/
		if(musicOn && (soundCard == 2 || soundCard == 1)) {
			if(musicMarker >= 5)
				done = 1;
		} else if(count > 10)
			done = 1;
		if(kbhit())
			escapePressed = 0xff;
	} while(escapePressed == 0);
	return;
all:
	for(i = 0; i < 0x1d; i++) {
		sparkleList[i].frame = 0;
		sparkleList[i].x = Random(0, 0x13f);
		sparkleList[i].y = Random(0, 0x7f);
		sparkleList[i].kind = Random(0, 2);
	}
	nFree = 0;
	j = 0;
	while(nFree != 0x1e) {
		if(++j == 4) {
			sparkleList[29].frame = 0;
			sparkleList[29].x = Random(0, 0x13f);
			sparkleList[29].y = Random(0, 0x7f);
			sparkleList[29].kind = Random(0, 2);
		}
		(*drvDrawShape)(surf, 0, 0, titleShapes[0], 0);
		nFree = DrawSprites(surf, 0x1e);
		DrawMountains(surf, x, 0x100);
		Show(surf, screen);
		if(kbhit())
			escapePressed = 0xff;
		if(escapePressed)
			return;
	}
	for(i = 0x1d; i >= 0; i--) {
		if(sparkleList[i].frame != -1)
			StopSound(sparkleList[i].sound, i);
	}
}

/*the number of frames between two pictures, from the speed of the computer*/
/*C_0CD9_0C2A*/TitleSpeed() {
	return 2 - DrvCalibrate() / 8 < 1 ? 1 : 2 - DrvCalibrate() / 8;
}

/*C_0CD9_0C5B*/void far *LoadShape(int n) {
	return LoadLzc("TITLE.LZC", n);
}

/*plays the title on surf, shown on screen; mode: the video mode (3: shapes 7-9 are not
  loaded); music: the song to play during it (or 0); speed: frames between two pictures
  (the caller's TitleSpeed())*/
/*C_0CD9_0C6E*/void PlayTitle(struct tSurface *surf, struct tSurface *screen, int mode, void far *music, int speed) {
	struct tSurface save;
	int i;

	save = *screen;
	screen->x0 = 0x18;
	screen->y0 = 0x97;
	for(i = 0; i < 12; i++) {
		if(mode != 3)
			titleShapes[i] = LoadShape(i);
		else if(i < 7 || i > 9)
			titleShapes[i] = LoadShape(i);
	}
	sparkleShapes = titleShapes[11];
	titleShapes[12] = LoadShape(12);
	escapePressed = 0;
	if(music)
		PlaySong(music);
	TitleFadeIn(surf, screen);
	if(escapePressed == 0xff)
		goto end;
	LettersChange(surf, screen, speed);
	if(escapePressed == 0xff)
		goto end;
	PlaySunFrames(surf, screen, speed);
	if(escapePressed == 0xff)
		goto end;
	LettersFlyApart(surf, screen, speed);
	if(escapePressed != 0xff) {
		i = MountainsRise(surf, screen, speed);
		if(musicOn && (soundCard == 2 || soundCard == 1)) {
			while(musicMarker < 3) {
				if(kbhit())
					escapePressed = 0xff;
				if(escapePressed == 0xff)
					break;
			}
		}
		if(escapePressed != 0xff)
			TitleSparkles(surf, screen, i);
	}
end:
	escapePressed = 0;
	if(music)
		FadeOutMusic(0);
	FADE_out(&save, 0, 1);
	(*drvClearSurface)(&save, 0);
	PAL_restore();
	for(i = 0; i < 13; i++)
		MEM_free(titleShapes[i]);
	if(music)
		MEM_free(music);
	*screen = save;
}
