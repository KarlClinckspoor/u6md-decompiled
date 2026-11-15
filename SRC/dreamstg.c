/* flags: -d -mm -p -c -Z -O -y */
/* stub order: 03ED 0000 0494 0CA9 165C 07E6 02A9 1393 037E 0785 0F20 0FE1 */
/*
 * Martian Dreams - GAME.EXE overlay 2F68: the dream's stages, called by
 * dream.c's DreamTrigger (overlay 2F5F) when the dreamer reaches a stage
 * (setup) or leaves it (cleanup). New code (Savage Empire has no dream).
 *
 * A stage puts its NPCs back (MoveObj to their place, frame 9 or 1,
 * NPCMode 0, D_3262 bit 7, NPCStatus bits 5-6 cleared), clears the
 * objects left in its rectangle of the map (by type) and makes them anew
 * with overlay 2F63's C_2F63_0000 (AddMapObj that clears ObjStatus 0x20),
 * AddInvObj and EGG_generate.
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

/*other game modules*/
extern COMBAT_begin();/*called when not InCombat*/
extern int16 MaxHP(int16);
extern AnimateTiles();
extern int16 EGG_generate(int16, int16, int16, int16);/*EGG_generate(type, x, y, z)*/
extern int16 OBJ_FindTypeAt(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 FindInv(int16);/*first object of an inventory*/
extern int16 NextInv(void);
extern int16 NextArea(void);/*next object of SearchArea*/
extern int16 SearchArea(int16, int16, int16, int16);/*SearchArea(x1, y1, x2, y2)*/
extern int16 AddInvObj(int16, int16, int16, int16);/*AddInvObj(shape, position, quantity, owner)*/
extern DeleteObj(int16);
extern MoveObj(int16, int16, int16, int16);/*MoveObj(obj, x, y, z)*/
extern ShowObjects();
extern OBJ_DeleteContents(int16);
extern MUS_PlayAreaSong();
extern STAT_ShowInventory(int16);
extern int16 CON_mouseOffAt(int16, int16, int16, int16);
extern CON_mouseOn(int16);
extern CON_setClip(struct tClipInfo *);
extern cdecl CON_printf(char far *, ...);
extern int16 C_2F63_0000(int16, int16, int16, int16, int16, int16);/*(shape, status, amount, x, y, z)*/
extern C_2F63_0043();
extern char far AarghMsg[];/*failure message*/

extern int16 ColorPaper;
extern char StatusDirty;
extern char PromptFlag;
extern int16 MapAnimEnabled;
extern int16 MapX, MapY, MapZ;
extern unsigned char DreamFoesLeft;
extern unsigned char PlayerIsFemale;
extern struct { unsigned a:4, b:4, c:4, d:4; } Talents[];
extern int16 D_1E48;/*dream.c's: the AnimTile entry of tile 0x6f (-1: not looked up)*/
extern int16 D_1E4A;/*dream.c's*/
extern struct tClipInfo *CurClip;
extern unsigned char AnimFrameMask[0x20];
extern unsigned char far *D_3262;/*talk flags 8-15*/
extern unsigned char ObjStatus[];
extern unsigned char far *NPCMode;
extern unsigned char far *NPCComMode;
extern int16 Amount[];/*quantity (low byte), quality (high byte)*/
extern unsigned char NPCStatus[];
extern unsigned char HitPoints[];
extern unsigned char TalkFlags[];
extern struct coord ObjPos[];
extern unsigned char PartySize;
extern int16 NumAnimData;
extern uint16 ObjShapeType[];
extern int16 AnimTile[0x20];
extern uint16 far *OrigShapeType;
extern union tHandler GrDriver;
extern struct tClipInfo TextWindows[4];
extern char DirIncrX[8], DirIncrY[8];



#define GR_SET_COLOR(c)              (GrDriver.iii.ofs = 0x06, (*GrDriver.f)(c))
#define GR_FILL_RECT(x0,y0,x1,y1)    (GrDriver.iii.ofs = 0x12, (*GrDriver.f)(x0,y0,x1,y1))

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])

/*an object of the map (not an NPC, not in an inventory) on MapZ in the
  rectangle (x1, y1)-(x2, y2)*/
#define InRect(i, x1, y1, x2, y2) (!(!ObjShapeType[i] || (ObjStatus[i] & 0x18) || GetZ(i) != MapZ || \
	GetX(i) < (x1) || GetX(i) > (x2) || GetY(i) < (y1) || GetY(i) > (y2)))

