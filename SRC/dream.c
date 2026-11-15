/* flags: -d -mm -p -c -Z -O -y */
/* stub order: 0000 0AC1 0477 0688 02CE */
/*
 * Martian Dreams - GAME.EXE overlay 2F5F: the dream. New code (Savage
 * Empire has no dream).
 *
 * When the avatar (NPC 1) sleeps, NPC 0, the dreamer (shape 0x157, or 0x158
 * for a woman), goes alone into the dream world on MapZ 2 or 3: the party
 * is reduced to it (Active = PartySize, Party[Active] = 0, its name copied),
 * the panel turns gray (ColorPaper 7) and D_16AA gets 0x10. Dream triggers
 * are map objects stepped on: their status bits (& 0xe5) give the kind,
 * quality and quantity a place, the frame a teleport mode
 * (DreamTrigger). Waking up (DreamWakeUp) puts everything back at 8:00 on
 * the date when the dream began (+1 day after 8:00). Another member who
 * sleeps "dreams until dawn" while time passes.
 */
#include "se.h"

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;/*1 wrap, 2 center, 4 capitalize, 8 runes, 0x10 paginate, 0x20 scroll*/
};

/*OSI routines (asm)*/
extern int16 OSI_rand(int16, int16);/*OSI_rand(lo, hi)*/
extern cdecl MOUSE_SERVE();

/*other game modules*/
extern PaletteAnimation();
extern COMBAT_breakOff();
extern int16 MaxHP(int16);
extern CloneObject(int16, int16);
extern DrawSkyBar();
extern DisplayTick();
extern PassTime(int16);/*PassTime(minutes)*/
extern SetPartyMode();
extern VIS_UpdateView();
extern MAP_LoadAround();
extern PartyTeleport(int16, int16, int16, int16);
extern int16 OBJ_FindTypeAt(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern DeleteObj(int16);
extern OBJ_PurgeFarLocals(int16, int16, int16);
extern MoveObj(int16, int16, int16, int16);/*MoveObj(obj, x, y, z)*/
extern ShowObjects();
extern char *GetObjectString(int16);
extern OBJ_DeleteContents(int16);
extern C_1FA9_02CE(int16);/*the morning song*/
extern MUS_PlayAreaSong();
extern STAT_ShowParty(int16);
extern int16 CON_mouseOffAt(int16, int16, int16, int16);
extern CON_mouseOn(int16);
extern CON_setClip(struct tClipInfo *);
extern cdecl CON_printf(char far *, ...);
extern C_2F68_0000(int16, int16);
extern C_2F68_02A9();
extern C_2F68_037E(int16, int16);
extern C_2F68_03ED();
extern C_2F68_0494();
extern C_2F68_0785();
extern C_2F68_07E6();
extern C_2F68_0CA9();
extern C_2F68_0F20();
extern C_2F68_0FE1(int16, int16);
extern C_2F68_1393();
extern C_2F68_165C(int16, int16, int16);
extern C_2F75_0757();
extern C_2F75_157A();
extern char far C_2BB4_22F1[];/*failure message*/

extern unsigned char MouseOn;
extern int16 ScreenFade;
extern int16 ColorPaper;
extern int16 D_082E;/*default paper color*/
extern char D_0F12;/*StatusDirty*/
extern char D_0F13;/*PromptFlag*/
extern int16 D_0F18;/*MapAnimEnabled*/
extern int16 VisibilityDirty;
extern unsigned char Time_M;
extern unsigned char Time_H;
extern unsigned char Date_D;
extern unsigned char Date_M;
extern int16 Date_Y;
extern unsigned char Active;
extern char AmbientLight;
extern int16 MapX, MapY, MapZ;
extern unsigned char D_1684, D_1685;/*where the dreamer wakes up (x, y)*/
extern unsigned char DrunkCounter;
extern unsigned char D_16A3;
extern char InCombat;
extern char SoloMember;
extern char SavedNPCMode;
extern unsigned char D_16A8;/*the player is a woman*/
extern int32 D_16AA;/*flags (0x10: the dream)*/
extern int16 D_16AE;/*rotation of the map view*/
extern struct { unsigned a:4, b:4, c:4, d:4; } D_16B0[];/*talents*/
extern unsigned char D_16D0, D_16D1;/*the day and month to wake up*/
extern int16 D_16D2;/*the year to wake up*/
extern int16 DreamStage;/*DreamStage: the kind of the last dream trigger*/
extern int16 StatusMember;
extern int16 StatusDisplay;/*0x90 a member, 0x91 the party, 0x92 the equipment*/
extern struct tClipInfo *CurClip;
extern char IsResting;
extern unsigned char far *D_3262;/*talk flags 8-15*/
extern unsigned char D_3266[];/*ObjStatus*/
extern char D_44C4[];/*MovePts*/
extern unsigned char D_45D0[];/*DEXTE*/
extern unsigned char far *D_46D0;/*NPCMode*/
extern unsigned char D_4EA4[];/*Party*/
extern char far *D_4EBA;/*Schedule (5 bytes each)*/
extern int16 D_4EBE[0x101];/*SchedPointer*/
extern char D_50CE[][14];/*Names*/
extern int16 D_54BA[];/*Amount: quantity (low byte), quality (high byte)*/
extern unsigned char D_6CBA[];/*NPCStatus*/
extern unsigned char D_6DBE[];/*HitPoints*/
extern unsigned char D_6EC6[];/*TalkFlags*/
extern struct coord D_78AE[];/*ObjPos*/
extern unsigned char D_9CC0;/*PartySize*/
extern uint16 D_9CC4[];/*ObjShapeType*/
extern union tHandler D_BA6A;/*GrDriver*/
extern struct tClipInfo D_EBD0[4];/*TextWindows*/

#define StatusDirty D_0F12
#define PromptFlag D_0F13
#define MapAnimEnabled D_0F18
#define WakeDay D_16D0
#define WakeMonth D_16D1
#define WakeYear D_16D2
#define ObjStatus D_3266
#define MovePts D_44C4
#define DEXTE D_45D0
#define NPCMode D_46D0
#define Party D_4EA4
#define Schedule D_4EBA
#define SchedPointer D_4EBE
#define Names D_50CE
#define Amount D_54BA
#define NPCStatus D_6CBA
#define HitPoints D_6DBE
#define TalkFlags D_6EC6
#define ObjPos D_78AE
#define PartySize D_9CC0
#define ObjShapeType D_9CC4
#define GrDriver D_BA6A
#define TextWindows D_EBD0


#define GR_SET_COLOR(c)              (GrDriver.iii.ofs = 0x06, (*GrDriver.f)(c))
#define GR_FILL_RECT(x0,y0,x1,y1)    (GrDriver.iii.ofs = 0x12, (*GrDriver.f)(x0,y0,x1,y1))

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])

