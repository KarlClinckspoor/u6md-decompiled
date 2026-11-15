/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE segment 170D: NPC tracker, movement and A.I. of
 * every actor (objects 0..0xff). Savage Empire's npctrack.c (segment 130F,
 * from Ultima VI's) reworked:
 *
 * - the passability test went to canmove.c (SE's CanMoveTo is MD's
 *   NPC_TryStep), and with it MoveFromX/Y/Set; SE's type tests
 *   (C_130F_0002-01C6), Board/Unboard, FindAdjacentPlate and the big
 *   creatures made of several objects are gone;
 * - new: IsSquareOutOfArea (IsOutOfArea for a square; both use WrapDelta),
 *   the turn/step wrappers UpdateTurnFrame/UpdateStepFrame of
 *   UpdateMoveFrame (which gained a "step" argument: the humanoids sit on
 *   the chairs of the square they step to), IsMale (a humanoid type's sex,
 *   for rest.c), NPC_BecomeObject (mode 0x1a: an NPC turns into an object)
 *   and TalkWhenNear (mode 0x95: talk to NPC 1 when near);
 * - coordinates wrap: differences go through WrapDelta and MapDistance,
 *   sums are masked with 0x3ff;
 * - MoveFollowers walks the party to its places on a barge (CurrentBarge)
 *   and knows the conveyor belts; PartyAdvance: "you are dizzy!" (drunk),
 *   ApplyTerrainEffects on a blocked step, BeltUpdate and BargeFindLeader;
 *   CheckGatesAndEntrances: the dream machine (0x1cd, 0x1ce), the 0x1d1
 *   squares that turn the view; NPC_TakeTurn: the coal workers (modes
 *   0x99-0x9e) and the new combat modes of MD;
 * - TeleportToSchedule no longer spares the NPCs near the player.
 *
 * NPCMode values: 0 motionless, 1 follow, 2 command, 0x80 schedule, 0x81
 * find path, 0x82 seek object, 0x83 on path, 0x84/0x85 path blocked, 0x86
 * path failed, 0x91 sleep.
 */
#include "se.h"

#define AREA_W 40
#define AREA_H 40

/*a schedule entry (Ultima VI's)*/
struct tSchedule/*size:5*/ {
	unsigned char time;/*bits 0-4 hour, bits 5-7 day of the week*/
	unsigned char action;
	struct coord xyz;
};

/*path slots (Ultima VI's)*/
struct tPathData {
	unsigned char content[8][0x20];
};

/*the path search work area in ScratchBuf (Ultima VI's)*/
typedef struct {
	unsigned char /*_0000*/PTH_map[AREA_H][AREA_W];
	int16 /*_0640*/PTH_rank[0x100];
	int16 /*_0840*/PTH_link[0x100];
	unsigned char /*_0a40*/PTH_x[0x100],/*_0b40*/PTH_y[0x100];
	unsigned char /*_0c40*/PTH_resist[AREA_H][AREA_W];
} t40x40;

/*console, map*/
extern cdecl CON_printf(char far *, ...);
extern MAP_LoadAround();
extern unsigned char GetTileAtXYZ(int16, int16, int16);
extern int16 WrapDelta(int16);
extern int16 MapDistance(int16, int16);
extern GoThroughEntrance(int16);/*goes through an entrance*/
extern PartyTeleport(int16, int16, int16, int16);
/*objects*/
extern int16 NextLoc();
extern int16 FindLoc(int16, int16, int16);/*FindLoc(x, y, z)*/
extern int16 OBJ_FindTypeAt(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 OBJ_NextOfTypeAt(int16);/*the next object of the same type there*/
extern int16 OBJ_IsOnScreen(int16);
extern int16 NextArea();
extern int16 SearchArea(int16, int16, int16, int16);/*SearchArea(x1, y1, x2, y2)*/
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);/*AddMapObj(shape, status, amount, x, y, z)*/
extern DeleteObj(int16);
extern OBJ_PurgeFarLocals(int16, int16, int16);
extern int16 MoveObj(int16, int16, int16, int16);/*MoveObj(objNum, x, y, z)*/
extern OBJ_ShowMultiTile(int16, int16, int16, int16);
extern ShowObjects();
extern int16 IsCrushableType(int16);
/*combat*/
extern COMBAT_stopCharm(int16);
extern int16 COMBAT_TileDistance(int16, int16, int16);/*COMBAT_TileDistance(objNum, x, y)*/
extern int16 GetDex(int16);/*an NPC's dexterity*/
extern SetTypeUnconscious(int16);/*an NPC falls asleep (shape)*/
extern int16 COMBAT_canSee(int16, int16);/*COMBAT_canSee(objNum, target)*/
extern COMBAT_showHit(int16);
extern int16 COMBAT_range(int16, int16, int16, int16);/*distance of (x1, y1) and (x2, y2)*/
extern int16 COMBAT_pickOpponent(int16);
extern COMBAT_AI_Assault(int16);
extern COMBAT_AI_Range(int16);
extern COMBAT_AI_Berserk(int16);
extern COMBAT_AI_Retreat(int16);
extern COMBAT_AI_Like(int16);
extern COMBAT_AI_Unfriendly(int16);
extern COMBAT_AI_Immobile(int16);
extern COMBAT_AI_Rampage(int16);/*A.I. of mode 0x13*/
extern COMBAT_AI_ChaseCape(int16);/*A.I. of mode 0x16*/
extern COMBAT_AI_HiddenInSand(int16);/*A.I. of mode 0x12*/
extern COMBAT_AI_Tentacle(int16);/*A.I. of mode 0x19*/
extern COMBAT_AI_Thief(int16);/*A.I. of mode 0x14*/
extern COMBAT_AI_Worm(int16);/*A.I. of mode 0x0d*/
extern COMBAT_AI_WormFlee(int16);/*A.I. of mode 0x11*/
extern COMBAT_AI_MoveSouth(int16);/*A.I. of mode 0x15*/
extern COMBAT_AI_Together(int16);/*A.I. of mode 0x17*/
extern AI_SeekSeed(int16);/*A.I. of mode 0x18*/
/*other modules*/
extern int16 OSI_rand(int16, int16);
extern void cdecl TimerDelay(int32);/*wait (2383 per millisecond)*/
extern int16 NPC_TryStep(int16, int16, int16);/*SE: CanMoveTo*/
extern DisplayTick();
extern FlushViewIfDirty();
extern SetActive(int16);
extern SetPartyMode();
extern PassTime(int16);
extern ApplyTerrainEffects(int16, int16, int16);
extern TALK_talkTo(int16, int16, int16);
extern DrawSkyBar();
extern MUS_PlaySfx(int16);
extern PartyUnconscious();/*the party is knocked out*/
extern BeltUpdate(int16);
extern BeltStart();
extern BargeFindLeader(int16);
extern MachineryOn();
extern MachineryOff();
extern Cutscene2();/*the underground power*/
extern DreamAnimFaster();
extern DreamTrigger(int16);
extern char far MustBeInPartyMsg[];/*you must be in party mode to %s*/
extern char far StokerBlockedMsg[];
extern char far AarghMsg[];/*failure message*/
/*functions of this file called before their definition*/
extern int16 TryMoveTo(int16, int16, int16);
extern NPC_BecomeObject(int16);
extern TalkWhenNear(int16);

extern char DirIncrX[], DirIncrY[];
extern int16 MousePress;
extern int16 MouseMapX, MouseMapY;
extern int16 MoveFromX, MoveFromY, MoveFromSet;/*canmove.c*/
extern int16 LeaderOnBelt, MemberOnBelt;/*canmove.c*/
extern char LeaderOnBarge;/*barge.c*/
extern char PromptFlag;
extern int16 LocPartOffset;
extern int16 MoveAppliesTerrain;
extern unsigned char Time_H;
extern unsigned char Date_D;
extern unsigned char Active;
extern int16 MapX, MapY, MapZ;
extern unsigned char DrunkCounter;
extern unsigned char PathObject[8];/*NPC following each path slot (0 = free)*/
extern unsigned char PathCounter[8];/*current entry of each path*/
extern unsigned char PathTries[8];/*steps done in the current entry*/
extern char InCombat;
extern char SoloMember;
extern unsigned char PlayerIsFemale;
extern int32 GameFlags;/*0x10: in a dream*/
extern int16 ViewRotation;
extern unsigned char far *TalkFlagsHi;/*0x100 bytes (0x80: NPC's "WAIT" flag)*/
extern unsigned char ObjStatus[];
/*44C4*/extern char MovePts[];/*movement points left this turn*/
/*45C4*/extern unsigned char far *SchedIndex;/*current schedule entry of each NPC, relative to SchedPointer*/
/*45C8*/extern struct tPathData far *PathData;
/*45CC*/extern unsigned char far *NPCFlag;/*0x80 walking (step frame), 0x10 dragged under, bits 0-2 direction*/
extern unsigned char DEXTE[];/*the base value; GetDex adds the modifiers*/
/*46D0*/extern unsigned char far *NPCMode;/*the A.I. mode of each NPC (see the header)*/
extern unsigned char AreaTiles[AREA_H][AREA_W];/*the ground tiles of the area*/
/*4EA4*/extern unsigned char Party[];/*the party members' object numbers*/
/*4EBA*/extern struct tSchedule far *Schedule;
/*4EBE*/extern int16 SchedPointer[0x101];/*first schedule entry of each NPC*/
/*50C0*/extern t40x40 far *ScratchBuf;/*the path search work area (main.c: 0x7900 bytes)*/
/*50CA*/extern unsigned char far *Leader;/*path slot of an NPC following a path (else its leader/target)*/
/*53AE*/extern unsigned char far *NPCComMode;/*the combat mode of each NPC*/
extern int16 Amount[];
/*6CBA*/extern unsigned char NPCStatus[];/*0x80 in the party, 0x60 alignment, 0x10 dead, 0x08 poisoned, 0x04 asleep, 0x02 paralyzed, 0x01 protected*/
extern unsigned char far *TileFlag;
extern unsigned char HitPoints[];
extern int16 far *BaseTile;
extern unsigned char TalkFlags[];
extern struct coord ObjPos[];
/*9CC0*/extern unsigned char PartySize;
extern uint16 ObjShapeType[];
extern unsigned char far *TerrainType;
/*B508*/extern int16 far *OrigShapeType;/*the shape an NPC gets back when it wakes up*/
extern int16 CurrentBarge[9];
extern int16 AreaX, AreaY;
/*C102*/extern unsigned char FormationObjs[];/*the leader then the followers placed so far*/
/*C112*/extern unsigned char PTH_Cur;/*cost bucket being expanded*/
/*C114*/extern int16 PTH_AnX;/*edge square reached (goal outside the area)*/
/*C116*/extern int16 PTH_AnY;
/*C118*/extern unsigned char PTH_MaxCost;/*highest cost queued so far*/
/*C119*/extern unsigned char PTH_Index;/*path slot being searched*/
/*C11C*/extern int16 PTH_Top;/*head of the free node list*/
/*C11E*/extern int16 PTH_object;/*object type seeked, -1 if none*/
/*C120*/extern int16 PTH_direct;/*area edge to reach (0 N, 1 E, 2 S, 3 W), -1 if none*/
/*C122*/extern int16 PTH_Found;



