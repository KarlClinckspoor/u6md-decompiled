/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE segment 03CE: the barges, new code with no
 * Savage Empire counterpart.
 *
 * A barge is a group of objects of type 0x1a1 (pieces) with one 0x1a2
 * (its controller), all with the barge's number as quality; the
 * controller's frame (0-3) is the direction it moves in. While the canal
 * pumps run (TalkFlags[0x76] & 0x40 and & 0x20) or in the dream, each turn
 * the controllers found on the map (BargeAddController) move their barge
 * one tile (BargeMoveAll): the pieces must all be on water tiles and not
 * meet anything else; everything standing on the barge moves along, and
 * the party's view follows when the leader rode it. A barge whose
 * controller has quantity other than its pieces' count is reported.
 * The first functions write a debugging dump of the barge state to a file
 * "brg".
 */
#include "se.h"
#include <stdio.h>

/*OSI file routines (asm)*/
extern int16 DOS_create(char far *);
extern int32 DOS_write(int16, int32, int32, void far *);

/*other modules*/
extern char DirIncrX[8], DirIncrY[8];
extern int16 D_1431;/*VisibilityDirty*/
extern int16 D_14F2;
extern unsigned char D_167A;/*Active*/
extern int16 D_167C, D_167E;/*MapX, MapY*/
extern char D_16A5;/*SoloMember*/
extern int32 D_16AA;/*flags (0x10: the dream)*/
extern unsigned char D_4EA4[];/*Party*/
extern int16 D_54BA[];/*Amount: quantity (low byte), quality (high byte)*/
extern unsigned char D_6CBA[];/*NPCStatus*/
extern unsigned char far *D_6DBA;/*TileFlag*/
extern int16 far *D_6EBE;/*BaseTile*/
extern unsigned char D_6EC6[];/*TalkFlags (per NPC)*/
extern unsigned char far *D_760A;/*TileFlag2*/
extern int16 D_770E[];/*AnimFirstFrame*/
extern struct coord D_78AE[];/*ObjPos*/
extern unsigned char D_9CC0;/*PartySize*/
extern int16 D_9CC2;/*NumAnimData*/
extern uint16 D_9CC4[];/*ObjShapeType*/
extern int16 D_B4C8[];/*AnimTile*/
extern int16 D_B530;/*barge_scrap_count*/
extern int16 D_B532;/*barge_piece_count*/
extern int16 D_B534[0x10];/*controllers*/
extern int16 D_B554[0x10];/*bargeids*/
extern int16 D_B574;/*numbarges*/
extern int16 D_B576[0x200];/*barge_scrap*/
extern int16 D_B976;/*barge_num*/
extern int16 D_B978[9];/*CurrentBarge*/
extern int16 D_B98A[0x20];/*barge_pieces*/

extern C_120E_0009();
extern C_120E_0221();/*DisplayTick*/
extern C_1534_056F();/*MAP_LoadAround*/
extern unsigned char C_1534_1072(int16, int16, int16);/*GetTileAtXYZ*/
extern int16 C_170D_0002(int16);
extern int16 C_170D_007E(int16, int16, int16);
extern int16 C_1B09_032A();/*next object of C_1B09_07D8's search*/
extern int16 C_1B09_07D8(int16, int16, int16);/*first object at (x, y, z)?*/
extern int16 C_1B09_09C7(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 C_1B09_0DD7(void);/*NextArea*/
extern int16 C_1B09_0EDE(int16, int16, int16, int16);/*SearchArea(x1, y1, x2, y2)*/
extern C_1B09_1F52(int16, int16, int16, int16);/*MoveObj*/
extern C_1B09_3E18();/*ShowObjects*/
extern C_1FA9_0919(int16);/*play a sound effect?*/
extern int16 C_222C_0008(int16);/*InRange_10_17_C0_C3: water tiles*/
extern cdecl CON_printf(char far *, ...);

#define VisibilityDirty D_1431
#define Active D_167A
#define MapX D_167C
#define MapY D_167E
#define SoloMember D_16A5
#define IN_DREAM (D_16AA & 0x10)
#define Party D_4EA4
#define Amount D_54BA
#define NPCStatus D_6CBA
#define TileFlag D_6DBA
#define BaseTile D_6EBE
#define TalkFlags D_6EC6
#define TileFlag2 D_760A
#define AnimFirstFrame D_770E
#define ObjPos D_78AE
#define PartySize D_9CC0
#define NumAnimData D_9CC2
#define ObjShapeType D_9CC4
#define AnimTile D_B4C8
#define barge_scrap_count D_B530
#define barge_piece_count D_B532
#define controllers D_B534
#define bargeids D_B554
#define numbarges D_B574
#define barge_scrap D_B576
#define barge_num D_B976
#define CurrentBarge D_B978
#define barge_pieces D_B98A

#define DisplayTick C_120E_0221
#define MAP_LoadAround C_1534_056F
#define GetTileAtXYZ C_1534_1072
#define OBJ_FindTypeAt C_1B09_09C7
#define NextArea C_1B09_0DD7
#define SearchArea C_1B09_0EDE
#define MoveObj C_1B09_1F52
#define ShowObjects C_1B09_3E18
#define InRange_10_17_C0_C3 C_222C_0008

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)

