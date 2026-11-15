/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE segment 1534: the map (Ultima VI's map.c, by way
 * of Savage Empire's: SE GAME.EXE 08CB).
 *
 * Ground tiles are stored in two files:
 * - "chunks": 8x8-tile blocks (64 bytes each), referenced by a 12-bit index;
 * - "map": the chunk indexes, packed 2 per 3 bytes (12 bits each).
 *   The surface (z=0, 1024x1024 tiles) is split in 8x8 "superchunks"
 *   of 16x16 chunks (0x180 bytes each); only a 2x2 window of superchunks
 *   is kept in the chunk index buffer (D_C0F6). The other levels are 32x32
 *   chunks (0x600 bytes) each and are loaded entirely.
 * The 40x40 tiles around the party (AreaTiles D_46D4, origin AreaX/AreaY
 * D_C0FA/D_C0FC) are rebuilt from a small LRU cache of 16 chunks (D_BCF2).
 * This module also moves the whole party between places: gathering
 * (PartyEnter), spreading out (PartyExit), teleport, going in and out
 * of caves by their entrances, and coming back to life after the party died.
 *
 * Differences from Savage Empire's: no DISK_confirm; MAP_BuildArea records
 * where the area crosses the map's edge (D_BCCE/D_C0F2, D_BCD0/D_C0F4) and
 * offsets are wrapped by WrapDelta (WrapDelta, new, like MapDistance
 * MapDistance); a party whose Active is PartySize (alone?) moves as one
 * object; PartyEnter's mode 2 shows an object 0x13c with a sound;
 * the entrances (C_1534_096C) are one table of (x, y, z) by quality, with
 * object 0xc5 (a talk flag needed) and the objects moved along by
 * 0xaf/0xd2/0xa3; SE's object 0x44 teleporters (C_08CB_0B8B) are gone;
 * after the party's death (C_1534_0C51) the party wakes up at 7 o'clock at
 * one of two places and the event conversation 0x12 runs.
 */
#include "se.h"

#define AREA_W 40
#define AREA_H 40

/*OSI file routines (asm)*/
extern int16 OSI_open(char *);
extern OSI_close(int16);
extern int32 OSI_read(int16, int32, int32, void far *);
extern int16 OSI_rand(int16, int16);/*OSI_rand(lo, hi)*/
extern void cdecl TimerDelay(int32);/*wait (2383 per millisecond)*/

/*other game modules*/
extern int16 CombatOutcome;
extern int16 ScreenFade;
extern int16 ViewX, ViewY;
extern char D_0F12;/*StatusDirty*/
extern int16 ForceHatching;
extern int16 VisibilityDirty;
extern int16 LocPartOffset;
extern int16 MoveAppliesTerrain;
extern unsigned char Time_M;
extern unsigned char Time_H;
extern unsigned char Date_D;
extern unsigned char Date_M;
extern int16 Date_Y;
extern unsigned char Active;
extern int16 MapX, MapY, MapZ;
extern unsigned char D_16A3;
extern char InCombat;
extern char SoloMember;
extern char SavedNPCMode;
extern int16 StatusDisplay;
extern unsigned char D_3266[];/*ObjStatus*/
extern char D_44C4[];/*MovePts*/
extern unsigned char far *D_45CC;/*NPCFlag*/
extern unsigned char far *D_46D0;/*NPCMode*/
extern unsigned char D_46D4[AREA_H][AREA_W];/*AreaTiles*/
extern unsigned char D_4EA4[];/*Party*/
extern unsigned char far *D_50CA;
extern uint16 D_51AE[];/*ExpPoints*/
extern int16 D_54BA[];/*Amount: quantity (low byte), quality (high byte)*/
extern unsigned char D_6CBA[];/*NPCStatus*/
extern unsigned char D_6DBE[];/*HitPoints*/
extern unsigned char D_6EC6[];/*TalkFlags (per NPC)*/
extern struct coord D_78AE[];/*ObjPos*/
extern unsigned char D_9CC0;/*PartySize*/
extern uint16 D_9CC4[];/*ObjShapeType*/
extern int16 D_B9D6[0x40];/*MissileHitList*/
extern int16 far *D_B508;/*OrigShapeType*/
extern int16 D_BCAE[16];/*ChunkCacheAge*/
extern int16 D_BCCE, D_BCD0;
extern int16 D_BCD2[16];/*ChunkCacheIdx*/
extern unsigned char D_BCF2[16][8][8];/*ChunkCache*/
extern int16 D_C0F2, D_C0F4;
extern unsigned char far *D_C0F6;/*ChunkIndexBuf*/
extern int16 D_C0FA, D_C0FC;/*AreaX, AreaY*/
extern int16 D_C59C[];/*Link*/
extern int16 D_DDB2[AREA_H][AREA_W];/*MapObjPtr*/
extern int16 D_EBCE;/*Selection.obj*/

extern DrawSkyBar();
extern int16 COMBAT_TileDistance(int16, int16, int16);/*COMBAT_TileDistance(objNum, x, y)*/
extern COMBAT_breakOff();
extern int16 MaxHP(int16);
extern int16 GetDex(int16);/*an NPC's dexterity*/
extern VIS_UpdateView();
extern DisplayTick();
extern PassTime(int16);
extern SetPartyMode();
extern EGG_hatchArea();
extern MoveFollowers(int16, int16);
extern TryStraightMove(int16, int16, int16);
extern int16 TryMoveTo(int16, int16, int16);
extern int16 NextLoc();/*next object of FindLoc's search*/
extern int16 FindLoc(int16, int16, int16);/*first object at (x, y, z)?*/
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);
extern DeleteObj(int16);
extern OBJ_PurgeFarLocals(int16, int16, int16);
extern OBJ_ReviveFromBody(int16);
extern MoveObj(int16, int16, int16, int16);
extern OBJ_RebuildMapObjPtr();
extern LoadNewRegions(int16, int16, int16);
extern ShowObjects();
extern MUS_PlaySong(int16);
extern MUS_PlayAreaSong();
extern MUS_PlaySfx(int16);/*play a sound effect?*/
extern C_1FA9_0CD2();
extern STAT_ShowParty(int16);
extern int16 CON_mouseOffAt(int16, int16, int16, int16);
extern CON_mouseOn(int16);
extern cdecl CON_printf(char far *, ...);
extern SPELL_ReviveNPC(int16);
extern TALK_talkTo(int16, int16, int16);
extern char far C_2BB4_219B[];/*far messages*/
extern char far C_2BB4_21CD[];
extern char far C_2BB4_22F1[];

