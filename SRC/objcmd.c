/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE
 * segment 047A: the object commands: Savage Empire's objcmd.c (Ultima VI's)
 * from GetPartyObjOrVehicle to CmdMove, reworked.
 *
 * "look" (CmdLook: what many objects show, the plaques with their
 * coordinates, portraits, searching a container), "drop" (CmdDrop: thrown
 * to a place within reach, into the water, into a container or on the
 * ground; the breakable types break), "get" (CmdGet: from within the
 * actor's reach, a body is searched), "move" (CmdMove: pushing on the map,
 * giving to party members, into/out of containers), with their helpers
 * (CanPutInContainer, ShowPortrait, SearchObj, HasSupportAt, PushObj, ...).
 *
 * From SE unchanged but for addresses: GetOuterHolder, ResistStrVsStr,
 * ParseNumberInput, PrintTileArticle, PrintObjCountOrArticle,
 * FindNonIgnoredObjAt, AskHowMany. MD's changes: no vehicles
 * (GetPartyObjOrVehicle, GetObjX/YOrVehicle are plain); the map wraps
 * (sums & 0x3ff, MapDistance); SE's cut scenes, type lists and tests
 * (IsContainerType ... IsUsableType, now objtype.c's) and everything after
 * CmdMove (use) are elsewhere; the commands reach as far as the actor's
 * first talent allows (REACH, C_07D1_35A8, the thrown object's path
 * C_07D1_0CFC), play a sound when refused (MUS_PlaySfx(5)) and print most
 * messages from the far message segment 2BB4; the hot objects 0x1c0/0x1c1
 * burn the hands; objects dropped or pushed into a running canal splash.
 */
#include "se.h"

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;
};

/*OSI*/
extern int16 OSI_rand(int16, int16);/*OSI_rand(lo, hi)*/
extern int16 cdecl MOUSE_SHOW(void);/*MOUSE_SHOW*/
extern int16 cdecl MOUSE_HIDE(void);/*MOUSE_HIDE*/
/*gameloop*/
extern DisplayTick(void);
extern PassTime(int16);
extern RefreshStatus(void);
/*console*/
extern int16 MkDirection(int16, int16);
extern CON_UpdateMousePointer(void);
extern int16 mkMouseDir(void);
extern CON_ReadCommand(void);/*reads a command (U6: CON_ReadCommand)*/
extern DrawSlotCursor(int16);
extern int16 CON_getch(void);
extern CON_gets(char *, int16);
extern CON_putch(int16);
extern CON_setClip(struct tClipInfo *);
extern CON_gotoxy(int16, int16);
extern cdecl CON_printf(char far *, ...);
/*map*/
extern unsigned char GetTileAtXYZ(int16, int16, int16);
extern int16 MapDistance(int16, int16);/*C_1534_1129 (wrapping)*/
/*visible*/
extern VIS_UpdateView(void);
/*object*/
extern int16 FindLoc(int16, int16, int16);
extern int16 NextLoc(void);
extern int16 FindInv(int16);
extern int16 OBJ_FindTypeAt(int16, int16, int16);
extern int16 OBJ_FindNPCAt(int16, int16, int16);
extern int16 C_1B09_0D34(int16, int16);/*inventory slots used by objNum with a type added*/
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);
extern int16 AddInvObj(int16, int16, int16, int16);
extern DeleteObj(int16);
extern MoveObj(int16, int16, int16, int16);
extern int16 InsertObj(int16, int16, int16);
extern char *GetObjectString(int16);
extern char *GetTileString(int16);
extern ShowObjects(void);
extern int16 LocCursor[4];/*LocCursor: FindLoc's cursors*/
extern int16 LocPartOffset;/*LocPartOffset*/
/*objtype*/
extern int16 IsContainerType(int16);
extern int16 C_100B_01AD(int16);/*breakable type*/
extern int16 C_100B_01D4(int16);/*a lamp type*/
extern int16 C_100B_01FB(int16);/*a light source type*/
extern int16 C_100B_0222(int16);/*pushable type*/
extern int16 C_100B_038E(int16);
extern int16 IsWeightPer10Type(int16);
extern int16 QuanType(int16);
extern int16 STAT_GetEquipSlot(int16);
/*typetest*/
extern int16 InRange_10_17_C0_C3(int16);
extern int16 Is_6C_6D_6E_4D_4F(int16);
/*status*/
extern STAT_DrawInventory(void);
extern int16 GetWeight(int16);
extern int16 Encumbrance(int16);
extern STAT_ComputeWeights(int16);
extern STAT_RefreshInventory(int16);
extern int16 STAT_FindEquipSlot(int16);
extern int16 STAT_CheckUnready(int16, int16);
extern STAT_StopEquipEffect(int16, int16);
/*talkdr*/
extern char *TALK_GetCharName(int16);
extern int16 IsNpcOnScreen(int16);
/*talk*/
extern TALK_talkTo(int16, int16, int16);
/*canmove*/
extern int16 CanMoveTo(int16, int16, int16);
/*npctrack*/
extern C_170D_083F(int16, int16);/*turns an NPC towards dir*/
extern SubMov(int16, int16);
/*combat*/
extern int16 C_07D1_0521(int16);/*GetStr*/
extern int16 C_07D1_0CFC(int16, int16 *, int16 *, int16, int16);/*the flight of a thrown object (actor, &x, &y, range, tile): 0 when blocked, x/y where it stops*/
extern C_07D1_126D(int16);/*COMBAT_showHit*/
extern C_07D1_1D64(int16, int16, int16);/*hurt (objNum, damage, kind)*/
extern int16 C_07D1_35A8(int16, int16, int16, int16);/*distance of (x1, y1) and (x2, y2)*/
/*sound*/
extern MUS_PlaySfx(int16);
extern C_1FA9_0A8A(void);/*splash*/
extern C_1FA9_0BE4(void);/*splash into a running canal*/
/*gfx*/
extern DrawPortrait(int16, int16, int16);
extern C_0E1A_158A(void);/*clears the status panel for a portrait*/
extern C_0E1A_15D1(unsigned char);/*shows picture n in the status panel*/

/*far messages (segment 2BB4)*/
extern char far C_2BB4_01C6[];/*"Where? "*/
extern char far C_2BB4_0210[];/*"It won't go in that direction!\n"*/
extern char far C_2BB4_0230[];/*"Blocked!\n"*/
extern char far C_2BB4_024E[];/*"You are carrying too much already.\n"*/
extern char far C_2BB4_0272[];/*"Failed.\n"*/
extern char far C_2BB4_034D[];/*"That is not possible.\n"*/
extern char far C_2BB4_03A6[];/*"Out of range!\n"*/
extern char far C_2BB4_03D7[];/*"\nThe total is too heavy.\n"*/
extern char far C_2BB4_03F8[];/*"nowhere.\n"*/
extern char far C_2BB4_0402[];/*"nothing!\n"*/
extern char far C_2BB4_040C[];/*"Nobody.\n"*/
extern char far C_2BB4_0449[];/*"\nYour hands are full.\n"*/
extern char far C_2BB4_0493[];/*"\nIt's already full.\n"*/
extern char far C_2BB4_053C[];/*"The cannon will fire steel cannon balls.\n"*/
extern char far C_2BB4_09CE[];/*"The panel is loose.\n"*/
extern char far C_2BB4_09E3[];/*"The panel is broken.\n"*/
extern char far C_2BB4_0C18[];/*"It is fastened securely.\n"*/
extern char far C_2BB4_0D50[];/*"OUCH. It is very hot.\n"*/
extern char far C_2BB4_0F45[];/*"There is no rubber in the sprayer system.\n"*/
extern char far C_2BB4_0F70[];/*"The rubber in the sprayer system is not flesh-colored.\n"*/
extern char far C_2BB4_0FA8[];/*"It is also not flesh-colored.\n"*/
extern char far C_2BB4_1223[];/*"It is apparently rusted shut.\n"*/
extern char far C_2BB4_1242[];/*"It is apparently locked.\n"*/
extern char far C_2BB4_13D8[];/*"It appears to control a dream machine.\n"*/
extern char far C_2BB4_1400[];/*"It appears to control a spray system.\n"*/
extern char far C_2BB4_1427[];/*"It appears to control the ruby lens system.\n"*/
extern char far C_2BB4_1454[];/*"It appears to control the cistern valves.\n"*/
extern char far C_2BB4_147F[];/*"These appear to be mechanical controls.\n"*/
extern char far C_2BB4_14A8[];/*"These appear to be valve controls.\n"*/
extern char far C_2BB4_14CC[];/*"These appear to be electrical controls.\n"*/
extern char far C_2BB4_14F5[];/*"You cannot decipher its purpose.\n"*/
extern char far C_2BB4_1517[];/*"It appears to control this dream machine.\n"*/
extern char far C_2BB4_1542[];/*"It appears to control this spray system.\n"*/
extern char far C_2BB4_156C[];/*"It appears to control the ruby lens system.\n"*/
extern char far C_2BB4_1599[];/*"It appears to control the cistern valves.\n"*/
extern char far C_2BB4_15C4[];/*"It appears to control a mechanical system.\n"*/
extern char far C_2BB4_15F0[];/*"It appears to control a valve system.\n"*/
extern char far C_2BB4_1617[];/*"It appears to control an electrical system.\n"*/
extern char far C_2BB4_1644[];/*"You cannot decipher what it controls.\n"*/
extern char far C_2BB4_1A50[];/*"Location: "*/
extern char far C_2BB4_1C68[];/*"              \n"*/
extern char far C_2BB4_1C78[];/*"\nSearching here, you find nothing.\n"*/
extern char far C_2BB4_1C9C[];/*"\nSearching here, you find "*/
extern char far C_2BB4_1CB7[];/*"You see "*/
extern char far C_2BB4_1CC0[];/*"darkness.\n"*/
extern char far C_2BB4_1CCB[];/*"Yourself\n"*/
extern char far C_2BB4_1CD5[];/*"It is growing.\n"*/
extern char far C_2BB4_1CE5[];/*"It is ripe.\n"*/
extern char far C_2BB4_1CF2[];/*"It is not growing.\n"*/
extern char far C_2BB4_1D06[];/*"It has a heartstone.\n"*/
extern char far C_2BB4_1D1C[];/*"It is empty.\n"*/
extern char far C_2BB4_1D2A[];/*"It has %d load\\s of "*/
extern char far C_2BB4_1D40[];/*"\n\nThe time is "*/
extern char far C_2BB4_1D4F[];/*"It is set to '%s'"*/
extern char far C_2BB4_1D61[];/*", and has %d charge\\s left.\n"*/
extern char far C_2BB4_1D7F[];/*"It appears to be loose.\n"*/
extern char far C_2BB4_1D98[];/*"It appears to be broken.\n"*/
extern char far C_2BB4_1DB2[];/*"It's loaded with %s"*/
extern char far C_2BB4_1DC6[];/*"It is ready to use.\n"*/
extern char far C_2BB4_1DDB[];/*"The %s cannot be dropped on the ground.\n"*/
extern char far C_2BB4_1E04[];/*"\nSearching the body, you find "*/
extern char far C_2BB4_1E23[];/*"Won't budge !\n"*/
extern char far C_2BB4_1E32[];/*"The ice is melting.\n"*/
extern char far C_2BB4_1E47[];/*"The %s cannot be moved onto the ground.\n"*/
extern char far C_2BB4_1E70[];/*"\nThat person isn't here!\n"*/
extern char far C_2BB4_1E8A[];/*"%s is carrying too much already.\n"*/
extern char far C_2BB4_1EAC[];/*"\n\nYou have to unready it first.\n"*/
extern char far C_2BB4_1ECD[];/*"\n\nAn item can't be placed inside itself!\n"*/
extern char far C_2BB4_1EF7[];/*"\n\nYou can't do that!\n"*/
extern char far C_2BB4_1F0D[];/*"That container is full.\n"*/
extern char far C_2BB4_22F1[];/*"Aargh!\n"*/

