/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE segment 2077: status, equipment and inventory panel
 * (Ultima VI's status.c, by way of Savage Empire's: SE GAME.EXE 0E13).
 *
 * Draws the status panel (top right of the screen, TextWindows[1]): the
 * party list (up to 5 members, with their talent icons), a character's
 * portrait and statistics, and the equipment / inventory view (8 equipment
 * slots around the character figure, a 4x4 grid of carried objects).
 * Also computes carried weights (in tenths of the unit shown) and fills Equipment[]
 * (object in each slot; 1 in the left hand means "taken by a two-handed
 * weapon"), maps mouse clicks on the panel to the selection, and implements
 * "Ready" / "Unready".
 *
 * Equipment slots: 0 head, 1 neck, 2 right hand, 3 right finger, 4 chest,
 * 5 left hand, 6 left finger, 7 feet; STAT_GetEquipSlot (C_100B_02C0) also
 * returns 8 for a two-handed weapon.
 * Object positions (ObjStatus & 0x18): 0x00 on the map, 0x08 contained,
 * 0x10 in an inventory, 0x18 equipped.
 *
 * Differences from Savage Empire's: the hit points' color has more cases
 * (STAT_PrintHP), the strength/dexterity/intelligence shown are computed
 * (C_07D1_0521/0576/05C0) and colored, the talents (D_16B0) are shown as
 * icons, the empty slot tiles follow the panel color, in the dream
 * (D_16AA & 0x10) only one inventory is shown, readying some objects has
 * side effects, unreadying may be refused, no "modern weapons" rule.
 * STAT_GetEquipSlot and IsWeightPer10Type moved to segment 100B; the
 * inventory scroll stubs and STAT_FindEquipSlotByType are gone.
 */
#include "se.h"

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;
};

/*console*/
extern int16 CON_mouseOffAt(int16, int16, int16, int16);
extern CON_mouseOn(int16);
extern DrawInvObject(int16, int16, int16);/*DrawInvObject(objNum, x, y)*/
extern DrawSlotTile(int16, int16, int16, int16);/*DrawSlotTile(tile, background tile, x, y)*/
extern DrawSlotTile2(int16, int16, int16, int16, int16);/*DrawSlotTile2(tile, middle tile, background tile, x, y)*/
extern CON_setClip(struct tClipInfo *);
extern CON_gotoxy(int16, int16);
extern cdecl CON_printf(char far *, ...);
/*object*/
extern int16 C_1B09_00C3(int16, int16);/*OBJ_MakeDirFrame(type, direction)*/
extern int16 C_1B09_0B1D(int16);/*FindInv*/
extern int16 C_1B09_0B76(void);/*NextInv*/
extern int16 C_1B09_116E(int16, int16, int16, int16, int16, int16);/*AddMapObj*/
extern int16 C_1B09_12EE(int16, int16, int16, int16);/*AddInvObj(shape, position, quantity, owner)*/
extern C_1B09_1872(int16);/*DeleteObj*/
extern int16 C_1B09_1ECF(int16, int16);/*OBJ_IsInside(objNum, container)*/
extern int16 C_1B09_239C(int16, int16, int16);/*InsertObj(objNum, owner, position)*/
extern char *C_1B09_4759(int16);/*GetObjectString*/
/*other modules*/
extern int16 C_07D1_0521(int16);/*an NPC's strength*/
extern int16 C_07D1_0576(int16);/*an NPC's dexterity*/
extern int16 C_07D1_05C0(int16);/*an NPC's intelligence*/
extern int16 C_07D1_064E(int16);/*MaxHP*/
extern int16 C_100B_0186(int16);/*IsContainerType*/
extern int16 C_100B_01FB(int16);
extern int16 C_100B_026E(int16);/*IsWeightPer10Type*/
extern int16 C_100B_0295(int16);/*QuanType: 4 quantity on 16 bits, 2 on 8 bits, else 0*/
extern int16 C_100B_02C0(int16);/*STAT_GetEquipSlot*/
extern C_0E1A_13A8(int16, int16, int16);/*DrawPortrait(npcNum, x, y)*/
extern C_0E1A_186B(int16, int16, int16);/*DrawPaperDoll(x, y, npcNum)*/
extern C_2FA1_0000(int16, int16, int16);
extern char far C_2BB4_0D8D[];
extern char far C_2BB4_137F[];/*BurnOutMsg*/
extern char far C_2BB4_22F1[];

#define OBJ_MakeDirFrame C_1B09_00C3
#define FindInv C_1B09_0B1D
#define NextInv C_1B09_0B76
#define AddMapObj C_1B09_116E
#define AddInvObj C_1B09_12EE
#define DeleteObj C_1B09_1872
#define OBJ_IsInside C_1B09_1ECF
#define InsertObj C_1B09_239C
#define GetObjectString C_1B09_4759
#define MaxHP C_07D1_064E
#define IsContainerType C_100B_0186
#define IsWeightPer10Type C_100B_026E
#define QuanType C_100B_0295
#define STAT_GetEquipSlot C_100B_02C0
#define DrawPortrait C_0E1A_13A8
#define DrawPaperDoll C_0E1A_186B
#define BurnOutMsg C_2BB4_137F

