/* flags: -mm -c -O -G -y */
/* MARTIAN.EXE's main module (04FE): the start (configuration, memory, drivers, the
   menu), loading helpers, the introduction (Intro, PlayIntro), character creation
   (PlayDialogue, CreateCharacter), the endgame and the closing credits.
   Written from the disassembly: Savage Empire's main.c (SRC_S/main.c) has the shape of
   main() but little of its code. No -d: repeated literals ("fair.lzc", "endgame.lzc")
   are separate strings in the original. Delay and Exit are called Pause and ExitGame here:
   the link is case-insensitive, so those names would take the place of the library's
   delay() and exit(). */
#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <math.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*a drawing surface of the OSI library (16 bytes)*/
struct tSurface {
	int _00, _02;
	int left, top, right, bottom;/*+04..+0A the clipping rectangle*/
	int _0c, _0e;
};

/*a text window of the OSI library (17 bytes)*/
struct tText {
	struct tSurface *surface;/*+00*/
	int curX, curY;/*+02,+04*/
	void far *font;/*+06*/
	unsigned char fg, bg;/*+0A,+0B*/
	char *buffer;/*+0C*/
	char *bufPtr;/*+0E*/
	char justify;/*+10*/
};

/*a point*/
struct tPoint {
	int x, y;
};

/*a step of the moongate scene (scene 3): text, its colour, frames*/
struct tStep {
	int text;/*+0 its line in scenetxt.lzc*/
	int color;/*+2*/
	int frame;/*+4 <0: frame -frame-1 of the close-up instead*/
	int frame2;/*+6*/
	int delay;/*+8 ms*/
};

/*a line of the character creation dialogue (scenetxt.lzc item 5)*/
struct tLine {
	int text;/*+0 its line (<0: the other speaker, colour 4); 0 ends a part*/
	int frame;/*+2 the speaker's frame (create.lzc 0)*/
	int animate;/*+4 the moving shape and the speaker's poses animate while it shows*/
	int wait;/*+6 >=0: until this many seconds from the start or a key;
		-1 a key, -2 a or b, -3 a, b or c, -4 the name*/
};

/*00A8*/char D_00A8[] = "MARTIAN DREAMS (c) 1991 Origin Systems, All Rights Reserved.";/*unused*/
/*00E5*/struct tPoint D_00E5[4] = {{26, 130}, {26, 147}, {26, 164}, {26, 181}};/*the main menu's buttons*/
/*00F5*/struct tPoint D_00F5[13] = {/*the cannon shot's path (scene 2)*/
	{190, 80}, {180, 82}, {170, 84}, {162, 81}, {154, 75}, {145, 73}, {135, 73},
	{125, 74}, {115, 76}, {105, 78}, {95, 81}, {86, 86}, {0, 0}
};
/*0129*/int D_0129 = 0;/*unused*/
/*012B*/int D_012B[15] = {4, 14, 14, 2, 14, 2, 9, 9, 2, 9, 2, 14, 2, 2, 14};/*text colours of scene 1, by line*/
/*0149*/struct tStep D_0149[10] = {/*scene 3*/
	{1, 2, 0, 0, 1000}, {1, 2, 0, 1, 4000}, {2, 14, 0, 1, 4000}, {3, 2, 0, 2, 4000},
	{4, 2, -1, 0, 4000}, {5, 14, -2, 0, 4000}, {6, 2, -1, 0, 4000}, {7, 14, 1, 3, 4000},
	{8, 2, 1, 2, 4000}, {9, 14, 0, 2, 4000}
};
/*01AD*/int D_01AD[27] = {4, 6, 7, 7, 14, 2, 2, 7, 4, 4, 7, 9, 7, 4, 7, 4, 7, 2, 7, 7, 7, 7, 7, 11, 2, 7, 7};/*text colours of scene 4*/
/*01E3*/struct tLine D_01E3[93] = {/*the character creation dialogue*/
	{-1, 1, 0, -1}, {-2, 1, 0, -1}, {3, 2, 0, -1}, {4, 3, 1, -1}, {5, 2, 0, -4}, {6, 3, 1, -1},
	{7, 1, 0, -2}, {0, 2, 0, -1}, {8, 3, 1, -1}, {9, 1, 0, -1}, {0, 2, 0, -1}, {10, 3, 1, -1},
	{11, 1, 0, -1}, {0, 2, 0, -1}, {12, 2, 0, -1}, {13, 1, 0, -1}, {14, 1, 0, -3}, {0, 2, 0, -1},
	{15, 3, 1, -1}, {0, 2, 0, -1}, {16, 3, 1, -1}, {17, 1, 0, -2}, {0, 2, 0, -1}, {18, 3, 1, -1},
	{19, 2, 0, -2}, {0, 2, 0, -1}, {20, 3, 1, -1}, {21, 2, 0, -2}, {0, 2, 0, -1}, {22, 3, 1, -2},
	{0, 1, 0, -1}, {23, 3, 1, -1}, {24, 2, 0, -1}, {0, 2, 0, -1}, {25, 3, 1, -1}, {26, 3, 1, -1},
	{0, 2, 0, -1}, {27, 3, 1, -1}, {28, 1, 0, -2}, {0, 2, 0, -1}, {29, 3, 1, -1}, {30, 2, 0, -1},
	{0, 2, 0, -1}, {31, 3, 1, -1}, {32, 2, 0, -1}, {0, 2, 0, -1}, {33, 3, 1, -1}, {34, 1, 0, -2},
	{0, 2, 0, -1}, {35, 3, 1, -1}, {36, 1, 0, -1}, {0, 2, 0, -1}, {37, 3, 1, -1}, {38, 2, 0, -1},
	{0, 2, 0, -1}, {39, 3, 1, -1}, {40, 2, 0, -1}, {0, 2, 0, -1}, {41, 3, 1, -1}, {42, 2, 0, -1},
	{0, 2, 0, -1}, {43, 3, 1, -1}, {44, 1, 0, -2}, {0, 2, 0, -1}, {45, 3, 1, -1}, {0, 2, 0, -1},
	{46, 3, 1, -1}, {47, 1, 0, -1}, {0, 2, 0, -1}, {48, 3, 1, -1}, {49, 1, 0, -1}, {0, 2, 0, -1},
	{50, 3, 1, -1}, {51, 2, 0, -2}, {0, 2, 0, -1}, {52, 3, 1, -1}, {53, 2, 0, -1}, {0, 2, 0, -1},
	{54, 1, 0, -1}, {55, 3, 1, -1}, {0, 2, 0, -1}, {56, 2, 0, -1}, {57, 3, 1, -1}, {0, 2, 0, -1},
	{58, 3, 1, -1}, {59, 2, 0, -2}, {0, 2, 0, -1}, {60, 3, 1, -1}, {61, 1, 0, -1}, {0, 2, 0, -1},
	{62, 3, 1, -1}, {63, 1, 0, -1}, {0, 2, 0, -1}
};
/*04CB*/char D_04CB[8] = {0x00, 0x74, 0x0C, 0x7F, 0x3C, 0x00, 0x00, 0x00};/*a sound: the flash at the fair*/
/*04D3*/char D_04D3[8] = {0x04, 0x2A, 0x30, 0x7F, 0x3C, 0x00, 0x00, 0x00};/*a sound (PlayIntro)*/

extern int soundCard;/*music.c*/
extern void (far *D_0E9A)();/*driver: draw a shape (surface, x, y, shapes, n)*/
extern void (far *D_0EAA)();/*driver: copy a surface onto another (from, to)*/
extern void (far *D_0EAE)();/*driver: clear a surface (surface, colour)*/
extern void (far *D_0EC6)();/*driver (ScrollCredits)*/
extern void (far *D_0ED2)();/*driver (PlayIntro)*/
extern void (far *D_0EDA)();/*driver: draw a shape rotated and scaled (surface, x, y, shapes, n, angle, scale, flags)*/
extern unsigned char D_0BC0[0x62];/*objlist's header: another module's initialised data*/
extern unsigned char D_0BF6, D_0BF7;/*in it: the gender, a character exists*/

/*uninitialised data*/
extern int D_2382;
extern struct tSurface D_236C;/*the screen*/
extern struct tSurface D_2384;/*the back buffer's picture area (y 24-151)*/
extern struct tText D_2394;/*the text window*/
extern struct tSurface D_23A6;/*the back buffer*/
extern int D_23B6;
extern int D_23B8;/*graphic adapter: 0 VGA, 1 EGA, 3 Tandy*/
extern long D_23BA;/*time() now*/
extern int D_23C2;/*speed of the machine (Calibrate); the intro draws every D_23C2-th frame*/
extern void far *D_23C4, far *D_23C8, far *D_23CC, far *D_23D0;/*mdd_mus.lzc 0..7*/
extern void far *D_23D4, far *D_23D8, far *D_23DC, far *D_23E0;
extern int D_23E4;/*'!': endgame only*/
extern char **D_23E6;/*argv*/
extern int D_23E8;/*X: skip the introduction*/
extern int D_23EE;
extern int D_23F0;/*M mouse, K keyboard*/
extern char D_23F2[];/*the text window's buffer*/
extern unsigned char D_24F6[0x1cf1];/*savegame\objlist*/
extern unsigned D_28F8;/*objlist +0x402: the avatar's shape*/
extern unsigned char D_2DF7, D_2EF7, D_2FF7;/*the avatar's strength, dexterity, intelligence*/
extern unsigned D_30F8;/*experience*/
extern unsigned char D_32F7, D_34E8, D_3AE8;/*hit points, level*/
extern char D_33F6[];/*the avatar's name*/
extern unsigned D_3BE7, D_3BE9;
extern void far *D_41F4;/*strx_mus.lzc 0*/
extern int D_41F8;
extern void far *D_4210, far *D_4214, far *D_4218, far *D_421C;/*mdd_mus.lzc 8..11*/
extern long D_4220;/*calibration count*/
extern long D_4224;/*time() at the start*/
extern int D_422A;/*music on*/
extern void far *D_422C;/*the font*/

extern long pascal LIB_entrySize(char far *name, int n);
extern void far * pascal LZC_loadType(char far *name, int n, void far *buf, int a, long b);
extern void far *MEM_alloc(long size, int flags);
extern void MEM_free(void far *p);/*C_1339_0135*/
extern void MEM_reset(int);
extern void MEM_init(int);
extern void MEM_shutdown(int);
extern int pascal DOS_open(char far *name);
extern void pascal DOS_close(int fd);
extern long pascal DOS_read(int fd, long offset, long length, void far *buf);
extern void pascal PAL_loadFile(char *name);/*load a palette file*/
extern int LoadSuperTM(int adapter);
extern void PAL_save(void);
extern void PAL_restore(void);
extern void pascal TXT_select(struct tText *t);
extern void pascal TXT_gotoxy(int x, int y);
extern void pascal TXT_puts(char far *s);
extern void pascal VID_setMode(int mode);
extern void VID_waitRetrace(void);
extern int pascal FADE_in(struct tSurface *from, struct tSurface *to);
extern void pascal FADE_out(struct tSurface *s, int a, int b);
extern void far *PlaySound(void far *steps, int flags, int volume, int pan, int id, int prio);
extern void StopSound(void far *v, int id);
extern void FadeOutMusic(int time);
extern void InitSound(char far *name);
extern void ShutdownSound(void);
extern int PlaySong(void far *song);
extern void WaitMusicFlags(int mask);
extern long EMS_totalFree(void);
extern int DrvCalibrate(void);
extern int pascal Random(int lo, int hi);
extern void pascal SeedRandom(int seed);
extern int pascal ScreenSurface(struct tSurface *s, int x);
extern int pascal BufferSurface(struct tSurface *s, int x);
extern void PlayTitle(struct tSurface *, struct tSurface *, int adapter, void far *song, int speed);

int Fatal(int code, char *msg);
int ExitGame(int code);
int FadeOut(void);
int FadeIn(void);
int Endgame(void);
int Intro(void);
int CreateCharacter(void);
int PlayIntro(void);
int ScrollCredits(void);

/*load a palette file (VGA) and save it*/
/* strings: none; data: D_23B8 */
/*C_04FE_0004*/void LoadPalette(char *name) {
	if(D_23B8 == 0) {
		PAL_loadFile(name);
		PAL_save();
	}
}

/*measure the machine: count until time() ticks over; *speed from the count*/
/* data: D_4224 (long), D_23BA (long) */
/*C_04FE_001D*/long Calibrate(long *count, int *speed) {
	time(&D_4224);
	*count = 0;
	while(time(&D_23BA) == D_4224)
		;
	D_4224++;
	while(time(&D_23BA) == D_4224)
		(*count)++;
	*speed = sqrt(max(1L, 25 - *count / 100));
	return *count;
}