#define TypeFrame(t, s) ((t) + ((s) << 10))
#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define SetFrame(i, v) (ObjShapeType[i] = TypeFrame(GetType(i),(v)))
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])
#define GetCoordX(o) ((*(int16 *)&((o).raw[0]))&0x3ff)
#define GetCoordY(o) (((*(int16 *)&((o).raw[1]))>>2)&0x3ff)
#define GetCoordZ(o) ((o).raw[2] >> 4)
#define GetX(i) GetCoordX(ObjPos[i])
#define GetY(i) GetCoordY(ObjPos[i])
#define GetZ(i) GetCoordZ(ObjPos[i])
#define GetCoordUse(i) (ObjStatus[i] & 0x18)
#define IsNPC(i) ((i) < 0x100 && (i) >= 0)
#define IsPlrControl(i) (NPCStatus[i] & 0x80)
#define SetAlignment(i, v) (NPCStatus[i] = (NPCStatus[i] & ~0x60) | (v))
#define ClrLocal(i) (ObjStatus[i] &= ~0x20)
#define IN_VEHICLE (Active == PartySize)
#define IsParalyzed(i) (NPCStatus[i] & 0x02)
#define IsAsleep(i) (NPCStatus[i] & 0x04)
#define ClrAilments(i) (NPCStatus[i] &= ~(0x10|0x08|0x04|0x02))
#define ClrCursedCharmed(i) (ObjStatus[i] &= ~(0x40|0x04))
#define ClrDraggedUnder(i) (NPCFlag[i] &= ~0x10)
#define IsCharmed(i) (ObjStatus[i] & 0x04)
#define IsDead(i) (NPCStatus[i] & 0x10)
#define SetAsleep(i) (NPCStatus[i] |= 0x04)
#define ClrAsleep(i) (NPCStatus[i] &= ~0x04)
#define IsIncapacitated(i) (NPCStatus[i] & 0x16)
#define IsDraggedUnder(i) (NPCFlag[i] & 0x10)
#define IsTerrainWet(tile) (TerrainType[tile] & 1)
#define IsTerrainWall(tile) (TerrainType[tile] & 4)
#define IsTerrainImpass(tile) (TerrainType[tile] & 2)
#define IsWalking(i) (NPCFlag[i] & 0x80)
#define SetWalking(i) (NPCFlag[i] |= 0x80)
#define ClrWalking(i) (NPCFlag[i] &= ~0x80)
#define GetDirection(i) (NPCFlag[i] & 7)
#define SetDirection(i, v) (NPCFlag[i] = (NPCFlag[i] & ~7) | (v))

/*furniture an NPC looks for (see AI_SEEKOBJ): only one kind left*/
/*1420*/int16 FurnitureTypes[] = {0x184};
/*1422*/int16 AllowNPCTeleport = 0;
/*1424*/int16 StepMovesParts = 0;
/*1426*/int16 TeleportCount = 0;
/*1428*/int16 PathSearchCount = 0;
/*142A*/int16 HazardFound = 0;
/*142C*/int16 IgnoreHazards = 0;
/*142E*/unsigned char ShowQuestBarrierMsg = 0;
/*142F*/int16 AdvanceBlocked = 0;
/*1431*/int16 VisibilityDirty = 0;
/*1433*/int16 PassThroughParty = 1;
/*1435*/int16 AvoidHazards = 0;
/*1437*/int16 EnemyWarningGiven = 0;
/*1439*/char FormationSide[] = { 0,-1, 1, 0,-2, 2,-1, 1,-3, 3, 0};
/*1444*/char FormationBack[] = { 0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 1};
/*144F*/unsigned char DirEdgeFlag[] = {0x80,0,0x40,0,0x20,0,0x10};
/*1456*/unsigned char ShipWindCost[] = {10,5,3,4, 5,4,3,5};
/*145E*/unsigned char SkiffCurrentCost[] = { 3,4,5,7,10,7,5,4};
/*1466*/unsigned char RaftDriftDir[][8] = {
	{0,0,1,1,0,7,7,0},
	{1,1,1,2,2,1,0,0},
	{1,2,2,2,3,3,2,1},
	{2,2,3,3,3,4,4,3},
	{4,3,3,4,4,4,5,5},
	{6,5,4,4,5,5,5,6},
	{7,7,6,5,5,6,6,6},
	{7,0,0,7,6,6,7,7}
};
/*14A6*/char PTH_DirX[] = { 0,1,0,-1};
/*14AA*/char PTH_DirY[] = {-1,0,1, 0};

/*is objNum outside the 40x40 area around the player (AreaX, AreaY) or on
 another level?*/
/*C_170D_0002*/IsOutOfArea(int16 objNum) {
	int16 dx, dy;

	dx = WrapDelta(GetX(objNum) - AreaX);
	dy = WrapDelta(GetY(objNum) - AreaY);
	return !(GetZ(objNum) == MapZ && dx <= (AREA_W-1) && dx >= 0 && dy <= (AREA_H-1) && dy >= 0);
}

/*the same for the square (x, y, z)*/
/*C_170D_007E*/IsSquareOutOfArea(int16 x, int16 y, int16 z) {
	int16 dx, dy;

	dx = WrapDelta(x - AreaX);
	dy = WrapDelta(y - AreaY);
	return !(z == MapZ && dx <= (AREA_W-1) && dx >= 0 && dy <= (AREA_H-1) && dy >= 0);
}

/*is (x, y, MapZ) blocked for creature objNum (a big creature turning)?
 1 if the base tile or an object there blocks it, or another actor is there*/
/*C_170D_00C9*/IsBlockedAt(int16 x, int16 y, int16 objNum) {
	int16 tile, obj;

	tile = GetTileAtXYZ(x, y, MapZ);
	if((TerrainType[tile] & 2) || (TerrainType[tile] & 4))
		return 1;
	for(obj = FindLoc(x, y, MapZ); obj >= 0; obj = NextLoc()) {
		tile = TILE_FRAME(obj) - LocPartOffset;
		if(IsNPC(obj) && obj != objNum)
			return 1;
		if((TerrainType[tile] & 2) && GetQual(obj) != objNum)
			return 1;
	}
	return 0;
}

/*turn the 4-way facing "frame" (0 N,1 E,2 S,3 W) toward the 8-way "dir":
 straight dirs set it directly; for a diagonal the facing is kept if it is
 one of the two components, otherwise it is reversed*/
#define FACE_TOWARD(dir,frame) {                        \
	if(!((dir) & 1))                                \
		(frame) = (dir) >> 1;                       \
	else if(((((dir) >> 1) - (frame) + 1) & 3) > 1) \
		(frame) = ((frame) + 2) & 3;                \
}

/*make objNum face the 8-way direction dir and update its shape frame
 (step animation, alternating feet with the NPCFlag "walking" bit); when
 step is set the square looked at for a chair is the one it steps to.
 Returns 0 when the creature refuses to turn, else 1 (and clears
 StepMovesParts)*/
/*C_170D_0170*/UpdateMoveFrame(int16 objNum, int16 dir, int16 step) {
	int16 objTyp, objFrm, dirFram;
	int16 tmpObj, isOnChair, chairType;
	int16 cur_x, cur_y, terrain;

	isOnChair = 0;
	cur_x = GetX(objNum);
	cur_y = GetY(objNum);
	if(step) {
		cur_x = (cur_x + DirIncrX[dir]) & 0x3ff;
		cur_y = (cur_y + DirIncrY[dir]) & 0x3ff;
	}
	if(!IsOutOfArea(objNum)) {
		for(tmpObj = FindLoc(cur_x, cur_y, GetZ(objNum)); tmpObj >= 0; tmpObj = NextLoc()) {
			chairType = GetType(tmpObj);
			if(
				chairType == 0x196 ||
				(chairType == 0x197 && GetFrame(tmpObj) - LocPartOffset) ||
				chairType == 0xd8 || chairType == 0x121
			) {
				isOnChair = 1;
				break;
			}
		}
	}
	objTyp = GetType(objNum);
	terrain = TerrainType[TILE_FRAME(objNum)];
	objFrm = GetFrame(objNum);
	if(objTyp >= 0x156 && objTyp <= 0x166) {
		/*humanoids: 4 frames per facing (1 standing, 0/2 steps, 3 sitting)*/
		if(isOnChair) {
			objFrm = 3;
			if(chairType == 0x196)
				dirFram = 0;
			else if(chairType == 0x121)
				dirFram = 2;
			else if(chairType == 0x197)
				dirFram = 1;
			else
				dirFram = dir >> 1;
		} else {
			dirFram = objFrm >> 2;
			FACE_TOWARD(dir, dirFram);
			switch(objFrm & 3) {
				case 0: ClrWalking(objNum); objFrm = 1; break;
				case 1: objFrm = IsWalking(objNum)?0:2; break;
				case 2: SetWalking(objNum); objFrm = 1; break;
				default: objFrm = 1;
			}
		}
		objFrm = objFrm + (dirFram << 2);
		SetFrame(objNum, objFrm);
	} else if(
		objTyp == 0x16b || objTyp == 0x16c || objTyp == 0x180 || objTyp == 0x181 ||
		objTyp == 0x16d || objTyp == 0x174 || objTyp == 0x177 || objTyp == 0x16f ||
		objTyp == 0x183 || objTyp == 0x171 || objTyp == 0x176 || objTyp == 0x17c ||
		objTyp == 0x18e
	) {
		/*2 frames per facing*/
		dirFram = objFrm >> 1;
		if(dir & 1) {
			cur_x = GetX(objNum);
			cur_y = GetY(objNum);
			if(IsBlockedAt(cur_x, (cur_y - DirIncrY[dir]) & 0x3ff, objNum))
				dir = (dir & 4) + 2;
			else if(IsBlockedAt((cur_x - DirIncrX[dir]) & 0x3ff, cur_y, objNum))
				dir = (dir + 1) & 4;
		}
		FACE_TOWARD(dir, dirFram);
		if(!(objFrm & 1))
			objFrm = 1;
		else
			objFrm = 0;
		objFrm = (objFrm & 1) + (dirFram << 1);
		SetFrame(objNum, objFrm);
	} else if(objTyp == 0x91) {
		if(step && (dir & 1))
			return 0;
		SetFrame(objNum, (dir >> 1) & 1);
	} else if(
		objTyp == 0x167 || objTyp == 0x168 || objTyp == 0x169 || objTyp == 0x16e ||
		objTyp == 0x170
	) {
		/*2x2 creatures, 4 frames per facing*/
		dirFram = objFrm >> 2;
		cur_x = GetX(objNum);
		cur_y = GetY(objNum);
		switch(dirFram << 1) {
			case 0: case 4:
				if((dir == 2 || dir == 6) && IsBlockedAt((cur_x - 1) & 0x3ff, cur_y, objNum))
					return 0;
			break;
			case 2: case 6:
				if((dir == 0 || dir == 4) && IsBlockedAt(cur_x, (cur_y - 1) & 0x3ff, objNum))
					return 0;
			break;
		}
		if(dir & 1) {
			if(IsBlockedAt(cur_x, (cur_y - DirIncrY[dir]) & 0x3ff, objNum))
				dir = (dir & 4) + 2;
			else if(IsBlockedAt((cur_x - DirIncrX[dir]) & 0x3ff, cur_y, objNum))
				dir = (dir + 1) & 4;
		}
		FACE_TOWARD(dir, dirFram);
		if(!(objFrm & 2))
			objFrm = 1;
		else
			objFrm = 0;
		objFrm = ((objFrm + (dirFram << 1) + 1) << 1) - 1;
		SetFrame(objNum, objFrm);
	} else if(objTyp == 0x17b || objTyp == 0x178) {
		/*8 frames per facing*/
		dirFram = objFrm >> 3;
		if(dir & 1) {
			cur_x = GetX(objNum);
			cur_y = GetY(objNum);
			if(IsBlockedAt(cur_x, (cur_y - DirIncrY[dir]) & 0x3ff, objNum))
				dir = (dir & 4) + 2;
			else if(IsBlockedAt((cur_x - DirIncrX[dir]) & 0x3ff, cur_y, objNum))
				dir = (dir + 1) & 4;
		}
		FACE_TOWARD(dir, dirFram);
		if(!(objFrm & 4))
			objFrm = 4;
		else
			objFrm = 0;
		objFrm = (objFrm & 4) + (dirFram << 3) + 3;
		SetFrame(objNum, objFrm);
	} else if(objTyp == 0x179) {
		if(!OSI_rand(0, 1)) {
			if(objFrm > 2)
				SetFrame(objNum, 0);
			else
				SetFrame(objNum, objFrm + 1);
		} else if(!OSI_rand(0, 1)) {
			if(objFrm == 0)
				SetFrame(objNum, 3);
			else
				SetFrame(objNum, objFrm - 1);
		}
	} else if(objTyp == 0x17a) {
		if(objFrm != 0)
			SetFrame(objNum, 0);
		else
			SetFrame(objNum, 1);
	} else if(objTyp == 0x17e) {
		if(dir == 0)
			SetFrame(objNum, 1);
		else if(dir == 4)
			SetFrame(objNum, 3);
		else
			return 0;
	} else if(objTyp == 0x187) {
		if(dir == 4)
			SetFrame(objNum, 1 - objFrm);
		else
			return 0;
	}
	StepMovesParts = 0;
	return 1;
}
#undef FACE_TOWARD

