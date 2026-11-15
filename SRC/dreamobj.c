/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE overlay 2F63: more of the dream (with dream.c,
 * overlay 2F5F, and dreamstg.c, overlay 2F68). New code (Savage Empire has
 * no dream).
 *
 * C_2F63_0000 is AddMapObj clearing the new object's ObjStatus 0x20 (the
 * dream's stages make their objects with it); C_2F63_0043 speeds up the
 * animation of tile 0x6f (AnimTile entry D_1E48, steps D_1E4A); the
 * dreamer passing out (C_2F63_0086) wakes the party in the real world;
 * C_2F63_062F calls conversation 0x66 after creatures 0x181/0x16b;
 * C_2F63_083C moves a creature towards an object 0xfb (or the party),
 * scoring the squares with C_2F63_0726; C_2F63_0A13, C_2F63_0ACF and
 * C_2F63_0C0C make a chain of objects 0x85 go off.
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
extern void cdecl TimerDelay(int32);

/*other game modules*/
extern C_07D1_02C4();/*COMBAT_breakOff*/
extern C_07D1_06C1(int16);/*SetTypeUnconscious*/
extern int16 C_07D1_1D64(int16, int16, int16);/*hurt (objNum, damage, ?)*/
extern int16 C_07D1_35A8(int16, int16, int16, int16);/*distance of (x1, y1) and (x2, y2)*/
extern C_0E1A_11D2();/*ResetMapView*/
extern int16 C_100B_0247(int16);
extern int16 C_100B_02C0(int16);/*STAT_GetEquipSlot*/
extern int16 NPC_TryStep(int16, int16, int16);
extern C_1157_0312();/*VIS_UpdateView*/
extern C_120E_0009();
extern C_120E_0221();/*DisplayTick*/
extern unsigned char C_1534_1072(int16, int16, int16);/*GetTileAtXYZ*/
extern C_1534_056F();/*MAP_LoadAround*/
extern int16 C_1534_1129(int16, int16);/*MapDistance*/
extern int16 C_1649_000A(int16);/*GetMonsterClass(type): < 0 if none*/
extern C_170D_0868(int16, int16);/*SubMov(npc, points)*/
extern C_170D_2231(int16, int16, int16);/*TryStraightMove*/
extern int16 C_1B09_032A();/*NextLoc*/
extern int16 C_1B09_07D8(int16, int16, int16);/*FindLoc(x, y, z)*/
extern int16 C_1B09_09C7(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 C_1B09_0B1D(int16);/*FindInv: first object of an inventory*/
extern int16 C_1B09_0B76(void);/*NextInv*/
extern int16 C_1B09_0DD7(void);/*NextArea: next object of SearchArea*/
extern int16 C_1B09_0EDE(int16, int16, int16, int16);/*SearchArea(x1, y1, x2, y2)*/
extern int16 C_1B09_116E(int16, int16, int16, int16, int16, int16);/*AddMapObj(type, status, amount, x, y, z)*/
extern int16 C_1B09_12EE(int16, int16, int16, int16);/*AddInvObj(shape, position, quantity, owner)*/
extern C_1B09_1872(int16);/*DeleteObj*/
extern C_1B09_1F52(int16, int16, int16, int16);/*MoveObj(obj, x, y, z)*/
extern C_1B09_3E18();/*ShowObjects*/
extern C_1B09_4963(int16);
extern char *C_1B09_4759(int16);/*GetObjectString*/
extern C_1FA9_02CE(int16);/*the morning song*/
extern C_1FA9_0919(int16);/*MUS_PlaySfx*/
extern C_2077_1179(int16);/*STAT_RefreshInventory*/
extern STAT_ShowInventory(int16);
extern C_2F83_0167(int16, int16, int16);/*TALK_talkTo*/
extern int16 CON_mouseOffAt(int16, int16, int16, int16);
extern CON_mouseOn(int16);
extern CON_setClip(struct tClipInfo *);
extern cdecl CON_printf(char far *, ...);
extern char far C_2BB4_04A8[];
extern char far C_2BB4_11B3[];
extern char far C_2BB4_22F1[];/*failure message*/
extern int16 far C_2DE5_01C6[];/*MonsterFlags (by GetMonsterClass)*/

extern int16 MusicAdapter;/*1: AdLib or Roland, 0: none*/
extern int16 D_081C;/*ColorPaper*/
extern int16 D_082E;/*default paper color*/
extern char D_0F13;/*PromptFlag*/
extern int16 D_1431;/*VisibilityDirty*/
extern int16 D_1435;/*TerrainType 8 blocks*/
extern unsigned char D_167A;/*Active*/
extern int16 D_167C, D_167E, D_1680;/*MapX, MapY, MapZ*/
extern char D_16A4;/*InCombat*/
extern int16 D_16D4;/*DreamStage*/
extern int16 StatusDisplay;/*0x90 a member, 0x91 the party, 0x92 the equipment*/
extern unsigned char AnimateAtPrompt;
extern int16 D_1E48;/*dream.c's: the AnimTile entry of tile 0x6f (-1: not looked up)*/
extern int16 D_1E4A;/*dream.c's: how fast it animates (0-4)*/
extern struct tClipInfo *CurClip;
extern unsigned char D_3240[0x20];/*AnimFrameMask*/
extern unsigned char D_3266[];/*ObjStatus*/
extern unsigned char D_4EA4[];/*Party*/
extern int16 D_54BA[];/*Amount: quantity (low byte), quality (high byte)*/
extern unsigned char D_6EC6[];/*TalkFlags*/
extern struct coord D_78AE[];/*ObjPos*/
extern unsigned char D_9CC0;/*PartySize*/
extern uint16 D_9CC4[];/*ObjShapeType*/
extern unsigned char far *D_B4C4;/*TerrainType*/
extern int16 far *D_B508;/*OrigShapeType*/
extern unsigned char D_B50C[0x20];/*AnimFrameShift*/
extern union tHandler D_BA6A;/*GrDriver*/
extern struct tClipInfo D_EBD0[4];/*TextWindows*/
extern char DirIncrX[8], DirIncrY[8];

#define ColorPaper D_081C
#define PromptFlag D_0F13
#define VisibilityDirty D_1431
#define Active D_167A
#define MapX D_167C
#define MapY D_167E
#define MapZ D_1680
#define InCombat D_16A4
#define DreamStage D_16D4
#define ObjStatus D_3266
#define Party D_4EA4
#define Amount D_54BA
#define TalkFlags D_6EC6
#define ObjPos D_78AE
#define PartySize D_9CC0
#define ObjShapeType D_9CC4
#define TerrainType D_B4C4
#define OrigShapeType D_B508
#define GrDriver D_BA6A
#define TextWindows D_EBD0
#define MonsterFlags C_2DE5_01C6

#define COMBAT_breakOff C_07D1_02C4
#define SetTypeUnconscious C_07D1_06C1
#define ResetMapView C_0E1A_11D2
#define STAT_GetEquipSlot C_100B_02C0
#define VIS_UpdateView C_1157_0312
#define DisplayTick C_120E_0221
#define GetTileAtXYZ C_1534_1072
#define MAP_LoadAround C_1534_056F
#define MapDistance C_1534_1129
#define GetMonsterClass C_1649_000A
#define SubMov C_170D_0868
#define TryStraightMove C_170D_2231
#define NextLoc C_1B09_032A
#define FindLoc C_1B09_07D8
#define OBJ_FindTypeAt C_1B09_09C7
#define FindInv C_1B09_0B1D
#define NextInv C_1B09_0B76
#define NextArea C_1B09_0DD7
#define SearchArea C_1B09_0EDE
#define AddMapObj C_1B09_116E
#define AddInvObj C_1B09_12EE
#define DeleteObj C_1B09_1872
#define MoveObj C_1B09_1F52
#define ShowObjects C_1B09_3E18
#define GetObjectString C_1B09_4759
#define MUS_PlaySfx C_1FA9_0919
#define STAT_RefreshInventory C_2077_1179
#define TALK_talkTo C_2F83_0167

#define GR_SET_COLOR(c)              (GrDriver.iii.ofs = 0x06, (*GrDriver.f)(c))
#define GR_FILL_RECT(x0,y0,x1,y1)    (GrDriver.iii.ofs = 0x12, (*GrDriver.f)(x0,y0,x1,y1))

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])