#define PUMPS_ON (!!(TalkFlags[0x76] & 0x40) && !!(TalkFlags[0x76] & 0x20))

/*028C*/char LeaderOnBarge = -1;/*the barge the party leader stands on (-1: none)*/
/*028D*/int16 BargeDumpFile = -1;

/*open the dump file*/
/*C_03CE_0006*/BargeDumpOpen() {
	CON_printf("Dumping Barge Info\n");
	if(BargeDumpFile == -1)
		BargeDumpFile = DOS_create("brg");
	if(BargeDumpFile < 0)
		CON_printf("Dump Fail\n");
}

/*C_03CE_003E*/BargeDumpNewLine() {
	DOS_write(BargeDumpFile, -1, 2, "\r\n");
}

/*C_03CE_0060*/BargeDumpStr(char *str) {
	DOS_write(BargeDumpFile, -1, strlen(str), str);
}

/*"name=value" on a line, or "value" in a column when there is no name*/
/*C_03CE_0089*/BargeDumpInt(register char *name, int16 val) {
	char buf[100];

	if(name && *name) {
		sprintf(buf, "%s=%d", name, val);
		BargeDumpStr(buf);
		BargeDumpNewLine();
	} else {
		sprintf(buf, "%5d   ", val);
		BargeDumpStr(buf);
	}
}

/*C_03CE_00E0*/BargeDumpArray(char *name, int16 *arr, int16 n) {
	register int16 i;

	BargeDumpStr(name);
	BargeDumpStr("[");
	BargeDumpInt(0, n);
	BargeDumpStr("] = ");
	for(i = 0; i < n; i++)
		BargeDumpInt("", arr[i]);
	BargeDumpNewLine();
}

/*C_03CE_012B*/BargeDump() {
	BargeDumpOpen();
	BargeDumpStr("=======BARGE INFO=========");
	BargeDumpNewLine();
	BargeDumpInt("barge_num", barge_num);
	BargeDumpArray("barge_pieces", barge_pieces, 0x20);
	BargeDumpInt("barge_piece_count", barge_piece_count);
	BargeDumpStr("-------");
	BargeDumpNewLine();
	BargeDumpInt("barge_scrap_count", barge_scrap_count);
	BargeDumpArray("barge_scrap", barge_scrap, 0x200);
	BargeDumpStr("-------");
	BargeDumpNewLine();
	BargeDumpInt("LeaderOnBarge", LeaderOnBarge);
	BargeDumpArray("CurrentBarge", CurrentBarge, 9);
	BargeDumpStr("-------");
	BargeDumpNewLine();
	BargeDumpInt("numbarges", numbarges);
	BargeDumpArray("bargeids", bargeids, 0x10);
	BargeDumpArray("controllers", controllers, 0x10);
}

/*forget the controllers found*/
/*C_03CE_01F1*/BargeClear() {
	numbarges = 0;
}

/*a controller (0x1a2) seen on the map: keep it for BargeMoveAll*/
/*C_03CE_01FC*/BargeAddController(register int16 objNum) {
	if((PUMPS_ON || IN_DREAM) && GetFrame(objNum) < 4 && numbarges < 0x10) {
		bargeids[numbarges] = GetQual(objNum);
		controllers[numbarges] = objNum;
		numbarges ++;
	}
}

