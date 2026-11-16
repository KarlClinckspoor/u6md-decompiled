/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE
 * segment 1B09: object list management: Savage Empire's object.c (Ultima
 * VI's), reworked.
 *
 * All objects live in parallel arrays indexed by an object number:
 * ObjStatus[], ObjPos[] (packed x/y/z or container number), ObjShapeType[]
 * (type | frame << 10), Amount[] (quantity | quality << 8) and Link[].
 *   0x000-0x0FF  NPCs/monsters (0xC0-0xFF temporary monster slots, kept on
 *                their own free list FreeMonsterHead/FreeMonsterCount)
 *   0x100        head of the main object list (never a real object)
 *   0x101-0xBFF  ordinary objects (free list FreeObjHead/FreeObjCount)
 * Link[] chains every loaded object into one list starting at Link[0x100],
 * map objects sorted by (y, x); the contents of an NPC or a container follow
 * their owner. MapObjPtr[][] caches the first object of every tile of the
 * 40x40 area around the party.
 *
 * Only the 2x2 outdoor areas (128x128 tiles) near the party, or the current
 * underground level, are kept in the list; the rest is in the
 * "savegame\objblkXY" files (".tmp" copies for areas left behind, renamed
 * over the real files when the game is saved, OBJ_CommitBlocks).
 *
 * The module also has the location/inventory/area iterators, object
 * creation/deletion/moving (with weight and inventory size checks) and the
 * 11x11 per-tile display stacks (ShowObject/ShowObjects).
 * Object positions (ObjStatus & 0x18): 0x00 on the map, 0x08 contained,
 * 0x10 in an inventory, 0x18 equipped.
 *
 * MD's changes from SE: the map wraps around (coordinate differences go
 * through WrapDelta/MapDistance, sums are & 0x3ff) and the loaded 40x40 area
 * may cross the map's edge (OBJ_FindListPos scans MapObjPtr in map order,
 * SearchArea takes rectangles across the edge); FindLoc keeps four cursors
 * (the square and the squares right, below and below-right of it, for the
 * double tiles); it begins with SE spell.c's OBJ_FindInContainer. Gone: the
 * creatures made of several objects (OBJ_MoveCreatureParts and its helpers),
 * OBJ_getHead, C_0A5D_0613/070D, QuanType and StackableTypes (objtype.c),
 * DirConnFlag, MusicianWasNear, DISK_confirm. New: OBJ_MapObjAt,
 * OBJ_IsOnScreen, OBJ_NextOfTypeAt, FindInvOfType/NextInvOfType, OBJ_InvSlotsUsed (the
 * inventory slots used, as AddInvObj and InsertObj count them),
 * OBJ_MoveLastOnSquare, OBJ_JoinCreepingCacti (the creeping cacti 0x17f join up),
 * OBJ_DeleteContents;
 * AddInvObj/InsertObj weigh against GetStr and check what is worn; the
 * object changes of MoveObj (the rail cars 0x19a float on the canals),
 * InsertObj, __ObjectsDeserialize, OBJ_CommitBlocks and ShowObjects (machine
 * sounds, the doors 0xe3/0xb3 and the mirror 0x12d, outlines by the talents).
 */
#include "se.h"

#define AREA_W 40
#define AREA_H 40

/*an objblk file in memory (ScratchBuf, a byte buffer): 0x800 entries of 8
  bytes (status, struct coord position, shape, amount), then the object
  number of each entry (0x800 words)*/
#define BLK_STATUS(i) ScratchBuf[(i) << 3]
#define BLK_POS(i) (*(struct coord far *)&ScratchBuf[((i) << 3) + 1])
#define BLK_SHAPE(i) (*(uint16 far *)&ScratchBuf[((i) << 3) + 4])
#define BLK_AMOUNT(i) (*(int16 far *)&ScratchBuf[((i) << 3) + 6])
#define BLK_OBJNUM(i) (*(int16 far *)&ScratchBuf[((i) << 1) + 0x4000])

/*OSI file routines (asm)*/
extern int16 OSI_open(char *);
extern OSI_close(int16);
extern int16 OSI_create(char *);
extern OSI_delete(char *);
extern int32 OSI_read(int16, int32, int32, void far *);
extern int32 OSI_write(int16, int32, int32, void far *);
extern OSI_rename(char *, char *);
extern void far *LZC_loadType(char far *name, int16 index, void far *out, int16 memflags, char far *work);

/*other game modules*/
extern char DirIncrX[];
extern char DirIncrY[];
extern unsigned char StatusDirty;
extern int16 MapAnimEnabled;
extern int16 Erupting;
extern int16 StatusMember;
extern int16 StatusDisplay;/*0x92 when the equipment view is shown*/
extern int16 MapCoordMask;
extern int16 ViewX, ViewY;
extern int16 VisibilityDirty;
extern int16 MapX, MapY, MapZ;
extern int16 DiskFullChoice;
/*3266*/extern unsigned char ObjStatus[];
extern char far *LzcWorkBuf;/*the .lzc loader's work area*/
extern unsigned char AreaFlags[AREA_H][AREA_W];
extern unsigned char far *NPCFlag;
extern unsigned char far *NPCMode;
extern unsigned char Party[];
extern unsigned char far *ScratchBuf;
extern char Names[][14];
extern char far *ScratchBufHigh;/*ScratchBuf + 0x36b0*/
/*54BA*/extern int16 Amount[];
extern unsigned char NPCStatus[];
extern unsigned char far *TileFlag;
extern int16 far *BaseTile;
extern unsigned char AreaLight[AREA_H][AREA_W];
extern unsigned char far *TileFlag2;
/*78AE*/extern struct coord ObjPos[];
extern unsigned char PartySize;
/*9CC4*/extern uint16 ObjShapeType[];
extern unsigned char far *TerrainType;
extern int16 far *OrigShapeType;
extern int16 AreaX, AreaY;
/*the display stacks ShowObjects builds: the first display node of each
  view cell, the tile | frame of each node, the next node ([0]: free list)*/
/*C130*/extern unsigned char Obj_11x11[11][11];
/*C1AA*/extern int16 CellObjTile[];
/*C3AA*/extern unsigned char CellObjNext[];
extern int16 Tile_11x11[11][11];
/*C59C*/extern int16 Link[];
extern int16 AreaFirstCol, AreaFirstRow, AreaLastCol, AreaLastRow;/*the loaded area's first and last column/row in map order (map.c)*/
/*the free lists of the monster slots (0xc0-0xff) and of the objects*/
/*DD9C*/extern int16 FreeMonsterCount;
/*DDA4*/extern int16 FreeMonsterHead;
/*DDB0*/extern int16 FreeObjHead;
/*EA4C*/extern int16 FreeObjCount;
/*FindInvOfType/NextInvOfType state: position, owner, type*/
/*DD9E*/extern int16 InvOfTypeCursor;
/*DDA6*/extern int16 InvOfTypeOwner;
/*DDAE*/extern int16 InvOfTypeType;
/*SearchArea's rectangle: x1, x2, y1, y2, z; whether it crosses the map's
  edge in x, in y*/
/*DDA0*/extern int16 AreaX1;
/*DDA2*/extern int16 AreaX2;
/*DDA8*/extern int16 AreaY1;
/*DDAA*/extern int16 AreaY2;
/*DDAC*/extern int16 AreaZ;
/*EA4E*/extern int16 AreaWrapX;
/*EA50*/extern int16 AreaWrapY;
/*the first object of every tile of the loaded 40x40 area*/
/*DDB2*/extern int16 MapObjPtr[AREA_H][AREA_W];
/*GetTileString's buffer*/
/*EA32*/extern char TileStringBuf[];
extern int16 WeightEquip;
extern int16 WeightInven;
extern int16 CharmedNPC;/*an NPC charmed (gameloop.c)*/

extern ApplyTerrainEffects(int16, int16, int16);
extern cdecl CON_printf(char far *, ...);
extern int16 GetWeight(int16);
extern int16 GetTypeWeight(int16, int16);
extern STAT_ComputeWeights(int16);
extern STAT_RefreshInventory(int16);
extern SubMov(int16, int16);/*SubMov(npc, points)*/
extern int16 IsContainerType(int16);



/*this module*/
static OBJ_FindListPos(int16, int16, int16);
NextLoc();
static OBJ_LastOfSubtree(int16);
int16 FindInvType(int16, int16, int16);
int16 OBJ_CountInv(int16);
int16 OBJ_CountEquipped(int16);
int16 AddMapObj(int16, int16, int16, int16, int16, int16);
int16 AddInvObj(int16, int16, int16, int16);
DeleteObj(int16);
int16 OBJ_IsInside(int16, int16);
int16 MoveObj(int16, int16, int16, int16);
int16 InsertObj(int16, int16, int16);
ShowObjects();
OBJ_JoinCreepingCacti();

/*more of the other modules*/
extern int16 QuanType(int16);/*objtype.c*/
extern int16 WrapDelta(int16);/*map.c*/
extern int16 MapDistance(int16, int16);/*map.c*/
extern unsigned char GetTileAtXYZ(int16, int16, int16);
extern int16 IsWaterTile(int16);
extern int16 GetStr(int16);
extern DisplayTick();
extern void cdecl TimerDelay(int32);/*wait (2383 per millisecond)*/
extern MUS_PlayLoopSfx(int16);/*start a looping sound effect (-1 stops)*/
extern MUS_SweepSfx();/*a sweep sound*/
extern MUS_MachinerySfx();/*machinery sound*/
extern MUS_SplashSfx();/*sound effect 35*/
extern unsigned char Time_H;
extern unsigned char Active;
extern int32 GameFlags;/*0x10: in a dream*/
#define IN_DREAM (GameFlags & 0x10)
extern struct { unsigned a:4, psychic:4, c:4, d:4; } Talents[];
extern int16 DreamStage;
extern unsigned char TalkFlags[];
/*when set, GiveObj refuses instead of dropping what does not fit*/
/*EBC0*/extern int16 GiveObjRefuses;

#define LOCXYZ 0
#define CONTAINED 8
#define INVEN 0x10
#define EQUIP 0x18

#define GetQuan(o) (((unsigned char *)(Amount + (o)))[0])
#define GetQual(o) (((unsigned char *)(Amount + (o)))[1])
#define SetQuan(o, v) (GetQuan(o) = (v))
#define SetQual(o, v) (GetQual(o) = (v))
#define AddQuan(o, v) (GetQuan(o) += (v))
#define SubQuan(o, v) (GetQuan(o) -= (v))
#define QualQuan(quality, quantity) ((quality) << 8 | (quantity))

#define SetCoordUse(i, v) (ObjStatus[i] = (ObjStatus[i] & ~0x18) | (v))
#define GetCoordUse(i) (ObjStatus[i] & 0x18)
#define IsHeldByNPC(i) (ObjStatus[i] & 0x10)
#define IsInvisible(i) (ObjStatus[i] & 0x02)
#define IsLocal(i) (ObjStatus[i] & 0x20)
#define IsHatched(i) (ObjStatus[i] & 0x40)
#define SetLocal(i) ObjStatus[i] |= 0x20
#define ClrLocal(i) ObjStatus[i] &= ~0x20
#define ClrHatched(i) ObjStatus[i] &= ~0x40
#define IsDead(i) (NPCStatus[i] & 0x10)
#define IsPlrControl(i) (NPCStatus[i] & 0x80)
#define IsNPC(i) ((i) < 0x100 && (i) >= 0)

#define TypeFrame(t, s) ((t) + ((s) << 10))
#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))

#define GetCoordX(o) ((*(int16 *)&((o).raw[0]))&0x3ff)
#define GetCoordY(o) (((*(int16 *)&((o).raw[1]))>>2)&0x3ff)
#define GetCoordZ(o) ((o).raw[2] >> 4)
#define GetCoordAssoc(i) *(uint16 *)(&(i))
#define SetCoordX(o, v) { (*(int16 *)&((o).raw[0])) = (*(int16 *)&((o).raw[0])) & ~0x3ff; (*(int16 *)&((o).raw[0])) |= (v); }
#define SetCoordY(o, v) { (*(int16 *)&((o).raw[1])) = (*(int16 *)&((o).raw[1])) & ~(0x3ff << 2); (*(int16 *)&((o).raw[1])) |= (v) << 2; }
#define SetCoordXYZ(o, x, y, z) {      \
	(o).raw[2] = ((z) << 4);           \
	*(int16 *)&((o).raw[0]) = (x);       \
	*(int16 *)&((o).raw[1]) |= (y) << 2; \
}
#define SetCoordAssoc(i, j) *(uint16 *)(&(i)) = (j)
#define SetXYZ(i, x, y, z) SetCoordXYZ(ObjPos[i], x, y, z)
#define SetAssoc(i, j) SetCoordAssoc(ObjPos[i], j)
#define GetX(i) GetCoordX(ObjPos[i])
#define GetY(i) GetCoordY(ObjPos[i])
#define GetZ(i) GetCoordZ(ObjPos[i])
#define GetAssoc(i) GetCoordAssoc(ObjPos[i])