/*load item n of an .lzc archive (allocating with flags); a fatal error if it fails*/
/* strings: 04DB "Bad data packet.", 04EC "Insufficient memory.", 0501 "Unable to load data packet." */
/*C_04FE_00F5*/void far *LoadLzcFlags(char *name, int n, int flags) {
	long size;
	void far *buf;

	size = LIB_entrySize(name, n);
	if(size < 0)
		Fatal(1, "Bad data packet.");
	buf = MEM_alloc(size, flags);
	if(buf == 0)
		Fatal(2, "Insufficient memory.");
	buf = LZC_loadType(name, n, buf, 0, 0L);
	if(buf == 0)
		Fatal(1, "Unable to load data packet.");
	return buf;
}

/*load item n of an .lzc archive*/
/*C_04FE_0197*/void far *LoadLzc(char *name, int n) {
	return LoadLzcFlags(name, n, 0);
}

/*the same, allocating with flag 4*/
/*C_04FE_01AC*/void far *LoadLzc4(char *name, int n) {
	return LoadLzcFlags(name, n, 4);
}

/*set up a text window on a surface with the font in item n of name*/
/* data: D_422C, D_23F2 */
/*C_04FE_01C2*/void InitText(struct tText *t, struct tSurface *s, char *name, int n) {
	D_422C = LoadLzc(name, n);
	t->surface = s;
	t->curX = 0;
	t->curY = 0;
	t->fg = 7;
	t->bg = 0xff;
	t->font = D_422C;
	t->buffer = D_23F2;
	t->bufPtr = t->buffer;
	t->justify = 0;
}

/*load item n of a text archive and split it into lines (ended by CR LF):
lines[] gets a pointer to each, then 0; returns the block*/
/*C_04FE_0213*/char far *ReadLines(char *name, int n, char far **lines) {
	char far *buf;
	long size;
	register int i;

	buf = LoadLzc(name, n);
	size = LIB_entrySize(name, n);
	i = 0;
	lines[i] = buf;
	do {
		i++;
		lines[i] = lines[i - 1];
		while(*lines[i]++ != '\r')
			;
		lines[i][-1] = 0;
		lines[i]++;
	} while((long)FP_OFF(lines[i]) - FP_OFF(buf) < size);
	lines[i] = 0;
	return buf;
}

/*print s in a text window from (x, y), a line for each part between '^'s,
12 pixels apart, in colour color with justification justify*/
/* data: D_23F2 */
/*C_04FE_02D5*/void PrintText(struct tText *t, int x, int y, char far *s, char justify, char color) {
	char fg;
	char just;
	char *buffer;
	char *p;
	int cy;
	int i;
	register char *q;

	fg = t->fg;
	buffer = t->buffer;
	just = t->justify;
	p = D_23F2;
	for(i = 0; s[i] != 0; i++)
		p[i] = s[i];
	p[i] = 0;
	q = p;
	cy = y;
	while(*p != 0) {
		while(*q != '^' && *q != 0)
			q++;
		if(*q == '^') {
			*q = 0;
			q++;
		}
		TXT_select(t);
		TXT_gotoxy(x, cy);
		t->fg = color;
		t->buffer = p;
		t->justify = justify;
		TXT_puts(p);
		p = q;
		cy += 12;
	}
	t->fg = fg;
	t->buffer = buffer;
	t->justify = just;
}

/*control-break: quit*/
/*C_04FE_0399*/int Quit() {
	ExitGame(3);
	return 0;
}

/*fatal error: back to text mode, print msg, exit with code*/
/*C_04FE_03AB*/int Fatal(int code, char *msg) {
	FadeOut();
	VID_setMode(3);
	puts(msg);
	ShutdownSound();
	MEM_reset(8);
	MEM_shutdown(8);
	exit(code);
	return 0;
}

/*back to text mode, exit with code*/
/*C_04FE_03EF*/int ExitGame(int code) {
	FadeOut();
	VID_setMode(3);
	ShutdownSound();
	MEM_reset(8);
	MEM_shutdown(8);
	exit(code);
	return 0;
}

/*load the songs; 0 if there is no music*/
/* strings: 051D "strx_mus.lzc", 052A..05AE "mdd_mus.lzc" (12 times);
   data: D_422A, soundCard soundCard, D_41F4, D_23C4..D_23E0, D_4210..D_421C */
/*C_04FE_0429*/int LoadMusic() {
	if(D_422A == 0 || soundCard == 0)
		return 0;
	if(EMS_totalFree() <= 0)
		return 0;
	D_41F4 = LoadLzc4("strx_mus.lzc", 0);
	D_23C4 = LoadLzc4("mdd_mus.lzc", 0);
	D_23C8 = LoadLzc4("mdd_mus.lzc", 1);
	D_23CC = LoadLzc4("mdd_mus.lzc", 2);
	D_23D0 = LoadLzc4("mdd_mus.lzc", 3);
	D_23D4 = LoadLzc4("mdd_mus.lzc", 4);
	D_23D8 = LoadLzc4("mdd_mus.lzc", 5);
	D_23DC = LoadLzc4("mdd_mus.lzc", 6);
	D_23E0 = LoadLzc4("mdd_mus.lzc", 7);
	D_4210 = LoadLzc4("mdd_mus.lzc", 8);
	D_4214 = LoadLzc4("mdd_mus.lzc", 9);
	D_4218 = LoadLzc4("mdd_mus.lzc", 10);
	D_421C = LoadLzc4("mdd_mus.lzc", 11);
	if(D_41F4 == 0 || D_421C == 0)
		return 0;
	return 1;
}

/*read a key: extended keys are 0x100 + their scan code*/
/*C_04FE_0588*/int GetKey() {
	int key;

	key = getch();
	if(key == 0 && kbhit())
		key = getch() + 0x100;
	return key;
}

/*empty the keyboard buffer*/
/*C_04FE_05AF*/int FlushKeys() {
	while(kbhit())
		GetKey();
	return 0;
}

/*fade the screen out (VGA), clear it, restore the palette*/
/* data: D_23B8, D_236C */
/*C_04FE_05C5*/int FadeOut() {
	if(D_23B8 == 0)
		FADE_out(&D_236C, 0, 1);
	VID_waitRetrace();
	(*D_0EAE)(&D_236C, 0);
	PAL_restore();
	return 0;
}

/*show the back buffer, fading in (VGA)*/
/* data: D_23B8, D_23A6, D_236C */
/*C_04FE_05FB*/int FadeIn() {
	if(D_23B8 == 0) {
		if(!FADE_in(&D_23A6, &D_236C))
			0;/*a dropped statement: keeps the original's jne/jmp*/
		else
			VID_waitRetrace();
		(*D_0EAA)(&D_23A6, &D_236C);
	}
	return 0;
}

/*wait ms milliseconds (ms < 0: until a key), stopped by a key; then read the keys:
-1 escape (Esc, ^E, ^Q, Alt-X), 1 a key was pressed (ms >= 0), else 0*/
/*C_04FE_0630*/int Pause(int ms) {
	int r;
	register int t, key;

	if(ms < 0) {
		while(!kbhit())
			;
	} else {
		t = ms;
		while(t > 0) {
			delay(t > 240 ? 240 : t);
			t -= 250;
			if(kbhit())
				break;
		}
	}
	r = 0;
	while(kbhit()) {
		if(ms >= 0)
			r = 1;
		key = getch();
		if(key == 0)
			key = getch() + 0x100;
		if(key == 0x1b || key == 5 || key == 0x11 || key == 0x12d)
			return -1;
	}
	return r;
}

/*parse a configuration string (CONFIG.MD or a command line argument), up to
its end or a CR: V/E/T graphic adapter, M/K mouse or keyboard, N/P/A/R music
(none, none, AdLib, Roland), X skip the introduction, ! the endgame*/
/* data: D_23B8, D_23F0, D_422A, soundCard soundCard, D_23E8, D_23E4 */
/*C_04FE_06C6*/void ParseConfig(char *s) {
	while(*s != 0 && *s != '\r') {
		switch(toupper(*s++)) {
			case 'V': D_23B8 = 0; break;
			case 'E': D_23B8 = 1; break;
			case 'T': D_23B8 = 3; break;
			case 'M': D_23F0 = 1; break;
			case 'K': D_23F0 = 0; break;
			case 'N': case 'P': D_422A = 0; soundCard = 0; break;
			case 'A': D_422A = 1; soundCard = 2; break;
			case 'R': D_422A = 1; soundCard = 1; break;
			case 'X': D_23E8 = 1; break;
			case '!': D_23E4 = 1; break;
		}
	}
}

/* strings: 05BA "config.md", 05C4 "fonts.lzc", 05CE "md.tim", 05D5 "md_title.pal",
   05E2 "strax.pal", 05EC "md_title.pal", 05F9 "mdmenu.lzc", 0604 "mdmenu.lzc", 060F "game.exe";
   data: D_00E5 (menu buttons), D_23E6, D_4220, D_23C2, D_2382, D_23EE, soundCard soundCard,
   D_23B6, D_41F8, D_23F0, D_422A, D_23E8, D_23E4, D_23B8, D_236C, D_23A6, D_2384, D_2394,
   D_41F4, D_4224, D_23BA */
/*C_04FE_0797*/main(int argc, char **argv) {
	int prev;
	int reload;
	int i;
	int fd;
	void far *shapes;
	char cfg[10];
	unsigned now[2];
	register int sel, key;

	D_23E6 = argv;
	setcbrk(0);
	ctrlbrk(Quit);
	Calibrate(&D_4220, &D_23C2);
	gettime((struct time *)now);
	MEM_init(8);
	SeedRandom(now[1]);
	for(i = 0; i < now[1]; i++)
		Random(0, 2);
	D_2382 = 0x182;
	D_23EE = 2;
	soundCard = 0;
	D_23B6 = 1;
	D_41F8 = 1;
	D_23F0 = 0;
	D_422A = 0;
	D_23E8 = 0;
	D_23E4 = 0;
	fd = DOS_open("config.md");
	if(fd >= 0) {
		DOS_read(fd, 0L, 6L, cfg);
		cfg[6] = 0;
		ParseConfig(cfg);
		DOS_close(fd);
	}
	for(i = 1; i < argc; i++)
		ParseConfig(argv[i]);
	switch(D_23B8) {
		case 0: VID_setMode(0x13); break;
		case 1: VID_setMode(0xd); break;
	}
	if(LoadSuperTM(D_23B8) == 0) {
		MEM_reset(8);
		MEM_shutdown(8);
		exit(1);
	}
	ScreenSurface(&D_236C, 0);
	BufferSurface(&D_23A6, 0);
	BufferSurface(&D_2384, 0);
	D_2384 = D_23A6;
	D_2384.top = 0x18;
	D_2384.bottom = 0x97;
	InitText(&D_2394, &D_23A6, "fonts.lzc", 0);
	InitSound("md.tim");
	if(LoadMusic())
		D_422A = 1;
	else
		D_422A = 0;
	if(max(1, 2 - DrvCalibrate() / 8) != 1)
		D_23EE = 0;
	if(D_23E4 == 1) {
		LoadPalette("md_title.pal");
		Endgame();
		FadeOutMusic(0);
		ScrollCredits();
		ExitGame(0);
	}
	LoadPalette("strax.pal");
	PlayTitle(&D_23A6, &D_236C, D_23B8, D_422A ? D_41F4 : (void far *)0, D_23C2);
	FadeOutMusic(0);
	LoadPalette("md_title.pal");
	if(D_23E8 == 0) {
		Intro();
		FadeOutMusic(0);
	}
	shapes = LoadLzc("mdmenu.lzc", 0);
	reload = 0;
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_23A6, 0, 0, shapes, 0);
	VID_waitRetrace();
	FadeIn();
	sel = 0;
	prev = 0;
	do {
		if(reload) {
			shapes = LoadLzc("mdmenu.lzc", 0);
			reload = 0;
		}
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_23A6, 0, 0, shapes, 0);
		(*D_0E9A)(&D_23A6, D_00E5[prev].x, D_00E5[prev].y, shapes, 1);
		(*D_0E9A)(&D_23A6, D_00E5[sel].x, D_00E5[sel].y, shapes, 2);
		VID_waitRetrace();
		(*D_0EAA)(&D_23A6, &D_236C);
		time(&D_4224);
		while(!kbhit()) {
			if(time(&D_23BA) > D_4224 + 120) {
				key = 8;
				break;
			}
		}
		if(kbhit())
			key = GetKey();
		prev = sel;
		switch(key) {
			case 0x14d:
			case 0x150:
				sel = sel == 3 ? 0 : sel + 1;
				break;
			case 0x14b:
			case 0x148:
				sel = sel == 0 ? 3 : sel - 1;
				break;
			case 8:
				MEM_free(shapes);
				Intro();
				FadeOutMusic(0);
				reload = 1;
				break;
			case 0xd:
				switch(sel) {
					case 0:
						MEM_free(shapes);
						PlayIntro();
						FadeOutMusic(0);
						reload = 1;
						break;
					case 1:
						MEM_free(shapes);
						CreateCharacter();
						FadeOutMusic(0);
						reload = 1;
						break;
					case 2:
						FadeOut();
						VID_setMode(3);
						ShutdownSound();
						MEM_reset(8);
						MEM_shutdown(8);
						execl("game.exe", 0);
						break;
					case 3:
						MEM_free(shapes);
						ScrollCredits();
						FadeOutMusic(0);
						reload = 1;
						break;
				}
				break;
		}
	} while(key != 5 && key != 0x11 && key != 0x12d);
	MEM_free(shapes);
	ExitGame(0);
	return 0;
}

