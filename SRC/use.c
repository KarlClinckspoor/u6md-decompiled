/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE segment 2597: the "use" command (CmdUse) and
 * its table of object combinations, new code: Savage Empire's CmdUse
 * (objcmd.c C_1DBD_8276) is a long chain of tests by type, MD's looks the
 * used object's type up in two far tables.
 *
 * UseObjects (far 2E08) gives, for an object type, up to 10 actions
 * (indexes into UseActions, 0xff ends; several rows may name the same
 * type). UseActions (far 2E67) gives, for each action, the types allowed
 * as the second object ("On - ") and the third ("To -"), -1 ending the
 * list (-1 alone: no such object; -2: any object, or a map tile), and the
 * function called with (object, second, third). UseObject asks for the
 * second and third objects while some action still wants them, keeps the
 * actions that accept what was chosen and calls the most specific one.
 */
#include "se.h"

typedef void (*tUseFunc)(int16, int16, int16);

struct tUseObject/*size:0xc*/ {
	int16 type;/*< 0 ends the table*/
	unsigned char actions[10];/*0xff ends*/
};

struct tUseAction/*size:0x24*/ {
	int16 on[8];/*types of the second object*/
	int16 to[8];/*types of the third object*/
	tUseFunc func;
};

/*other modules*/
extern int16 C_047A_04DA(int16);/*FindNonIgnoredObjAt*/
extern C_047A_0416(int16);/*show a tile in the text window?*/
extern int16 C_07D1_298A(int16);/*COMBAT_getHead*/
extern int16 C_07D1_35A8(int16, int16, int16, int16);/*distance of (x1, y1) and (x2, y2)*/
extern C_120E_0221();/*DisplayTick*/
extern C_170D_083F(int16, int16);/*UpdateMoveFrame(npc, direction)*/
extern C_170D_0868(int16, int16);/*SubMov(npc, points)*/
extern char *C_1B09_46E4(int16);/*GetTileString*/
extern char *C_1B09_4759(int16);/*GetObjectString*/
extern C_1157_0312();/*VIS_UpdateView*/
extern unsigned char C_1534_1072(int16, int16, int16);/*GetTileAtXYZ*/
extern int16 C_1534_1129(int16, int16);/*MapDistance*/
extern C_1FA9_0919(int16);/*play a sound effect?*/
extern C_2238_1D17(int16);/*DrawSlotCursor*/
extern int16 C_2238_0062(int16, int16);/*MkDirection*/
extern int16 CON_getch();
extern CON_putch(int16);
extern cdecl CON_printf(char far *, ...);

extern char far C_2BB4_01C0[];/*"On - "*/
extern char far C_2BB4_02A4[];/*"It has no effect.\n"*/
extern char far C_2BB4_03A6[];/*"Out of range!\n"*/
extern char far C_2BB4_0402[];/*"nothing!\n"*/
extern char far C_2BB4_1C19[];/*"%s can't be used.\n"*/
extern char far C_2BB4_1C2C[];/*"%s has no effect on "*/
extern char far C_2BB4_1C41[];/*"%s has no effect.\n"*/
extern struct tUseObject far C_2E08_0000[];/*UseObjects*/
extern struct tUseAction far C_2E67_0000[];/*UseActions*/

extern unsigned char D_167A;/*Active*/
extern int16 D_1680;/*MapZ*/
extern struct { unsigned a:4, psychic:4, c:4, d:4; } D_16B0[];/*talents*/
extern unsigned char SelectMode;
extern char AimX, AimY, SelectRange;
extern int16 EndPagingAtPrompt;
extern int16 MouseMode;
extern struct { unsigned char originX, originY, cols, rows, curX, curY, fgColor, bgColor, attrFlags; } *CurClip;
extern unsigned char D_3266[];/*ObjStatus*/
extern unsigned char D_4EA4[];/*Party*/
extern unsigned char far *D_4EB6;/*Level*/
extern int16 far *D_6EBE;/*BaseTile*/
extern unsigned char far *D_760A;/*TileFlag2: 0x10 ignored*/
extern struct coord D_78AE[];/*ObjPos*/
extern uint16 D_9CC4[];/*ObjShapeType*/
extern int16 D_EB74, D_EB76;/*PointerX, PointerY*/
extern int16 D_EBCE;/*Selection.obj*/
extern int16 D_EBF6, D_EBF8;/*Selection.x, Selection.y*/
extern int16 D_EC46;/*distance of the last target*/