extern int16 LastNumberPlural;
extern int16 StatusMember;/*party index shown in the portrait/equipment view*/
extern int16 StatusDisplay;/*0x92 when the equipment view is shown*/
extern struct tClipInfo *CurClip;/*current text window*/
extern int16 D_0810;/*ColorRed*/
extern int16 D_0814;
extern int16 D_0816;
extern int16 D_081A;/*color set before drawing a portrait*/
extern int16 D_081C;/*ColorPaper*/
extern int16 D_081E;/*ColorText*/
extern int16 D_0826;
extern int16 D_082A;
extern int16 D_082C;
extern char D_0F13;/*PromptFlag?*/
extern int16 D_1680;/*MapZ*/
extern int32 D_16AA;/*flags (0x10: the dream)*/
extern struct { unsigned a:4, psychic:4, c:4, d:4; } D_16B0[];/*talents (shown as tiles 0x120, 0x121, 0x122)*/
extern unsigned char D_3266[];/*ObjStatus*/
extern unsigned char D_4EA4[];/*Party*/
extern unsigned char far *D_4EB6;/*Level*/
extern char D_50CE[][14];/*Names*/
extern uint16 D_51AE[];/*ExpPoints*/
extern unsigned char far *D_53AE;/*NPCComMode*/
extern int16 D_54BA[];/*Amount*/
extern unsigned char D_6CBA[];/*NPCStatus: 0x08 poisoned*/
extern unsigned char D_6DBE[];/*HitPoints*/
extern int16 far *D_6EBE;/*BaseTile*/
extern unsigned char far *D_7606;/*TypeWeight*/
extern struct coord D_78AE[];/*ObjPos*/
extern unsigned char D_9CC0;/*PartySize*/
extern uint16 D_9CC4[];/*ObjShapeType*/
extern int16 far *D_B508;/*OrigShapeType*/
extern union tHandler D_BA6A;/*GrDriver*/
extern int16 D_C59C[];/*Link*/
extern int16 D_EAD8[16];/*InvenSlotObj*/
extern unsigned char D_EAF8;/*InvenSize*/
extern unsigned char D_EAF9[8];/*Party_2Handed*/
extern int16 D_EB00;/*ShowNPCInven*/
extern int16 D_EB02;/*NPC_2Handed*/
extern int16 D_EB04[8];/*Equipment*/
extern int16 D_EB16;/*WeightEquip*/
extern int16 D_EB18;/*InvenOwner*/
extern int16 D_EB1A;/*WeightInven*/
extern int16 D_EB74, D_EB76;/*PointerX, PointerY*/
extern int16 D_EBCE;/*Selection.obj*/
extern struct tClipInfo D_EBD0[4];/*TextWindows*/
extern int16 D_EBF6, D_EBF8;/*Selection.x, Selection.y*/
extern int16 D_167C, D_167E;/*MapX, MapY*/

#define ColorRed D_0810
#define ColorPaper D_081C
#define ColorText D_081E
#define PromptFlag D_0F13
#define MapX D_167C
#define MapY D_167E
#define MapZ D_1680
#define ObjStatus D_3266
#define Party D_4EA4
#define Level D_4EB6
#define Names D_50CE
#define ExpPoints D_51AE
#define NPCComMode D_53AE
#define Amount D_54BA
#define NPCStatus D_6CBA
#define HitPoints D_6DBE
#define BaseTile D_6EBE
#define TypeWeight D_7606
#define ObjPos D_78AE
#define PartySize D_9CC0
#define ObjShapeType D_9CC4
#define OrigShapeType D_B508
#define GrDriver D_BA6A
#define Link D_C59C
#define InvenSlotObj D_EAD8
#define InvenSize D_EAF8
#define Party_2Handed D_EAF9
#define ShowNPCInven D_EB00
#define NPC_2Handed D_EB02
#define Equipment D_EB04
#define WeightEquip D_EB16
#define InvenOwner D_EB18
#define WeightInven D_EB1A
#define PointerX D_EB74
#define PointerY D_EB76
#define SelectionObj D_EBCE
#define TextWindows D_EBD0
#define SelectionX D_EBF6
#define SelectionY D_EBF8

#define GR_SET_COLOR(c)              (GrDriver.iii.ofs = 0x06, (*GrDriver.f)(c))
#define GR_FILL_RECT(x0,y0,x1,y1)    (GrDriver.iii.ofs = 0x12, (*GrDriver.f)(x0,y0,x1,y1))
#define GR_PUT_TILE_SCREEN(tile,x,y) (GrDriver.iii.ofs = 0x2d, (*GrDriver.f)(tile,x,y))

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetShape(i) (GetType(i) | (GetFrame(i) << 10))
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))
#define GetCoordUse(i) (ObjStatus[i] & 0x18)
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetAssoc(i) (*(int16 *)&(ObjPos[i]))
#define IsNPC(i) ((i) < 0x100 && (i) >= 0)
#define IN_DREAM (D_16AA & 0x10)

/*the panel's tiles follow its paper color*/
#define EMPTY_SLOT_TILE (ColorPaper == 7 ? 0x126 : 0x110)
#define SLOT_FRAME_TILE (ColorPaper == 7 ? 0x127 : 0x111)
#define SLOT_2HANDED_TILE (ColorPaper == 7 ? 0x128 : 0x112)
#define PANEL_CORNER_L (ColorPaper == 7 ? 0x12d : 0x124)
#define PANEL_CORNER_R (ColorPaper == 7 ? 0x12e : 0x125)

