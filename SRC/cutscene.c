/* flags: -d -mm -p -c -Z -O -y */
/* stub order: 1429 157A 1607 1657 0000 16A8 02E1 042D 0532 0757 07AA 07FA 0951 0A64 0B90 032D */
/*
 * Martian Dreams - GAME.EXE overlay 2F75: the cut scenes. New code (Savage
 * Empire has nothing like it).
 *
 * PlayCutscene shows cut scene n of MIDGAMEX.LZC (MIDGAME1.LZC ...: the
 * digit is "123456789abcdef"[n - 1]): each page is five entries of the
 * file, three texts (one page of the text window each, "*E" skips one)
 * and two pictures (the second drawn over the first, by the player's sex).
 * The others play one cut scene each and change the world to what it
 * tells (the power station, the canal pumps, the dream, the end).
 * ShowMdMap shows a map of MDMAP.LZC; GameEnd is the end of the
 * game: it shuts everything down and starts "martian.exe !".
 */
#include "se.h"

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;/*1 wrap, 2 center, 4 capitalize, 8 runes, 0x10 paginate, 0x20 scroll*/
};

/*the graphic driver's state (Ultima VI's tGr)*/
struct tGr {
	unsigned char header[6];/*_00*/
	void far *font;/*_06*/
	unsigned char textFg, textBg;/*_0a,_0b*/
	unsigned char xorMode;
	unsigned char unknown0d;
	unsigned char animOn;
	unsigned char toBackBuffer;/*_0f*/
};

/*OSI routines (asm)*/
extern cdecl MOUSE_SHOW();
extern cdecl MOUSE_HIDE();
extern cdecl ShutdownSound(void);
extern cdecl MEM_shutdown(int16);
extern cdecl UnhookAll(void);
extern void far *LZC_loadType(char far *name, int16 index, void far *out, int16 memflags, char far *work);

/*other game modules*/
extern ShowPortrait(int16);
extern COMBAT_Kill(int16);
extern ClearMapWindow(int16);
extern VIS_UpdateView();
extern DrawMapWindow(int16);
extern RefreshStatus();
extern MAP_LoadAround();
extern NextArea();/*next object of SearchArea*/
extern int16 SearchArea(int16, int16, int16, int16);/*SearchArea(x1, y1, x2, y2)*/
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);/*AddMapObj(shape, status, amount, x, y, z)*/
extern DeleteObj(int16);
extern MoveObj(int16, int16, int16, int16);/*MoveObj(obj, x, y, z)*/
extern OBJ_DeleteTmpBlocks();
extern MUS_PlaySong(int16);
extern MUS_PlayAreaSong();
extern CON_setClip(struct tClipInfo *);
extern CON_gotoxy(int16, int16);
extern int16 CON_getch();
extern CON_WaitKeyOrClick();
extern cdecl CON_printf(char far *, ...);
extern char far AarghMsg[];/*failure message*/

extern unsigned char MouseOn;
extern int16 ScreenFade;
extern int16 ColorPaper;
extern int16 ColorText;
extern int16 ColorDarkBlue;
extern char StatusDirty;
extern char PromptFlag;
extern int16 MapAnimEnabled;
extern int16 MapX, MapY, MapZ;
extern unsigned char PlayerIsFemale;
extern char SelectMode;
extern char AnimateAtPrompt;
extern int16 EndPagingAtPrompt;
extern int16 StatusDisplay;
extern struct tClipInfo *CurClip;
extern unsigned char IsInConversation;
extern char Screen;/*a drawing surface (main.c)*/
extern unsigned char far *TalkFlagsHi;/*talk flags 8-15*/
extern unsigned char ObjStatus[];
extern char far *LzcWorkBuf;/*work area of the .lzc loader*/
extern unsigned char far *SchedIndex;/*0x100 bytes*/
extern unsigned char far *NPCMode;
extern char far *Schedule;/*5 bytes each*/
extern int16 SchedPointer[0x101];
extern char far *ScratchBuf;
extern unsigned char NPCStatus[];
extern char far *TalkBuf;
extern struct coord ObjPos[];
extern uint16 ObjShapeType[];
extern struct tGr far *GrInfo;
extern union tHandler GrDriver;
extern int16 AreaX, AreaY;
extern struct tClipInfo TextWindows[4];
extern int16 LinesBeforeMore;