/*1E48*/int16 D_1E48 = -1;/*the AnimTile entry of tile 0x6f (-1: not looked up yet; overlays 2F63, 2F68)*/
/*1E4A*/int16 D_1E4A = 0;/*how much faster tile 0x6f animates, 0-4 (overlays 2F63, 2F68)*/

DreamTrigger(int16);

/*npc goes to sleep: the avatar (1) enters the dream (how <= 1: at
  (0x64, 0x3b), else (0x93, 0x34), MapZ 2), another member dreams until
  dawn while the time passes*/
/*C_2F5F_0000*/DreamSleep(register int16 npc, int16 how) {
	struct tClipInfo *savedClip;
	int16 mouse;
	register int16 i;

	WakeDay = Date_D;
	WakeMonth = Date_M;
	WakeYear = Date_Y;
	if(Time_H >= 8) {
		if(++WakeDay > 28) {
			WakeDay = 1;
			if(++WakeMonth > 12) {
				WakeMonth = 1;
				WakeYear ++;
			}
		}
	}
	if(npc == 1) {
		C_2F75_0757();
		if(SoloMember >= 0)
			SetPartyMode();
		CloneObject(1, 0);
		ObjShapeType[0] = GetType(0) + 0x400;
		HitPoints[0] = MaxHP(1);
		ObjStatus[0] &= 0x18;
		NPCStatus[0] = (NPCStatus[0] & 0x9f) | 0x40;
		NPCStatus[0] |= 0x80;
		NPCStatus[0] &= 0xe0;
		MovePts[0] = DEXTE[0];
		DrunkCounter = 0;
		if(how <= 1)
			MoveObj(0, 0x64, 0x3b, 2);
		else
			MoveObj(0, 0x93, 0x34, 2);
		ObjShapeType[0] = (ObjShapeType[0] & 0xfc00) + (D_16A8 ? 0x158 : 0x157);
		NPCMode[0] = 2;
		*(int16 *)D_16B0 = 0;
		Party[Active = PartySize] = 0;
		for(i = 0; i < 14; i ++)
			Names[Active][i] = Names[0][i];
		StatusDisplay = 0x92;
		StatusMember = PartySize;
		StatusDirty = 1;
		MapX = GetX(0);
		MapY = GetY(0);
		MapZ = GetZ(0);
		MAP_LoadAround();
		OBJ_DeleteContents(0);
		MapAnimEnabled = 1;
		PromptFlag = 1;
		ColorPaper = 7;
		GR_SET_COLOR(7);
		mouse = CON_mouseOffAt(184, 128, 311, 195);
		GR_FILL_RECT(184, 128, 311, 195);
		CON_mouseOn(mouse);
		savedClip = CurClip;
		for(i = 0; i < 4; i ++) {
			CON_setClip(&TextWindows[i]);
			CurClip->bgColor = 7;
		}
		CON_setClip(savedClip);
		D_16AA |= 0x10;
		AmbientLight = 6;
		DreamStage = 0;
		D_1684 = 0;
		D_1685 = 0;
		MUS_PlayAreaSong();
	} else {
		NPCStatus[npc] |= 4;
		StatusDisplay = 0x91;
		STAT_ShowParty(0);
		CON_printf("%s dreams until dawn while the party waits.\n", GetObjectString(npc));
		IsResting = 1;
		PassTime(60 - Time_M);
		while(Date_Y < WakeYear || Date_M < WakeMonth || Date_D < WakeDay || Time_H < 8) {
			PassTime(20);
			VIS_UpdateView();
			ShowObjects();
			DisplayTick();
			PaletteAnimation();
			if(MouseOn)
				MOUSE_SERVE();
		}
		IsResting = 0;
		NPCStatus[npc] &= ~4;
		C_1FA9_02CE(1);
	}
}