#define IsTileFor(tile) (TileFlag[tile] & 0x10)
#define IsTileDoubleV(tile) (TileFlag[tile] & 0x40)
#define IsTileDoubleH(tile) (TileFlag[tile] & 0x80)
#define IsTileBa(tile) (TileFlag2[tile] & 0x20)
#define IsTerrainWall(tile) (TerrainType[tile] & 0x04)

#define CHARPTR_POSTINC(p) (((char far *)p)++)

/*FindLoc/NextLoc: is object o at location t (struct coord)?*/
#define LOC_AT(o, t) ( \
	ObjPos[o].raw[0] == (t).raw[0] && \
	ObjPos[o].raw[1] == (t).raw[1] && \
	ObjPos[o].raw[2] == (t).raw[2] \
)
/*skip contained objects*/
#define LOC_SKIP(n) for(; LocCursor[n] >= 0 && GetCoordUse(LocCursor[n]); LocCursor[n] = Link[LocCursor[n]])
#define LOC_NEXT(n) while((LocCursor[n] = Link[LocCursor[n]]) >= 0 && GetCoordUse(LocCursor[n]))

/*SearchArea's rectangle*/

/*when set, NextInv/NextInvType skip equipped items*/
/*14F0*/int16 InvSkipEquipped = 0;
/*set by FindLoc/NextLoc: which part of a multi-tile object covers the
  searched tile (0 its own tile, 1 the tile left of/above it, 2/3 the
  upper/upper-left part of a 2x2 tile)*/
/*14F2*/int16 LocPartOffset = 0;
/*14F4*/static int16 AllowSubmissionMove = 0;
/*[0..63] outdoor areas, [64..68] underground levels 1..5: a ".tmp" objblk
  file newer than the saved one exists*/
/*14F6*/int16 ObjBlkTmpPending[8*8 + 5] = {0};
/*when set, MoveObj applies terrain effects to moved NPCs*/
/*1580*/int16 MoveAppliesTerrain = 0;
/*ids (yyyxxx) of the up to 4 outdoor areas loaded, -1 = free slot*/
/*1582*/int16 LoadedAreas[4] = {-1, -1, -1, -1};
/*id of the top-left area of the loaded 2x2 window, -1 = none*/
/*158A*/int16 LoadedWindowArea = -1;
/*z level loaded (0 = surface), -1 = none*/
/*158C*/int16 LoadedZ = -1;
/*FindLoc/NextLoc state: a list cursor and a location for the square and
  the squares right, below and below-right of it*/
/*158E*/int16 LocCursor[4] = {1, 1, 1, 1};
/*unused (SE's LocCursor and LocOnNextRow)*/
/*1596*/static int16 OldLocCursor = -1;
/*1598*/static int16 OldLocOnNextRow = 0;
/*159A*/static struct coord LocTarget[4] = {{0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}};
/*FindInv/NextInv/FindInvType/NextInvType state: position, owner, type,
  quality (-1 = any)*/
/*15A6*/int16 InvCursor = -1;
/*15A8*/int16 InvOwner = -1;
/*15AA*/int16 InvType = -1;
/*15AC*/int16 InvQuality = -1;
/*SearchArea/NextArea state: position and end position*/
/*15AE*/int16 AreaCursor = -1;
/*15B0*/int16 AreaEnd = -1;
/*last content object moved out by DeleteObj (-1 if none)*/
/*15B2*/int16 LastSpilledObj = -1;

/*objblk file names; characters [15] and [16] are patched with the area letters*/
/*15B4*/static char __Sav_file[] = "savegame\\objblkaa";
/*15C6*/static char __Tmp_file[] = "savegame\\objblkaa.tmp";
/*15DC*/static char __Bak_file[] = "savegame\\objblkaa.bak";

/*machines heard from nearby (ShowObjects): types, then their sound
  effects (& 0x3f; 0x40-0x7f with TalkFlags[0x73] & 4, 0x80-0xc3 with
  TalkFlags[0x73] & 0x10, from 0xc4 without it; 0x38 ends), the sound
  playing*/
/*15F2*/int16 MachineTypes[] = {0x17D, 0xD1, 0xD7, 0xE9, 0xBC, 0x1B0, 0x1B3, 0xC9, 0xD6, 0};
/*1606*/unsigned char MachineSfx[] = {0x25, 0x18, 0x15, 0x56, 0x57, 0x57, 0x57, 0x94, 0x94, 0x38};
/*1610*/int16 MachineLoopSfx = -1;

/*returns the first object of type objType (and quality objQual, 0 = any)
  inside objNum (inventory or container), -1 if none (SE's spell.c)*/
/*C_1B09_0002*/OBJ_FindInContainer(int16 objNum, int16 objType, int16 objQual) {
	int16 item;
	for(
		item = Link[objNum];
		item >= 0 && GetCoordUse(item) && OBJ_IsInside(item, objNum);
		item = Link[item]
	) {
		if(GetType(item) == objType) {
			if(objQual == 0 || GetQual(item) == objQual)
				return item;
		}
	}
	return -1;
}

/*is location (x, y, z) loaded in the object list?*/
/*C_1B09_005C*/OBJ_IsLocLoaded(int16 x, int16 y, int16 z) {
	register int16 i, isLoaded;
	int16 mapId;

	isLoaded = 0;
	if(z) {
		isLoaded = (z == MapZ);
	} else if(MapZ == 0) {
		mapId = (x >> 7) + ((y >> 4) & 0x38);
		for(i = 0; i < 4; i ++)
			if(LoadedAreas[i] == mapId) {
				isLoaded = 1;
				break;
			}
	}
	return isLoaded;
}

/*the frame of a humanoid (types 0x157-0x166) facing direction; 0 for others*/
/*C_1B09_00C3*/OBJ_MakeDirFrame(int16 objTyp, int16 direction) {
	direction >>= 1;
	if(objTyp >= 0x157 && objTyp <= 0x166)
		return direction << 2;
	return 0;
}

/*returns the next free object slot if at least num slots are free, else -1*/
/*C_1B09_00E6*/static GetFreeObjSlot(int16 num) {
	if(FreeObjCount >= num)
		return FreeObjHead;
	return -1;
}

/*position in the object list for location (x, y): the first object at or
  after it (wantPrev = 0), or the object after which an object placed there
  is to be linked (wantPrev != 0). Inside the loaded 40x40 area MapObjPtr
  is scanned in map order: the area may cross the map's edge, AreaFirstCol/AreaLastCol
  (AreaFirstRow/AreaLastRow) are its first and last column (row) in map order.*/
/*C_1B09_00FD*/static OBJ_FindListPos(int16 x, int16 y, int16 wantPrev) {
	struct coord xyz;
	int16 i, prev, cur, objY;
	int16 relX, relY, row;
	register int16 obj, col;

	relX = WrapDelta(x - AreaX) & 0x3ff;
	relY = WrapDelta(y - AreaY) & 0x3ff;
	if(relX < AREA_W && relY < AREA_H) {
		/*first non null object at or after (relX, relY)*/
		col = relX;
		row = relY;
		while((obj = MapObjPtr[row][col]) == -1) {
			if(col == AreaLastCol) {
				col = AreaFirstCol;
				if(row == AreaLastRow) {
					obj = -1;
					break;
				}
				if(row == AREA_H - 1)
					row = 0;
				else
					row ++;
			} else if(col == AREA_W - 1)
				col = 0;
			else
				col ++;
		}
		if(wantPrev) {
			/*first non null object before (relX, relY)*/
			col = relX;
			row = relY;
			do {
				if(col == AreaFirstCol) {
					col = AreaLastCol;
					if(row == AreaFirstRow) {
						prev = 0x100;
						break;
					}
					if(row == 0)
						row = AREA_H - 1;
					else
						row --;
				} else if(col == 0)
					col = AREA_W - 1;
				else
					col --;
			} while((prev = MapObjPtr[row][col]) == -1);
			for(cur = Link[prev]; cur >= 0 && cur != obj; prev = cur, cur = Link[prev]) {
				if(GetCoordUse(cur) == LOCXYZ) {
					objY = GetY(cur);
					if(
						(objY > y) ||
						((objY == y) && (GetX(cur) >= x))
					) break;
				}
			}
			return prev;
		}
		return obj;
	}
	SetCoordXYZ(xyz, x, y, 0);
	prev = 0x100;
	for(obj = Link[prev]; obj >= 0; prev = obj, obj = Link[obj]) {
		if(GetCoordUse(obj) == LOCXYZ) {
			/*compares the 20 low bits of the coordinates, the key (y << 10 | x)*/
			if((ObjPos[obj].raw[2] & 0xf) > xyz.raw[2])
				break;
			if((ObjPos[obj].raw[2] & 0xf) == xyz.raw[2]) {
				if(ObjPos[obj].raw[1] > xyz.raw[1])
					break;
				if(ObjPos[obj].raw[1] == xyz.raw[1] && ObjPos[obj].raw[0] >= xyz.raw[0])
					break;
			}
		}
	}
	i = prev;
	if(!wantPrev)
		i = Link[i];
	return i;
}

/*next object for FindLoc: objects at the location itself, then the parts
  of double-width/double-height objects on the squares right, below and
  below-right of it whose tile covers it (LocPartOffset tells which);
  -1 at the end*/
/*C_1B09_032A*/NextLoc() {
	register int16 found;

	found = -1;
	LOC_SKIP(0);
	if(LocCursor[0] >= 0 && LOC_AT(LocCursor[0], LocTarget[0])) {
		found = LocCursor[0];
		LocPartOffset = 0;
		LOC_NEXT(0);
		if(ObjPos[found].raw[2] == LocTarget[0].raw[2])
			return found;
	}
	LocCursor[0] = -1;
	LOC_SKIP(1);
	if(LocCursor[1] >= 0 && LOC_AT(LocCursor[1], LocTarget[1])) {
		for(; LocCursor[1] >= 0 && (GetCoordUse(LocCursor[1]) || LOC_AT(LocCursor[1], LocTarget[1])); LocCursor[1] = Link[LocCursor[1]]) {
			if(GetCoordUse(LocCursor[1]) == LOCXYZ && IsTileDoubleH(TILE_FRAME(LocCursor[1]))) {
				found = LocCursor[1];
				LocPartOffset = 1;
				LOC_NEXT(1);
				return found;
			}
		}
	}
	LocCursor[1] = -1;
	LOC_SKIP(2);
	if(LocCursor[2] >= 0 && LOC_AT(LocCursor[2], LocTarget[2])) {
		for(; LocCursor[2] >= 0 && (GetCoordUse(LocCursor[2]) || LOC_AT(LocCursor[2], LocTarget[2])); LocCursor[2] = Link[LocCursor[2]]) {
			if(GetCoordUse(LocCursor[2]) == LOCXYZ && IsTileDoubleV(TILE_FRAME(LocCursor[2]))) {
				found = LocCursor[2];
				LocPartOffset = IsTileDoubleH(TILE_FRAME(LocCursor[2]))?2:1;
				LOC_NEXT(2);
				return found;
			}
		}
	}
	LocCursor[2] = -1;
	LOC_SKIP(3);
	if(LocCursor[3] >= 0 && LOC_AT(LocCursor[3], LocTarget[3])) {
		for(; LocCursor[3] >= 0 && (GetCoordUse(LocCursor[3]) || LOC_AT(LocCursor[3], LocTarget[3])); LocCursor[3] = Link[LocCursor[3]]) {
			if(GetCoordUse(LocCursor[3]) == LOCXYZ && (TileFlag[TILE_FRAME(LocCursor[3])] & 0xc0) == 0xc0) {
				found = LocCursor[3];
				LocPartOffset = 3;
				LOC_NEXT(3);
				return found;
			}
		}
	}
	LocCursor[3] = -1;
	return -1;
}

/*start iterating over the objects at (x, y, z) (and the double tiles of the
  squares right, below and below-right of it); returns the first one or -1*/
/*C_1B09_07D8*/FindLoc(register int16 x, register int16 y, int16 z) {
	int16 ret;

	if(z) {
		x &= 0xff;
		y &= 0xff;
	}
	LocCursor[0] = OBJ_FindListPos(x, y, 0);
	SetCoordXYZ(LocTarget[0], x, y, z);
	LocCursor[1] = OBJ_FindListPos((x + 1) & 0x3ff, y, 0);
	SetCoordXYZ(LocTarget[1], (x + 1) & 0x3ff, y, z);
	LocCursor[2] = OBJ_FindListPos(x, (y + 1) & 0x3ff, 0);
	SetCoordXYZ(LocTarget[2], x, (y + 1) & 0x3ff, z);
	LocCursor[3] = OBJ_FindListPos((x + 1) & 0x3ff, (y + 1) & 0x3ff, 0);
	SetCoordXYZ(LocTarget[3], (x + 1) & 0x3ff, (y + 1) & 0x3ff, z);
	ret = NextLoc();
}

/*first map object of tile (x, y) of the loaded 40x40 area, -1 outside it*/
/*C_1B09_08BC*/OBJ_MapObjAt(int16 x, int16 y) {
	register int16 dx, dy;

	dx = WrapDelta(x - AreaX) & 0x3ff;
	dy = WrapDelta(y - AreaY) & 0x3ff;
	if(dx < AREA_W && dy < AREA_H)
		return MapObjPtr[dy][dx];
	return -1;
}