/*move every barge found one tile along its controller's direction*/
/*C_03CE_0261*/BargeMoveAll() {
	int16 dx, dy, moved, x, leaderMoved;
	register int16 y, d;

	moved = 0;
	if(!(PUMPS_ON || IN_DREAM))
		return;
	leaderMoved = 0;
	while(numbarges --) {
		if(GetType(controllers[numbarges]) != 0x1a2) {
			CON_printf("The barge creaks.");
			C_1FA9_0919(0xc);
			continue;
		}
		barge_num = bargeids[numbarges];
		BargeFindPieces(controllers[numbarges]);
		if(GetQuan(controllers[numbarges]) == barge_piece_count) {
			d = GetFrame(controllers[numbarges]) << 1;
			dx = DirIncrX[d];
			dy = DirIncrY[d];
			if(BargeCanMove(dx, dy) && !BargeTooHeavy()) {
				BargeMove(dx, dy, &leaderMoved);
				moved = 1;
			}
		} else if(GetQuan(controllers[numbarges]) < barge_piece_count) {
			CON_printf("Found %d barge pieces when expecting %d!\r\n", barge_piece_count, GetQuan(controllers[numbarges]));
		}
	}
	if(leaderMoved) {
		x = GetX(Party[Active]);
		y = GetY(Party[Active]);
		if(x != MapX || y != MapY) {
			MapX = x;
			MapY = y;
			MAP_LoadAround();
			VisibilityDirty = 1;
		}
	}
	if(moved) {
		DisplayTick();
		ShowObjects();
	}
}

/*barge_pieces: the controller, then the pieces of barge barge_num near it*/
/*C_03CE_0419*/BargeFindPieces(register int16 objNum) {
	register int16 i;

	barge_piece_count = 1;
	barge_pieces[0] = objNum;
	for(
		i = SearchArea((GetX(objNum) - 6) & 0x3ff, (GetY(objNum) - 6) & 0x3ff, (GetX(objNum) + 6) & 0x3ff, (GetY(objNum) + 6) & 0x3ff);
		i != -1;
		i = NextArea()
	) {
		if(!C_170D_0002(i) && GetType(i) == 0x1a1 && GetQual(i) == barge_num && barge_piece_count < 0x20) {
			barge_pieces[barge_piece_count] = i;
			barge_piece_count ++;
		}
	}
}

/*can the barge move by (dx, dy)? Every piece must reach a water tile
  with nothing on it but pieces of the same barge; the party cannot be
  left behind in part*/
/*C_03CE_04E7*/BargeCanMove(int16 dx, int16 dy) {
	int16 i, x, y, z, type, sameBarge, blocked, npcs;
	register int16 objNum, ctrl;

	npcs = 0;
	ctrl = barge_pieces[0];
	for(i = 0; i < barge_piece_count; i++) {
		x = GetX(barge_pieces[i]);
		y = GetY(barge_pieces[i]);
		z = GetZ(barge_pieces[i]);
		for(objNum = C_1B09_07D8(x, y, z); objNum >= 0; objNum = C_1B09_032A()) {
			if(objNum < 0x100 && objNum >= 0 && (NPCStatus[objNum] & 0x80))
				npcs ++;
		}
		x = (x + dx) & 0x3ff;
		y = (y + dy) & 0x3ff;
		if(C_170D_007E(x, y, z))
			return 0;
		if(!InRange_10_17_C0_C3(GetTileAtXYZ(x, y, z))) {
			ObjShapeType[ctrl] = GetType(ctrl) + 0x1000;
			return 0;
		}
		sameBarge = 0;
		blocked = 0;
		for(objNum = C_1B09_07D8(x, y, z); objNum >= 0; objNum = C_1B09_032A()) {
			if(D_14F2)
				break;
			type = GetType(objNum);
			if(type == 0x1a1 || type == 0x1a2) {
				if(GetQual(objNum) != barge_num) {
					ObjShapeType[ctrl] = GetType(ctrl) + 0x1000;
					return 0;
				}
				sameBarge = 1;
			} else if(type < 0x1cd) {
				blocked = 1;
			}
		}
		if(blocked && !sameBarge) {
			ObjShapeType[ctrl] = GetType(ctrl) + 0x1000;
			return 0;
		}
	}
	if(SoloMember == -1 && !IN_DREAM && npcs > 0 && PartySize > npcs)
		return 0;

	return 1;
}