extern char DirIncrX[];/*DirIncrX*/
extern char DirIncrY[];/*DirIncrY*/
extern char *DirNames[];/*DirNames*/
extern char MouseOn;/*MouseOn*/
extern int16 EndPagingAtPrompt;/*EndPagingAtPrompt*/
extern int16 StatusDisplay;/*StatusDisplay*/
extern int16 GetsAborted;/*GetsAborted*/
extern struct tClipInfo *CurClip;/*CurClip*/
extern char SelectMode;/*SelectMode*/
extern char SelectRange;/*SelectRange*/
extern char AimX, AimY;/*AimX, AimY*/
extern int16 LastNumberPlural;/*LastNumberPlural*/
extern char DirOriginX, DirOriginY;/*DirOriginX, DirOriginY*/
extern int16 AdvanceBlocked;/*AdvanceBlocked*/
extern unsigned char D_0F12;/*StatusDirty*/
extern int16 StatusMember;/*StatusMember*/
extern int16 MouseMode;/*MouseMode*/
extern unsigned char IsInConversation;/*IsInConversation*/
extern int16 ColorPaper;/*ColorPaper*/
extern unsigned char Time_M, Time_H;/*Time_M, Time_H*/
extern unsigned char Active;/*Active*/
extern int16 MapX, MapY, MapZ;/*MapX, MapY, MapZ*/
extern unsigned char D_16A8;/*the player is a woman*/
extern int32 D_16AA;/*flags (0x10: the dream)*/
extern struct { unsigned a:4, b:4, c:4, d:4; } D_16B0[];/*talents*/
extern unsigned char D_3266[];/*ObjStatus*/
extern unsigned char D_4EA4[];/*Party*/
extern unsigned char far *D_4EB6;/*Level*/
extern int16 D_54BA[];/*Amount*/
extern unsigned char D_6CBA[];/*NPCStatus*/
extern int16 far *D_6EBE;/*BaseTile*/
extern unsigned char D_6EC6[];/*TalkFlags*/
extern unsigned char far *D_7606;/*TypeWeight*/
extern unsigned char far *D_760A;/*TileFlag2*/
extern struct coord D_78AE[];/*ObjPos*/
extern unsigned char D_9CC0;/*PartySize*/
extern uint16 D_9CC4[];/*ObjShapeType*/
extern unsigned char far *D_B4C4;/*TerrainType*/
extern union tHandler D_BA6A;/*GrDriver*/
extern unsigned char D_EAF8;/*InvenSize*/
extern unsigned char D_EAF9[8];/*Party_2Handed*/
extern int16 D_EB04[8];/*Equipment*/
extern int16 D_EB16;/*WeightEquip*/
extern int16 D_EB18;/*InvenOwner*/
extern int16 D_EB1A;/*WeightInven*/
extern int16 D_EB72;/*AdvanceDir*/
extern int16 D_EB74, D_EB76;/*PointerX, PointerY*/
extern int16 D_EBCE;/*Selection.obj*/
extern struct tClipInfo D_EBD0[4];/*TextWindows*/
extern int16 D_EBF6, D_EBF8;/*Selection.x, Selection.y*/
extern int16 D_EC44;/*SelectionIsDark*/

#define GetStr C_07D1_0521
#define StatusDirty D_0F12
#define ObjStatus D_3266
#define Party D_4EA4
#define Level D_4EB6
#define Amount D_54BA
#define NPCStatus D_6CBA
#define BaseTile D_6EBE
#define TalkFlags D_6EC6
#define TypeWeight D_7606
#define TileFlag2 D_760A
#define ObjPos D_78AE
#define PartySize D_9CC0
#define ObjShapeType D_9CC4
#define TerrainType D_B4C4
#define GrDriver D_BA6A
#define InvenSize D_EAF8
#define Party_2Handed D_EAF9
#define Equipment D_EB04
#define WeightEquip D_EB16
#define InvenOwner D_EB18
#define WeightInven D_EB1A
#define AdvanceDir D_EB72
#define PointerX D_EB74
#define PointerY D_EB76
#define SelectionObj D_EBCE
#define TextWindows D_EBD0
#define SelectionX D_EBF6
#define SelectionY D_EBF8
#define SelectionIsDark D_EC44

#define GR_SET_COLOR(c)                    (GrDriver.iii.ofs = 0x06, (*GrDriver.f)(c))
#define GR_FILL_RECT(x0,y0,x1,y1)          (GrDriver.iii.ofs = 0x12, (*GrDriver.f)(x0,y0,x1,y1))

#define LOCXYZ 0
#define CONTAINED 8
#define INVEN 0x10
#define EQUIP 0x18

#define GetQuan(o) (((unsigned char *)(Amount + (o)))[0])
#define GetQual(o) (((unsigned char *)(Amount + (o)))[1])
#define GetCoordUse(i) (ObjStatus[i] & 0x18)
#define TypeFrame(t, s) ((t) + ((s) << 10))
#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define SetFrame(i, v) (ObjShapeType[i] = TypeFrame(GetType(i),(v)))
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))
#define GetCoordX(o) ((*(int16 *)&((o).raw[0]))&0x3ff)
#define GetCoordY(o) (((*(int16 *)&((o).raw[1]))>>2)&0x3ff)
#define GetCoordAssoc(i) *(uint16 *)(&(i))
#define GetX(i) GetCoordX(ObjPos[i])
#define GetY(i) GetCoordY(ObjPos[i])
#define GetAssoc(i) GetCoordAssoc(ObjPos[i])
#define SetQuan(o, v) (GetQuan(o) = (v))
#define SetQual(o, v) (GetQual(o) = (v))
#define SubQuan(o, v) (GetQuan(o) -= (v))
#define QualQuan(quality, quantity) ((quality) << 8 | (quantity))
#define IsTerrainImpass(tile) (TerrainType[tile] & 0x02)
#define IsTileSu(tile) (TileFlag2[tile] & 0x02)
#define IsTileIg(tile) (TileFlag2[tile] & 0x10)
#define IsInvisible(i) (ObjStatus[i] & 0x02)
#define IsPlrControl(i) (NPCStatus[i] & 0x80)
#define HideMouse if(MouseOn) MOUSE_HIDE();
#define ShowMouse if(MouseOn) MOUSE_SHOW();

/*how far the active member reaches: its level with the first talent*/
#define REACH (D_16B0[Party[Active]].a ? Level[Party[Active]] : 0)

/*the plaques: 0xac by its quality 1-12 (on the surface, with the
  coordinates), 0xe8 by quality 1*/
/*042A*/char *PlaqueNames[] = {
	"Coprates Chasma", "Arsia Mons", "Pavonis Mons", "Ascraeus Mons",
	"Alber Tholus", "Elysium Mons", "Hecates Tholus", "Terra Sirenum",
	"Noctis Labyrinthus", "Deuteronicus Mensae", "Syrtis Major Planum",
	"Olympus Mons", "Test Plaque"
};