#define StatusDirty D_0F12
#define ObjStatus D_3266
#define MovePts D_44C4
#define NPCFlag D_45CC
#define NPCMode D_46D0
#define AreaTiles D_46D4
#define Party D_4EA4
#define ExpPoints D_51AE
#define Amount D_54BA
#define NPCStatus D_6CBA
#define HitPoints D_6DBE
#define TalkFlags D_6EC6
#define ObjPos D_78AE
#define PartySize D_9CC0
#define ObjShapeType D_9CC4
#define MissileHitList D_B9D6
#define OrigShapeType D_B508
#define ChunkCacheAge D_BCAE
#define ChunkCacheIdx D_BCD2
#define ChunkCache D_BCF2
#define ChunkIndexBuf D_C0F6
#define AreaX D_C0FA
#define AreaY D_C0FC
#define Link D_C59C
#define MapObjPtr D_DDB2
#define Selection D_EBCE


#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)


/*132E*/int16 MapCoordMask = 0x3FF;/*coordinate wrap mask: 0x3ff on surface, 0xff underground*/
/*1330*/int16 ChunksFile = -1;/*file "chunks" handler*/
/*1332*/int16 MapFile = -1;/*file "map" handler*/
/*1334*/int16 LoadedMapZ = -1;/*level (z) currently loaded in the chunk index buffer*/
/*1336*/int16 LoadedSuperChunks[4] = {-1, -1, -1, -1};/*surface: ids of the 2x2 superchunks loaded (top-left, top-right, bottom-left, bottom-right)*/
/*133E*/int16 BuiltAreaX = -1;/*AreaX of the area tiles currently built*/
/*1340*/int16 BuiltAreaY = -1;/*AreaY of the area tiles currently built*/
/*1342*/int16 BuiltAreaZ = -1;/*MapZ of the area tiles currently built*/