#define Active D_167A
#define MapZ D_1680
#define Party D_4EA4
#define Level D_4EB6
#define ObjStatus D_3266
#define BaseTile D_6EBE
#define TileFlag2 D_760A
#define ObjPos D_78AE
#define ObjShapeType D_9CC4
#define PointerX D_EB74
#define PointerY D_EB76
#define SelectionObj D_EBCE
#define SelectionX D_EBF6
#define SelectionY D_EBF8
#define TargetDist D_EC46
#define UseObjects C_2E08_0000
#define UseActions C_2E67_0000
#define FindNonIgnoredObjAt C_047A_04DA
#define COMBAT_getHead C_07D1_298A
#define DisplayTick C_120E_0221
#define UpdateMoveFrame C_170D_083F
#define SubMov C_170D_0868
#define GetTileString C_1B09_46E4
#define GetObjectString C_1B09_4759
#define VIS_UpdateView C_1157_0312
#define GetTileAtXYZ C_1534_1072
#define MapDistance C_1534_1129
#define DrawSlotCursor C_2238_1D17
#define MkDirection C_2238_0062
#define OnMsg C_2BB4_01C0
#define NoEffectMsg C_2BB4_02A4
#define OutOfRangeMsg C_2BB4_03A6
#define NothingMsg C_2BB4_0402

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))
#define IsTileIg(t) (TileFlag2[t] & 0x10)
#define GetCoordUse(i) (ObjStatus[i] & 0x18)
/*the active member's reach: its level when its first talent is set*/
#define REACH (D_16B0[Party[Active]].a ? Level[Party[Active]] : 0)

/*let the player aim at a second/third object from (x, y): returns it, -1
  for a map tile (not allowed with noTile), -2 when cancelled, out of
  range or not allowed; the distance goes to TargetDist*/
/*C_2597_0002*/GetUseTarget(int16 x, register int16 y, int16 noTile) {
	register int16 ch;

	DrawSlotCursor(0);
	SelectMode = 1;
	AimX =
	AimY = 5;
	if(REACH)
		SelectRange = REACH + 1;
	else
		SelectRange = -1;
	MouseMode = 1;
	ch = CON_getch();
	MouseMode = 0;
	if(ch != 0x8e) {
		CON_printf(NothingMsg);
		return -2;
	}
	if(SelectionObj != -1 && IsTileIg(TILE_FRAME(SelectionObj)))
		SelectionObj = FindNonIgnoredObjAt(SelectionObj);
	if(SelectionObj == -1 && (SelectionX < 0 || SelectionY < 0)) {
		CON_printf(NothingMsg);
		return -2;
	}
	if(SelectionObj == -1) {
		CON_printf(GetTileString(GetTileAtXYZ(SelectionX, SelectionY, MapZ)));
		CON_putch('\n');
		if(noTile) {
			CON_printf(NoEffectMsg);
			C_1FA9_0919(5);
			return -2;
		}
		TargetDist = C_07D1_35A8(SelectionX, SelectionY, x, y);
	} else {
		CON_printf(GetObjectString(SelectionObj));
		CON_putch('\n');
		if(GetCoordUse(SelectionObj) == 0)
			TargetDist = C_07D1_35A8(SelectionX, SelectionY, x, y);
		else
			TargetDist = 1;
	}
	if(REACH + 1 < TargetDist && (MapDistance(SelectionX, x) > 1 || MapDistance(SelectionY, y) > 1)) {
		CON_printf(OutOfRangeMsg);
		C_1FA9_0919(5);
		return -2;
	}
	return SelectionObj;
}

/*print prompt and aim from the active member (returns as GetUseTarget,
  through AX: no return statement)*/
/*C_2597_022F*/PromptUseTarget(char far *prompt) {
	register int16 objNum;

	CON_printf(prompt);
	objNum = GetUseTarget(GetX(Party[Active]), GetY(Party[Active]), 0);
}

/*remove list[idx]: the last entry (of at most 11) takes its place*/
/*C_2597_028A*/ListRemove(int16 idx, register int16 *list) {
	register int16 i;

	for(i = 0; list[i + 1] != -1 && i < 10; i ++);
	list[idx] = list[i];
	list[i] = -1;
}

/*the actions of the type in list (no duplicates); returns their count*/
/*C_2597_02C1*/GetUseActions(int16 type, int16 *list) {
	int16 i = 0, j = 0, n = 0;

	for(; UseObjects[i].type >= 0; i ++) {
		if(UseObjects[i].type == type) {
			for(j = 0; UseObjects[i].actions[j] != 0xff && n < 10; j ++)
				list[n ++] = UseObjects[i].actions[j];
		}
	}
	for(i = 0; i < n; i ++) {
		for(j = i + 1; j < n; ) {
			if(list[i] == list[j]) {
				ListRemove(j, list);
				n --;
			} else {
				j ++;
			}
		}
	}
	return n;
}

/*does an action of list want a second (which 1) or third (2) object?*/
/*C_2597_038E*/WantsTarget(int16 which, int16 *list) {
	int16 found = 0, i = 0;

	while(list[i] != -1 && !found) {
		if((UseActions[list[i]].on[0] != -1 && which == 1) || (UseActions[list[i]].to[0] != -1 && which == 2))
			found = 1;
		else
			i ++;
	}
	return found;
}

/*does the type list accept type?*/
/*C_2597_0403*/TypeInList(int16 type, int16 far *list) {
	int16 i = 0, found = 0;

	if(list[0] == -2)
		return 1;
	if(list[0] == -1 && type == -1)
		return 1;
	while(list[i] != -1 && !found) {
		if(list[i] == type)
			found = 1;
		else
			i ++;
	}
	return found;
}