/*turn objNum toward dir*/
/*C_170D_083F*/UpdateTurnFrame(int16 objNum, int16 dir) {
	return UpdateMoveFrame(objNum, dir, 0);
}

/*step animation of objNum moving in direction dir*/
/*C_170D_0853*/UpdateStepFrame(int16 objNum, int16 dir) {
	return UpdateMoveFrame(objNum, dir, 1);
}

/*spend pts movement points (MovePts) for objNum: 2 less for a party
 member out of combat; at least 1*/
/*C_170D_0868*/SubMov(int16 objNum, int16 pts) {
	if(objNum < 0x10 && !InCombat)
		pts -= 2;
	if(pts < 1)
		pts = 1;
	MovePts[objNum] -= pts;
}

/*formation helper: is (x, y) next to one of the first "count" members
 already placed in FormationObjs[] (leader first), i.e. could that member
 step onto it? 0 if (x, y) is occupied by one of them*/
/*C_170D_0898*/IsNextToFormation(int16 count, int16 x, int16 y) {
	int16 placedObj, placedType, delta_x, delta_y;
	int16 k, isAdjacent;

	isAdjacent = 0;
	MoveFromSet = 1;
	for(k = 0; k < count; k ++) {
		placedObj = FormationObjs[k];
		placedType = GetType(placedObj);
		delta_x = WrapDelta(GetX(placedObj) - x);
		delta_y = WrapDelta(GetY(placedObj) - y);
		if(delta_x == 0 && delta_y == 0) {
			isAdjacent = 0;
			break;
		}
		if(delta_x > -2 && delta_x < 2 && delta_y > -2 && delta_y < 2) {
			MoveFromX = x;
			MoveFromY = y;
			if(NPC_TryStep(placedObj, (x + delta_x) & 0x3ff, (y + delta_y) & 0x3ff))
				isAdjacent = 1;
		}
	}
	MoveFromSet = 0;
	return isAdjacent;
}

/*move the AI_FOLLOW party members one step after the leader moved.
 Member k aims for formation slot k (FormationSide/FormationBack, relative to the leader's
 facing). Pass 0 only moves members that lost contact with the chain;
 pass 1 also moves those behind their slot (or, with regroup set, every
 member not exactly on its slot). Each move picks among the 8 neighbours
 the one that keeps contact and is closest to the slot, avoiding damaging
 tiles; conveyor belts weigh in (LeaderOnBelt, MemberOnBelt). On a barge
 (LeaderOnBarge) everybody walks to his place on it (CurrentBarge) instead.
 regroup 2 also plays a sound first. Followers pay 5 points.*/
/*C_170D_0976*/MoveFollowers(int16 leader, int16 regroup) {
	int16 follower;
	int16 partyIdx, try_dir, pass;
	int16 follower_x, follower_y;
	int16 try_x, try_y, eager, follow_pos, new_dir;
	int16 max_eager, savedMovePts, facing_dir, leaderDir, x, y, target_x, target_y;
	int16 leaderType, wasNextTo, onBelt;

	if(IN_VEHICLE)
		return;
	FormationObjs[0] = leader;
	x = GetX(leader);
	y = GetY(leader);
	follow_pos = 1;
	leaderDir = GetDirection(leader);
	leaderType = GetType(leader);
	facing_dir = leaderDir & 6;
	for(pass = 0; pass < 2; pass ++) {
		follow_pos = 1;
		for(partyIdx = 0; partyIdx < PartySize; partyIdx ++) {
			FormationObjs[follow_pos] = follower = Party[partyIdx];
			if(regroup == 2 && pass == 0) {
				MUS_PlaySfx(0x22);
				TimerDelay(238300L);
			}
			if(LeaderOnBarge >= 0) {
				/*on a barge: everybody walks to his place on it*/
				target_x = GetX(CurrentBarge[partyIdx]);
				target_y = GetY(CurrentBarge[partyIdx]);
				if(partyIdx != 0 && x == target_x && y == target_y) {
					target_x = GetX(CurrentBarge[PartySize]);
					target_y = GetY(CurrentBarge[PartySize]);
				}
				if(follower == leader)
					continue;
				if(GetZ(follower) != MapZ)
					continue;
				if(NPCMode[follower] != 1/*AI_FOLLOW*/)
					continue;
				TryMoveTo(follower, target_x, target_y);
				continue;
			}
			/*formation slot = leader - facing * back + perpendicular * side*/
			target_x = (x
				- DirIncrX[facing_dir] * FormationBack[follow_pos]
				- DirIncrY[facing_dir] * FormationSide[follow_pos]
			) & 0x3ff;
			target_y = (y
				+ DirIncrX[facing_dir] * FormationSide[follow_pos]
				- DirIncrY[facing_dir] * FormationBack[follow_pos]
			) & 0x3ff;
			if(follower == leader)
				continue;
			if(GetZ(follower) != MapZ)
				continue;
			if(NPCMode[follower] != 1/*AI_FOLLOW*/)
				continue;
			if(IsIncapacitated(follower))
				continue;
			if(IsDraggedUnder(follower))
				continue;
			savedMovePts = MovePts[follower];
			follower_x = GetX(follower);
			follower_y = GetY(follower);
			if(
				!IsNextToFormation(follow_pos, follower_x, follower_y)
				/*on second pass*/
				|| (pass == 1 && (
					(regroup && !(target_x == follower_x && target_y == follower_y)) ||
					(!regroup && WrapDelta(target_x - follower_x) * DirIncrX[facing_dir] + WrapDelta(target_y - follower_y) * DirIncrY[facing_dir] > 0)
				))
			) {
				wasNextTo = IsNextToFormation(follow_pos, follower_x, follower_y);
				new_dir = -1;
				max_eager = 0;
				for(try_dir = 0; try_dir < 8; try_dir ++) {
					try_x = (follower_x + DirIncrX[try_dir]) & 0x3ff;
					try_y = (follower_y + DirIncrY[try_dir]) & 0x3ff;
					/*score: 256 (128 on a damaging tile, refused if already in
					contact), +256 if the square keeps contact with the chain, 0 if
					it would break an existing contact, minus the distance to the
					slot; conveyor belts move the leader or the follower*/
					if(NPC_TryStep(follower, try_x, try_y)) {
						eager = 0;
						if(HazardFound) {
							if(wasNextTo)
								continue;
							eager = 0x80;
						} else {
							eager = 0x100;
						}
						onBelt = 0;
						if((!LeaderOnBelt && MemberOnBelt) || (LeaderOnBelt && !MemberOnBelt))
							onBelt = 1;
						if(IsNextToFormation(follow_pos, try_x, try_y))
							eager += 0x100;
						else if(wasNextTo && (pass < 2 || !onBelt))
							eager = 0;
						if(!LeaderOnBelt && MemberOnBelt)
							eager -= 0x70;
						if(LeaderOnBelt) {
							if(MemberOnBelt)
								eager += 0x70;
							else
								eager -= 0x70;
						}
						eager -= MapDistance(try_x, target_x);
						eager -= MapDistance(try_y, target_y);
						if(eager > max_eager) {
							max_eager = eager;
							new_dir = try_dir;
						}
					}
				}
				if(new_dir >= 0) {
					StepMovesParts = 1;
					if(UpdateStepFrame(follower, new_dir) == 1)
						MoveObj(follower, (follower_x + DirIncrX[new_dir]) & 0x3ff, (follower_y + DirIncrY[new_dir]) & 0x3ff, MapZ);
				}
			}
			/*following always costs 5 points whatever the terrain*/
			MovePts[follower] = savedMovePts;
			SubMov(Party[partyIdx], 5);
			follow_pos ++;
		}
	}
}

/*the party leader stepped on (MapX, MapY): entrances, the dream machine's
 tiles and objects, and the dizziness of the 0x1d1 squares; then time
 passes*/
/*C_170D_0E16*/CheckGatesAndEntrances() {
	int16 objNum, objType;
	int16 needPartyMode, obj;

	needPartyMode = 0;
	if(GetTileAtXYZ(MapX, MapY, MapZ) == 0x6f)
		DreamAnimFaster();
	for(objNum = FindLoc(MapX, MapY, MapZ); objNum >= 0; objNum = NextLoc()) {
		if(Party[Active] == objNum)
			continue;
		objType = GetType(objNum);
		if(
			objType == 0xaf || objType == 0xa3 || objType == 0xd2 ||
			(objType == 0xb2 && GetFrame(objNum) - LocPartOffset < 2) ||
			(objType == 0xb4 && GetFrame(objNum) - LocPartOffset > 1) ||
			(objType == 0xc5 && GetFrame(objNum) - LocPartOffset == 3)
		) {
			if(SoloMember != -1)
				needPartyMode = 1;
			else
				GoThroughEntrance(objNum);
			break;
		}
		if(objType == 0x1cd) {
			obj = OBJ_FindTypeAt(0x124, MapX, (MapY - 1) & 0x3ff);
			if(obj >= 0) {
				DisplayTick();
				ShowObjects();
				OBJ_ShowMultiTile(TILE_FRAME(obj), 0x1000, 5, 4);
				DisplayTick();
				TimerDelay(238300L);
			}
			if((ObjStatus[objNum] & 0xe5) == 0xa5 && HitPoints[0] <= 4)
				break;
			if(ObjStatus[objNum] & 0xe5) {
				DreamTrigger(objNum);
				break;
			}
			OBJ_PurgeFarLocals(-1, -1, -1);
			PartyTeleport(GetQual(objNum), GetQuan(objNum), MapZ, GetFrame(objNum));
			break;
		}
		if(objType == 0x1ce) {
			if(GetQual(objNum))
				ObjShapeType[0] = PlayerIsFemale ? 0x13f : 0x13e;
			else
				ObjShapeType[0] = OrigShapeType[1];
			continue;
		}
		if(objType == 0x1d1) {
			if(GetQual(objNum) != ViewRotation & 7) {
				ViewRotation = GetQual(objNum) & 7;
				MUS_PlaySfx(0x32);
				CON_printf("You feel dizzy...\n");
				PromptFlag = 1;
			}
		}
	}
	DrawSkyBar();
	PassTime(0);
	if(needPartyMode) {
		switch(objType) {
			case 0xa3: case 0xb2: CON_printf(MustBeInPartyMsg, "leave"); break;
			default: CON_printf(MustBeInPartyMsg, "enter");
		}
		PromptFlag = 1;
	}
}