/*the object of party slot partyIdx (MD has no vehicles)*/
/*C_047A_000E*/GetPartyObjOrVehicle(int16 partyIdx) {
	return Party[partyIdx];
}

/*X of objNum*/
/*C_047A_001E*/static GetObjXOrVehicle(int16 objNum) {
	return GetX(objNum);
}

/*Y of objNum*/
/*C_047A_0036*/static GetObjYOrVehicle(int16 objNum) {
	return GetY(objNum);
}

/*the object on the map that (directly or not) holds objNum*/
/*C_047A_0052*/GetOuterHolder(register int16 objNum) {
	do
		objNum = GetAssoc(objNum);
	while(GetCoordUse(objNum) != LOCXYZ);
	return objNum;
}

/*resistance roll, target strength vs. caster strength:
  returns non-zero if the target resists*/
/*C_047A_0074*/ResistStrVsStr(int16 caster, int16 target) {
	return ((GetStr(target) / 2) + 30 - GetStr(caster)) / 2 > OSI_rand(1, 30);
}

/*can itemObj be put into containerObj? returns 1/0*/
/*C_047A_00BA*/CanPutInContainer(int16 containerObj, int16 itemObj) {
	int16 contTyp, itemTyp;

	if(itemObj >= 0x100) {
		itemTyp = GetType(itemObj);
		contTyp = GetType(containerObj);
		if(C_1B09_0D34(itemTyp, containerObj) >= 0x10)
			return 0;
		if(itemTyp == 0x100 && contTyp == 0xa0 && !GetFrame(containerObj))
			return 1;
		if(IsContainerType(contTyp) && !GetFrame(containerObj)) {
			if(contTyp == 0x155)
				return 0;
			if(C_100B_01D4(itemObj) && GetFrame(itemObj) > 0)
				return 0;
			if(IsContainerType(itemTyp))
				return 0;
			return 1;
		}
	}
	return 0;
}

/*parse a number typed by the player.
 coordMode 0: plain number; returns the value, -1 for an empty line
   accepted with <Enter>, -2 for an escaped line or a bad character.
 coordMode 1: "<n>S" (positive) or "<n>N" (negative),
 coordMode 2: "<n>E" (positive) or "<n>W" (negative);
   returns 100 when the letter is missing/wrong.*/
/*C_047A_016F*/ParseNumberInput(unsigned char *str, int16 coordMode) {
	int16 i, value;

	value = 0;
	for(i = 0; i < strlen(str); i ++) {
		if(str[i] >= '0' && str[i] <= '9') {
			value = value * 10 + (str[i] - '0');
			continue;
		}
		if(coordMode == 1) {
			if(toupper(str[i]) == 'S')
				return value;
			else if(toupper(str[i]) == 'N')
				return -value;
			else
				return 100;
		}
		if(coordMode == 2) {
			if(toupper(str[i]) == 'E')
				return value;
			else if(toupper(str[i]) == 'W')
				return -value;
			else
				return 100;
		}
		return -2;
	}
	if(coordMode)
		return 100;
	/*GetsAborted is set by CON_gets when the input was escaped*/
	if(i == 0 && GetsAborted == 0)
		return -1;
	if(i == 0 && GetsAborted)
		return -2;
	return value;
}

/*show the portrait of objNum in the status panel, with its name*/
/*C_047A_024B*/void ShowPortrait(register int16 objNum) {
	int16 unused2, unused4, unused6;
	int16 savedStatus;
	struct tClipInfo *savedClip;
	char *name;

	unused2 = 0;
	unused4 = 0;
	unused6 = 0;
	if(objNum == 0x67 && !(TalkFlags[0x67] & 4))
		objNum = 0x6d;
	if(objNum >= 10 && objNum <= 15)
		objNum = 0x51;
	if(objNum >= 0xc0) {
		switch(GetType(objNum)) {
			case 0x179:
				objNum = 0x1d;
			default:
				return;
		}
	} else if(objNum > 0xbf) {
		return;
	}
	if(ObjStatus[objNum] & 1)
		objNum = D_16A8 ? 0x73 : 0x72;
	savedClip = CurClip;
	HideMouse
	savedStatus = StatusDisplay;
	if(!(D_16AA & 0x10)) {
		C_0E1A_158A();
	} else {
		GR_SET_COLOR(ColorPaper);
		GR_FILL_RECT(0xb8, 2, 0x137, 0x77);
	}
	DrawPortrait(objNum, 0xd1, 8);
	if(objNum >= 0xc0) {
		switch(ObjShapeType[objNum]) {
			case 0x179:
				name = "wisp";
				break;
			default:
				name = GetObjectString(objNum);
				break;
		}
	} else if(objNum <= 1 || objNum == (D_16A8 ? 0x73 : 0x72)) {
		name = "yourself";
	} else if(!(TalkFlags[objNum] & 1)) {
		name = GetObjectString(objNum);
	} else {
		name = TALK_GetCharName(objNum);
	}
	CON_setClip(&TextWindows[1]);
	CON_gotoxy(1, 12);
	CON_printf(C_2BB4_1C68);
	CON_gotoxy((14 - strlen(name)) / 2 + 1, 12);
	CON_printf("%s\n", name);
	StatusDisplay = 0x9e;
	if(!IsInConversation) {
		CON_setClip(&TextWindows[3]);
		CON_ReadCommand();
	}
	CON_setClip(savedClip);
	ShowMouse
	StatusDisplay = savedStatus;
	if(!IsInConversation) {
		StatusDirty = 1;
		RefreshStatus();
	}
}

/*print the article ("a ", "an " or "the ") of tile numTile*/
/*C_047A_0416*/void PrintTileArticle(int16 numTile) {
	switch(TileFlag2[numTile] & 0xc0) {
		case 0x40: CON_printf("a "); break;
		case 0x80: CON_printf("an "); break;
		case 0xc0: CON_printf("the "); break;
	}
}

/*print the count (stackables, gold) or the article of objNum before
 its name; returns the count (1 for a single object)*/
/*C_047A_0455*/PrintObjCountOrArticle(register int16 objNum) {
	int16 quanType;
	register int16 count;

	/*QuanType: 4 = the whole Amount is the count, 2 = stackable (low
	byte), 0 = single object*/
	quanType = QuanType(GetType(objNum));
	if(quanType == 4) {
		count = Amount[objNum];
		CON_printf("%d ", count);
	} else if(quanType == 2) {
		count = GetQuan(objNum);
		CON_printf("%d ", count);
	} else {
		count = 1;
		PrintTileArticle(TILE_FRAME(objNum));
	}
	return count;
}

/*the first object at objNum's location, after objNum, that is not
 "ignored" (TileFlag2 0x10) and is not an NPC; -1 if none*/
/*C_047A_04DA*/FindNonIgnoredObjAt(register int16 objNum) {
	register int16 other;

	for(other = FindLoc(GetX(objNum), GetY(objNum), MapZ); other >= 0; other = NextLoc()) {
		if(other == objNum)
			continue;
		if(!(TileFlag2[TILE_FRAME(other)] & 0x10)) {
			if(other > 0xff)
				break;
		}
	}
	return other;
}

/*search objNum (after looking at it): empties a container on the map onto
  its square and lists what was found*/
/*C_047A_0564*/void SearchObj(register int16 objNum) {
	register int16 item;
	int16 actor, actorX, actorY, objTyp, objFrm;
	int16 found, nextItem;

	found = 0;
	actor = Party[Active];
	actorX = GetX(actor);
	actorY = GetY(actor);
	if(IsTileIg(TILE_FRAME(objNum)))
		objNum = FindNonIgnoredObjAt(objNum);
	objTyp = GetType(objNum);
	objFrm = GetFrame(objNum);
	if(GetCoordUse(objNum) != LOCXYZ)
		return;
	if(!IsContainerType(objTyp))
		return;
	if(objTyp < 0x151 && objFrm)
		return;
	if((item = FindInv(objNum)) < 0) {
		CON_printf(C_2BB4_1C78);
		return;
	}
	CON_printf(C_2BB4_1C9C);
	while(item >= 0) {
		objTyp = GetType(item);
		MoveObj(item, GetX(objNum), GetY(objNum), MapZ);
		MoveObj(actor, actorX, actorY, MapZ);
		VIS_UpdateView();
		DisplayTick();
		nextItem = FindInv(objNum);
		if(found && nextItem >= 0)
			CON_printf(", ");
		else if(found && nextItem == -1)
			CON_printf(" and ");
		if(QuanType(objTyp) && GetQuan(item) == 0)
			SetQuan(item, 1);
		PrintObjCountOrArticle(item);
		CON_printf(GetObjectString(item));
		found = 1;
		item = nextItem;
	}
	CON_printf(".\n");
}

/*"look" command: describe the selected object or tile (the state of many
  objects: lamps, plants, carts, clocks, guns, panels, plaques with their
  coordinates), show the portrait of a person, then search a container
  next to the party*/