/*the endgame (scenetxt.lzc item 3, endgame.lzc, moongate.lzc): 1 if objlist
cannot be read, 0 if stopped by a key, -1 at its end*/
/* strings: 0618 "savegame\\objlist", 0629 "rb", 062C "scenetxt.lzc", 0639 0645 0651 065D 0669
   "endgame.lzc", 0675 "moongate.lzc", 0682 068E 069A 06A6 "endgame.lzc";
   data: D_24F6 (0x1CF1 bytes), D_28F8, D_422A, D_421C, D_23A6, D_2384, D_2394, D_236C */
/*C_04FE_0CA4*/int Endgame() {
	void far *shapes;
	void far *gate;
	void far *ship;
	void far *shapes7;
	void far *shapes8;
	char far *txt;
	int j;
	FILE *fp;
	int avatar;
	char far *text[20];
	register int i, key;

	fp = fopen("savegame\\objlist", "rb");
	if(fread(D_24F6, 0x1cf1, 1, fp) < 1)
		return 1;
	fclose(fp);
	if((D_28F8 & 0x3ff) == 0x157)
		avatar = 0;
	else
		avatar = 1;
	txt = ReadLines("scenetxt.lzc", 3, text);
	shapes = LoadLzc("endgame.lzc", 0);
	if(D_422A == 1) {
		FadeOutMusic(0);
		PlaySong(D_421C);
	}
	for(i = 0; i < 2; i++) {
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_2384, 0, 24, shapes, 0);
		(*D_0E9A)(&D_2384, 155, 151, shapes, avatar + 1);
		PrintText(&D_2394, 0, 160, text[i], 2, 4);
		if(i > 0) {
			VID_waitRetrace();
			(*D_0EAA)(&D_23A6, &D_236C);
		} else
			FadeIn();
		if((key = Pause(6000)) == -1) {
			MEM_free(shapes);
			MEM_free(txt);
			return 0;
		}
	}
	MEM_free(shapes);
	shapes = LoadLzc("endgame.lzc", 1);
	key = 0;
	for(i = -1; i < 40 && key == 0; i++) {
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_2384, (i + 2) % 5 - 2, (i + 3) % 4 + 23, shapes, 0);
		if(i > -1 && i < 13) {
			(*D_0EDA)(&D_2384, i * i * 2 / 3 + 105, 112 - i * i, shapes, 1, 0, i * i * 2 + 16, 0);
			(*D_0E9A)(&D_2384, 105, 112, shapes, i / 2 + 2);
		}
		PrintText(&D_2394, 0, 160, text[2], 2, 4);
		if(i > -1) {
			VID_waitRetrace();
			(*D_0EAA)(&D_23A6, &D_236C);
		} else if(!FADE_in(&D_23A6, &D_236C))
			(*D_0EAA)(&D_23A6, &D_236C);
		if((key = Pause(100)) == -1) {
			MEM_free(shapes);
			MEM_free(txt);
			return 0;
		}
	}
	if(key == 0 && Pause(2000) == -1) {
		MEM_free(shapes);
		MEM_free(txt);
		return 0;
	}
	MEM_free(shapes);
	shapes = LoadLzc("endgame.lzc", 2);
	key = 0;
	for(j = 0; j < 10 && key == 0; j++) {
		for(i = 0; i < 4 && key == 0; i++) {
			(*D_0EAE)(&D_23A6, 0);
			(*D_0E9A)(&D_2384, 0, 24, shapes, 0);
			(*D_0E9A)(&D_2384, 170, 150, shapes, abs(2 - i) + 1);
			PrintText(&D_2394, 0, 160, text[(j > 4) + 3], 2, 4);
			if(j > 0 || i > 0) {
				VID_waitRetrace();
				(*D_0EAA)(&D_23A6, &D_236C);
			} else if(!FADE_in(&D_23A6, &D_236C))
				(*D_0EAA)(&D_23A6, &D_236C);
			if((key = Pause(300)) == -1) {
				MEM_free(shapes);
				MEM_free(txt);
				return 0;
			}
		}
	}
	MEM_free(shapes);
	shapes = LoadLzc("endgame.lzc", 3);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, shapes, 0);
	(*D_0E9A)(&D_2384, 190, 151, shapes, avatar + 1);
	PrintText(&D_2394, 0, 160, text[5], 2, 4);
	if(!FADE_in(&D_23A6, &D_236C))
		(*D_0EAA)(&D_23A6, &D_236C);
	if(Pause(6000) == -1) {
		MEM_free(shapes);
		MEM_free(txt);
		return 0;
	}
	FadeOut();
	MEM_free(shapes);
	shapes = LoadLzc("endgame.lzc", 4);
	gate = LoadLzc("moongate.lzc", 1);
	key = 0;
	for(i = 0; i < 80 && key == 0; i++) {
		(*D_0EAE)(&D_23A6, 0);
		if(i == 40) {
			(*D_0E9A)(&D_2384, 0, 24, shapes, 5);
			(*D_0EAA)(&D_23A6, &D_236C);
			FadeOut();
			if((key = Pause(10)) == -1) {
				MEM_free(shapes);
				MEM_free(gate);
				MEM_free(txt);
				return 0;
			}
		} else {
			(*D_0E9A)(&D_2384, 0, 24, shapes, 0);
			(*D_0E9A)(&D_2384, 35, 135, gate, i % 8);
			(*D_0E9A)(&D_2384, 195, 151, shapes, avatar + 1);
			(*D_0E9A)(&D_2384, 75, 151, shapes, (i >= 38 && i <= 42) + 3);
			PrintText(&D_2394, 0, 160, text[(i > 39) + 6], 2, 4);
			VID_waitRetrace();
			if(i > 0 && i != 41) {
				VID_waitRetrace();
				(*D_0EAA)(&D_23A6, &D_236C);
			} else
				FadeIn();
			if((key = Pause(100)) == -1) {
				MEM_free(shapes);
				MEM_free(gate);
				MEM_free(txt);
				return 0;
			}
		}
	}
	FadeOut();
	MEM_free(shapes);
	MEM_free(gate);
	shapes = LoadLzc("endgame.lzc", 5);
	ship = LoadLzc("endgame.lzc", 6);
	shapes7 = LoadLzc("endgame.lzc", 7);
	shapes8 = LoadLzc("endgame.lzc", 8);
	key = 0;
	for(i = 0; i < 63 && key == 0; i++) {
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_2384, 0, 24, shapes, 0);
		(*D_0E9A)(&D_2384, 240, 140, shapes7, i / 8 > 4 ? 4 : i / 8);
		if(i < 56)
			(*D_0E9A)(&D_2384, 0, 140, shapes8, i - 50 < 0 ? 0 : i - 50);
		(*D_0E9A)(&D_2384, 340 - i * 10 - Random(0, 19), 24 - Random(0, 9), ship, 0);
		(*D_0E9A)(&D_2384, 420 - i * 20 - Random(0, 19), 24 - Random(0, 9), ship, 0);
		(*D_0E9A)(&D_2384, 500 - i * 30 - Random(0, 19), 24 - Random(0, 9), ship, 0);
		(*D_0E9A)(&D_2384, 580 - i * 30 - Random(0, 19), 24 - Random(0, 9), ship, 0);
		(*D_0E9A)(&D_2384, 660 - i * 20 - Random(0, 19), 24 - Random(0, 9), ship, 0);
		PrintText(&D_2394, 0, 160, text[(i > 61) + 8], 2, 4);
		if(i > 0) {
			VID_waitRetrace();
			(*D_0EAA)(&D_23A6, &D_236C);
		} else
			FadeIn();
		if((key = Pause(50)) == -1) {
			MEM_free(shapes);
			MEM_free(ship);
			MEM_free(shapes7);
			MEM_free(shapes8);
			MEM_free(txt);
			return 0;
		}
	}
	if(key == 0 && Pause(6000) == -1) {
		MEM_free(shapes);
		MEM_free(ship);
		MEM_free(shapes7);
		MEM_free(shapes8);
		MEM_free(txt);
		return 0;
	}
	if(D_422A == 1)
		FadeOutMusic(120);
	FadeOut();
	MEM_free(shapes);
	MEM_free(ship);
	MEM_free(shapes7);
	MEM_free(shapes8);
	MEM_free(txt);
	return -1;
}

/*the introduction: Origin's logos, the 1893 World's Fair, Lowell, the space
bullet's flight and the cabin, Mars, with the credits in between; Esc (-1 from
Pause) stops it, another key skips to the next scene
(returns -1 when stopped, else 0)*/
/* strings: 06B2 06BC 06C6 "logos.lzc", 06D0 06D9 06E2 06EB 06F4 06FD "fair.lzc",
 0706 "credits.lzc", 0712 "lowell.lzc", 071D "credits.lzc", 0729 "fuse.lzc",
 0732 "credits.lzc", 073E 0747 0750 0759 "fair.lzc", 0762 076C "flash.lzc",
 0776 "credits.lzc", 0782 078D "flight.lzc", 0798 07A2 07AC 07B6 07C0 07CA 07D4 "cabin.lzc",
 07DE "credits.lzc", 07EA "mars.lzc", 07F3 "credits.lzc" (07FF) */
/* data: D_04CB D_0E9A D_0EAA D_0EAE D_0EDA D_236C D_2384 D_23A6 D_23C2
 D_23C4 D_23C8 D_23CC D_23D0 D_23D4 D_23D8 D_23DC D_422A */