/*AddMapObj, the new object's ObjStatus 0x20 cleared*/
/*C_2F63_0000*/C_2F63_0000(int16 type, int16 status, int16 amount, int16 x, int16 y, int16 z) {
	register int16 obj;

	if((obj = AddMapObj(type, status, amount, x, y, z)) < 0)
		CON_printf(C_2BB4_22F1);
	else
		ObjStatus[obj] &= 0xdf;
	return obj;
}

/*tile 0x6f animates faster (up to 4 steps)*/
/*C_2F63_0043*/C_2F63_0043() {
	D_1E4A ++;
	if(D_1E4A > 4)
		D_1E4A = 4;
	D_B50C[D_1E48] = 4 - D_1E4A;
	D_3240[D_1E48] = 3 << D_B50C[D_1E48];
	if(D_1E4A != 1)
		CON_printf("...faster!\n");
}

/*the dreamer passes out: in (0x21, 0x19)-(0x27, 0x1f) each object 0x151
  becomes a body (0x155, frame by the next member's original shape) with
  0x18 objects (0x34 shared out, + 0-3 each), objects 0x150 (turned
  0x14e), 0x14e, 0x38 (turned 0x10) and those of status 0x20 are moved by
  (-0x13, +0x18); the dreamer keeps only its objects 0x38, gets NPC 1's
  inventory and wakes at (0x10, 0x33, 2); DreamStage 0xa0*/