/*where the entrances lead (x, y, z), by the entrance object's quality 1-24
  (see C_1534_096C)*/
/*1344*/int16 Entrances[24][3] = {
	{0x043,0x051,1},{0x080,0x08D,0},{0x076,0x0F2,3},{0x07A,0x00C,3},
	{0x09A,0x040,4},{0x2B2,0x1CC,0},{0x073,0x040,4},{0x29D,0x1CE,0},
	{0x0B3,0x0E1,4},{0x27B,0x1F1,0},{0x0C3,0x070,1},{0x0CB,0x1D1,0},
	{0x024,0x0F1,1},{0x31A,0x232,0},{0x05C,0x0F1,1},{0x34C,0x25A,0},
	{0x07C,0x028,4},{0x13E,0x118,0},{0x00A,0x060,5},{0x3B7,0x1C4,0},
	{0x05C,0x0B0,5},{0x3DC,0x1DD,0},{0x05E,0x029,4},{0x227,0x1AB,0}
};
/*where object 0xc5 leads (x, y on the surface), by its quality 1-4*/
/*13D4*/int16 D_13D4[4][2] = {
	{0x0CA,0x295},{0x34D,0x295},{0x13A,0x17A},{0x2CD,0x1FA}
};

/*superchunk id = row * 8 + column (each superchunk is 128x128 tiles)*/
#define MK_MAP_ID(x,y) (((x) >> 7) + (((y) >> 4) & 0x38))

/*make sure chunk #chunkIdx is in the chunk cache (load it over the
  least recently used entry if needed); returns its cache slot*/
static /*C_1534_0000*/cacheChunk(int16 chunkIdx) {
	int16 i, cacheIdx;
	int16 oldestIdx;

	cacheIdx = -1;
	oldestIdx = 0;
	/*lookup chunk in cache*/
	for(i = 0; i < 16; i ++) {
		if(ChunkCacheIdx[i] == chunkIdx) {
			cacheIdx = i;
			break;
		}
		if(ChunkCacheAge[i] > ChunkCacheAge[oldestIdx])
			oldestIdx = i;
	}
	/*load if not cached*/
	if(cacheIdx < 0) {
		cacheIdx = oldestIdx;
		ChunkCacheIdx[cacheIdx] = chunkIdx;
		ChunksFile = OSI_open("chunks");
		OSI_read(ChunksFile, (int32)chunkIdx << 6, 0x40, ChunkCache[cacheIdx]);
		OSI_close(ChunksFile);
	}
	/*update chunks "age"*/
	for(i = 0; i < 16; i ++) {
		if(ChunkCacheAge[i] < ChunkCacheAge[cacheIdx])
			ChunkCacheAge[i] ++;
	}
	ChunkCacheAge[cacheIdx] = 0;

	return cacheIdx;
}

/*load into the chunk index buffer the chunk indexes needed for (x, y, z):
  the whole level if z != 0, else a 2x2 window of superchunks
  containing (x, y); the window is only moved when (x, y) leaves it*/