/*the party leader steps in direction dir (if he can), the followers follow,
 then CheckGatesAndEntrances*/
/*C_170D_1097*/PartyAdvance(int16 dir) {
	int16 obj, partyIdx, new_x, tmp_dir;
	int16 new_y, tileOrType;

	ShowQuestBarrierMsg = 1;
	obj = Party[Active];
	MapX = GetX(obj);
	MapY = GetY(obj);
	AdvanceBlocked = 0;
	if(DrunkCounter && OSI_rand(0, 1)) {
		CON_printf("you are dizzy!\n");
		PromptFlag = 1;
		dir = OSI_rand(0, 7);
	}
	tmp_dir = dir;
	new_x = (MapX + DirIncrX[tmp_dir]) & 0x3ff;
	new_y = (MapY + DirIncrY[tmp_dir]) & 0x3ff;
	if(!NPC_TryStep(obj, new_x, new_y)) {
		AdvanceBlocked = 1;
		/*diagonal mouse move blocked: slide along the main axis of the mouse*/
		if(MousePress && DirIncrX[tmp_dir] && DirIncrY[tmp_dir]) {
			if(MapDistance(MouseMapX, MapX) > MapDistance(MouseMapY, MapY)) {
				if(NPC_TryStep(obj, new_x, MapY)) {
					AdvanceBlocked = 0;
					new_y = MapY;
				} else if(NPC_TryStep(obj, MapX, new_y)) {
					AdvanceBlocked = 0;
					new_x = MapX;
				}
			} else {
				if(NPC_TryStep(obj, MapX, new_y)) {
					AdvanceBlocked = 0;
					new_x = MapX;
				} else if(NPC_TryStep(obj, new_x, MapY)) {
					AdvanceBlocked = 0;
					new_y = MapY;
				}
			}
		}
	}
	if(AdvanceBlocked) {
		ApplyTerrainEffects(obj, new_x, new_y);
		MUS_PlaySfx(0);
		MapX = GetX(obj);
		MapY = GetY(obj);
		ShowQuestBarrierMsg = 0;
		return;
	}
	/*a party member stands on the destination: it swaps places with the leader*/
	for(partyIdx = 0; partyIdx < PartySize; partyIdx ++) {
		if(Active != partyIdx) {
			obj = Party[partyIdx];
			if(
				!(NPCStatus[obj] & 0x10) &&
				GetX(obj) == new_x && GetY(obj) == new_y && GetZ(obj) == MapZ
			) {
				StepMovesParts = 1;
				UpdateStepFrame(obj, (tmp_dir + 4) & 7);
				MoveObj(obj, MapX, MapY, MapZ);
				break;
			}
		}
	}
	/*movement cost; diagonal steps cost 5 more*/
	obj = Party[Active];
	tileOrType = GetType(obj);
	if(DirIncrX[tmp_dir] && DirIncrY[tmp_dir]) {
		SubTerrainMov(obj);
		SubMov(obj, 5);
	} else {
		SubTerrainMov(obj);
	}
	StepMovesParts = 1;
	if(UpdateStepFrame(obj, tmp_dir) == 1)
		SetDirection(obj, tmp_dir);
	MapX = new_x;
	MapY = new_y;
	MAP_LoadAround();
	if(MoveObj(obj, new_x, new_y, MapZ) >= 0) {
		VisibilityDirty = 1;
	} else {
		MapX = GetX(obj);
		MapY = GetY(obj);
		MAP_LoadAround();
		CON_printf("Object mismatch!\n");
	}
	BeltUpdate(1);
	BargeFindLeader(obj);
	/*the followers don't trigger the barrier message*/
	ShowQuestBarrierMsg = 0;
	MoveFollowers(obj, 0);
	CheckGatesAndEntrances();
	ShowQuestBarrierMsg = 0;
}

/*what is left of Ultima VI's FindFurnitureHere: the search loop has lost
 its body (it never ends if there is an object on objNum's square)*/
/*C_170D_13EC*/FindFurnitureHere(int16 objNum, int16 kind) {
	int16 obj, wantedType;
	int16 x, y, z;

	x = GetX(objNum);
	y = GetY(objNum);
	z = GetZ(objNum);
	wantedType = FurnitureTypes[kind];
	for(obj = FindLoc(x, y, z); obj >= 0; );
	return obj;
}

/*objNum reached the time of a schedule entry: start its action (sleep,
 ...) or set it to find its path to the entry's location*/
/*C_170D_145E*/StartScheduleAction(int16 objNum) {
	struct coord xyz;
	int16 action, isAtDest;

	action = Schedule[SchedIndex[objNum] + SchedPointer[objNum]].action;
	xyz = Schedule[SchedIndex[objNum] + SchedPointer[objNum]].xyz;
	isAtDest = (
		ObjPos[objNum].raw[0] == xyz.raw[0] &&
		ObjPos[objNum].raw[1] == xyz.raw[1] &&
		ObjPos[objNum].raw[2] == xyz.raw[2]
	);
	if((TalkFlagsHi[objNum] & 0x80) && !(NPCStatus[objNum] & 0x80)) {
		/*told to wait*/
		isAtDest = 1;
		action = NPCMode[objNum];
	} else {
		NPCMode[objNum] = action;
	}
	if(action == 0x91/*AI_SLEEP*/) {
		isAtDest = 1;
		SetAsleep(objNum);
		if(!IsOutOfArea(objNum))
			SetTypeUnconscious(objNum);
		else
			NPCMode[objNum] = 0x80/*AI_SCHEDULE*/;
	} else {
		ClrAsleep(objNum);
		ObjShapeType[objNum] = OrigShapeType[objNum];
	}
	if(!isAtDest)
		NPCMode[objNum] = 0x81/*AI_FINDPATH*/;
}

/*build the path once the search succeeded at cell (area_x, area_y):
 first walk down the costs back to the cost-0 start cell, prepending the
 reversed steps; then (two-ended search only) walk from the meeting cell
 down the goal front to the goal, appending the steps. Path longer than 31
 entries are truncated. The result goes to PathData->content[PTH_Index].*/
/*C_170D_158F*/PTH_BuildPath(int16 area_x, int16 area_y) {
	int16 i, pathLen;
	int16 some_min, dir, dir_min, back_x, back_y, new_x, new_y;

	back_x = area_x;
	back_y = area_y;

	pathLen =
	dir = 0;
	PathData->content[PTH_Index][0] = 0;

	while(ScratchBuf->PTH_map[area_y][area_x]) {
		some_min = 0xff;
		for(i = 0; i < 4; i ++) {
			new_x = (area_x + PTH_DirX[i]) & 0x3ff;
			new_y = (area_y + PTH_DirY[i]) & 0x3ff;
			if(new_x >= 0 && new_x <= (AREA_W-1) && new_y >= 0 && new_y <= (AREA_H-1)) {
				if(
					ScratchBuf->PTH_map[new_y][new_x] < some_min ||
					(ScratchBuf->PTH_map[new_y][new_x] == some_min && dir == i)
				) {
					some_min = ScratchBuf->PTH_map[new_y][new_x];
					dir_min = i;
				}
			}
		}
		dir = dir_min;
		area_x = (area_x + PTH_DirX[dir]) & 0x3ff;
		area_y = +PTH_DirY[dir] & 0x3ff;/*sic (=+)*/

		/*new direction: shift the path and insert at the front; same
		direction: one more step in the first entry*/
		if(pathLen == 0 || (PathData->content[PTH_Index][0] & 3) != (dir ^ 2)) {
			if(pathLen < 0x1f)
				pathLen ++;
			for(i = pathLen; i > 0; i --)
				PathData->content[PTH_Index][i] = PathData->content[PTH_Index][i - 1];
			PathData->content[PTH_Index][0] = (dir ^ 2) + 4;
		} else {
			PathData->content[PTH_Index][0] += 4;
		}
	}

	if(PTH_object < 0 && PTH_direct < 0) {
		area_x = back_x;
		area_y = back_y;

		ScratchBuf->PTH_map[area_y][area_x] = 0xff;

		/*follow the goal front (values >= 0x80) down to the goal (0x80)*/
		while(ScratchBuf->PTH_map[area_y][area_x] > 0x80) {
			some_min = 0xff;
			for(i = 0; i < 4; i ++) {
				new_x = (area_x + PTH_DirX[i]) & 0x3ff;
				new_y = (area_y + PTH_DirY[i]) & 0x3ff;
				if(new_x >= 0 && new_x <= (AREA_W-1) && new_y >= 0 && new_y <= (AREA_H-1)) {
					/*only through the goal front*/
					if(ScratchBuf->PTH_map[(area_y + PTH_DirY[i]) & 0x3ff][(area_x + PTH_DirX[i]) & 0x3ff] < 0x80)
						continue;
					if(
						ScratchBuf->PTH_map[new_y][new_x] < some_min ||
						(ScratchBuf->PTH_map[new_y][new_x] == some_min && dir == i)
					) {
						some_min = ScratchBuf->PTH_map[new_y][new_x];
						dir_min = i;
					}
				}
			}
			dir = dir_min;
			area_x = (area_x + PTH_DirX[dir]) & 0x3ff;
			area_y = (area_y + PTH_DirY[dir]) & 0x3ff;
			if(pathLen >= 0x1f)
				break;
			if(pathLen == 0 || (PathData->content[PTH_Index][pathLen - 1] & 3) != dir) {
				PathData->content[PTH_Index][pathLen] = dir + 4;
				pathLen ++;
			} else {
				PathData->content[PTH_Index][pathLen - 1] += 4;
			}
		}
	}
	PathData->content[PTH_Index][pathLen] = 0;
}

/*teleport objNum to the location of its current schedule entry*/
/*C_170D_190F*/TeleportToSchedule(int16 objNum) {
	struct coord sched_xyz;
	int16 x, y, z;

	x = GetX(objNum);
	y = GetY(objNum);
	z = GetZ(objNum);
	sched_xyz = Schedule[SchedIndex[objNum] + SchedPointer[objNum]].xyz;
	x = GetCoordX(sched_xyz);
	y = GetCoordY(sched_xyz);
	if(NPCMode[objNum] >= 0x83/*AI_ONPATH*/ && NPCMode[objNum] < 0x86/*AI_PATH_FAILED*/)
		PathObject[Leader[objNum]] = 0;
	/*MoveAppliesTerrain = 0: no terrain-damage check in MoveObj*/
	MoveAppliesTerrain = 0;
	MoveObj(objNum, x, y, GetCoordZ(sched_xyz));
	MoveAppliesTerrain = 1;
	StartScheduleAction(objNum);
	SubMov(objNum, 10);
	TeleportCount ++;
}

/*path search: reach cell (x, y) with total cost "resist" from the front
 flag_80 (0 or 0x80). Detects success (other front met, area edge reached
 in direction PTH_direct); otherwise queues the cell in bucket
 PTH_rank[resist] if this is cheaper than what it had. (The search for an
 object type of Ultima VI is gone: only its FindLoc call is left.)*/