/*is objNum near the middle of the map window (or the right/bottom half of a
  double tile just outside it)?*/
/*C_1B09_090B*/OBJ_IsOnScreen(int16 objNum) {
	int16 x;
	register int16 y, tile;

	x = GetX(objNum);
	y = GetY(objNum);
	tile = TILE_FRAME(objNum);
	if(MapDistance(x, MapX) <= 5 && MapDistance(y, MapY) <= 4)
		return 1;
	if(IsTileDoubleH(tile) && ((MapX + 6) & 0x3ff) == x)
		return 1;
	if(IsTileDoubleV(tile) && ((MapY + 5) & 0x3ff) == y)
		return 1;
	return 0;
}

/*find an object of type objTyp exactly at (x, y, MapZ); -1 if none*/
/*C_1B09_09C7*/OBJ_FindTypeAt(int16 objTyp, int16 x, int16 y) {
	register int16 obj, found;

	found = -1;
	for(
		obj = OBJ_FindListPos(x, y, 0);
		obj >= 0 && (
			GetCoordUse(obj) != LOCXYZ ||
			(GetX(obj) == x && GetY(obj) == y)
		);
		obj = Link[obj]
	) {
		if(GetCoordUse(obj) == LOCXYZ && GetZ(obj) == MapZ && GetType(obj) == objTyp) {
			found = obj;
			break;
		}
	}

	return found;
}

/*the next map object on the same square as objNum with objNum's type, -1 if
  none*/
/*C_1B09_0A5D*/OBJ_NextOfTypeAt(register int16 objNum) {
	struct coord pos;
	register int16 objTyp;

	if(objNum < 0 || GetCoordUse(objNum))
		return -1;
	pos.raw[0] = ObjPos[objNum].raw[0];
	pos.raw[1] = ObjPos[objNum].raw[1];
	pos.raw[2] = ObjPos[objNum].raw[2];
	objTyp = GetType(objNum);
	for(
		objNum = OBJ_LastOfSubtree(objNum), objNum = Link[objNum];
		objNum >= 0 &&
		ObjPos[objNum].raw[0] == pos.raw[0] &&
		ObjPos[objNum].raw[1] == pos.raw[1] &&
		ObjPos[objNum].raw[2] == pos.raw[2];
		objNum = OBJ_LastOfSubtree(objNum), objNum = Link[objNum]
	) {
		if(GetType(objNum) == objTyp)
			return objNum;
	}
	return -1;
}

/*start iterating over the direct contents of objNum; returns the first one or -1*/
/*C_1B09_0B1D*/FindInv(int16 objNum) {
	register int16 first, ret;

	ret = -1;
	first = Link[objNum];
	if(GetCoordUse(first) != LOCXYZ && GetAssoc(first) == objNum) {
		ret = first;
		InvCursor = Link[first];
		InvOwner = objNum;
		InvType = GetType(ret);
		InvQuality = -1;
	}
	return ret;
}

/*next direct content of the object given to FindInv (skips equipped
  items if InvSkipEquipped is set); -1 at the end*/
/*C_1B09_0B76*/NextInv() {
	register int16 ret;

	for(ret = -1; InvCursor >= 0 && ret < 0; InvCursor = Link[InvCursor]) {
		if(GetCoordUse(InvCursor) == LOCXYZ) {
			InvCursor = -1;
			break;
		}
		if(GetAssoc(InvCursor) == InvOwner) {
			if(InvSkipEquipped == 0 || GetCoordUse(InvCursor) != EQUIP || GetCoordUse(InvCursor) == CONTAINED)
				ret = InvCursor;
		}
	}
	return ret;
}

/*first object of type objTyp (0 = any), not equipped, at any depth inside
  objNum; -1 if none. NextInvOfType gives the next one.*/
/*C_1B09_0BEE*/FindInvOfType(register int16 objNum, int16 objTyp) {
	register int16 holder;

	InvOfTypeOwner = objNum;
	InvOfTypeType = objTyp;
	InvOfTypeCursor = Link[objNum];
	if(GetCoordUse(InvOfTypeCursor) == LOCXYZ || GetAssoc(InvOfTypeCursor) != objNum) {
		InvOfTypeCursor = -1;
	} else {
		for(; InvOfTypeCursor >= 0; InvOfTypeCursor = Link[InvOfTypeCursor]) {
			if(GetCoordUse(InvOfTypeCursor) == LOCXYZ) {
				InvOfTypeCursor = -1;
				break;
			}
			for(holder = GetAssoc(InvOfTypeCursor); GetCoordUse(holder) != LOCXYZ; holder = GetAssoc(holder));
			if(holder != objNum) {
				InvOfTypeOwner = -1;
				return -1;
			}
			if(GetCoordUse(InvOfTypeCursor) != EQUIP && (objTyp == 0 || GetType(InvOfTypeCursor) == objTyp))
				return InvOfTypeCursor;
		}
	}
	InvOfTypeOwner = -1;
	return -1;
}

/*next object for FindInvOfType; -1 at the end*/
/*C_1B09_0CA4*/NextInvOfType() {
	register int16 holder;

	if(InvOfTypeOwner < 0)
		return -1;
	for(InvOfTypeCursor = Link[InvOfTypeCursor]; InvOfTypeCursor >= 0; InvOfTypeCursor = Link[InvOfTypeCursor]) {
		if(GetCoordUse(InvOfTypeCursor) == LOCXYZ) {
			InvOfTypeCursor = -1;
			break;
		}
		for(holder = GetAssoc(InvOfTypeCursor); GetCoordUse(holder) != LOCXYZ; holder = GetAssoc(holder));
		if(holder != InvOfTypeOwner) {
			InvOfTypeOwner = -1;
			return -1;
		}
		if(GetCoordUse(InvOfTypeCursor) != EQUIP && (InvOfTypeType == 0 || GetType(InvOfTypeCursor) == InvOfTypeType))
			return InvOfTypeCursor;
	}
	InvOfTypeOwner = -1;
	return -1;
}

/*inventory slots objNum uses (equipped objects not counted for NPCs 0-15),
  less one when an object of type objTyp would join a stack it carries*/
/*C_1B09_0D34*/OBJ_InvSlotsUsed(int16 objTyp, int16 objNum) {
	int16 quan, cursor, owner, type, quality;
	register int16 obj, n;

	n = OBJ_CountInv(objNum);
	if(objNum < 0x10)
		n -= OBJ_CountEquipped(objNum);
	if(!QuanType(objTyp))
		return n;
	cursor = InvCursor;
	owner = InvOwner;
	type = InvType;
	quality = InvQuality;
	if((obj = FindInvType(objNum, objTyp, 0)) != -1 && (quan = QuanType(GetType(obj))) && GetQuan(obj) < 0x90)
		n--;
	InvCursor = cursor;
	InvOwner = owner;
	InvType = type;
	InvQuality = quality;
	return n;
}

/*next map object inside the rectangle given to SearchArea; -1 at the end*/
/*C_1B09_0DD7*/NextArea() {
	int16 z, ret;
	register int16 x, y;

	ret = -1;
	if(AreaCursor >= 0) {
		for(; AreaCursor != AreaEnd && AreaCursor >= 0 && ret < 0; AreaCursor = Link[AreaCursor]) {
			if(GetCoordUse(AreaCursor) == LOCXYZ) {
				x = GetX(AreaCursor) & MapCoordMask;
				y = GetY(AreaCursor) & MapCoordMask;
				z = GetZ(AreaCursor);
				if(
					z == AreaZ && (
						(AreaWrapX && (x >= AreaX1 || x <= AreaX2)) ||
						(!AreaWrapX && x >= AreaX1 && x <= AreaX2)
					) && (
						(AreaWrapY && (y >= AreaY1 || y <= AreaY2)) ||
						(!AreaWrapY && y >= AreaY1 && y <= AreaY2)
					)
				) ret = AreaCursor;
			}
		}
		if(AreaCursor == AreaEnd)
			AreaCursor = AreaEnd = -1;
	}
	return ret;
}

/*start iterating over the map objects inside the rectangle
  (x1, y1)-(x2, y2) at level MapZ (x1 > x2 or y1 > y2: across the map's
  edge); returns the first one or -1*/
/*C_1B09_0EDE*/SearchArea(int16 x1, int16 y1, int16 x2, int16 y2) {
	int16 xs, xe, ret;
	register int16 ys, ye;

	AreaX1 = x1;
	AreaX2 = x2;
	if(x1 <= x2) {
		AreaWrapX = 0;
		xs = x1;
		xe = x2;
	} else {
		AreaWrapX = 1;
		xs = 0;
		xe = 0x3ff;
	}
	AreaY1 = y1;
	AreaY2 = y2;
	if(y1 <= y2) {
		AreaWrapY = 0;
		ys = y1;
		ye = y2;
	} else {
		AreaWrapY = 1;
		ys = 0;
		ye = 0x3ff;
	}
	AreaZ = MapZ;
	AreaCursor = OBJ_FindListPos(xs, ys, 0);
	AreaEnd = OBJ_FindListPos((xe + 1) & 0x3ff, ye, 0);
	ret = NextArea();
}

/*next direct content of the FindInvType owner with the wanted type
  (and quality, unless -1); -1 at the end*/
/*C_1B09_0F7D*/NextInvType() {
	register int16 ret;

	for(ret = -1; InvCursor >= 0 && ret < 0; InvCursor = Link[InvCursor]) {
		if(GetCoordUse(InvCursor) == LOCXYZ) {
			InvCursor = -1;
			break;
		}
		if(GetAssoc(InvCursor) == InvOwner && GetType(InvCursor) == InvType) {
			if(
				(InvQuality < 0 || GetQual(InvCursor) == InvQuality) &&
				(InvSkipEquipped == 0 || GetCoordUse(InvCursor) != EQUIP || GetCoordUse(InvCursor) == CONTAINED)
			) ret = InvCursor;
		}
	}
	return ret;
}

/*first direct content of objNum of type objType and the given quality
  (-1 = any quality); -1 if none*/
/*C_1B09_1026*/FindInvType(int16 objNum, int16 objType, int16 quality) {
	register int16 first, ret;

	if(objNum < 0)
		return -1;
	ret = -1;
	first = Link[objNum];
	if(GetCoordUse(first) != LOCXYZ && GetAssoc(first) == objNum) {
		ret = first;
		InvCursor = first;
		InvOwner = objNum;
		InvType = objType;
		InvQuality = quality;
		ret = NextInvType();
	}
	return ret;
}

/*find a living NPC/monster (slot startNum..0xFF, at least 0) standing at
  (x, y, MapZ); -1 if none*/
/*C_1B09_1081*/OBJ_FindNPCAt(int16 startNum, int16 x, int16 y) {
	register int16 i, ret;

	ret = -1;
	if(startNum < 0)
		startNum = 0;
	for(i = startNum; i < 0x100; i ++) {
		if(
			ObjShapeType[i] &&
			!IsDead(i) &&
			GetX(i) == x &&
			GetY(i) == y &&
			GetZ(i) == MapZ
		) {
			ret = i;
			break;
		}
	}
	return ret;
}

/*create an object on the map or inside an object (by status & 0x18)*/
/*C_1B09_1109*/AddObj(int16 status, struct coord location, int16 shapeType, int16 shapeNum, int16 amount) {
	register int16 obj;

	if((status & 0x18) == LOCXYZ)
		obj = AddMapObj(shapeType | (shapeNum << 10), status, amount, GetCoordX(location), GetCoordY(location), GetCoordZ(location));
	else
		obj = AddInvObj(shapeType | (shapeNum << 10), status, amount, GetCoordAssoc(location));
	return obj;
}

/*create an object on the map at (x, y, z) (in front of the objects
  already there); returns the new object or -1*/
/*C_1B09_116E*/AddMapObj(int16 shapeType, int16 status, int16 amount, int16 x, register int16 y, int16 z) {
	register int16 objNum;
	int16 prev, posMask;

	posMask = z?0xff:0x3ff;
	x &= posMask;
	y &= posMask;
	if(!OBJ_IsLocLoaded(x, y, z))
		return -1;
	objNum = GetFreeObjSlot(1);
	if(objNum >= 0) {
		prev = OBJ_FindListPos(x, y, 1);
		FreeObjHead = Link[FreeObjHead];
		FreeObjCount --;
		ObjStatus[objNum] = status;
		SetLocal(objNum);
		SetXYZ(objNum, x, y, z);
		ObjShapeType[objNum] = shapeType;
		Amount[objNum] = amount;
		Link[objNum] = Link[prev];
		Link[prev] = objNum;
		x = WrapDelta(x - AreaX) & 0x3ff;
		y = WrapDelta(y - AreaY) & 0x3ff;
		if(x < AREA_W && y < AREA_H)
			MapObjPtr[y][x] = objNum;
	}
	return objNum;
}

/*number of objects directly inside objNum*/
/*C_1B09_129A*/OBJ_CountInv(int16 objNum) {
	register int16 count, obj;

	count = 0;
	for(obj = FindInv(objNum); obj >= 0; obj = NextInv())
		count ++;
	return count;
}

/*number of objects equipped by objNum*/
/*C_1B09_12BD*/OBJ_CountEquipped(int16 objNum) {
	register int16 obj, count;

	count = 0;
	for(obj = FindInv(objNum); obj >= 0; obj = NextInv())
		if(GetCoordUse(obj) == EQUIP)
			count ++;
	return count;
}