#define GR_SET_COLOR(c)              (GrDriver.iii.ofs = 0x06, (*GrDriver.f)(c))
#define GR_FILL_RECT(x0,y0,x1,y1)    (GrDriver.iii.ofs = 0x12, (*GrDriver.f)(x0,y0,x1,y1))
#define GR_DRAW_BITMAP(s,x,y,p,m)    (GrDriver.iii.ofs = 0x30, (*GrDriver.f)(s,x,y,p,m))

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)

/*an object of the map (not an NPC, not in an inventory) on MapZ in the
  rectangle (x1, y1)-(x2, y2)*/
#define InRect(i, x1, y1, x2, y2) (!(!ObjShapeType[i] || (ObjStatus[i] & 0x18) || GetZ(i) != MapZ || \
	GetX(i) < (x1) || GetX(i) > (x2) || GetY(i) < (y1) || GetY(i) > (y2)))

/*clears the text window (the lower right of the screen)*/
#define ClearText() \
	GR_SET_COLOR(ColorPaper); \
	GR_FILL_RECT(0xb8, 0x80, 0x137, 0xbf); \
	CON_gotoxy(0, 0); \
	LinesBeforeMore = CurClip->rows

/*NPC n (removed from the map) back to (x, y, z), its shape type plus
  frame f << 10*/
#define PutBack(n, f, x, y, z) \
	TalkFlagsHi[n] |= 0x80; \
	NPCMode[n] = 0; \
	ObjShapeType[n] = GetType(n) + ((f) << 10); \
	MoveObj(n, x, y, z)

/*after a cut scene: the map window and the status back*/
#define EndCutscene() \
	AnimateAtPrompt = 1; \
	VIS_UpdateView(); \
	ScreenFade = 1; \
	DrawMapWindow(0); \
	IsInConversation = 0; \
	StatusDirty ++; \
	RefreshStatus()

/*20F4*/int16 CutsceneLateSong = 0;/*PlayCutscene starts song 3 at the second page (set by Cutscene4)*/

/*cut scene num (1-15: MIDGAME1.LZC ... MIDGAMEF.LZC, as MIDGAMEX.LZC), pages
  pages of five entries (three texts, two pictures); portraits[page] the
  portrait shown first (0: none)*/
/*C_2F75_0000*/PlayCutscene(int16 num, int16 pages, char *portraits) {
	int16 toBack;
	int16 j;
	int16 oldStatus;
	char fname[13] = "MIDGAMEX.LZC";
	char digits[16] = "123456789abcdef";
	register int16 page, k;

	IsInConversation = 1;
	ClearMapWindow(1);
	SelectMode = 0;
	CON_setClip(&TextWindows[3]);
	CurClip->attrFlags |= 1;
	CurClip->attrFlags |= 0x20;
	CurClip->attrFlags |= 4;
	EndPagingAtPrompt = 1;
	ClearText();
	fname[7] = digits[num - 1];
	k = 0;
	toBack = GrInfo->toBackBuffer;
	GrInfo->toBackBuffer = 0;
	oldStatus = StatusDisplay;
	StatusDisplay = 0x9e;
	if(MouseOn)
		MOUSE_HIDE();
	for(page = 0; page < pages; page ++, k ++) {
		if(CutsceneLateSong && page == 1)
			MUS_PlaySong(3);
		if(portraits[k])
			ShowPortrait((unsigned char)portraits[k]);
		if(LZC_loadType(fname, page * 5 + 4, ScratchBuf, 0, LzcWorkBuf) == 0)
			break;
		GR_DRAW_BITMAP(&Screen, 8, 0x10, ScratchBuf, 0);
		if(LZC_loadType(fname, page * 5 + 5, ScratchBuf, 0, LzcWorkBuf))
			GR_DRAW_BITMAP(&Screen, 8, 0x10, ScratchBuf, PlayerIsFemale);
		ClearText();
		for(j = 0; j < 3; j ++) {
			LZC_loadType(fname, page * 5 + j + 1, TalkBuf, 0, LzcWorkBuf);
			if(TalkBuf[0] == '*' && TalkBuf[1] == 'E')
				continue;
			CurClip->fgColor = ColorText;
			CON_printf(TalkBuf);
			CurClip->fgColor = ColorDarkBlue;
			MapAnimEnabled = 0;
			CON_getch();
			MapAnimEnabled = 1;
			ClearText();
		}
		CurClip->fgColor = ColorText;
	}
	if(MouseOn)
		MOUSE_SHOW();
	StatusDisplay = oldStatus;
	GrInfo->toBackBuffer = toBack;
}

