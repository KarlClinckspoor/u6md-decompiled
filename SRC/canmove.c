/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE segment 104E: can an NPC stand on or move to a
 * map square. New code (nothing close in Savage Empire: SE's equivalents
 * are scattered in objcmd.c and npctrack.c).
 *
 * A creature's movement class comes from its class flags (MonsterFlags,
 * far table C_2DE5_01C6 by GetMonsterClass): 0x100 goes anywhere but walls
 * (TerrainType 4), 0x200 swims (TerrainType 2 and 1, not 4 nor 8; on tile
 * 8 always, else only while the canal pumps run: TalkFlags[0x76] & 0x20 and
 * & 0x40), 0x400 flies; with none of them it walks (no water 2, wall 4 or
 * 8; 0x8000: only on InRange_20_3F_7C_7F tiles). NPCs 0x100 and more,
 * and negative numbers, walk. TerrainType bits 0x10/0x20/0x40/0x80 are the
 * passable sides (W/S/E/N) of a "water" (2) tile such as a wall or a fence.
 */
#include "se.h"

extern int16 far C_2DE5_01C6[];/*MonsterFlags (by GetMonsterClass)*/

extern char DirIncrX[8], DirIncrY[8];
extern int16 D_142A;/*the mover stands on a tile of TerrainType 8*/
extern int16 D_1433;/*party members may swap places?*/
extern int16 D_1435;/*TerrainType 8 blocks*/
extern int16 D_14F2;/*tile offset of the objects (animation)*/
extern unsigned char D_167A;/*Active*/
extern int16 D_1680;/*MapZ*/
extern int16 D_1689;
extern unsigned char D_3266[];/*ObjStatus*/
extern unsigned char far *D_45CC;/*NPCFlag*/
extern unsigned char D_4EA4[];/*Party*/
extern unsigned char D_6CBA[];/*NPCStatus*/
extern unsigned char far *D_6DBA;/*TileFlag*/
extern unsigned char D_6EC6[];/*TalkFlags*/
extern int16 far *D_6EBE;/*BaseTile*/
extern unsigned char far *D_760A;/*TileFlag2*/
extern struct coord D_78AE[];/*ObjPos*/
extern uint16 D_9CC4[];/*ObjShapeType*/
extern unsigned char far *D_B4C4;/*TerrainType*/
extern char far C_2BB4_0210[];

extern int16 C_047A_234C(int16, int16, int16, int16);/*push obj from (x, y) in dir: 0 moved, 2 message*/
extern int16 C_100B_0222(int16);/*pushable type*/
extern int16 C_1534_1101(int16);/*WrapDelta*/
extern unsigned char C_1534_1072(int16, int16, int16);/*GetTileAtXYZ*/
extern int16 C_1649_000A(int16);/*GetMonsterClass(type): < 0 if none*/
extern C_1FA9_0919(int16);/*MUS_PlaySfx*/
extern int16 C_1B09_032A();/*next object of C_1B09_07D8's search*/
extern int16 C_1B09_07D8(int16, int16, int16);/*first object at (x, y, z)?*/
extern int16 C_1B09_08BC(int16, int16);
extern int16 C_1B09_09C7(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern C_1B09_1F52(int16, int16, int16, int16);/*MoveObj(obj, x, y, z)*/
extern C_1B09_3E18();/*ShowObjects*/
extern int16 C_222C_0008(int16);/*InRange_10_17_C0_C3: water tiles*/
extern int16 C_222C_005F(int16);/*InRange_20_3F_7C_7F*/
extern cdecl CON_printf(char far *, ...);
extern void cdecl TimerDelay(int32);

#define MonsterFlags C_2DE5_01C6
#define Active D_167A
#define MapZ D_1680
#define ObjStatus D_3266
#define NPCFlag D_45CC
#define Party D_4EA4
#define NPCStatus D_6CBA
#define TileFlag D_6DBA
#define TalkFlags D_6EC6
#define BaseTile D_6EBE
#define TileFlag2 D_760A
#define ObjPos D_78AE
#define ObjShapeType D_9CC4
#define TerrainType D_B4C4

#define PushObj C_047A_234C
#define WrapDelta C_1534_1101
#define GetTileAtXYZ C_1534_1072
#define GetMonsterClass C_1649_000A
#define MUS_PlaySfx C_1FA9_0919
#define OBJ_FindTypeAt C_1B09_09C7
#define MoveObj C_1B09_1F52
#define ShowObjects C_1B09_3E18
#define IsWaterTile C_222C_0008
#define InRange_20_3F_7C_7F C_222C_005F

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)