/*a random frame 0-2 of shape*/
#define RandFrame(shape) ((shape) | (OSI_rand(0, 5) >> 1 << 10))

/*1F02*/unsigned char D_1F02[12] = {/*by the quality of an object 0x78: what C_2F68_0FE1 puts there*/
	0, 0x7a, 0x4b, 3, 7, 0xb8, 0x2c, 0x65, 0x2d, 0x16, 0x81, 0
};

/*stage 0x20: NPC 0x5b back to (0xd, 0x1d) unless already there with
  shape 0x15b, talk flags 0x59-0x5d cleared, object 0x4f at (9, 0x1f) to
  frame 0, the objects 0x10, 0x38, 0x14b, 0x14e and 0x155 in (8, 0x10)-
  (0x1f, 0x37) removed, the dreamer given object 0x14b holding 0x38, then
  PartySize - 1 (or 4) creatures 0x16c around (x, y)*/
/*C_2F68_0000*/C_2F68_0000(int16 x, int16 y) {
	int16 i;
	register int16 obj;

	if(GetType(0x5b) != 0x15b || GetZ(0x5b) != 2) {
		NPCStatus[0x5b] &= 0xef;
		HitPoints[0x5b] = MaxHP(0x5b);
		NPCStatus[0x5b] &= 0xf7;
		ObjShapeType[0x5b] = OrigShapeType[0x5b];
		MoveObj(0x5b, 0xd, 0x1d, 2);
	}
	TalkFlags[0x5d] &= 0xa7;
	TalkFlags[0x5c] &= 0xe6;
	TalkFlags[0x5b] = 0;
	TalkFlags[0x5a] = 0;
	TalkFlags[0x59] = 0;
	NPCStatus[0x5b] &= 0x9f;
	NPCStatus[0x5a] &= 0x9f;
	NPCStatus[0x5d] &= 0x9f;
	NPCStatus[0x5c] &= 0x9f;
	NPCStatus[0x59] &= 0x9f;
	NPCMode[0x5b] = 0x8f;
	NPCMode[0x5a] = 0;
	NPCMode[0x5d] = 0;
	NPCMode[0x5c] = 0;
	NPCMode[0x59] = 0;
	if((i = OBJ_FindTypeAt(0x4f, 9, 0x1f)) < 0)
		CON_printf(AarghMsg);
	else
		ObjShapeType[i] = GetType(i);
	for(i = 0x101; i < 0xc00; i ++) {
		if(InRect(i, 8, 0x10, 0x1f, 0x37)) {
			switch(GetType(i)) {
			case 0x10: case 0x38: case 0x14b: case 0x14e: case 0x155:
				DeleteObj(i);
			}
		}
	}
	if((i = AddInvObj(0x14b, 0x10, 0, 0)) < 0)
		CON_printf(AarghMsg);
	else {
		ObjStatus[i] &= 0xdf;
		if((obj = AddInvObj(0x38, 8, 0, i)) < 0)
			CON_printf(AarghMsg);
		else
			ObjStatus[obj] &= 0xdf;
	}
	StatusDirty = 1;
	if((i = PartySize - 1) < 1)
		i = 4;
	DreamFoesLeft = i;
	while(i-- > 0)
		EGG_generate(0x16c, x + DirIncrX[i * 2], y + DirIncrY[i * 2], MapZ);
	COMBAT_begin();
}

/*end of stage 0xa0: the panel gray again, the dreamer's inventory
  emptied but for objects 0x18*/
/*C_2F68_02A9*/C_2F68_02A9() {
	int16 next;
	register int16 i;
	struct tClipInfo *savedClip;

	ColorPaper = 7;
	GR_SET_COLOR(7);
	i = CON_mouseOffAt(184, 128, 311, 195);
	GR_FILL_RECT(184, 128, 311, 195);
	CON_mouseOn(i);
	savedClip = CurClip;
	for(i = 0; i < 4; i ++) {
		CON_setClip(&TextWindows[i]);
		CurClip->bgColor = 7;
	}
	CON_setClip(savedClip);
	for(i = FindInv(0); i >= 0; i = next) {
		next = NextInv();
		if(GetType(i) != 0x18)
			DeleteObj(i);
	}
	STAT_ShowInventory(PartySize);
	MapAnimEnabled = 1;
	PromptFlag = 1;
	StatusDirty = 1;
	MUS_PlayAreaSong();
}