/*C_170D_19F8*/PTH_Visit(int16 resist, int16 x, int16 y, int16 flag_80) {
	int16 obj, newTop;
	int16 origin_x, origin_y;

	if(
		PTH_Found ||
		resist > 0x7f ||
		!(x >= 0 && x <= (AREA_W-1) && y >= 0 && y <= (AREA_H-1))
	) return;

	/*reached a cell of the other front: the two searches met*/
	if(ScratchBuf->PTH_map[y][x] != 0xff && (ScratchBuf->PTH_map[y][x] & 0x80) != flag_80) {
		PTH_Found = 1;
		PTH_BuildPath(x, y);
		return;
	}

	if(PTH_object >= 0) {
		origin_x = (MapX - 0x10) & 0x3f8;
		origin_y = (MapY - 0x10) & 0x3f8;
		obj = FindLoc((x + origin_x) & 0x3ff, (y + origin_y) & 0x3ff, MapZ);
	}

	/*goal outside the area: reached the edge on its side?*/
	if(PTH_direct >= 0) {
		if(
			(x == 0 && PTH_direct == 3) || (x == (AREA_W-1) && PTH_direct == 1) ||
			(y == 0 && PTH_direct == 0) || (y == (AREA_H-1) && PTH_direct == 2)
		) {
			PTH_Found = 1;
			PTH_AnX = x;
			PTH_AnY = y;
			PTH_BuildPath(x, y);
			return;
		}
	}
	/*already reached at a lower or equal cost (0xff & 0x7f = 0x7f = never)*/
	if((ScratchBuf->PTH_map[y][x] & 0x7f) <= resist)
		return;
	/*take a node from the free list (none left: give up on this cell)*/
	newTop = PTH_Top;
	if(newTop < 0)
		return;
	/*PTH_MaxCost is in fact the highest cost queued so far*/
	if(PTH_MaxCost < resist)
		PTH_MaxCost = resist;
	PTH_Top = ScratchBuf->PTH_link[newTop];
	ScratchBuf->PTH_link[newTop] = ScratchBuf->PTH_rank[resist];
	ScratchBuf->PTH_x[newTop] = x;
	ScratchBuf->PTH_y[newTop] = y;
	ScratchBuf->PTH_rank[resist] = newTop;
	ScratchBuf->PTH_map[y][x] = resist | flag_80;
}

/*reverse path slot "index" in place: entries in reverse order and each
 direction flipped (^2: N<->S, E<->W). Used when the search was run from
 the goal toward the NPC.*/
/*C_170D_1BB8*/PTH_ReversePath(int16 index) {
	int16 k, len;
	int16 tmp;

	for(len = 0; len < 0x20; len ++) {
		if(PathData->content[index][len] == 0)
			break;
	}
	for(k = 0; k < (len + 1) / 2; k ++) {
		tmp = PathData->content[index][k] ^ 2;
		PathData->content[index][k] = PathData->content[index][len - 1 - k] ^ 2;
		PathData->content[index][len - 1 - k] = tmp;
	}
}

/*find a path for objNum (stored in slot PTH_Index):
 - x >= 0: to the square (x, y). If the NPC or the goal is outside the
   area the NPC is teleported if possible (TeleportToSchedule), else the search
   goes toward the area edge on the far side (PTH_direct); when the NPC is
   the one outside, the search is run from the goal and the NPC is put on
   the edge square found ("swapped").
 - x < 0: PTH_object = y (no longer searched for, see PTH_Visit).
 Success: NPCMode = AI_ONPATH, Leader[npc] = path slot. Failure:
 AI_FINDPATH -> AI_PATH_FAILED (retry later), AI_SEEKOBJ -> AI_SCHEDULE.*/
/*C_170D_1C4B*/PTH_FindPath(int16 objNum, register int16 x, int16 y) {
	int16 start_x, start_y, i, j, node;
	int16 area_x, area_y, cost, flag_80, tmp, swapped;

	PTH_Found = swapped = 0;
	PTH_direct = -1;
	area_x = (MapX - 0x10) & 0x3f8;
	area_y = (MapY - 0x10) & 0x3f8;
	/*coord "in area"*/
	start_x = WrapDelta(GetX(objNum) - area_x);
	start_y = WrapDelta(GetY(objNum) - area_y);
	if(x >= 0) {
		/*coord "in area"*/
		x = WrapDelta(x - area_x);
		y = WrapDelta(x - area_y);/*sic*/
		PTH_object = -1;
		if(start_x == x && start_y == y) {
			StartScheduleAction(objNum);
			return;
		}
		if(!(
			GetZ(objNum) == MapZ &&
			start_x >= 0 && start_x <= (AREA_W-1) &&
			start_y >= 0 && start_y <= (AREA_H-1) &&
			x >= 0 && x <= (AREA_W-1) &&
			y >= 0 && y <= (AREA_H-1)
		)) {
			/*at most 3 teleports per turn*/
			if(TeleportCount >= 3)
				return;
			TeleportToSchedule(objNum);
			if(NPCMode[objNum] != 0x81/*AI_FINDPATH*/)
				return;
			if(!(start_x >= 0 && start_x <= (AREA_W-1) && start_y >= 0 && start_y <= (AREA_H-1))) {
				tmp = x; x = start_x; start_x = tmp;
				tmp = y; y = start_y; start_y = tmp;
				swapped = 1;
			}
			/*edge to head for: the side of the area (seen from its center
			20,20) where the outside end lies; 0 N, 1 E, 2 S, 3 W*/
			if(abs(x - 20) > abs(y - 20))
				PTH_direct = (x < 20)?3:1;
			else
				PTH_direct = (y < 20)?0:2;
		}
	} else {
		/*seeked object*/
		PTH_object = y;
	}

	/*at most 3 searches per turn (see PTH_SearchPaths)*/
	PathSearchCount ++;
	__ComputeResistance(objNum);

	/*reset the map, the node free list and the buckets*/
	for(i = 0; i < AREA_H; i ++) {
		for(j = 0; j < AREA_W; j ++)
			ScratchBuf->PTH_map[i][j] = 0xff;
	}
	for(i = 0; i < 0x100; i ++)
		ScratchBuf->PTH_link[i] = i + 1;
	for(i = 0; i < 0x100; i ++)
		ScratchBuf->PTH_rank[i] = -1;
	ScratchBuf->PTH_link[0xff] = -1;
	PTH_Top = PTH_Found = 0;
	PTH_Cur = PTH_MaxCost = 0;

	/*seed the NPC's square (front 0) and, for a square goal, the goal
	(front 0x80)*/
	PTH_Visit(0, start_x, start_y, 0);
	if(PTH_object == -1 && PTH_direct == -1)
		PTH_Visit(0, x, y, 0x80);

	/*pop the cheapest cell, give its node back to the free list and
	expand its 4 neighbours*/
	while(ScratchBuf->PTH_rank[PTH_Cur] >= 0 && !PTH_Found) {
		node = ScratchBuf->PTH_rank[PTH_Cur];
		ScratchBuf->PTH_rank[PTH_Cur] = ScratchBuf->PTH_link[node];
		x = ScratchBuf->PTH_x[node];
		y = ScratchBuf->PTH_y[node];
		ScratchBuf->PTH_link[node] = PTH_Top;
		PTH_Top = node;
		cost = ScratchBuf->PTH_map[y][x];
		flag_80 = cost & 0x80;
		cost &= 0x7f;
		PTH_Visit(cost + ScratchBuf->PTH_resist[y][(x - 1) & 0x3ff], (x - 1) & 0x3ff, y, flag_80);
		PTH_Visit(cost + ScratchBuf->PTH_resist[y][(x + 1) & 0x3ff], (x + 1) & 0x3ff, y, flag_80);
		PTH_Visit(cost + ScratchBuf->PTH_resist[(y - 1) & 0x3ff][x], x, (y - 1) & 0x3ff, flag_80);
		PTH_Visit(cost + ScratchBuf->PTH_resist[(y + 1) & 0x3ff][x], x, (y + 1) & 0x3ff, flag_80);
		/*bucket empty: advance to the next non-empty one*/
		if(ScratchBuf->PTH_rank[PTH_Cur] < 0) {
			while(PTH_Cur < PTH_MaxCost && ScratchBuf->PTH_rank[PTH_Cur] < 0)
				PTH_Cur ++;
		}
	}
	if(PTH_Found) {
		/*searched from the goal: put the NPC on the edge square reached
		and turn the path around*/
		if(swapped) {
			MoveAppliesTerrain = 0;
			MoveObj(objNum, (PTH_AnX + area_x) & 0x3ff, (PTH_AnY + area_y) & 0x3ff, GetZ(objNum));
			MoveAppliesTerrain = 1;
			PTH_ReversePath(PTH_Index);
		}
		NPCMode[objNum] = 0x83/*AI_ONPATH*/;
		Leader[objNum] = PTH_Index;
		PathObject[PTH_Index] = objNum;
		PathCounter[PTH_Index] = 0;
		PathTries[PTH_Index] = 0;
	} else if(NPCMode[objNum] == 0x81/*AI_FINDPATH*/) {
		NPCMode[objNum] = 0x86/*AI_PATH_FAILED*/;
	} else if(NPCMode[objNum] == 0x82/*AI_SEEKOBJ*/) {
		NPCMode[objNum] = 0x80/*AI_SCHEDULE*/;
	}
}

/*random signed offset with a geometric distribution: 0 (50%), +-1 (25%)...*/
/*C_170D_2117*/RandomSpread() {
	int16 n;

	for(n = 0; OSI_rand(0, 1); n ++);
	if(OSI_rand(0, 1))
		n = -n;
	return n;
}

/*consume movement points according to terrain: 5 + the upper nibble of
 TerrainType (movement cost) of the base tile and of every object on the
 object's square (impassable tiles excluded)*/
/*C_170D_214B*/SubTerrainMov(int16 numObject) {
	int16 numTile;
	int16 x, y, z, obj, pts;

	pts = 5;
	x = GetX(numObject);
	y = GetY(numObject);
	z = GetZ(numObject);
	numTile = GetTileAtXYZ(x, y, z);
	if(!IsTerrainImpass(numTile))
		pts += TerrainType[numTile] >> 4;
	for(obj = FindLoc(x, y, z); obj >= 0; obj = NextLoc()) {
		numTile = TILE_FRAME(obj);
		if(!IsTerrainImpass(numTile))
			pts += TerrainType[numTile] >> 4;
	}
	SubMov(numObject, pts);
}

/*objNum takes one step in direction dir if it can (avoidHazard: refuse
 damaging terrain); 1 if it moved. 0x178 and 0x16f creatures crush the
 objects in their way instead*/
/*C_170D_2231*/TryStraightMove(int16 objNum, int16 dir, int16 avoidHazard) {
	int16 new_x, new_y, unused, frm;
	int16 obj;

	SubTerrainMov(objNum);
	dir &= 7;
	new_x = (GetX(objNum) + DirIncrX[dir]) & 0x3ff;
	new_y = (GetY(objNum) + DirIncrY[dir]) & 0x3ff;
	if(IsPlrControl(objNum) && InCombat && (MapDistance(MapX, new_x) > 8 || MapDistance(MapY, new_y) > 8))
		return 0;
	AvoidHazards = avoidHazard;
	if(GetType(objNum) == 0x178 || GetType(objNum) == 0x16f) {
		unused = 0;
		for(obj = FindLoc(new_x, new_y, MapZ); obj >= 0; obj = NextLoc()) {
			if(IsCrushableType(GetType(obj)))
				DeleteObj(obj);
		}
	} else if(NPC_TryStep(objNum, new_x, new_y)) {
		frm = GetFrame(objNum);
		AvoidHazards = 0;
		StepMovesParts = 1;
		if(UpdateStepFrame(objNum, dir) == 1) {
			if(GetType(objNum) == 0x91) {
				/*leaves a trail of 0x91 objects*/
				for(obj = OBJ_FindTypeAt(0x91, new_x, new_y); obj >= 0; obj = OBJ_NextOfTypeAt(obj)) {
					if(!IsNPC(obj))
						break;
				}
				if(obj < 0)
					AddMapObj(0x91 | (frm << 10), 0, 0, GetX(objNum), GetY(objNum), MapZ);
				else
					SetFrame(obj, frm);
			}
			MoveObj(objNum, new_x, new_y, MapZ);
			SetDirection(objNum, dir);
			return 1;
		}
		return 0;
	}
	AvoidHazards = 0;
	return 0;
}