/*C_047A_0734*/void CmdLook() {
	unsigned char numTile;
	int16 count, x, y, dir, objFrm, lat, lon, readSomething;
	register int16 qual, objTyp;

	readSomething = 0;
	CurClip->attrFlags |= 0x10;
	if(QuanType(GetType(SelectionObj)) && GetQuan(SelectionObj) == 0)
		SetQuan(SelectionObj, 1);
	CON_printf(C_2BB4_1CB7);
	dir = MkDirection(PointerX, PointerY);
	if(dir != -1) {
		C_170D_083F(Party[Active], dir);
		VIS_UpdateView();
	}
	if(SelectionIsDark) {
		CON_printf(C_2BB4_1CC0);
		EndPagingAtPrompt = 1;
		return;
	}
	x = GetObjXOrVehicle(Party[Active]);
	y = GetObjYOrVehicle(Party[Active]);
	if(SelectionObj == -1 || IsInvisible(SelectionObj) || IsTileIg(TILE_FRAME(SelectionObj))) {
		/*no object: describe the map tile under the pointer*/
		if(PointerX < 168 && PointerY < 168) {
			SelectionX = (SCR2TIL(PointerX) + MapX - 5) & 0x3ff;
			SelectionY = (SCR2TIL(PointerY) + MapY - 5) & 0x3ff;
			numTile = GetTileAtXYZ(SelectionX, SelectionY, MapZ);
			PrintTileArticle(numTile);
			CON_printf(GetTileString(numTile));
		} else {
			CON_printf(C_2BB4_0402);
		}
		CON_printf(".\n");
	} else {
	objTyp = GetType(SelectionObj);
	objFrm = GetFrame(SelectionObj);
	if(SelectionObj <= 1) {
		CON_printf(C_2BB4_1CCB);
		EndPagingAtPrompt = 1;
		return;
	}
	if(SelectionObj < 0x100 && SelectionObj >= 0) {
		/*a person or a creature: name and portrait*/
		if(!IsPlrControl(SelectionObj))
			PrintObjCountOrArticle(SelectionObj);
		CON_printf(GetObjectString(SelectionObj));
		SelectMode = 0;
		ShowPortrait(SelectionObj);
	} else if(objTyp == 0x124) {
		PrintObjCountOrArticle(SelectionObj);
		CON_printf(GetObjectString(SelectionObj));
		CON_printf(".\n");
		C_0E1A_15D1(GetQual(SelectionObj));
		CON_ReadCommand();
		StatusDirty = 1;
		RefreshStatus();
		return;
	} else {
		PrintObjCountOrArticle(SelectionObj);
		CON_printf(GetObjectString(SelectionObj));
		CON_printf(".\n");
		if(objTyp == 0xfb && objFrm <= 3) {
			/*a plant*/
			switch(qual = GetQual(SelectionObj)) {
				case 0xf: CON_printf(C_2BB4_1CD5); break;
				case 0x10: CON_printf(C_2BB4_1CE5); break;
				default: CON_printf(C_2BB4_1CF2);
			}
		} else if(C_100B_01D4(objTyp) || C_100B_01FB(objTyp)) {
			/*a light: its fuel or wick*/
			if((objTyp == 0x6d || objTyp == 0x6e) && GetQuan(SelectionObj) > 1)
				;
			else {
				qual = GetQual(SelectionObj);
				if(qual > 30)
					CON_printf("\nPlenty of ");
				else if(qual > 6 && qual <= 30)
					CON_printf("\nIt has %d more minutes of ", qual);
				else if(qual > 0 && qual <= 6)
					CON_printf("\nIt's almost out of \n");
				else if(objTyp == 0x73 || objTyp == 0x75 || objTyp == 0x74 || objTyp == 0x76)
					CON_printf("\nNo ");
				else
					CON_printf("\nPlenty of ");
				if(objTyp == 0x73 || objTyp == 0x75 || objTyp == 0x74 || objTyp == 0x76)
					CON_printf("fuel.\n");
				else
					CON_printf("wick.\n");
			}
		} else if(objTyp == 0x19b) {
			qual = GetQual(SelectionObj);
			if(qual == 1)
				CON_printf("\nIt seems to be loose.\n");
			else if(qual == 2)
				CON_printf("\nIt is not installed.\n");
		} else if(objTyp == 0x11f) {
			qual = GetQual(SelectionObj);
			if(qual)
				CON_printf(C_2BB4_1D06);
		} else if(objTyp == 0x19a || objTyp == 0x10c) {
			/*a cart or a barrel: its load*/
			if((count = GetQuan(SelectionObj)) == 0) {
				CON_printf(C_2BB4_1D1C);
			} else {
				CON_printf(C_2BB4_1D2A, count);
				switch(GetQual(SelectionObj)) {
					case 1: CON_printf("dirt.\n"); break;
					case 2: CON_printf("rock.\n"); break;
					case 3: CON_printf("iron ore.\n"); break;
					case 4: CON_printf("coal.\n"); break;
				}
			}
		} else if(objTyp == 0x5b || objTyp == 0x62) {
			/*a clock*/
			CON_printf(C_2BB4_1D40);
			if(Time_H == 0)
				CON_printf("12:%02d A.M.\n", Time_M);
			else if(Time_H < 12)
				CON_printf("%d:%02d A.M.\n", Time_H, Time_M);
			else if(Time_H == 12)
				CON_printf("12:%02d P.M.\n", Time_M);
			else
				CON_printf("%d:%02d P.M.\n", Time_H - 12, Time_M);
		} else if(objTyp == 0x2d) {
			qual = GetQual(SelectionObj);
			CON_printf(C_2BB4_1D4F, qual?(qual > 1?"Shotgun":"Rifle"):"Combination");
			CON_printf(".\n");
		} else if(objTyp == 0xf0 || objTyp == 0xf1) {
			qual = GetQual(SelectionObj);
			CON_printf(C_2BB4_1D4F, qual?(qual > 1?"Shotgun":"Rifle"):"Combination");
			CON_printf(C_2BB4_1D61, GetQuan(SelectionObj));
		} else if(objTyp == 0x13a) {
			qual = GetQual(SelectionObj);
			if((qual & 1) && GetCoordUse(SelectionObj) == LOCXYZ)
				CON_printf(C_2BB4_1D7F);
			if(qual & 2)
				CON_printf(C_2BB4_1D98);
		} else if(objTyp == 0x1ca) {
			/*a control panel: what it controls*/
			count = GetQuan(SelectionObj);
			switch(count) {
				case 4: CON_printf(C_2BB4_13D8); break;
				case 5: CON_printf(C_2BB4_1400); break;
				case 6: CON_printf(C_2BB4_1427); break;
				case 7: CON_printf(C_2BB4_1454); break;
				case 0:
					switch(GetFrame(SelectionObj)) {
						case 0: CON_printf(C_2BB4_147F); break;
						case 1: CON_printf(C_2BB4_14A8); break;
						case 2: CON_printf(C_2BB4_14CC); break;
					}
					break;
				default: CON_printf(C_2BB4_14F5);
			}
			qual = GetQual(SelectionObj);
			if((qual & 1) && GetCoordUse(SelectionObj) == LOCXYZ)
				CON_printf(C_2BB4_09CE);
			if(qual & 2)
				CON_printf(C_2BB4_09E3);
		} else if(objTyp == 0x1c9) {
			qual = GetQual(SelectionObj);
			switch(qual) {
				case 1: CON_printf(C_2BB4_15C4); break;
				case 2: CON_printf(C_2BB4_15F0); break;
				case 3: CON_printf(C_2BB4_1617); break;
				case 4: CON_printf(C_2BB4_1517); break;
				case 5: CON_printf(C_2BB4_1542); break;
				case 6: CON_printf(C_2BB4_156C); break;
				case 7: CON_printf(C_2BB4_1599); break;
				default: CON_printf(C_2BB4_1644);
			}
		} else if(objTyp == 0x81 || objTyp == 0x105) {
			CON_printf(C_2BB4_1DB2, GetQual(SelectionObj)?GetTileString(BaseTile[0x77]):GetTileString(BaseTile[0x80]));
			CON_printf(C_2BB4_1D61, GetQuan(SelectionObj));
		} else if(objTyp == 0x114) {
			/*the sprayer*/
			qual = GetQual(SelectionObj);
			if(!(qual & 1)) {
				CON_printf(C_2BB4_0F45);
				if((TalkFlags[0x3e] & 0x20) && !(qual & 2))
					CON_printf(C_2BB4_0FA8);
			} else if((TalkFlags[0x3e] & 0x20) && !(qual & 2)) {
				CON_printf(C_2BB4_0F70);
			} else {
				CON_printf(C_2BB4_1DC6);
			}
		} else if(objTyp == 0xb3 || objTyp == 0xe3) {
			qual = GetQual(SelectionObj);
			if(qual & 0x80) {
				if(qual - 0x80 >= 0x40)
					CON_printf(C_2BB4_1223);
				else
					CON_printf(C_2BB4_1242);
			}
		} else if(objTyp == 0xac && MapZ == 0) {
			/*a plaque on the surface: its name and coordinates*/
			qual = GetQual(SelectionObj);
			if(qual > 12)
				CON_printf(C_2BB4_22F1);
			else if(qual)
				CON_printf(PlaqueNames[qual - 1]);
			lat = (GetY(SelectionObj) - 0x200L) * 0xf0 / 0x400;
			if(lat > 0)
				CON_printf(" %dS ", lat);
			else if(lat < 0)
				CON_printf(" %dN ", -lat);
			else
				CON_printf(" 0 ");
			lon = (GetX(SelectionObj) - 0x200L) * 0x168 / 0x400;
			if(lon == 180 || lon == -180 || lon == 0)
				CON_printf("%d\n", lon);
			if(lon > 0)
				CON_printf(" %dW\n", 180 - lon);
			else if(lon != 0)
				CON_printf(" %dE\n", lon + 180);
		} else if(objTyp == 0xe8) {
			qual = GetQual(SelectionObj);
			if(qual > 1)
				CON_printf(C_2BB4_22F1);
			else if(qual)
				CON_printf(PlaqueNames[qual + 11]);
		} else if(objTyp == 0x14d) {
			if(GetQual(SelectionObj))
				CON_printf(C_2BB4_053C);
		} else if(objTyp == 0x1cc) {
			TalkFlags[2] |= 0x40;
			TALK_talkTo(1, 2, 0);
			return;
		} else if(IsContainerType(objTyp) && GetCoordUse(SelectionObj) == LOCXYZ &&
			MapDistance(x, SelectionX) <= 1 && MapDistance(y, SelectionY) <= 1) {
			SearchObj(SelectionObj);
		}
	}
	}
	EndPagingAtPrompt = 1;
}