/*create an object inside owner (an NPC or a container); refused (-1) when
  the owner is not loaded, its inventory is full (16 slots, OBJ_InvSlotsUsed),
  or the weight is too much for its strength (GetStr: 20 times for all it
  carries, 10 times for what it wears); stackable objects are added to an
  existing stack when possible (QuanType 4: the whole word). Returns the
  object.*/
/*C_1B09_12EE*/AddInvObj(int16 shapeType, int16 status, int16 amount, register int16 owner) {
	int16 root, obj, strength, weight, refused, coordUse, numInven;
	int16 quanType, type, count;
	register int16 objNum;

	strength = -1;
	refused = 0;
	if(owner == -1)
		return -1;
	for(root = owner; GetCoordUse(root) != LOCXYZ; root = GetAssoc(root));
	if(!OBJ_IsLocLoaded(GetX(root), GetY(root), GetZ(root)))
		return -1;
	quanType = QuanType(type = shapeType & 0x3ff);
	if(quanType == 4)
		count = amount;
	else if(quanType == 2)
		count = amount & 0xff;
	else
		count = 1;
	STAT_RefreshInventory(owner);
	numInven = OBJ_InvSlotsUsed(type, owner);
	if(StatusDisplay == 0x92 && Party[StatusMember] == owner)
		StatusDirty = 1;
	coordUse = status & 0x18;
	if(numInven >= 0x10 && (coordUse == INVEN || coordUse == CONTAINED) && GetType(owner) != 0x155) {
		STAT_RefreshInventory(Party[StatusMember]);
		return -1;
	}
	STAT_ComputeWeights(owner);
	weight = GetTypeWeight(type, count);
	if(IsNPC(owner))
		strength = GetStr(owner);
	if(strength != -1 && strength * 20 < weight + WeightInven + WeightEquip) {
		STAT_RefreshInventory(Party[StatusMember]);
		return -1;
	}
	if(coordUse == EQUIP && strength != -1 && strength * 10 < weight + WeightEquip) {
		STAT_RefreshInventory(Party[StatusMember]);
		return -1;
	}
	SubMov(owner, 3);
	if(quanType && coordUse != EQUIP) {
		for(obj = FindInvType(owner, shapeType, -1); obj != -1; obj = NextInvType()) {
			if(GetQuan(obj) + amount < 0x90 && GetCoordUse(obj) != EQUIP && IsHatched(obj) == 0)
				break;
		}
		if(obj != -1) {
			if(quanType == 4)
				Amount[obj] += amount;
			else
				AddQuan(obj, amount);
			STAT_RefreshInventory(Party[StatusMember]);
			return obj;
		}
		if(numInven >= 0x10) {
			STAT_RefreshInventory(Party[StatusMember]);
			return -1;
		}
	}
	objNum = GetFreeObjSlot(1);
	if(owner != StatusMember)
		STAT_ComputeWeights(Party[StatusMember]);
	if(objNum >= 0 && !refused) {
		FreeObjHead = Link[FreeObjHead];
		FreeObjCount --;
		ObjStatus[objNum] = status;
		SetLocal(objNum);
		SetAssoc(objNum, owner);
		ObjShapeType[objNum] = shapeType;
		Amount[objNum] = amount;
		Link[objNum] = Link[owner];
		Link[owner] = objNum;
	}
	STAT_RefreshInventory(Party[StatusMember]);
	return objNum;
}

/*create a temporary monster of type objType at (x, y, z) in one of the
  slots 0xC0-0xFF; returns it or -1 if none is free*/
/*C_1B09_15C7*/AddMonster(int16 objType, int16 x, register int16 y, int16 z) {
	register int16 objNum;
	int16 prev, posMask;

	posMask = z?0xff:0x3ff;
	x &= posMask;
	y &= posMask;
	objNum = -1;
	if(FreeMonsterCount > 0)
		objNum = FreeMonsterHead;
	if(objNum >= 0 && objNum <= 0xff) {
		prev = OBJ_FindListPos(x, y, 1);
		FreeMonsterHead = Link[FreeMonsterHead];
		FreeMonsterCount --;
		ObjStatus[objNum] = 0x20;
		NPCStatus[objNum] = 0;
		NPCMode[objNum] = 8;
		SetXYZ(objNum, x, y, z);
		ObjShapeType[objNum] =
		OrigShapeType[objNum] = objType;
		Amount[objNum] = 1;
		Link[objNum] = Link[prev];
		Link[prev] = objNum;
		x = WrapDelta(x - AreaX) & 0x3ff;
		y = WrapDelta(y - AreaY) & 0x3ff;
		if(x < AREA_W && y < AREA_H)
			MapObjPtr[y][x] = objNum;
		if(objType == 0x17f)
			OBJ_JoinCreepingCacti();
	}
	return objNum;
}

/*the node preceding objNum in the Link[] chain (-1 if not found)*/
/*C_1B09_1704*/static OBJ_FindPrevLink(register int16 objNum) {
	register int16 prev;

	if(GetCoordUse(objNum) == LOCXYZ)
		prev = OBJ_FindListPos(GetX(objNum), GetY(objNum), 1);
	else
		prev = GetAssoc(objNum);
	for(; prev >= 0 && Link[prev] != objNum; prev = Link[prev]);

	return prev;
}

/*objNum (at (x, y, z)) leaves its place in the list: if it was the first
  object of its square in MapObjPtr, the next map object of the same
  square (after nextObj's contents) takes its place, else -1*/
/*C_1B09_1772*/static OBJ_FixMapObjPtr(int16 x, int16 y, int16 z, int16 objNum, register int16 nextObj) {
	register int16 relY;
	int16 relX, posMask;

	if(GetCoordUse(objNum) == LOCXYZ) {
		posMask = z?0xff:0x3ff;
		relX = WrapDelta(x - AreaX) & posMask;
		relY = WrapDelta(y - AreaY) & posMask;
		if(relX < AREA_W && relY < AREA_H && MapObjPtr[relY][relX] == objNum) {
			for(; nextObj >= 0 && GetCoordUse(nextObj); nextObj = Link[nextObj]);
			if(nextObj < 0 || GetCoordUse(nextObj) || GetX(nextObj) != x || GetY(nextObj) != y || nextObj == objNum)
				MapObjPtr[relY][relX] = -1;
			else
				MapObjPtr[relY][relX] = nextObj;
		}
	}
}

/*delete an object (a creeping cactus 0x17f: the others rejoin,
  OBJ_JoinCreepingCacti); the
  contents of a container are dropped where it was (or put in its owner),
  those of anything else deleted; the slot goes back to the monster or
  object free list*/
/*C_1B09_1872*/DeleteObj(register int16 objNum) {
	int16 objTyp, prev;
	register int16 content;

	if(objNum < 0)
		return;
	LastSpilledObj = -1;
	if(StatusDisplay == 0x92 && OBJ_IsInside(objNum, Party[StatusMember]))
		StatusDirty = 1;
	objTyp = GetType(objNum);
	/*spill the contents*/
	for(content = FindInv(objNum); content >= 0; content = NextInv()) {
		LastSpilledObj = content;
		if(GetCoordUse(objNum) == LOCXYZ && IsContainerType(objTyp)) {
			MoveObj(content, GetX(objNum), GetY(objNum), GetZ(objNum));
		} else if(IsContainerType(objTyp)) {
			InsertObj(content, GetAssoc(objNum), GetCoordUse(objNum));
		} else {
			DeleteObj(content);
			LastSpilledObj = InvCursor;
		}
	}
	ObjShapeType[objNum] = 0;
	prev = OBJ_FindPrevLink(objNum);
	if(prev >= 0) {
		Link[prev] = Link[objNum];
		if(IsNPC(objNum)) {
			Link[objNum] = FreeMonsterHead;
			FreeMonsterHead = objNum;
			FreeMonsterCount ++;
		} else {
			Link[objNum] = FreeObjHead;
			FreeObjHead = objNum;
			FreeObjCount ++;
		}
	}
	OBJ_FixMapObjPtr(GetX(objNum), GetY(objNum), GetZ(objNum), objNum, Link[prev]);
	if(objTyp == 0x17f && IsNPC(objNum))
		OBJ_JoinCreepingCacti();
}

/*delete the local objects and creatures farther than 20 squares from
  (x, y) or not on level z (x < 0: all of them), with the parts 0x172/0x173
  of a creature 0x174; eggs 0x1d2 can hatch again*/
/*C_1B09_1A1D*/OBJ_PurgeFarLocals(int16 x, int16 y, int16 z) {
	int16 objTyp, objX, objY, objZ, owner;
	register int16 next, obj;

	if(x >= 0) {
		x &= MapCoordMask;
		y &= MapCoordMask;
	}
	/*delete local monsters*/
	for(obj = 0xc0; obj < 0x100; obj ++) {
		if(ObjShapeType[obj]) {
			objX = GetX(obj);
			objY = GetY(obj);
			objZ = GetZ(obj);
			if(x < 0 || MapDistance(objX, x) >= 20 || MapDistance(objY, y) >= 20 || objZ != z) {
				if(IsLocal(obj)) {
					if(GetType(obj) == 0x174) {
						for(next = Link[0x100]; next >= 0; next = Link[next]) {
							objTyp = GetType(next);
							if((objTyp == 0x173 || objTyp == 0x172) && GetQual(next) == obj)
								ObjShapeType[next] = 0;
						}
					}
					DeleteObj(obj);
				}
			}
		}
	}
	/*local objects and eggs*/
	for(obj = Link[0x100]; obj >= 0; obj = next) {
		next = Link[obj];
		objTyp = GetType(obj);
		if(GetCoordUse(obj) != LOCXYZ)
			continue;
		objX = GetX(obj);
		objY = GetY(obj);
		objZ = GetZ(obj);
		if(ObjShapeType[obj] == 0 || x < 0 || MapDistance(objX, x) >= 20 || MapDistance(objY, y) >= 20 || objZ != z) {
			if((IsLocal(obj) && objTyp != 0x1cd) || ObjShapeType[obj] == 0) {
				if(objTyp == 0x173 && (owner = GetQual(obj)) && ObjShapeType[owner] == 0x174)
					continue;
				if(obj == CharmedNPC)
					CharmedNPC = 0;
				DeleteObj(obj);
				if(LastSpilledObj >= 0)
					next = LastSpilledObj;
			} else if(objTyp == 0x1d2 && IsHatched(obj)) {
				ClrHatched(obj);
			}
		}
	}
}

/*replace NPC objNum by a dead body (its shape, quality = the NPC number)
  holding its inventory (equipped objects flagged 0x40); objects 0xef it
  carries are dropped, objects 0x14e-0x150 first at its place moved over
  the body; the NPC goes to level 6. Returns the body, -1 if none.*/
/*C_1B09_1C43*/OBJ_MakeDeadBody(register int16 objNum) {
	int16 body, firstAtLoc, x, y, z, type;
	register int16 content;

	body = -1;
	if(objNum < 0)
		return -1;
	if(ObjShapeType[objNum]) {
		x = GetX(objNum);
		y = GetY(objNum);
		z = GetZ(objNum);
		firstAtLoc = FindLoc(x, y, z);

		AllowSubmissionMove = 1;
		body = AddMapObj(ObjShapeType[objNum], ObjStatus[objNum], QualQuan(objNum, 0), x, y, z);
		if(body >= 0) {
			if(objNum < 0xc0)
				ClrLocal(body);
			else
				;
			for(content = FindInv(objNum); content >= 0; content = FindInv(objNum)) {
				if(GetType(content) == 0xef) {
					MoveObj(content, x, y, z);
				} else if(GetCoordUse(content) == EQUIP) {
					if(InsertObj(content, body, CONTAINED) == -1)
						CON_printf(/*1612*/"Oh no!\n");
					ObjStatus[content] |= 0x40;
				} else {
					if(InsertObj(content, body, CONTAINED) == -1)
						CON_printf("Oh no!\n");
				}
				ClrLocal(content);
			}
			if((type = GetType(firstAtLoc)) >= 0x14e && type <= 0x150)
				MoveObj(firstAtLoc, x, y, z);
			MoveObj(objNum, objNum, objNum, 6);
			NPCStatus[objNum] |= 0x10;
			if(objNum >= 0xc0)
				ObjShapeType[objNum] = 0;
		}
		AllowSubmissionMove = 0;
	}

	return body;
}

/*bring the NPC of a dead body back where the body is, with the body's
  contents (equipped again when flagged 0x40), and delete the body;
  returns the NPC*/
/*C_1B09_1DF5*/OBJ_ReviveFromBody(int16 body) {
	register int16 root, content;
	int16 npc, coordUse;

	if(body < 0)
		return -1;

	npc = GetQual(body);
	for(root = body; GetCoordUse(root) != LOCXYZ; root = GetAssoc(root));
	MoveObj(npc, GetX(root), GetY(root), GetZ(root));
	for(content = FindInv(body); content >= 0; content = FindInv(body)) {
		if(IsHatched(content)) {
			coordUse = EQUIP;
			ClrHatched(content);
		} else
			coordUse = INVEN;
		if(InsertObj(content, npc, coordUse) == -1) {
			CON_printf("Oh no!\n");
			break;
		}
	}
	DeleteObj(body);

	return npc;
}