/*C_1534_00C4*/MAP_LoadChunkIndexes(int16 x, int16 y, int16 z) {
	int16 mapId, i;
	int16 mustLoad, colDiff, rowDiff, bufOfs;
	int32 fileOfs;
	unsigned char far *pSlot;

	mustLoad = (LoadedMapZ != z);
	if(!mustLoad && z == 0) {
		mapId = MK_MAP_ID(x, y);
		mustLoad = 1;
		for(i = 0; i < 4; i ++) {
			if(LoadedSuperChunks[i] == mapId) {
				mustLoad = 0;
				break;
			}
		}
	}
	if(!mustLoad)
		return;
	if(z != 0) {
		MapFile = OSI_open("map");
		/*z * 0x600 + 0x5a00 = 0x6000 + (z - 1) * 0x600*/
		OSI_read(MapFile, (int32)((z + z + z) << 9) + 0x5a00, 4 * 0x180, ChunkIndexBuf);
		OSI_close(MapFile);
		LoadedMapZ = z;
		for(i = 0; i < 4; i ++)
			LoadedSuperChunks[i] = -1;
	} else {
		mapId = MK_MAP_ID(x, y);
		if(LoadedSuperChunks[0] >= 0) {
			/*move the current window just enough to contain mapId
			  (column/row differences are modulo 8, the world wraps)*/
			colDiff = (mapId - LoadedSuperChunks[0]) & 7;
			rowDiff = (mapId - LoadedSuperChunks[0]) & 0x38;
			mapId = LoadedSuperChunks[0];
			if(colDiff > 1 && colDiff < 5)
				mapId = (mapId & 0x38) + ((mapId + colDiff - 1) & 7);
			else if(colDiff > 4)
				mapId = (mapId & 0x38) + ((mapId + colDiff) & 7);
			/*note: "- 1" here only lowers the row when the column is 0
			  ("- 8" was probably meant); the window still contains the target*/
			if(rowDiff > 0x08 && rowDiff < 0x28)
				mapId = (mapId & 7) + ((mapId + rowDiff - 1) & 0x38);
			else if(rowDiff > 0x20)
				mapId = (mapId & 7) + ((mapId + rowDiff) & 0x38);
		}
		if(mapId != LoadedSuperChunks[0]) {
			LoadedSuperChunks[0] = mapId;
			LoadedSuperChunks[1] = (mapId + 1) & 0x3f;
			LoadedSuperChunks[2] = (mapId + 8) & 0x3f;
			LoadedSuperChunks[3] = (mapId + 9) & 0x3f;
			MapFile = OSI_open("map");
			for(i = 0; i < 4; i ++) {
				/*superchunk id * 0x180*/
				fileOfs = (int32)LoadedSuperChunks[i] << 7; fileOfs += fileOfs << 1;
				/*window slot * 0x180*/
				bufOfs = i << 7; bufOfs += bufOfs << 1;
				pSlot = ChunkIndexBuf + bufOfs;
				OSI_read(MapFile, fileOfs, 0x180, pSlot);
			}
			OSI_close(MapFile);
		}
		LoadedMapZ = 0;
	}
}

/*recompute AreaX/AreaY (40x40 area aligned on chunks, party around the
  middle) and ViewX/ViewY (top-left of the 11x11 view inside the area);
  if the area moved, re-place the NPCs, rebuild the area tiles from the
  chunks and hatch the eggs; then ask for a view redraw*/
/*C_1534_02DC*/MAP_BuildArea() {
	unsigned char *pMap, *pChunk;
	int16 i, j;
	int16 anX, anY, cacheIdx;

	AreaX = anX = (MapX - 0x10) & 0x3f8;
	ViewX = (MapX - anX - 5) & 0x1f;
	AreaY = anY = (MapY - 0x10) & 0x3f8;
	ViewY = (MapY - anY - 5) & 0x1f;
	if(MapZ)
		MapCoordMask = 0xff;
	else
		MapCoordMask = 0x3ff;
	/*where the area crosses the map's edge (0 and 39 if it does not)*/
	if(AreaX <= 0x3d8) {
		D_BCCE = 0;
		D_C0F2 = 39;
	} else {
		D_BCCE = 0x400 - AreaX;
		D_C0F2 = D_BCCE - 1;
	}
	if(AreaY <= 0x3d8) {
		D_BCD0 = 0;
		D_C0F4 = 39;
	} else {
		D_BCD0 = 0x400 - AreaY;
		D_C0F4 = D_BCD0 - 1;
	}
	if(anX != BuiltAreaX || anY != BuiltAreaY || MapZ != BuiltAreaZ) {
		OBJ_RebuildMapObjPtr();
		/*NPCs inside the new area but missing from MapObjPtr: put them back*/
		MoveAppliesTerrain = 0;
		for(i = 0; i < 0x100; i ++) {
			anX = WrapDelta(GetX(i) - AreaX) & MapCoordMask;
			anY = WrapDelta(GetY(i) - AreaY) & MapCoordMask;
			if(ObjShapeType[i])
			if((ObjStatus[i] & 0x18) == 0)
			if(GetZ(i) == MapZ)
			if(anX < 40 && anY < 40)
			if(MapObjPtr[anY][anX] != i)
				MoveObj(i, GetX(i), GetY(i), MapZ);
		}
		MoveAppliesTerrain = 1;

		anX = AreaX;
		anY = AreaY;
		OBJ_PurgeFarLocals((AreaX + 20) & 0x3ff, (AreaY + 20) & 0x3ff, MapZ);

		BuiltAreaX = anX;
		BuiltAreaY = anY;
		BuiltAreaZ = MapZ;
		/*copy the 5x5 chunks of the area (i = x, j = y)*/
		for(i = 0; i < 40; i += 8) {
			for(j = 0; j < 40; j += 8) {
				cacheIdx = cacheChunk(getChunkIdxAt((BuiltAreaX + i) & 0x3ff, (BuiltAreaY + j) & 0x3ff, MapZ));
				pChunk = ChunkCache[cacheIdx][0];
				pMap = &(AreaTiles[j][i]);
				for(anY = 0; anY < 8; anY ++) {
					for(anX = 0; anX < 8; anX ++)
						*pMap++ = *pChunk++;
					pMap += 32;/*next line: AREA_W - 8*/
				}
			}
		}
		EGG_hatchArea();
	}
	VisibilityDirty = 1;
}