/*is there something to stand an object on at (x, y): an object whose tile
  has TileFlag2 0x02 before one that is impassable?*/
/*C_047A_1173*/static HasSupportAt(int16 x, int16 y) {
	int16 objNum;

	for(objNum = FindLoc(x, y, MapZ); objNum >= 0; objNum = NextLoc()) {
		if(IsTileSu(TILE_FRAME(objNum) - LocPartOffset))
			return 1;
		if(IsTerrainImpass(TILE_FRAME(objNum) - LocPartOffset))
			return 0;
	}
	return 0;
}

/*ask "How many?" for a stack of objNum (not when readied);
 returns the quantity chosen (all on empty input, 0 if invalid),
 1 for a single object*/
/*C_047A_1209*/AskHowMany(int16 objNum) {
	int16 answer, available;
	char buf[5];
	int16 quanType, cursorY, savedSel, savedSelMode;

	quanType = QuanType(GetType(objNum));
	if(quanType && GetCoordUse(objNum) != EQUIP) {
		available = quanType == 2?GetQuan(objNum):Amount[objNum];
		if(available != 1) {
			savedSel = SelectionObj;
			cursorY = CurClip->curY;
			CON_printf("How many? ");
			savedSelMode = SelectMode;
			DrawSlotCursor(0);
			SelectMode = 0;
			CON_gets(buf, 4);
			SelectMode = savedSelMode;
			SelectionObj = savedSel;
			answer = ParseNumberInput(buf, 0);
			if(answer == -1) {
				CON_gotoxy(10, cursorY);
				CON_printf("%d\n", available);
				return available;
			}
			if(answer > available || answer == 0 || answer == -2) {
				CON_gotoxy(10, cursorY);
				CON_printf("0   \n");
				return 0;
			}
			CON_putch('\n');
		} else {
			answer = 1;
		}
	} else {
		answer = 1;
	}
	return answer;
}

/*can an object be put at (x, y): neither the map tile nor an object there
  is impassable (terrain bit 0x02) without one of the side bits 0xf0, and
  the map tile has no bit 0x04? returns 1 when it can*/
/*C_047A_1325*/C_047A_1325(int16 x, int16 y) {
	int16 flags, obj;

	flags = TerrainType[GetTileAtXYZ(x, y, MapZ)];
	if(flags & 4)
		return 0;
	if((flags & 2) && !(flags & 0xf0))
		return 0;
	for(obj = FindLoc(x, y, MapZ); obj >= 0; obj = NextLoc()) {
		flags = TerrainType[TILE_FRAME(obj) - LocPartOffset];
		if((flags & 2) && !(flags & 0xf0))
			return 0;
	}
	return 1;
}

/*is (x, y) water with nothing on it to stand an object on (terrain bits
  0x01 and 0x02, no object whose tile has TileFlag2 0x02 or 0x04)?*/
/*C_047A_13C5*/C_047A_13C5(int16 x, int16 y) {
	int16 flags, tile, obj;

	flags = TerrainType[tile = GetTileAtXYZ(x, y, MapZ)];
	if((flags & 1) && (flags & 2)) {
		for(obj = FindLoc(x, y, MapZ); obj >= 0; obj = NextLoc()) {
			tile = TILE_FRAME(obj) - LocPartOffset;
			if((TileFlag2[tile] & 2) || (TileFlag2[tile] & 4))
				return 0;
		}
		return 1;
	}
	return 0;
}

/*"drop" command: drop or throw the selected inventory object of the
 displayed party member to a location (not onto a person): into the water
 (lost, with a splash), into a container, or on the ground; the breakable
 types (C_100B_01AD) break when thrown farther than the actor's reach. The
 hot objects 0x1c0/0x1c1 burn whoever has no talent or gloves 0x88*/
/*C_047A_145E*/void CmdDrop() {
	int16 destX, destY, newObj, actor, target, objFrm, targetTyp, holder;
	int16 qtyCode, count, quanType, cantDrop, unused, dist, key;
	register int16 obj, objTyp;

	unused = 0;
	if(SelectionObj == -1 || GetCoordUse(SelectionObj) == LOCXYZ || SelectionObj == InvenOwner) {
		CON_printf(C_2BB4_0402);
		return;
	}
	CON_printf(GetObjectString(SelectionObj));
	CON_putch('\n');
	obj = SelectionObj;
	objTyp = GetType(obj);
	quanType = QuanType(objTyp);
	actor = Party[StatusMember];
	holder = GetAssoc(obj);
	objFrm = GetFrame(obj);
	if((objTyp == 0x1c1 || objTyp == 0x1c0) && !REACH && actor != 6 &&
		GetType(Equipment[2]) != 0x88 && GetType(Equipment[5]) != 0x88) {
		C_07D1_126D(Party[Active]);
		NPCStatus[Party[Active]] |= 8;
		CON_printf(C_2BB4_0D50);
	}
	if(C_100B_01FB(objTyp) && objTyp != 0x6d)
		cantDrop = 0;
	else
		cantDrop = STAT_CheckUnready(obj, actor);
	if(cantDrop == 0) {
		if(quanType && (count = AskHowMany(SelectionObj)) == 0)
			return;
		CON_printf(C_2BB4_1A50);
		MouseMode = 1;
		if(SelectMode != 0) {
			DrawSlotCursor(0);
			SelectMode = 1;
			AimX = AimY = 5;
		}
		key = CON_getch();
		if(key != 0x8e || SelectionX < 0 || SelectionY < 0) {
			CON_printf(C_2BB4_03F8);
			return;
		}
		CON_putch('\n');
		destX = SelectionX;
		destY = SelectionY;
		dist = C_07D1_35A8(destX, destY, GetX(actor), GetY(actor));
		if(OBJ_FindNPCAt(1, destX, destY) != -1) {
			CON_printf(C_2BB4_034D);
			MUS_PlaySfx(5);
			return;
		}
		target = FindLoc(destX, destY, MapZ);
		if(C_07D1_0CFC(actor, &destX, &destY, 8, 0)) {
			if(C_047A_13C5(destX, destY) && objTyp != 0x18b) {
				/*into the water: lost*/
				if(obj >= 0x100 || obj < 0) {
					C_07D1_0CFC(actor, &destX, &destY, 8, TILE_FRAME(obj));
					if(quanType && GetQuan(obj) > count) {
						SubQuan(obj, count);
					} else {
						DeleteObj(obj);
						obj = -1;
					}
					if(StatusDisplay == 0x92)
						STAT_DrawInventory();
					if(InRange_10_17_C0_C3(GetTileAtXYZ(destX, destY, MapZ)) && (TalkFlags[0x76] & 0x40) && (TalkFlags[0x76] & 0x20) ||
						GetTileAtXYZ(destX, destY, MapZ) >= 1 && GetTileAtXYZ(destX, destY, MapZ) <= 4 && (TalkFlags[0x76] & 0x20))
						C_1FA9_0BE4();
					else
						C_1FA9_0A8A();
					ShowObjects();
					return;
				} else {
					CON_printf(C_2BB4_034D);
					MUS_PlaySfx(5);
					return;
				}
			}
		} else {
			CON_printf(C_2BB4_0230);
			MUS_PlaySfx(5);
			return;
		}
		if(!C_047A_1325(destX, destY) && !HasSupportAt(destX, destY)) {
			if(target == -1 || !CanPutInContainer(target, obj)) {
				CON_printf(C_2BB4_034D);
				MUS_PlaySfx(5);
				return;
			}
		}
		/*unready it*/
		STAT_StopEquipEffect(obj, actor);
		if(GetCoordUse(obj) == EQUIP) {
			Equipment[STAT_FindEquipSlot(obj)] = 0;
			if(STAT_GetEquipSlot(obj) == 8) {
				Party_2Handed[StatusMember] = 0;
				Equipment[5] = 0;
			}
		}
		STAT_ComputeWeights(actor);
		if((objTyp == 0x13a || objTyp == 0xce) && !HasSupportAt(destX, destY)) {
			CON_printf(C_2BB4_1DDB, GetObjectString(obj));
			return;
		}
		SubMov(actor, 3);
		/*animate the throw; destX/destY = where it lands*/
		C_07D1_0CFC(actor, &destX, &destY, 8, TILE_FRAME(obj));
		if(dist > 1 && REACH < dist && C_100B_01AD(objTyp)) {
			if(quanType) {
				qtyCode = QualQuan(GetQual(obj), count);
				if(GetQuan(obj) > count) {
					SubQuan(obj, count);
				} else {
					DeleteObj(obj);
					obj = -1;
				}
			} else if(objTyp == 0x133) {
				MoveObj(obj, destX, destY, MapZ);
				C_07D1_1D64(obj, 1, 0);
				STAT_RefreshInventory(InvenOwner);
				StatusDirty = 0;
				if(StatusDisplay == 0x92)
					STAT_DrawInventory();
				return;
			} else if(C_100B_038E(objTyp) && GetQuan(obj)) {
				MoveObj(obj, destX, destY, MapZ);
				C_07D1_1D64(obj, GetQuan(obj), 0);
			} else {
				if(IsContainerType(objTyp))
					MoveObj(obj, destX, destY, MapZ);
				DeleteObj(obj);
			}
			CON_printf("\nIt broke!\n");
			VIS_UpdateView();
			StatusDirty = 0;
			STAT_RefreshInventory(InvenOwner);
			if(StatusDisplay == 0x92)
				STAT_DrawInventory();
			return;
		}
		if(GetType(target) == 0xa0 && objTyp == 0x100) {
			DeleteObj(obj);
			SetFrame(target, 2);
			ShowObjects();
		} else if(target >= 0 && IsContainerType(targetTyp = GetType(target)) && targetTyp != 0x101 && CanPutInContainer(target, obj)) {
			/*into the container at the destination*/
			if(quanType) {
				qtyCode = QualQuan(GetQual(obj), count);
				if((newObj = AddInvObj(objTyp | objFrm << 10, CONTAINED, qtyCode, target)) >= 0) {
					if(GetQuan(obj) > count)
						SubQuan(obj, count);
					else
						DeleteObj(obj);
				}
			} else {
				InsertObj(obj, target, CONTAINED);
			}
		} else {
			/*on the ground (a new stack for a partial quantity)*/
			if(quanType) {
				qtyCode = QualQuan(GetQual(obj), count);
				if((newObj = AddMapObj(objTyp | objFrm << 10, 0, qtyCode, destX, destY, MapZ)) >= 0) {
					if(GetQuan(obj) > count)
						SubQuan(obj, count);
					else
						DeleteObj(obj);
				}
			} else {
				MoveObj(obj, destX, destY, MapZ);
			}
		}
		if(obj != -1 && C_100B_01D4(obj) && GetFrame(obj))
			PassTime(0);
		VIS_UpdateView();
		StatusDirty = 0;
	}
	STAT_RefreshInventory(InvenOwner);
}