/*0E96*/int16 MoveFromX = -1;/*the square the mover comes from*/
/*0E98*/int16 MoveFromY = -1;
/*0E9A*/int16 MoveFromSet = 0;/*set: MoveFromX/Y given by the caller*/
/*0E9C*/int16 LeaderOnBelt = 0;/*NPC 1 moved onto a conveyor belt (0xbc)*/
/*0E9E*/int16 MemberOnBelt = 0;/*a party member moved onto a conveyor belt*/

CanStandAt(int16, int16, int16);

/*can the NPC be at (x, y): a two-tile creature (types 0x167-0x169 by
  its direction, or the tile's 0x40/0x80 flag) needs the square behind it
  too*/
/*C_104E_0007*/CanMoveTo(register int16 npc, int16 x, register int16 y) {
	int16 tile, result = 0, type, dir;

	tile = BaseTile[GetType(npc)] + GetFrame(npc);
	type = GetType(npc);
	if(npc == -1) {
		result = 0;
	} else {
		if(npc == 0 && GetTileAtXYZ(x, y, MapZ) == 0x6f)
			return 1;
		if(type != 0x10c && (TileFlag[tile] & 0xc0) && (TileFlag[tile] & 0xc0) != 0xc0 && type != 0x1b9) {
			if(type == 0x167 || type == 0x168 || type == 0x169) {
				dir = GetFrame(npc) / 4 * 2;
				switch(dir) {
				case 0: case 4:
					result = CanStandAt(npc, x, y) && CanStandAt(npc, x, y - 1);
					break;
				case 2: case 6:
					result = CanStandAt(npc, x, y) && CanStandAt(npc, x - 1, y);
					break;
				}
			} else if((TileFlag[tile] & 0xc0) == 0x40) {
				result = CanStandAt(npc, x, y) && CanStandAt(npc, x, y - 1);
			} else if((TileFlag[tile] & 0xc0) == 0x80) {
				result = CanStandAt(npc, x, y) && CanStandAt(npc, x - 1, y);
			}
		} else {
			result = CanStandAt(npc, x, y);
		}
	}
	return result;
}