/*1A8A*/unsigned char PartyListTop = 0;/*first party member shown in the party view*/
/*1A8B*/unsigned char InvenScroll = 0;/*index of the first shown inventory object (multiple of 4)*/

/*combat modes (indexed by NPCComMode - 2)*/
/*1A8C*/char *ComModeNames[] = {
	/*1A9A*/"CMND",
	/*1A9E*/"",
	/*1A9F*/"RANGE",
	/*1A9E*/"",
	/*1AA5*/"WILD",
	/*1AAA*/"FLEE",
	/*1AAF*/"ATTK"
};

/*print an NPC's hit points at the cursor, colored by its state*/
/*C_2077_0003*/STAT_PrintHP(register int16 objNum) {
	if(NPCStatus[objNum] & 2)
		CurClip->fgColor = D_0816;
	if(NPCStatus[objNum] & 8)
		CurClip->fgColor = ColorRed;
	else if(NPCStatus[objNum] & 1)
		CurClip->fgColor = D_082A;
	else if(HitPoints[objNum] < 10)
		CurClip->fgColor = D_082C;
	else if(ObjStatus[objNum] & 0x40)
		CurClip->fgColor = D_0814;
	else if((ObjStatus[objNum] & 0x80) && objNum != 1)
		CurClip->fgColor = D_0826;
	else
		CurClip->fgColor = ColorText;
	CON_printf(/*1AB4*/"%3d\n", HitPoints[objNum]);
	CurClip->fgColor = ColorText;
}

/*display party info: up to 5 members (picture, talents, hit points,
  name); the rows are shifted up by "first"*/
/*C_2077_009D*/STAT_ShowParty(int16 first) {
	register int16 i, objNum;
	int16 mouseState, tile, objTyp;

	mouseState = CON_mouseOffAt(184, 2, 311, 119);
	GR_SET_COLOR(ColorPaper);
	GR_FILL_RECT(184, 2, 311, 119);
	GR_PUT_TILE_SCREEN(PANEL_CORNER_L, 184, 0);
	GR_PUT_TILE_SCREEN(PANEL_CORNER_R, 296, 0);
	CON_setClip(&TextWindows[1]);
	for(i = 0; i < PartySize && i < 5; i ++) {
		objNum = Party[i];
		/*original (not current) shape, facing south*/
		objTyp = OrigShapeType[objNum] & 0x3ff;
		tile = OBJ_MakeDirFrame(objTyp, 4) + BaseTile[objTyp];
		DrawSlotTile(tile, EMPTY_SLOT_TILE, 192, (i - first) * 24 + 8);
		if(D_16B0[objNum].a)
			DrawSlotTile(0x120, EMPTY_SLOT_TILE, 208, (i - first) * 24 + 8);
		if(D_16B0[objNum].psychic) {
			if(D_16B0[objNum].c)
				DrawSlotTile2(0x121, 0x122, EMPTY_SLOT_TILE, 224, (i - first) * 24 + 8);
			else
				DrawSlotTile(0x121, EMPTY_SLOT_TILE, 224, (i - first) * 24 + 8);
		} else if(D_16B0[objNum].c) {
			DrawSlotTile(0x122, EMPTY_SLOT_TILE, 224, (i - first) * 24 + 8);
		}
		CON_gotoxy(12, (i - first) * 3 + 1);
		STAT_PrintHP(objNum);
		CON_gotoxy(9 - strlen(Names[i]) / 2, (i - first) * 3);
		CON_printf(/*1AB9*/"%s\n", Names[i]);
	}
	CON_setClip(&TextWindows[3]);
	CON_mouseOn(mouseState);
}

/*character combat mode: (re)print the combat mode of the displayed
  party member (StatusMember) at the bottom right of the panel ("WILD"
  if ObjStatus 0x80 and not the player)*/
/*C_2077_030B*/STAT_refreshComMode() {
	register int16 objNum;
	register struct tClipInfo *savedClip;
	int16 len, mouseState;

	mouseState = CON_mouseOffAt(184, 2, 311, 119);
	savedClip = CurClip;
	CON_setClip(&TextWindows[1]);
	GR_SET_COLOR(ColorPaper);
	GR_FILL_RECT(264, 104, 303, 111);
	objNum = Party[StatusMember];
	if((ObjStatus[objNum] & 0x80) && objNum != 1) {
		len = strlen(ComModeNames[4]);
		CON_gotoxy(10, 12);
		CON_printf(/*1ABD*/"%s", ComModeNames[4]);
	} else {
		len = strlen(ComModeNames[NPCComMode[objNum] - 2]);
		CON_gotoxy(10, 12);
		CON_printf(/*1ABD*/"%s", ComModeNames[NPCComMode[objNum] - 2]);
	}
	CON_setClip(savedClip);
	CON_mouseOn(mouseState);
}

/*character status view: portrait, attributes, health, level/experience
  of party member #partyIdx, and the panel buttons*/