/*"get" command: pick up the selected object, next to the actor or within
 its reach (it flies to the actor). A body 0x155 is searched first; the
 panels and covers (0x19b, 0x1ca, 0x13a, 0xce) come off only when loose
 (quality odd); the lamps 0x6e-0x76 go into a free hand, lit; the hot
 objects 0x1c0/0x1c1 burn as in CmdDrop*/
/*C_047A_1C0E*/void CmdGet() {
	int16 weight, strength, actorX, actorY, dir, room, caught;
	int16 item, nextItem, found, x, y, objFrm, dist, w;
	register int16 objTyp, actor;

	caught = 0;
	nextItem = -1;
	actor = Party[Active];
	actorX = GetX(actor);
	actorY = GetY(actor);
	/*pointing at an NPC or an "ignore" tile: take what is under it*/
	if((SelectionObj <= 0xff && SelectionX != -1) || IsTileIg(TILE_FRAME(SelectionObj)))
		SelectionObj = FindNonIgnoredObjAt(SelectionObj);
	if(SelectionObj == -1 || GetCoordUse(SelectionObj) != LOCXYZ) {
		CON_printf(C_2BB4_0402);
		return;
	}
	dir = MkDirection(PointerX, PointerY);
	if(dir != -1) {
		C_170D_083F(actor, dir);
		VIS_UpdateView();
	}
	if(QuanType(GetType(SelectionObj)) && GetQuan(SelectionObj) == 0)
		SetQuan(SelectionObj, 1);
	PrintObjCountOrArticle(SelectionObj);
	CON_printf(GetObjectString(SelectionObj));
	CON_putch('\n');
	if(MapDistance(actorX, SelectionX) <= 1 && MapDistance(actorY, SelectionY) <= 1) {
		dist = 1;
	} else if(!REACH) {
		CON_printf(C_2BB4_03A6);
		MUS_PlaySfx(5);
		return;
	} else {
		dist = C_07D1_35A8(actorX, actorY, SelectionX, SelectionY);
		if(!C_07D1_0CFC(SelectionObj, &actorX, &actorY, 8, 0)) {
			CON_printf(C_2BB4_0230);
			MUS_PlaySfx(5);
			return;
		}
	}
	objTyp = GetType(SelectionObj);
	objFrm = GetFrame(SelectionObj);
	w = GetWeight(SelectionObj);
	if(objTyp == 0x155)
		weight = w;
	else
		weight = w + Encumbrance(SelectionObj);
	strength = GetStr(Party[Active]);
	if(Active != StatusMember || StatusDisplay != 0x92)
		STAT_ComputeWeights(actor);
	/*weight 0 (except the types weighed per 10) or 255 means "can not be
	taken"; a character carries 20 x STR (weights are in tenths)*/
	if((weight == 0 && !IsWeightPer10Type(objTyp)) || w == 0xff || (SelectionObj < 0x100 && SelectionObj >= 0)) {
		CON_printf(C_2BB4_034D);
		MUS_PlaySfx(5);
	} else if(strength * 20 < weight + WeightInven + WeightEquip) {
		CON_printf(C_2BB4_03D7);
		MUS_PlaySfx(5);
	} else {
		if(dist > 1) {
			MUS_PlaySfx(0x32);
			C_07D1_0CFC(SelectionObj, &actorX, &actorY, 8, TILE_FRAME(SelectionObj));
		}
		SubMov(actor, 3);
		if((objTyp == 0x1c1 || objTyp == 0x1c0) && !REACH && actor != 6 &&
			GetType(Equipment[2]) != 0x88 && GetType(Equipment[5]) != 0x88) {
			C_07D1_126D(Party[Active]);
			NPCStatus[Party[Active]] |= 8;
			CON_printf(C_2BB4_0D50);
		}
		if(QuanType(objTyp)) {
			/*stackable: merge into the inventory*/
			if(AddInvObj(GetType(SelectionObj) | objFrm << 10, INVEN, GetQuan(SelectionObj), actor) == -1) {
				CON_printf(C_2BB4_024E);
				MUS_PlaySfx(5);
			} else {
				DeleteObj(SelectionObj);
			}
		} else if(objTyp == 0x155) {
			/*a body: take the content too*/
			if((item = FindInv(SelectionObj)) >= 0) {
				CON_printf(C_2BB4_1E04);
				x = GetX(SelectionObj);
				y = GetY(SelectionObj);
				while(item >= 0) {
					objTyp = GetType(item);
					MoveObj(item, x, y, MapZ);
					MoveObj(actor, actorX, actorY, MapZ);
					VIS_UpdateView();
					DisplayTick();
					nextItem = FindInv(SelectionObj);
					if(found && nextItem >= 0)
						CON_printf(", ");
					else if(found && nextItem == -1)
						CON_printf(" and ");
					if(QuanType(objTyp) && GetQuan(item) == 0)
						SetQuan(item, 1);
					PrintObjCountOrArticle(item);
					CON_printf(GetObjectString(item));
					found = 1;
					item = nextItem;
				}
				CON_printf(".\n");
			} else {
				CON_printf(C_2BB4_1E04);
				CON_printf(C_2BB4_0402);
			}
			if(InsertObj(SelectionObj, actor, INVEN) == -1) {
				CON_printf(C_2BB4_024E);
				MUS_PlaySfx(5);
				return;
			}
		} else if(objTyp == 0x19b || objTyp == 0x1ca || objTyp == 0x13a || objTyp == 0xce) {
			/*comes off only when loose*/
			if((room = GetQual(SelectionObj)) % 2) {
				if(InsertObj(SelectionObj, actor, INVEN) != -1) {
					SetQual(SelectionObj, room);
				} else {
					CON_printf(C_2BB4_024E);
					MUS_PlaySfx(5);
				}
			} else {
				CON_printf(C_2BB4_1E23);
				MUS_PlaySfx(0);
			}
		} else if(objTyp == 0x6e || objTyp == 0x70 || objTyp == 0x72 || objTyp == 0x74 || objTyp == 0x76) {
			/*a lamp: into a free hand*/
			if(Party[Active] == actor) {
				if(Equipment[2] && Equipment[5]) {
					CON_printf(C_2BB4_0449);
					return;
				}
				if(InsertObj(SelectionObj, actor, EQUIP) == -1) {
					CON_printf(C_2BB4_034D);
					MUS_PlaySfx(5);
					return;
				}
				if(objTyp == 0x6e)
					SetQual(SelectionObj, 0xb4);
				PassTime(0);
			}
		} else {
			if(InsertObj(SelectionObj, actor, INVEN) == -1) {
				CON_printf(C_2BB4_024E);
				MUS_PlaySfx(5);
				return;
			}
			if(objTyp == 0x100)
				CON_printf(C_2BB4_1E32);
		}
		if(Party[StatusMember] == actor && StatusDisplay == 0x92)
			STAT_RefreshInventory(actor);
		VIS_UpdateView();
	}
	if(Party[StatusMember] != actor)
		STAT_ComputeWeights(actor);
	MouseMode = 0;
}