/*stage 0x25: tile 0x6f stops animating, a creature 0x187 (combat mode
  0x15) above (x, y)*/
/*C_2F68_037E*/C_2F68_037E(int16 x, int16 y) {
	register int16 i;

	if(D_1E48 < 0) {
		for(i = 0; i < NumAnimData; i ++)
			if(AnimTile[i] == 0x6f)
				D_1E48 = i;
	}
	AnimFrameMask[D_1E48] = 0;
	if((i = EGG_generate(0x187, x, y - 1, MapZ)) >= 0)
		NPCMode[i] = NPCComMode[i] = 0x15;
	else
		CON_printf(AarghMsg);
}

/*stage 0x40: the dreamer falls off the cliff (shape 0x126, 0x127 for a
  woman), the creature 0x187 above goes, tile 0x6f animates again
  (C_2F63_0043)*/
/*C_2F68_03ED*/C_2F68_03ED() {
	register int16 i;

	if(D_1E48 < 0) {
		for(i = 0; i < NumAnimData; i ++)
			if(AnimTile[i] == 0x6f)
				D_1E48 = i;
	}
	CON_printf("You fall off the cliff!\n");
	ObjShapeType[0] = PlayerIsFemale ? 0x127 : 0x126;
	AnimateTiles();
	ShowObjects();
	for(i = SearchArea(MapX - 1, MapY - 8, MapX + 1, MapY - 4); i >= 0; i = NextArea()) {
		if(GetType(i) == 0x187) {
			NPCStatus[i] |= 0x10;
			DeleteObj(i);
			break;
		}
	}
	D_1E4A = 0;
	C_2F63_0043();
}

/*stage 0x44: NPC 0x54 back to (0x37, 0x3d), the objects 0xa1, 0xa2,
  0xd9, 0xda and 0x18e in (0x21, 0x33)-(0x3d, 0x43) made anew*/
/*C_2F68_0494*/C_2F68_0494() {
	register int16 i;

	TalkFlags[0x54] &= 0xbe;
	MoveObj(0x54, 0x37, 0x3d, 2);
	ObjShapeType[0x54] = GetType(0x54) + 0x2400;
	NPCMode[0x54] = 0;
	D_3262[0x54] |= 0x80;
	NPCStatus[0x54] &= 0x9f;
	for(i = 0x101; i < 0xc00; i ++) {
		if(InRect(i, 0x21, 0x33, 0x3d, 0x43)) {
			switch(GetType(i)) {
			case 0xa1: case 0xa2: case 0xd9: case 0xda: case 0x18e:
				DeleteObj(i);
			}
		}
	}
	C_2F63_0000(0xa1, 0, 0, 0x24, 0x36, MapZ);
	for(i = 0x39; i <= 0x41; i ++) {
		if(i != 0x3e)
			C_2F63_0000(RandFrame(0xd9), 0, 3, 0x37, i, MapZ);
		if(i > 0x3a && i != 0x40)
			C_2F63_0000(RandFrame(0xd9), 0, 3, 0x39, i, MapZ);
		C_2F63_0000(RandFrame(0xd9), 0, 3, 0x3b, i, MapZ);
	}
	C_2F63_0000(RandFrame(0xd9), 0, 3, 0x38, 0x39, MapZ);
	C_2F63_0000(RandFrame(0xd9), 0, 3, 0x38, 0x41, MapZ);
	C_2F63_0000(RandFrame(0xd9), 0, 3, 0x3a, 0x39, MapZ);
	C_2F63_0000(RandFrame(0xd9), 0, 3, 0x3a, 0x41, MapZ);
	if((i = EGG_generate(0x18e, 0x3b, 0x41, MapZ)) >= 0) {
		NPCMode[i] = NPCComMode[i] = 0x16;
		ObjStatus[i] &= 0xdf;
	} else
		CON_printf(AarghMsg);
}

/*end of stage 0x44: TalkFlags[0x54] gets 8, or 0x10 when it has 8
  (not with 0x80), the creature 0x18e goes*/
/*C_2F68_0785*/C_2F68_0785() {
	register int16 i;

	if(!(TalkFlags[0x54] & 0x80)) {
		if(TalkFlags[0x54] & 8)
			TalkFlags[0x54] |= 0x10;
		else
			TalkFlags[0x54] |= 8;
	}
	for(i = SearchArea(0x21, 0x33, 0x3d, 0x43); i >= 0; i = NextArea()) {
		if(GetType(i) == 0x18e) {
			DeleteObj(i);
			break;
		}
	}
}