/*C_04FE_177E*/int Intro() {
	void far *s0;
	void far *s1;
	void far *s2;
	void far *s3;
	void far *s4;
	void far *s5;
	void far *s6;
	int j;
	int k;
	int music;
	register int i, key;

	s0 = LoadLzc("logos.lzc", 0);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_23A6, 0, 0, s0, 0);
	if(D_422A == 1) {
		FadeOutMusic(0);
		PlaySong(D_23C4);
		music = 1;
	}
	FadeIn();
	while(kbhit())
		getch();
	if((key = Pause(4000)) == -1) {
		MEM_free(s0);
		return -1;
	}
	if(key)
		music = 0;
	MEM_free(s0);
	s0 = LoadLzc("logos.lzc", 1);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_23A6, 0, 0, s0, 0);
	if(!FADE_in(&D_23A6, &D_236C))
		(*D_0EAA)(&D_23A6, &D_236C);
	if((key = Pause(4000)) == -1) {
		MEM_free(s0);
		return -1;
	}
	if(key)
		music = 0;
	MEM_free(s0);
	s0 = LoadLzc("logos.lzc", 2);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_23A6, 15, 0, s0, 0);
	if(D_422A == 1) {
		if(music)
			WaitMusicFlags(1);
		FadeOutMusic(0);
		PlaySong(D_23C8);
		music = 1;
	}
	if(!FADE_in(&D_23A6, &D_236C))
		(*D_0EAA)(&D_23A6, &D_236C);
	if((key = Pause(3000)) == -1) {
		MEM_free(s0);
		return -1;
	}
	if(key)
		music = 0;
	MEM_free(s0);
	s0 = LoadLzc("fair.lzc", 0);
	s1 = LoadLzc("fair.lzc", 1);
	s2 = LoadLzc("fair.lzc", 2);
	s3 = LoadLzc("fair.lzc", 3);
	s4 = LoadLzc("fair.lzc", 4);
	s6 = LoadLzc("fair.lzc", 5);
	key = 0;
	for(i = 0; i < 10 && key == 0; i++) {
		if(i % D_23C2 == 0) {
			(*D_0EAE)(&D_23A6, 0);
			(*D_0E9A)(&D_23A6, 0, 24, s0, 0);
			(*D_0E9A)(&D_23A6, 0, 24, s1, 0);
			(*D_0E9A)(&D_23A6, 0, 24, s2, i % 5);
			(*D_0E9A)(&D_23A6, 190, 24, s3, 0);
			(*D_0E9A)(&D_23A6, 240, 24, s4, 0);
			(*D_0E9A)(&D_23A6, 0, 24, s6, 0);
			if(i > 0) {
				VID_waitRetrace();
				(*D_0EAA)(&D_23A6, &D_236C);
			} else if(!FADE_in(&D_23A6, &D_236C))
				(*D_0EAA)(&D_23A6, &D_236C);
			if((key = Pause(50)) == -1) {
				MEM_free(s0);
				MEM_free(s1);
				MEM_free(s2);
				MEM_free(s3);
				MEM_free(s4);
				MEM_free(s6);
				return -1;
			}
			if(key)
				music = 0;
		}
	}
	for(i = 0; i < 80 && key == 0; i++) {
		if((i + 10) % D_23C2 == 0 || i >= 79) {
			(*D_0EAE)(&D_23A6, 0);
			(*D_0E9A)(&D_23A6, 0, 24, s0, 0);
			(*D_0E9A)(&D_23A6, 0 - i, 24, s1, 0);
			(*D_0E9A)(&D_23A6, 320 - i, 24, s1, 1);
			(*D_0E9A)(&D_23A6, 0 - i * 2, 24, s2, i % 5);
			(*D_0E9A)(&D_23A6, 190 - i * 2, 24, s3, 0);
			(*D_0E9A)(&D_23A6, 240 - i * 2, 24, s4, 0);
			(*D_0E9A)(&D_23A6, 0 - i * 3, 24, s6, 0);
			(*D_0E9A)(&D_23A6, 320 - i * 3, 24, s6, 1);
			VID_waitRetrace();
			(*D_0EAA)(&D_23A6, &D_236C);
			if((key = Pause(50)) == -1) {
				MEM_free(s0);
				MEM_free(s1);
				MEM_free(s2);
				MEM_free(s3);
				MEM_free(s4);
				MEM_free(s6);
				return -1;
			}
			if(key)
				music = 0;
		}
	}
	if(key == 0) {
		if((key = Pause(1000)) == -1) {
			MEM_free(s0);
			MEM_free(s1);
			MEM_free(s2);
			MEM_free(s3);
			MEM_free(s4);
			MEM_free(s6);
			return -1;
		}
		if(key)
			music = 0;
	}
	MEM_free(s0);
	MEM_free(s1);
	MEM_free(s2);
	MEM_free(s3);
	MEM_free(s4);
	MEM_free(s6);
	s0 = LoadLzc("credits.lzc", 0);
	VID_waitRetrace();
	(*D_0EAE)(&D_236C, 15);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_23A6, 0, 0, s0, 0);
	if(!FADE_in(&D_23A6, &D_236C))
		(*D_0EAA)(&D_23A6, &D_236C);
	if((key = Pause(3000)) == -1) {
		MEM_free(s0);
		return -1;
	}
	if(key)
		music = 0;
	MEM_free(s0);
	s0 = LoadLzc("lowell.lzc", 0);
	key = 0;
	for(i = 1; i < 21 && key == 0; i++) {
		if(i % D_23C2 == 0 || i >= 20) {
			(*D_0EAE)(&D_23A6, 0);
			(*D_0E9A)(&D_23A6, 0, 24, s0, 0);
			(*D_0E9A)(&D_23A6, 0, 24, s0, i);
			if(i > 1) {
				VID_waitRetrace();
				(*D_0EAA)(&D_23A6, &D_236C);
			} else if(!FADE_in(&D_23A6, &D_236C))
				(*D_0EAA)(&D_23A6, &D_236C);
			if((key = Pause(50)) == -1) {
				MEM_free(s0);
				return -1;
			}
			if(key)
				music = 0;
		}
	}
	MEM_free(s0);
	s0 = LoadLzc("credits.lzc", 1);
	VID_waitRetrace();
	(*D_0EAE)(&D_236C, 15);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_23A6, 0, 0, s0, 0);
	if(!FADE_in(&D_23A6, &D_236C))
		(*D_0EAA)(&D_23A6, &D_236C);
	if((key = Pause(3000)) == -1) {
		MEM_free(s0);
		return -1;
	}
	if(key)
		music = 0;
	MEM_free(s0);
	s0 = LoadLzc("fuse.lzc", 0);
	key = 0;
	for(i = 1; i < 29 && key == 0; i++) {
		if(i % D_23C2 == 0 || i >= 28) {
			(*D_0EAE)(&D_23A6, 0);
			(*D_0E9A)(&D_23A6, 0, 24, s0, 0);
			(*D_0E9A)(&D_23A6, 0, 24, s0, i);
			if(i > 1) {
				VID_waitRetrace();
				(*D_0EAA)(&D_23A6, &D_236C);
			} else {
				if(D_422A == 1) {
					if(music)
						WaitMusicFlags(1);
					FadeOutMusic(0);
					PlaySong(D_23CC);
					music = 1;
				}
				if(!FADE_in(&D_23A6, &D_236C))
					(*D_0EAA)(&D_23A6, &D_236C);
			}
			if((key = Pause(50)) == -1) {
				MEM_free(s0);
				return -1;
			}
			if(key)
				music = 0;
		}
	}
	MEM_free(s0);
	s0 = LoadLzc("credits.lzc", 2);
	VID_waitRetrace();
	(*D_0EAE)(&D_236C, 15);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_23A6, 0, 0, s0, 0);
	if(!FADE_in(&D_23A6, &D_236C))
		(*D_0EAA)(&D_23A6, &D_236C);
	if((key = Pause(3000)) == -1) {
		MEM_free(s0);
		return -1;
	}
	if(key)
		music = 0;
	MEM_free(s0);
	s0 = LoadLzc("fair.lzc", 0);
	s1 = LoadLzc("fair.lzc", 1);
	s2 = LoadLzc("fair.lzc", 3);
	s6 = LoadLzc("fair.lzc", 4);
	s3 = LoadLzc("flash.lzc", 0);
	s4 = LoadLzc("flash.lzc", 1);
	key = 0;
	for(i = 0; i < 20 && key == 0; i++) {
		if(i % D_23C2 == 0 || i >= 19) {
			(*D_0EAE)(&D_23A6, 0);
			(*D_0E9A)(&D_23A6, 0, 24, s0, 0);
			(*D_0E9A)(&D_23A6, -80, 24, s1, 0);
			(*D_0E9A)(&D_23A6, 240, 24, s1, 1);
			(*D_0E9A)(&D_23A6, 30, 24, s2, 0);
			(*D_0E9A)(&D_23A6, 80, 24, s6, 0);
			if(i > 0 && i < 11)
				(*D_0E9A)(&D_23A6, 80, 24, s3, i - 1);
			if(i > 10)
				(*D_0E9A)(&D_23A6, 80, 24, s4, i - 11);
			if(i == 1)
				PlaySound(D_04CB, 0, 255, 192, 0, 1);
			if(i > 0) {
				VID_waitRetrace();
				(*D_0EAA)(&D_23A6, &D_236C);
			} else {
				if(D_422A == 1) {
					if(music)
						WaitMusicFlags(1);
					FadeOutMusic(0);
					PlaySong(D_23D0);
					music = 1;
				}
				if(!FADE_in(&D_23A6, &D_236C))
					(*D_0EAA)(&D_23A6, &D_236C);
			}
			if((key = Pause(75)) == -1) {
				MEM_free(s0);
				MEM_free(s1);
				MEM_free(s2);
				MEM_free(s3);
				MEM_free(s4);
				return -1;
			}
			if(key)
				music = 0;
		}
	}
	MEM_free(s0);
	MEM_free(s1);
	MEM_free(s2);
	MEM_free(s6);
	MEM_free(s3);
	MEM_free(s4);
	s0 = LoadLzc("credits.lzc", 3);
	VID_waitRetrace();
	(*D_0EAE)(&D_236C, 15);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_23A6, 0, 0, s0, 0);
	if(!FADE_in(&D_23A6, &D_236C))
		(*D_0EAA)(&D_23A6, &D_236C);
	if((key = Pause(3000)) == -1) {
		MEM_free(s0);
		return -1;
	}
	if(key)
		music = 0;
	MEM_free(s0);
	s0 = LoadLzc("flight.lzc", 0);
	s6 = LoadLzc("flight.lzc", 1);
	key = 0;
	for(i = 0; i < 12 && key == 0; i++) {
		if(i % D_23C2 == 0 || i >= 11) {
			(*D_0EAE)(&D_23A6, 0);
			(*D_0E9A)(&D_2384, 0, 24, s0, 0);
			(*D_0EDA)(&D_2384, 270 - i * 4 * i, 140 - i * 2 * i, s6, 0, 0, i * 4 * i, 0);
			if(i > 0) {
				VID_waitRetrace();
				(*D_0EAA)(&D_23A6, &D_236C);
			} else {
				if(D_422A == 1) {
					if(music)
						WaitMusicFlags(1);
					FadeOutMusic(0);
					PlaySong(D_23D4);
					music = 1;
				}
				if(!FADE_in(&D_23A6, &D_236C))
					(*D_0EAA)(&D_23A6, &D_236C);
			}
			if((key = Pause(5)) == -1) {
				MEM_free(s0);
				MEM_free(s6);
				return -1;
			}
			if(key)
				music = 0;
		}
	}
	MEM_free(s0);
	MEM_free(s6);
	s0 = LoadLzc("cabin.lzc", 0);
	s1 = LoadLzc("cabin.lzc", 1);
	s2 = LoadLzc("cabin.lzc", 3);
	s3 = LoadLzc("cabin.lzc", 4);
	s4 = LoadLzc("cabin.lzc", 5);
	s5 = LoadLzc("cabin.lzc", 6);
	s6 = LoadLzc("cabin.lzc", 2);
	key = 0;
	for(i = 0; i < 30 && key == 0; i++) {
		if(i % D_23C2 == 0 || i >= 29) {
			(*D_0EAE)(&D_23A6, 0);
			(*D_0E9A)(&D_2384, 0, 24, s0, 0);
			(*D_0E9A)(&D_2384, 116, 28, s1, (i + 2) % 15);
			(*D_0E9A)(&D_2384, 222, 53, s1, i % 15);
			(*D_0EDA)(&D_2384, i * 64 - 900, i * 16 - 150, s2, 0, i * 32 % 360, 256, 0);
			(*D_0EDA)(&D_2384, i * 32 + 62, (i << (3 - (i > 6))) + 15, s3, 0, i * 16 % 360, 256, 0);
			(*D_0EDA)(&D_2384, i * 32 - 150, i * 8, s4, 0, i * 32 % 360, 256, 0);
			(*D_0EDA)(&D_2384, i * 32 - 500, i * 4, s5, 0, i * 16 % 360, 256, 0);
			(*D_0EDA)(&D_2384, -(i * 4), i * i * 2 + 50, s6, 0, 0, i * 64 + 256, 0);
			if(i > 0) {
				VID_waitRetrace();
				(*D_0EAA)(&D_23A6, &D_236C);
			} else {
				if(D_422A == 1) {
					if(music)
						WaitMusicFlags(1);
					FadeOutMusic(0);
					PlaySong(D_23D8);
					music = 1;
				}
				if(!FADE_in(&D_23A6, &D_236C))
					(*D_0EAA)(&D_23A6, &D_236C);
			}
			if((key = Pause(5)) == -1) {
				MEM_free(s0);
				MEM_free(s1);
				MEM_free(s2);
				MEM_free(s3);
				MEM_free(s4);
				MEM_free(s5);
				MEM_free(s6);
				return -1;
			}
			if(key)
				music = 0;
		}
	}
	MEM_free(s0);
	MEM_free(s1);
	MEM_free(s2);
	MEM_free(s3);
	MEM_free(s4);
	MEM_free(s5);
	MEM_free(s6);
	s0 = LoadLzc("credits.lzc", 4);
	VID_waitRetrace();
	(*D_0EAE)(&D_236C, 15);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_23A6, 0, 0, s0, 0);
	if(!FADE_in(&D_23A6, &D_236C))
		(*D_0EAA)(&D_23A6, &D_236C);
	if((key = Pause(3000)) == -1) {
		MEM_free(s0);
		return -1;
	}
	if(key)
		music = 0;
	MEM_free(s0);
	s0 = LoadLzc("mars.lzc", 0);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_23A6, 0, 24, s0, 0);
	(*D_0E9A)(&D_23A6, 0, 24, s0, 1);
	if(D_422A == 1) {
		if(music)
			WaitMusicFlags(1);
		FadeOutMusic(0);
		PlaySong(D_23DC);
		music = 1;
	}
	if(!FADE_in(&D_23A6, &D_236C))
		(*D_0EAA)(&D_23A6, &D_236C);
	if((key = Pause(80)) == -1) {
		MEM_free(s0);
		return -1;
	}
	if(key)
		music = 0;
	key = 0;
	for(j = -1; j < 2 && key == 0; j++) {
		for(k = 0; k < 3 && key == 0; k++) {
			for(i = -1; i < 3 && key == 0; i++) {
				if(((j + 1) * 12 + k * 4 + i + 1) % D_23C2 == 0 || (j >= 1 && k >= 2 && i >= 2)) {
					(*D_0EAE)(&D_23A6, 0);
					(*D_0E9A)(&D_23A6, 0, 24, s0, 0);
					(*D_0E9A)(&D_23A6, 0, 24, s0, 3 - abs(i));
					VID_waitRetrace();
					(*D_0EAA)(&D_23A6, &D_236C);
					if((key = Pause((j == 0) * 60 + 20)) == -1) {
						MEM_free(s0);
						return -1;
					}
					if(key)
						music = 0;
				}
			}
			if(key == 0) {
				if((key = Pause((j == 0) * 360 + 60)) == -1) {
					MEM_free(s0);
					return -1;
				}
				if(key)
					music = 0;
			}
		}
		if(key == 0) {
			if((key = Pause(1000)) == -1) {
				MEM_free(s0);
				return -1;
			}
			if(key)
				music = 0;
		}
	}
	MEM_free(s0);
	s0 = LoadLzc("credits.lzc", 5);
	VID_waitRetrace();
	(*D_0EAE)(&D_236C, 15);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_23A6, 0, 0, s0, 0);
	if(!FADE_in(&D_23A6, &D_236C))
		(*D_0EAA)(&D_23A6, &D_236C);
	if((key = Pause(3000)) == -1) {
		MEM_free(s0);
		return -1;
	}
	if(key)
		music = 0;
	if(D_422A == 1) {
		if(music)
			WaitMusicFlags(1);
		FadeOutMusic(0);
	}
	FadeOut();
	MEM_free(s0);
	return 0;
}