/*can the NPC stand on the square (x, y): its tile and the objects there*/
/*C_104E_01D8*/CanStandAt(int16 npc, int16 x, int16 y) {
	int16 anywhere, swims, flies, walks, class, terrain;
	int16 objType, objTerrain, tile, type;
	register int16 obj, ok;

	ok = 0;
	if(npc == -1) {
		ok = 0;
	} else {
		type = GetType(npc);
		if(npc >= 0x100 || npc < 0) {
			anywhere = 0;
			swims = 0;
			flies = 0;
			walks = 1;
		} else {
			class = GetMonsterClass(type);
			anywhere = MonsterFlags[class] & 0x100;
			swims = MonsterFlags[class] & 0x200;
			flies = MonsterFlags[class] & 0x400;
			walks = !swims && !anywhere && !flies;
		}
		tile = GetTileAtXYZ(x, y, MapZ);
		terrain = TerrainType[tile];
		ok = 0;
		if(flies) {
			ok = 1;
		} else if(anywhere) {
			if(terrain & 4)
				ok = 0;
			else
				ok = 1;
		} else if(swims) {
			if(!(terrain & 4) && (terrain & 2) && (terrain & 1) && (tile == 8 || (TalkFlags[0x76] & 0x20) && (TalkFlags[0x76] & 0x40)) && !(terrain & 8))
				ok = 1;
			else
				ok = 0;
		} else if(walks) {
			ok = 1;
			if((terrain & 4) || (terrain & 2) || (terrain & 8))
				ok = 0;
			if(npc < 0x100 && npc >= 0 && (MonsterFlags[class] & 0x8000) && !InRange_20_3F_7C_7F(tile))
				ok = 0;
		}
		for(obj = C_1B09_07D8(x, y, MapZ); obj >= 0; obj = C_1B09_032A()) {
			if(obj == npc)
				continue;
			objType = GetType(obj);
			if((type == 0x1b9 || type == 0x10c) && objType == 0xaf)
				continue;
			tile = BaseTile[GetType(obj)] + GetFrame(obj) - D_14F2;
			if((TileFlag2[tile] & 0x10) && !(TerrainType[tile] & 2) && !(TileFlag2[tile] & 4))
				continue;
			objTerrain = TerrainType[tile];
			if(TileFlag2[tile] & 4) {
				if(walks) {
					if(npc >= 0x100 || npc < 0 || !(MonsterFlags[class] & 0x8000) || InRange_20_3F_7C_7F(tile))
						ok = 1;
					if(objTerrain & 2)
						ok = 0;
					if(!(TileFlag2[tile] & 0x10))
						break;
				} else if(swims) {
					ok = 0;
				}
			} else if(
				(walks && ((objTerrain & 2) || (objTerrain & 8) || (obj < 0x100 && obj >= 0 && !(MonsterFlags[GetMonsterClass(objType)] & 0x100)))) ||
				(swims && (!(objTerrain & 2) || (objTerrain & 8) || !(objTerrain & 1) || (tile != 8 && (!(TalkFlags[0x76] & 0x20) || !(TalkFlags[0x76] & 0x40))) || (obj < 0x100 && obj >= 0 && !(MonsterFlags[GetMonsterClass(objType)] & 0x100)))) ||
				(anywhere && ((objTerrain & 4) || (obj < 0x100 && obj >= 0 && (MonsterFlags[GetMonsterClass(objType)] & 0x100)))) ||
				(flies && obj < 0x100 && obj >= 0)
			) {
				ok = 0;
			}
			if(!ok)
				break;
		}
	}
	return ok;
}

/*the NPC tries to step from MoveFromX/Y (its position unless MoveFromSet)
  to (x, y): checks the sides of fences and walls on both squares, and
  the party leader pushes pushable objects out of the way (type 0x19a into
  water, type 0x10c turned); returns 1 if the step is possible*/