/*stage 0xa1: NPC 0x52 back to (0x5f, 0x10); in (0x58, 8)-(0x7f, 0x2e)
  the objects 0x59 (holding 0x2e) and 0x68 (holding 0x3c) and the
  objects 0x77, 0x49 and three 0x14b at their places are made anew*/
/*C_2F68_07E6*/C_2F68_07E6() {
	int16 next, inv, next2, has59, has68;
	register int16 obj, keep;

	has59 = 0;
	has68 = 0;
	TalkFlags[0x52] = 0;
	MoveObj(0x52, 0x5f, 0x10, 2);
	ObjShapeType[0x52] = GetType(0x52) + 0x2400;
	NPCMode[0x52] = 0;
	D_3262[0x52] |= 0x80;
	NPCStatus[0x52] &= 0x9f;
	for(obj = SearchArea(0x58, 8, 0x7f, 0x2e); obj >= 0; obj = next) {
		next = NextArea();
		switch(GetType(obj)) {
		case 0x59:
			keep = -1;
			has59 = 1;
			for(inv = FindInv(obj); inv >= 0; inv = next2) {
				next2 = NextInv();
				if(GetType(inv) != 0x2e)
					DeleteObj(inv);
				else
					keep = inv;
			}
			if(keep < 0) {
				if((keep = AddInvObj(0x2e, 8, 0, obj)) >= 0)
					ObjStatus[keep] &= 0xdf;
				else
					CON_printf(AarghMsg);
			}
			break;
		case 0x68:
			keep = -1;
			has68 = 1;
			ObjShapeType[obj] = GetType(obj) + 0x400;
			for(inv = FindInv(obj); inv >= 0; inv = next2) {
				next2 = NextInv();
				if(GetType(inv) != 0x3c)
					DeleteObj(inv);
				else
					keep = inv;
			}
			if(keep < 0) {
				if((keep = AddInvObj(0x3c, 8, 0x10, obj)) >= 0)
					ObjStatus[keep] &= 0xdf;
				else
					CON_printf(AarghMsg);
			}
			break;
		case 0x77:
			if(GetX(obj) != 0x7e || GetY(obj) != 0x21)
				DeleteObj(obj);
			break;
		case 0x49:
			if(GetX(obj) != 0x75 || GetY(obj) != 0x21)
				DeleteObj(obj);
			break;
		case 0x14b:
			inv = FindInv(obj);
			switch(GetType(inv)) {
			case 0x2a: case 0x39: case 0x81:
				DeleteObj(inv);
				DeleteObj(obj);
				break;
			}
			break;
		case 0x2a: case 0x2e: case 0x39: case 0x3c: case 0x81:
			DeleteObj(obj);
			break;
		}
	}
	if(!has59) {
		if((obj = C_2F63_0000(0x59, 0, 0, 0x58, 0x20, MapZ)) >= 0) {
			if((keep = AddInvObj(0x2e, 8, 0, obj)) >= 0)
				ObjStatus[keep] &= 0xdf;
			else
				CON_printf(AarghMsg);
		}
	}
	if(!has68) {
		if((obj = C_2F63_0000(0x68, 0, 0, 0x58, 0x20, MapZ)) >= 0) {
			if((keep = AddInvObj(0x3c, 8, 0x10, obj)) >= 0)
				ObjStatus[keep] &= 0xdf;
			else
				CON_printf(AarghMsg);
		}
	}
	if(OBJ_FindTypeAt(0x77, 0x7e, 0x21) < 0)
		C_2F63_0000(0x77, 0, 0, 0x7e, 0x21, MapZ);
	if(OBJ_FindTypeAt(0x49, 0x75, 0x21) < 0)
		C_2F63_0000(0x49, 0, 0, 0x75, 0x21, MapZ);
	if(OBJ_FindTypeAt(0x14b, 0x73, 0x21) < 0) {
		if((keep = C_2F63_0000(0x14b, 0, 0, 0x73, 0x21, MapZ)) >= 0) {
			if((obj = AddInvObj(0x81, 8, 0, keep)) < 0)
				CON_printf(AarghMsg);
			else
				ObjStatus[obj] &= 0xdf;
		}
	}
	if(OBJ_FindTypeAt(0x14b, 0x75, 0x29) < 0) {
		if((keep = C_2F63_0000(0x14b, 0, 0, 0x75, 0x29, MapZ)) >= 0) {
			if((obj = AddInvObj(0x2a, 8, 0, keep)) < 0)
				CON_printf(AarghMsg);
			else
				ObjStatus[obj] &= 0xdf;
		}
	}
	if(OBJ_FindTypeAt(0x14b, 0x76, 0x2a) < 0) {
		if((keep = C_2F63_0000(0x14b, 0, 0, 0x76, 0x2a, MapZ)) >= 0) {
			if((obj = AddInvObj(0x39, 8, 0x18, keep)) < 0)
				CON_printf(AarghMsg);
			else
				ObjStatus[obj] &= 0xdf;
		}
	}
}