/*cut scene 1*/
/*C_2F75_02E1*/Cutscene1() {
	char portraits[2];

	portraits[0] = portraits[1] = 0;
	PlayCutscene(1, 2, portraits);
	EndCutscene();
	MUS_PlayAreaSong();
}

/*map n of MDMAP.LZC, until a key is pressed*/
/*C_2F75_032D*/ShowMdMap(int16 n) {
	char fname[10] = "MDMAP.LZC";

	ClearMapWindow(1);
	SelectMode = 0;
	GR_SET_COLOR(ColorPaper);
	GR_FILL_RECT(0xb8, 0x80, 0x137, 0xbf);
	GrInfo->toBackBuffer = 0;
	if(MouseOn)
		MOUSE_HIDE();
	if(LZC_loadType(fname, n, ScratchBuf, 0, LzcWorkBuf)) {
		GR_DRAW_BITMAP(&Screen, 8, 0x10, ScratchBuf, 0);
		MapAnimEnabled = 0;
		CON_printf("Press any key...");
		CON_WaitKeyOrClick();
		MapAnimEnabled = 1;
		CON_printf("\n");
	}
	AnimateAtPrompt = 1;
	VIS_UpdateView();
	ScreenFade = 1;
	DrawMapWindow(0);
	MUS_PlayAreaSong();
	if(MouseOn)
		MOUSE_SHOW();
}

/*cut scene 2 (the underground power is on: LeverMachinery 1): around
  (0x3e0, 0x1e0, 0) the objects 0xc9 get frame bit 2, 0xd6 become 0xd7*/
/*C_2F75_042D*/Cutscene2() {
	char portraits[3];
	int16 x, y, z;
	register int16 obj;

	portraits[0] = portraits[1] = portraits[2] = 0;
	PlayCutscene(2, 3, portraits);
	x = MapX;
	y = MapY;
	z = MapZ;
	MapX = 0x3e0;
	MapY = 0x1e0;
	MapZ = 0;
	MAP_LoadAround();
	for(obj = SearchArea(0x3d0, 0x1d0, 0x3f0, 0x1e7); obj >= 0; obj = NextArea()) {
		switch(GetType(obj)) {
			case 0xd6:
				ObjShapeType[obj] = (ObjShapeType[obj] & 0xfc00) + 0xd7;
				break;
			case 0xc9:
				ObjShapeType[obj] = GetType(obj) + ((GetFrame(obj) | 4) << 10);
				break;
		}
	}
	MapX = x;
	MapY = y;
	MapZ = z;
	MAP_LoadAround();
	EndCutscene();
	PromptFlag = 1;
	MUS_PlayAreaSong();
}

/*cut scene 3 (the surface power is on: LeverMachinery 2): around
  (0x3e0, 0x1e0, 0) the objects 0xd7 become 0xd6 again; the objects 0x13b
  around (0xca, 0x295), (0x34d, 0x295), (0x13a, 0x17a), (0x2cd, 0x1fa)
  removed*/
/*C_2F75_0532*/Cutscene3() {
	char portraits[2];
	int16 x, y, z;
	register int16 obj, next;

	portraits[0] = portraits[1] = 0;
	PlayCutscene(3, 2, portraits);
	x = MapX;
	y = MapY;
	z = MapZ;
	MapX = 0x3e0;
	MapY = 0x1e0;
	MapZ = 0;
	MAP_LoadAround();
	for(obj = SearchArea(0x3d0, 0x1d0, 0x3f0, 0x1e7); obj >= 0; obj = NextArea()) {
		if(GetType(obj) == 0xd7)
			ObjShapeType[obj] = (ObjShapeType[obj] & 0xfc00) + 0xd6;
	}
	MapX = 0xca;
	MapY = 0x295;
	MapZ = 0;
	MAP_LoadAround();
	for(obj = SearchArea(0xc8, 0x290, 0xd7, 0x2a0); obj >= 0; obj = next) {
		next = NextArea();
		if(GetType(obj) == 0x13b)
			DeleteObj(obj);
	}
	MapX = 0x34d;
	MapY = 0x295;
	MapZ = 0;
	MAP_LoadAround();
	for(obj = SearchArea(0x348, 0x290, 0x357, 0x2a0); obj >= 0; obj = next) {
		next = NextArea();
		if(GetType(obj) == 0x13b)
			DeleteObj(obj);
	}
	MapX = 0x13a;
	MapY = 0x17a;
	MapZ = 0;
	MAP_LoadAround();
	for(obj = SearchArea(0x138, 0x178, 0x147, 0x187); obj >= 0; obj = next) {
		next = NextArea();
		if(GetType(obj) == 0x13b)
			DeleteObj(obj);
	}
	MapX = 0x2cd;
	MapY = 0x1fa;
	MapZ = 0;
	MAP_LoadAround();
	for(obj = SearchArea(0x2c8, 0x1f8, 0x2d7, 0x207); obj >= 0; obj = next) {
		next = NextArea();
		if(GetType(obj) == 0x13b)
			DeleteObj(obj);
	}
	MapX = x;
	MapY = y;
	MapZ = z;
	MAP_LoadAround();
	EndCutscene();
	MUS_PlayAreaSong();
}