/*can objNum be pushed from (x, y) one square in direction dir? returns 0
  when it can, 1 when it is blocked, 2 when it won't go that way. The
  barrel 0x19a floats on the canals only along their direction (its frame:
  even north-south, odd east-west); the type 0x1b9 turns towards the
  blocked direction instead*/
/*C_047A_234C*/PushObj(int16 objNum, int16 x, int16 y, register int16 dir) {
	int16 destX, destY, sideX1, sideY1, sideX2, sideY2;
	int16 frame, f, waterObj, blockObj, cur, ret, tile, isWater;
	int16 cursor0, cursor1, cursor2, cursor3, partOffset;
	register int16 type;

	destX = (x + DirIncrX[dir]) & 0x3ff;
	destY = (y + DirIncrY[dir]) & 0x3ff;
	tile = GetTileAtXYZ(destX, destY, MapZ);
	if((type = GetType(objNum)) == 0x19a) {
		isWater = Is_6C_6D_6E_4D_4F(tile);
		frame = GetFrame(objNum);
		if(dir % 2 || frame % 2 == 0 && (dir == 0 || dir == 4) || frame % 2 == 1 && (dir == 6 || dir == 2)) {
			ret = 2;
		} else {
			waterObj = -1;
			blockObj = -1;
			cursor0 = LocCursor[0];
			cursor1 = LocCursor[1];
			cursor2 = LocCursor[2];
			cursor3 = LocCursor[3];
			partOffset = LocPartOffset;
			for(cur = FindLoc(destX, destY, MapZ); cur != -1; cur = NextLoc()) {
				type = GetType(cur);
				if(type == 0x19e || type == 0x19d || type == 0x19c || type == 0x1a3 || type == 0xaf || type == 0xa3) {
					waterObj = cur;
					break;
				}
				if(type == 0x14e || type == 0x14f || type == 0x150) {
					cur = -1;
				} else if(type != 0x1a1) {
					blockObj = cur;
					break;
				}
			}
			LocCursor[0] = cursor0;
			LocCursor[1] = cursor1;
			LocCursor[2] = cursor2;
			LocCursor[3] = cursor3;
			LocPartOffset = partOffset;
			if(waterObj == -1 && !isWater || blockObj != -1) {
				ret = 1;
			} else if(waterObj == -1) {
				ret = 0;
			} else {
				f = waterObj == -1?isWater - 1:GetFrame(waterObj);
				if(frame % 2 == f || f == 2 || type == 0x19c || type == 0xaf || type == 0xa3)
					ret = 0;
				else
					ret = 2;
			}
		}
	} else if(type == 0x18b && OBJ_FindTypeAt(0xb0, destX, destY) >= 0) {
		ret = 0;
	} else if(CanMoveTo(objNum, destX, destY) || HasSupportAt(destX, destY)) {
		if(dir & 1) {
			if(C_100B_0222(type)) {
				ret = 2;
			} else {
				sideX1 = (x + DirIncrX[dir - 1]) & 0x3ff;
				sideY1 = (y + DirIncrY[dir - 1]) & 0x3ff;
				sideX2 = (x + DirIncrX[dir + 1]) & 0x3ff;
				sideY2 = (y + DirIncrY[dir + 1]) & 0x3ff;
				if(
					CanMoveTo(objNum, sideX1, sideY1) || HasSupportAt(sideX1, sideY1) ||
					CanMoveTo(objNum, sideX2, sideY2) || HasSupportAt(sideX2, sideY2)
				)
					ret = 0;
				else
					ret = 1;
			}
		} else {
			ret = 0;
		}
	} else if(type == 0x1b9) {
		ret = 1;
		switch(dir) {
			case 0: case 7:
				if(OBJ_FindTypeAt(0xaf, x, (y + 1) & 0x3ff) >= 0)
					f = 5;
				else
					f = 3;
				break;
			case 1: case 2:
				f = 4;
				break;
			case 3: case 4:
				f = 5;
				break;
			default:
				f = 1;
		}
		ObjShapeType[objNum] = GetType(objNum) + (f << 10);
		ShowObjects();
	} else {
		ret = 1;
	}
	return ret;
}

/*"move" command: push an object on the map by one tile within the actor's
 reach (into a container there, or lost in the water with a splash), or
 move an inventory object to another party member or into/out of a
 container; the panels 0x19b/0x1ca move only when loose, the hot objects
 0x1c0/0x1c1 burn as in CmdDrop*/