/* data: 01E3 (the dialogue's lines), 0E9A, 0EAA, 0EAE, 236C, 2384, 2394, 23A6,
   33F6 (the character's name, "$p" in a line);
   strings: 07FF 18.2 (CLK_TCK, a double constant) */
/*play lines start.. of the character creation dialogue (up to one with text 0) over
the speaker (shapes: frame 0 the background, then its pose), a picture (pic) and a
drifting shape (moving); the lines come from lines[]. Returns the last key read
(-1: none); a line with wait -4 reads the character's name into D_33F6*/
/*C_04FE_2E9A*/int PlayDialogue(void far *shapes, void far *pic, void far *moving,
	char far **lines, int start)
{
	char far *line;
	int key;
	long cursor;
	long tMove;
	long t0;
	long tPause;
	long tBlink;
	int i;
	unsigned n;
	int shown;
	int pause;
	int blink;
	int movX;
	int movY;
	int movFrame;
	char buf[256];
	register int j;
	register int k;

	pause = 0;
	blink = 0;
	movX = 0;
	movY = 0;
	movFrame = 0;
	t0 = clock();
	tMove = t0;
	tPause = t0;
	tBlink = t0;
	for(i = start; D_01E3[i].text != 0; i++) {
		shown = 0;
		do {
			if(blink > 0 && clock() > tBlink + blink) {
				blink = 0;
				shown = 0;
			}
			if(pause > 0 && clock() > tPause + pause) {
				pause = 0;
				shown = 0;
			}
			if(clock() > tBlink + (unsigned)Random(100, 200)) {
				tBlink = clock();
				blink = Random(2, 10);
				shown = 0;
			}
			if(D_01E3[i].animate != 0 && clock() > tMove + (unsigned)Random(2, 20) && blink == 0) {
				tMove = clock();
				if(clock() > tPause + (unsigned)Random(150, 300)) {
					tPause = clock();
					pause = Random(20, 80);
				}
				if(pause == 0) {
					movX += (unsigned)Random(0, 3);
					if(movX > 29) {
						movX %= 30;
						movY = (movY + 2) % 20;
					}
					movFrame = (movFrame + (unsigned)Random(0, 2) - 1) % 7;
				}
				shown = 0;
			}
			if(shown == 0) {
				(*D_0EAE)(&D_23A6, 0);
				(*D_0E9A)(&D_2384, 0, 24, shapes, 0);
				if(blink)
					(*D_0E9A)(&D_2384, 0, 24, shapes, D_01E3[i].frame == 2 ? 5 : 4);
				else if(pause)
					(*D_0E9A)(&D_2384, 0, 24, shapes, 1);
				else
					(*D_0E9A)(&D_2384, 0, 24, shapes, D_01E3[i].frame);
				(*D_0E9A)(&D_2384, 0x80 - movX, movY + 0x53, moving, movFrame);
				(*D_0E9A)(&D_2384, 0x59, 0x6d, pic, 0);
				line = lines[abs(D_01E3[i].text)];
				for(j = 0, n = 0; line[j] != 0; j++, n++) {
					if(line[j] != '$' || tolower(line[j + 1]) != 'p')
						buf[n] = line[j];
					else {
						strcpy(&buf[n], D_33F6);
						j++;
						n += strlen(D_33F6) - 1;
					}
				}
				buf[n] = 0;
				PrintText(&D_2394, 0, 0xa0, buf, 2, D_01E3[i].text < 0 ? 4 : 6);
				VID_waitRetrace();
				(*D_0EAA)(&D_23A6, &D_236C);
			}
			shown = 1;
			key = -1;
			if(D_01E3[i].wait >= 0) {
				if(kbhit()) {
					shown = 0;
					while(kbhit())
						getch();
				} else if((clock() - t0) / CLK_TCK > D_01E3[i].wait)
					shown = 0;
			} else switch(D_01E3[i].wait) {
				case -1:
					if(kbhit()) {
						if(getch() == 0)
							getch();
						shown = 0;
						while(kbhit())
							getch();
					}
					break;
				case -2:
					if(kbhit()) {
						if((key = tolower(getch())) == 0)
							key = (key << 8) + getch();
						if(key >= 'a' && key <= 'b')
							shown = 0;
						while(kbhit())
							getch();
					}
					break;
				case -3:
					if(kbhit()) {
						if((key = tolower(getch())) == 0)
							key = (key << 8) + getch();
						if(key >= 'a' && key <= 'c')
							shown = 0;
						while(kbhit())
							getch();
					}
					break;
				case -4:
					buf[0] = '_';
					buf[1] = 0;
					k = 0;
					cursor = clock() % 16 / 8;
					for(;;) {
						PrintText(&D_2394, 0x78, 0xb8, buf, 0, 0);
						if(kbhit()) {
							if((key = getch()) == '\r' && k > 0)
								break;
							if(key == 0)
								getch();
							else if(key == 8)
								k = k - 1 < 0 ? 0 : k - 1;
							else if(tolower(key) >= 'a' && tolower(key) <= 'z' && k < 12)
								buf[k++] = key;
						} else if(clock() % 16 / 8 == cursor)
							continue;
						cursor = clock() % 16 / 8;
						if(cursor != 0 && k < 12)
							buf[k] = '_';
						else
							buf[k] = ' ';
						buf[k + 1] = 0;
						PrintText(&D_2394, 0x78, 0xb8, buf, 0, 0xe);
						(*D_0EAA)(&D_23A6, &D_236C);
					}
					buf[k] = 0;
					strcpy(D_33F6, buf);
					shown = 0;
					while(kbhit())
						getch();
			}
		} while(shown);
	}
	return key;
}

#define LINES(n) PlayDialogue(shapes, pic, moving, lines, n)

/* data: 0BC0 (0x62 bytes, 0BF6 sex, 0BF7 a character exists), 24F6 (0x1CF1 bytes),
   the new character in it: 28F8 shape, 2DF7 str, 2EF7 dex, 2FF7 int, 30F8 exp, 32F7 hp,
   34E8 level, 3AE8, 3BE7, 3BE9; 23E0 (song), 422A (music on), 23A6, 236C, 2394;
   strings: 0807 "savegame\objlist", 0818 "Run INSTALL to remove your existing character
   before creating another.", 085F "scenetxt.lzc", 086C "create.lzc", 0877 "create.lzc",
   0882 "create.lzc", 088D "savegame\objlist" */
/*character creation: the dialogue, the answers set strength, dexterity and intelligence;
saves savegame\objlist (1: no objlist or a character exists, -1: done)*/
/*C_04FE_3577*/int CreateCharacter() {
	void far *shapes;
	void far *pic;
	void far *moving;
	char far *txt;
	int q1;
	int q2;
	int q3;
	unsigned len1 = 0x1cf1;
	unsigned len2 = 0x62;
	int hi;
	int mid;
	int lo;
	int fd;
	char far *lines[100];
	register int sex;
	register int q4;

	if((fd = open("savegame\\objlist", O_RDONLY | O_BINARY)) == -1)
		return 1;
	if(read(fd, D_24F6, len1) < 1)
		return 1;
	if(read(fd, D_0BC0, len2) < 1)
		return 1;
	close(fd);
	if(D_0BF7 == 1) {
		(*D_0EAE)(&D_23A6, 0);
		PrintText(&D_2394, 0, 0x14,
			"Run INSTALL to remove your existing character before creating another.", 2, 0xf);
		(*D_0EAA)(&D_23A6, &D_236C);
		Pause(-1);
		return 1;
	}
	hi = Random(24, 26);
	mid = Random(22, 24);
	lo = Random(18, 22);
	D_2DF7 = lo;
	D_2EF7 = lo;
	D_2FF7 = lo;
	D_3AE8 = 20;
	D_30F8 = 600;
	D_34E8 = 4;
	if(D_422A == 1)
		PlaySong(D_23E0);
	txt = ReadLines("scenetxt.lzc", 5, lines);
	shapes = LoadLzc("create.lzc", 0);
	pic = LoadLzc("create.lzc", 1);
	moving = LoadLzc("create.lzc", 2);
	while(kbhit())
		getch();
	sex = LINES(0);
	if(sex == 'a')
		LINES(8);
	if(sex == 'b')
		LINES(0xb);
	if((q1 = LINES(0xe)) == 'a') {
		LINES(0x12);
		if((q2 = LINES(0x14)) == 'a') {
			if((q3 = LINES(0x17)) == 'a') {
				if((q4 = LINES(0x1d)) == 'a') {
					D_2DF7 = hi;
					D_2FF7 = mid;
					if(sex == 'a')
						LINES(0x1f);
					if(sex == 'b')
						LINES(0x22);
				}
				if(q4 == 'b') {
					D_2FF7 = hi;
					D_2DF7 = mid;
				}
			}
			if(q3 == 'b') {
				if((q4 = LINES(0x25)) == 'a') {
					D_2FF7 = hi;
					D_2EF7 = mid;
					LINES(0x28);
				}
				if(q4 == 'b') {
					D_2EF7 = hi;
					D_2FF7 = mid;
					LINES(0x2b);
				}
			}
		}
		if(q2 == 'b') {
			if((q3 = LINES(0x1a)) == 'a') {
				if((q4 = LINES(0x2e)) == 'a') {
					D_2DF7 = hi;
					D_2EF7 = mid;
					if(sex == 'a')
						LINES(0x31);
					if(sex == 'b')
						LINES(0x34);
				}
				if(q4 == 'b') {
					D_2EF7 = hi;
					D_2DF7 = mid;
					if(sex == 'a')
						LINES(0x3a);
					if(sex == 'b')
						LINES(0x37);
				}
			}
			if(q3 == 'b') {
				if((q4 = LINES(0x3d)) == 'a') {
					D_2FF7 = hi;
					D_2EF7 = mid;
					LINES(0x40);
				}
				if(q4 == 'b') {
					D_2EF7 = hi;
					D_2FF7 = mid;
					LINES(0x42);
				}
			}
		}
	}
	if(q1 == 'b') {
		if((q2 = LINES(0x1a)) == 'a') {
			if((q3 = LINES(0x17)) == 'a') {
				if((q4 = LINES(0x1d)) == 'a') {
					D_2DF7 = hi;
					D_2FF7 = mid;
					if(sex == 'a')
						LINES(0x1f);
					if(sex == 'b')
						LINES(0x22);
				}
				if(q4 == 'b') {
					D_2FF7 = hi;
					D_2DF7 = mid;
				}
			}
			if(q3 == 'b') {
				if((q4 = LINES(0x25)) == 'a') {
					D_2FF7 = hi;
					D_2EF7 = mid;
					LINES(0x28);
				}
				if(q4 == 'b') {
					D_2EF7 = hi;
					D_2FF7 = mid;
					LINES(0x2b);
				}
			}
		}
		if(q2 == 'b') {
			if((q3 = LINES(0x14)) == 'a') {
				if((q4 = LINES(0x48)) == 'a') {
					D_2FF7 = hi;
					D_2DF7 = mid;
					LINES(0x4b);
				}
				if(q4 == 'b') {
					D_2DF7 = hi;
					D_2FF7 = mid;
					LINES(0x4e);
				}
			}
			if(q3 == 'b') {
				if((q4 = LINES(0x2e)) == 'a') {
					D_2DF7 = hi;
					D_2EF7 = mid;
					if(sex == 'a')
						LINES(0x31);
					if(sex == 'b')
						LINES(0x34);
				}
				if(q4 == 'b') {
					D_2EF7 = hi;
					D_2DF7 = mid;
					if(sex == 'a')
						LINES(0x3a);
					if(sex == 'b')
						LINES(0x37);
				}
			}
		}
	}
	if(q1 == 'c') {
		LINES(0x51);
		if((q2 = LINES(0x17)) == 'a') {
			if((q3 = LINES(0x1a)) == 'a') {
				if((q4 = LINES(0x2e)) == 'a') {
					D_2DF7 = hi;
					D_2EF7 = mid;
					if(sex == 'a')
						LINES(0x31);
					if(sex == 'b')
						LINES(0x34);
				}
				if(q4 == 'b') {
					D_2EF7 = hi;
					D_2DF7 = mid;
					if(sex == 'a')
						LINES(0x3a);
					if(sex == 'b')
						LINES(0x37);
				}
			}
			if(q3 == 'b') {
				if((q4 = LINES(0x25)) == 'a') {
					D_2FF7 = hi;
					D_2EF7 = mid;
					LINES(0x28);
				}
				if(q4 == 'b') {
					D_2EF7 = hi;
					D_2FF7 = mid;
					LINES(0x2b);
				}
			}
		}
		if(q2 == 'b') {
			if((q3 = LINES(0x14)) == 'a') {
				if((q4 = LINES(0x1d)) == 'a') {
					D_2DF7 = hi;
					D_2FF7 = mid;
					if(sex == 'a')
						LINES(0x1f);
					if(sex == 'b')
						LINES(0x22);
				}
				if(q4 == 'b') {
					D_2FF7 = hi;
					D_2DF7 = mid;
				}
			}
			if(q3 == 'b') {
				if((q4 = LINES(0x54)) == 'a') {
					D_2EF7 = hi;
					D_2DF7 = mid;
					LINES(0x57);
				}
				if(q4 == 'b') {
					D_2DF7 = hi;
					D_2EF7 = mid;
					LINES(0x5a);
				}
			}
		}
	}
	LINES(0x45);
	MEM_free(shapes);
	MEM_free(pic);
	MEM_free(moving);
	MEM_free(txt);
	D_32F7 = (D_2DF7 << 1) + D_34E8 * 24;
	if(sex == 'b') {
		D_0BF6 = 1;
		D_28F8 = D_28F8 & 0xfc00 | 0x158;
	} else {
		D_0BF6 = 0;
		D_28F8 = D_28F8 & 0xfc00 | 0x157;
	}
	D_3BE9 = D_28F8;
	D_3BE7 = D_28F8;
	D_0BF7 = 1;
	fd = open("savegame\\objlist", O_RDWR | O_BINARY);
	write(fd, D_24F6, len1);
	write(fd, D_0BC0, len2);
	close(fd);
	FadeOutMusic(120);
	FadeOut();
	return -1;
}