/*cut scene 4 (the dream, overlay 2F5F), with song 3 from its second page*/
/*C_2F75_0757*/Cutscene4() {
	char portraits[2];

	portraits[0] = portraits[1] = 0;
	CutsceneLateSong = 1;
	PlayCutscene(4, 2, portraits);
	CutsceneLateSong = 0;
	EndCutscene();
}

/*cut scene 5*/
/*C_2F75_07AA*/Cutscene5() {
	char portraits[3];

	portraits[0] = portraits[1] = portraits[2] = 0;
	PlayCutscene(5, 3, portraits);
	EndCutscene();
	MUS_PlayAreaSong();
}

/*cut scene 6 (the ruby lenses melt the ice: LeverMachinery 3, cabinet panel 6):
  the ice 0xd2 of frame 0-1 in (0x13b, 0x116)-(0x140, 0x11a) on MapZ 0 removed*/
/*C_2F75_07FA*/Cutscene6() {
	char portraits[3];
	int16 x, y, z;
	register int16 obj;

	portraits[0] = portraits[1] = portraits[2] = 0;
	PlayCutscene(6, 3, portraits);
	x = MapX;
	y = MapY;
	z = MapZ;
	MapX = 0x13d;
	MapY = 0x118;
	MapZ = 0;
	MAP_LoadAround();
	for(obj = 0x101; obj < 0xc00; obj ++) {
		if(InRect(obj, 0x13b, 0x116, 0x140, 0x11a) && GetType(obj) == 0xd2 && GetFrame(obj) < 2)
			DeleteObj(obj);
	}
	MapX = x;
	MapY = y;
	MapZ = z;
	MAP_LoadAround();
	EndCutscene();
	MUS_PlayAreaSong();
}

/*cut scene 7 (the valves open the canals: LeverMachinery 4, panel 7, the
  valve; nothing in the game turns the pumps off again): around (0x241,
  0x27b, 0) the objects 0x1cb of frame 1 get frame 2, a new 0x1cc at
  (0x243, 0x27a)*/
/*C_2F75_0951*/Cutscene7() {
	char portraits[3];
	int16 x, y, z;
	register int16 obj;

	portraits[0] = portraits[1] = portraits[2] = 0;
	PlayCutscene(7, 3, portraits);
	x = MapX;
	y = MapY;
	z = MapZ;
	MapX = 0x241;
	MapY = 0x27b;
	MapZ = 0;
	MAP_LoadAround();
	for(obj = SearchArea(AreaX, AreaY, AreaX + 0x27, AreaY + 0x27); obj >= 0; obj = NextArea()) {
		if(ObjShapeType[obj] == 0x5cb)
			ObjShapeType[obj] = GetType(obj) + 0x800;
	}
	if((obj = AddMapObj(0x1cc, 0, 0, 0x243, 0x27a, 0)) < 0)
		CON_printf(AarghMsg);
	else
		ObjStatus[obj] &= ~0x20;
	MapX = x;
	MapY = y;
	MapZ = z;
	MAP_LoadAround();
	EndCutscene();
	MUS_PlayAreaSong();
}

/*cut scene 8 (LeverMachinery 6): COMBAT_Kill on NPCs 0x41, 0x43-0x45;
  NPCs 0x42 to (0x394, 0x284, 0), 0x46 to (0x393, 0x282, 0), 0x40 removed*/