/*stage 5: NPC 0x50 back to (0xa5, 0xa4), in (0x76, 0x98)-(0xce, 0xcc)
  the objects 0xf, 0x28, 0x93, 0x96, 0x97, 0x14e-0x150 made anew, talent
  a 2, creatures 0x193 and 0x17b (frame 15), combat mode 8*/
/*C_2F68_0CA9*/C_2F68_0CA9() {
	register int16 obj, i;

	TalkFlags[0x50] &= 0xde;
	MoveObj(0x50, 0xa5, 0xa4, 2);
	ObjShapeType[0x50] = GetType(0x50) + 0x2400;
	NPCMode[0x50] = 0;
	D_3262[0x50] |= 0x80;
	NPCStatus[0x50] &= 0x9f;
	if((obj = OBJ_FindTypeAt(0x92, 0xc9, 0x9b)) >= 0)
		DeleteObj(obj);
	for(i = 0x101; i < 0xc00; i ++) {
		if(InRect(i, 0x76, 0x98, 0xce, 0xcc)) {
			switch(GetType(i)) {
			case 0xf: case 0x28: case 0x93: case 0x96: case 0x97: case 0x14e: case 0x14f: case 0x150:
				DeleteObj(i);
			}
		}
	}
	C_2F63_0000(0x93, 0, 0, 0xc1, 0x9c, 2);
	C_2F63_0000(0x28, 0, 0, 0xc1, 0x9b, 2);
	C_2F63_0000(0xf, 0, 0, 0xc7, 0x9a, 2);
	C_2F63_0000(0x96, 0, 0x100, 0xc1, 0xa1, 2);
	C_2F63_0000(0x97, 0, 0x800, 0xc1, 0xa1, 2);
	Talents[0].a = 2;
	if((obj = EGG_generate(0x193, 0xb0, 0xa2, MapZ)) < 0)
		CON_printf(AarghMsg);
	else {
		ObjStatus[obj] &= 0xdf;
		NPCMode[obj] = NPCComMode[obj] = 8;
	}
	if((obj = EGG_generate(0x3d7b, 0xa0, 0xa3, MapZ)) < 0)
		CON_printf(AarghMsg);
	else {
		ObjStatus[obj] &= 0xdf;
		NPCMode[obj] = NPCComMode[obj] = 8;
	}
}

/*end of stage 5: the creatures 0x17b and 0x193 in (0x76, 0x98)-(0xce, 0xcc)
  go*/
/*C_2F68_0F20*/C_2F68_0F20() {
	register int16 i;

	for(i = 2; i < 0x100; i ++) {
		if(InRect(i, 0x76, 0x98, 0xce, 0xcc)) {
			switch(GetType(i)) {
			case 0x17b: case 0x193:
				DeleteObj(i);
			}
		}
	}
}

/*stage 4: a barge (0x1a1 pieces, controller 0x1a2) around (x, y - 1),
  NPC 0x57 on it; in the 0x60 x 0x40 squares from (x - 3, y - 8) the
  objects 0x78 become what D_1F02 gives for their quality*/