/*the introduction (scenetxt.lzc items 0-2): the professor's lecture and Mars (scene1.lzc),
the cannon on the map (scene2.lzc), the space bullet and the moongate (scene3.lzc,
moongate.lzc), the moongate on Mars (scene4a.lzc, scene4b.lzc); 0: a key stopped it*/
/* strings: 089E-0BAA, 60 literals in order of use, none shared (no -d): 089E "scenetxt.lzc",
   08AB.. 14 x "scene1.lzc", 093A.. 5 x "scene2.lzc", 0971 "scenetxt.lzc", 097E.. 5 x "scene3.lzc",
   09B5.. 4 x "moongate.lzc", 09E9 "scenetxt.lzc", 09F6.. "scene4a/4b.lzc", "moongate.lzc" ... 0B9E "scene4b.lzc"
   variables: D_00F5, D_012B, D_0149, D_01AD, D_04D3 (initialised); D_0E9A, D_0EAA, D_0EAE, D_0ED2, D_0EDA,
   D_236C, D_2384, D_2394, D_23A6, D_4210, D_4214, D_4218, D_422A */
/*C_04FE_4286*/int PlayIntro() {
	void far *bg;
	void far *p2;
	void far *p3;
	void far *p4;
	void far *p5;
	void far *p6;
	void far *p7;
	void far *p8;
	void far *p9;
	void far *p10;
	void far *p11;
	char far *txt;
	int j;
	char far *text[100];
	register int key, i;

	/* scene 1 */
	txt = ReadLines("scenetxt.lzc", 0, text);
	bg = LoadLzc("scene1.lzc", 0);
	p4 = LoadLzc("scene1.lzc", 1);
	p5 = LoadLzc("scene1.lzc", 2);
	p6 = LoadLzc("scene1.lzc", 8);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 1);
	(*D_0E9A)(&D_2384, 240, 151, p4, 0);
	PrintText(&D_2394, 0, 160, text[1], 2, D_012B[1]);
	FadeIn();
	while(kbhit())
		getch();
	if(D_422A == 1) {
		FadeOutMusic(0);
		PlaySong(D_4210);
	}
	if(Pause(3000) == -1) {
		MEM_free(bg);
		MEM_free(p4);
		MEM_free(p5);
		MEM_free(p6);
		MEM_free(txt);
		return 0;
	}
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	(*D_0E9A)(&D_2384, 240, 151, p4, 1);
	(*D_0E9A)(&D_2384, 0, 151, p6, 0);
	(*D_0E9A)(&D_2384, 60, 151, p5, 0);
	PrintText(&D_2394, 0, 160, text[2], 2, D_012B[2]);
	(*D_0EAA)(&D_23A6, &D_236C);
	if(Pause(4000) == -1) {
		MEM_free(bg);
		MEM_free(p4);
		MEM_free(p5);
		MEM_free(p6);
		MEM_free(txt);
		return 0;
	}
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	(*D_0E9A)(&D_2384, 240, 151, p4, 3);
	(*D_0E9A)(&D_2384, 0, 151, p6, 0);
	(*D_0E9A)(&D_2384, 80, 151, p5, 1);
	PrintText(&D_2394, 0, 160, text[3], 2, D_012B[3]);
	(*D_0EAA)(&D_23A6, &D_236C);
	if(Pause(4000) == -1) {
		MEM_free(bg);
		MEM_free(p4);
		MEM_free(p5);
		MEM_free(p6);
		MEM_free(txt);
		return 0;
	}
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	(*D_0E9A)(&D_2384, 240, 151, p4, 3);
	(*D_0E9A)(&D_2384, 0, 151, p6, 1);
	(*D_0E9A)(&D_2384, 80, 151, p5, 1);
	PrintText(&D_2394, 0, 160, text[4], 2, D_012B[4]);
	(*D_0EAA)(&D_23A6, &D_236C);
	if(Pause(4000) == -1) {
		MEM_free(bg);
		MEM_free(p4);
		MEM_free(p5);
		MEM_free(p6);
		MEM_free(txt);
		return 0;
	}
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	(*D_0E9A)(&D_2384, 240, 151, p4, 3);
	(*D_0E9A)(&D_2384, 0, 151, p6, 2);
	(*D_0E9A)(&D_2384, 100, 151, p5, 2);
	PrintText(&D_2394, 0, 160, text[5], 2, D_012B[5]);
	(*D_0EAA)(&D_23A6, &D_236C);
	if(Pause(3000) == -1) {
		MEM_free(bg);
		MEM_free(p4);
		MEM_free(p5);
		MEM_free(p6);
		MEM_free(txt);
		return 0;
	}
	MEM_free(p6);
	p6 = LoadLzc("scene1.lzc", 3);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	(*D_0E9A)(&D_2384, 240, 151, p4, 3);
	(*D_0E9A)(&D_2384, -10, 155, p6, 0);
	(*D_0E9A)(&D_2384, 145, 151, p5, 3);
	PrintText(&D_2394, 0, 160, text[6], 2, D_012B[6]);
	(*D_0EAA)(&D_23A6, &D_236C);
	if(Pause(3000) == -1) {
		MEM_free(bg);
		MEM_free(p4);
		MEM_free(p5);
		MEM_free(p6);
		MEM_free(txt);
		return 0;
	}
	key = 0;
	for(i = 0; i < 11 && key == 0; i++) {
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_2384, 0, 24, bg, 0);
		(*D_0E9A)(&D_2384, 240, 151, p4, 3);
		(*D_0E9A)(&D_2384, -10, 155, p6, min(9, i));
		(*D_0E9A)(&D_2384, 145, 151, p5, i + 5);
		PrintText(&D_2394, 0, 160, text[7], 2, D_012B[7]);
		(*D_0EAA)(&D_23A6, &D_236C);
		if((key = Pause(100)) == -1) {
			MEM_free(bg);
			MEM_free(p4);
			MEM_free(p5);
			MEM_free(p6);
			MEM_free(txt);
			return 0;
		}
	}
	if(key == 0 && Pause(2000) == -1) {
		MEM_free(bg);
		MEM_free(p4);
		MEM_free(p5);
		MEM_free(p6);
		MEM_free(txt);
		return 0;
	}
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	(*D_0E9A)(&D_2384, 240, 151, p4, 3);
	(*D_0E9A)(&D_2384, -10, 155, p6, 9);
	(*D_0E9A)(&D_2384, 145, 151, p5, 15);
	PrintText(&D_2394, 0, 160, text[8], 2, D_012B[8]);
	(*D_0EAA)(&D_23A6, &D_236C);
	if(Pause(4000) == -1) {
		MEM_free(bg);
		MEM_free(p4);
		MEM_free(p5);
		MEM_free(p6);
		MEM_free(txt);
		return 0;
	}
	MEM_free(bg);
	MEM_free(p4);
	MEM_free(p5);
	MEM_free(p6);
	bg = LoadLzc("scene1.lzc", 7);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	PrintText(&D_2394, 0, 160, text[9], 2, D_012B[9]);
	(*D_0EAA)(&D_23A6, &D_236C);
	if(Pause(4000) == -1) {
		MEM_free(bg);
		MEM_free(txt);
		return 0;
	}
	MEM_free(bg);
	bg = LoadLzc("scene1.lzc", 0);
	p4 = LoadLzc("scene1.lzc", 1);
	p5 = LoadLzc("scene1.lzc", 2);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 1);
	(*D_0E9A)(&D_2384, 270, 151, p4, 1);
	(*D_0E9A)(&D_2384, 193, 161, p5, 16);
	PrintText(&D_2394, 0, 160, text[10], 2, D_012B[10]);
	(*D_0EAA)(&D_23A6, &D_236C);
	if(Pause(3000) == -1) {
		MEM_free(bg);
		MEM_free(p4);
		MEM_free(p5);
		MEM_free(txt);
		return 0;
	}
	MEM_free(bg);
	MEM_free(p4);
	MEM_free(p5);
	bg = LoadLzc("scene1.lzc", 5);
	p2 = LoadLzc("scene1.lzc", 4);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	(*D_0E9A)(&D_2384, 50, 24, bg, 1);
	(*D_0E9A)(&D_2384, 235, 78, p2, 0);
	PrintText(&D_2394, 0, 160, text[11], 2, D_012B[11]);
	(*D_0EAA)(&D_23A6, &D_236C);
	if(Pause(4000) == -1) {
		MEM_free(bg);
		MEM_free(p2);
		MEM_free(txt);
		return 0;
	}
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	(*D_0E9A)(&D_2384, 50, 24, bg, 1);
	PrintText(&D_2394, 0, 160, text[12], 2, D_012B[12]);
	(*D_0EAA)(&D_23A6, &D_236C);
	if(Pause(4000) == -1) {
		MEM_free(bg);
		MEM_free(p2);
		MEM_free(txt);
		return 0;
	}
	MEM_free(bg);
	MEM_free(p2);
	bg = LoadLzc("scene1.lzc", 5);
	p2 = LoadLzc("scene1.lzc", 6);
	key = 0;
	for(i = 0; i < 20 && key == 0; i++) {
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_2384, 0, 24, bg, 0);
		(*D_0EDA)(&D_2384, 0, -20 - i, p2, 0, -i << 1, 256, 0);
		PrintText(&D_2394, 0, 160, text[13], 2, D_012B[13]);
		(*D_0EAA)(&D_23A6, &D_236C);
		if((key = Pause(25)) == -1) {
			MEM_free(bg);
			MEM_free(p2);
			MEM_free(txt);
			return 0;
		}
	}
	if(key == 0 && Pause(2000) == -1) {
		MEM_free(bg);
		MEM_free(p2);
		MEM_free(txt);
		return 0;
	}
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	PrintText(&D_2394, 0, 160, text[14], 2, D_012B[14]);
	(*D_0EAA)(&D_23A6, &D_236C);
	if(Pause(3000) == -1) {
		MEM_free(bg);
		MEM_free(p2);
		MEM_free(txt);
		return 0;
	}
	FadeOut();
	MEM_free(bg);
	MEM_free(p2);
	MEM_free(txt);

	/* scene 2 */
	bg = LoadLzc("scene2.lzc", 0);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	FadeIn();
	if((key = Pause(1000)) == -1) {
		MEM_free(bg);
		MEM_free(txt);
		return 0;
	}
	for(i = 0; i < 13 && key == 0; i++) {
		(*D_0ED2)(&D_2384, D_00F5[i].x, D_00F5[i].y - 5, 1, 1, 9);
		(*D_0EAA)(&D_23A6, &D_236C);
		if((key = Pause(400)) == -1) {
			MEM_free(bg);
			MEM_free(txt);
			return 0;
		}
	}
	if(key == 0 && Pause(2000) == -1) {
		MEM_free(bg);
		MEM_free(txt);
		return 0;
	}
	MEM_free(bg);
	FadeOut();
	bg = LoadLzc("scene2.lzc", 1);
	p3 = LoadLzc("scene2.lzc", 2);
	p4 = LoadLzc("scene2.lzc", 3);
	p5 = LoadLzc("scene2.lzc", 4);
	key = 0;
	for(i = 0; i < 95 && key == 0; i++) {
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_2384, 0, 24, bg, 0);
		(*D_0E9A)(&D_2384, i * 4 - 15, 148, p4, i % 12);
		(*D_0E9A)(&D_2384, i * 4 - 45, 149, p5, (i + 4) % 12);
		(*D_0E9A)(&D_2384, 0, 24, p3, 0);
		if(i > 0)
			(*D_0EAA)(&D_23A6, &D_236C);
		else
			FadeIn();
		if((key = Pause(50)) == -1) {
			MEM_free(bg);
			MEM_free(p3);
			MEM_free(p4);
			MEM_free(p5);
			MEM_free(txt);
			return 0;
		}
	}
	if(D_422A == 1)
		FadeOutMusic(120);
	FadeOut();
	MEM_free(bg);
	MEM_free(p3);
	MEM_free(p4);
	MEM_free(p5);

	/* scene 3 */
	txt = ReadLines("scenetxt.lzc", 1, text);
	bg = LoadLzc("scene3.lzc", 0);
	p3 = LoadLzc("scene3.lzc", 1);
	p4 = LoadLzc("scene3.lzc", 2);
	p5 = LoadLzc("scene3.lzc", 3);
	p2 = LoadLzc("scene3.lzc", 4);
	for(i = 0; i < 7; i++) {
		(*D_0EAE)(&D_23A6, 0);
		if(D_0149[i].frame >= 0) {
			(*D_0E9A)(&D_2384, 0, 24, bg, 0);
			(*D_0E9A)(&D_2384, 165, 137, p5, D_0149[i].frame2);
			(*D_0E9A)(&D_2384, 65, 150, p4, min(7, D_0149[i].frame));
			(*D_0E9A)(&D_2384, 151, 82, p3, 0);
		} else
			(*D_0E9A)(&D_2384, 0, 24, p2, abs(D_0149[i].frame) - 1);
		PrintText(&D_2394, 0, 160, text[D_0149[i].text], 2, D_0149[i].color);
		if(i > 0)
			(*D_0EAA)(&D_23A6, &D_236C);
		else {
			FadeIn();
			if(D_422A == 1)
				PlaySong(D_4214);
		}
		if(Pause(D_0149[i].delay) == -1) {
			MEM_free(bg);
			MEM_free(p2);
			MEM_free(p3);
			MEM_free(p4);
			MEM_free(p5);
			MEM_free(txt);
			return 0;
		}
	}
	MEM_free(p2);
	p2 = LoadLzc("moongate.lzc", 0);
	key = 0;
	for(i = 0; i < 9 && key == 0; i++) {
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_2384, 0, 24, bg, 0);
		(*D_0E9A)(&D_2384, 125, 130, p2, i);
		(*D_0E9A)(&D_2384, 165, 137, p5, 3);
		(*D_0E9A)(&D_2384, 65, 150, p4, 1);
		(*D_0E9A)(&D_2384, 151, 82, p3, 1);
		(*D_0EAA)(&D_23A6, &D_236C);
		if((key = Pause(50)) == -1) {
			MEM_free(bg);
			MEM_free(p2);
			MEM_free(p3);
			MEM_free(p4);
			MEM_free(p5);
			MEM_free(txt);
			return 0;
		}
	}
	MEM_free(p2);
	p2 = LoadLzc("moongate.lzc", 1);
	for(i = 7; i < 10; i++) {
		key = 0;
		for(j = 0; j < 40 && key == 0; j++) {
			(*D_0EAE)(&D_23A6, 0);
			(*D_0E9A)(&D_2384, 0, 24, bg, 0);
			(*D_0E9A)(&D_2384, 125, 130, p2, j % 8);
			(*D_0E9A)(&D_2384, 165, 137, p5, D_0149[i].frame2);
			(*D_0E9A)(&D_2384, 65, 150, p4, min(7, D_0149[i].frame));
			(*D_0E9A)(&D_2384, 151, 82, p3, 1);
			PrintText(&D_2394, 0, 160, text[D_0149[i].text], 2, D_0149[i].color);
			(*D_0EAA)(&D_23A6, &D_236C);
			if((key = Pause(50)) == -1) {
				MEM_free(bg);
				MEM_free(p2);
				MEM_free(p3);
				MEM_free(p4);
				MEM_free(p5);
				MEM_free(txt);
				return 0;
			}
		}
	}
	MEM_free(p5);
	MEM_free(p4);
	p4 = LoadLzc("moongate.lzc", 2);
	p5 = LoadLzc("moongate.lzc", 4);
	key = 0;
	for(i = 0; i < 65 && key == 0; i++) {
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_2384, 0, 24, bg, 0);
		(*D_0E9A)(&D_2384, 125, 130, p2, i % 8);
		(*D_0E9A)(&D_2384, 165, 137, p5, 0);
		(*D_0E9A)(&D_2384, 65, 150, p4, i / 2);
		(*D_0E9A)(&D_2384, 151, 82, p3, 1);
		(*D_0EAA)(&D_23A6, &D_236C);
		if((key = Pause(50)) == -1) {
			MEM_free(bg);
			MEM_free(p2);
			MEM_free(p3);
			MEM_free(p4);
			MEM_free(p5);
			MEM_free(txt);
			return 0;
		}
	}
	key = 0;
	for(i = 0; i < 40 && key == 0; i++) {
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_2384, 0, 24, bg, 0);
		(*D_0E9A)(&D_2384, 125, 130, p2, i % 8);
		if(i < 39)
			(*D_0E9A)(&D_2384, 165, 137, p5, i / 2);
		(*D_0E9A)(&D_2384, 151, 82, p3, 1);
		(*D_0EAA)(&D_23A6, &D_236C);
		if((key = Pause(50)) == -1) {
			MEM_free(bg);
			MEM_free(p2);
			MEM_free(p3);
			MEM_free(p4);
			MEM_free(p5);
			MEM_free(txt);
			return 0;
		}
	}
	if(D_422A == 1)
		FadeOutMusic(120);
	FadeOut();
	MEM_free(bg);
	MEM_free(p3);
	MEM_free(p4);
	MEM_free(p5);
	MEM_free(p2);
	MEM_free(txt);

	/* scene 4 */
	txt = ReadLines("scenetxt.lzc", 2, text);
	bg = LoadLzc("scene4a.lzc", 0);
	p2 = LoadLzc("scene4b.lzc", 2);
	p7 = LoadLzc("scene4a.lzc", 4);
	p8 = LoadLzc("scene4a.lzc", 6);
	p9 = LoadLzc("scene4a.lzc", 5);
	for(j = 1; j < 3; j++) {
		key = 0;
		for(i = 0; i < 21 && key == 0; i++) {
			(*D_0EAE)(&D_23A6, 0);
			(*D_0E9A)(&D_2384, 0, 24, bg, 0);
			(*D_0E9A)(&D_2384, 187, 125, p8, min(5, 10 - abs(i - 10)));
			if(i > 4 && i < 16) {
				PlaySound(D_04D3, 0, 255, 64, 0, 1);
				(*D_0E9A)(&D_2384, 32, 24, p2, i % 10);
			}
			(*D_0E9A)(&D_2384, 240, 40, p9, 0);
			(*D_0E9A)(&D_2384, 0, 74, p7, 0);
			(*D_0E9A)(&D_2384, 23, 100, p7, 2);
			PrintText(&D_2394, 0, 160, text[j], 2, D_01AD[j]);
			if(j > 1 || i > 0)
				(*D_0EAA)(&D_23A6, &D_236C);
			else {
				FadeIn();
				if(D_422A == 1)
					PlaySong(D_4218);
			}
			if(i == 16)
				StopSound(0, 0);
			if((key = Pause(100)) == -1) {
				MEM_free(bg);
				MEM_free(p2);
				MEM_free(p7);
				MEM_free(p8);
				MEM_free(p9);
				MEM_free(txt);
				return 0;
			}
		}
		if(key == 0 && Pause(2000) == -1) {
			MEM_free(bg);
			MEM_free(p2);
			MEM_free(p7);
			MEM_free(p8);
			MEM_free(p9);
			MEM_free(txt);
			return 0;
		}
	}
	MEM_free(p2);
	p2 = LoadLzc("moongate.lzc", 0);
	key = 0;
	for(i = 0; i < 9 && key == 0; i++) {
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_2384, 0, 24, bg, 0);
		(*D_0E9A)(&D_2384, 188, 128, p8, (i > 4) + 6);
		(*D_0E9A)(&D_2384, 140, 125, p2, i);
		(*D_0E9A)(&D_2384, 240, 40, p9, i > 1);
		(*D_0E9A)(&D_2384, 0, 74, p7, 0);
		(*D_0E9A)(&D_2384, 23, 100, p7, 2);
		(*D_0EAA)(&D_23A6, &D_236C);
		if((key = Pause(100)) == -1) {
			MEM_free(bg);
			MEM_free(p2);
			MEM_free(p7);
			MEM_free(p8);
			MEM_free(p9);
			MEM_free(txt);
			return 0;
		}
	}
	MEM_free(p2);
	p2 = LoadLzc("moongate.lzc", 1);
	p4 = LoadLzc("moongate.lzc", 3);
	p5 = LoadLzc("moongate.lzc", 5);
	key = 0;
	for(i = 0; i < 69 && key == 0; i++) {
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_2384, 0, 24, bg, 0);
		(*D_0E9A)(&D_2384, 180, 133, p8, 8);
		(*D_0E9A)(&D_2384, 140, 125, p2, i % 8);
		(*D_0E9A)(&D_2384, 240, 40, p9, (i > 2) + 1);
		(*D_0E9A)(&D_2384, 80, 150, p4, i / 2);
		(*D_0E9A)(&D_2384, 0, 74, p7, 0);
		(*D_0E9A)(&D_2384, 23, 100, p7, (i > 3) + 2);
		if(i > 10)
			PrintText(&D_2394, 0, 160, text[3], 2, D_01AD[3]);
		(*D_0EAA)(&D_23A6, &D_236C);
		if((key = Pause(50)) == -1) {
			MEM_free(bg);
			MEM_free(p2);
			MEM_free(p7);
			MEM_free(p8);
			MEM_free(p9);
			MEM_free(p4);
			MEM_free(p5);
			MEM_free(txt);
			return 0;
		}
	}
	key = 0;
	for(i = 0; i < 25 && key == 0; i++) {
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_2384, 0, 24, bg, 0);
		(*D_0E9A)(&D_2384, 180, 133, p8, 8);
		(*D_0E9A)(&D_2384, 140, 125, p2, i % 8);
		(*D_0E9A)(&D_2384, 240, 40, p9, 2);
		(*D_0E9A)(&D_2384, 140, 130, p5, i / 2);
		(*D_0E9A)(&D_2384, 80, 150, p4, 34);
		(*D_0E9A)(&D_2384, 0, 74, p7, 0);
		(*D_0E9A)(&D_2384, 23, 100, p7, 3);
		PrintText(&D_2394, 0, 160, text[4], 2, D_01AD[4]);
		(*D_0EAA)(&D_23A6, &D_236C);
		if((key = Pause(50)) == -1) {
			MEM_free(bg);
			MEM_free(p2);
			MEM_free(p7);
			MEM_free(p8);
			MEM_free(p9);
			MEM_free(p4);
			MEM_free(p5);
			MEM_free(txt);
			return 0;
		}
	}
	MEM_free(bg);
	MEM_free(p2);
	MEM_free(p7);
	MEM_free(p8);
	MEM_free(p9);
	MEM_free(p4);
	MEM_free(p5);
	bg = LoadLzc("scene4a.lzc", 9);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 1);
	PrintText(&D_2394, 0, 160, text[5], 2, D_01AD[5]);
	(*D_0EAA)(&D_23A6, &D_236C);
	if(Pause(4000) == -1) {
		MEM_free(bg);
		MEM_free(txt);
		return 0;
	}
	MEM_free(bg);
	bg = LoadLzc("scene4a.lzc", 0);
	p7 = LoadLzc("scene4a.lzc", 4);
	p8 = LoadLzc("scene4a.lzc", 6);
	p9 = LoadLzc("scene4a.lzc", 5);
	p4 = LoadLzc("scene4a.lzc", 2);
	p5 = LoadLzc("scene4a.lzc", 3);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	(*D_0E9A)(&D_2384, 180, 133, p8, 9);
	(*D_0E9A)(&D_2384, 240, 40, p9, 2);
	(*D_0E9A)(&D_2384, 145, 140, p5, 1);
	(*D_0E9A)(&D_2384, 80, 150, p4, 1);
	(*D_0E9A)(&D_2384, 0, 74, p7, 0);
	(*D_0E9A)(&D_2384, 23, 100, p7, 2);
	PrintText(&D_2394, 0, 160, text[6], 2, D_01AD[6]);
	(*D_0EAA)(&D_23A6, &D_236C);
	if(Pause(4000) == -1) {
		MEM_free(bg);
		MEM_free(p7);
		MEM_free(p8);
		MEM_free(p9);
		MEM_free(p4);
		MEM_free(p5);
		MEM_free(txt);
		return 0;
	}
	MEM_free(bg);
	MEM_free(p7);
	MEM_free(p8);
	MEM_free(p9);
	MEM_free(p4);
	MEM_free(p5);
	bg = LoadLzc("scene4a.lzc", 7);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 3);
	PrintText(&D_2394, 0, 160, text[7], 2, D_01AD[7]);
	(*D_0EAA)(&D_23A6, &D_236C);
	if(Pause(4000) == -1) {
		MEM_free(bg);
		MEM_free(txt);
		return 0;
	}
	MEM_free(bg);
	bg = LoadLzc("scene4a.lzc", 8);
	for(i = 8; i < 10; i++) {
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_2384, 0, 24, bg, 0);
		(*D_0E9A)(&D_2384, 0, 24, bg, i - 7);
		PrintText(&D_2394, 0, 160, text[i], 2, D_01AD[i]);
		(*D_0EAA)(&D_23A6, &D_236C);
		if(Pause(4000) == -1) {
			MEM_free(bg);
			MEM_free(txt);
			return 0;
		}
	}
	MEM_free(bg);
	bg = LoadLzc("scene4a.lzc", 0);
	p7 = LoadLzc("scene4a.lzc", 4);
	p8 = LoadLzc("scene4a.lzc", 6);
	p9 = LoadLzc("scene4a.lzc", 5);
	p4 = LoadLzc("scene4a.lzc", 2);
	p5 = LoadLzc("scene4a.lzc", 3);
	p10 = LoadLzc("scene4b.lzc", 0);
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_2384, 0, 24, bg, 0);
	(*D_0E9A)(&D_2384, 180, 133, p8, 10);
	(*D_0E9A)(&D_2384, 240, 40, p9, 2);
	(*D_0E9A)(&D_2384, 145, 140, p5, 0);
	(*D_0E9A)(&D_2384, 80, 150, p4, 0);
	(*D_0E9A)(&D_2384, 0, 74, p7, 0);
	(*D_0E9A)(&D_2384, 23, 100, p7, 2);
	PrintText(&D_2394, 0, 160, text[10], 2, D_01AD[10]);
	(*D_0EAA)(&D_23A6, &D_236C);
	if(Pause(4000) == -1) {
		MEM_free(bg);
		MEM_free(p7);
		MEM_free(p8);
		MEM_free(p9);
		MEM_free(p4);
		MEM_free(p5);
		MEM_free(p10);
		MEM_free(txt);
		return 0;
	}
	key = 0;
	for(i = 0; i < 32 && key == 0; i++) {
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_2384, 0, 24, bg, 0);
		(*D_0E9A)(&D_2384, 180, 133, p8, 10);
		(*D_0E9A)(&D_2384, 240, 40, p9, 2);
		(*D_0E9A)(&D_2384, 150, 140, p5, (i > 4) << 1);
		(*D_0E9A)(&D_2384, 80, 150, p4, min(10, i));
		(*D_0E9A)(&D_2384, 270, 45, p10, i);
		(*D_0E9A)(&D_2384, 0, 74, p7, 0);
		(*D_0E9A)(&D_2384, 23, 100, p7, 2);
		PrintText(&D_2394, 0, 160, text[11], 2, D_01AD[11]);
		(*D_0EAA)(&D_23A6, &D_236C);
		if((key = Pause(50)) == -1) {
			MEM_free(bg);
			MEM_free(p7);
			MEM_free(p8);
			MEM_free(p9);
			MEM_free(p4);
			MEM_free(p5);
			MEM_free(p10);
			MEM_free(txt);
			return 0;
		}
	}
	if(key == 0 && Pause(1000) == -1) {
		MEM_free(bg);
		MEM_free(p7);
		MEM_free(p8);
		MEM_free(p9);
		MEM_free(p4);
		MEM_free(p5);
		MEM_free(p10);
		MEM_free(txt);
		return 0;
	}
	for(i = 12; i < 21; i++) {
		(*D_0EAE)(&D_23A6, 0);
		if(i != 13 && i != 16 && i != 20) {
			(*D_0E9A)(&D_2384, 0, 24, bg, 0);
			(*D_0E9A)(&D_2384, 180, 133, p8, 11 - ((i == 12 || i == 17) << 1) - (i == 14 || i == 15));
			(*D_0E9A)(&D_2384, 240, 40, p9, 2);
			(*D_0E9A)(&D_2384, 155, 140, p5, (i < 14) << 1);
			(*D_0E9A)(&D_2384, 80, 150, p4, 10);
			(*D_0E9A)(&D_2384, 270, 45, p10, 31);
			(*D_0E9A)(&D_2384, 0, 74, p7, 0);
			(*D_0E9A)(&D_2384, 23, 100, p7, 1);
			PrintText(&D_2394, 0, 160, text[i], 2, D_01AD[i]);
			(*D_0EAA)(&D_23A6, &D_236C);
			if(Pause(4000) == -1) {
				MEM_free(bg);
				MEM_free(p7);
				MEM_free(p8);
				MEM_free(p9);
				MEM_free(p4);
				MEM_free(p5);
				MEM_free(p10);
				MEM_free(txt);
				return 0;
			}
		} else {
			MEM_free(bg);
			MEM_free(p7);
			MEM_free(p8);
			MEM_free(p9);
			MEM_free(p4);
			MEM_free(p5);
			MEM_free(p10);
			if(i == 13) {
				bg = LoadLzc("scene4a.lzc", 8);
				(*D_0E9A)(&D_2384, 0, 24, bg, 0);
				(*D_0E9A)(&D_2384, 0, 24, bg, 1);
			} else {
				bg = LoadLzc("scene4a.lzc", 7);
				if(i == 16) {
					(*D_0E9A)(&D_2384, 0, 24, bg, 0);
					(*D_0E9A)(&D_2384, 0, 24, bg, 2);
				} else {
					(*D_0E9A)(&D_2384, 0, 24, bg, 0);
					(*D_0E9A)(&D_2384, 0, 24, bg, 1);
				}
			}
			PrintText(&D_2394, 0, 160, text[i], 2, D_01AD[i]);
			(*D_0EAA)(&D_23A6, &D_236C);
			if(Pause(4000) == -1) {
				MEM_free(bg);
				MEM_free(txt);
				return 0;
			}
			MEM_free(bg);
			bg = LoadLzc("scene4a.lzc", 0);
			p7 = LoadLzc("scene4a.lzc", 4);
			p8 = LoadLzc("scene4a.lzc", 6);
			p9 = LoadLzc("scene4a.lzc", 5);
			p4 = LoadLzc("scene4a.lzc", 2);
			p5 = LoadLzc("scene4a.lzc", 3);
			p10 = LoadLzc("scene4b.lzc", 0);
		}
	}
	p2 = LoadLzc("scene4a.lzc", 1);
	p11 = LoadLzc("scene4b.lzc", 1);
	key = 0;
	for(i = 0; i < 32 && key == 0; i++) {
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_2384, 0, 24, bg, 0);
		(*D_0E9A)(&D_2384, 180, 133, p8, 10);
		(*D_0E9A)(&D_2384, 320 - i * 3, (max(0, i - 26) << 1) + 125, p11, i % 4);
		(*D_0E9A)(&D_2384, 242, 24, p2, 0);
		(*D_0E9A)(&D_2384, 240, 40, p9, 2);
		(*D_0E9A)(&D_2384, 160, 140, p5, 0);
		(*D_0E9A)(&D_2384, 80, 150, p4, max(4, min(10, 20 - i)));
		(*D_0E9A)(&D_2384, 270, 45, p10, max(18, min(31, 45 - i)));
		(*D_0E9A)(&D_2384, 0, 74, p7, 0);
		(*D_0E9A)(&D_2384, 23, 100, p7, (i > 6) + 1);
		PrintText(&D_2394, 0, 160, text[21], 2, D_01AD[21]);
		(*D_0EAA)(&D_23A6, &D_236C);
		if((key = Pause(100)) == -1) {
			MEM_free(bg);
			MEM_free(p7);
			MEM_free(p8);
			MEM_free(p9);
			MEM_free(p4);
			MEM_free(p5);
			MEM_free(p10);
			MEM_free(p11);
			MEM_free(p2);
			MEM_free(txt);
			return 0;
		}
	}
	for(i = 22; i < 27; i++) {
		(*D_0EAE)(&D_23A6, 0);
		(*D_0E9A)(&D_2384, 0, 24, bg, 0);
		(*D_0E9A)(&D_2384, 180, 133, p8, 12 - (i == 25) * 3 - (i == 26));
		(*D_0E9A)(&D_2384, 227, 135, p11, (i == 23) + ((i == 24) << 1) + 5);
		(*D_0E9A)(&D_2384, 242, 24, p2, 0);
		(*D_0E9A)(&D_2384, 240, 40, p9, 2);
		(*D_0E9A)(&D_2384, 165, 140, p5, (i == 24) * 3);
		(*D_0E9A)(&D_2384, 80, 150, p4, 4);
		(*D_0E9A)(&D_2384, 270, 45, p10, 18);
		(*D_0E9A)(&D_2384, 0, 74, p7, 0);
		(*D_0E9A)(&D_2384, 23, 100, p7, 2);
		PrintText(&D_2394, 0, 160, text[i], 2, D_01AD[i]);
		(*D_0EAA)(&D_23A6, &D_236C);
		if(Pause(4000) == -1) {
			MEM_free(bg);
			MEM_free(p7);
			MEM_free(p8);
			MEM_free(p9);
			MEM_free(p4);
			MEM_free(p5);
			MEM_free(p10);
			MEM_free(p11);
			MEM_free(p2);
			MEM_free(txt);
			return 0;
		}
	}
	if(D_422A == 1)
		FadeOutMusic(120);
	FadeOut();
	MEM_free(bg);
	MEM_free(p2);
	MEM_free(p7);
	MEM_free(p8);
	MEM_free(p9);
	MEM_free(p4);
	MEM_free(p5);
	MEM_free(p10);
	MEM_free(p11);
	MEM_free(txt);
	return -1;
}