/*loiter around (x, y): 1 chance in 8 to take a random straight step,
 biased toward (x, y) the farther the NPC is; otherwise just wait*/
/*C_170D_2438*/LoiterNear(int16 objNum, int16 x, int16 y) {
	int16 delta_x, delta_y, dir;

	if(OSI_rand(0, 7) == 0) {
		delta_x = WrapDelta(GetX(objNum) - x);
		delta_y = WrapDelta(GetY(objNum) - y);
		if(abs(delta_y) - abs(delta_x) < RandomSpread())
			dir = (RandomSpread() > delta_x)?2:6;
		else
			dir = (RandomSpread() > delta_y)?4:0;
		TryStraightMove(objNum, dir, 1);
	} else {
		SubMov(objNum, 5);
	}
}

/*try a diagonal step combining h_dir (2/6) and v_dir (0/4).
 Returns 1 if moved.*/
/*C_170D_24F1*/__TryDiagMove(int16 objNum, int16 h_dir, int16 v_dir) {
	int16 dir;
	int16 new_x, new_y;

	if(v_dir == 0 && h_dir == 6)
		dir = 7;
	else
		dir = (v_dir + h_dir) >> 1;
	SubTerrainMov(objNum);
	new_x = (GetX(objNum) + DirIncrX[dir]) & 0x3ff;
	new_y = (GetY(objNum) + DirIncrY[dir]) & 0x3ff;
	PassThroughParty = 1;
	AvoidHazards = 1;
	PassThroughParty = 0;
	if(NPC_TryStep(objNum, new_x, new_y)) {
		StepMovesParts = 1;
		if(UpdateStepFrame(objNum, dir) == 1) {
			AvoidHazards = 0;
			MoveObj(objNum, new_x, new_y, MapZ);
			SetDirection(objNum, dir);
			return 1;
		}
		return 0;
	}
	AvoidHazards = 0;

	return 0;
}

/*take one step from objNum's position toward (new_x, new_y) (not further).
 Chooses the main axis (the larger delta; randomly weighted when far), then
 tries in order: straight on the main axis, diagonal, straight on the other
 axis, and (1 chance in 2) straight back on the other axis. Party members
 don't walk through each other meanwhile (PassThroughParty = 0).
 Returns 1 if a step was made.*/
/*C_170D_25D5*/TryMoveTo(int16 objNum, int16 new_x, int16 new_y) {
	int16 delta_x, delta_y;
	int16 h_dir, v_dir, moved, isVMove, avoidHazard;

	avoidHazard = !IgnoreHazards;
	moved = 1;
	delta_x = WrapDelta(new_x - GetX(objNum));
	delta_y = WrapDelta(new_y - GetY(objNum));
	if((delta_x == 0 && delta_y == 0) || NPCMode[objNum] == 0xe/*AI_IMMOBILE*/) {
		SubMov(objNum, 5);
		return 0;
	}
	if(delta_x == 0)
		h_dir = OSI_rand(0, 1)?2:6;
	else if(delta_x < 0)
		h_dir = 6;
	else
		h_dir = 2;
	if(delta_y == 0)
		v_dir = OSI_rand(0, 1)?0:4;
	else if(delta_y < 0)
		v_dir = 0;
	else
		v_dir = 4;
	PassThroughParty = 0;
	if(abs(delta_x) < 4 && abs(delta_y) < 4) {
		isVMove =
			(abs(delta_x) < abs(delta_y)) ||
			(abs(delta_x) == abs(delta_y) && OSI_rand(0, 1))
		;
	} else {
		isVMove = OSI_rand(1, abs(delta_x) + abs(delta_y)) > abs(delta_x);
	}
	if(isVMove) {
		if(
			!TryStraightMove(objNum, v_dir, avoidHazard) &&
			!__TryDiagMove(objNum, h_dir, v_dir) &&
			!TryStraightMove(objNum, h_dir, avoidHazard) &&
			(OSI_rand(0, 1) || !TryStraightMove(objNum, h_dir ^ 4, 1))
		) {
			SubTerrainMov(objNum);
			moved = 0;
		}
	} else {
		if(
			!TryStraightMove(objNum, h_dir, avoidHazard) &&
			!__TryDiagMove(objNum, h_dir, v_dir) &&
			!TryStraightMove(objNum, v_dir, avoidHazard) &&
			(OSI_rand(0, 1) || !TryStraightMove(objNum, v_dir ^ 4, 1))
		) {
			SubTerrainMov(objNum);
			moved = 0;
		}
	}
	PassThroughParty = 1;

	return moved;
}

/*wander at random: a party member sometimes walks back to the leader,
 others take a random straight step 1 time in 8; otherwise wait*/
/*C_170D_281F*/WanderRandomly(int16 objNum) {
	if(NPCMode[objNum] != 0xe/*AI_IMMOBILE*/) {
		if(
			IsPlrControl(objNum) &&
			(GetX(objNum) != MapX || GetY(objNum) != MapY) &&
			OSI_rand(0, 3) == 0
		) {
			TryMoveTo(objNum, MapX, MapY);
			return;
		}
		if(OSI_rand(0, 7) == 0) {
			TryStraightMove(objNum, OSI_rand(0, 3) << 1, 1);
			return;
		}
	}
	SubMov(objNum, 5);
}

/*follow the stored path one step (AI_ONPATH): slot = Leader[npc],
 PathCounter[slot] = current entry, PathTries[slot] = steps already done
 in it. At the end of the path the schedule action starts if the NPC is
 within 1 square of (x, y) (its scheduled spot), else it searches again.
 When blocked it waits up to 3 turns (AI_ONPATH -> AI_PATH_BLOCKED1 -> AI_PATH_BLOCKED2 -> AI_PATH_FAILED),
 then gives up the path (AI_PATH_FAILED = to be searched again later).*/
/*C_170D_28C1*/__DoOnPath(int16 objNum, int16 x, int16 y) {
	int16 index;
	int16 info;

	index = Leader[objNum];
	info = PathData->content[index][PathCounter[index]];
	if(info == 0) {
		PathObject[index] = 0;
		if(COMBAT_TileDistance(objNum, x, y) < 2)
			StartScheduleAction(objNum);
		else
			NPCMode[objNum] = 0x81/*AI_FINDPATH*/;
		SubMov(objNum, 5);
	} else if(TryStraightMove(objNum, (info & 3) << 1, 1)) {
		if(++PathTries[index] >= (info >> 2)) {
			PathCounter[index] ++;
			PathTries[index] = 0;
		}
		NPCMode[objNum] = 0x83/*AI_ONPATH*/;
	} else if(NPCMode[objNum] == 0x83/*AI_ONPATH*/) {
		NPCMode[objNum] = 0x84/*AI_PATH_BLOCKED1*/;
		MovePts[objNum] = 0;
	} else if(NPCMode[objNum] == 0x84/*AI_PATH_BLOCKED1*/) {
		NPCMode[objNum] = 0x85/*AI_PATH_BLOCKED2*/;
		MovePts[objNum] = 0;
	} else if(NPCMode[objNum] == 0x85/*AI_PATH_BLOCKED2*/) {
		NPCMode[objNum] = 0x86/*AI_PATH_FAILED*/;
		MovePts[objNum] = 0;
		PathObject[index] = 0;
		/*blocked right next to the goal: good enough
		(note: DirIncrX[info] uses the raw path byte, not (info & 3) << 1)*/
		if(((GetX(objNum) + DirIncrX[info]) & 0x3ff) == x && GetY(objNum) + DirIncrY[info] == y & 0x3ff/*sic*/)
			StartScheduleAction(objNum);
	}
}

/*objNum walks toward the nearest party member it can see (or toward the
 player when none); returns that member if it is now next to him, else -1*/
/*C_170D_2A06*/ApproachNearestMember(int16 objNum) {
	int16 nearest;
	int16 x, y, x_dest, y_dest, dist, dist_min;
	register int16 member, partyIdx;

	nearest = -1;
	x = GetX(objNum);
	y = GetY(objNum);
	dist_min = 50;
	for(partyIdx = 0; partyIdx < PartySize; partyIdx ++) {
		member = Party[partyIdx];
		if(ObjShapeType[member] && IsDead(member) == 0 && COMBAT_canSee(objNum, member)) {
			x_dest = GetX(member);
			y_dest = GetY(member);
			dist = COMBAT_range(x_dest, y_dest, x, y);
			if(
				(dist < dist_min) ||
				(dist == dist_min && OSI_rand(0, 1))
			) {
				dist_min = dist;
				nearest = member;
			}
		}
	}
	if(nearest < 0) {
		TryMoveTo(objNum, MapX, MapY);
	} else {
		/*note: toward the last member seen, not the nearest*/
		TryMoveTo(objNum, x_dest, y_dest);
		if(MapDistance(x_dest, GetX(objNum)) > 1 || MapDistance(y_dest, GetY(objNum)) > 1)
			nearest = -1;
	}
	return nearest;
}

/*mode 0x96: the NPC starts a conversation with the party.
 note: TALK_talkTo() expects a party index as first parameter but gets
 the member's object number here*/
/*C_170D_2B52*/TalkToParty(int16 npc, int16 member) {
	TALK_talkTo(member, npc, 0);
	PromptFlag = 1;
}

/*find a free path slot (0..7), freeing the slots whose NPC is no longer
 following a path. Returns 8 if none.*/
/*C_170D_2B6C*/__NewPathIndex() {
	int16 slot, mode;

	for(slot = 0; slot < 8; slot ++) {
		if(PathObject[slot]) {
			mode = NPCMode[PathObject[slot]];
			if(!(mode >= 0x83/*AI_ONPATH*/  && mode <= 0x86/*AI_PATH_FAILED*/))
				PathObject[slot] = 0;
		}
		if(!PathObject[slot])
			break;
	}
	return slot;
}

/*is objNum a man? by its humanoid type (0x157 the male player, 0x158 the
 female one, ...); a coin toss for the other types*/
/*C_170D_2BB4*/IsMale(int16 objNum) {
	int16 objTyp;

	objTyp = GetType(objNum);
	switch(objTyp) {
		case 0x156: case 0x157: case 0x159: case 0x15b: case 0x15c:
		case 0x15d: case 0x15e: case 0x15f: case 0x160: case 0x161:
			return 1;
		case 0x158: case 0x15a: case 0x162: case 0x163: case 0x164: case 0x165:
			return 0;
		default:
			return OSI_rand(0, 1);
	}
}