/*the dreamer enters the dream at (0x35, 0x26, 3)*/
/*C_2F5F_02CE*/DreamEnter() {
	struct tClipInfo *savedClip;
	int16 mouse;
	register int16 i;

	C_2F75_157A();
	if(SoloMember >= 0)
		SetPartyMode();
	CloneObject(1, 0);
	HitPoints[0] = MaxHP(1);
	ObjShapeType[0] = GetType(0) + 0x400;
	ObjStatus[0] &= 0x18;
	NPCStatus[0] = (NPCStatus[0] & 0x9f) | 0x40;
	NPCStatus[0] |= 0x80;
	NPCStatus[0] &= 0xe0;
	MovePts[0] = DEXTE[0];
	MoveObj(0, 0x35, 0x26, 3);
	ObjShapeType[0] = (ObjShapeType[0] & 0xfc00) + (D_16A8 ? 0x158 : 0x157);
	NPCMode[0] = 2;
	*(int16 *)D_16B0 = 0;
	OBJ_DeleteContents(0);
	Party[Active = PartySize] = 0;
	for(i = 0; i < 14; i ++)
		Names[Active][i] = Names[0][i];
	StatusDisplay = 0x92;
	StatusMember = PartySize;
	StatusDirty = 1;
	MapX = 0x35;
	MapY = 0x26;
	MapZ = 3;
	MAP_LoadAround();
	MapAnimEnabled = 1;
	PromptFlag = 1;
	ColorPaper = 7;
	GR_SET_COLOR(7);
	mouse = CON_mouseOffAt(184, 128, 311, 195);
	GR_FILL_RECT(184, 128, 311, 195);
	CON_mouseOn(mouse);
	savedClip = CurClip;
	for(i = 0; i < 4; i ++) {
		CON_setClip(&TextWindows[i]);
		CurClip->bgColor = 7;
	}
	CON_setClip(savedClip);
	D_16AA |= 0x10;
	DreamStage = 0xe1;
	AmbientLight = 6;
	D_1684 = 0x35;
	D_1685 = 0x26;
}

/*wake up from the dream: 8:00 on the wake-up date, the talents cleared,
  poisoned members lose hit points, back to NPC 1's place*/