/*C_2F63_0086*/C_2F63_0086() {
	int16 type, k, obj, per, amt;
	struct tClipInfo *savedClip;
	int16 slot, mouse, ready;
	int16 objs[32];
	char equipped[32];
	register int16 i, n;

	ready = 0;
	CON_printf("Overcome by fatigue, you pass out.\n");
	if(InCombat) {
		i = MusicAdapter;
		MusicAdapter = 0;
		COMBAT_breakOff();
		MusicAdapter = i;
	}
	SetTypeUnconscious(0);
	VIS_UpdateView();
	ShowObjects();
	DisplayTick();
	TimerDelay(0x245c98L);
	ResetMapView();
	AnimateAtPrompt = 0;
	if(PartySize > 1) {
		k = 1;
		per = 0x34 / (PartySize - 1);
	} else {
		k = 2;
		per = 0xd;
	}
	for(i = 0x101; i < 0xc00; i ++) {
		if(!(!ObjShapeType[i] || (ObjStatus[i] & 0x18) || GetZ(i) != MapZ ||
			GetX(i) < 0x21 || GetX(i) > 0x27 || GetY(i) < 0x19 || GetY(i) > 0x1f || !i)) {
			switch(type = GetType(i)) {
			case 0x151:
				if((obj = AddMapObj(0x155 | (((OrigShapeType[PartySize > 1 ? Party[k] : k] & 0x3ff) - 0x156) << 10),
					0, 0, GetX(i) - 0x13, GetY(i) + 0x18, MapZ)) < 0)
					CON_printf(C_2BB4_22F1);
				else {
					ObjStatus[obj] &= 0xdf;
					for(amt = per + OSI_rand(0, 3); amt > 0xf0; amt -= 0xf0)
						AddInvObj(0x18, 8, 0xf0, obj);
					AddInvObj(0x18, 8, amt, obj);
				}
				DeleteObj(i);
				k ++;
				break;
			case 0x150:
				MoveObj(i, GetX(i) - 0x13, GetY(i) + 0x18, MapZ);
				ObjShapeType[i] = (ObjShapeType[i] & 0xfc00) + 0x14e;
				ObjStatus[i] &= 0xdf;
				break;
			case 0x38:
				ObjShapeType[i] = (ObjShapeType[i] & 0xfc00) + 0x10;
				MoveObj(i, GetX(i) - 0x13, GetY(i) + 0x18, MapZ);
				break;
			case 0x14e:
				MoveObj(i, GetX(i) - 0x13, GetY(i) + 0x18, MapZ);
				break;
			case 0x14b: case 0x1cd:
				break;
			default:
				if((ObjStatus[i] & 0x20) == 0)
					break;
				CON_printf("Moving %s to new room.\n", GetObjectString(i));
				MoveObj(i, GetX(i) - 0x13, GetY(i) + 0x18, MapZ);
			}
		}
	}
	for(i = FindInv(0); i >= 0; i = obj) {
		obj = NextInv();
		if(GetType(i) == 0x38) {
			ObjShapeType[i] = (ObjShapeType[i] & 0xfc00) + 0x10;
			ready = (ObjStatus[i] & 0x18) == 0x18;
		} else
			DeleteObj(obj);
	}
	MoveObj(0, GetX(1), GetY(1), GetZ(1));
	MapX = GetX(1);
	MapY = GetY(1);
	MapZ = GetZ(1);
	MAP_LoadAround();
	StatusDisplay = 0x91;
	n = 0;
	for(i = FindInv(1); i >= 0; i = NextInv()) {
		equipped[n] = (ObjStatus[i] & 0x18) == 0x18;
		objs[n] = i;
		n ++;
	}
	while(n) {
		i = objs[--n];
		if(equipped[n]) {
			slot = STAT_GetEquipSlot(i);
			if(!ready || slot != 2 && slot != 8)
				AddInvObj(ObjShapeType[i], 0x18, Amount[i], 0);
		} else
			AddInvObj(ObjShapeType[i], 0x10, Amount[i], 0);
	}
	StatusDisplay = 0x92;
	MoveObj(0, 0x10, 0x33, 2);
	MapX = 0x10;
	MapY = 0x33;
	MapZ = 2;
	MAP_LoadAround();
	ObjShapeType[0] = GetType(1) | 0x2c00;
	TimerDelay(0x48b930L);
	AnimateAtPrompt = 1;
	VIS_UpdateView();
	ShowObjects();
	ColorPaper = D_082E;
	GR_SET_COLOR(D_082E);
	mouse = CON_mouseOffAt(184, 128, 311, 195);
	GR_FILL_RECT(184, 128, 311, 195);
	CON_mouseOn(mouse);
	savedClip = CurClip;
	for(slot = 0; slot < 4; slot ++) {
		CON_setClip(&TextWindows[slot]);
		CurClip->bgColor = D_082E;
	}
	CON_setClip(savedClip);
	STAT_RefreshInventory(0);
	CON_printf("You wake up!\n");
	STAT_ShowInventory(PartySize);
	PromptFlag = 1;
	C_1FA9_02CE(1);
	DreamStage = 0xa0;
	CON_printf("All your friends have been killed!\n");
}