/*the A.I. of objNum for one turn, by its NPCMode*/
/*C_170D_2C12*/NPC_TakeTurn(int16 objNum) {
	int16 other;
	int16 sched_x, sched_y, startMovePts;
	struct coord sched_xyz;

	if(IsPlrControl(objNum) && !IsCharmed(objNum))
		COMBAT_stopCharm(objNum);
	startMovePts = MovePts[objNum];
	sched_xyz = Schedule[SchedIndex[objNum] + SchedPointer[objNum]].xyz;
	sched_x = GetCoordX(sched_xyz);
	sched_y = GetCoordY(sched_xyz);
	if(ObjStatus[objNum] & 0x80) {
		COMBAT_AI_Berserk(objNum);
	} else switch(NPCMode[objNum]) {
		int16 lever;
		case 0x04: COMBAT_AI_Range(objNum); break;
		case 0x06: COMBAT_AI_Berserk(objNum); break;
		case 0x08:
		case 0xa0: COMBAT_AI_Assault(objNum); break;
		case 0x0e: COMBAT_AI_Immobile(objNum); break;
		case 0x07:
		case 0x09:
		case 0x1b:
		case 0x9f: COMBAT_AI_Retreat(objNum); break;
		case 0x0a: COMBAT_AI_Like(objNum); break;
		case 0x0b: COMBAT_AI_Unfriendly(objNum); break;
		case 0x98:
			/*brawl: from time to time go and (harmlessly) hit an
			opponent near the brawl spot, then come back*/
			if(OSI_rand(0, 3) == 0) {
				other = COMBAT_pickOpponent(objNum);
				if(other >= 0) {
					if(COMBAT_TileDistance(other, sched_x, sched_y) < 5) {
						if(COMBAT_TileDistance(objNum, GetX(other), GetY(other)) < 2) {
							COMBAT_showHit(other);
							TryMoveTo(objNum, sched_x, sched_y);
						} else {
							TryMoveTo(objNum, GetX(other), GetY(other));
						}
						break;
					}
				}
			}
			LoiterNear(objNum, sched_x, sched_y);
		break;
		case 0x95: TalkWhenNear(objNum); break;
		case 0x96:
			/*converse*/
			if(MapDistance(sched_x, MapX) < 5 && MapDistance(sched_y, MapY) < 5) {
				if((other = ApproachNearestMember(objNum)) >= 0) {
					NPCMode[objNum] = 0x90;
					TalkToParty(objNum, other);
				}
			} else {
				LoiterNear(objNum, sched_x, sched_y);
			}
		break;
		case 0x83:
		case 0x84:
		case 0x85: __DoOnPath(objNum, sched_x, sched_y); break;
		case 0x82:
			/*Leader[] holds the furniture kind (index in FurnitureTypes)*/
			PTH_Index = __NewPathIndex();
			if(PTH_Index < 8) {
				PTH_FindPath(objNum, -1, FurnitureTypes[Leader[objNum]]);
				SubMov(objNum, 5);
			} else {
				MovePts[objNum] = 0;
			}
		break;
		case 0x0c:
		case 0x8f: WanderRandomly(objNum); break;
		case 0x90:
		case 0x94: LoiterNear(objNum, sched_x, sched_y); break;
		case 0x99:
			/*the coal worker: up to the coal pile (0x1be two squares north)*/
			if(TryStraightMove(objNum, 0, 1))
				break;
			if(OBJ_FindTypeAt(0x1be, GetX(objNum), (GetY(objNum) - 2) & 0x3ff) < 0)
				break;
			if(OBJ_IsOnScreen(objNum))
				MUS_PlaySfx(0x10);
			NPCMode[objNum] = 0x9a;
		break;
		case 0x9a:
			/*drops coal (0x1bf) on the conveyor belt (0xbc) south of him*/
			if(OBJ_FindTypeAt(0xbc, GetX(objNum), (GetY(objNum) + 1) & 0x3ff) >= 0) {
				if((other = AddMapObj(0x1bf, 0, 0, GetX(objNum), (GetY(objNum) + 1) & 0x3ff, MapZ)) >= 0)
					ClrLocal(other);
				NPCMode[objNum] = 0x9b;
			} else if(OBJ_FindTypeAt(0xc0, GetX(objNum), (GetY(objNum) + 1) & 0x3ff) >= 0) {
				NPCMode[objNum] = 0x9b;
			} else {
				TryStraightMove(objNum, 4, 1);
			}
		break;
		case 0x9b:
			/*waits for the coal to leave*/
			if((other = OBJ_FindTypeAt(0x1bf, GetX(objNum), (GetY(objNum) + 1) & 0x3ff)) >= 0)
				break;
			if((other = OBJ_FindTypeAt(0xc0, GetX(objNum), (GetY(objNum) + 1) & 0x3ff)) >= 0)
				break;
			NPCMode[objNum] = 0x99;
		break;
		case 0x9c:
			if(OBJ_FindTypeAt(0xbf, GetX(objNum), (GetY(objNum) + 2) & 0x3ff) >= 0) {
				NPCMode[objNum] = 0x9d;
				break;
			}
			if(TryStraightMove(objNum, 4, 1))
				break;
			if(OBJ_IsOnScreen(objNum)) {
				CON_printf(StokerBlockedMsg);
				MUS_PlaySfx(0);
			}
		break;
		case 0x9d:
			if((other = OBJ_FindTypeAt(0x1bf, GetX(objNum), (GetY(objNum) + 1) & 0x3ff)) >= 0) {
				do {
					DeleteObj(other);
				} while((other = OBJ_FindTypeAt(0x1bf, GetX(objNum), (GetY(objNum) + 1) & 0x3ff)) >= 0);
				NPCMode[objNum] = 0x9e;
			}
		break;
		case 0x9e:
			/*pulls the lever (0xe9) north of him*/
			if(TryStraightMove(objNum, 0, 1))
				break;
			lever = OBJ_FindTypeAt(0xe9, GetX(objNum), (GetY(objNum) - 1) & 0x3ff);
			if(lever < 0)
				lever = OBJ_FindTypeAt(0xe9, (GetX(objNum) + 1) & 0x3ff, (GetY(objNum) - 1) & 0x3ff);
			if(lever >= 0) {
				if(TalkFlags[0x72] & 4) {
					if(!(TalkFlags[0x73] & 4) || (TalkFlags[0x71] & 8)) {
						if(TalkFlags[0x71] & 8) {
							TalkFlags[0x73] &= ~4;
							TalkFlags[0x71] &= ~8;
							MachineryOff();
						}
						TalkFlags[0x73] |= 4;
						MachineryOn();
						Cutscene2();
					}
				} else {
					BeltStart();
					SetFrame(lever, GetFrame(lever | 4));/*sic*/
				}
				NPCMode[objNum] = 0x9c;
			} else if(OBJ_IsOnScreen(objNum)) {
				CON_printf(StokerBlockedMsg);
				MUS_PlaySfx(0);
			}
		break;
		case 0x13: COMBAT_AI_Rampage(objNum); break;
		case 0x16: COMBAT_AI_ChaseCape(objNum); break;
		case 0x12: COMBAT_AI_HiddenInSand(objNum); break;
		case 0x14: COMBAT_AI_Thief(objNum); break;
		case 0x0d: COMBAT_AI_Worm(objNum); break;
		case 0x11: COMBAT_AI_WormFlee(objNum); break;
		case 0x15: COMBAT_AI_MoveSouth(objNum); break;
		case 0x17: COMBAT_AI_Together(objNum); break;
		case 0x18: AI_SeekSeed(objNum); break;
		case 0x19: COMBAT_AI_Tentacle(objNum); break;
		case 0x1a: NPC_BecomeObject(objNum); break;
		case 0x0f:
		case 0x10:
		case 0x8b:
		case 0x8c:
		case 0x8d:
		case 0x8e:
		break;
		default:
			SubMov(objNum, 5);
	}
	if(MovePts[objNum] == startMovePts)
		SubMov(objNum, 10);
}

/*add the terrain of an object tile to the path resistance of a cell:
 a non-impassable object on impassable water makes it passable (bridge),
 an impassable one blocks it, others add their movement cost (max 0xff)*/
/*C_170D_337D*/PTH_AddObjResist(int16 tile, int16 area_x, int16 area_y) {
	int16 sum;

	if(ScratchBuf->PTH_resist[area_y][area_x] == 0xff) {
		if(IsTerrainWet(AreaTiles[area_y][area_x]) && !IsTerrainImpass(tile))
			ScratchBuf->PTH_resist[area_y][area_x] = (TerrainType[tile] >> 4) + 1;
	} else if(IsTerrainImpass(tile)) {
		ScratchBuf->PTH_resist[area_y][area_x] = 0xff;
	} else if(!IsTerrainWall(tile)) {
		sum = ScratchBuf->PTH_resist[area_y][area_x] + (TerrainType[tile] >> 4);
		if(sum > 0xff)
			sum = 0xff;
		ScratchBuf->PTH_resist[area_y][area_x] = sum;
	}
}

/*fill PTH_resist: the cost of entering each square of the area (terrain
 type, then the objects on it)*/
/*C_170D_347C*/__ComputeResistance(int16 unusedObj) {
	int16 row, col, objNum, tile;
	int16 origin_x, origin_y, area_x, area_y, objTyp, tileFlags;

	origin_x = (MapX - 0x10) & 0x3f8;
	origin_y = (MapY - 0x10) & 0x3f8;

	for(row = 0; row < AREA_H; row ++) {
		for(col = 0; col < AREA_W; col ++) {
			tile = AreaTiles[row][col];
			if(IsTerrainImpass(tile))
				ScratchBuf->PTH_resist[row][col] = 0xff;
			else
				ScratchBuf->PTH_resist[row][col] = (TerrainType[tile] >> 4) + 1;
		}
	}

	for(objNum = SearchArea(origin_x, origin_y, (origin_x + (AREA_W-1)) & 0x3ff, (origin_y + (AREA_H-1)) & 0x3ff); objNum >= 0; objNum = NextArea()) {
		area_x = WrapDelta(GetX(objNum) - origin_x);
		area_y = WrapDelta(GetY(objNum) - origin_y);
		objTyp = GetType(objNum);
		tileFlags = TileFlag[TILE_FRAME(objNum)];
		if(!IsNPC(objNum)) {
			tile = TILE_FRAME(objNum);
			PTH_AddObjResist(tile, area_x, area_y);
			if(tileFlags & 0x80/*TILE_DOUBLE_WIDTH*/) {
				if(area_x)
					PTH_AddObjResist(tile - 1, area_x - 1, area_y);
				if(tileFlags & 0x40/*TILE_DOUBLE_HEIGHT*/) {
					if(area_y)
						PTH_AddObjResist(tile - 2, area_x, area_y - 1);
					if(area_x && area_y)
						PTH_AddObjResist(tile - 3, area_x - 1, area_y - 1);
				}
			} else if(tileFlags & 0x40/*TILE_DOUBLE_HEIGHT*/) {
				if(area_y)
					PTH_AddObjResist(tile - 1, area_x, area_y - 1);
			}
		}
	}
}

/*once per turn: path searches for the NPCs (2..0xbf) waiting in AI_FINDPATH,
 at most 3 teleports and 3 searches per turn while free path slots
 remain. If no search was needed, the NPCs whose path failed (AI_PATH_FAILED) are
 put back to AI_FINDPATH for another try.*/
