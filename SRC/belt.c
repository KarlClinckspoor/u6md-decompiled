/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE overlay 2F52: the conveyor belts carrying coal,
 * new code with no Savage Empire counterpart.
 *
 * While the belts run (TalkFlags[0x73] & 4) each conveyor belt object
 * (type 0xbc) in the area moves what lies on it one tile: belts with
 * quality bit 0 set westwards, the others eastwards (these are listed in
 * barge_scrap first, and moved in reverse order). The belt piece at the
 * belt's start (frame 2 westwards, 0 eastwards) puts a lump of coal (type
 * 0xbd) on the belt every fourth turn (its quantity counts down), the
 * piece at the end (the other frame) takes the coal off. NPCs are carried
 * along, and the view follows the party leader. With TalkFlags[0x71] & 8
 * the belt can stop at random (and the machinery with it).
 */
#include "se.h"

/*other modules*/
extern C_120E_0221();/*DisplayTick*/
extern C_1534_056F();/*MAP_LoadAround*/
extern int16 C_1534_1101(int16);/*WrapDelta*/
extern int16 C_1B09_032A();/*next object of C_1B09_07D8's search*/
extern int16 C_1B09_07D8(int16, int16, int16);/*first object at (x, y, z)?*/
extern int16 C_1B09_09C7(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 C_1B09_0DD7(void);/*NextArea*/
extern int16 C_1B09_0EDE(int16, int16, int16, int16);/*SearchArea(x1, y1, x2, y2)*/
extern int16 C_1B09_116E(int16, int16, int16, int16, int16, int16);/*AddMapObj*/
extern C_1B09_1872(int16);/*DeleteObj*/
extern C_1B09_1F52(int16, int16, int16, int16);/*MoveObj*/
extern C_2F72_0000();/*MachineryOn*/
extern C_2F72_0210();/*MachineryOff*/
extern int16 OSI_rand(int16, int16);/*OSI_rand(lo, hi)*/
extern cdecl CON_printf(char far *, ...);
extern char far C_2BB4_2203[];/*"The coal begins moving down the belt.\n"*/
extern char far C_2BB4_222A[];/*"The conveyor belt stops.\n"*/
extern char far C_2BB4_22F1[];/*"Aargh!\n"*/

extern int16 D_1431;/*VisibilityDirty*/
extern unsigned char D_167A;/*Active*/
extern int16 D_167C, D_167E, D_1680;/*MapX, MapY, MapZ*/
extern unsigned char D_3266[];/*ObjStatus*/
extern unsigned char D_4EA4[];/*Party*/
extern int16 D_54BA[];/*Amount: quantity (low byte), quality (high byte)*/
extern unsigned char D_6EC6[];/*TalkFlags (per NPC)*/
extern unsigned char far *D_7606;/*TypeWeight*/
extern struct coord D_78AE[];/*ObjPos*/
extern uint16 D_9CC4[];/*ObjShapeType*/
extern int16 D_B530;/*barge_scrap_count (barge.c)*/
extern int16 D_B576[0x200];/*barge_scrap (barge.c)*/
extern int16 D_C0FA, D_C0FC;/*AreaX, AreaY*/

#define VisibilityDirty D_1431
#define Active D_167A
#define MapX D_167C
#define MapY D_167E
#define MapZ D_1680
#define ObjStatus D_3266
#define Party D_4EA4
#define Amount D_54BA
#define TalkFlags D_6EC6
#define TypeWeight D_7606
#define ObjPos D_78AE
#define ObjShapeType D_9CC4
#define barge_scrap_count D_B530
#define barge_scrap D_B576
#define AreaX D_C0FA
#define AreaY D_C0FC

#define DisplayTick C_120E_0221
#define MAP_LoadAround C_1534_056F
#define WrapDelta C_1534_1101
#define OBJ_FindTypeAt C_1B09_09C7
#define NextArea C_1B09_0DD7
#define SearchArea C_1B09_0EDE
#define AddMapObj C_1B09_116E
#define DeleteObj C_1B09_1872
#define MoveObj C_1B09_1F52
#define MachineryOn C_2F72_0000
#define MachineryOff C_2F72_0210

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)