/*empty the chunk cache*/
/*C_1534_054C*/MAP_InitChunkCache() {
	int16 i;

	for(i = 0; i < 0x10; i++) {
		ChunkCacheIdx[i] = -1;
		ChunkCacheAge[i] = i;
	}
}

/*load everything (objects regions, chunk indexes, area tiles)
  around MapX, MapY, MapZ*/
/*C_1534_056F*/MAP_LoadAround() {
	LoadNewRegions(MapX, MapY, MapZ);
	MAP_LoadChunkIndexes(MapX, MapY, MapZ);
	MAP_BuildArea();
}

/*gather the party on the leader's square (MapX, MapY) and take
  everybody off the map (z = 6), e.g. before a teleport or a level change.
  withAnimation: 0 = instant; else members walk to the leader for up to 30
  steps; when a member disappears, 2 shows object 0x13c there with a sound,
  else the screen fades. When Active is PartySize, only that object moves*/
/*C_1534_0599*/PartyEnter(int16 withAnimation) {
	int16 i;

	/*(U6 dismounted the horses here)*/
	for(i = 0; i < PartySize; i ++);

	MAP_LoadAround();

	if(withAnimation && Active != PartySize) {
		int16 j, allArrived;
		for(j = 0; j < 30; j ++) {
			allArrived = 1;
			for(i = 0; i < PartySize; i ++) {
				int16 objNum = Party[i];
				/*wake up*/
				if(NPCStatus[objNum] & 0x04)
					ObjShapeType[objNum] = OrigShapeType[objNum];
				NPCStatus[objNum] &= ~(0x04|0x02);
				if(GetZ(objNum) == MapZ && !((NPCStatus[objNum] & 0x16) || (NPCFlag[objNum] & 0x10))) {
					if(GetX(objNum) == MapX && GetY(objNum) == MapY) {
						/*arrived: remove from map*/
						ShowObjects();
						DisplayTick();
						MoveAppliesTerrain = 0;
						MoveObj(objNum, MapX, MapY, 6);
						MoveAppliesTerrain = 1;
						if(withAnimation == 2) {
							int16 fx;
							fx = AddMapObj(0x13c, 0, 0, MapX, MapY, MapZ);
							ShowObjects();
							DisplayTick();
							MUS_PlaySfx(0x22);
							TimerDelay(0x91726L);
							DeleteObj(fx);
						} else {
							ScreenFade = 1;
						}
						ShowObjects();
						DisplayTick();
					} else if(!TryMoveTo(objNum, MapX, MapY)) {
						/*blocked: jump if adjacent, else random step*/
						if(COMBAT_TileDistance(objNum, MapX, MapY) < 2)
							MoveObj(objNum, MapX, MapY, MapZ);
						else
							TryStraightMove(objNum, OSI_rand(0, 3) << 1, 1);
					} else {
						allArrived = 0;
					}
				}
				MovePts[objNum] = GetDex(objNum);
			}
			ShowObjects();
			DisplayTick();
			if(allArrived)
				break;
		}
	}

	/*whoever is left (asleep, paralyzed, other level...) is taken anyway*/
	MoveAppliesTerrain = 0;
	if(Active == PartySize) {
		MoveObj(Party[Active], MapX, MapY, 6);
		if(withAnimation) {
			ScreenFade = 1;
			ShowObjects();
			DisplayTick();
		}
	} else {
		for(i = 0; i < PartySize; i ++) {
			if(GetZ(Party[i]) != 6)
				MoveObj(Party[i], MapX, MapY, 6);
		}
	}
	MoveAppliesTerrain = 1;
}