/*C_170D_3685*/PTH_SearchPaths() {
	int16 objNum, noneSearched;
	int16 schedEntry;
	struct coord xyz;

	noneSearched = 1;
	PTH_Index = __NewPathIndex();
	if(PTH_Index == 8)
		return;
	TeleportCount =
	PathSearchCount = 0;
	for(objNum = 2; objNum < 0xc0; objNum ++) {
		if(NPCMode[objNum] == 0x81/*AI_FINDPATH*/) {
			if(TeleportCount < 3)
				TeleportToSchedule(objNum);
			if(NPCMode[objNum] == 0x81/*AI_FINDPATH*/ && PathSearchCount < 3) {
				noneSearched = 0;
				schedEntry = SchedIndex[objNum] + SchedPointer[objNum];
				xyz = Schedule[schedEntry].xyz;
				PTH_FindPath(objNum, GetCoordX(xyz), GetCoordY(xyz));
				while(PathObject[PTH_Index] && PTH_Index < 8)
					PTH_Index ++;
				if(PTH_Index == 8)
					break;
			}
		}
	}
	if(noneSearched) {
		for(objNum = 2; objNum < 0xc0; objNum ++) {
			if(NPCMode[objNum] == 0x86/*AI_PATH_FAILED*/) {
				NPCMode[objNum] = 0x81/*AI_FINDPATH*/;
				if(TeleportCount < 3)
					TeleportToSchedule(objNum);
			}
		}
	}
}

/*check who the player controls:
 - in a vehicle, actor 0 (the vehicle) is kept healthy;
 - the Avatar (object 1) dead, or NPC 0 in the dream -> PartyUnconscious (the
   party is knocked out; nothing more in the dream);
 - if the active member is asleep/paralyzed/dead/dragged under/charmed,
   control goes to the next able member (in solo mode: back to party mode);
 - out of combat, control goes back to the Avatar as soon as he is able
   (in combat the former active member resumes its combat mode).*/
/*C_170D_3781*/CheckActiveMember() {
	int16 i;

	if(Active > PartySize)
		return;
	if(IsDead(1) || (IsDead(0) && (GameFlags & 0x10))) {
		PartyUnconscious();
		if(GameFlags & 0x10)
			return;
	}
	if(IN_VEHICLE) {
		ClrCursedCharmed(0);
		ClrAilments(0);
		ClrDraggedUnder(0);
		SetAlignment(0, 0x40/*GOOD*/);
	}
	if(SoloMember >= 0) {
		if(IsIncapacitated(Party[Active]) || IsDraggedUnder(Party[Active]) || IsCharmed(Party[Active]))
			SetPartyMode();
	}
	if(SoloMember >= 0)
		return;
	if(
		IsIncapacitated(Party[Active]) ||
		IsDraggedUnder(Party[Active]) ||
		IsCharmed(Party[Active])
	) {
		for(i = 0; i < PartySize; i ++) {
			if(
				!(IsIncapacitated(Party[i]) || IsDraggedUnder(Party[i])) &&
				!IsCharmed(Party[i]) &&
				NPCMode[Party[i]] != 0/*AI_MOTIONLESS*/
			) {
				NPCMode[Party[Active]] = 1/*AI_FOLLOW*/;
				NPCMode[Party[i]] = 2/*AI_COMMAND*/;
				Active = i;
				PromptFlag = 1;
				MapX = GetX(Party[i]);
				MapY = GetY(Party[i]);
				MAP_LoadAround();
				break;
			}
		}
	}
	if(Active < PartySize && Active && SoloMember < 0) {
		if(
			!(IsIncapacitated(Party[0]) || IsDraggedUnder(Party[0])) &&
			!IsCharmed(Party[0])
		) {
			if(InCombat) {
				NPCMode[Party[Active]] = NPCComMode[Party[Active]];
			} else {
				NPCMode[Party[Active]] = 1/*AI_FOLLOW*/;
				Active = 0;
				PromptFlag = 1;
				MapX = GetX(Party[0]);
				MapY = GetY(Party[0]);
				MAP_LoadAround();
			}
			NPCMode[Party[0]] = 2/*AI_COMMAND*/;
		}
	}
}

/*the turn scheduler, called after each player command: run the A.I. of all
 the actors of the area until a player-controlled one (AI_COMMAND) has
 to act. The next actor is the one with the highest MovePts/DEXTE ratio
 (one with MovePts >= DEXTE is taken at once); when nobody has points left
 everybody gets DEXTE more points and the clock advances one minute.
 Actors walking off the area are frozen (path NPCs teleported/re-pathed).
 AI_FOLLOW members move only with the leader (MoveFollowers); the leader
 can't act while a follower has no points left.*/
/*C_170D_3A6D*/RunNPCTurns() {
	int16 i, bestPts;
	int16 area_x, area_y, actor, bestDex, cmp, coordMask;

	CheckActiveMember();
	PTH_SearchPaths();
	actor = Party[Active];
	/*coordinates wrap at 0x400 on the surface, at 0x100 underground*/
	if(MapZ)
		coordMask = 0xff;
	else
		coordMask = 0x3ff;

	do {
		actor = -1;
		bestPts = 0;
		bestDex = 1;

		do {
			area_x = (MapX - 0x10) & 0x3f8;
			area_y = (MapY - 0x10) & 0x3f8;

			for(i = 1; i < PartySize; i ++) {
				if(NPCMode[Party[i]] == 1/*AI_FOLLOW*/ && MovePts[Party[i]] <= 0)
					MovePts[Party[0]] = 0;
			}

			for(i = 0; i < 0x100; i ++) {
				if(
					!ObjShapeType[i] ||
					NPCMode[i] == 0/*AI_MOTIONLESS*/ ||
					IsParalyzed(i) ||
					(IsAsleep(i) && NPCMode[i] != 0x80/*AI_SCHEDULE*/) ||
					IsDead(i) ||
					MovePts[i] <= 0 ||
					GetCoordUse(i) || GetZ(i) != MapZ
				) continue;
				if(
					(WrapDelta(GetX(i) - area_x) & coordMask) > (AREA_W-1) ||
					(WrapDelta(GetY(i) - area_y) & coordMask) > (AREA_H-1)
				) {
					if(NPCMode[i] >= 0x83/*AI_ONPATH*/ && NPCMode[i] <= 0x86/*AI_PATH_FAILED*/)
						TeleportToSchedule(i);
					if(NPCMode[i] >= 0x83/*AI_ONPATH*/ && NPCMode[i] < 0x86/*AI_PATH_FAILED*/) {
						PathObject[Leader[i]] = 0;
						NPCMode[i] = 0x81/*AI_FINDPATH*/;
					}
					MovePts[i] = 0;
				} else {
					if(NPCMode[i] == 1/*AI_FOLLOW*/)
						continue;
					if(NPCMode[i] == 0x80/*AI_SCHEDULE*/)
						StartScheduleAction(i);
					/*cmp > 0 <=> MovePts[i]/GetDex(i) > bestPts/bestDex*/
					cmp = MovePts[i] * bestDex - GetDex(i) * bestPts;
					if(
						MovePts[i] >= GetDex(i) || cmp > 0 ||
						(cmp == 0 && GetDex(i) > bestDex)
					) {
						actor = i;
						bestPts = MovePts[i];
						bestDex = GetDex(i);
					}
					if(MovePts[i] >= GetDex(i))
						break;
				}
			}

			/*new round: give everybody its dexterity in points (debts are
			carried over) and let a minute pass*/
			if(bestPts <= 0) {
				for(i = 0; i < 0x100; i ++) {
					MovePts[i] = (MovePts[i] < 0)?(MovePts[i] + GetDex(i)):GetDex(i);
				}
				PassTime(1);
			}

		} while(bestPts <= 0);

		if(NPCMode[actor] != 2/*AI_COMMAND*/ && NPCMode[actor] != 1/*AI_FOLLOW*/) {
			NPC_TakeTurn(actor);
			CheckActiveMember();
			/*in combat-type modes Leader[] is the actor's current target/leader;
			reset it to itself*/
			if(NPCMode[actor] > 1 && NPCMode[actor] < 0x1b)
				Leader[actor] = actor;
		}
		FlushViewIfDirty();

	} while(NPCMode[actor] != 2/*AI_COMMAND*/);

	/*give control to the member whose turn it is*/
	if(IN_VEHICLE)
		actor = 0;
	if(Party[Active] != actor) {
		for(i = 0; i < PartySize; i ++) {
			if(Party[i] == actor) {
				SetActive(i);
				PromptFlag = 1;
				break;
			}
		}
	}
}

/*initialization after a game is loaded/started: no actor with 0 dexterity,
 actor 0 (vehicle) not in a schedule mode*/
/*C_170D_3D71*/InitNPCTracker() {
	int16 i;

	for(i = 0; i < 0x100; i ++) {
		if(DEXTE[i] == 0)
			DEXTE[i] = 1;
	}
	if(NPCMode[0] >= 0x80/*AI_SCHEDULE*/)
		NPCMode[0] = 0/*AI_MOTIONLESS*/;
}

/*NPC update, every hour: for each scheduled NPC (2..0xbe) not in party or
 combat (except mode 7), find the schedule entry for this hour and day and
 make it walk there (AI_FINDPATH). Schedule entries of NPC n are
 Schedule[SchedPointer[n] .. SchedPointer[n+1]-1]; SchedIndex[n] = current
 entry relative to SchedPointer[n]. Entry time byte: bits 0-4 hour,
 bits 5-7 day of the week (0 = every day). Mode 0xa0 calms down; an NPC
 waiting (TalkFlagsHi 0x80) out of the party keeps its mode.*/
/*C_170D_3D9D*/UpdateSchedules() {
	int16 npc, entry;
	int16 first, last, when;

	for(npc = 2; npc < 0xbf; npc ++) {
		if(NPCMode[npc] == 0xa0)
			NPCMode[npc] = 0x81/*AI_FINDPATH*/;
		if((TalkFlagsHi[npc] & 0x80) && !IsPlrControl(npc))
			continue;
		if(ObjShapeType[npc] == 0)
			continue;
		if(NPCMode[npc] < 0x80/*AI_SCHEDULE*/ && NPCMode[npc] != 7)
			continue;
		if(!IsPlrControl(npc)) {
			first = SchedPointer[npc];
			last = SchedPointer[npc + 1] - 1;
			if(last >= first) {
				for(entry = last; entry >= first; entry --) {
					when = Schedule[entry].time;
					/*searched backward, so the last matching entry wins*/
					if((when & 0x1f) == Time_H)
					if((when >> 5) == ((Date_D - 1) % 7 + 1) || (when & 0xe0) == 0) {
						SchedIndex[npc] = entry - first;
						NPCMode[npc] = 0x81/*AI_FINDPATH*/;
						break;
					}
				}
			}
		}
	}
}

/*NPC objNum becomes an object (its shape, its quality and hit points in
 the amount) where it stands (mode 0x1a)*/
/*C_170D_3E97*/NPC_BecomeObject(int16 objNum) {
	int16 obj;

	if(IsNPC(objNum)) {
		obj = AddMapObj(ObjShapeType[objNum], 0, (GetQual(objNum) << 8) | HitPoints[objNum], GetX(objNum), GetY(objNum), GetZ(objNum));
		if(obj < 0) {
			CON_printf(AarghMsg);
		} else {
			SubMov(objNum, MovePts[objNum]);
			DeleteObj(objNum);
		}
	}
}

/*mode 0x95: talks to NPC 1 when he comes within 2 squares*/
/*C_170D_3F3E*/TalkWhenNear(int16 objNum) {
	if(!IsOutOfArea(objNum) && GetZ(objNum) == MapZ) {
		if(COMBAT_range(GetX(objNum), GetY(objNum), GetX(1), GetY(1)) > 2)
			SubMov(objNum, 5);
		else
			TalkToParty(objNum, 1);
	}
}