/*C_2F68_0FE1*/C_2F68_0FE1(int16 x, int16 y) {
	int16 x0, y0, obj, inv, t;
	register int16 i;

	x0 = x - 3;
	y0 = y - 8;
	for(i = 0x101; i < 0xc00; i ++) {
		if(InRect(i, x0, y0, x0 + 0x5f, y0 + 0x3f)) {
			switch(GetType(i)) {
			case 0x78:
				ObjStatus[i] |= 2;
				break;
			case 0x14b:
				OBJ_DeleteContents(i);
			case 1: case 3: case 7: case 0x2c: case 0x4b: case 0x58: case 0x65: case 0x7a:
			case 0xb8: case 0x100: case 0x116: case 0x12d: case 0x181: case 0x1a1: case 0x1a2:
				DeleteObj(i);
			}
		}
	}
	C_2F63_0000(0x1a1, 0, 0x102, x, y - 2, MapZ);
	C_2F63_0000(0x5a1, 0, 0x100, x + 1, y - 2, MapZ);
	C_2F63_0000(0x9a1, 0, 0x100, x + 2, y - 2, MapZ);
	C_2F63_0000(0xda1, 0, 0x100, x, y - 1, MapZ);
	C_2F63_0000(0x15a1, 0, 0x100, x + 2, y - 1, MapZ);
	C_2F63_0000(0x19a1, 0, 0x101, x, y, MapZ);
	C_2F63_0000(0x1da1, 0, 0x100, x + 1, y, MapZ);
	C_2F63_0000(0x21a1, 0, 0x100, x + 2, y, MapZ);
	C_2F63_0000(0x11a2, 0, 0x109, x + 1, y - 1, MapZ);
	TalkFlags[0x57] = 0;
	MoveObj(0x57, x, y - 2, MapZ);
	ObjShapeType[0x57] = GetType(0x57) + 0x2400;
	NPCMode[0x57] = 0;
	D_3262[0x57] |= 0x80;
	NPCStatus[0x57] &= 0x9f;
	for(i = SearchArea(x0, y0, x0 + 0x5f, y0 + 0x3f); i >= 0; i = NextArea()) {
		if(GetType(i) == 0x78) {
			if((t = D_1F02[GetQual(i)]) == 0x7a)
				C_2F63_0000(t, 0, 1, GetX(i), GetY(i), MapZ);
			else if(t) {
				if((obj = C_2F63_0000(0x14b, 0, 0, GetX(i), GetY(i), MapZ)) >= 0) {
					if((inv = AddInvObj(t, 8, t == 0x4b ? 4 : 0, obj)) < 0)
						CON_printf(AarghMsg);
					else
						ObjStatus[inv] &= 0xdf;
				}
			}
		}
	}
}

/*stage 0x60: NPC 0x55 back to (0x4e, 0x4d), in (0x49, 0x4a)-(0x57, 0x51)
  the objects 0x6f, 0x70, 0x84, 0x1a9 and 0x1aa made anew: four 0x70 at
  the corners, 0x1a9 and the objects 0x84 (quantity by place)*/
/*C_2F68_1393*/C_2F68_1393() {
	int16 k;
	register int16 i, j;

	TalkFlags[0x55] = 0;
	MoveObj(0x55, 0x4e, 0x4d, 2);
	ObjShapeType[0x55] = GetType(0x55) + 0x400;
	NPCMode[0x55] = 0;
	D_3262[0x55] |= 0x80;
	NPCStatus[0x55] &= 0x9f;
	for(i = 0x101; i < 0xc00; i ++) {
		if(InRect(i, 0x49, 0x4a, 0x57, 0x51)) {
			switch(GetType(i)) {
			case 0x6f: case 0x70: case 0x84: case 0x1a9: case 0x1aa:
				DeleteObj(i);
			}
		}
	}
	C_2F63_0000(0x70, 0, 0xff00, 0x49, 0x4a, 2);
	C_2F63_0000(0x70, 0, 0xff00, 0x51, 0x4a, 2);
	C_2F63_0000(0x70, 0, 0xff00, 0x49, 0x51, 2);
	C_2F63_0000(0x70, 0, 0xff00, 0x51, 0x51, 2);
	C_2F63_0000(0x1a9, 0, 0, 0x4d, 0x50, 2);
	C_2F63_0000(0x84, 0, 9, 0x4d, 0x50, 2);
	for(j = 0; j <= 8; j += 2) {
		if(j != 4) {
			for(k = 2; k <= 6; k += 2)
				C_2F63_0000(0x84, 0, j + k + 2, j + 0x49, k + 0x4a, 2);
		}
	}
	for(j = 1; j <= 7; j += 2) {
		for(k = 5; k <= 7; k += 2)
			C_2F63_0000(0x84, 0, 0xa, j + 0x49, k + 0x4a, 2);
	}
	C_2F63_0000(0x84, 0, 0xb, 0x4a, 0x4b, 2);
	C_2F63_0000(0x84, 0, 0xb, 0x4a, 0x4d, 2);
	C_2F63_0000(0x84, 0, 0xb, 0x50, 0x4b, 2);
	C_2F63_0000(0x84, 0, 0xb, 0x50, 0x4d, 2);
}