/*keep the actions of list that accept type as their second (which 1) or
  third (2) object; 0 if none is left*/
/*C_2597_045D*/FilterActions(int16 type, int16 which, register int16 *list) {
	register int16 i = 0;
	int16 far *types;

	while(list[i] != -1) {
		if(which == 1)
			types = UseActions[list[i]].on;
		else
			types = UseActions[list[i]].to;
		if(!TypeInList(type, types))
			ListRemove(i, list);
		else
			i ++;
	}
	if(list[0] == -1)
		return 0;
	return 1;
}

/*how specific a type list is: 0 any (-2), 1 none (-1), 2 types*/
/*C_2597_04D7*/Specificity(int16 type) {
	register int16 n;

	switch(type) {
		case -2: n = 0; break;
		case -1: n = 1; break;
		default: n = 2;
	}
	return n;
}

/*the function of the most specific action of list (the last of equals)*/
/*C_2597_04FD*/tUseFunc BestAction(int16 *list) {
	int16 best, bestScore;
	register int16 i, score;

	i = 0;
	best = -1;
	bestScore = -1;
	for(; list[i] != -1; i ++) {
		score = 0;
		score += Specificity(UseActions[list[i]].on[0]);
		score += Specificity(UseActions[list[i]].to[0]);
		if(score >= bestScore) {
			bestScore = score;
			best = i;
		}
	}
	return UseActions[list[best]].func;
}

/*use objNum (the type of SelectionObj): ask for the other objects the
  actions want, then call the chosen action*/
/*C_2597_059D*/UseObject(int16 objNum) {
	int16 type, onType, toType, unused, i;
	tUseFunc func;
	int16 list[15];
	register int16 on, to;

	unused = 0;
	for(i = 0; i < 15; i ++)
		list[i] = -1;
	type = GetType(SelectionObj);
	on = -1;
	to = -1;
	if(!GetUseActions(type, list)) {
		CON_printf(C_2BB4_1C19, GetObjectString(objNum));
		return;
	}
	if(WantsTarget(1, list)) {
		on = PromptUseTarget(OnMsg);
		if(on == -2)
			return;
		onType = GetType(on);
		if(!FilterActions(onType, 1, list)) {
			CON_printf("\n");
			C_047A_0416(TILE_FRAME(objNum));
			CON_printf(C_2BB4_1C2C, GetObjectString(objNum));
			C_047A_0416(TILE_FRAME(on));
			CON_printf("%s.\n", GetObjectString(on));
			return;
		}
		if(TargetDist > 1)
			C_1FA9_0919(0x32);
		if(WantsTarget(2, list)) {
			to = PromptUseTarget("To -\n");
			if(to == -2)
				return;
			toType = GetType(to);
			if(!FilterActions(toType, 2, list)) {
				CON_printf("\n");
				C_047A_0416(TILE_FRAME(objNum));
				CON_printf("%s on ", GetObjectString(objNum));
				C_047A_0416(TILE_FRAME(on));
				CON_printf("%s to ", GetObjectString(on));
				C_047A_0416(TILE_FRAME(to));
				CON_printf(C_2BB4_1C41, GetObjectString(to));
				return;
			}
			if(TargetDist > 1)
				C_1FA9_0919(0x32);
		}
	}
	func = BestAction(list);
	CON_printf("\n");
	if(func)
		(*func)(objNum, on, to);
}

/*the "use" command on SelectionObj*/
/*C_2597_083C*/CmdUse() {
	int16 type, dir, x;
	register int16 y, dist;

	CurClip->attrFlags |= 0x10;
	SelectionObj = COMBAT_getHead(SelectionObj);
	type = GetType(SelectionObj);
	x = GetX(Party[Active]);
	y = GetY(Party[Active]);
	if((SelectionObj < 0x100 && SelectionObj >= 0 && SelectionX != -1) || IsTileIg(TILE_FRAME(SelectionObj)))
		SelectionObj = FindNonIgnoredObjAt(SelectionObj);
	if(SelectionObj == -1) {
		CON_printf(NothingMsg);
		EndPagingAtPrompt = 1;
		return;
	}
	SelectionObj = COMBAT_getHead(SelectionObj);
	CON_printf(GetObjectString(SelectionObj));
	CON_putch('\n');
	if(GetCoordUse(SelectionObj) == 0) {
		dist = C_07D1_35A8(x, y, SelectionX, SelectionY);
		if(REACH + 1 < dist) {
			CON_printf(OutOfRangeMsg);
			return;
		}
		if(dist > 1)
			C_1FA9_0919(0x32);
	}
	if(GetCoordUse(SelectionObj) == 0) {
		dir = MkDirection(PointerX, PointerY);
		if(dir != -1) {
			UpdateMoveFrame(Party[Active], dir);
			VIS_UpdateView();
		}
		x = GetX(Party[Active]);
		y = GetY(Party[Active]);
	}
	UseObject(SelectionObj);
	SubMov(Party[Active], 5);
	EndPagingAtPrompt = 1;
	VIS_UpdateView();
	DisplayTick();
}