/*C_2F75_0A64*/Cutscene8() {
	char portraits[3];
	int16 x, y, z;

	portraits[0] = portraits[1] = portraits[2] = 0;
	PlayCutscene(8, 3, portraits);
	x = MapX;
	y = MapY;
	z = MapZ;
	MapX = 0x384;
	MapY = 0x280;
	MapZ = 0;
	MAP_LoadAround();
	COMBAT_Kill(0x41);
	COMBAT_Kill(0x43);
	COMBAT_Kill(0x44);
	COMBAT_Kill(0x45);
	TalkFlagsHi[0x42] |= 0x80;
	NPCMode[0x42] = 0;
	MoveObj(0x42, 0x394, 0x284, 0);
	TalkFlagsHi[0x40] |= 0x80;
	NPCMode[0x40] = 0;
	MoveObj(0x40, 0x40, 0x40, 6);
	TalkFlagsHi[0x46] |= 0x80;
	NPCMode[0x46] = 0;
	MoveObj(0x46, 0x393, 0x282, 0);
	MapX = x;
	MapY = y;
	MapZ = z;
	MAP_LoadAround();
	EndCutscene();
	MUS_PlayAreaSong();
}

/*cut scene 9 (no music): three objects 0xef at (-2, -2), (-2, -1),
  (-2, 0) from the avatar on MapZ 0; on MapZ 4 NPC 5 (unless NPCStatus bit
  7), 0x10-0x16, 0x19-0x1c, 0x1e, 0x2a-0x2c in a row at y 0xc4 (frame 9),
  0x2d-0x31, 0x34-0x39, 0x3b, 0x3d, 0x3e, 0x42, 0x46 at y 0xc2 (frame 1),
  0x33 at (0xa1, 0xbd) (frame 13)*/
/*C_2F75_0B90*/Cutscene9() {
	char portraits[2];
	int16 x, y, z;
	register int16 obj;

	MUS_PlaySong(-1);
	portraits[0] = portraits[1] = 0;
	PlayCutscene(9, 2, portraits);
	x = MapX;
	y = MapY;
	z = MapZ;
	MapX = GetX(1);
	MapY = GetY(1);
	MapZ = 0;
	MAP_LoadAround();
	if((obj = AddMapObj(0xef, 0, 0, MapX - 2, MapY - 2, MapZ)) < 0)
		CON_printf(AarghMsg);
	else
		ObjStatus[obj] &= ~0x20;
	if((obj = AddMapObj(0xef, 0, 0, MapX - 2, MapY - 1, MapZ)) < 0)
		CON_printf(AarghMsg);
	else
		ObjStatus[obj] &= ~0x20;
	if((obj = AddMapObj(0xef, 0, 0, MapX - 2, MapY, MapZ)) < 0)
		CON_printf(AarghMsg);
	else
		ObjStatus[obj] &= ~0x20;
	MapX = 0xb0;
	MapY = 0xb0;
	MapZ = 4;
	MAP_LoadAround();
	if(!(NPCStatus[5] & 0x80)) {
		PutBack(5, 9, 0xa3, 0xc4, 4);
	}
	PutBack(0x10, 9, 0xa4, 0xc4, 4);
	PutBack(0x11, 9, 0xa5, 0xc4, 4);
	PutBack(0x12, 9, 0xa6, 0xc4, 4);
	PutBack(0x13, 9, 0xa7, 0xc4, 4);
	PutBack(0x14, 9, 0xa8, 0xc4, 4);
	PutBack(0x15, 9, 0xa9, 0xc4, 4);
	PutBack(0x16, 9, 0xaa, 0xc4, 4);
	PutBack(0x19, 9, 0xab, 0xc4, 4);
	PutBack(0x1a, 9, 0xac, 0xc4, 4);
	PutBack(0x1b, 9, 0xad, 0xc4, 4);
	PutBack(0x1c, 9, 0xae, 0xc4, 4);
	PutBack(0x1e, 9, 0xaf, 0xc4, 4);
	PutBack(0x2a, 9, 0xb0, 0xc4, 4);
	PutBack(0x2b, 9, 0xb1, 0xc4, 4);
	PutBack(0x2c, 9, 0xb2, 0xc4, 4);
	PutBack(0x2d, 1, 0xa3, 0xc2, 4);
	PutBack(0x2e, 1, 0xa4, 0xc2, 4);
	PutBack(0x2f, 1, 0xa5, 0xc2, 4);
	PutBack(0x30, 1, 0xa6, 0xc2, 4);
	PutBack(0x31, 1, 0xa7, 0xc2, 4);
	PutBack(0x34, 1, 0xa9, 0xc2, 4);
	PutBack(0x35, 1, 0xaa, 0xc2, 4);
	PutBack(0x36, 1, 0xab, 0xc2, 4);
	PutBack(0x37, 1, 0xac, 0xc2, 4);
	PutBack(0x38, 1, 0xad, 0xc2, 4);
	PutBack(0x39, 1, 0xae, 0xc2, 4);
	PutBack(0x3b, 1, 0xaf, 0xc2, 4);
	PutBack(0x3d, 1, 0xb0, 0xc2, 4);
	PutBack(0x3e, 1, 0xb1, 0xc2, 4);
	PutBack(0x42, 1, 0xb2, 0xc2, 4);
	PutBack(0x46, 1, 0xa8, 0xc2, 4);
	PutBack(0x33, 13, 0xa1, 0xbd, 4);
	MapX = x;
	MapY = y;
	MapZ = z;
	MAP_LoadAround();
	AnimateAtPrompt = 1;
	ScreenFade = 1;
	IsInConversation = 0;
}