/*stage 0x85: entering (first), NPCs 0x51 and 0xa-0xf back to their
  places and an object 0x14b holding 0xeb (frame 4) at (0x34, 0x50); else
  the object 0x14b or 0xeb near the dreamer is moved to the 8x8 block of
  (x, y)*/
/*C_2F68_165C*/C_2F68_165C(int16 first, int16 x, int16 y) {
	int16 obj, x0, y0, east;
	register int16 i;

	if(first) {
		OBJ_DeleteContents(0);
		TalkFlags[0x51] = 0;
		MoveObj(0x51, 0x39, 0x65, 2);
		MoveObj(0xa, 0x3d, 0x66, 2);
		MoveObj(0xb, 0x34, 0x66, 2);
		MoveObj(0xc, 0x3c, 0x5e, 2);
		MoveObj(0xd, 0x34, 0x5e, 2);
		MoveObj(0xe, 0x3c, 0x56, 2);
		MoveObj(0xf, 0x34, 0x56, 2);
		ObjShapeType[0x51] = GetType(0x51) + 0x2400;
		NPCMode[0x51] = 0;
		D_3262[0x51] |= 0x80;
		NPCStatus[0x51] &= 0x9f;
		ObjShapeType[0xa] = GetType(0xa) + 0x2400;
		NPCMode[0xa] = 0;
		D_3262[0xa] |= 0x80;
		NPCStatus[0xa] &= 0x9f;
		ObjShapeType[0xb] = GetType(0xb) + 0x2400;
		NPCMode[0xb] = 0;
		D_3262[0xb] |= 0x80;
		NPCStatus[0xb] &= 0x9f;
		ObjShapeType[0xc] = GetType(0xc) + 0x2400;
		NPCMode[0xc] = 0;
		D_3262[0xc] |= 0x80;
		NPCStatus[0xc] &= 0x9f;
		ObjShapeType[0xd] = GetType(0xd) + 0x2400;
		NPCMode[0xd] = 0;
		D_3262[0xd] |= 0x80;
		NPCStatus[0xd] &= 0x9f;
		ObjShapeType[0xe] = GetType(0xe) + 0x2400;
		NPCMode[0xe] = 0;
		D_3262[0xe] |= 0x80;
		NPCStatus[0xe] &= 0x9f;
		ObjShapeType[0xf] = GetType(0xf) + 0x2400;
		NPCMode[0xf] = 0;
		D_3262[0xf] |= 0x80;
		NPCStatus[0xf] &= 0x9f;
		for(i = 0x101; i < 0xc00; i ++) {
			if(InRect(i, 0x30, 0x50, 0x3f, 0x67) && (GetType(i) == 0x14b || GetType(i) == 0xeb)) {
				OBJ_DeleteContents(i);
				DeleteObj(i);
			}
		}
		if((obj = C_2F63_0000(0x14b, 0, 0, 0x34, 0x50, 2)) >= 0) {
			if((i = AddInvObj(0xeb, 8, 0, obj)) < 0)
				CON_printf(AarghMsg);
			else {
				ObjShapeType[i] = GetType(i) + 0x1000;
				ObjStatus[i] &= 0xdf;
			}
		}
	} else {
		x0 = GetX(0) & 0xf8;
		y0 = GetY(0) & 0xf8;
		east = x > 0x3a && y >= 0x60;
		x &= 0xf8;
		y &= 0xf8;
		for(i = SearchArea(x, y, x + 7, y + 7); i >= 0; i = NextArea()) {
			if(GetType(i) == 0x14b || GetType(i) == 0xeb) {
				DeleteObj(i);
				break;
			}
		}
		for(i = SearchArea(x0, y0, x0 + 7, y0 + 7); i >= 0; i = NextArea()) {
			if(GetType(i) == 0x14b || GetType(i) == 0xeb) {
				if(east)
					MoveObj(i, x + 5, y, MapZ);
				else if(x == 0x38 && y == 0x60)
					MoveObj(i, x + 1, y, MapZ);
				else
					MoveObj(i, x + 4, y, MapZ);
				break;
			}
		}
	}
}