/*after creature type 0x181 or 0x16b: objects 0x1d2 removed (0x181 also
  puts an object 0x1cd at (0x70, 0x17)), TalkFlags[0x66] gets 0xa or 0x12
  and conversation 0x66 starts*/
/*C_2F63_062F*/C_2F63_062F(int16 type) {
	register int16 obj;

	if(type == 0x181) {
		if((obj = OBJ_FindTypeAt(0x1d2, 0x6d, 0x16)) < 0)
			CON_printf(C_2BB4_22F1);
		else {
			C_1B09_4963(obj);
			DeleteObj(obj);
		}
		if((obj = OBJ_FindTypeAt(0x1cd, 0x70, 0x17)) < 0) {
			if((obj = AddMapObj(0x5cd, 0, 0x7317, 0x70, 0x17, MapZ)) < 0)
				CON_printf(C_2BB4_22F1);
			else
				ObjStatus[obj] &= 0xdf;
		}
		TalkFlags[0x66] |= 0xa;
		TALK_talkTo(Active, 0x66, 0);
	} else if(type == 0x16b) {
		if((obj = OBJ_FindTypeAt(0x1d2, 0x73, 0xec)) < 0)
			CON_printf(C_2BB4_22F1);
		else {
			C_1B09_4963(obj);
			DeleteObj(obj);
		}
		TalkFlags[0x66] |= 0x12;
		TALK_talkTo(Active, 0x66, 0);
	}
}

/*how good a step of npc to (x, y) is when going to (x2, y2): 200 - 15 *
  distance (+50 next to it), -50 or -100 when it cannot step there, -5
  for each object 0x9d around (-1000 if one is open), -1 for each 0x9e*/
/*C_2F63_0726*/C_2F63_0726(int16 npc, int16 x, int16 y, int16 x2, int16 y2) {
	int16 dist, type, saved, terrain;
	register int16 score, obj;

	saved = D_1435;
	dist = C_07D1_35A8(x, y, x2, y2);
	score = 200 - dist * 15 + (dist <= 1) * 50;
	D_1435 = 1;
	if(!NPC_TryStep(npc, x, y)) {
		D_1435 = 0;
		if(NPC_TryStep(npc, x, y))
			score -= 50;
		else {
			terrain = TerrainType[GetTileAtXYZ(x, y, MapZ)];
			if(terrain & 6)
				return -1000;
			if(terrain & 8)
				score -= 100;
			else
				score -= 50;
		}
	}
	D_1435 = saved;
	for(obj = SearchArea(x - 1, y - 1, x + 1, y + 1); obj >= 0; obj = NextArea()) {
		if((type = GetType(obj)) == 0x9d) {
			if(GetFrame(obj) > 0)
				return -1000;
			score -= 5;
		} else if(type == 0x9e)
			score --;
	}
	return score;
}

/*npc goes towards an object 0xfb within 10 squares (or the party), by
  the best of its 8 steps; next to it: the object goes and npc loses 10
  points*/