/*C_2F5F_0477*/DreamWakeUp() {
	struct tClipInfo *savedClip;
	int16 mouse;
	register int16 i, npc;

	DreamTrigger(0);
	Active = 0;
	NPCStatus[0] &= 0x7f;
	OBJ_DeleteContents(0);
	D_16AE = 0;
	D_16A3 = 0;
	Time_H = 8;
	Time_M = 0;
	Date_D = WakeDay;
	Date_M = WakeMonth;
	Date_Y = WakeYear;
	for(i = 0; i < PartySize; i ++) {
		npc = Party[i];
		D_16B0[npc].a = 0;
		D_16B0[npc].b = 0;
		D_16B0[npc].c = 0;
		if(NPCStatus[npc] & 8) {
			if(HitPoints[npc] > 30)
				HitPoints[npc] -= OSI_rand(24, 30);
			else if(HitPoints[npc] > 10)
				HitPoints[npc] = OSI_rand(1, 10);
			else
				HitPoints[npc] = 1;
		}
	}
	MapX = GetX(1);
	MapY = GetY(1);
	MapZ = GetZ(1);
	MAP_LoadAround();
	NPCMode[0] = 0;
	MoveObj(0, MapX, MapY, 6);
	PassTime(0);
	DrawSkyBar();
	ColorPaper = D_082E;
	GR_SET_COLOR(D_082E);
	mouse = CON_mouseOffAt(184, 128, 311, 195);
	GR_FILL_RECT(184, 128, 311, 195);
	CON_mouseOn(mouse);
	savedClip = CurClip;
	for(i = 0; i < 4; i ++) {
		CON_setClip(&TextWindows[i]);
		CurClip->bgColor = D_082E;
	}
	CON_setClip(savedClip);
	D_16AA &= ~0x10L;
	StatusMember = 0;
	MapAnimEnabled = 1;
	PromptFlag = 1;
	StatusDirty = 1;
	VisibilityDirty = 1;
	StatusDirty = 1;
	ScreenFade = 1;
	SavedNPCMode = NPCMode[Party[1]];
	NPCMode[1] = 2;
	SetPartyMode();
	if(InCombat)
		COMBAT_breakOff();
	CON_printf("You wake up.\n");
	DisplayTick();
	C_1FA9_02CE(1);
}

/*the dreamer steps on a dream trigger (0: none, when waking up): its
  kind (ObjStatus & 0xe5) ends the previous stage's effect, does this
  one's, then the party is teleported to (quality, quantity) on MapZ with
  the frame as the mode*/
