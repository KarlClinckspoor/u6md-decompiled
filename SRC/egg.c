/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE segment 1649: creature generation "eggs"
 * (Ultima VI's egg.c, by way of Savage Empire's: SE GAME.EXE 2667).
 *
 * An egg (object type 0x1d2) is an invisible map object whose inventory
 * holds the "embryos": one object per creature (or object) type to make,
 * quantity = maximum number made. The egg's quantity is the percentage
 * chance to hatch (100: always, and always the embryo's full quantity);
 * its quality: tens 0 any time, 1 by day (6-17 h), 2 by night, units 1-4
 * the creatures' alignment (NPCStatus bits 5-6: 0, 0x20, 0x40, 0x60); an egg
 * of quality 100 or more only hatches once talk flag TalkFlags[0x46] & 8 is
 * set. An embryo's quality is the creatures' NPCMode/NPCComMode (0x1a: an
 * object type >= 0x156 made as an object), its frame theirs, its
 * ObjStatus 2 and 0x80 bits theirs. Eggs hatch when their area is loaded
 * (EGG_hatchArea), once until their "hatched" status bit (0x40) is
 * cleared. EGG_generate makes one creature with random attributes from
 * its class tables (MD's monsdata, far segment 2DE5).
 *
 * Differences from Savage Empire's: no EGG_CombatRange; EGG_GivePossessions
 * is empty; EGG_generate has no multi-tile creatures and no special types,
 * sets OrigShapeType, and creatures without a class get 15/15/15, 30 HP and
 * alignment 0x60; EGG_hatches is reworked as described above (type 0x144
 * makes a pile of objects once, type 0x172 only one creature per egg place and
 * COMBAT_AI_Worm on it, a stackable object one pile, a count of unplaced
 * creatures DreamFoesLeft printed); EGG_hatchArea wraps the area and the distance.
 */
#include "se.h"

#undef abs

/*monster tables (MD's monsdata, far segment 2DE5; class + 1)*/
extern int16 far MonsterTypes[];
extern unsigned char far MonsterStr[];
extern unsigned char far MonsterDex[];
extern unsigned char far MonsterInt[];
extern unsigned char far MonsterHP[];
extern unsigned char far MonsterAlignment[];
extern char far HearSomethingMsg[];/*"You hear something to the %s!\n"*/

extern char *DirNames[];
extern char PromptFlag;
extern int16 EnemyWarningGiven;
extern unsigned char Time_H;
extern int16 MapZ;
extern int16 MapX, MapY;
extern unsigned char DreamFoesLeft;
extern int32 GameFlags;/*0x10: in a dream*/
extern unsigned char DirOriginX, DirOriginY;/*screen position of the party*/
extern unsigned char far *D_3262;/*per NPC: talk flags 8-15 (0x80: waiting)*/
extern unsigned char ObjStatus[];
extern char MovePts[];
extern unsigned char DEXTE[];
extern unsigned char far *NPCMode;
extern unsigned char far *Level;
extern uint16 ExpPoints[];
extern unsigned char far *NPCComMode;
extern unsigned char STREN[];
extern int16 Amount[];/*quantity (low byte), quality (high byte)*/
extern unsigned char NPCStatus[];
extern unsigned char HitPoints[];
extern unsigned char TalkFlags[];
extern unsigned char INTEL[];
extern struct coord ObjPos[];
extern uint16 ObjShapeType[];
extern unsigned char far *TerrainType;
extern int16 far *OrigShapeType;
extern int16 AreaX, AreaY;
/*C0FE*/extern int16 MonClassLastType;/*GetMonsterClass: last type looked up*/
/*C100*/extern int16 MonClassLast;/*GetMonsterClass: its class*/
extern int16 HatchingDisabled;

extern int16 IsOpenWater(int16, int16);
extern int16 COMBAT_TryTeleport(int16, int16);
extern COMBAT_AI_Worm(int16);
extern int16 IsBreakableType(int16);
extern int16 CanMoveTo(int16, int16, int16);/*CanMoveTo(npc, x, y)*/
extern int16 OBJ_FindTypeAt(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 FindInv(int16);
extern int16 NextInv(void);
extern int16 NextArea(void);
extern int16 SearchArea(int16, int16, int16, int16);
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);
extern int16 AddMonster(int16, int16, int16, int16);
extern DeleteObj(int16);
extern int16 QuanType(int16);
extern MUS_EventJingle();
extern int16 MkDirection(int16, int16);
extern cdecl CON_printf(char far *, ...);
extern unsigned char GetTileAtXYZ(int16, int16, int16);
extern int16 MapDistance(int16, int16);
extern int16 OSI_rand(int16, int16);/*OSI_rand(lo, hi)*/



#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)

/*13F0*/int16 ForceHatching = 0;
/*13F2*/int16 HatchingDisabled = 0;/*set while a game is loaded*/

/*monster class of an object type (index in the monsdata tables,
  minus 1); -1 if the type is not a creature. The last answer is kept.*/
/*C_1649_000A*/GetMonsterClass(int16 objType) {
	int16 far *p;

	if(objType == MonClassLastType)
		return MonClassLast;
	MonClassLastType = objType;
	MonClassLast = 0;
	for(p = MonsterTypes + 1; *p; p ++) {
		if(*p == objType)
			return MonClassLast;
		MonClassLast ++;
	}
	return MonClassLast = -1;
}

/*random value in [val - val/2, val + val/2] (triangular distribution)*/
/*C_1649_005D*/mkRandom(int16 val) {
	int16 half = val >> 1;
	if(half)
		val = (val - half) + OSI_rand(0, half) + OSI_rand(0, half);
	return val;
}

/*the possessions of a new creature: none in Martian Dreams*/
/*C_1649_0095*/EGG_GivePossessions(int16 objNum) {
}

/*create a creature of type objType at x,y,z: attributes from its class
  (mkRandom'ized), alignment from the class. returns the new actor
  number (< 0 if none was free)*/
/*C_1649_009C*/EGG_generate(int16 objType, int16 x, int16 y, int16 z) {
	int16 objNum;
	int16 objClass;

	objNum = AddMonster(objType, x, y, z);
	if(objNum < 0)
		return objNum;
	if((objClass = GetMonsterClass(objType)) >= 0) {
		STREN[objNum] = mkRandom(MonsterStr[objClass + 1]);
		DEXTE[objNum] = mkRandom(MonsterDex[objClass + 1]);
		INTEL[objNum] = mkRandom(MonsterInt[objClass + 1]);
		HitPoints[objNum] = mkRandom(MonsterHP[objClass + 1]);
		Level[objNum] = 0;
		NPCStatus[objNum] = MonsterAlignment[objClass + 1];
	} else {
		STREN[objNum] =
		DEXTE[objNum] =
		INTEL[objNum] = 15;
		HitPoints[objNum] = 30;
		Level[objNum] = 1;
		NPCStatus[objNum] = 0x60;
	}
	D_3262[objNum] = 0;
	OrigShapeType[objNum] = objType;
	MovePts[objNum] = DEXTE[objNum];
	ExpPoints[objNum] = 0;
	NPCMode[objNum] =
	NPCComMode[objNum] = 8;

	EGG_GivePossessions(objNum);

	return objNum;
}

/*try to hatch an egg (see the module header). The first creature
  appears on the egg if it can stand there, the others are scattered
  around it (off screen unless ForceHatching is set); objects are put
  within 3 tiles. When hostile creatures appear, a message says where
  something is heard. The egg is then marked hatched (and invisible)*/
/*C_1649_019B*/EGG_hatches(register int16 egg_objNum) {
	int16 i, k, quan, objTyp, typParam, numMonsters;
	int16 eggQual, x, y, embryo_objNum, dir;
	int16 hostileOffscreen, isFirstBorn, npcMod, onlyOne, onScreenNotOk;
	int16 rx, ry;
	int16 eggX, eggY;
	register int16 monster;

	npcMod = 0;/*leftovers, unused*/
	onlyOne = 0;

	if(HatchingDisabled)
		return;
	eggX = GetX(egg_objNum);
	eggY = GetY(egg_objNum);
	onScreenNotOk = !ForceHatching;
	if(!(ObjStatus[egg_objNum] & 0x40)) {
		eggQual = GetQual(egg_objNum);
		if(eggQual < 100 || !!(TalkFlags[0x46] & 8)) {
		if(
			eggQual / 10 == 0 ||
			(eggQual / 10 == 1 && Time_H > 5 && Time_H < 18) ||
			(eggQual / 10 == 2 && (Time_H < 6 || Time_H > 17))
		) {
		/*Quantity of egg: percent chance of hatching*/
		if(OSI_rand(1, 100) <= GetQuan(egg_objNum)) {
			hostileOffscreen = 0;
			isFirstBorn = 1;
			for(embryo_objNum = FindInv(egg_objNum); embryo_objNum >= 0; embryo_objNum = NextInv()) {
				objTyp =
				typParam = GetType(embryo_objNum);
				numMonsters = GetQuan(embryo_objNum);
				if(GetQuan(egg_objNum) != 100)
					numMonsters = OSI_rand(1, numMonsters);
				if(objTyp == 0x144) {
					if(OBJ_FindTypeAt(0x144, eggX, eggY) != -1)
						return;
					for(k = 0; k < numMonsters; k ++) {
						if(monster = (AddMapObj(objTyp, 0, 1, eggX, eggY, MapZ) == -1))
							break;
					}
					EnemyWarningGiven = 1;
				} else if((objTyp < 0x156 || GetQual(embryo_objNum) == 0x1a) && QuanType(objTyp)) {
					/*a stackable object: one pile*/
					for(k = 0; k < 3; k ++) {
						rx = (eggX + OSI_rand(0, 6) - 3) & 0x3ff;
						ry = (eggY + OSI_rand(0, 6) - 3) & 0x3ff;
						if(
							!IsOpenWater(rx, ry) &&
							!(TerrainType[GetTileAtXYZ(rx, ry, MapZ)] & 2) &&
							!(TerrainType[GetTileAtXYZ(rx, ry, MapZ)] & 4) &&
							OBJ_FindTypeAt(objTyp, rx, ry) < 0
						) {
							if((monster = AddMapObj(objTyp, 0x20, numMonsters, rx, ry, MapZ)) >= 0)
								ObjShapeType[monster] = GetType(monster) + ((ObjShapeType[embryo_objNum] >> 10) << 10);
							break;
						}
					}
				} else for(i = 0; i < numMonsters; i ++) {
					monster = -1;
					if(objTyp < 0x156 || GetQual(embryo_objNum) == 0x1a) {
						/*not a creature: plain object*/
						if(IsBreakableType(objTyp) && objTyp < 0x156)
							quan = GetQual(embryo_objNum);
						else
							quan = 0;
						for(k = 0; k < 3; k ++) {
							rx = (eggX + OSI_rand(0, 6) - 3) & 0x3ff;
							ry = (eggY + OSI_rand(0, 6) - 3) & 0x3ff;
							if(
								!IsOpenWater(rx, ry) &&
								!(TerrainType[GetTileAtXYZ(rx, ry, MapZ)] & 2) &&
								!(TerrainType[GetTileAtXYZ(rx, ry, MapZ)] & 4) &&
								OBJ_FindTypeAt(objTyp, rx, ry) < 0 &&
								OBJ_FindTypeAt(0x14b, rx, ry) < 0
							) {
								if((monster = AddMapObj(objTyp, 0x20, quan, rx, ry, MapZ)) >= 0) {
									ObjShapeType[monster] = GetType(monster) + ((ObjShapeType[embryo_objNum] >> 10) << 10);
									if(objTyp == 0x12c && !OSI_rand(0, 1))
										AddMapObj(0x133, 0x20, OSI_rand(4, 10), rx, ry, MapZ);
								}
								break;
							}
						}
					} else {
						if(objTyp == 0x172 && OBJ_FindTypeAt(objTyp, eggX, eggY) >= 0)
							monster = -1;
						else
							monster = EGG_generate(typParam, eggX, eggY, MapZ);
						if(monster >= 0) {
							ObjShapeType[monster] = GetType(monster) + ((ObjShapeType[embryo_objNum] >> 10) << 10);
							switch(eggQual % 10) {
								case 1: NPCStatus[monster] &= 0x9f; break;
								case 2: NPCStatus[monster] = (NPCStatus[monster] & 0x9f) | 0x20; break;
								case 3: NPCStatus[monster] = (NPCStatus[monster] & 0x9f) | 0x40; break;
								case 4: NPCStatus[monster] = (NPCStatus[monster] & 0x9f) | 0x60; break;
							}
							if(NPCStatus[monster] & 0x20)
								hostileOffscreen = 1;
							NPCComMode[monster] = NPCMode[monster] = GetQual(embryo_objNum);
							if(NPCComMode[monster] == 0x12 || (ObjStatus[embryo_objNum] & 2))
								ObjStatus[monster] |= 2;
							if(ObjStatus[embryo_objNum] & 0x80)
								ObjStatus[monster] |= 0x80;
						} else if(DreamFoesLeft) {
							DreamFoesLeft --;
						}
						if(monster >= 0) {
							if(!isFirstBorn || !CanMoveTo(monster, eggX, eggY)) {
								if(!COMBAT_TryTeleport(monster, onScreenNotOk)) {
									DeleteObj(monster);
									if(typParam >= 0x156 && DreamFoesLeft) {
										CON_printf("monster %d (0x%x) unplaced.\n", DreamFoesLeft, typParam);
										DreamFoesLeft --;
									}
									monster = -1;
								}
							} else {
								isFirstBorn = 0;
							}
							if(monster >= 0 && typParam == 0x172)
								COMBAT_AI_Worm(monster);
						}
					}
				}
			}
			/*"You hear something to the <dir>"*/
			if(!EnemyWarningGiven && hostileOffscreen && !(GameFlags & 0x10)) {
				x = eggX - MapX;
				y = eggY - MapY;
				dir = MkDirection((x << 4) + DirOriginX, (y << 4) + DirOriginY);
				MUS_EventJingle();
				CON_printf(HearSomethingMsg, DirNames[dir]);
				PromptFlag = 1;
				EnemyWarningGiven = 1;
			}
		}
		}
		}
	}
	ObjStatus[egg_objNum] |= 0x40;/*hatched*/
	ObjStatus[egg_objNum] |= 0x02;/*invisible*/
}

/*hatch the eggs of the current level in the loaded 40x40 area: the
  ones out of sight (more than 8 tiles away), or all of them with
  ForceHatching*/
/*C_1649_081E*/EGG_hatchArea() {
	int16 objNum, x, y, objTyp;

	if(HatchingDisabled)
		return;
	x = AreaX;
	y = AreaY;
	for(objNum = SearchArea(x, y, (x + 39) & 0x3ff, (y + 39) & 0x3ff); objNum >= 0; objNum = NextArea()) {
		x = GetX(objNum);
		y = GetY(objNum);
		objTyp = GetType(objNum);
		if(objTyp == 0x1d2 && GetZ(objNum) == MapZ) {
			if(ForceHatching || MapDistance(x, MapX) > 8 || MapDistance(y, MapY) > 8)
				EGG_hatches(objNum);
		}
	}
}