/*the closing credits: lines 9.. of scenetxt.lzc item 4 scroll up over mars.lzc
(0: a key stopped them)*/
/* strings: 0BAA "scenetxt.lzc", 0BB7 "mars.lzc" (_DATA of this file ends at 0BC0)
   variables: D_0E9A, D_0EAA, D_0EAE, D_0EC6, D_236C, D_23A6, D_2394, D_4210, D_422A */
/*C_04FE_792B*/int ScrollCredits() {
	void far *shapes;
	char far *txt;
	int scroll;
	int count;
	int key;
	char far *text[100];
	register int line, i;

	txt = ReadLines("scenetxt.lzc", 4, text);
	for(count = 0; text[count] != 0; count++)
		;
	shapes = LoadLzc("mars.lzc", 0);
	if(D_422A == 1) {
		FadeOutMusic(0);
		PlaySong(D_4210);
	}
	(*D_0EAE)(&D_23A6, 0);
	(*D_0E9A)(&D_23A6, 0, 24, shapes, 0);
	(*D_0E9A)(&D_23A6, 0, 24, shapes, 1);
	FadeIn();
	while(kbhit())
		getch();
	for(line = 0; line < count + 9; line++) {
		for(scroll = 0; scroll < 14; scroll++) {
			(*D_0E9A)(&D_23A6, 0, 24, shapes, 0);
			for(i = 0; i < 10; i++) {
				if(line + i - 9 >= 0 && line + i - 9 < count) {
					PrintText(&D_2394, 11, i * 14 - scroll + 26, text[line + i - 9], 0, 6);
					PrintText(&D_2394, 12, i * 14 - scroll + 25, text[line + i - 9], 0, 14);
				}
			}
			(*D_0EC6)(&D_23A6, 11, 12, 240, 23, 0);
			(*D_0EC6)(&D_23A6, 11, 152, 240, 162, 0);
			VID_waitRetrace();
			(*D_0EAA)(&D_23A6, &D_236C);
			if((key = Pause(10)) == -1) {
				MEM_free(shapes);
				MEM_free(txt);
				return 0;
			}
			if(key != 0)
				line += 4;
		}
	}
	if(D_422A == 1)
		FadeOutMusic(120);
	FadeOut();
	MEM_free(shapes);
	MEM_free(txt);
	return -1;
}