/*C_2F5F_0688*/DreamTrigger(int16 obj) {
	int16 y2, kind, x, y, frame;
	register int16 obj2;

	kind = ObjStatus[obj] & 0xe5;
	x = GetQual(obj);
	y = GetQuan(obj);
	frame = GetFrame(obj);
	if(DreamStage && kind != 0xa5 && kind != 0xe5) {
		switch(DreamStage) {
		case 5: C_2F68_0F20(); break;
		case 0x20: OBJ_DeleteContents(0); break;
		case 0x40: ObjShapeType[0] = GetType(1) | 0x2400; break;
		case 0x44: C_2F68_0785(); break;
		case 0xa0: C_2F68_02A9(); break;
		}
	}
	if(kind) {
		switch(kind) {
		case 1:
			DreamStage = 0;
			return;
		case 4:
			C_2F68_0FE1(x, y);
			break;
		case 0x24:
			DreamWakeUp();
			return;
		case 0x40:
			C_2F68_03ED();
			break;
		case 0x85:
			C_2F68_165C(DreamStage != 0x85, x, y);
			break;
		case 0xa1:
			C_2F68_07E6();
			break;
		case 0xa4:
			TalkFlags[0x56] &= 1;
			MoveObj(0x56, 0x19, 0x6f, 2);
			ObjShapeType[0x56] = GetType(0x56) + 0x400;
			NPCMode[0x56] = 0;
			D_3262[0x56] |= 0x80;
			NPCStatus[0x56] &= 0x9f;
			break;
		case 0xc1:
			TalkFlags[0x6b] |= 4;
			if((obj2 = OBJ_FindTypeAt(0x124, GetX(obj), GetY(obj) - 1)) >= 0)
				DeleteObj(obj2);
			DeleteObj(obj);
			if(OSI_rand(1, 100) <= 50) {
				obj2 = GetX(0x6d);
				y2 = GetY(0x6d);
				MoveObj(0x6d, GetX(0x67), GetY(0x67), MapZ);
				MoveObj(0x67, obj2, y2, MapZ);
				obj2 = GetFrame(0x6d);
				ObjShapeType[0x6d] = GetType(0x6d) + (GetFrame(0x67) << 10);
				ObjShapeType[0x67] = GetType(0x67) + (obj2 << 10);
			}
			break;
		case 0xc4:
			OBJ_DeleteContents(0);
			break;
		case 0xc5:
			TalkFlags[0x6a] |= 4;
			if((obj2 = OBJ_FindTypeAt(0x124, GetX(obj), GetY(obj) - 1)) >= 0)
				DeleteObj(obj2);
			DeleteObj(obj);
			break;
		case 0xe0:
			TalkFlags[0x6c] |= 4;
			if((obj2 = OBJ_FindTypeAt(0x124, GetX(obj), GetY(obj) - 1)) >= 0)
				DeleteObj(obj2);
			DeleteObj(obj);
			break;
		case 0xc0:
			HitPoints[0] = MaxHP(1);
			StatusDirty = 1;
			OBJ_DeleteContents(0);
			break;
		}
	}
	if(kind != 0xa5) {
		OBJ_PurgeFarLocals(-1, -1, -1);
		D_16A3 = 0;
	}
	D_16B0[0].a = 0;
	D_16B0[0].b = 0;
	D_16B0[0].c = 0;
	PartyTeleport(x, y, MapZ, frame);
	if(kind) {
		switch(kind) {
		case 5:
			C_2F68_0CA9();
			break;
		case 0x20:
			C_2F68_0000(x, y);
			break;
		case 0x25:
			C_2F68_037E(x, y);
			break;
		case 0x44:
			C_2F68_0494();
			break;
		case 0x60:
			C_2F68_1393();
			break;
		case 0xa5:
			return;
		case 0xc4:
			D_1684 = x;
			D_1685 = y;
			break;
		case 0xc5:
			TalkFlags[0x68] = 0;
			break;
		case 0xc0:
			HitPoints[0] = MaxHP(1);
			D_1684 = x;
			D_1685 = y;
			OBJ_DeleteContents(0);
			break;
		case 0xe5:
			if(!(TalkFlags[0x66] & 8))
				D_16A3 = 6;
			return;
		}
	}
	ShowObjects();
	DreamStage = kind;
}

/*npc (0x5c: 0x53 instead, 0x5c leaves the map) goes back to its
  schedule's place, where the objects 0x1cd and 0x124 (above) are
  removed, and gets talk flag 0x80; with NPCs 0x52, 0x54, 0x55 and 0x56
  back TalkFlags[0x60] gets 0x10, with 0x50-0x57 TalkFlags[0x20]*/
/*C_2F5F_0AC1*/C_2F5F_0AC1(register int16 npc) {
	struct coord pos;
	int16 x, y;
	register int16 obj;

	switch(npc) {
	case 0x5c:
		MoveObj(npc, npc, npc, 6);
		D_3262[npc] |= 0x80;
		NPCMode[npc] = 0;
		npc = 0x53;
	}
	pos = *(struct coord far *)(Schedule + SchedPointer[npc] * 5 + 2);
	x = (*(int16 *)&pos.raw[0]) & 0x3ff;
	y = ((*(int16 *)&pos.raw[1]) >> 2) & 0x3ff;
	MoveObj(npc, x, y, MapZ);
	if((obj = OBJ_FindTypeAt(0x1cd, x, y)) < 0)
		CON_printf(C_2BB4_22F1);
	else
		DeleteObj(obj);
	if((obj = OBJ_FindTypeAt(0x124, x, y - 1)) < 0)
		CON_printf(C_2BB4_22F1);
	else
		DeleteObj(obj);
	TalkFlags[npc] |= 0x80;
	if((TalkFlags[0x56] & 0x80) && (TalkFlags[0x54] & 0x80) && (TalkFlags[0x52] & 0x80) && (TalkFlags[0x55] & 0x80))
		TalkFlags[0x60] |= 0x10;
	if((TalkFlags[0x50] & 0x80) && (TalkFlags[0x57] & 0x80) && (TalkFlags[0x56] & 0x80) && (TalkFlags[0x54] & 0x80) &&
		(TalkFlags[0x55] & 0x80) && (TalkFlags[0x52] & 0x80) && (TalkFlags[0x51] & 0x80) && (TalkFlags[0x53] & 0x80))
		TalkFlags[0x20] |= 0x10;
}