/*C_2077_03F1*/STAT_ShowPortrait(register int16 partyIdx) {
	register int16 objNum;
	int16 mouseState;

	mouseState = CON_mouseOffAt(184, 2, 311, 119);
	GR_SET_COLOR(ColorPaper);
	GR_FILL_RECT(184, 2, 311, 119);
	GR_PUT_TILE_SCREEN(PANEL_CORNER_L, 184, 0);
	GR_PUT_TILE_SCREEN(PANEL_CORNER_R, 296, 0);
	CON_setClip(&TextWindows[1]);
	CON_gotoxy((17 - strlen(Names[partyIdx])) / 2, 0);
	CON_printf(/*1AB9*/"%s\n", Names[partyIdx]);
	objNum = Party[partyIdx];
	GR_SET_COLOR(D_081A);
	DrawPortrait(objNum, 185, 16);
	CON_gotoxy(10, 1); CON_printf(/*1AC0*/"STR:");
	CON_gotoxy(10, 2); CON_printf(/*1AC5*/"DEX:");
	CON_gotoxy(10, 3); CON_printf(/*1ACA*/"INT:");
	CON_gotoxy(14, 1);
	if(ObjStatus[objNum] & 0x40)
		CurClip->fgColor = D_0814;
	if(objNum <= 15 && D_16B0[objNum].a)
		CurClip->fgColor = D_0816;
	else if(ObjStatus[objNum] & 0x80)
		CurClip->fgColor = D_0826;
	CON_printf(/*1ACF*/"%d\n", C_07D1_0521(objNum));
	CON_gotoxy(14, 2);
	if(ObjStatus[objNum] & 0x40)
		CurClip->fgColor = D_0814;
	else if(ObjStatus[objNum] & 0x80)
		CurClip->fgColor = D_0826;
	else
		CurClip->fgColor = ColorText;
	CON_printf(/*1ACF*/"%d\n", C_07D1_0576(objNum));
	CON_gotoxy(14, 3);
	if(objNum == 1 && !(ObjStatus[objNum] & 0x40) && (ObjStatus[objNum] & 0x80))
		CurClip->fgColor = ColorText;
	CON_printf(/*1ACF*/"%d\n", C_07D1_05C0(objNum));
	CurClip->fgColor = ColorText;
	CON_gotoxy(10, 4); CON_printf(/*1AD3*/"HP:");
	STAT_PrintHP(objNum);
	CON_gotoxy(10, 5); CON_printf(/*1AD7*/"HM:%3d", MaxHP(objNum));
	CON_gotoxy(10, 6); CON_printf(/*1ADE*/"Lev:%2d", Level[objNum]);
	CON_gotoxy(10, 7); CON_printf(/*1AE6*/"Exper:");
	CON_gotoxy(10, 8); CON_printf(/*1AED*/"  %4d", ExpPoints[objNum]);
	if(StatusMember > 0)
		GR_PUT_TILE_SCREEN(0x11a, 184, 102);/*previous member*/
	GR_PUT_TILE_SCREEN(0x117, 200, 102);/*party*/
	GR_PUT_TILE_SCREEN(0x119, 216, 102);/*inventory*/
	if(PartySize - 1 > StatusMember)
		GR_PUT_TILE_SCREEN(0x11b, 232, 102);/*next member*/
	CON_setClip(&TextWindows[3]);
	CON_mouseOn(mouseState);
}

/*weight of an object (not of its content), in tenths*/
/*C_2077_0747*/GetWeight(int16 objNum) {
	register int16 qty, quanType;
	int16 objTyp;

	objTyp = GetType(objNum);
	quanType = QuanType(objTyp);
	if(quanType == 4)
		/*quantity on 16 bits*/
		qty = Amount[objNum];
	else if(quanType == 2)
		/*quantity on 8 bits*/
		qty = GetQuan(objNum);
	else
		qty = 1;
	return GetTypeWeight(objTyp, qty);
}

/*weight of qty objects of type objTyp, in tenths*/
/*C_2077_079D*/GetTypeWeight(register int16 objTyp, int16 qty) {
	register int16 weight;

	if(IsWeightPer10Type(objTyp))
		weight = (TypeWeight[objTyp] * qty) / 10;
	else
		weight = TypeWeight[objTyp] * qty;
	return weight;
}

/*total weight of the objects contained (at any depth) in objNum:
  they follow it in the Link list; 0 for type 0x14b*/
/*C_2077_07D9*/Encumbrance(int16 objNum) {
	register int16 item, weight;

	if(GetType(objNum) == 0x14b)
		return 0;
	weight = 0;
	for(
		item = Link[objNum];
		item >= 0 && GetCoordUse(item) && OBJ_IsInside(item, objNum);
		item = Link[item]
	) weight += GetWeight(item);
	return weight;
}

/*compute InvenSize, WeightInven and (for an NPC) WeightEquip of
  objNum's inventory, without touching Equipment[]*/
/*C_2077_0829*/STAT_ComputeWeights(int16 objNum) {
	register int16 item, partyIdx;
	int16 objTyp;

	InvenSize = 0;
	WeightInven = 0;
	if(IsNPC(objNum))
		WeightEquip = 0;
	/*party index: computed but unused*/
	for(partyIdx = 0; partyIdx < 8; partyIdx ++)
		if(Party[partyIdx] == objNum)
			break;
	for(item = FindInv(objNum); item >= 0; item = NextInv()) {
		objTyp = GetType(item);
		if(GetCoordUse(item) == 0x18) {
			WeightEquip += TypeWeight[objTyp];
		} else {
			WeightInven += GetWeight(item) + Encumbrance(item);
			InvenSize ++;
		}
	}
}