/*cut scene 10 (talk.c): NPCs 0x20, 0x22-0x29 and 0x50-0x57 removed,
  0x2a-0x31 back to their schedule's place*/
/*C_2F75_1429*/Cutscene10() {
	char portraits[2];
	int16 x, y, z;
	struct coord pos;
	register int16 i;

	portraits[0] = portraits[1] = 0;
	PlayCutscene(10, 2, portraits);
	x = MapX;
	y = MapY;
	z = MapZ;
	for(i = 0x20; i <= 0x29; i ++) {
		if(i == 0x21)
			continue;
		TalkFlagsHi[i] |= 0x80;
		NPCMode[i] = 0;
		MoveObj(i, i, i, 6);
	}
	for(i = 0x2a; i <= 0x31; i ++) {
		TalkFlagsHi[i] &= ~0x80;
		pos = *(struct coord far *)(Schedule + (SchedIndex[i] + SchedPointer[i]) * 5 + 2);
		MoveObj(i, (*(int16 *)&pos.raw[0]) & 0x3ff, ((*(int16 *)&pos.raw[1]) >> 2) & 0x3ff, pos.raw[2] >> 4);
	}
	MapX = 0x64;
	MapY = 0x3b;
	MapZ = 2;
	MAP_LoadAround();
	for(i = 0x50; i <= 0x57; i ++) {
		TalkFlagsHi[i] |= 0x80;
		NPCMode[i] = 0;
		MoveObj(i, i, i, 6);
	}
	MapX = x;
	MapY = y;
	MapZ = z;
	MAP_LoadAround();
	EndCutscene();
	MUS_PlayAreaSong();
}

/*cut scene 11 (the dream, overlay 2F5F; song 0xf)*/
/*C_2F75_157A*/Cutscene11() {
	char portraits[2];
	int16 x, y, z;

	MUS_PlaySong(0xf);
	portraits[0] = portraits[1] = 0;
	PlayCutscene(0xb, 1, portraits);
	x = MapX;
	y = MapY;
	z = MapZ;
	MapX = 0;
	MapY = 0;
	MapZ = 0;
	MAP_LoadAround();
	MapX = x;
	MapY = y;
	MapZ = z;
	MAP_LoadAround();
	EndCutscene();
}

/*cut scene 12*/
/*C_2F75_1607*/Cutscene12() {
	char portraits[3];

	portraits[0] = portraits[1] = portraits[2] = 0;
	PlayCutscene(0xc, 3, portraits);
	EndCutscene();
	MUS_PlayAreaSong();
}

/*cut scene 13 (no music)*/
/*C_2F75_1657*/Cutscene13() {
	char portraits[1];

	MUS_PlaySong(-1);
	portraits[0] = 0;
	PlayCutscene(0xd, 1, portraits);
	EndCutscene();
	MUS_PlayAreaSong();
}

/*the end of the game (talk.c): song 0xc, everything shut down, then
  MARTIAN.EXE runs the ending ("!")*/
/*C_2F75_16A8*/GameEnd() {
	MUS_PlaySong(0xc);
	OBJ_DeleteTmpBlocks();
	ShutdownSound();
	MEM_shutdown(8);
	UnhookAll();
	GrDriver.iii.ofs = 3;
	(*GrDriver.f)();
	for(;;)
		execl("martian.exe", "martian.exe", "!", 0L);
}