/*C_2F63_083C*/C_2F63_083C(int16 npc) {
	int16 best, bestScore, score, found, tx, ty, nx, ny;
	register int16 d, obj;

	nx = GetX(npc);
	ny = GetY(npc);
	found = -1;
	for(obj = SearchArea(nx - 10, ny - 10, nx + 10, ny + 10); obj >= 0; obj = NextArea()) {
		if(GetType(obj) == 0xfb) {
			tx = GetX(obj);
			ty = GetY(obj);
			found = obj;
			break;
		}
	}
	if(found == -1) {
		tx = MapX;
		ty = MapY;
	} else if(MapDistance(nx, tx) <= 1 && MapDistance(ny, ty) <= 1) {
		MUS_PlaySfx(0x24);
		CON_printf(C_2BB4_11B3);
		MUS_PlaySfx(5);
		if((obj = OBJ_FindTypeAt(0xfb, tx, ty)) >= 0)
			DeleteObj(obj);
		SubMov(npc, 10);
		VisibilityDirty = 1;
		return;
	}
	best = -1;
	bestScore = C_2F63_0726(npc, nx, ny, tx, ty);
	for(d = 0; d < 8; d ++) {
		if((score = C_2F63_0726(npc, nx + DirIncrX[d], ny + DirIncrY[d], tx, ty)) > bestScore ||
			score == bestScore && OSI_rand(0, 1)) {
			bestScore = score;
			best = d;
		}
	}
	if(bestScore < 0)
		best = OSI_rand(0, 7);
	if(best >= 0)
		TryStraightMove(npc, best, 1);
	SubMov(npc, 5);
}

/*obj goes off: on the map an object 0x85 takes its place and the chain
  starts (C_2F63_0C0C), else the dreamer is hurt*/
/*C_2F63_0A13*/C_2F63_0A13(register int16 obj) {
	int16 newObj, x;
	register int16 y;

	CON_printf(C_2BB4_04A8);
	if((ObjStatus[obj] & 0x18) == 0) {
		x = GetX(obj);
		y = GetY(obj);
		if((newObj = AddMapObj(0x85, 0, 0, x, y, MapZ)) < 0)
			CON_printf(C_2BB4_22F1);
		else {
			VisibilityDirty = 1;
			DisplayTick();
			C_2F63_0C0C(x, y, newObj, 1);
		}
	} else {
		C_07D1_1D64(0, OSI_rand(1, 10), 3);
		DeleteObj(obj);
	}
}

/*the object 0x85 obj at (x, y) goes off: creatures there (MonsterFlags
  0x80 or 0x4000) are hurt, objects C_100B_0247 is true for are removed
  (each 0x84 makes the objects 0x84 around it into 0x85 of quality q)*/
/*C_2F63_0ACF*/C_2F63_0ACF(int16 x, int16 y, int16 obj, int16 q) {
	int16 found, type, cls, xx;
	register int16 o, yy;

	MUS_PlaySfx(9);
	for(o = FindLoc(x, y, MapZ); o >= 0; o = NextLoc()) {
		type = GetType(o);
		if(o < 0x100 && o >= 0) {
			if((cls = GetMonsterClass(type)) == 0)
				continue;
			if((MonsterFlags[cls] & 0x80) || (MonsterFlags[cls] & 0x4000))
				C_07D1_1D64(o, OSI_rand(1, 10), 3);
		} else if(C_100B_0247(type)) {
			DeleteObj(o);
			if(type == 0x84) {
				for(xx = x - 1; xx < x + 2; xx ++)
					for(yy = y - 1; yy < y + 2; yy ++)
						if((xx != x || yy != y) && (found = OBJ_FindTypeAt(0x84, xx, yy)) != -1)
							AddMapObj(0x85, 0, q << 8, xx, yy, MapZ);
			}
		}
	}
	DeleteObj(obj);
	C_120E_0009();
	TimerDelay(0x1d16eL);
}

/*the chain: obj at (x, y) goes off, then every object 0x85 of quality q,
  then the first 0x85 of quality 1 - q starts the next round*/
/*C_2F63_0C0C*/C_2F63_0C0C(int16 x, int16 y, int16 obj, int16 q) {
	register int16 i;

	VisibilityDirty = 1;
	DisplayTick();
	C_2F63_0ACF(x, y, obj, 1 - q);
	for(i = 0xbff; i > 0x100; i --)
		if(GetType(i) == 0x85 && GetQual(i) == q)
			C_2F63_0ACF(GetX(i), GetY(i), i, 1 - q);
	for(i = 0xbff; i > 0x100; i --) {
		if(GetType(i) == 0x85 && GetQual(i) == 1 - q) {
			C_2F63_0C0C(GetX(i), GetY(i), i, 1 - q);
			break;
		}
	}
	VisibilityDirty = 1;
	DisplayTick();
}