/*is objNum (directly or not) inside container?*/
/*C_1B09_1ECF*/OBJ_IsInside(register int16 objNum, int16 container) {
	register int16 ret;

	ret = 0;
	while(GetCoordUse(objNum) != LOCXYZ) {
		objNum = GetAssoc(objNum);
		if(objNum == container) {
			ret = 1;
			break;
		}
	}
	return ret;
}

/*last node of objNum's subtree (its contents, recursively), objNum if empty*/
/*C_1B09_1F01*/static OBJ_LastOfSubtree(int16 objNum) {
	register int16 i, last;

	if(objNum < 0)
		return -1;
	for(i = last = objNum; Link[i] >= 0 && GetCoordUse(Link[i]) != LOCXYZ; ) {
		i = Link[i];
		if(OBJ_IsInside(i, objNum))
			last = i;
	}
	return last;
}

/*move objNum (with its contents) to (toX, toY, toZ) on the map; returns
  objNum, or -1 when it is not loaded. Afterwards: the pocketwatch 0x62 becomes 0x5b,
  the creeping cacti 0x17f join up (OBJ_JoinCreepingCacti), and a rail car
  0x19a pushed into water (tile 0x10-0x17, nothing else there) floats down the current
  (tiles 0x10-0x15 move it east/south) to a free shore square (tile 0x16/
  0x17), where it breaks into a smashed rail car 0x1a0, or sinks when the canal
  pumps run (TalkFlags[0x76] & 0x20 and & 0x40)*/
/*C_1B09_1F52*/MoveObj(register int16 objNum, int16 toX, int16 toY, int16 toZ) {
	int16 insertAfter, prev, last, oldX, oldY, objTyp, posMask;
	int16 i, other, x, y, z;
	register int16 dir;

	x = toX;
	y = toY;
	z = toZ;
	if(objNum < 0)
		return -1;
	if(StatusDisplay == 0x92 && OBJ_IsInside(objNum, Party[StatusMember]))
		StatusDirty = 1;
	objTyp = GetType(objNum);
	if(IsNPC(objNum))
		;
	posMask = toZ?0xff:0x3ff;
	toX &= posMask;
	toY &= posMask;
	oldX = GetX(objNum);
	oldY = GetY(objNum);
	if(
		MapDistance(oldX, MapX) <= 5 && MapDistance(oldY, MapY) <= 5 &&
		MapDistance(toX, MapX) <= 5 && MapDistance(toY, MapY) <= 5
	) VisibilityDirty = 1;
	prev = OBJ_FindPrevLink(objNum);
	if(prev < 0)
		return -1;

	/*unlink the subtree prev->[objNum..last] and insert it at the new place*/
	last = OBJ_LastOfSubtree(objNum);
	Link[prev] = Link[last];
	OBJ_FixMapObjPtr(GetX(objNum), GetY(objNum), GetZ(objNum), objNum, Link[prev]);
	insertAfter = OBJ_FindListPos(toX, toY, 1);
	Link[last] = Link[insertAfter];
	Link[insertAfter] = objNum;
	SetCoordUse(objNum, LOCXYZ);
	SetXYZ(objNum, toX, toY, toZ);
	toX = WrapDelta(toX - AreaX) & 0x3ff;
	toY = WrapDelta(toY - AreaY) & 0x3ff;
	if(toX < AREA_W && toY < AREA_H)
		MapObjPtr[toY][toX] = objNum;
	if(IsNPC(objNum) && MoveAppliesTerrain && toZ == MapZ) {
		NPCFlag[objNum] &= ~0x08;
		ApplyTerrainEffects(objNum, toX, toY);/*sic: the area-relative coordinates*/
	}

	if(objTyp == 0x62) {
		ObjShapeType[objNum] = (ObjShapeType[objNum] & 0xfc00) + 0x5b;
	} else if(objTyp == 0x17f && IsNPC(objNum)) {
		OBJ_JoinCreepingCacti();
	} else if(objTyp == 0x19a && IsWaterTile(GetTileAtXYZ(x, y, z))) {
		other = FindLoc(x, y, z);
		if((other = NextLoc()) == -1) {
			/*the direction it was pushed*/
			for(
				dir = 0;
				((oldX + DirIncrX[dir]) & 0x3ff) != x &&
				((oldY + DirIncrY[dir]) & 0x3ff) != y &&
				dir < 8;
				dir ++
			);
			if(dir == 8) {
				CON_printf(/*161A*/"BOOM!!!\n");
			} else {
				ShowObjects();
				DisplayTick();
				TimerDelay(0x5744aL);
				if(!(TalkFlags[0x76] & 0x20) || !(TalkFlags[0x76] & 0x40)) {
					MUS_SweepSfx();
					switch(GetTileAtXYZ(x, y, z)) {
						case 0x10: case 0x12:
							x = (x + 1) & 0x3ff;
							y = (y + 1) & 0x3ff;
							break;
						case 0x11: case 0x13:
							y = (y + 1) & 0x3ff;
							break;
						case 0x14: case 0x15:
							x = (x + 1) & 0x3ff;
					}
					i = -1;
					for(;;) {
						if(i == 8)
							break;
						i ++;
						oldX = (x + DirIncrX[(i + dir) % 8]) & 0x3ff;
						oldY = (y + DirIncrY[(i + dir) % 8]) & 0x3ff;
						if(
							(GetTileAtXYZ(oldX, oldY, z) == 0x16 || GetTileAtXYZ(oldX, oldY, z) == 0x17) &&
							(other = OBJ_MapObjAt(oldX, oldY)) == -1
						) break;
					}
					if(i < 8)
						AddMapObj(0x1a0, 0, 1, oldX, oldY, z);
					CON_printf(/*1623*/"Crash!!\n");
				} else {
					MUS_SplashSfx();
					CON_printf(/*162C*/"Splash!!\n");
				}
				DeleteObj(objNum);
				objNum = -1;
			}
		}
	}

	return objNum;
}

/*move objNum (with its contents) into destNum (an NPC or a container) as
  objLoc (CONTAINED/INVEN/EQUIP); refused (-1) when the inventory is full,
  the weight too much (GetStr, as AddInvObj; not while AllowSubmissionMove)
  or destNum is inside objNum; a stackable object joins an existing stack
  (and is deleted). Returns the object.*/
/*C_1B09_239C*/InsertObj(register int16 objNum, register int16 destNum, int16 objLoc) {
	int16 prev, last, objTyp, coordUse, obj, quantity, strength, weight, numInven;
	int16 cursor, owner, type, quality;

	strength = -1;
	if(objNum < 0 || destNum < 0)
		return -1;
	STAT_RefreshInventory(destNum);
	coordUse = objLoc & 0x18;
	numInven = OBJ_InvSlotsUsed(GetType(objNum), destNum);
	if(numInven >= 0x10 && (coordUse == INVEN || coordUse == CONTAINED) && !QuanType(objTyp)/*sic: not set yet*/ && GetType(destNum) != 0x155 && !AllowSubmissionMove) {
		STAT_RefreshInventory(Party[StatusMember]);
		return -1;
	}
	weight = GetWeight(objNum);
	quantity = GetQuan(objNum);
	if(IsNPC(destNum) && !AllowSubmissionMove) {
		STAT_ComputeWeights(destNum);
		strength = GetStr(destNum);
		if(strength != -1 && GetCoordUse(objNum) == LOCXYZ && strength * 20 < weight + WeightInven + WeightEquip) {
			STAT_RefreshInventory(Party[StatusMember]);
			return -1;
		}
		if(coordUse == EQUIP && GetCoordUse(objNum) != EQUIP && strength * 10 < weight + WeightEquip) {
			STAT_RefreshInventory(Party[StatusMember]);
			return -1;
		}
	}
	objTyp = GetType(objNum);
	if(QuanType(objTyp) && coordUse != EQUIP) {
		cursor = InvCursor;
		owner = InvOwner;
		type = InvType;
		quality = InvQuality;
		for(obj = FindInvType(destNum, objTyp, -1); obj != -1; obj = NextInvType()) {
			if(GetQuan(obj) + quantity < 0x90 && GetCoordUse(obj) != EQUIP && IsHatched(obj) == 0)
				break;
		}
		if(obj != -1) {
			AddQuan(obj, quantity);
			DeleteObj(objNum);
			STAT_RefreshInventory(Party[StatusMember]);
			return obj;
		}
		if(numInven >= 0x10 && !AllowSubmissionMove) {
			STAT_RefreshInventory(Party[StatusMember]);
			return -1;
		}
		InvCursor = cursor;
		InvOwner = owner;
		InvType = type;
		InvQuality = quality;
	}
	if(objNum == destNum || OBJ_IsInside(destNum, objNum)) {
		STAT_RefreshInventory(Party[StatusMember]);
		return -1;
	}
	prev = OBJ_FindPrevLink(objNum);
	last = OBJ_LastOfSubtree(objNum);
	Link[prev] = Link[last];
	OBJ_FixMapObjPtr(GetX(objNum), GetY(objNum), GetZ(objNum), objNum, Link[prev]);
	Link[last] = Link[destNum];
	Link[destNum] = objNum;
	SetCoordUse(objNum, objLoc);
	SetAssoc(objNum, destNum);
	if(QuanType(objTyp))
		SetQuan(objNum, quantity);
	STAT_RefreshInventory(Party[StatusMember]);
	if(GetType(objNum) == 0x5b)
		ObjShapeType[objNum] = (ObjShapeType[objNum] & 0xfc00) + 0x62;
	return objNum;
}

/*move the map object objNum after the last object of its square in the
  list (only objNum itself is relinked, not what it holds); returns objNum,
  -1 when it is not on the map, nothing when it is already the last*/
/*C_1B09_268D*/OBJ_MoveLastOnSquare(register int16 objNum) {
	int16 x, y, z, prev, last, end;
	register int16 next;

	if(objNum < 0)
		return -1;
	if(GetCoordUse(objNum))
		return -1;
	x = GetX(objNum);
	y = GetY(objNum);
	z = GetZ(objNum);
	last = OBJ_LastOfSubtree(objNum);
	next = Link[last];
	if(GetX(next) == x && GetY(next) == y && GetZ(next) == z) {
		do {
			end = OBJ_LastOfSubtree(next);
			next = Link[end];
		} while(GetX(next) == x && GetY(next) == y && GetZ(next) == z);
		if((prev = OBJ_FindPrevLink(objNum)) < 0)
			return -1;
		Link[prev] = Link[objNum];
		OBJ_FixMapObjPtr(x, y, z, objNum, Link[objNum]);
		Link[objNum] = next;
		Link[end] = objNum;
		return objNum;
	}
}

/*number of items an object stands for (stack quantity, at least 1)*/
/*C_1B09_27F3*/static OBJ_GetCount(int16 objNum) {
	register int16 quantity, quanType;

	quanType = QuanType(ObjShapeType[objNum]);
	quantity = GetQuan(objNum);
	if(quanType == 2) {
		if(quantity == 0)
			SetQuan(objNum, 1);
		else
			return quantity;
	}
	return 1;
}

/*give owner amount (quality << 8 | quantity) objects of objType: as one
  stack (QuanType 2) or one by one; what does not fit is dropped at the
  owner's place (unless GiveObjRefuses: -1) and the owner moved over it. Returns
  the last object made.*/
/*C_1B09_2837*/GiveObj(int16 owner, int16 objType, int16 amount) {
	int16 quality, quantity, quanType, coordUse, dropped, x, y;
	register int16 root, obj;

	dropped = 0;
	for(root = owner; GetCoordUse(root) != LOCXYZ; root = GetAssoc(root));
	if(owner < 0)
		return -1;
	x = GetX(root);
	y = GetY(root);
	STAT_RefreshInventory(owner);
	quality = amount >> 8;
	quantity = (unsigned char)amount;
	quanType = QuanType(objType);
	if(IsNPC(owner))
		coordUse = INVEN;
	else
		coordUse = CONTAINED;
	if(quanType == 2) {
		if((obj = AddInvObj(objType, coordUse, QualQuan(quality, quantity), owner)) == -1) {
			if(GiveObjRefuses)
				return -1;
			obj = AddMapObj(objType, 0, QualQuan(quality, quantity), x, y, MapZ);
			dropped = 1;
		}
	} else {
		for(; quantity > 0; quantity --) {
			if((obj = AddInvObj(objType, coordUse, QualQuan(quality, 0), owner)) < 0) {
				if(GiveObjRefuses)
					return -1;
				dropped = 1;
				obj = AddMapObj(objType, 0, QualQuan(quality, 0), x, y, MapZ);
			}
			if(obj >= 0)
				ClrLocal(obj);
		}
	}
	if(dropped)
		MoveObj(owner, x, y, MapZ);
	return obj;
}

/*take amount (quality << 8 | quantity; quality 0 = any) objects of
  objType from inside owner; returns the object after the last one looked
  at (-1 at the end)*/