/*C_047A_26A5*/void CmdMove() {
	unsigned char ch;
	int16 quanType, destX, destY, objX, objY, dir;
	int16 weight, dist, count, qtyCode, srcMember, target, objFrm, actor, cantMove, unused;
	int16 itemTyp, dirMoved, actorX, actorY, targetTyp, pushRet;
	register int16 obj, objTyp;

	unused = 0;
	if(SelectionObj == -1 || SelectionObj == InvenOwner) {
		CON_printf(C_2BB4_0402);
		return;
	}
	LastNumberPlural = 0;
	CON_printf(GetObjectString(SelectionObj));
	CON_putch('\n');
	objTyp = GetType(SelectionObj);
	objFrm = GetFrame(SelectionObj);
	quanType = QuanType(objTyp);
	actorX = GetX(Party[Active]);
	actorY = GetY(Party[Active]);
	if(GetCoordUse(SelectionObj) == LOCXYZ) {
		dist = C_07D1_35A8(actorX, actorY, SelectionX, SelectionY);
		if(REACH + 1 < dist) {
			CON_printf(C_2BB4_03A6);
			return;
		}
		if((objTyp == 0x1c1 || objTyp == 0x1c0) && !REACH && Party[Active] != 6 &&
			GetType(Equipment[2]) != 0x88 && GetType(Equipment[5]) != 0x88) {
			C_07D1_126D(Party[Active]);
			NPCStatus[Party[Active]] |= 8;
			CON_printf(C_2BB4_0D50);
		}
		if((objTyp == 0x19b || objTyp == 0x1ca) && !(GetQual(SelectionObj) & 1)) {
			CON_printf(C_2BB4_0C18);
			MUS_PlaySfx(5);
			return;
		}
		dir = MkDirection(PointerX, PointerY);
		if(dir != -1) {
			C_170D_083F(Party[Active], dir);
			VIS_UpdateView();
		}
		objX = GetX(SelectionObj);
		objY = GetY(SelectionObj);
		MouseMode = 0;
		/*center the mouse direction cursor on the object
		(88,88 = center of the map window)*/
		DirOriginX = TIL2SCR((objX - MapX) & 0xff) + 88;
		DirOriginY = TIL2SCR((objY - MapY) & 0xff) + 88;
		CON_printf(C_2BB4_01C6);
		if(CON_getch() != 0x80) {
			CON_printf(C_2BB4_03F8);
			MouseMode = 0;
			DirOriginX = 88;
			DirOriginY = 88;
			CON_UpdateMousePointer();
			return;
		}
		DirOriginX = 88;
		DirOriginY = 88;
		CON_UpdateMousePointer();
		AdvanceBlocked = 1;
		dirMoved = AdvanceDir;
		AdvanceDir = mkMouseDir();
		CON_printf("%s.\n", DirNames[dirMoved]);
		actor = Party[Active];
		weight = GetWeight(SelectionObj) + Encumbrance(SelectionObj);
		if(TypeWeight[objTyp] == 0 && !IsWeightPer10Type(objTyp)) {
			CON_printf(C_2BB4_034D);
			MUS_PlaySfx(5);
		} else {
			/*an NPC resists being pushed (strength contest), can't push
			oneself; (frame & 3) == 3: lying*/
			if(SelectionObj < 0x100 && SelectionObj >= 0 && (ResistStrVsStr(actor, SelectionObj) || actor == SelectionObj ||
				((objFrm & 3) == 3 && objTyp != 0x155))) {
				CON_printf(C_2BB4_0272);
				MUS_PlaySfx(5);
				MouseMode = 0;
				return;
			}
			SubMov(Party[Active], 5);
			destX = (objX + DirIncrX[dirMoved]) & 0x3ff;
			destY = (objY + DirIncrY[dirMoved]) & 0x3ff;
			if(C_047A_13C5(destX, destY) && objTyp != 0x18b) {
				/*pushed into the water*/
				if(SelectionObj >= 0x100 || SelectionObj < 0) {
					DeleteObj(SelectionObj);
					if(InRange_10_17_C0_C3(GetTileAtXYZ(actorX, actorY, MapZ)) && (TalkFlags[0x76] & 0x40) && (TalkFlags[0x76] & 0x20) ||
						GetTileAtXYZ(actorX, actorY, MapZ) >= 1 && GetTileAtXYZ(actorX, actorY, MapZ) <= 4 && (TalkFlags[0x76] & 0x20))
						C_1FA9_0BE4();
					else
						C_1FA9_0A8A();
					ShowObjects();
					return;
				} else {
					CON_printf(C_2BB4_0272);
					MUS_PlaySfx(5);
					return;
				}
			}
			target = FindLoc(destX, destY, MapZ);
			targetTyp = GetType(target);
			if((objTyp == 0x13a || objTyp == 0xce) && !HasSupportAt(destX, destY)) {
				CON_printf(C_2BB4_1E47, GetObjectString(obj));
				return;
			}
			if(target != -1 && objTyp == 0x100 && targetTyp == 0xa0 && !GetFrame(target)) {
				SetFrame(target, 2);
				DeleteObj(SelectionObj);
				ShowObjects();
				return;
			}
			/*pushed onto a container: put it inside*/
			if(target != -1 && CanPutInContainer(target, SelectionObj) && weight != 0xff) {
				if(GetType(target) == 0x101 || InsertObj(SelectionObj, target, CONTAINED) == -1) {
					MoveObj(SelectionObj, destX, destY, MapZ);
					return;
				}
			} else {
				if((pushRet = PushObj(SelectionObj, objX, objY, dirMoved)) != 0) {
					if(pushRet == 2)
						CON_printf(C_2BB4_0210);
					else
						CON_printf(C_2BB4_0230);
					MUS_PlaySfx(5);
					MouseMode = 0;
					return;
				}
				MoveObj(SelectionObj, destX, destY, MapZ);
			}
			VIS_UpdateView();
		}
	} else {
		/*inventory object of the displayed member StatusMember*/
		if((objTyp == 0x1c1 || objTyp == 0x1c0) && !REACH && Party[Active] != 6 &&
			GetType(Equipment[2]) != 0x88 && GetType(Equipment[5]) != 0x88) {
			C_07D1_126D(Party[Active]);
			NPCStatus[Party[Active]] |= 8;
			CON_printf(C_2BB4_0D50);
		}
		cantMove = STAT_CheckUnready(SelectionObj, Party[StatusMember]);
		if(cantMove) {
			STAT_RefreshInventory(InvenOwner);
			return;
		}
		if(!IsNpcOnScreen(Party[StatusMember])) {
			CON_printf(C_2BB4_1E70);
			MUS_PlaySfx(5);
			return;
		}
		count = AskHowMany(SelectionObj);
		if(!count)
			return;
		CON_printf(C_2BB4_01C6);
		obj = SelectionObj;
		itemTyp = GetType(obj);
		srcMember = StatusMember;
		if(SelectMode != 0) {
			SelectRange = 7;
			DrawSlotCursor(0);
			SelectMode = 1;
			AimX = AimY = 5;
		}
		ch = CON_getch();
		if(ch != 0x8e || SelectionObj == -1) {
			CON_printf(C_2BB4_040C);
			return;
		}
		if(GetCoordUse(SelectionObj) == LOCXYZ) {
			dist = C_07D1_35A8(SelectionX, SelectionY, actorX, actorY);
			if((SelectionObj < 0x100 && SelectionObj >= 0 && !IsNpcOnScreen(SelectionObj)) ||
				((SelectionObj >= 0x100 || SelectionObj < 0) && REACH + 1 < dist &&
				MapDistance(SelectionX, actorX) > 1 && MapDistance(SelectionY, actorY) > 1)) {
				CON_printf(C_2BB4_03A6);
				MUS_PlaySfx(5);
				return;
			}
		}
		if(SelectionObj < 0x100 && SelectionObj >= 0) {
			/*give to another party member*/
			if(Party[srcMember] != SelectionObj) {
				CON_printf(GetObjectString(SelectionObj));
				CON_printf(".\n");
				if(!IsNpcOnScreen(SelectionObj)) {
					CON_printf(C_2BB4_1E70);
					MUS_PlaySfx(5);
					return;
				}
				if(!IsPlrControl(SelectionObj)) {
					CON_printf("\nOnly within the party!\n");
					MUS_PlaySfx(5);
					return;
				}
				STAT_ComputeWeights(SelectionObj);
				if(quanType) {
					if(IsWeightPer10Type(objTyp))
						weight = TypeWeight[objTyp] * count / 10;
					else
						weight = TypeWeight[objTyp] * count;
				} else {
					weight = GetWeight(obj) + Encumbrance(obj);
				}
				if(GetStr(SelectionObj) * 20 < weight + WeightInven + WeightEquip || C_1B09_0D34(objTyp, SelectionObj) >= 0x10) {
					CON_printf(C_2BB4_1E8A, GetObjectString(SelectionObj));
					MUS_PlaySfx(5);
					STAT_ComputeWeights(Party[StatusMember]);
					return;
				}
				if(GetCoordUse(obj) == EQUIP && Party[srcMember] != SelectionObj) {
					Equipment[STAT_FindEquipSlot(obj)] = 0;
					if(STAT_GetEquipSlot(obj) == 8) {
						Party_2Handed[srcMember] = 0;
						Equipment[5] = 0;
					}
				}
				if(quanType) {
					qtyCode = QualQuan(GetQual(obj), count);
					if(AddInvObj(objTyp | objFrm << 10, INVEN, qtyCode, SelectionObj) != -1) {
						if(GetQuan(obj) > qtyCode)
							SubQuan(obj, qtyCode);
						else
							DeleteObj(obj);
					} else {
						CON_printf(C_2BB4_1E8A, GetObjectString(SelectionObj));
						MUS_PlaySfx(5);
					}
				} else if(C_100B_01D4(obj) && GetFrame(obj) > 0) {
					/*a burning light stays readied*/
					if(InsertObj(obj, SelectionObj, EQUIP) == -1) {
						CON_printf(C_2BB4_024E);
						MUS_PlaySfx(5);
						return;
					}
				} else {
					if(InsertObj(obj, SelectionObj, INVEN) == -1) {
						CON_printf(C_2BB4_1E8A, GetObjectString(SelectionObj));
						MUS_PlaySfx(5);
						return;
					}
				}
				STAT_ComputeWeights(Party[StatusMember]);
			} else {
				CON_printf(C_2BB4_1CCB);
			}
			dir = MkDirection(PointerX, PointerY);
			if(dir != -1) {
				C_170D_083F(Party[Active], dir);
				VIS_UpdateView();
			}
			SubMov(Party[Active], 5);
		} else {
			/*into (or out of) a container of the same owner*/
			if(GetCoordUse(SelectionObj) == LOCXYZ) {
				CON_printf(C_2BB4_040C);
				return;
			}
			if(GetOuterHolder(SelectionObj) != GetOuterHolder(obj)) {
				CON_printf(GetObjectString(SelectionObj));
				CON_printf("\n\nThat container belongs to someone else!\n");
				MUS_PlaySfx(5);
				return;
			}
			if(GetCoordUse(obj) == EQUIP) {
				CON_printf(GetObjectString(SelectionObj));
				CON_printf(C_2BB4_1EAC);
				MUS_PlaySfx(5);
				return;
			}
			if(SelectionObj == obj) {
				CON_printf(C_2BB4_1ECD);
				MUS_PlaySfx(5);
				return;
			}
			if(GetAssoc(obj) == SelectionObj) {
				/*already in it: move it out to the container's holder*/
				CON_printf("out of ");
				CON_printf(GetObjectString(SelectionObj));
				CON_putch('\n');
				if(C_1B09_0D34(itemTyp, GetAssoc(SelectionObj)) >= 0x10) {
					CON_printf(C_2BB4_024E);
					MUS_PlaySfx(5);
					return;
				}
				SubMov(Party[Active], 5);
				if(quanType) {
					qtyCode = QualQuan(GetQual(obj), count);
					if(GetQuan(obj) > qtyCode)
						SubQuan(obj, qtyCode);
					else
						DeleteObj(obj);
					if(AddInvObj(objTyp | objFrm << 10, INVEN, qtyCode, GetAssoc(SelectionObj)) == -1) {
						AddMapObj(objTyp | objFrm << 10, 0, qtyCode, GetX(GetAssoc(SelectionObj)), GetY(GetAssoc(SelectionObj)), MapZ);
						return;
					}
				} else {
					if(InsertObj(obj, GetAssoc(SelectionObj), CONTAINED) == -1) {
						MoveObj(obj, GetX(GetAssoc(SelectionObj)), GetY(GetAssoc(SelectionObj)), MapZ);
						return;
					}
				}
				InvenSize --;
			} else {
				CON_printf("into ");
				CON_printf(GetObjectString(SelectionObj));
				CON_putch('\n');
				if(C_1B09_0D34(itemTyp, SelectionObj) >= 0x10) {
					CON_printf(C_2BB4_0493);
					MUS_PlaySfx(5);
					return;
				}
				if(!CanPutInContainer(SelectionObj, obj)) {
					CON_printf(C_2BB4_1EF7);
					MUS_PlaySfx(5);
					return;
				}
				SubMov(Party[Active], 5);
				if(objTyp == 0x100 && GetType(SelectionObj) == 0xa0) {
					if(!GetFrame(SelectionObj)) {
						SetFrame(SelectionObj, 2);
						DeleteObj(obj);
						StatusDirty = 1;
					} else {
						CON_printf(C_2BB4_1F0D);
						MUS_PlaySfx(5);
					}
					return;
				}
				if(quanType) {
					if(AddInvObj(objTyp | objFrm << 10, CONTAINED, count, SelectionObj) == -1) {
						CON_printf(C_2BB4_1F0D);
						MUS_PlaySfx(5);
					} else if(GetQuan(obj) == count) {
						DeleteObj(obj);
					} else {
						SubQuan(obj, count);
					}
				} else if(itemTyp == 0x100 && ObjShapeType[SelectionObj] == 0xa0) {
					SetFrame(target, 2);
					DeleteObj(SelectionObj);
					StatusDirty = 1;
				} else if(InsertObj(obj, SelectionObj, CONTAINED) == -1) {
					CON_printf(C_2BB4_1F0D);
					MUS_PlaySfx(5);
					return;
				}
				InvenSize --;
			}
		}
		STAT_RefreshInventory(InvenOwner);
	}
	StatusDirty = 0;
	MouseMode = 0;
}