/*same as STAT_ComputeWeights, and also put each equipped object of objNum
  in its Equipment[] slot (Equipment[] must have been cleared)*/
/*C_2077_08C4*/STAT_ComputeEquipment(int16 objNum) {
	register int16 item, partyIdx;
	int16 slot, objTyp;

	InvenSize = 0;
	WeightInven = 0;
	if(IsNPC(objNum))
		WeightEquip = 0;
	for(partyIdx = 0; partyIdx < 8; partyIdx ++) {
		if(Party[partyIdx] == objNum)
			break;
	}
	for(item = FindInv(objNum); item >= 0; item = NextInv()) {
		objTyp = GetType(item);
		if(GetCoordUse(item) == 0x18) {
			WeightEquip += TypeWeight[objTyp];
			slot = STAT_GetEquipSlot(item);
			if(slot == 8) {
				/*two-handed: right hand, left hand marked as taken*/
				slot = 2;
				Equipment[5] = 1;
				if(!ShowNPCInven && partyIdx < 7)
					Party_2Handed[partyIdx] = 1;
				else
					NPC_2Handed = 1;
			} else if(slot == 2 && Equipment[2]) {
				slot = 5;
			} else if(slot == 5 && Equipment[5]) {
				slot = 2;
			}
			Equipment[slot] = item;
		} else {
			WeightInven += GetWeight(item) + Encumbrance(item);
			InvenSize ++;
		}
	}
}

/*draw equipment: the 8 slots of Equipment[] (empty slot frame if
  nothing; left hand "taken" if a two-handed weapon is readied),
  shifted by (x, y)*/
/*C_2077_09C7*/STAT_DrawEquipment(int16 x, register int16 y) {
	register int16 objNum;
	int16 slot;

	for(slot = 0; slot < 8; slot ++) {
		if((objNum = Equipment[slot]) != 0) {
			switch(slot) {
				case 0: DrawSlotTile(TILE_FRAME(objNum), EMPTY_SLOT_TILE, x + 208, y + 16); break;
				case 1: DrawSlotTile(TILE_FRAME(objNum), EMPTY_SLOT_TILE, x + 184, y + 24); break;
				case 4: DrawSlotTile(TILE_FRAME(objNum), EMPTY_SLOT_TILE, x + 232, y + 24); break;
				case 2: DrawSlotTile(TILE_FRAME(objNum), EMPTY_SLOT_TILE, x + 184, y + 40); break;
				case 5:
					if((!ShowNPCInven && Party_2Handed[StatusMember]) || (ShowNPCInven && NPC_2Handed))
						GR_PUT_TILE_SCREEN(SLOT_2HANDED_TILE, x + 232, y + 40);
					else
						DrawSlotTile(TILE_FRAME(objNum), EMPTY_SLOT_TILE, x + 232, y + 40);
				break;
				case 3: DrawSlotTile(TILE_FRAME(objNum), EMPTY_SLOT_TILE, x + 184, y + 56); break;
				case 6: DrawSlotTile(TILE_FRAME(objNum), EMPTY_SLOT_TILE, x + 232, y + 56); break;
				case 7: DrawSlotTile(TILE_FRAME(objNum), EMPTY_SLOT_TILE, x + 208, y + 64); break;
			}
		} else {
			switch(slot) {
				case 0: GR_PUT_TILE_SCREEN(SLOT_FRAME_TILE, x + 208, y + 16); break;
				case 1: GR_PUT_TILE_SCREEN(SLOT_FRAME_TILE, x + 184, y + 24); break;
				case 4: GR_PUT_TILE_SCREEN(SLOT_FRAME_TILE, x + 232, y + 24); break;
				case 2: GR_PUT_TILE_SCREEN(SLOT_FRAME_TILE, x + 184, y + 40); break;
				case 5:
					if((!ShowNPCInven && Party_2Handed[StatusMember]) || (ShowNPCInven && NPC_2Handed))
						GR_PUT_TILE_SCREEN(SLOT_2HANDED_TILE, x + 232, y + 40);
					else
						GR_PUT_TILE_SCREEN(SLOT_FRAME_TILE, x + 232, y + 40);
				break;
				case 3: GR_PUT_TILE_SCREEN(SLOT_FRAME_TILE, x + 184, y + 56); break;
				case 6: GR_PUT_TILE_SCREEN(SLOT_FRAME_TILE, x + 232, y + 56); break;
				case 7: GR_PUT_TILE_SCREEN(SLOT_FRAME_TILE, x + 208, y + 64); break;
			}
		}
	}
}

/*tenths -> displayed units: rounded to nearest, but anything below 1
  shows as 1*/
/*C_2077_0DF6*/static WeightToStones(int16 tenths) {
	register int16 rest, stones;

	stones = tenths / 10;
	rest = tenths % 10;
	if(stones)
		return stones + (rest >= 5);
	else
		return stones + (rest != 0);
}

/*draw the equipment and inventory of InvenOwner (16 objects, recorded
  in InvenSlotObj), the owner's picture and the equipped/total weights
  vs capacity*/