/*barge_scrap: the objects on the barge; 1 when there are too many*/
/*C_03CE_0698*/BargeTooHeavy() {
	int16 tile;
	register int16 i, objNum;

	barge_scrap_count = 0;
	for(i = 0; i < barge_piece_count; i++) {
		for(
			objNum = C_1B09_07D8(GetX(barge_pieces[i]), GetY(barge_pieces[i]), GetZ(barge_pieces[i]));
			objNum >= 0;
			objNum = C_1B09_032A()
		) {
			tile = BaseTile[GetType(objNum)] + GetFrame(objNum) - D_14F2;
			if(barge_scrap_count < 0x200 && !(TileFlag2[tile] & 4) && !(TileFlag[tile] & 0x10) && GetType(objNum) < 0x1cd) {
				barge_scrap[barge_scrap_count] = objNum;
				barge_scrap_count ++;
			}
		}
	}
	if(barge_scrap_count < 0x19c)
		return 0;
	if(barge_scrap_count < 0x200) {
		CON_printf("The barge is riding low in the water.\n");
		return 0;
	}
	CON_printf("The barge is scraping the bottom of the canal.\n");
	return 1;
}

/*move the barge's pieces and what stands on it by (dx, dy)*/
/*C_03CE_07BB*/BargeMove(int16 dx, int16 dy, int16 *leaderMoved) {
	register int16 i;

	for(i = 0; i < barge_piece_count; i++) {
		if(GetType(barge_pieces[i]) == 0x1a1 || GetType(barge_pieces[i]) == 0x1a2) {
			MoveObj(barge_pieces[i], (GetX(barge_pieces[i]) + dx) & 0x3ff, (GetY(barge_pieces[i]) + dy) & 0x3ff, GetZ(barge_pieces[i]));
		} else {
			CON_printf("The barge groans!\n");
			C_1FA9_0919(0xc);
		}
	}
	for(i = barge_scrap_count - 1; i >= 0; i--) {
		if(GetType(barge_scrap[i]) == 0x1a1 || GetType(barge_scrap[i]) == 0x1a2) {
			CON_printf("The barge shudders!\n");
			C_1FA9_0919(0xc);
		} else {
			MoveObj(barge_scrap[i], (GetX(barge_scrap[i]) + dx) & 0x3ff, (GetY(barge_scrap[i]) + dy) & 0x3ff, GetZ(barge_scrap[i]));
		}
		if(barge_scrap[i] == Party[Active])
			*leaderMoved = 1;
	}
}

/*is objNum (the party leader) on a barge? LeaderOnBarge and CurrentBarge*/
/*C_03CE_0954*/BargeFindLeader(register int16 objNum) {
	register int16 i;

	if((i = OBJ_FindTypeAt(0x1a1, GetX(objNum), GetY(objNum))) >= 0 || (i = OBJ_FindTypeAt(0x1a2, GetX(objNum), GetY(objNum))) >= 0) {
		LeaderOnBarge = GetQual(i);
		for(i = 0; i < 9; i++)
			CurrentBarge[i] = objNum;
		for(i = 0x101; i < 0xc00; i++) {
			if(GetType(i) == 0x1a1 && GetQual(i) == LeaderOnBarge && GetQuan(i) > 0 && GetQuan(i) < 9)
				CurrentBarge[GetQuan(i) - 1] = i;
		}
	} else {
		LeaderOnBarge = -1;
	}
}

/*the pumps (TalkFlags[0x76] 0x20 and 0x40): start the water's animation;
  1 when they run*/
/*C_03CE_0A42*/BargePumps(int16 flag) {
	register int16 i;

	if(TalkFlags[0x76] & 0x20) {
		if(TalkFlags[0x76] & 0x40) {
			TalkFlags[0x76] |= 0x80;
			for(i = 0; i < NumAnimData; i++) {
				if(InRange_10_17_C0_C3(AnimTile[i]))
					AnimFirstFrame[i] ++;
			}
		}
		if(flag) {
			for(i = 0; i < NumAnimData; i++) {
				if(AnimTile[i] >= 1 && AnimTile[i] <= 4)
					AnimFirstFrame[i] ++;
			}
		}
	}
	C_120E_0009();
	return (TalkFlags[0x76] & 0x20) && (TalkFlags[0x76] & 0x40);
}