/*C_1B09_29AE*/TakeObj(int16 owner, int16 objType, int16 amount) {
	int16 count, next, quality, quantity, changed;
	register int16 quanType, obj;

	changed = 0;
	if(owner < 0)
		return -1;
	quality = amount >> 8;
	quantity = (unsigned char)amount;
	quanType = QuanType(objType);
	if(quanType != 4)
		amount = quantity;
	obj = FindInv(owner);
	while(obj >= 0 && GetCoordUse(obj) != LOCXYZ && OBJ_IsInside(obj, owner)) {
		if(GetType(obj) == objType) {
			if(quality == 0 || GetQual(obj) == quality)  {
				if(quanType) {
					count = quanType == 2 ? OBJ_GetCount(obj) : Amount[obj];
					if(amount >= count) {
						amount -= count;
						next = Link[obj];
						DeleteObj(obj);
						obj = next;
						changed = 1;
						continue;
					} else {
						if(quanType == 2)
							SubQuan(obj, amount);
						else if(quanType)
							Amount[obj] -= amount;
						amount = 0;
						changed = 1;
					}
				} else {
					amount --;
					next = Link[obj];
					DeleteObj(obj);
					obj = next;/*sic: the loop then skips this object*/
					changed = 1;
				}
				if(amount <= 0)
					break;
			}
		}
		obj = Link[obj];
	}
	if(changed && StatusDisplay == 0x92 && Party[StatusMember] == owner)
		StatusDirty = 1;
	return obj;
}

/*count the items of objType that objNum possesses (recursively)*/
/*C_1B09_2B0C*/TestObj(int16 objNum, int16 objType) {
	register int16 obj, total;

	total = 0;
	obj = FindInv(objNum);
	while(obj >= 0 && GetCoordUse(obj) != LOCXYZ && OBJ_IsInside(obj, objNum)) {
		if(GetType(obj) == objType)
			total += OBJ_GetCount(obj);
		obj = Link[obj];
	}
	return total;
}

/*move amount items of objType from oldOwner to newOwner*/
/*C_1B09_2B5C*/TransferObj(int16 oldOwner, int16 newOwner, int16 objType, int16 amount) {
	if(TakeObj(oldOwner, objType, amount) >= 0)
		GiveObj(newOwner, objType, amount);
}

/*copy object objNum to entry index of the objblk buffer and free it
  (ObjShapeType = 0). BLK_OBJNUM() maps buffer entries back to
  object numbers so that a CONTAINED object can store its container as a
  buffer index; NPC-held items keep the NPC number.*/
/*C_1B09_2B81*/static __ObjSerialize(register int16 objNum, register int16 index) {
	struct coord containerPos;
	int16 containerIdx;

	if(GetCoordUse(objNum) != LOCXYZ) {
		if((int16)GetAssoc(objNum) < 0x100 && (int16)GetAssoc(objNum) >= 0) {
			if(GetCoordUse(objNum) == CONTAINED)
				SetCoordUse(objNum, INVEN);
		} else {
			SetCoordUse(objNum, CONTAINED);
		}
	}
	BLK_STATUS(index) = ObjStatus[objNum];
	BLK_POS(index) = ObjPos[objNum];
	BLK_SHAPE(index) = ObjShapeType[objNum];
	BLK_AMOUNT(index) = Amount[objNum];
	if(GetCoordUse(objNum) == CONTAINED) {
		for(
			containerIdx = index;
			BLK_OBJNUM(containerIdx) != GetAssoc(objNum);
			containerIdx --
		);
		SetCoordAssoc(containerPos, containerIdx);
		BLK_POS(index) = containerPos;
	}
	ObjShapeType[objNum] = 0;
	if(index + 1 >= 0x800)
		CON_printf(/*1636*/"Error-too many objects in region!\n");
}

/*delete every "objblkXY.tmp" file flagged in ObjBlkTmpPending*/
/*C_1B09_2CD2*/OBJ_DeleteTmpBlocks() {
	int16 areaId;

	for(areaId = 0; areaId < (8*8 + 5); areaId ++) {
		if(ObjBlkTmpPending[areaId]) {
			__Tmp_file[15] = (areaId & 7) + 'A';
			__Tmp_file[16] = (areaId >> 3) + 'A';
			OSI_delete(__Tmp_file);
		}
	}
}

/*write the numObj entries of the objblk buffer to __Tmp_file (whose
  letters were set by the caller): a word count then 8 bytes per object.
  If the disk gets full (DISK_full sets DiskFullChoice): on 'R' (restore)
  nothing more is written; on 'S' (save) every pending .tmp file is renamed
  over the real file, this block is written directly to __Sav_file and a
  save game is requested (DiskFullChoice = 'G').*/
/*C_1B09_2D18*/static OBJ_WriteBlockFile(int16 numObj, int16 areaId) {
	register int16 fd;
	int16 i;
	char savLetters[2], tmpLetters[2];

	ObjBlkTmpPending[areaId] = 1;
	fd = OSI_create(__Tmp_file);
	OSI_write(fd, 0, 2, &numObj);
	if(DiskFullChoice != 'S' && DiskFullChoice != 'R')
		OSI_write(fd, -1, (int32)numObj << 3, ScratchBuf);
	if(DiskFullChoice == 'S') {
		tmpLetters[0] = __Tmp_file[15];
		tmpLetters[1] = __Tmp_file[16];
		savLetters[0] = __Sav_file[15];
		savLetters[1] = __Sav_file[16];
		OSI_close(fd);
		for(i = 0; i < (8*8 + 5); i ++) {
			if(ObjBlkTmpPending[i]) {
				__Sav_file[15] = __Tmp_file[15] = (i & 7) + 'A';
				__Sav_file[16] = __Tmp_file[16] = (i >> 3) + 'A';
				OSI_delete(__Sav_file);
				if(i != areaId)
					OSI_rename(__Tmp_file, __Sav_file);
				ObjBlkTmpPending[i] = 0;
			}
		}
		__Tmp_file[15] = __Sav_file[15] = tmpLetters[0];
		__Tmp_file[16] = __Sav_file[16] = tmpLetters[1];
		fd = OSI_create(__Sav_file);
		OSI_write(fd, 0, 2, &numObj);
		OSI_write(fd, -1, (int32)numObj << 3, ScratchBuf);
		__Sav_file[15] = savLetters[0];
		__Sav_file[16] = savLetters[1];
		DiskFullChoice = 'G';
		MapAnimEnabled = 1;
	}
	OSI_close(fd);
}

/*unload the outdoor area areaId (yyyxxx, 128x128 tiles at z 0): every
  object there and its contents, including the inventories of the NPCs
  standing there (the NPCs themselves stay), is serialized, removed from
  the chain and freed (the shape of the first freed object cleared), then
  written to the area's .tmp file*/
/*C_1B09_2E82*/static OBJ_UnloadArea(int16 areaId) {
	register int16 obj, numObj;
	int16 prev, last, baseX, baseY, freed;

	if(areaId < 0 || DiskFullChoice == 'R')
		return;
	numObj = 0;
	baseX = (areaId & 7) << 7;
	baseY = (areaId & 0x38) << 4;
	prev = 0x100;
	obj = Link[0x100];
	while(numObj < 0x800 && obj >= 0) {
		if(GetCoordUse(obj) == LOCXYZ && GetZ(obj) == 0 &&
			(GetX(obj) & 0x380) == baseX && (GetY(obj) & 0x380) == baseY
		) {
			freed = 1;
			if(IsNPC(obj)) {
				prev = obj;
				freed = 0;
			} else {
				BLK_OBJNUM(numObj) = obj;
				__ObjSerialize(obj, numObj++);
				FreeObjCount ++;
			}
			last = obj;
			obj = Link[obj];
			while(obj >= 0 && GetCoordUse(obj) != LOCXYZ) {
				if(numObj < 0x800) {
					BLK_OBJNUM(numObj) = obj;
					__ObjSerialize(obj, numObj++);
				}
				FreeObjCount ++;
				last = obj;
				obj = Link[obj];
			}
			Link[last] = FreeObjHead;
			FreeObjHead = Link[prev];
			if(freed)
				ObjShapeType[FreeObjHead] = 0;
			Link[prev] = obj;
		} else {
			prev = obj;
			obj = Link[obj];
		}
	}
	__Tmp_file[15] = (areaId & 7) + 'A';
	__Tmp_file[16] = (areaId >> 3) + 'A';
	OBJ_WriteBlockFile(numObj, areaId);
}

/*order of two objects in the chain for the merge: < 0 if obj0 must come
  first (-1 stands for the end of a list). An NPC-held object sorts
  before a map object (OBJ_RelinkHeldItems moves it behind its owner later);
  otherwise objects are compared by the location of their outermost
  container/owner: y, then x, then higher z first.*/
/*C_1B09_2FE9*/static OBJ_CompareOrder(int16 obj0, int16 obj1) {
	register int16 root0, root1;
	int16 dist;

	if(obj0 == -1)
		return 1;
	if(obj1 == -1)
		return -1;
	root0 = obj0; root1 = obj1;
	while(GetCoordUse(root0) == CONTAINED)
		root0 = GetAssoc(root0);
	while(GetCoordUse(root1) == CONTAINED)
		root1 = GetAssoc(root1);
	if(GetCoordUse(root0) != LOCXYZ && GetCoordUse(obj1) == LOCXYZ)
		return -1;
	if(GetCoordUse(root1) != LOCXYZ && GetCoordUse(obj0) == LOCXYZ)
		return 1;
	if(IsHeldByNPC(root0))
		root0 = GetAssoc(root0);
	if(IsHeldByNPC(root1))
		root1 = GetAssoc(root1);
	dist = GetY(root0) - GetY(root1);
	if(dist == 0)
		dist = GetX(root0) - GetX(root1);
	if(dist == 0)
		dist = GetZ(root1) - GetZ(root0);
	if(root0 == root1) {
		if(GetCoordUse(root0) == LOCXYZ)
			dist = -1;
		else if(GetCoordUse(root1) == LOCXYZ)
			dist = 1;
	}
	return dist;
}

/*after loading blocks: move every NPC-held (INVEN/EQUIP) object that is
  not inside its owner's subtree (with its own contents) behind its owner*/
/*C_1B09_314A*/static OBJ_RelinkHeldItems() {
	register int16 obj, walk;
	int16 prev, last, owner;

	prev = 0x100;
	while(Link[prev] >= 0) {
		obj = Link[prev];
		if(IsHeldByNPC(obj)) {
			owner = GetAssoc(obj);
			for(
				walk = Link[owner];
				walk >= 0 && GetCoordUse(walk) != LOCXYZ && walk != obj;
				walk = Link[walk]
			);
			if(walk != obj) {
				last = OBJ_LastOfSubtree(obj);
				owner = OBJ_LastOfSubtree(owner);
				Link[prev] = Link[last];
				Link[last] = Link[owner];
				Link[owner] = obj;
				continue;
			}
		}
		prev = obj;
	}
}

/*allocate numObj objects from the objblk buffer (as long as slots are
  free), translate the CONTAINED buffer indexes back to object numbers,
  then merge the new (sorted) chain into the main list. Objects 0xe4 get
  frame 7 at night with power (TalkFlags[0x73] & 0x10), else 3; 0x9a frame 1
  with TalkFlags[0x73] & 4, else 0; 0xc9 frame bit 2 likewise.*/
/*C_1B09_31F3*/static __ObjectsDeserialize(int16 numObj) {
	register int16 cur, incoming;
	int16 i, tmp, restart;
	int16 objTyp, lampFrame, onFrame;

	/*objects whose frame follows the power and the time of day*/
	lampFrame = (TalkFlags[0x73] & 0x10) && (Time_H < 6 || Time_H > 0x11) ? 7 : 3;
	onFrame = (TalkFlags[0x73] & 4) ? 1 : 0;
	for(i = 0; i < numObj; i ++) {
		if(FreeObjCount == 0) {
			BLK_OBJNUM(i) = -1;
			if(i)
				Link[BLK_OBJNUM(i - 1)] = -1;
			break;
		}
		cur = BLK_OBJNUM(i) = FreeObjHead;
		FreeObjHead = Link[FreeObjHead];
		FreeObjCount --;
		ObjStatus[cur] = BLK_STATUS(i);
		ObjPos[cur] =  BLK_POS(i);
		ObjShapeType[cur] = BLK_SHAPE(i);
		Amount[cur] = BLK_AMOUNT(i);
		if(GetCoordUse(cur) == CONTAINED)
			GetAssoc(cur) = BLK_OBJNUM((int16)GetAssoc(cur));
		objTyp = GetType(cur);
		if(objTyp == 0xe4)
			ObjShapeType[cur] = GetType(cur) + (lampFrame << 10);
		else if(objTyp == 0x9a)
			ObjShapeType[cur] = GetType(cur) + (onFrame << 10);
		else if(objTyp == 0xc9)
			ObjShapeType[cur] = GetType(cur) + (((TalkFlags[0x73] & 4) ? GetFrame(cur) | 4 : GetFrame(cur) & 0xfb) << 10);
	}
	if(i == numObj)
		Link[BLK_OBJNUM(i - 1)] = -1;
	/*merge: cur walks the main list, incoming the new chain*/
	cur = 0x100;
	incoming = BLK_OBJNUM(0);
	restart = 0;
	while(cur >= 0 && incoming >= 0) {
		if(OBJ_CompareOrder(Link[cur], incoming) < 0) {
			cur = Link[cur];
		} else {
			tmp = Link[cur];
			Link[cur] = incoming;
			cur = tmp;
			while(Link[incoming] >= 0 && OBJ_CompareOrder(cur, Link[incoming]) > 0) {
				if(OBJ_CompareOrder(incoming, Link[incoming]) <= 0) {
					incoming = Link[incoming];
				} else {
					restart = 1;
					break;
				}
			}
			tmp = Link[incoming];
			Link[incoming] = cur;
			incoming = tmp;
			if(restart) {
				cur = 0x100;
				restart = 0;
				continue;
			}
		}
		if(Link[cur] < 0) {
			Link[cur] = incoming;
			cur = -1;
		}
	}
}