/*C_2077_0E34*/STAT_DrawInventory() {
	register int16 slot, item;
	int16 mouseState, objTyp, tile, owner;
	struct tClipInfo *savedClip;

	if(StatusDisplay != 0x92)
		return;
	mouseState = CON_mouseOffAt(184, 2, 311, 119);
	savedClip = CurClip;
	CON_setClip(&TextWindows[1]);
	GR_SET_COLOR(ColorPaper);
	GR_FILL_RECT(248, 16, 311, 95);
	STAT_DrawEquipment(0, 0);
	item = FindInv(InvenOwner);
	slot = 0;
	/*the last row became empty: scroll back one row*/
	if(InvenScroll >= 4 && InvenScroll + 12 >= InvenSize)
		InvenScroll -= 4;
	/*skip the scrolled-out objects*/
	while(InvenScroll > slot) {
		if(GetCoordUse(item) != 0x18 || GetCoordUse(item) == 8)
			slot ++;
		item = NextInv();
	}
	/*owner's picture (an NPC facing south, or the container)*/
	objTyp = GetType(InvenOwner);
	if(InvenOwner == 0) {
		objTyp = OrigShapeType[0] & 0x3ff;
		if(objTyp == 0x157 || objTyp == 0x158)
			tile = BaseTile[objTyp] + OBJ_MakeDirFrame(objTyp, 4);
		else
			tile = BaseTile[objTyp] + 2;
		DrawSlotTile(tile, EMPTY_SLOT_TILE, 272, 16);
	} else if(IsNPC(InvenOwner)) {
		objTyp = OrigShapeType[InvenOwner] & 0x3ff;
		tile = OBJ_MakeDirFrame(objTyp, 4) + BaseTile[objTyp];
		DrawSlotTile(tile, EMPTY_SLOT_TILE, 272, 16);
	} else {
		DrawSlotTile(TILE_FRAME(InvenOwner), EMPTY_SLOT_TILE, 272, 16);
	}
	/*the 4x4 grid*/
	for(slot = 0; item >= 0 && slot < 16; item = NextInv()) {
		if(GetCoordUse(item) != 0x18 || GetCoordUse(item) == 8) {
			DrawInvObject(item, 248 + TIL2SCR(slot & 3), 32 + TIL2SCR(slot >> 2));
			InvenSlotObj[slot++] = item;
		}
	}
	for( ;slot < 16; slot ++) {
		InvenSlotObj[slot] = 0;
		DrawSlotTile(SLOT_FRAME_TILE, EMPTY_SLOT_TILE, 248 + TIL2SCR(slot & 3), 32 + TIL2SCR(slot >> 2));
	}
	/*the NPC carrying it all (InvenOwner may be a container)*/
	owner = InvenOwner;
	if(!IsNPC(owner)) {
		do
			owner = GetAssoc(owner);
		while(!IsNPC(owner));
	}
	/*display equipped weight (capacity: strength)*/
	CON_gotoxy(2, 9);
	CON_printf(/*1AF3*/"      ");
	CON_gotoxy(2, 9);
	CON_printf(/*1AFA*/"%d%/%ds", WeightToStones(WeightEquip), C_07D1_0521(owner));
	/*display inventory weight (total, capacity: 2 x strength)*/
	CON_gotoxy(2, 10);
	CON_printf(/*1AF3*/"      ");
	CON_gotoxy(2, 10);
	CON_printf(/*1AFA*/"%d%/%ds", WeightToStones(WeightInven + WeightEquip), C_07D1_0521(owner) << 1);
	/* */
	CON_setClip(savedClip);
	CON_mouseOn(mouseState);
}

/*recompute Equipment[] and weights of objNum and redraw the
  inventory panel if it shows objNum or objNum is a container*/
/*C_2077_1179*/STAT_RefreshInventory(register int16 objNum) {
	register int16 i;

	for(i = 0; i < 16; i ++)
		InvenSlotObj[i] = 0;
	if(IsNPC(objNum)) {
		for(i = 0; i < 8; i ++)
			Equipment[i] = 0;
	}
	STAT_ComputeEquipment(objNum);
	if((StatusDisplay == 0x92 && Party[StatusMember] == objNum) || IsContainerType(GetType(objNum)))
		STAT_DrawInventory();
}

/*character equipment view: of party member StatusMember, or of NPC
  ShowNPCInven if not 0, with the panel buttons; in the dream, of the
  player only, with the talents and hit points instead of the buttons*/