/*put the whole party back on the map at (x, y, z) and let the
  followers step out around the leader (5 steps)*/
/*C_1534_0816*/PartyExit(int16 x, int16 y, int16 z, register int16 withAnimation) {
	register int16 i;

	MoveAppliesTerrain = 0;
	if(Active == PartySize) {
		MoveObj(Party[Active], x, y, z);
	} else {
		for(i = 0; i < PartySize; i ++)
			MoveObj(Party[i], x, y, z);
	}
	MoveAppliesTerrain = 1;

	if(Active != PartySize)
		SetPartyMode();

	if(withAnimation == 1) {
		ScreenFade = 1;
		ShowObjects();
		DisplayTick();
	}

	if(Active != PartySize) {
		for(i = 0; i < 5; i ++) {
			MoveFollowers(Party[Active], (withAnimation == 2 && i == 0)?2:1);
			if(withAnimation) {
				ShowObjects();
				DisplayTick();
			}
		}
		for(i = 0; i < PartySize; i ++)
			MovePts[Party[i]] = GetDex(Party[i]);
	}
}

/*move the whole party to (x, y, z)*/
/*C_1534_0906*/PartyTeleport(int16 x, int16 y, int16 z, register int16 withAnimation) {
	PartyEnter(withAnimation);
	if(withAnimation == 1)
		MUS_PlaySfx(0x33);

	MapX = x;
	MapY = y;
	MapZ = z;
	MAP_LoadAround();

	ForceHatching = 1;
	EGG_hatchArea();
	ForceHatching = 0;

	VIS_UpdateView();
	DrawSkyBar();
	PassTime(0);
	PartyExit(x, y, z, withAnimation);
}

/*go through the entrance objNum (cave, stairs...): the party moves to the
  place its quality gives (table Entrances, D_13D4 for object 0xc5)*/
/*C_1534_096C*/C_1534_096C(int16 objNum) {
	int16 obj;
	int16 x, y, z;
	register int16 qual, type;

	type = GetType(objNum);
	qual = GetQual(objNum);
	if(type == 0xc5) {
		if(!(TalkFlags[0x73] & 0x10)) {
			CON_printf(C_2BB4_219B);
			return;
		}
		if(qual == 0) {
			CON_printf(C_2BB4_21CD);
			return;
		}
		if(qual > 4) {
			CON_printf(C_2BB4_22F1);
			return;
		}
		x = D_13D4[qual - 1][0];
		y = D_13D4[qual - 1][1];
		z = 0;
	} else {
		if(qual < 1 || qual > 24) {
			CON_printf(C_2BB4_22F1);
			return;
		}
		x = Entrances[qual - 1][0];
		y = Entrances[qual - 1][1];
		z = Entrances[qual - 1][2];
	}
	if(InCombat)
		COMBAT_breakOff();
	MapX = GetX(objNum);
	MapY = GetY(objNum);
	MAP_LoadAround();
	MUS_PlaySong(-1);
	PartyEnter((type == 0xa3)?0:((type == 0xc5)?2:1));
	C_1FA9_0CD2();
	if(type == 0xaf || type == 0xd2) {
		for(obj = FindLoc(MapX, (MapY - 1) & 0x3ff, MapZ); obj >= 0; obj = NextLoc()) {
			if(LocPartOffset)
				break;
			MoveObj(obj, x, (y - 1) & 0x3ff, z);
			if(GetType(obj) == 0x1b9)
				D_9CC4[obj] = GetType(obj) + 0xc00;
		}
	} else if(type == 0xa3) {
		for(obj = FindLoc(MapX, (MapY + 1) & 0x3ff, MapZ); obj >= 0; obj = NextLoc()) {
			if(LocPartOffset)
				break;
			MoveObj(obj, x, (y + 2) & 0x3ff, z);
		}
	} else if(type != 0xb2 && type != 0xb4 && type != 0xc5) {
		CON_printf(C_2BB4_22F1);
		x = GetX(objNum);
		y = GetY(objNum);
		z = MapZ;
	}
	if(MapZ == 3 && qual == 3 && !(TalkFlags[0x66] & 0x10)) {
		OBJ_PurgeFarLocals(-1, -1, -1);
		D_16A3 = 6;
		ForceHatching = 1;
	}
	MapX = x;
	MapY = y;
	MapZ = z;
	MAP_LoadAround();
	if(MapZ == 3 && qual == 3 && !(TalkFlags[0x66] & 0x10))
		ForceHatching = 0;
	VIS_UpdateView();
	DrawSkyBar();
	PassTime(0);
	PartyExit(MapX, MapY, MapZ, (type == 0xa3)?0:((type == 0xc5)?2:1));
	MUS_PlayAreaSong();
}