/*load the outdoor area areaId from its objblk file (the .tmp one if any)*/
/*C_1B09_34D1*/static OBJ_LoadArea(int16 areaId) {
	register int16 fd;
	int16 numObj;

	if(areaId < 0 || DiskFullChoice == 'R')
		return;
	areaId &= 0x3f;
	if(!ObjBlkTmpPending[areaId]) {
		__Sav_file[15] = 'A' + (areaId & 7);
		__Sav_file[16] = 'A' + (areaId >> 3);
		fd = OSI_open(__Sav_file);
	} else {
		__Tmp_file[15] = 'A' + (areaId & 7);
		__Tmp_file[16] = 'A' + (areaId >> 3);
		fd = OSI_open(__Tmp_file);
	}
	OSI_read(fd, 0, 2, (void far *)&numObj);
	if(numObj > 0x800)
		numObj = 0x800;
	if(numObj)
		OSI_read(fd, -1, (int32)numObj << 3, ScratchBuf);
	OSI_close(fd);
	if(numObj)
		__ObjectsDeserialize(numObj);
}

/*rebuild the MapObjPtr[][] cache: first chained object of every tile of
  the 40x40 area*/
/*C_1B09_35A0*/OBJ_RebuildMapObjPtr() {
	register int16 obj, relY;
	int16 relX;
	int16 unusedMask;

	for(relY = 0; relY < AREA_H; relY ++) {
		for(relX = 0; relX < AREA_W; relX ++)
			MapObjPtr[relY][relX] = -1;
	}
	for(obj = Link[0x100]; obj >= 0; obj = Link[obj]) {
		if(GetCoordUse(obj) == LOCXYZ) {
			unusedMask = GetZ(obj)?0xff:0x3ff;/*never used*/
			relX = WrapDelta(GetX(obj) - AreaX) & 0x3ff;
			relY = WrapDelta(GetY(obj) - AreaY) & 0x3ff;
			if(relX < AREA_W && relY < AREA_H && MapObjPtr[relY][relX] == -1)
				MapObjPtr[relY][relX] = obj;
		}
	}
}

/*unload underground level z (1..5) like OBJ_UnloadArea does for an outdoor
  area; its file is "objblk<'A'+z-1>i", flag ObjBlkTmpPending[64 + z - 1]*/
/*C_1B09_3697*/static OBJ_UnloadDungeon(int16 z) {
	register int16 obj, numObj;
	int16 prev, last;

	if(z < 0 || z > 5 || DiskFullChoice == 'R')
		return;
	numObj = 0;
	prev = 0x100;
	obj = Link[0x100];
	while(numObj < 0x800 && obj >= 0) {
		if(GetCoordUse(obj) == LOCXYZ && GetZ(obj) == z) {
			if(IsNPC(obj)) {
				prev = obj;
			} else {
				BLK_OBJNUM(numObj) = obj;
				__ObjSerialize(obj, numObj++);
				FreeObjCount ++;
			}
			last = obj;
			obj = Link[obj];
			while(obj >= 0 && GetCoordUse(obj) != LOCXYZ) {
				if(numObj < 0x800) {
					BLK_OBJNUM(numObj) = obj;
					__ObjSerialize(obj, numObj++);
				}
				FreeObjCount ++;
				last = obj;
				obj = Link[obj];
			}
			Link[last] = FreeObjHead;
			FreeObjHead = Link[prev];
			Link[prev] = obj;
		} else {
			prev = obj;
			obj = Link[obj];
		}
	}
	__Tmp_file[15] = z - 1 + 'A';
	__Tmp_file[16] = 'I';
	OBJ_WriteBlockFile(numObj, z + 0x3f);
}

/*load underground level z (1..5) from its objblk file (the .tmp one if any)*/
/*C_1B09_3794*/static OBJ_LoadDungeon(int16 z) {
	register int16 fd;
	int16 numObj;

	if(z < 0 || z > 5 || DiskFullChoice == 'R')
		return;
	if(!ObjBlkTmpPending[8*8 + z - 1]) {
		__Sav_file[15] = 'A' + z - 1;
		__Sav_file[16] = 'I';
		fd = OSI_open(__Sav_file);
	} else {
		__Tmp_file[15] = 'A' + z - 1;
		__Tmp_file[16] = 'I';
		fd = OSI_open(__Tmp_file);
	}
	OSI_read(fd, 0, 2, (void far *)&numObj);
	if(numObj > 0x800)
		numObj = 0x800;
	if(numObj)
		OSI_read(fd, -1, (int32)numObj << 3, ScratchBuf);
	OSI_close(fd);
	if(numObj)
		__ObjectsDeserialize(numObj);
}

/*make sure the objects around (x, y, z) are loaded. On the surface a
  2x2 window of areas is kept; it is only shifted when (x, y) comes
  within 16 tiles of its border. Areas leaving the window (or the level
  left) are unloaded, new ones loaded, then NPC inventories relinked.*/
/*C_1B09_3851*/LoadNewRegions(int16 x, int16 y, int16 z) {
	register int16 i, j;
	int16 shiftX, shiftY, relX, relY, numUnloaded, numLoaded;
	int16 mapIds[4];

	numUnloaded =
	numLoaded = 0;
	if(LoadedZ > 0 && LoadedZ != z) {
		OBJ_PurgeFarLocals(x, y, z);
		OBJ_UnloadDungeon(LoadedZ);
		numUnloaded ++;
	}
	if(z == 0) {
		/*mapIds[0] = top-left area of the window*/
		if(LoadedWindowArea == -1) {
			mapIds[0] = (((x - 0x40) >> 7) & 7) + (((y - 0x40) >> 4) & 0x38);
		} else {
			shiftX = 0;
			relX = x - ((LoadedWindowArea & 7) << 7);
			for(; relX < 0x10; relX += 0x80, shiftX --);
			for(; relX >= 0xf0; relX -= 0x80, shiftX ++);
			shiftY = 0;
			relY = y - ((LoadedWindowArea & 0x38) << 4);
			for(; relY < 0x10; relY += 0x80, shiftY -= 8);
			for(; relY >= 0xf0; relY -= 0x80, shiftY += 8);
			mapIds[0] = (((LoadedWindowArea & 7) + shiftX) & 7) + (((LoadedWindowArea & 0x38) + shiftY) & 0x38);
		}
		mapIds[1] = ((mapIds[0] + 1) & 7) + (mapIds[0] & 0x38);
		mapIds[2] = (mapIds[0] + 8) & 0x3f;
		mapIds[3] = (mapIds[1] + 8) & 0x3f;
		LoadedWindowArea = mapIds[0];
	} else {
		for(i = 0; i < 4; i ++)
			mapIds[i] = -1;
		LoadedWindowArea = -1;
	}
	/*unload the areas no longer needed*/
	if(LoadedZ == 0) {
		for(i = 0; i < 4; i ++) {
			for(j = 0; j < 4; j ++) {
				if(mapIds[j] == LoadedAreas[i])
					break;
			}
			if(j != 4 || LoadedAreas[i] == -1)
				continue;
			if(numUnloaded == 0)
				OBJ_PurgeFarLocals(x, y, z);
			OBJ_UnloadArea(LoadedAreas[i]);
			numUnloaded ++;
			LoadedAreas[i] = -1;
		}
	}
	/*load the new ones*/
	if(z == 0) {
		for(j = 0; j < 4; j ++) {
			for(i = 0; i < 4; i++) {
				if(mapIds[j] == LoadedAreas[i])
					break;
			}
			if(i != 4)
				continue;
			for(i = 0; i < 4; i ++) {
				if(LoadedAreas[i] == -1)
					break;
			}
			OBJ_LoadArea(mapIds[j]);
			numLoaded ++;
			LoadedAreas[i] = mapIds[j];
		}
	} else if(LoadedZ != z) {
		OBJ_LoadDungeon(z);
		numLoaded ++;
	}
	LoadedZ = z;
	if(numLoaded)
		OBJ_RelinkHeldItems();
}

/*"commit" obj files (save game): write the loaded blocks, rename every
  .tmp file over its objblk file, then reload the blocks*/
/*C_1B09_3A9A*/OBJ_CommitBlocks() {
	int16 i;
	register int16 obj;

	if(MapZ) {
		OBJ_UnloadDungeon(MapZ);
	} else {
		for(i = 0; i < 4; i ++)
			OBJ_UnloadArea(LoadedAreas[i]);
	}
	for(i = 0; i < (8*8 + 5); i ++) {
		if(ObjBlkTmpPending[i]) {
			__Tmp_file[15] = __Sav_file[15] = __Bak_file[15] = (i & 7) + 'A';
			__Tmp_file[16] = __Sav_file[16] = __Bak_file[16] = (i >> 3) + 'A';/*.bak is never used*/
			OSI_delete(__Sav_file);
			OSI_rename(__Tmp_file, __Sav_file);
			ObjBlkTmpPending[i] = 0;
		}
	}
	for(i = 0; i < 4; i ++)
		LoadedAreas[i] = -1;
	LoadedWindowArea = LoadedZ = -1;
	LoadNewRegions(MapX, MapY, MapZ);
	OBJ_RebuildMapObjPtr();
	/*power: an object 0xd1 south of every 0xd0, else none*/
	for(i = 0x101; i < 0xc00; i ++) {
		if(TalkFlags[0x73] & 4) {
			if(GetType(i) == 0xd0 && OBJ_FindTypeAt(0xd1, GetX(i), (GetY(i) + 1) & 0x3ff) < 0) {
				if((obj = AddMapObj(0xd1, 0, 0, GetX(i), (GetY(i) + 1) & 0x3ff, MapZ)) >= 0)
					ClrLocal(obj);
			}
		} else if(GetType(i) == 0xd1)
			DeleteObj(i);
	}
}

/*add a tile to the display stack of the 11x11 view cell (x, y).
  Each cell's stack is a list of nodes: Obj_11x11[y][x] = first node,
  CellObjNext[node] = next node (CellObjNext[0] = free node list),
  CellObjTile[node] = tile | frame; it is drawn from the first node on.
  A "background" tile replaces the cell's map tile instead.
  Non-foreground tiles go first (below everything already there);
  foreground tiles (or mode 2) go, for an odd mode, last (on top), else
  just before the first foreground tile already there.
  Nothing is shown on hidden or dark cells; tiles from BaseTile[0x1cd] up are never shown.*/
/*C_1B09_3C3D*/ShowObject(int16 tileFrame, int16 x, int16 y, int16 mode) {
	register unsigned char *slot;
	register int16 tile;
	int16 mapTile, onTop/*set, never read*/, node;

	tile = tileFrame & 0x7ff;
	if(BaseTile[0x1cd] <= tile)
		return;
	mapTile = Tile_11x11[y][x];
	if(mapTile == 0xff/*hidden*/ || mapTile == 0x10c/*darkness*/)
		return;
	if(IsTileBa(tile)) {
		Tile_11x11[y][x] = tileFrame;
		return;
	}
	node = CellObjNext[0];
	if(node == 0)
		return;
	CellObjNext[0] = CellObjNext[node];
	CellObjTile[node] = tileFrame;
	slot = &(Obj_11x11[y][x]);
	if(IsTileFor(tile) || mode == 2) {
		onTop = 1;
		if(mode & 1) {
			while(*slot)
				slot = CellObjNext + *slot;
		} else {
			while(*slot && !IsTileFor(CellObjTile[*slot] & 0x7ff))
				slot = CellObjNext + *slot;
		}
	}
	CellObjNext[node] = *slot;
	*slot = node;
}

/*show tile + frame at view cell (x, y) together with the left/upper parts
  of double-width/double-height tiles (x, y may be 11 for those)*/
/*C_1B09_3D47*/OBJ_ShowMultiTile(int16 tile, int16 frame, register int16 x, register int16 y) {
	int16 flags;

	if(x < 11 && y < 11)
		ShowObject(tile + frame, x, y, 0);

	flags = TileFlag[tile];
	if(flags & 0x80) {
		if(x != 0 && y < 11)
			ShowObject(tile + frame - 1, x - 1, y, 1);
		if(flags & 0x40) {
			if(x < 11 && y != 0)
				ShowObject(tile + frame - 2, x, y - 1, 1);
			if(x != 0 && y != 0)
				ShowObject(tile + frame - 3, x - 1, y - 1, 1);
		}
	} else if(x < 11 && y != 0 && (flags & 0x40)) {
		ShowObject(tile + frame - 1, x, y - 1, 1);
	}
}