/*C_2077_11ED*/STAT_ShowInventory(register int16 partyIdx) {
	register int16 mouseState;

	InvenScroll = 0;
	if(IN_DREAM) {
		partyIdx = PartySize;
		StatusMember = PartySize;
	}
	mouseState = CON_mouseOffAt(184, 2, 311, 119);
	GR_SET_COLOR(ColorPaper);
	GR_FILL_RECT(184, 2, 311, 119);
	GR_PUT_TILE_SCREEN(PANEL_CORNER_L, 184, 0);
	GR_PUT_TILE_SCREEN(PANEL_CORNER_R, 296, 0);
	CON_setClip(&TextWindows[1]);
	if(!ShowNPCInven) {
		CON_gotoxy((17 - strlen(Names[StatusMember])) / 2, 0);
		CON_printf(/*1AB9*/"%s\n", Names[StatusMember]);
	}
	DrawPaperDoll(200, 32, Party[StatusMember]);
	CON_gotoxy(0, 9); CON_printf(/*1B02*/"E:");
	CON_gotoxy(0, 10); CON_printf(/*1B05*/"I:");
	if(!ShowNPCInven && !IN_DREAM) {
		if(StatusMember > 0)
			GR_PUT_TILE_SCREEN(0x11a, 184, 102);/*previous member*/
		GR_PUT_TILE_SCREEN(0x117, 200, 102);/*party*/
		GR_PUT_TILE_SCREEN(0x118, 216, 102);/*portrait*/
		if(PartySize - 1 > StatusMember)
			GR_PUT_TILE_SCREEN(0x11b, 232, 102);/*next member*/
		/*combat mode button: active unless the player or ObjStatus 0x80*/
		if(Party[partyIdx] != 1 && !(ObjStatus[Party[partyIdx]] & 0x80))
			GR_PUT_TILE_SCREEN(0x11d, 248, 102);
		else
			GR_PUT_TILE_SCREEN(0x11c, 248, 102);
		GR_PUT_TILE_SCREEN(0x11e, 264, 102);
		GR_PUT_TILE_SCREEN(0x11e, 280, 102);
		GR_PUT_TILE_SCREEN(0x11f, 296, 102);
	}
	if(IN_DREAM && !ShowNPCInven) {
		if(D_16B0[0].a)
			DrawSlotTile(0x120, EMPTY_SLOT_TILE, 184, 96);
		if(D_16B0[0].psychic) {
			if(D_16B0[0].c)
				DrawSlotTile2(0x121, 0x122, EMPTY_SLOT_TILE, 200, 96);
			else
				DrawSlotTile(0x121, EMPTY_SLOT_TILE, 200, 96);
		} else if(D_16B0[0].c) {
			DrawSlotTile(0x122, EMPTY_SLOT_TILE, 200, 96);
		}
		CON_gotoxy(0, 11); CON_printf(/*1AD3*/"HP:");
		CON_gotoxy(4, 11);
		STAT_PrintHP(0);
	}
	if(!ShowNPCInven)
		InvenOwner = Party[StatusMember];
	else
		InvenOwner = ShowNPCInven;
	Party_2Handed[StatusMember] = 0;
	STAT_RefreshInventory(InvenOwner);
	if(!IN_DREAM)
		STAT_refreshComMode();
	CON_setClip(&TextWindows[3]);
	CON_mouseOn(mouseState);
}

/*mouse on the inventory grid or the owner's picture?
  sets the selection (obj: object or -1, x: grid slot + 1 / 0 for a
  container owner / map position for an NPC owner); returns 1 if hit*/
/*C_2077_1572*/STAT_PickInvenSlot() {
	register int16 slot;

	if(
		PointerX >= 248 && PointerX < 312 &&
		PointerY >= 32 && PointerY < 96
	) {
		slot = SCR2TIL(PointerX - 248) + (SCR2TIL(PointerY - 32) << 2);
		SelectionObj = InvenSlotObj[slot];
		if(SelectionObj == 0)
			SelectionObj = -1;
		SelectionX = slot + 1;
		return 1;
	}
	if(PointerX >= 272 && PointerX < 288 && PointerY >= 16 && PointerY < 32) {
		if(!IsNPC(InvenOwner)) {
			SelectionX = 0;
		} else {
			SelectionX = MapX;
			SelectionY = MapY;
		}
		SelectionObj = InvenOwner;
		return 1;
	}
	SelectionObj = -1;
	return 0;
}

/*mouse on an equipment slot? sets selection x = slot and obj = object
  (-1 if empty or taken by a two-handed weapon); returns 1 if hit*/
/*C_2077_1623*/STAT_PickEquipSlot() {
	if(PointerX >= 208 && PointerX < 224 && PointerY >= 16 && PointerY < 32) {
		SelectionX = 0;
		SelectionObj = Equipment[0];
	} else if(PointerX >= 184 && PointerX < 200 && PointerY >= 24 && PointerY < 72) {
		/*left column: neck, right hand, right finger*/
		SelectionX = SCR2TIL(PointerY - 24) + 1;
		SelectionObj = Equipment[SelectionX];
	} else if(PointerX >= 232 && PointerX < 248 && PointerY >= 24 && PointerY < 72) {
		/*right column: chest, left hand, left finger*/
		SelectionX = SCR2TIL(PointerY - 24) + 4;
		SelectionObj = Equipment[SelectionX];
	} else if(PointerX >= 208 && PointerX < 224 && PointerY >= 64 && PointerY < 96) {
		SelectionX = 7;
		SelectionObj = Equipment[7];
	} else {
		SelectionObj = -1;
		return 0;
	}
	if(SelectionObj == 0 || SelectionObj == 1)
		SelectionObj = -1;
	return 1;
}

/*slot where objNum is equipped, or 9*/
/*C_2077_1705*/STAT_FindEquipSlot(int16 objNum) {
	register int16 i;

	for(i = 0; i < 8; i++) {
		if(Equipment[i] == objNum)
			return i;
	}

	return 9;
}