/*move the belts' loads; mayStop: the belt may break down*/
/*C_2F52_0000*/BeltUpdate(int16 mayStop) {
	int16 x, y, type, npc;
	register int16 objNum, belt;

	if(!(TalkFlags[0x73] & 4))
		return;
	barge_scrap_count = 0;
	for(belt = SearchArea(AreaX, AreaY, (AreaX + 0x27) & 0x3ff, (AreaY + 0x27) & 0x3ff); belt >= 0; belt = NextArea()) {
		if(GetType(belt) != 0xbc)
			continue;
		x = GetX(belt);
		y = GetY(belt);
		if(GetQual(belt) & 1) {
			/*westwards*/
			if(GetFrame(belt) == 2) {
				if(!GetQuan(belt)) {
					if((objNum = AddMapObj(0xbd, 0, 0, x, y, MapZ)) < 0)
						CON_printf(C_2BB4_22F1);
					else
						ObjStatus[objNum] &= ~0x20;
					GetQuan(belt) = 4;
				}
				GetQuan(belt) = GetQuan(belt) - 1;
			} else if(GetFrame(belt) == 0) {
				if((objNum = OBJ_FindTypeAt(0xbd, x, y)) >= 0)
					DeleteObj(objNum);
			}
			if(WrapDelta(x - AreaX) <= 0)
				continue;
			if((objNum = OBJ_FindTypeAt(0xbd, x, y)) >= 0)
				MoveObj(objNum, (x - 1) & 0x3ff, y, MapZ);
			npc = -1;
			for(objNum = C_1B09_07D8(x, y, MapZ); objNum >= 0; objNum = C_1B09_032A()) {
				type = GetType(objNum);
				if((objNum < 0x100 && objNum >= 0) || type == 0x1bf || TypeWeight[type] > 0) {
					MoveObj(objNum, (x - 1) & 0x3ff, y, MapZ);
					if(objNum < 0x100 && objNum >= 0) {
						npc = objNum;
						if(Party[Active] == objNum) {
							MapX = (x - 1) & 0x3ff;
							MapY = y;
							MAP_LoadAround();
							VisibilityDirty = 1;
						}
					}
				}
			}
			if(npc >= 0)
				MoveObj(npc, (x - 1) & 0x3ff, y, MapZ);
		} else {
			if(WrapDelta(x - AreaX) < 0x27 && barge_scrap_count < 0x200) {
				barge_scrap[barge_scrap_count] = belt;
				barge_scrap_count ++;
			}
		}
	}
	/*eastwards*/
	while(barge_scrap_count) {
		belt = barge_scrap[--barge_scrap_count];
		x = GetX(belt);
		y = GetY(belt);
		if(GetFrame(belt) == 0) {
			if(!GetQuan(belt)) {
				if((objNum = AddMapObj(0xbd, 0, 0, x, y, MapZ)) < 0)
					CON_printf(C_2BB4_22F1);
				else
					ObjStatus[objNum] &= ~0x20;
				GetQuan(belt) = 4;
			}
			GetQuan(belt) = GetQuan(belt) - 1;
		} else if(GetFrame(belt) == 2) {
			if((objNum = OBJ_FindTypeAt(0xbd, x, y)) >= 0)
				DeleteObj(objNum);
		}
		if((objNum = OBJ_FindTypeAt(0xbd, x, y)) >= 0)
			MoveObj(objNum, (x + 1) & 0x3ff, y, MapZ);
		npc = -1;
		for(objNum = C_1B09_07D8(x, y, MapZ); objNum >= 0; objNum = C_1B09_032A()) {
			type = GetType(objNum);
			if((objNum < 0x100 && objNum >= 0) || type == 0x1bf || TypeWeight[type] > 0) {
				MoveObj(objNum, (x + 1) & 0x3ff, y, MapZ);
				if(objNum < 0x100 && objNum >= 0) {
					npc = objNum;
					if(Party[Active] == objNum) {
						MapX = (x + 1) & 0x3ff;
						MapY = y;
						MAP_LoadAround();
						VisibilityDirty = 1;
					}
				}
			}
		}
		if(npc >= 0)
			MoveObj(npc, (x + 1) & 0x3ff, y, MapZ);
	}
	if(mayStop && (TalkFlags[0x71] & 8) && !OSI_rand(0, 6)) {
		CON_printf(C_2BB4_222A);
		TalkFlags[0x73] &= ~4;
		TalkFlags[0x71] &= ~8;
		MachineryOff();
		DisplayTick();
	}
}

/*the belt is started*/
/*C_2F52_0473*/BeltStart() {
	CON_printf(C_2BB4_2203);
	TalkFlags[0x73] |= 4;
	TalkFlags[0x71] |= 8;
	MachineryOn();
	DisplayTick();
	BeltUpdate(0);
	DisplayTick();
}