/*after the party's death: bring the dead party members back to life (their
  bodies are objects 0x155), heal everybody (they lose a quarter of their
  experience) and bring the party to (0xa4, 0xc3, 4) when talk flag
  TalkFlags[0x46] & 8 is set, else to (0x19d, 0x278, 0); a week passes, the
  party wakes up at 7 o'clock and conversation 0x12 runs*/
/*C_1534_0C51*/C_1534_0C51() {
	int16 i, flag, clip;
	int16 next;
	int16 memberObj, bodyObj;

	Active = -1;
	STAT_ShowParty(0);
	StatusDisplay = 0x91;
	for(i = 0; i < 0xc0; i ++) {
		if((NPCStatus[i] & 0x80) && (NPCStatus[i] & 0x10)) {
			/*look for the NPC's dead body*/
			for(bodyObj = Link[0x100]; bodyObj >= 0; bodyObj = next) {
				next = Link[bodyObj];
				if(GetType(bodyObj) == 0x155 && GetQual(bodyObj) == i) {
					Selection = bodyObj;
					if(i == 1) {
						OBJ_ReviveFromBody(bodyObj);
						NPCStatus[1] &= ~0x10;
						NPCStatus[1] &= ~0x08;
						HitPoints[1] = MaxHP(1);
						ObjShapeType[1] = OrigShapeType[1];
						NPCStatus[1] &= ~0x1f;
					} else {
						MoveAppliesTerrain = 0;
						SPELL_ReviveNPC(bodyObj);
					}
					break;
				}
			}
		}
		if(NPCMode[i] == 0xa0)
			NPCMode[i] = 0x81;
	}
	for(i = 0; i < PartySize; i ++) {
		memberObj = Party[i];
		if(NPCStatus[memberObj] & 0x10)
			continue;
		if(NPCStatus[memberObj] & 0x04) {
			NPCStatus[memberObj] &= ~0x04;
			ObjShapeType[memberObj] = OrigShapeType[memberObj];
		}
		NPCStatus[memberObj] = (NPCStatus[memberObj] & ~0x60) | 0x40;
		ObjStatus[memberObj] &= ~0x46;
		NPCStatus[memberObj] &= ~0x1e;
		HitPoints[memberObj] = MaxHP(memberObj);
		ExpPoints[memberObj] = ExpPoints[memberObj] * 3 / 4;
		MovePts[memberObj] = GetDex(memberObj);
	}
	for(i = 0; i < 0x40; i ++)
		MissileHitList[i] = -1;
	SetPartyMode();
	if(InCombat)
		COMBAT_breakOff();
	if(SoloMember < 0)
		flag = 1;
	else
		flag = 0;
	for(i = 0; i < PartySize; i ++) {
		if(NPCStatus[Party[i]] & 0x10) {
			NPCMode[Party[i]] = 0;
		} else if(!(ObjStatus[Party[i]] & 0x04)) {
			NPCMode[Party[i]] = flag;
			if(flag == 1)
				D_50CA[Party[i]] = i;
		}
	}
	if(SoloMember < 0)
		i = 0;
	else
		i = SoloMember;
	SavedNPCMode = NPCMode[Party[i]];
	NPCMode[Party[i]] = 2;
	clip = CON_mouseOffAt(0x40, 0, 0x5f, 0xf);
	CON_mouseOn(clip);
	CombatOutcome = 0;
	if(TalkFlags[0x46] & 8)
		PartyTeleport(0xa4, 0xc3, 4, 0);
	else
		PartyTeleport(0x19d, 0x278, 0, 0);
	/*a week passes*/
	if((Date_D += 7) > 28) {
		Date_D -= 28;
		if(++Date_M > 12) {
			Date_M = 1;
			Date_Y ++;
		}
	}
	Time_H = 7;
	Time_M = OSI_rand(0, 59);
	PassTime(0);
	TalkFlags[0x12] |= 0x20;
	TALK_talkTo(1, 0x12, 0);
	ScreenFade = 1;
	ShowObjects();
	DisplayTick();
	StatusDirty = 1;
	MoveAppliesTerrain = 1;
}