/*"Ready": equip the selected object on the displayed party member*/
/*C_2077_172C*/STAT_Ready() {
	register int16 slot, owner;
	int16 newObj, objTyp, strength, npcNum, qty;

	npcNum = Party[StatusMember];
	objTyp = GetShape(SelectionObj);
	strength = C_07D1_0521(npcNum);
	LastNumberPlural = 0;
	PromptFlag = 1;
	CON_printf(/*1B08*/"Ready-");
	if(objTyp == 0x6d)
		CON_printf(/*1B0F*/"torch\n");
	else
		CON_printf(/*1AB9*/"%s\n", GetObjectString(SelectionObj));
	if((slot = STAT_GetEquipSlot(SelectionObj)) == -1) {
		CON_printf(/*1B16*/"\nCan't be readied!\n");
	} else {
		if(slot == 8) {
			if(Equipment[2]) {
				slot = 2;
			} else if(Equipment[5]) {
				slot = 5;
			} else {
				slot = 2;
				Equipment[5] = 1;
				Party_2Handed[StatusMember] = 1;
			}
		} else if(slot == 2 && Equipment[2] && Equipment[5] == 0) {
			slot = 5;
		} else if(slot == 5 && Equipment[5] && Equipment[2] == 0) {
			slot = 2;
		}
		if(Equipment[slot]) {
			CON_printf(/*1B2A*/"\nNo place to put!\n");
		} else {
			/*the NPC carrying the object*/
			owner = SelectionObj;
			do
				owner = GetAssoc(owner);
			while(!IsNPC(owner));
			if(QuanType(objTyp)) {
				/*take one from the stack*/
				qty = (((unsigned char *)(Amount + SelectionObj))[1] << 8) | 1;
				if(GetQuan(SelectionObj) > 1)
					GetQuan(SelectionObj) -= 1;
				else
					DeleteObj(SelectionObj);
				Equipment[slot] = AddInvObj(objTyp, 0x18, qty, owner);
				if(Equipment[slot] >= 0) {
					InvenSize --;
				} else if(slot == 2 && Party_2Handed[StatusMember]) {
					Party_2Handed[StatusMember] = 0;
					Equipment[5] = 0;
				}
			} else {
				if(objTyp == 0xa1)
					ObjShapeType[SelectionObj] = (ObjShapeType[SelectionObj] & 0xfc00) + 0xa2;
				Equipment[slot] = InsertObj(SelectionObj, owner, 0x18);
				if(Equipment[slot] >= 0) {
					InvenSize --;
					if(objTyp == 0x88) {
						CON_printf(C_2BB4_0D8D);
					} else if(objTyp == 0xf && MapZ == 2) {
						newObj = AddMapObj(0x92, 0, 0, 0xc9, 0x9b, MapZ);
						if(newObj < 0)
							CON_printf(C_2BB4_22F1);
						else
							ObjStatus[newObj] &= 0xdf;
					}
				} else if(slot == 2 && Party_2Handed[StatusMember]) {
					Party_2Handed[StatusMember] = 0;
					Equipment[5] = 0;
				}
			}
			STAT_RefreshInventory(Party[StatusMember]);
		}
	}
	PromptFlag = 1;
}

/*can equipped objNum be removed? 2 if it was a lit torch (now burnt
  out and deleted), 1 if it won't come off (type 0xf), 0 otherwise
  (npcNum is ignored)*/
/*C_2077_19CD*/STAT_CheckUnready(register int16 objNum, int16 npcNum) {
	register int16 objTyp;
	int16 shape, frame;

	if(GetCoordUse(objNum) != 0x18)
		return 0;
	objTyp = GetType(objNum);
	frame = GetFrame(objNum);
	shape = objTyp | (frame << 10);
	if(shape == 0x6e) {
		DeleteObj(objNum);
		CON_printf(/*1AB7*/"\n");
		CON_printf(BurnOutMsg);
		return 2;
	}
	if(C_100B_01FB(objTyp))
		C_2FA1_0000(objNum, objNum, -1);
	else if(objTyp == 0xf) {
		CON_printf(/*1B3D*/"They won't come off!\n");
		return 1;
	}
	return 0;
}

/*stop the effect of equipped objNum worn by npcNum: none*/
/*C_2077_1A6F*/STAT_StopEquipEffect(int16 objNum, int16 npcNum) {
	return 0;
}

/*"Unready": put the selected equipped object of the displayed party
  member back in the inventory*/
/*C_2077_1A78*/STAT_Unready() {
	register int16 npcNum, result;
	int16 objTyp;

	result = 0;
	if(SelectionObj == 1)
		return;
	CON_printf(/*1B53*/"Unready-%s\n", GetObjectString(SelectionObj));
	objTyp = GetShape(SelectionObj);
	npcNum = Party[StatusMember];
	result = STAT_CheckUnready(SelectionObj, npcNum);
	if(result == 1)
		return;
	if(result != 2) {
		if(InsertObj(SelectionObj, npcNum, 0x10) == -1) {
			CON_printf(/*1B5F*/"There is no room left in your pack.\n");
			return;
		}
	}
	STAT_StopEquipEffect(SelectionObj, npcNum);
	Equipment[STAT_FindEquipSlot(SelectionObj)] = 0;
	if(STAT_GetEquipSlot(SelectionObj) == 8) {
		Party_2Handed[StatusMember] = 0;
		Equipment[5] = 0;
	}
	STAT_RefreshInventory(Party[StatusMember]);
	PromptFlag = 1;
}