/*build the display stacks of the 11x11 view from the objects around
  (MapX, MapY), then hide what is on walls not seen from the party side and
  draw the darkness of the badly lit cells. On the way: the looping sound
  of the nearest machine (MachineTypes/MachineSfx; 0x25 in the dream's stage 0x40),
  plants 0xfb of quality 0xff shrink (never: GetQual is a byte), the
  doors 0xe3/0xb3 (with power) react to creatures standing on them, the
  mirror 0x12d shows the NPC south of it, 0xc9 lights up with power. Invisible objects show
  with an outline (bit 12; bits 11-12 for objects flagged 1 seen by a
  psychic, bit 11 for NPCs flagged 4) to party members with the third
  talent, else not at all.*/
/*C_1B09_3E18*/ShowObjects() {
	register int16 objNum, j;
	int16 i, tile, objX, objY, light, newFrame, objTyp, stack, node;
	int16 unused, frame, nearest, sfx, k, dir, npc, blocked;

	unused = -1;
	for(i = 0; i < 0x100; i ++)
		CellObjNext[i] = i + 1;

	for(i = 0; i < 11; i ++)
		for(j = 0; j < 11; j ++)
			Obj_11x11[i][j] = 0;

	if(IN_DREAM && DreamStage == 0x40) {
		sfx = 0x25;
	} else {
		nearest = 99;
		sfx = -1;
	}
	for(
		objNum = SearchArea((MapX - 5) & 0x3ff, (MapY - 5) & 0x3ff, (MapX + 6) & 0x3ff, (MapY + 6) & 0x3ff);
		objNum >= 0;
		objNum = NextArea()
	) {
		newFrame = 0;
		objTyp = GetType(objNum);
		if(!(IN_DREAM && DreamStage == 0x40)) {
			/*the nearest machine running gives the looping sound*/
			for(i = 0; ; i ++) {
				if(MachineSfx[i] == 0x38)
					break;
				if(!(TalkFlags[0x73] & 4) && MachineSfx[i] >= 0x40)
					break;
				if((TalkFlags[0x73] & 0x10) && MachineSfx[i] >= 0xc4)
					break;
				if(!(TalkFlags[0x73] & 0x10) && MachineSfx[i] >= 0x80 && MachineSfx[i] < 0xc4)
					break;
				if(MachineTypes[i] == objTyp) {
					objX = MapDistance(GetX(objNum), MapX);
					objY = MapDistance(GetY(objNum), MapY);
					if(objX < nearest || objY < nearest) {
						sfx = MachineSfx[i] & 0x3f;
						if(objX < objY)
							nearest = objX;
						else
							nearest = objY;
						break;
					}
				}
			}
		}
		if(objTyp == 0xfb && GetQual(objNum) == -1) {
			if((frame = GetFrame(objNum)) != 0)
				ObjShapeType[objNum] = GetType(objNum) + ((frame - 1) << 10);
		} else if((objTyp == 0xe3 && (TalkFlags[0x73] & 4)) || (objTyp == 0xb3 && (TalkFlags[0x73] & 0x10))) {
			if(!(GetQual(objNum) & 0x80)) {
				k = GetFrame(objNum);
				dir = k & 2;
				if((npc = OBJ_FindNPCAt(0, GetX(objNum), GetY(objNum))) >= 0) {
					if(dir + 9 != k && OBJ_IsOnScreen(objNum))
						MUS_MachinerySfx();
					ObjShapeType[objNum] = GetType(objNum) + ((dir + 9) << 10);
				} else {
					blocked = OBJ_FindNPCAt(0, (GetX(objNum) + DirIncrX[dir]) & 0x3ff, (GetY(objNum) + DirIncrY[dir]) & 0x3ff) >= 0;
					if(!blocked)
						blocked = OBJ_FindNPCAt(0, (GetX(objNum) + DirIncrX[dir + 4]) & 0x3ff, (GetY(objNum) + DirIncrY[dir + 4]) & 0x3ff) >= 0;
					if(k < 4) {
						if(blocked && OBJ_IsOnScreen(objNum)) {
							MUS_MachinerySfx();
							ObjShapeType[objNum] = GetType(objNum) + ((dir + 5) << 10);
						}
					} else if(k < 8) {
						if(!blocked && OBJ_IsOnScreen(objNum)) {
							MUS_MachinerySfx();
							ObjShapeType[objNum] = GetType(objNum) + ((dir + 1) << 10);
						}
					} else {
						if(!blocked && OBJ_IsOnScreen(objNum)) {
							MUS_MachinerySfx();
							ObjShapeType[objNum] = GetType(objNum) + ((dir + 5) << 10);
						}
					}
				}
			}
		} else if(objTyp == 0xc9 && (TalkFlags[0x73] & 0x10) && GetFrame(objNum) < 4) {
			ObjShapeType[objNum] = GetType(objNum) + ((GetFrame(objNum) | 4) << 10);
		} else if(objTyp == 0x12d && GetFrame(objNum) < 3) {
			k = OBJ_FindNPCAt(0, GetX(objNum), (GetY(objNum) + 1) & 0x3ff);
			if(k < 0 || k >= 0x70 || k == 0x5d || (k >= 0x6a && k <= 0x6c))
				ObjShapeType[objNum] = GetType(objNum);
			else if((k < 0x20 && k != 6) || (k >= 0x2a && k <= 0x5c) || k == 0x60 || k == 0x69 || k == 0x6d)
				ObjShapeType[objNum] = GetType(objNum) + 0x400;
			else
				ObjShapeType[objNum] = GetType(objNum) + 0x800;
		}
		if(!IsInvisible(objNum) || (IsNPC(objNum) && IsPlrControl(objNum)) || Talents[Party[Active]].c) {
			if(!IsNPC(objNum) || !(NPCFlag[objNum] & 0x10)) {
				tile = TILE_FRAME(objNum);
				/*outline drawn with the tile (bits 11-12 of the tile number)*/
				if(IsInvisible(objNum))
					newFrame = 0x1000;
				if((ObjStatus[objNum] & 1) && Talents[Party[Active]].psychic)
					newFrame = 0x1800;
				if(IsNPC(objNum) && (ObjStatus[objNum] & 4))
					newFrame = 0x800;
				objX = WrapDelta(GetX(objNum) - MapX) + 5;
				objY = WrapDelta(GetY(objNum) - MapY) + 5;
				if(newFrame)
					OBJ_ShowMultiTile(tile, newFrame, objX, objY);
				if(!IsInvisible(objNum))
					OBJ_ShowMultiTile(tile, 0, objX, objY);
			}
		}
	}
	for(i = 0; i < 11; i ++) {
		for(j = 0; j < 11; j ++) {
			if(IsTerrainWall(Tile_11x11[j][i])) {
				if(
					!(AreaFlags[j     + ViewY][i + 1 + ViewX] & 0x80) ||
					!(AreaFlags[j + 1 + ViewY][i     + ViewX] & 0x80) ||
					!(AreaFlags[j + 1 + ViewY][i + 1 + ViewX] & 0x80)
				) {
					stack = Obj_11x11[j][i];
					/*wall not seen from the party side: hide what is on it*/
					if(stack) {
						Obj_11x11[j][i] = 0;
						for(node = stack; CellObjNext[node]; node = CellObjNext[node]);
						CellObjNext[node] = CellObjNext[0];
						CellObjNext[0] = stack;
					}
				}
			}
			/*draw obscurity*/
			light = AreaLight[j + ViewY][i + ViewX];
			if(light < 4 && light > 0) {
				tile = 0x10c + light;
				ShowObject(tile, i, j, 3);
			}
		}
	}
	if(Erupting == 1)
		Erupting = 0;
	if(sfx != MachineLoopSfx) {
		MUS_PlayLoopSfx(sfx);
		MachineLoopSfx = sfx;
	}
}

/*Initialization: empty object list, all slots free (monsters 0xC0-0xFF,
  objects 0x101-0xBFF), no area loaded*/
/*C_1B09_45DA*/OBJ_InitLists() {
	register int16 i;

	for(i = 0; i < 0xbff; i ++) {
		Link[i] = i + 1;
		ObjShapeType[i] = 0;
	}
	Link[0xbff] = -1;
	Link[0xff] = -1;
	FreeMonsterHead = 0xc0;
	FreeMonsterCount = 0x3f;
	FreeObjHead = 0x101;
	FreeObjCount = 0xaff;
	Link[0x100] = -1;
	for(i = 0; i < 4; i ++)
		LoadedAreas[i] = -1;
	LoadedWindowArea = LoadedZ = -1;
}

/*called after a restore: link the existing NPCs (0x00-0xFF) into the
  sorted object list, remove the used temporary monster slots from their
  free list*/
/*C_1B09_4648*/OBJ_RelinkNPCs() {
	register int16 npc, prev;
	int16 lastFree;

	lastFree = 0x100;
	for(npc = 0; npc < 0x100; npc ++) {
		if(ObjShapeType[npc]) {
			if(npc >= 0xc0) {
				if(lastFree == 0x100)
					FreeMonsterHead = npc + 1;
				else
					Link[lastFree] = npc + 1;
				FreeMonsterCount --;
			}
			for(prev = 0x100; Link[prev] >= 0 && OBJ_CompareOrder(Link[prev], npc) < 0; prev = Link[prev]);
			Link[npc] = Link[prev];
			Link[prev] = npc;
		} else {
			if(npc >= 0xc0)
				lastFree = npc;
		}
	}
}

/*load tile description from "look.lzc": entries are (word first tile,
  zero-terminated string) sorted by tile; the entry for numTile is the
  first one whose first tile is >= numTile. Returns "nothing" if empty.*/
/*C_1B09_46E4*/char *GetTileString(int16 numTile) {
	register char *dst;
	register int16 tileNum;
	int16 far *src;

	src = LZC_loadType(/*1659*/"look.lzc", 0, ScratchBufHigh, 0, LzcWorkBuf);
	tileNum = *(src++);
	while(tileNum < numTile) {
		while(*CHARPTR_POSTINC(src));
		tileNum = *(src++);
	}
	dst = TileStringBuf;
	while(*dst++ = *CHARPTR_POSTINC(src));
	return TileStringBuf[0]?TileStringBuf:/*1662*/"nothing";
}

/*name of an object: the party member's name, the original shape's name for
  a dead or sleeping NPC, otherwise its tile description*/
/*C_1B09_4759*/char *GetObjectString(register int16 objNum) {
	char *str;
	register int16 i;

	if(objNum < 0) {
		str = /*166A*/"Nothing";
	} else if(objNum == 0) {
		strcpy(TileStringBuf, Names[0]);
		str = TileStringBuf;
	} else if(objNum < 0x10) {
		str = GetTileString(TILE_FRAME(objNum));
		for(i = 0; i < PartySize; i ++) {
			if(Party[i] == objNum) {
				strcpy(TileStringBuf, Names[i]);
				str = TileStringBuf;
				break;
			}
		}
	} else if(IsNPC(objNum) && (NPCStatus[objNum] & 0x14)) {
		str = GetTileString(BaseTile[OrigShapeType[objNum] & 0x3ff] + (OrigShapeType[objNum] >> 10));
	} else {
		str = GetTileString(TILE_FRAME(objNum));
	}
	return str;
}

/*the creeping cacti 0x17f (NPCs 0xc0-0xff, not dead) take the frame of their
  neighbours: bits 0-3 set when another live 0x17f stands north, east,
  south, west of them*/
/*C_1B09_4868*/OBJ_JoinCreepingCacti() {
	int16 x, y, nx, ny, frame, bit, other;
	register int16 dir, npc;

	for(npc = 0xc0; npc < 0x100; npc ++) {
		if(GetType(npc) == 0x17f && !(NPCStatus[npc] & 0x10)) {
			x = GetX(npc);
			y = GetY(npc);
			bit = 1;
			frame = 0;
			for(dir = 0; dir < 8; dir += 2) {
				nx = x + DirIncrX[dir];
				ny = y + DirIncrY[dir];
				for(other = OBJ_FindNPCAt(0xc0, nx, ny); other >= 0; other = OBJ_FindNPCAt(other + 1, nx, ny)) {
					if(GetType(other) == 0x17f && !(NPCStatus[other] & 0x10)) {
						frame |= bit;
						break;
					}
				}
				bit <<= 1;
			}
			ObjShapeType[npc] = GetType(npc) + (frame << 10);
		}
	}
}

/*delete everything inside objNum (all depths): the objects go back to the
  free list*/
/*C_1B09_4963*/OBJ_DeleteContents(register int16 objNum) {
	int16 last, next, count;
	register int16 obj;

	if(objNum < 0)
		return;
	if(StatusDisplay == 0x92 && OBJ_IsInside(objNum, Party[StatusMember]))
		StatusDirty = 1;
	if((last = OBJ_LastOfSubtree(objNum)) == objNum)
		return;
	next = Link[last];
	count = 0;
	for(obj = Link[objNum]; obj != next; obj = Link[obj]) {
		ObjShapeType[obj] = 0;
		count ++;
	}
	Link[last] = FreeObjHead;
	FreeObjHead = Link[objNum];
	FreeObjCount += count;
	Link[objNum] = next;
}