/*12-bit index of the chunk containing tile (x, y, z)
  (loads the needed part of the map first)*/
/*C_1534_0F91*/getChunkIdxAt(register int16 x, int16 y, int16 z) {
	int16 chunkIdx, mapId, quadOfs;
	register int16 bufPos;

	x &= 0x3ff;
	y &= 0x3ff;
	MAP_LoadChunkIndexes(x, y, z);
	if(z != 0) {
		/*32x32 chunks: (chunkY * 32 + chunkX) * 1.5*/
		bufPos = ((x >> 3) & 0x1f) + ((y << 2) & 0x3e0);
		bufPos += bufPos >> 1;
		/*odd chunk: upper 12 bits, even chunk: lower 12 bits*/
		if(x & 8)
			chunkIdx = *((int16 far *)(ChunkIndexBuf + bufPos)) >> 4;
		else
			chunkIdx = *((int16 far *)(ChunkIndexBuf + bufPos)) & 0xfff;
	} else {
		/*which superchunk of the 2x2 window (x 0x100 chunk entries)*/
		mapId = MK_MAP_ID(x, y);
		quadOfs = 0;
		if((mapId - LoadedSuperChunks[0]) & 1)
			quadOfs = 0x100;
		if((mapId - LoadedSuperChunks[0]) & 8)
			quadOfs += 0x200;
		/*16x16 chunks: (quadOfs + chunkY * 16 + chunkX) * 1.5*/
		bufPos = quadOfs + ((x >> 3) & 0xf) + ((y << 1) & 0xf0);
		bufPos += bufPos >> 1;
		if(x & 8)
			chunkIdx = *((int16 far *)(ChunkIndexBuf + bufPos)) >> 4;
		else
			chunkIdx = *((int16 far *)(ChunkIndexBuf + bufPos)) & 0xfff;
	}

	return chunkIdx;
}

/*ground tile at (x, y, z): from the area tiles if inside the current area,
  else from the chunks*/
unsigned char /*C_1534_1072*/GetTileAtXYZ(register int16 x, register int16 y, int16 z) {
	int16 cacheIdx;
	int16 relX, relY;

	x &= MapCoordMask;
	y &= MapCoordMask;
	relX = WrapDelta(x - AreaX) & MapCoordMask;
	relY = WrapDelta(y - AreaY) & MapCoordMask;
	if(relX < 40 && relY < 40)
		return AreaTiles[relY][relX];

	cacheIdx = cacheChunk(getChunkIdxAt(x, y, z));
	return ChunkCache[cacheIdx][y & 7][x & 7];
}

/*a difference of two map coordinates brought into -0x200..0x200
  (the surface wraps at 0x400)*/
/*C_1534_1101*/WrapDelta(register int16 d) {
	if(d > 0x200)
		return d - 0x400;
	else if(d < -0x200)
		return d + 0x400;
	else
		return d;
}

/*distance between two map coordinates (the surface wraps at 0x400)*/
/*C_1534_1129*/MapDistance(register int16 a, register int16 b) {
	register int16 d;

	if(a < b)
		d = b - a;
	else
		d = a - b;
	if(d > 0x200)
		d = 0x400 - d;
	return d;
}