/*C_104E_056F*/NPC_TryStep(register int16 npc, int16 x, int16 y) {
	int16 anywhere, swims, flies, walks, class;
	int16 east, west, north, south, diagonal;
	int16 tile, terrain, fromTerrain, objType, objTerrain;
	int16 okFrom = 0, okTo = 0;
	int16 dtile, twoTiles, dir;
	int16 pushed = -1;
	int16 nx, ny;
	int16 water = 0, objAt = 0;
	int16 frame, pushResult;
	int16 onRail = 0, onBelt = 0;/*type 0xaf (rails?) here or under the mover; type 0xbc (belt) there*/
	register int16 obj;

	if(npc == -1)
		return 0;
	if(Party[Active] == npc && D_1689 == 1)
		return 1;
	dtile = GetTileAtXYZ(x, y, MapZ);
	if(npc == 0 && dtile == 0x6f) {
		if(OBJ_FindTypeAt(0x1a1, x, y) >= 0)
			return OBJ_FindTypeAt(0x124, x, y) < 0;
		return GetX(npc) == x && GetY(npc) + 1 == y;
	}
	if(npc >= 0x100 || npc < 0) {
		anywhere = 0;
		swims = 0;
		flies = 0;
		walks = 1;
	} else {
		class = GetMonsterClass(GetType(npc));
		anywhere = MonsterFlags[class] & 0x100;
		swims = MonsterFlags[class] & 0x200;
		flies = MonsterFlags[class] & 0x400;
		walks = !swims && !anywhere && !flies;
	}
	tile = BaseTile[GetType(npc)] + GetFrame(npc);
	twoTiles = (TileFlag[tile] & 0xc0) && (TileFlag[tile] & 0xc0) != 0xc0;
	if(!(GetZ(npc) != MapZ || (ObjStatus[npc] & 0x18))) {
		if(MoveFromSet == 0) {
			MoveFromX = GetX(npc);
			MoveFromY = GetY(npc);
		}
		east = WrapDelta(MoveFromX - x) < 0;
		west = WrapDelta(MoveFromX - x) > 0;
		north = WrapDelta(MoveFromY - y) > 0;
		south = WrapDelta(MoveFromY - y) < 0;
		diagonal = (north & east) || (north & west) || (south & east) || (south & west);
		if(twoTiles) {
			if(diagonal)
				return 0;
			if(west) {
				MoveFromX = (MoveFromX - 1) & 0x3ff;
				x = (x - 1) & 0x3ff;
			}
			if(north) {
				MoveFromY = (MoveFromY - 1) & 0x3ff;
				y = (y - 1) & 0x3ff;
			}
		}
		terrain = TerrainType[dtile];
		fromTerrain = TerrainType[GetTileAtXYZ(MoveFromX, MoveFromY, MapZ)];
	} else {
		return CanMoveTo(npc, x, y);
	}
	if(OBJ_FindTypeAt(0xaf, x, y) != -1)
		onRail = 1;
	okFrom = okTo = 0;
	if(flies) {
		okFrom = okTo = 1;
	} else if(anywhere) {
		okFrom = 1;
		if(terrain & 4)
			okTo = 0;
		else
			okTo = 1;
	} else if(swims) {
		okFrom = 1;
		if(!(terrain & 4) && (terrain & 2) && (terrain & 1) && (dtile == 8 || (TalkFlags[0x76] & 0x20) && (TalkFlags[0x76] & 0x40)))
			okTo = 1;
	} else if(walks) {
		okFrom = okTo = 1;
		if(terrain & 4) {
			okTo = 0;
		} else if(terrain & 2) {
			if((west && !(terrain & 0x40)) || (east && !(terrain & 0x10)) || (north && !(terrain & 0x20)) || (south && !(terrain & 0x80)))
				okTo = 0;
		} else if(npc < 0x100 && npc >= 0 && !InRange_20_3F_7C_7F(dtile) && (MonsterFlags[class] & 0x8000)) {
			okTo = 0;
		}
		if(fromTerrain & 2) {
			if((west && !(fromTerrain & 0x10)) || (east && !(fromTerrain & 0x40)) || (north && !(fromTerrain & 0x80)) || (south && !(fromTerrain & 0x20)))
				okFrom = 0;
		}
	}
	if((terrain & 8) && !anywhere && !flies) {
		D_142A = 1;
		if(D_1435)
			okTo = 0;
	} else {
		D_142A = 0;
	}
	if(walks) {
		for(obj = C_1B09_07D8(MoveFromX, MoveFromY, MapZ); obj >= 0; obj = C_1B09_032A()) {
			if(obj == npc)
				continue;
			if(GetType(obj) == 0xaf)
				onRail = 2;
			dtile = BaseTile[GetType(obj)] + GetFrame(obj) - D_14F2;
			if((TileFlag2[dtile] & 0x10) && !(TerrainType[dtile] & 2) && !(TileFlag2[dtile] & 4))
				continue;
			objTerrain = TerrainType[dtile];
			if(TileFlag2[dtile] & 4) {
				if(npc >= 0x100 || npc < 0 || !(MonsterFlags[class] & 0x8000) || InRange_20_3F_7C_7F(dtile))
					okFrom = 1;
				if(objTerrain & 2) {
					if((west && !(objTerrain & 0x10)) || (east && !(objTerrain & 0x40)) || (north && !(objTerrain & 0x80)) || (south && !(objTerrain & 0x20)))
						okFrom = 0;
				}
				if(!(TileFlag2[dtile] & 0x10))
					break;
			}
		}
	}
	if(okFrom) {
		for(obj = C_1B09_07D8(x, y, MapZ); obj >= 0; obj = C_1B09_032A()) {
			if(obj == npc)
				continue;
			objType = GetType(obj);
			if(objType == 0xbc)
				onBelt = 1;
			dtile = BaseTile[GetType(obj)] + GetFrame(obj) - D_14F2;
			if((TileFlag2[dtile] & 0x10) && !(TerrainType[dtile] & 2) && !(TileFlag2[dtile] & 4))
				continue;
			objTerrain = TerrainType[dtile];
			if(Party[Active] == npc && C_100B_0222(objType)) {
				if(north) {
					if(west)
						dir = 7;
					else if(east)
						dir = 1;
					else
						dir = 0;
				} else if(south) {
					if(west)
						dir = 5;
					else if(east)
						dir = 3;
					else
						dir = 4;
				} else if(west) {
					dir = 6;
				} else {
					dir = 2;
				}
				if(objType != 0x19a) {
					if(objType == 0x10c) {
						switch(dir) {
						case 0: frame = 0; break;
						case 1: case 2: case 3: frame = 1; break;
						case 4: frame = 2; break;
						default: frame = 3;
						}
					} else if(onRail) {
						frame = 5;
					} else {
						switch(dir) {
						case 0: case 7: frame = 3; break;
						case 1: case 2: frame = 4; break;
						case 3: case 4: frame = 5; break;
						default: frame = 1;
						}
					}
					ObjShapeType[obj] = GetType(obj) + (frame << 10);
					ShowObjects();
				}
				nx = (x + DirIncrX[dir]) & 0x3ff;
				ny = (y + DirIncrY[dir]) & 0x3ff;
				if(!(pushResult = PushObj(obj, x, y, dir)) ||
					((water = IsWaterTile(GetTileAtXYZ(nx, ny, MapZ))) == 1 && GetType(obj) == 0x19a && (objAt = C_1B09_08BC(nx, ny)) == -1)
				) {
					pushed = obj;
					okTo = 1;
					continue;
				}
				if(pushResult == 2)
					CON_printf(C_2BB4_0210);
				okTo = 0;
				break;
			} else if(TileFlag2[dtile] & 4) {
				if(walks) {
					if(npc >= 0x100 || npc < 0 || !(MonsterFlags[class] & 0x8000) || InRange_20_3F_7C_7F(dtile))
						okTo = 1;
					if(objTerrain & 2) {
						if((west && !(objTerrain & 0x10)) || (east && !(objTerrain & 0x40)) || (north && !(objTerrain & 0x80)) || (south && !(objTerrain & 0x20)))
							okTo = 0;
					}
					if(okTo == 1)
						D_142A = 0;
					if(!(TileFlag2[dtile] & 0x10))
						break;
				} else if(swims) {
					okTo = 0;
				}
			} else if(
				(walks && ((objTerrain & 2) || (obj < 0x100 && obj >= 0 && !(MonsterFlags[GetMonsterClass(objType)] & 0x100)))) ||
				(swims && (!(objTerrain & 2) || !(objTerrain & 1) || (dtile != 8 && (!(TalkFlags[0x76] & 0x20) || !(TalkFlags[0x76] & 0x40))) || (obj < 0x100 && obj >= 0 && !(MonsterFlags[GetMonsterClass(objType)] & 0x100)))) ||
				(anywhere && ((objTerrain & 4) || (obj < 0x100 && obj >= 0 && (MonsterFlags[GetMonsterClass(objType)] & 0x100)))) ||
				(flies && obj < 0x100 && obj >= 0)
			) {
				if(NPCStatus[npc] & 0x80) {
					if(Party[Active] == obj || !D_1433 || obj >= 0x100 || obj < 0 || !(NPCStatus[obj] & 0x80) || (NPCStatus[obj] & 0x16) || (NPCFlag[obj] & 0x10))
						okTo = 0;
				} else {
					okTo = 0;
				}
			} else if((objTerrain & 8) && !anywhere && !flies) {
				if(D_1435)
					okTo = 0;
				D_142A = 1;
			}
		}
	}
	if(pushed != -1) {
		MoveObj(pushed, nx, ny, MapZ);
		if(GetType(pushed) == 0x19a) {
			MUS_PlaySfx(0xc);
			if(OBJ_FindTypeAt(0x1a3, nx, ny) >= 0) {
				TimerDelay(0x3a2dcL);
				MUS_PlaySfx(0xd);
			}
		}
	}
	if(MoveFromSet == 0) {
		if(npc == 1 && okFrom && okTo)
			LeaderOnBelt = onBelt;
		if(npc != 1 && (NPCStatus[npc] & 0x80) && okFrom && okTo)
			MemberOnBelt = onBelt;
	}
	return okFrom && okTo;
}
