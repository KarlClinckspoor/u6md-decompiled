/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE
 * segment 07D1 (from 07D1:000D, after objcmd.c's last bytes): combat
 * (Savage Empire's combat.c, Ultima VI's, reworked).
 *
 * - combat mode on/off for the party (COMBAT_begin, COMBAT_breakOff; the
 *   combat mark drawn on the command bar),
 * - derived attributes (GetStr with the first talent, GetDex, GetInt,
 *   MaxHP: 240 for NPC 6, 2*STR + 24*level for the party, the class's
 *   MonsterHP for the others),
 * - targets and missiles (COMBAT_GetTargetAt, COMBAT_Missile,
 *   COMBAT_FireRangedWeapon with C_100B_00FF's missile tile, sound and
 *   ammunition), the spraying weapons (C_07D1_0D54),
 * - death (SetTypeUnconscious: MD's bodies 0x151-0x155; COMBAT_Kill: the
 *   creature 0x175 and its parts, the NPCs whose death makes the game
 *   unwinnable, the dream stages), bleeding (C_07D1_1483), damage
 *   (LoseHP(obj, damage, kind): kind 1 paralyzes and freezes, 3 burns),
 * - resolving one attack (__TryHit -> __AttackTest -> __DoHit -> LoseHP),
 * - the "Attack" command (COMBAT_attack, COMBAT_AttackWithNextWeapon:
 *   MD's ammunition checks; the weapon 0x139 strikes through C_07D1_5FEF),
 *   combat distances (COMBAT_range, C_07D1_35F5 for the tiles of two or
 *   four squares), the choice of weapons (__PickWeapon, C_07D1_3D62) and
 *   of targets (COMBAT_pickOpponent),
 * - the combat A.I. modes: SE's (Assault, Range, Berserk, Retreat, Like,
 *   Unfriendly, Immobile) reworked, and MD's own: 0x13 and 0x16 (the
 *   dream's crushers), 0x12 and 0x19 (the creature 0x175 under the sand
 *   and its parts 0x16a), 0x14 (the oxium thief), 0x0d and 0x11 (the
 *   worms 0x172/0x174/0x173, C_07D1_52E6 backing up), 0x15, 0x17 (the
 *   0x198 and 0x1a8 together), COMBAT_beginAttacked,
 * - special deaths and hits: C_07D1_5DB2 (the 0x17b swallowing a body),
 *   C_07D1_5F2B (class flag 0x800 explodes), C_07D1_632C (NPC 0x40 hurls
 *   the player).
 * Kept from SE but for addresses: IsInvulnerable's form, COMBAT_stopCharm,
 * GetDex, GetInt, ComputeLevel, AddXP, COMBAT_canSee, COMBAT_Missile,
 * COMBAT_InitWeaponList, COMBAT_attack (but its pending command 0x81),
 * COMBAT_beginAttacked (but the mark and music); the rest reworked or new.
 * The map wraps (sums & 0x3ff, WrapDelta, MapDistance).
 */
#include "se.h"

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;
};

#undef abs

/*monster tables (MD's monsdata, far segment 2DE5; class + 1)*/
extern unsigned char far C_2DE5_0066[];/*MonsterStr*/
extern unsigned char far C_2DE5_0098[];/*MonsterDex*/
extern unsigned char far C_2DE5_00CA[];/*MonsterInt*/
extern unsigned char far C_2DE5_00FC[];/*MonsterArmor*/
extern unsigned char far C_2DE5_012E[];/*MonsterDamage*/
extern unsigned char far C_2DE5_0160[];/*MonsterHP*/
extern int16 far C_2DE5_01C6[];/*MonsterFlags (by GetMonsterClass)*/

/*far messages (segment 2BB4)*/
extern char far C_2BB4_0000[];/*"critical!\n"*/
extern char far C_2BB4_000B[];/*"heavily"*/
extern char far C_2BB4_0013[];/*"lightly"*/
extern char far C_2BB4_001B[];/*"barely"*/
extern char far C_2BB4_0022[];/*" wounded.\n"*/
extern char far C_2BB4_002D[];/*" damaged.\n"*/
extern char far C_2BB4_0038[];/*"`%s grazed.\n"*/
extern char far C_2BB4_0045[];/*"`%s killed!\n"*/
extern char far C_2BB4_0052[];/*"`%s destroyed!\n"*/
extern char far C_2BB4_0062[];/*"`%s falls!\n"*/
extern char far C_2BB4_00EB[];/*"with bare hands-"*/
extern char far C_2BB4_00FC[];/*"with %s-"*/
extern char far C_2BB4_02A4[];/*"It has no effect.\n"*/
extern char far C_2BB4_0FDE[];/*"`%s becomes visible!\n"*/
extern char far C_2BB4_13B1[];/*"`%s paralyzed!\n"*/
extern char far C_2BB4_1393[];/*"`%s attacks!\n"*/
extern char far C_2BB4_174D[];/*"You have killed someone whose help you need to win the game!\n"*/
extern char far C_2BB4_20C6[];/*"The ice thaws.\n"*/
extern char far C_2BB4_20D6[];/*"The water freezes.\n"*/
extern char far C_2BB4_22F1[];/*"Aargh!\n"*/

extern char far C_2BB4_00D3[];
extern char far C_2BB4_00E4[];
extern char far C_2BB4_0105[];
extern char far C_2BB4_0115[];
extern char far C_2BB4_0129[];
extern char far C_2BB4_034D[];
extern char far C_2BB4_03A6[];
extern char far C_2BB4_03F1[];
extern char far C_2BB4_0402[];

/*variables of other modules*/
extern char DirIncrX[];/*DirIncrX*/
extern char DirIncrY[];/*DirIncrY*/
extern unsigned char D_0F12;/*StatusDirty*/
extern unsigned char D_0F13;/*PromptFlag*/
extern char D_0F15;/*IsArmageddon: 1 the game cannot be won any more, 2 (some NPCs)*/
extern int16 D_1431;/*VisibilityDirty*/
extern int16 D_1437;/*EnemyWarningGiven*/
extern unsigned char Active;/*Active*/
extern int16 D_167C, D_167E, D_1680;/*MapX, MapY, MapZ*/
extern unsigned char D_1684, D_1685;/*where the dreamer wakes up (x, y)*/
extern unsigned char D_16A3;/*creatures left to kill in the dream stage*/
extern char D_16A4;/*InCombat*/
extern char D_16A5;/*SoloMember: -1 in party mode*/
extern unsigned char D_16A6;/*SavedNPCMode*/
extern unsigned char D_16A8;/*the player is a woman*/
extern int32 D_16AA;/*flags (0x10: the dream)*/
extern struct { unsigned a:4, b:4, c:4, d:4; } D_16B0[];/*talents*/
extern int16 D_16D4;/*DreamStage*/
extern unsigned char PartyListTop;/*PartyListTop*/
extern char SelectMode;/*SelectMode*/
extern int16 TextOn;/*TextOn*/
extern int16 EndPagingAtPrompt;/*EndPagingAtPrompt*/
extern int16 LastAttackTarget;/*LastAttackTarget*/
extern int16 StatusDisplay;/*StatusDisplay: 0x92 when the equipment view is shown*/
extern int16 StatusMember;/*StatusMember*/
extern unsigned char D_2805;/*IsInConversation*/
extern struct tSurface D_3232;/*Screen*/
extern unsigned char D_3266[];/*ObjStatus*/
extern char D_44C4[];/*MovePts*/
extern unsigned char far *D_45C4;/*SchedIndex*/
extern unsigned char far *D_45CC;/*NPCFlag*/
extern unsigned char D_45D0[];/*DEXTE*/
extern unsigned char far *D_46D0;/*NPCMode*/
extern unsigned char D_4EA4[];/*Party*/
extern unsigned char far *D_4EB6;/*Level*/
extern unsigned char far *D_50CA;/*Leader*/
extern char D_50CE[][14];/*Names*/
extern uint16 D_51AE[];/*ExpPoints*/
extern unsigned char far *D_53AE;/*NPCComMode*/
extern unsigned char D_53B2[];/*STREN*/
extern int16 D_54BA[];/*Amount*/
extern unsigned char D_6CBA[];/*NPCStatus*/
extern unsigned char D_6DBE[];/*HitPoints*/
extern int16 far *D_6EBE;/*BaseTile*/
extern unsigned char D_6EC6[];/*TalkFlags*/
extern unsigned char far *D_760A;/*TileFlag2*/
extern unsigned char D_760E[];/*INTEL*/
extern struct coord D_78AE[];/*ObjPos*/
extern unsigned char D_9CC0;/*PartySize*/
extern uint16 D_9CC4[];/*ObjShapeType*/
extern unsigned char far *D_B4C4;/*TerrainType*/
extern int16 far *D_B508;/*OrigShapeType*/
extern int16 D_B9CA;/*MissileHitObj*/
extern int16 D_B9CC, D_B9CE;/*KillX, KillY: where the last actor killed was*/
extern int16 D_B9D0;/*KillBody: its body (-1: none)*/
extern int16 D_B9D2[2];/*MissileEnd*/
extern int16 D_BA56[8];/*EquipWeapons*/
extern union tHandler D_BA6A;/*GrDriver*/
extern void far *D_BA7E;/*mdscreen.lzc entry 2: default command mark*/
extern int16 D_EB04[8];/*Equipment*/
extern int16 D_EBCE;/*Selection.obj*/

/*functions of other modules*/
extern int16 OSI_rand(int16, int16);/*OSI_rand(lo, hi)*/
extern AnimateTiles(void);
extern DrawMapWindow(int16);
extern DisplayTick(void);
extern C_120E_024B(int16);
extern PassTime(int16);
extern SetPartyMode(void);
extern int16 CON_mouseOffAt(int16, int16, int16, int16);
extern CON_mouseOn(int16);
extern cdecl CON_printf(char far *, ...);
extern unsigned char GetTileAtXYZ(int16, int16, int16);
extern int16 WrapDelta(int16);
extern int16 MapDistance(int16, int16);
extern int16 GetMonsterClass(int16);
extern int16 CanMoveTo(int16, int16, int16);
extern VIS_UpdateView(void);
extern int16 FindLoc(int16, int16, int16);
extern int16 NextLoc(void);
extern int16 OBJ_FindTypeAt(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 FindInv(int16);
extern int16 NextInv(void);
extern int16 FindInvOfType(int16, int16);
extern TryMoveTo(int16, int16, int16);
extern WanderRandomly(int16);
extern TryStraightMove(int16, int16, int16);
extern ShakeScreen(int16, int16);
extern C_2F5F_0AC1(int16);
extern DreamWakeUp();
extern C_2F75_0B90();/*a cut scene*/
extern int16 D_C0FA, D_C0FC;/*AreaX, AreaY*/
extern int16 D_14F2;/*LocPartOffset*/
extern int16 InsertObj(int16, int16, int16);
extern int16 OBJ_FindNPCAt(int16, int16, int16);/*OBJ_FindNPCAt(startNum, x, y)*/
extern int16 C_170D_0002(int16);
extern int16 C_1B09_090B(int16);/*on the screen*/
extern int16 far *Explosion(int16, int16, int16);/*Explosion(x, y, missileTile): the objects hit, -1 terminated*/
extern int16 C_170D_007E(int16, int16, int16);
extern int16 C_170D_0853(int16, int16);
extern int16 NPC_TryStep(int16, int16, int16);
extern int16 C_1B09_0A5D(int16);/*the next map object of the same type on the same square*/
extern void cdecl TimerDelay(int32);/*wait (2383 per millisecond)*/
extern int16 FindInvType(int16, int16, int16);/*FindInvType(owner, type, quality)*/
extern int16 SearchArea(int16, int16, int16, int16);/*SearchArea(x1, y1, x2, y2)*/
extern int16 NextArea(void);
extern int16 AddMonster(int16, int16, int16, int16);/*AddMonster(type, x, y, z)*/
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);/*AddMapObj(shape, status, amount, x, y, z)*/
extern int16 AddInvObj(int16, int16, int16, int16);/*AddInvObj(shapeType, status, amount, owner)*/
extern DeleteObj(int16);
extern int16 OBJ_MakeDeadBody(int16);
extern MoveObj(int16, int16, int16, int16);
extern int16 C_1B09_268D(int16);/*moves a map object after the last one of its square*/
extern TakeObj(int16, int16, int16);/*TakeObj(npc, type, quantity)*/
extern ShowObject(int16, int16, int16, int16);/*ShowObject(tile, x, y, mode)*/
extern ShowObjects(void);
extern char *GetObjectString(int16);
extern OBJ_DeleteContents(int16);
extern int16 GetDamagePoints(int16);
extern int16 GetWeaponRange(int16);
extern int16 C_100B_00C4(int16);/*protection of an armour object*/
extern C_100B_00FF(int16, int16 *, int16 *, int16 *);/*the missile tile, sound and ammunition of a weapon type*/
extern int16 C_100B_0247(int16);
extern int16 QuanType(int16);
extern int16 C_100B_038E(int16);/*breakable type*/
extern int16 C_100B_0410(int16);
extern int16 EGG_generate(int16, int16, int16, int16);/*EGG_generate(type, x, y, z)*/
extern PartyTeleport(int16, int16, int16, int16);
extern int16 Is_6C_6D_6E_4D_4F(int16);
extern int16 C_047A_13C5(int16, int16);/*open water*/
extern STAT_ShowParty(int16);
extern STAT_refreshComMode(void);
extern STAT_ShowInventory(int16);
extern CheckActiveMember(void);
extern TALK_talkTo(int16, int16, int16);
extern MUS_PlaySfx(int16);
extern C_1FA9_0555(void);/*the combat music*/
extern C_1FA9_032B(void);/*the music of a combat begun by the enemy*/
extern C_1FA9_03DA(int16);/*the music at the end of a combat (CombatOutcome)*/
extern int16 Missiles(int16, int16, int16, int16 *, int16 *, int16, int16, int16);
extern int16 far *C_0E1A_1003(int16, int16, int16, int16, int16);/*a spray of 12 missiles: the objects hit, -1 terminated*/
extern C_2F63_0086(void);/*the dreamer passes out*/
extern C_2F63_062F(int16);
extern char *C_1B09_46E4(int16);/*GetTileString*/
extern int16 C_2238_0062(int16, int16);/*MkDirection*/
extern C_170D_083F(int16, int16);/*UpdateMoveFrame(npc, direction)*/
extern far CON_UpdateMousePointer();
extern int16 CON_getch();
extern C_170D_0868(int16, int16);/*SubMov(npc, points)*/
extern C_120E_18B4();/*RefreshStatus*/

extern char AimX, AimY;
extern char SelectRange;
extern int16 MouseMode;
extern int16 PendingCommand;
extern int16 SkipNewlines;
extern int16 LastNumberPlural;
extern int16 D_0F18;/*MapAnimEnabled*/
extern int16 D_EB74, D_EB76;/*PointerX, PointerY*/
extern int16 D_EBF6, D_EBF8;/*Selection.x, Selection.y*/
extern int16 D_EC44;/*SelectionIsDark*/
extern unsigned char far *D_6DBA;/*TileFlag*/

#define GetTileString C_1B09_46E4
#define MkDirection C_2238_0062
#define UpdateMoveFrame C_170D_083F
#define SubMov C_170D_0868
#define RefreshStatus C_120E_18B4
#define MapAnimEnabled D_0F18
#define PointerX D_EB74
#define PointerY D_EB76
#define SelectionX D_EBF6
#define SelectionY D_EBF8
#define SelectionIsDark D_EC44
#define TileFlag D_6DBA
#define AreaX D_C0FA
#define AreaY D_C0FC
#define LocPartOffset D_14F2

#define StatusDirty D_0F12
#define PromptFlag D_0F13
#define IsArmageddon D_0F15
#define VisibilityDirty D_1431
#define EnemyWarningGiven D_1437
#define MapX D_167C
#define MapY D_167E
#define MapZ D_1680
#define InCombat D_16A4
#define SoloMember D_16A5
#define SavedNPCMode D_16A6
#define DreamStage D_16D4
#define IsInConversation D_2805
#define Screen D_3232
#define ObjStatus D_3266
#define MovePts D_44C4
#define SchedIndex D_45C4
#define NPCFlag D_45CC
#define DEXTE D_45D0
#define NPCMode D_46D0
#define Party D_4EA4
#define Level D_4EB6
#define Leader D_50CA
#define Names D_50CE
#define ExpPoints D_51AE
#define NPCComMode D_53AE
#define STREN D_53B2
#define Amount D_54BA
#define NPCStatus D_6CBA
#define HitPoints D_6DBE
#define BaseTile D_6EBE
#define TalkFlags D_6EC6
#define TileFlag2 D_760A
#define INTEL D_760E
#define ObjPos D_78AE
#define PartySize D_9CC0
#define ObjShapeType D_9CC4
#define TerrainType D_B4C4
#define OrigShapeType D_B508
#define MissileHitObj D_B9CA
#define KillX D_B9CC
#define KillY D_B9CE
#define KillBody D_B9D0
#define MissileEnd D_B9D2
#define EquipWeapons D_BA56
#define GrDriver D_BA6A
#define Equipment D_EB04
#define SelectionObj D_EBCE
#define MonsterStr C_2DE5_0066
#define MonsterDex C_2DE5_0098
#define MonsterInt C_2DE5_00CA
#define MonsterArmor C_2DE5_00FC
#define MonsterDamage C_2DE5_012E
#define MonsterHP C_2DE5_0160
#define MonsterFlags C_2DE5_01C6

#define GR_DRAW_BITMAP(s,x,y,p,m)    (GrDriver.iii.ofs = 0x30, (*GrDriver.f)(s,x,y,p,m))

#define TILE_AT(x, y) GetTileAtXYZ(x, y, MapZ)
#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))
#define GetCoordX(o) ((*(int16 *)&((o).raw[0]))&0x3ff)
#define GetCoordY(o) (((*(int16 *)&((o).raw[1]))>>2)&0x3ff)
#define GetCoordZ(o) ((o).raw[2] >> 4)
#define GetX(i) GetCoordX(ObjPos[i])
#define GetY(i) GetCoordY(ObjPos[i])
#define GetZ(i) GetCoordZ(ObjPos[i])
#define GetQuan(o) (((unsigned char *)(Amount + (o)))[0])
#define SubQuan(o, v) (GetQuan(o) -= (v))
#define GetQual(o) (((unsigned char *)(Amount + (o)))[1])
#define SubQual(o, v) (GetQual(o) -= (v))
#define GetDirection(i) (NPCFlag[i] & 7)
#define SetHurtThisTurn(i) (NPCFlag[i] |= 8)
#define ClrInvisible(i) ObjStatus[i] &= ~2
#define ClrAsleep(i) (NPCStatus[i] &= ~4)
#define SetDead(i) (NPCStatus[i] |= 0x10)
#define IsPoisoned(i) (NPCStatus[i] & 8)
#define SetPoisoned(i) (NPCStatus[i] |= 8)
#define SetAsleep(i) (NPCStatus[i] |= 4)
#define GetAlignment(i) (NPCStatus[i] & 0x60)
#define Is_ATKPLR(i) (GetAlignment(i) & 0x20)
#define Is_ATKMON(i) (GetAlignment(i) & 0x40)
#define SetFrame(i, v) (ObjShapeType[i] = GetType(i) + ((v) << 10))
#define SetDirection(i, v) (NPCFlag[i] = (NPCFlag[i] & ~7) | (v))
#define SetType(i, v) (ObjShapeType[i] = (ObjShapeType[i] & ~0x3ff) + (v))
#define SetQuan(o, v) (GetQuan(o) = (v))
#define SetQual(o, v) (GetQual(o) = (v))
#define IsNPC(i) ((i) < 0x100 && (i) >= 0)
#define GetCoordUse(i) (ObjStatus[i] & 0x18)
#define IsInvisible(i) (ObjStatus[i] & 2)
#define IsTileIg(tile) (TileFlag2[tile] & 0x10)
#define IsProtected(i) (NPCStatus[i] & 1)
#define IsCharmed(i) (ObjStatus[i] & 4)
#define IsCursed(i) (ObjStatus[i] & 0x40)
#define ClrCharmed(i) ObjStatus[i] &= ~4
#define SetAlignment(i, v) (NPCStatus[i] = (NPCStatus[i] & ~0x60) | (v))
#define GetBKAlignment(i) (NPCFlag[i] & 0x60)
#define IsIncapacitated(i) (NPCStatus[i] & 0x16)
#define IsAsleep(i) (NPCStatus[i] & 4)
#define IsDead(i) (NPCStatus[i] & 0x10)
#define IsPlrControl(i) (NPCStatus[i] & 0x80)
#define IsDraggedUnder(i) (NPCFlag[i] & 0x10)

/*this module's functions used before their definition*/
C_07D1_1378(int16);
C_07D1_1483(int16, int16, int16, int16);
C_07D1_274C(int16, int16, int16, int16);
extern int16 C_07D1_35F5(int16, int16, int16);/*distance of (x, y) from an object*/
extern COMBAT_beginAttacked(void);/*C_07D1_5CAF*/
extern C_07D1_5DB2(int16);
extern C_07D1_5F2B(int16, int16);
extern C_07D1_632C(void);
extern int16 C_07D1_5FEF(int16, int16, int16, int16);

/*when 0, the next COMBAT_showHit() call skips the hit flash*/
/*06BA*/int16 ShowHitFlash = 1;
/*object firing the current missile (-1: none); lets Missiles() find
  the object hit at the impact tile (COMBAT_GetTargetAt)*/
/*06BC*/int16 MissileShooter = -1;
/*how the current combat goes, for the music at its end (C_1FA9_03DA):
  -1 a party member died, 1 an enemy died, 0 nothing yet*/
/*06BE*/int16 CombatOutcome = 0;

/*range table: RangeTable[|dx|][|dy|] is the "combat distance" between two
  tiles (roughly max + min/2)*/
/*06C0*/unsigned char RangeTable[][8] = {
	0,1,2,3,4,5,6,7,
	1,1,2,3,4,5,6,7,
	2,2,2,3,4,5,6,7,
	3,3,3,4,5,6,7,7,
	4,4,4,5,6,7,7,8,
	5,5,5,6,7,7,8,8,
	6,6,6,7,7,8,8,8,
	7,7,7,7,8,8,8,8
};

/*0700*/static int16 NumEquipWeapons = 0;/*number of equiped weapons in EquipWeapons*/
/*0702*/static int16 CurEquipWeapon = 0;/*index in EquipWeapons of the weapon currently attacking*/
/*0704*/char D_0704 = 0;

/*the spraying weapons (C_07D1_0D54): their type, missile tile (0xff:
  none), kind of hit (C_07D1_274C), damage and sound; 0xf1 freezes*/
/*0705*/int16 SprayTypes[] = {0x2b, 0x2d, 0xf0, 0xf1, 0};
/*070F*/int16 SprayTiles[] = {0x106, 0x106, 0x14e, 0x14f, 0xff};
/*0719*/unsigned char SprayKinds[] = {0, 0, 3, 1, 0};
/*071E*/unsigned char SprayDamages[] = {0x14, 0x14, 0x19, 0x0a, 0x0c};
/*0723*/unsigned char SpraySfx[] = {8, 8, 10, 10, 0xff};
/*direction (0 N, 1 NE ... 7 NW) from (x0, y0) towards (x1, y1), on the
  wrapping map*/
/*C_07D1_000D*/C_07D1_000D(int16 x0, int16 y0, int16 x1, int16 y1) {
	int16 delta_x, delta_y;

	delta_x = WrapDelta(x1 - x0);
	delta_y = WrapDelta(y1 - y0);
	if(delta_x < 0) {
		if(delta_y < 0)
			return 7;
		if(delta_y)
			return 5;
		return 6;
	}
	if(delta_x) {
		if(delta_y < 0)
			return 1;
		if(delta_y)
			return 3;
		return 2;
	}
	if(delta_y < 0)
		return 0;
	return 4;
}

/*actors that cannot be hurt: the ones of original type 0x17d, 0x187,
  0x186, 0x166, 0x17e and NPCs 0x5c, 0x5d, 0x59, 0x5a, 0x54, 0x52, 0x6d,
  0x68, 0x67, 0x69, 0x77, 0x78, 0x40, 0x3c*/
/*C_07D1_0075*/IsInvulnerable(int16 objNum) {
	int16 objType;

	objType = OrigShapeType[objNum] & 0x3ff;
	if(
		objType == 0x17d || objType == 0x187 || objType == 0x186 || objType == 0x166 || objType == 0x17e ||
		objNum == 0x5c || objNum == 0x5d || objNum == 0x59 || objNum == 0x5a || objNum == 0x54 ||
		objNum == 0x52 || objNum == 0x6d || objNum == 0x68 || objNum == 0x67 || objNum == 0x69 ||
		objNum == 0x77 || objNum == 0x78 || objNum == 0x40 || objNum == 0x3c
	) return 1;
	return 0;
}

/*distance in tiles (max(|dx|,|dy|), on the wrapping map) between an
  object and a location*/
/*C_07D1_00FB*/COMBAT_TileDistance(int16 objNum, int16 x, int16 y) {
	int16 delta_x, delta_y;

	delta_x = MapDistance(GetX(objNum), x);
	delta_y = MapDistance(GetY(objNum), y);
	if(delta_x > delta_y)
		return delta_x;
	else
		return delta_y;
}

/*end the "charm" effect: restore the original alignment (kept in
  NPCFlag) and give the NPC back its normal mode*/
/*C_07D1_014C*/COMBAT_stopCharm(int16 objNum) {
	ClrCharmed(objNum);
	SetAlignment(objNum, GetBKAlignment(objNum));
	if(IsPlrControl(objNum))
		SetAlignment(objNum, 0x40);
	if(InCombat)
		NPCMode[objNum] = NPCComMode[objNum];
	else if(SoloMember < 0)/*party mode*/
		NPCMode[objNum] = 1;
	else/*solo mode*/
		NPCMode[objNum] = 0;
}

/*enter combat mode: every party member takes its combat mode
  (NPCComMode), the controlled member (first able one in party mode, the
  solo member otherwise) gets mode 2 (command), the combat mark is drawn
  on the command bar and the combat music starts*/
/*C_07D1_01BB*/COMBAT_begin() {
	int16 partyIdx, mouseSave;

	InCombat = 1;
	CombatOutcome = 0;
	for(partyIdx = 0; partyIdx < PartySize; partyIdx ++) {
		if(!IsCharmed(Party[partyIdx]))
			NPCMode[Party[partyIdx]] = NPCComMode[Party[partyIdx]];
		Leader[Party[partyIdx]] = Party[partyIdx];
	}
	if(SoloMember < 0) {
		for(
			partyIdx = 0;
			(IsIncapacitated(Party[partyIdx]) || IsDraggedUnder(Party[partyIdx])) && partyIdx < PartySize;
			partyIdx ++
		);
	} else {
		partyIdx = SoloMember;
	}
	SavedNPCMode = NPCMode[Party[partyIdx]];
	NPCMode[Party[partyIdx]] = 2;
	mouseSave = CON_mouseOffAt(0x8e, 0xa3, 0xa0, 0xc3);
	GR_DRAW_BITMAP(&Screen, 0x93, 0xa9, D_BA7E, 0);
	CON_mouseOn(mouseSave);
	C_1FA9_0555();
}

/*leave combat mode: party members follow again (or stay put in solo
  mode), the combat mark is erased, the music says how the fight went
  (CombatOutcome) and time passes*/
/*C_07D1_02C4*/COMBAT_breakOff() {
	int16 mouseSave;
	int16 partyIdx, newMode;

	InCombat =
	EnemyWarningGiven = 0;
	if(SoloMember < 0)
		newMode = 1;
	else
		newMode = 0;
	for(partyIdx = 0; partyIdx < PartySize; partyIdx ++) {
		if(IsDead(Party[partyIdx])) {
			NPCMode[Party[partyIdx]] = 0;
		} else if(!IsCharmed(Party[partyIdx])) {
			NPCMode[Party[partyIdx]] = newMode;
			if(newMode == 1)
				Leader[Party[partyIdx]] = partyIdx;
		}
	}
	if(SoloMember < 0)
		partyIdx = 0;
	else
		partyIdx = SoloMember;
	SavedNPCMode = NPCMode[Party[partyIdx]];
	NPCMode[Party[partyIdx]] = 2;
	mouseSave = CON_mouseOffAt(0x8d, 0xa3, 0x9e, 0xc4);
	GR_DRAW_BITMAP(&Screen, 0x93, 0xa9, D_BA7E, 1);
	CON_mouseOn(mouseSave);
	C_1FA9_03DA(CombatOutcome);
	CombatOutcome = 0;
	PassTime(1);
	if(StatusDisplay == 0x92) {
		STAT_refreshComMode();
		STAT_ShowInventory(StatusMember);
	}
}

/*move an object to a random free spot at most 3 tiles away (8 tries);
  with onScreenNotOk the spot must be more than 5 tiles from the party.
  returns 1 if the object was moved*/
/*C_07D1_0416*/COMBAT_TryTeleport(int16 objNum, int16 onScreenNotOk) {
	int16 tries, x, y, z, new_x, new_y;

	if(objNum < 0)
		return 0;
	x = GetX(objNum);
	y = GetY(objNum);
	z = GetZ(objNum);
	for(tries = 0; tries < 8; tries ++) {
		new_x = (OSI_rand(1, 4) + OSI_rand(1, 4) + x - 5) & 0x3ff;
		new_y = (OSI_rand(1, 4) + OSI_rand(1, 4) + y - 5) & 0x3ff;
		if(onScreenNotOk && MapDistance(new_x, MapX) <= 5 && MapDistance(new_y, MapY) <= 5)
			continue;
		if(CanMoveTo(objNum, new_x, new_y)) {
			MoveObj(objNum, new_x, new_y, z);
			return 1;
		}
	}
	return 0;
}

/*effective strength: -3 when cursed, -3 for a member (NPCs 0-15) with
  the first talent, +3 with ObjStatus 0x80; between 1 and 30*/
/*C_07D1_0521*/GetStr(int16 objNum) {
	int16 value = STREN[objNum];
	if(IsCursed(objNum))
		value -= 3;
	if(objNum <= 15 && D_16B0[objNum].a)
		value -= 3;
	if(ObjStatus[objNum] & 0x80)
		value += 3;
	if(value < 1)
		value = 1;
	else if(value > 30)
		value = 30;
	return value;
}

/*effective dexterity: -3 when cursed (minimum 1), +3 with ObjStatus 0x80
  (maximum 30); 1 when asleep*/
/*C_07D1_0576*/GetDex(int16 objNum) {
	int16 value = DEXTE[objNum];
	if(IsCursed(objNum)) {
		if((value -= 3) < 1)
			value = 1;
	}
	if(ObjStatus[objNum] & 0x80) {
		if((value += 3) > 30)
			value = 30;
	}
	if(IsAsleep(objNum))
		value = 1;
	return value;
}

/*effective intelligence: -3 (minimum 1) when cursed or with ObjStatus
  0x80 (but for NPC 1)*/
/*C_07D1_05C0*/GetInt(int16 objNum) {
	int16 value = INTEL[objNum];
	if(IsCursed(objNum) || ((ObjStatus[objNum] & 0x80) && objNum != 1)) {
		if((value -= 3) < 1)
			value = 1;
	}
	return value;
}

/*level from experience: 1 + number of halvings to get 100xp or less*/
/*C_07D1_05F8*/ComputeLevel(int16 objNum) {
	uint16 lvl, xp;

	xp = ExpPoints[objNum];
	lvl = 1;
	while(xp > 99) {
		lvl ++;
		xp >>= 1;
	}
	return lvl;
}

/*add experience points (capped at 9999)*/
/*C_07D1_061B*/AddXP(int16 objNum, int16 xp) {
	if(ExpPoints[objNum] < 9999 - xp)
		ExpPoints[objNum] += xp;
	else
		ExpPoints[objNum] = 9999;
}

/*maximum hit points: 240 for NPC 6; a controlled actor 2*STR + 24*level,
  the others their class's (MonsterHP); at least 1*/
/*C_07D1_064E*/MaxHP(int16 objNum) {
	int16 objCls, hp;

	if(objNum == 6)
		return 0xf0;
	if(IsPlrControl(objNum)) {
		hp = STREN[objNum] * 2 + Level[objNum] * 24;
	} else {
		objCls = GetMonsterClass(GetType(objNum));
		hp = MonsterHP[objCls + 1];
	}
	if(!hp)
		return 1;
	return hp;
}

/*set to "unconscious" type: called when an actor falls asleep or dies.
  Bodies (0x155, 0x151-0x154, 0x9f) stay; 0x172 becomes 0x154; the
  creatures of a class with flag 0x800 or 0x1000 have no body (0x1000:
  C_120E_024B(1) when near the party); 0x156-0x165 fall as 0x155 (asleep
  on a 0x8d: 0x8e), 0x169-0x171 as 0x151, 0x167/0x168 as 0x152, 0x178 as
  0x153, 0x174 as 0x9f, 0x198 as 0xfd; class flag 0x2000 bleeds
  (C_07D1_1483, four squares for 0x175). returns 1 if the actor has a body
  (0: it must disappear)*/
/*C_07D1_06C1*/SetTypeUnconscious(int16 objNum) {
	int16 objCls, isDead, obj, ret;
	int16 objType;

	ret = 1;
	objType = GetType(objNum);
	isDead = IsDead(objNum);
	if(isDead && (
		objType == 0x155 || objType == 0x151 || objType == 0x152 ||
		objType == 0x153 || objType == 0x154 || objType == 0x9f
	)) return 1;
	if(objType == 0x172) {
		SetType(objNum, 0x154);
		SetFrame(objNum, 0);
		return 1;
	}
	if((objCls = GetMonsterClass(objType)) < 0)
		return 0;
	if(!(MonsterFlags[objCls] & 0x800) && !(MonsterFlags[objCls] & 0x1000)) {
		if(objType >= 0x156 && objType <= 0x165) {
			if(!isDead) {
				for(obj = FindLoc(GetX(objNum), GetY(objNum), GetZ(objNum)); obj >= 0; obj = NextLoc()) {
					if(GetType(obj) == 0x8d) {
						SetType(objNum, 0x8e);
						SetFrame(objNum, GetFrame(obj) > 1);
						VisibilityDirty = 1;
						return 1;
					}
				}
			}
			SetType(objNum, 0x155);
			SetFrame(objNum, objType - 0x156);
		} else if(objType >= 0x169 && objType <= 0x171) {
			SetType(objNum, 0x151);
			SetFrame(objNum, objType - 0x169);
		} else if(objType == 0x167 || objType == 0x168) {
			obj = (GetFrame(objNum) >> 1) & 2;
			SetType(objNum, 0x152);
			SetFrame(objNum, (objType - 0x167) << 2 | obj | 1);
		} else if(objType == 0x178) {
			SetType(objNum, 0x153);
			SetFrame(objNum, 3);
		} else if(objType == 0x174) {
			SetType(objNum, 0x9f);
			SetFrame(objNum, GetFrame(objNum) >> 1);
		} else if(objType == 0x198) {
			SetType(objNum, 0xfd);
			SetFrame(objNum, 0);
		}
	}
	if(isDead) {
		if(MonsterFlags[objCls] & 0x2000) {
			C_07D1_1483(objCls, GetX(objNum), GetY(objNum), 1);
			if(objType == 0x175) {
				C_07D1_1483(objCls, GetX(objNum) - 1, GetY(objNum), 1);
				C_07D1_1483(objCls, GetX(objNum), GetY(objNum) - 1, 1);
				C_07D1_1483(objCls, GetX(objNum) - 1, GetY(objNum) - 1, 1);
			}
		}
		if((MonsterFlags[objCls] & 0x800) || (MonsterFlags[objCls] & 0x1000)) {
			ret = 0;
			if((MonsterFlags[objCls] & 0x1000) && MapDistance(GetX(objNum), MapX) <= 5 && MapDistance(GetY(objNum), MapY) <= 5)
				C_120E_024B(1);
		}
	}
	VisibilityDirty = 1;
	return ret;
}

/*can viewerObj see (and so target) targetObj? not if it is invisible
  (unless both are party members), dragged under, an "ignore" tile or on
  another level*/
/*C_07D1_0B2A*/COMBAT_canSee(int16 viewerObj, int16 targetObj) {
	int16 isVisible;

	isVisible = !IsInvisible(targetObj);

	if(
		IsPlrControl(viewerObj) &&
		!isVisible && IsNPC(targetObj) && IsPlrControl(targetObj)
	) isVisible = 1;

	if(IsNPC(targetObj) && IsDraggedUnder(targetObj))
		isVisible = 0;

	if(IsTileIg(TILE_FRAME(targetObj)))
		isVisible = 0;

	if(GetZ(viewerObj) != GetZ(targetObj))
		isVisible = 0;

	return isVisible;
}

/*pick the object targeted at (x,y) by viewerObj: the first visible
  living actor (a part 0x172/0x173 of a creature gives the creature, its
  number in the part's quality), else the first visible object, else the
  first object; -1 if the tile is empty*/
/*C_07D1_0BE1*/COMBAT_GetTargetAt(int16 viewerObj, int16 x, int16 y) {
	int16 firstObj, objType;
	int16 obj, firstSeen;

	firstObj = -1;
	firstSeen = -1;
	for(obj = FindLoc(x, y, MapZ); obj >= 0; obj = NextLoc()) {
		if(firstObj < 0)
			firstObj = obj;
		if(IsTileIg(TILE_FRAME(obj)))
			continue;
		if(COMBAT_canSee(viewerObj, obj)) {
			if(firstSeen < 0)
				firstSeen = obj;
			objType = GetType(obj);
			if((objType == 0x173 || objType == 0x172) && GetQual(obj))
				obj = GetQual(obj);
			if(IsNPC(obj) && ObjShapeType[obj] && !IsDead(obj))
				return obj;
		}
	}
	if(firstSeen >= 0)
		return firstSeen;
	return firstObj;
}

/*total protection of an actor: its equipped armours (C_100B_00C4)*/
/*C_07D1_0CC2*/GetProtection(int16 objNum) {
	int16 protection, invObj;

	protection = 0;
	for(invObj = FindInv(objNum); invObj >= 0; invObj = NextInv()) {
		if(GetCoordUse(invObj) == 0x18)
			protection += C_100B_00C4(invObj);
	}
	return protection;
}

/*fire one missile (tile numTile, 0: invisible) from objNum toward
  *pX_dst,*pY_dst; on return they hold where it stopped*/
/*C_07D1_0CFC*/COMBAT_Missile(int16 objNum, int16 *pX_dst, int16 *pY_dst, int16 max_range, int16 numTile) {
	register int16 ret;

	MissileShooter = objNum;
	ret = Missiles(GetX(objNum), GetY(objNum), 1, pX_dst, pY_dst, max_range, numTile, 0);
	MissileShooter = -1;
}

/*a spraying weapon of type weapType fired by attacker at (x, y)
  (C_0E1A_1003: 12 missiles): every object hit is hurt
  (C_07D1_274C(attacker, obj, SprayDamages, SprayKinds)), actors may dodge
  (OSI_rand(1, 60) against their dexterity); 0xf1 freezes the water
  (0xa0 frame 1 to frame 2) and paralyzes the actors of class flag 0x4000
  or 0x80 (0x80 alone also hurts, 0x4000 shows the hit)*/
/*C_07D1_0D54*/C_07D1_0D54(int16 attacker, int16 x, int16 y, int16 weapType) {
	int16 far *hits;
	int16 objCls, i;

	for(i = 0; i < 4; i ++) {
		if(SprayTypes[i] == weapType) {
			MUS_PlaySfx(SpraySfx[i]);
			break;
		}
	}
	EndPagingAtPrompt = 1;
	for(hits = C_0E1A_1003(GetX(attacker), GetY(attacker), x, y, SprayTiles[i]); *hits >= 0; hits ++) {
		if(weapType == 0xf1) {
			if(ObjShapeType[*hits] == 0x4a0) {
				SetFrame(*hits, 2);
				CON_printf(C_2BB4_20D6);
			} else if(IsNPC(*hits)) {
				if(OSI_rand(1, 45) > GetDex(*hits)) {
					if((objCls = GetMonsterClass(GetType(*hits))) >= 0 && ((MonsterFlags[objCls] & 0x4000) || (MonsterFlags[objCls] & 0x80))) {
						NPCStatus[*hits] |= 2;
						CON_printf(C_2BB4_13B1, GetObjectString(*hits));
						if((MonsterFlags[objCls] & 0x80) && !(MonsterFlags[objCls] & 0x4000))
							C_07D1_274C(attacker, *hits, SprayDamages[i], 1);
						else
							C_07D1_1378(*hits);
					} else {
						CON_printf(C_2BB4_02A4);
					}
				} else {
					CON_printf("%s dodges!\n", GetObjectString(*hits));
					MUS_PlaySfx(3);
				}
			}
		} else {
			if(!IsNPC(*hits) || OSI_rand(1, 60) > GetDex(*hits)) {
				C_07D1_274C(attacker, *hits, SprayDamages[i], SprayKinds[i]);
			} else {
				CON_printf("%s dodges!\n", GetObjectString(*hits));
				MUS_PlaySfx(3);
			}
		}
	}
	EndPagingAtPrompt = 0;
	VIS_UpdateView();
	DisplayTick();
}

/*fire a ranged weapon (or throw an object) from att_objNum toward
  *pX_dest,*pY_dest: the missile, sound and ammunition come from
  C_100B_00FF (ammo >= 0: one object of that type is used; -2: the
  weapon's own quantity; -3: nothing); without ammunition an object
  thrown further than 1 square lands where the missile stopped (gone in
  the water) and the next one of its type is readied in its hand. returns 1*/
/*C_07D1_0FCC*/static COMBAT_FireRangedWeapon(int16 att_objNum, int16 *pX_dest, int16 *pY_dest, int16 weap_objNum) {
	int16 typ, itemObj, invChanged, range, missileTile, dest_x, dest_y;
	int16 sfx, ammo, thrown, slot;

	typ = GetType(weap_objNum);
	range = GetWeaponRange(weap_objNum);
	C_100B_00FF(typ, &missileTile, &sfx, &ammo);
	if(sfx >= 0)
		MUS_PlaySfx(sfx);
	dest_x = *pX_dest;
	dest_y = *pY_dest;
	thrown = C_07D1_35F5(dest_x, dest_y, att_objNum) > 1;
	if(missileTile == 0 && att_objNum != weap_objNum)
		missileTile = TILE_FRAME(weap_objNum);
	COMBAT_Missile(att_objNum, pX_dest, pY_dest, range, missileTile);
	invChanged = 0;
	if(ammo >= 0) {
		TakeObj(att_objNum, ammo, 1);
		invChanged = 1;
	} else if(ammo == -2) {
		SubQuan(weap_objNum, 1);
	} else if(ammo != -3 && thrown) {
		if(Equipment[2] == weap_objNum)
			slot = 2;
		else if(Equipment[5] == weap_objNum)
			slot = 5;
		else if(Equipment[8] == weap_objNum)
			slot = 8;
		else
			slot = -1;
		if(C_047A_13C5(MissileEnd[0], MissileEnd[1]))
			DeleteObj(weap_objNum);
		else
			MoveObj(weap_objNum, MissileEnd[0], MissileEnd[1], MapZ);
		if(slot != -1 && (itemObj = FindInvOfType(att_objNum, typ)) >= 0) {
			if(QuanType(typ) && GetQuan(itemObj) > 1)
				SubQuan(itemObj, 1);
			else
				DeleteObj(itemObj);
			Equipment[slot] = AddInvObj(typ, 0x18, 1, att_objNum);
			C_120E_024B(0);
		}
	}
	if(invChanged && StatusDisplay == 0x92 && Party[StatusMember] == att_objNum)
		StatusDirty = 1;
	return 1;
}

/*copy an actor's attributes to another actor (its movement points
  cleared)*/
/*C_07D1_11BB*/CloneObject(int16 src_objNum, int16 dst_objNum) {
	ObjStatus[dst_objNum] = ObjStatus[src_objNum];
	ObjShapeType[dst_objNum] = ObjShapeType[src_objNum];
	Amount[dst_objNum] = Amount[src_objNum];
	NPCStatus[dst_objNum] = NPCStatus[src_objNum];
	STREN[dst_objNum] = STREN[src_objNum];
	DEXTE[dst_objNum] = DEXTE[src_objNum];
	INTEL[dst_objNum] = INTEL[src_objNum];
	MovePts[dst_objNum] = 0;
	ExpPoints[dst_objNum] = ExpPoints[src_objNum];
	HitPoints[dst_objNum] = HitPoints[src_objNum];
	NPCMode[dst_objNum] = NPCMode[src_objNum];
	NPCComMode[dst_objNum] = NPCComMode[src_objNum];
	SchedIndex[dst_objNum] = SchedIndex[src_objNum];
	Leader[dst_objNum] = Leader[src_objNum];
	NPCFlag[dst_objNum] = NPCFlag[src_objNum];
	Level[dst_objNum] = Level[src_objNum];
}

/*flash the "hit" tile 0x101 over an object (if on screen and ShowHitFlash
  set) and wake it up if it was asleep*/
/*C_07D1_126D*/COMBAT_showHit(int16 objNum) {
	if(ShowHitFlash) {
		int16 x = (GetX(objNum) - MapX + 5) & 0x3ff;
		int16 y = (GetY(objNum) - MapY + 5) & 0x3ff;
		if(x >= 0 && x < 11 && y >= 0 && y < 11) {
			/*display hit*/
			AnimateTiles();
			if(VisibilityDirty)
				VIS_UpdateView();
			ShowObjects();
			ShowObject(0x101, x, y, 1);
			DrawMapWindow(1);
			MUS_PlaySfx(IsPlrControl(objNum)?1:2);
			VisibilityDirty = 1;
		}
		if(IsNPC(objNum))
			SetHurtThisTurn(objNum);
	}
	if(IsNPC(objNum) && IsAsleep(objNum)) {
		ClrAsleep(objNum);
		ObjShapeType[objNum] = OrigShapeType[objNum];
		VisibilityDirty = 1;
	}
	ShowHitFlash = 1;
}

/*the same with the tile 0x102 (a paralyzing hit)*/
/*C_07D1_1378*/C_07D1_1378(int16 objNum) {
	if(ShowHitFlash) {
		int16 x = (GetX(objNum) - MapX + 5) & 0x3ff;
		int16 y = (GetY(objNum) - MapY + 5) & 0x3ff;
		if(x >= 0 && x < 11 && y >= 0 && y < 11) {
			/*display hit*/
			AnimateTiles();
			if(VisibilityDirty)
				VIS_UpdateView();
			ShowObjects();
			ShowObject(0x102, x, y, 1);
			DrawMapWindow(1);
			MUS_PlaySfx(IsPlrControl(objNum)?1:2);
			VisibilityDirty = 1;
		}
		if(IsNPC(objNum))
			SetHurtThisTurn(objNum);
	}
	if(IsNPC(objNum) && IsAsleep(objNum)) {
		ClrAsleep(objNum);
		ObjShapeType[objNum] = OrigShapeType[objNum];
		VisibilityDirty = 1;
	}
	ShowHitFlash = 1;
}

/*a creature of class objCls (with flag 0x2000) bleeds at (x, y): blood
  0x14f (flags 0x80 and 0x4000), 0x150 (0x80) or 0x14e (0x4000), with a
  random frame; a blood object already there (0x14e-0x150) is reused (moved
  to the end of the square, or with move set by MoveObj); none on the
  tiles of Is_6C_6D_6E_4D_4F or with an object 0x19c-0x19f/0x1a3 there*/
/*C_07D1_1483*/C_07D1_1483(int16 objCls, int16 x, int16 y, int16 move) {
	int16 obj;
	int16 tile, blood;

	blood = -1;
	if(MonsterFlags[objCls] & 0x2000) {
		if(Is_6C_6D_6E_4D_4F(GetTileAtXYZ(x, y, MapZ)))
			return;
		for(obj = FindLoc(x, y, MapZ); obj >= 0; obj = NextLoc()) {
			switch(GetType(obj)) {
				case 0x14e: case 0x14f: case 0x150:
					blood = obj;
				break;
				case 0x19c: case 0x19d: case 0x19e: case 0x19f: case 0x1a3:
					return;
			}
		}
		if(MonsterFlags[objCls] & 0x80) {
			if(MonsterFlags[objCls] & 0x4000)
				tile = 0x14f;
			else
				tile = 0x150;
		} else if(MonsterFlags[objCls] & 0x4000) {
			tile = 0x14e;
		} else {
			return;
		}
		if(blood < 0)
			blood = AddMapObj(tile, 0x20, 0, x, y, MapZ);
		if(blood >= 0) {
			if(move)
				MoveObj(blood, x, y, MapZ);
			else
				C_1B09_268D(blood);
			SetType(blood, tile);
			SetFrame(blood, OSI_rand(0, 2));
		}
	}
}

/*an actor dies. NPC 0 (the player) only falls (his inventory dropped
  but on MapZ 3). Its position is kept in KillX/KillY; the creature
  0x175 takes its parts (0x16a/0xfe holding its number) along, and on
  MapZ 3 its death opens the way (0x1d2 at (0x87, 0x17) removed, a
  0xce0 at (0x7f, 0x18)) and calls conversation 0x66; the parts
  0x172/0x174 die together. Killing some NPCs makes the game
  unwinnable (IsArmageddon); in the dream the last creatures to kill end
  stages 0x20, 0xa1, 0xc0. Party members leave the party; the body
  (KillBody) is made by OBJ_MakeDeadBody, or none for the actors without
  one (moved to MapZ 6). returns the experience gained by killing a
  hostile monster*/
/*C_07D1_15F0*/COMBAT_Kill(int16 objNum) {
	int16 objCls, xp, noShape, objType;
	int16 i;

	xp = 0;
	if(!IsNPC(objNum))
		return 0;
	if(objNum == 0) {
		if(MapZ != 3)
			OBJ_DeleteContents(0);
		HitPoints[objNum] = 0;
		SetDead(objNum);
		CheckActiveMember();
		return 0;
	}
	KillX = GetX(objNum);
	KillY = GetY(objNum);
	objType = GetType(objNum);
	if(LastAttackTarget == objNum)
		LastAttackTarget = -1;
	if(StatusDisplay != 0x91 && Party[StatusMember] == objNum) {
		SelectMode = 0;
		StatusDisplay = 0x91;
		StatusMember = 0;
		STAT_ShowParty(0);
	}
	if(InCombat) {
		if(IsPlrControl(objNum))
			CombatOutcome = -1;
		else if((NPCStatus[objNum] & 0x20) && objType != 0x16a && CombatOutcome <= 0)
			CombatOutcome = 1;
	}
	if(objType == 0x175) {
		objCls = GetMonsterClass(0x16a);
		for(i = 0xc0; i < 0x100; i ++) {
			if((GetType(i) == 0x16a || GetType(i) == 0xfe) && !IsDead(i) && GetQual(i) == objNum) {
				if(LastAttackTarget == i)
					LastAttackTarget = -1;
				HitPoints[objNum] = 0;
				SetDead(objNum);
				xp += ((MonsterFlags[objCls] & 0x20)?MonsterStr[objCls + 1]:MonsterDex[objCls + 1]) * MonsterDamage[objCls + 1];
				xp += (MonsterHP[objCls + 1] + MonsterArmor[objCls + 1]) * MonsterDex[objCls + 1];
				NPCMode[i] = 0;
				ObjShapeType[i] = 0;
				MoveObj(i, GetX(i), GetY(i), 6);
			}
		}
		if(MapZ == 3) {
			if((i = OBJ_FindTypeAt(0x1d2, 0x87, 0x17)) < 0)
				CON_printf(C_2BB4_22F1);
			else
				DeleteObj(i);
			if((i = OBJ_FindTypeAt(0xe0, 0x7f, 0x18)) < 0) {
				if((i = AddMapObj(0xce0, 0, 0, 0x7f, 0x18, MapZ)) < 0)
					CON_printf(C_2BB4_22F1);
				else
					ObjStatus[i] &= ~0x20;
			}
			TalkFlags[0x66] |= 0x22;
			TALK_talkTo(Active, 0x66, 0);
			HitPoints[objNum] = 0;
			SetDead(objNum);
			SetTypeUnconscious(objNum);
			NPCMode[objNum] = 0;
			KillBody = OBJ_MakeDeadBody(objNum);
			VisibilityDirty = 1;
			return 0;
		}
	} else if(objType == 0x172) {
		if((i = GetQual(objNum)) && !IsDead(i) && GetType(i) == 0x174)
			return COMBAT_Kill(i);
		else
			SetQual(objNum, 0);
	} else if(objType == 0x174) {
		if((i = GetQual(objNum)) && !IsDead(i) && GetType(i) == 0x172) {
			if(LastAttackTarget == i)
				LastAttackTarget = -1;
			HitPoints[i] = 0;
			SetDead(i);
			NPCMode[i] = 0;
			OBJ_MakeDeadBody(i);
			ObjShapeType[i] = 0;
			MoveObj(i, GetX(i), GetY(i), 6);
			SetQual(i, 0);
			SetQual(objNum, 0);
		} else {
			SetQual(objNum, 0);
		}
	}
	if(HitPoints[objNum] >= 0) {
		HitPoints[objNum] = 0;
		SetDead(objNum);
		if(objNum == 0x3b || objNum == 0x36) {
			TalkFlags[0x3b] |= 2;
			IsArmageddon = 1;
			for(i = 0x33; i <= 0x3e; i ++) {
				NPCStatus[i] |= 0x20;
				NPCMode[i] = 8;
			}
		} else if(objNum == 0x12 || objNum == 0x1a) {
			IsArmageddon = 1;
			CON_printf(C_2BB4_174D);
		} else if(objNum == 0x20) {
			IsArmageddon = 1;
		} else if(
			(objNum >= 0x10 && objNum < 0x1d) || (objNum >= 0xa && objNum <= 0xf) ||
			(objNum >= 0x22 && objNum <= 0x34) || (objNum >= 0x37 && objNum <= 0x3a) ||
			(objNum >= 0x3d && objNum <= 0x3e) || objNum == 0x42 || objNum == 0x46
		) {
			IsArmageddon = 2;
		}
		if(objNum == 0x5b)
			TalkFlags[0x5b] |= 0x10;
		objCls = GetMonsterClass(objType);
		if(NPCStatus[objNum] & 0x20) {
			if(objCls >= 0) {
				if(MonsterFlags[objCls] & 0x20)
					xp += MonsterStr[objCls + 1] * MonsterDamage[objCls + 1];
				else
					xp += MonsterDex[objCls + 1] * MonsterDamage[objCls + 1];
				xp += (MonsterHP[objCls + 1] + MonsterArmor[objCls + 1]) * MonsterDex[objCls + 1];
			}
			if((D_16AA & 0x10) && DreamStage && D_16A3 && objType != 0x16a && objType != 0x180 && --D_16A3 == 0) {
				if(DreamStage == 0x20) {
					SetTypeUnconscious(objNum);
					NPCMode[objNum] = 0;
					KillBody = OBJ_MakeDeadBody(objNum);
					C_2F63_0086();
					return 0;
				} else if(DreamStage == 0xa1) {
					TalkFlags[0x52] |= 2;
				} else if(DreamStage == 0xc0) {
					C_2F63_062F(objType);
					return 0;
				}
			}
		}
		ClrInvisible(objNum);
		if(objType != 0x172 && !IsInConversation) {
			if(objNum >= 0x10) {
				if(!(MonsterFlags[objCls] & 0x4000) && !(MonsterFlags[objCls] & 0x80))
					CON_printf(C_2BB4_0052, GetObjectString(objNum));
				else
					CON_printf(C_2BB4_0045, GetObjectString(objNum));
			} else {
				CON_printf(C_2BB4_0062, GetObjectString(objNum));
			}
		}
		noShape = !SetTypeUnconscious(objNum);
		NPCMode[objNum] = 0;
		if(IsPlrControl(objNum) && Party[0] != objNum) {
			for(i = 1; i <= PartySize; i ++) {
				if(Party[i] == objNum) {
					if(SoloMember >= i)
						SoloMember --;
					if(Active == i)
						SetPartyMode();
					if(Active > i)
						Active --;
					for(; i < PartySize; i ++) {
						Party[i] = Party[i + 1];
						strcpy(Names[i], Names[i + 1]);
					}
					PartySize --;
					StatusDirty = 1;
					break;
				}
			}
			STAT_ShowParty(0);
			StatusDisplay = 0x91;
		}
		if(!noShape) {
			KillBody = OBJ_MakeDeadBody(objNum);
		} else {
			KillBody = -1;
			MoveObj(objNum, KillX, KillY, 6);
			if(objNum >= 0xc0)
				ObjShapeType[objNum] = 0;
		}
		if(MonsterFlags[objCls] & 0x800)
			C_07D1_5F2B(KillX, KillY);
		VisibilityDirty = 1;
	}
	if(Party[Active] == objNum)
		CheckActiveMember();
	return xp >> 4;
}

/*inflict damage to an object; kind 0 a normal hit (actors may bleed),
  1 a paralyzing one (C_07D1_1378: freezes the water 0xa0), 3 a burning
  one (thaws 0xa0; class flag 8 resists it). Actors: the invulnerable
  ones keep 255 hit points (some talk back, 0x187 hits the player three
  times, 0x6d/0x67 send the party away, 0x40 calls C_07D1_632C), others
  die (COMBAT_Kill) or lose hit points (the class flag 0x40 divides, the
  dreamer survives stage 0xe0 with 1 point). Breakable items
  (C_100B_038E) lose "quantity" points and are destroyed at 0, some with
  effects (0x133 the pod, 0x184, 0xd9 in dream stage 0x44). returns the
  experience gained by killing a hostile monster*/
/*C_07D1_1D64*/LoseHP(int16 objNum, int16 damage, int16 kind) {
	int16 i, x, y, newObj, objCls, xp, savedTextOn;
	int16 objType;

	if(GetZ(objNum) != MapZ)
		return 0;
	if(damage < 0)
		return 0;
	xp = 0;
	x = GetX(objNum);
	y = GetY(objNum);
	objType = GetType(objNum);
	savedTextOn = TextOn;
	TextOn = 1;
	if(objNum == 0 && DreamStage == 0xe0 && HitPoints[0] <= damage)
		damage = HitPoints[0] - 1;
	if(IsNPC(objNum)) {
		objCls = GetMonsterClass(objType);
		if(kind == 1)
			C_07D1_1378(objNum);
		else
			COMBAT_showHit(objNum);
		if(IsInvulnerable(objNum)) {
			if(objNum == 0x69) {
				CON_printf("`%s ", GetObjectString(0x19));
				CON_printf(C_2BB4_0000);
				PromptFlag = 1;
			} else if(objNum == 0x3c) {
				CON_printf("`%s ", GetObjectString(objNum));
				CON_printf(C_2BB4_0000);
				PromptFlag = 1;
			} else if(objNum == 0x52) {
				CON_printf("`%s ", GetObjectString(0x52));
				CON_printf(C_2BB4_000B);
				CON_printf(C_2BB4_0022);
			} else if(objNum == 0x68) {
				CON_printf("`%s ", GetObjectString(objNum));
				if(TalkFlags[0x68] & 4) {
					CON_printf(C_2BB4_0000);
				} else {
					CON_printf(C_2BB4_000B);
					CON_printf(C_2BB4_0022);
				}
				TalkFlags[0x68] |= 0x20;
				TALK_talkTo(Active, 0x68, 0);
			} else {
				CON_printf(C_2BB4_02A4);
			}
			HitPoints[objNum] = 0xff;
			if(GetType(objNum) == 0x187) {
				CON_printf("How dare you, young %sman!\n", D_16A8?"wo":"");
				C_07D1_274C(objNum, 0, 0x2d, 0);
				C_07D1_274C(objNum, 0, 0x2d, 0);
				C_07D1_274C(objNum, 0, 0x2d, 0);
			} else if(objNum == 0x6d || objNum == 0x67) {
				TalkFlags[0x6b] |= 8;
				PartyTeleport(D_1684, D_1685, MapZ, 1);
			} else if(objNum == 0x40) {
				C_07D1_632C();
				return 0;
			}
			return 0;
		}
		if(kind == 3) {
			if(MonsterFlags[objCls] & 8) {
				CON_printf(C_2BB4_02A4);
				return 0;
			}
			if(MonsterFlags[objCls] & 0x10)
				damage << 1;/*(sic)*/
		}
		if(kind == 0 && (objNum != 0 || DreamStage != 0xe0) && OSI_rand(6, 100) <= damage)
			C_07D1_1483(objCls, x, y, 0);
		if(HitPoints[objNum] <= damage) {
			xp = COMBAT_Kill(objNum);
		} else {
			HitPoints[objNum] -= damage;
			switch(NPCMode[objNum]) {
				case 9: case 11:
					NPCMode[objNum] = 8;
				break;
				case 10: case 12:
					NPCMode[objNum] = 0x1b;
				break;
			}
			if(MonsterFlags[GetMonsterClass(objType)] & 0x40) {
				for(i = 0; i < 8; i ++) {
					x = GetX(objNum) + OSI_rand(0, 2) - 1;
					y = GetY(objNum) + OSI_rand(0, 2) - 1;
					if(CanMoveTo(objNum, x, y)) {
						if((newObj = AddMonster(objType, x, y, MapZ)) >= 0) {
							CloneObject(objNum, newObj);
							CON_printf("%s divides!\n", GetObjectString(objNum));
							PromptFlag = 1;
							VisibilityDirty = 1;
						}
						break;
					}
				}
			}
		}
		if(IsPlrControl(objNum)) {
			if((D_16AA & 0x10) || (StatusDisplay == 0x90 && Party[StatusMember] == objNum))
				StatusDirty = 1;
			else if(StatusDisplay == 0x91)
				STAT_ShowParty(PartyListTop);
		}
	} else if(GetQuan(objNum) && C_100B_038E(objType)) {
		if(kind == 0 || kind == 3 || C_100B_0410(objType)) {
			if(kind == 1)
				C_07D1_1378(objNum);
			else
				COMBAT_showHit(objNum);
			if(kind == 3 && C_100B_0247(objType))
				damage = GetQuan(objNum);
			if(GetQuan(objNum) > damage) {
				if(objType == 0x133) {
					CON_printf("The pod splits open!\n");
					EGG_generate(0x180, GetX(objNum), GetY(objNum), MapZ);
					DeleteObj(objNum);
					VisibilityDirty = 1;
					return 0;
				}
				if(objType == 0x184) {
					SetFrame(objNum, 0);
					SetQual(objNum, OSI_rand(0xb, 0x12));
					MUS_PlaySfx(0x22);
				}
				SubQuan(objNum, damage);
			} else {
				PromptFlag = 1;
				if(objType == 0xd9) {
					MUS_PlaySfx(0x1a);
					ObjShapeType[objNum] = 0xda;
					if((D_16AA & 0x10) && DreamStage == 0x44) {
						for(i = SearchArea(0x37, 0x37, 0x47, 0x47); i >= 0; i = NextArea())
							if(GetType(i) == 0xd9)
								break;
						if(i < 0)
							TalkFlags[0x54] |= 0x40;
					}
				} else {
					MUS_PlaySfx(0);
					DeleteObj(objNum);
					VisibilityDirty = 1;
				}
			}
		}
	} else if(GetType(objNum) == 0xa0) {
		if(kind == 3 && GetFrame(objNum) == 2) {
			CON_printf(C_2BB4_20C6);
			SetFrame(objNum, 1);
			VisibilityDirty = 1;
		} else if(kind == 1 && GetFrame(objNum) == 1) {
			CON_printf(C_2BB4_20D6);
			SetFrame(objNum, 2);
			VisibilityDirty = 1;
		}
	}
	TextOn = savedTextOn;
	return xp;
}

/*does the attack hit? always against an object; dexterity (or, for a
  creature attacking with its own body and class flag 0x20, strength) of
  the attacker against the defender's dexterity*/
/*C_07D1_23D5*/static __AttackTest(int16 objNum_att, int16 objNum_def, int16 objNum_weapon) {
	int16 objTyp, defDex, objCls, defTyp;
	int16 attackStat, strOverDex;

	if(!IsNPC(objNum_def))
		return 1;
	objTyp = GetType(objNum_weapon);
	defTyp = GetType(objNum_def);
	if(objNum_weapon == objNum_att) {
		objCls = GetMonsterClass(objTyp);
		strOverDex = MonsterFlags[objCls] & 0x20;
	} else {
		strOverDex = 0;
	}
	if(strOverDex)
		attackStat = GetStr(objNum_att);
	else
		attackStat = GetDex(objNum_att);
	defDex = GetDex(objNum_def);

	return OSI_rand(1, 30) >= (defDex + 30 - attackStat) / 2;
}

/*print how badly an actor is wounded (in quarters of its max HP;
  "damaged" for the creatures of class objCls without flag 0x4000 or
  0x80); a critically wounded NPC: 0x175 ends the combat well, mode 0xd
  (and its part in its quality) becomes 0x11, an intelligent one in a
  mobile combat mode (and of class 0 or a living one) flees (7)*/
/*C_07D1_2489*/COMBAT_displaySeverity(int16 objNum, int16 objCls) {
	int16 objType, severity;

	if((objType = GetType(objNum)) == 0x172)
		return;
	if(HitPoints[objNum] == 0)
		return;
	severity = (HitPoints[objNum] << 2) / MaxHP(objNum);
	if(severity < 4) {
		CON_printf("`%s ", GetObjectString(objNum));
		PromptFlag = 1;
	}
	if(severity == 0) {/*critical*/
		CON_printf(C_2BB4_0000);
		if(InCombat && objType == 0x175)
			CombatOutcome = 1;
		if(NPCMode[objNum] == 0xd) {
			NPCMode[objNum] = 0x11;
			if(GetQual(objNum))
				NPCMode[GetQual(objNum)] = 0x11;
		} else if(
			NPCMode[objNum] != 6 &&
			NPCMode[objNum] != 0xe &&
			NPCMode[objNum] != 2 &&
			NPCMode[objNum] != 1 &&
			NPCMode[objNum] != 0 &&
			NPCMode[objNum] != 0x11 &&
			NPCMode[objNum] != 0x19 &&
			(objCls == 0 || (MonsterFlags[objCls] & 0x80) || (MonsterFlags[objCls] & 0x4000)) &&
			GetInt(objNum) >= 5 &&
			objType != 0x91
		) NPCMode[objNum] = 7;
	} else if(severity == 1) {/*heavily*/
		CON_printf(C_2BB4_000B);
	} else if(severity == 2) {/*lightly*/
		CON_printf(C_2BB4_0013);
	} else if(severity == 3) {/*barely*/
		CON_printf(C_2BB4_001B);
	}
	if(severity != 0 && severity < 4) {
		if(!(MonsterFlags[objCls] & 0x4000) && !(MonsterFlags[objCls] & 0x80))
			CON_printf(C_2BB4_002D);
		else
			CON_printf(C_2BB4_0022);
	}
}

/*a neutral actor (not in the party, in a mode up to 0x10 or a scheduled
  one) is attacked: "<name> attacks!" when the player hits one of a
  fighting class (0x4000 without 0x80), a scheduled NPC hit by the player
  gets mode 0xA0, else it turns hostile (if attacked by a good one);
  temporary monsters get mode 8*/
/*C_07D1_2643*/COMBAT_ProvokeNeutral(int16 victim, int16 attacker, int16 damage) {
	int16 objCls;

	if(!IsNPC(victim))
		return;
	if(GetAlignment(victim))
		return;
	if(!IsPlrControl(victim) && (NPCMode[victim] <= 0x10 || NPCMode[victim] >= 0x80)) {
		objCls = GetMonsterClass(GetType(victim));
		if(NPCMode[attacker] == 2 && objCls >= 0 && (MonsterFlags[objCls] & 0x4000) && !(MonsterFlags[objCls] & 0x80) && damage > 0) {
			CON_printf(C_2BB4_1393, GetObjectString(victim));
			PromptFlag = 1;
		}
		if(victim < 0xc0 && NPCMode[victim] >= 0x80) {
			if(NPCMode[attacker] == 2)
				NPCMode[victim] = 0xa0;
			return;
		}
		if(GetAlignment(attacker) == 0x40 && !Is_ATKPLR(victim))
			SetAlignment(victim, 0x20);
		if(victim >= 0xc0)
			NPCMode[victim] = 8;
	}
}

/*a hit of kind (LoseHP's) landed: roll the damage (1..hitPts; 255 is a
  sure kill), subtract 1..armour (none for kind 2; 2 for an object 0x184
  of frame 0), apply it, print the messages; a creature of class flag 4
  paralyzes, 0x17b calls C_07D1_5DB2 when its victim left a body*/
/*C_07D1_274C*/static __DoHit(int16 attacker, int16 defender, int16 hitPts, int16 kind) {
	char *pObjStr;
	int16 armorPts, defTyp, attCls, defCls;

	if(defender < 0)
		return;
	defTyp = GetType(defender);
	defCls = GetMonsterClass(defTyp);
	pObjStr = GetObjectString(defender);
	attCls = GetMonsterClass(GetType(attacker));

	if(hitPts == -1)
		hitPts = 1;
	else if(hitPts > 1 && hitPts < 255)
		hitPts = OSI_rand(1, hitPts);

	if(kind == 2)
		armorPts = 0;
	else if(IsNPC(defender))
		armorPts = GetProtection(defender);
	else if(defTyp == 0x184 && GetFrame(defender) == 0)
		armorPts = 2;
	else
		armorPts = 0;
	if(armorPts > 0 && hitPts < 255)
		hitPts -= OSI_rand(1, armorPts);

	if(hitPts <= 0) {
		if(kind == 1)
			C_07D1_1378(defender);
		else
			COMBAT_showHit(defender);
		CON_printf(C_2BB4_0038, pObjStr);
		PromptFlag = 1;
	} else {
		if(Is_ATKPLR(attacker) && IsNPC(defender) && IsPlrControl(defender) && !InCombat && SoloMember == -1)
			C_07D1_5CAF();
		AddXP(attacker, LoseHP(defender, hitPts, kind));
	}
	if(!IsNPC(defender))
		return;

	if(defender == SelectionObj)
		COMBAT_ProvokeNeutral(defender, attacker, hitPts);
	if(HitPoints[defender] == 0) {
		PromptFlag = 1;
		if(StatusDisplay == 0x90 && Party[StatusMember] == attacker)
			StatusDirty = 1;
		if(GetType(attacker) == 0x17b && KillBody >= 0)
			C_07D1_5DB2(attacker);
		return;
	}
	if(IsNPC(defender) && (MonsterFlags[attCls] & 4) && OSI_rand(0, 3) == 0 && !IsInvulnerable(defender)) {
		CON_printf(C_2BB4_13B1, pObjStr);
		PromptFlag = 1;
		NPCStatus[defender] |= 2;
		if(
			StatusDisplay == 0x91 ||
			(StatusDisplay == 0x90 && Party[StatusMember] == defender)
		) StatusDirty = 1;
	}
	if(!IsDead(defender) && hitPts > 0)
		COMBAT_displaySeverity(defender, defCls);
}

/*return the actor owning a part 0x172/0x173 of a creature (its number
  in the part's quality, when alive); the object itself otherwise*/
/*C_07D1_298A*/COMBAT_getHead(int16 objNum) {
	int16 objType;
	int16 head;

	if(objNum < 0)
		return -1;
	objType = GetType(objNum);
	if((objType == 0x173 || objType == 0x172) && (head = GetQual(objNum)) && ObjShapeType[head] && !IsDead(head))
		objNum = head;
	return objNum;
}

/*attack (x,y) with a weapon: range, ammunition (0x2f needs 0x3f, 0x29/0x2a
  0x39, 0x2e 0x3c, 0x2c 0x3b, 0x2b 0x3a; 0x81/0x105 their quantity), hit
  test, missile, damage; the sprayers 0x2b, 0x2d, 0xf0, 0xf1 also (or
  only) spray (C_07D1_0D54, quality 0: both, 1: hit, 2+: spray; four
  charges or one 0x3a), 0xf1 freezes and paralyzes, 0x81/0x105 with a
  quality make invisible actors visible (0x91 becomes 0x16c), 0x28 charms.
  returns 0: done, 1: no ammunition, 2: out of range*/
/*C_07D1_29E2*/static __TryHit(int16 att_objNum, int16 x, int16 y, int16 weap_objNum) {
	int16 damage, didHit, isRanged, tmp, range, objCls, kind, doHit, doSpray;
	int16 objTyp, opp_objNum;

	kind = 0;
	objTyp = GetType(weap_objNum);
	if((range = C_07D1_35F5(x, y, att_objNum)) > GetWeaponRange(weap_objNum))
		return 2;
	/*ammunition*/
	if(objTyp == 0x2f && FindInvType(att_objNum, 0x3f, -1) < 0)
		return 1;
	if((objTyp == 0x29 || objTyp == 0x2a) && FindInvType(att_objNum, 0x39, -1) < 0)
		return 1;
	if(objTyp == 0x2e && FindInvType(att_objNum, 0x3c, -1) < 0)
		return 1;
	if(objTyp == 0x2c && FindInvType(att_objNum, 0x3b, -1) < 0)
		return 1;
	if(objTyp == 0x2b && FindInvType(att_objNum, 0x3a, -1) < 0)
		return 1;
	if((objTyp == 0x81 || objTyp == 0x105) && !GetQuan(weap_objNum))
		return 1;
	if(objTyp == 0xf0 || objTyp == 0xf1 || objTyp == 0x2d) {
		tmp = GetQual(weap_objNum);
		doHit = tmp < 2;
		doSpray = tmp != 1;
		if(objTyp == 0x2d) {
			if(doHit && FindInvType(att_objNum, 0x3b, -1) < 0)
				doHit = 0;
			if(doSpray && FindInvType(att_objNum, 0x3a, -1) < 0)
				doSpray = 0;
		} else {
			if(!(tmp = GetQuan(weap_objNum)))
				return 1;
			if(doHit + doSpray * 4 > tmp)
				doSpray = 0;
		}
		if(!doHit && !doSpray)
			return 1;
	} else if(objTyp == 0x2b) {
		doHit = 0;
		doSpray = 1;
	} else {
		doHit = 1;
		doSpray = 0;
	}
	if(objTyp == 0xf0)
		kind = 3;
	else if(objTyp == 0xf1)
		kind = 1;

	MissileHitObj = SelectionObj;
	if(doHit) {
		isRanged = 0;
		if(range > 1)
			isRanged = 1;
		if(
			(objTyp >= 0x29 && objTyp <= 0x30) || objTyp == 0x81 || objTyp == 0x105 ||
			objTyp == 0xf0 || objTyp == 0xf1 || objTyp == 0x28
		) isRanged = 1;

		if(weap_objNum == att_objNum) {
			if((objCls = GetMonsterClass(GetType(att_objNum))) >= 0)
				damage = MonsterDamage[objCls + 1];
			else
				damage = 1;
		} else {
			if((damage = GetDamagePoints(weap_objNum)) < 0)
				damage = 1;
		}

		didHit = -1;
		opp_objNum = COMBAT_getHead(MissileHitObj);
		if(MissileHitObj >= 0 && IsNPC(opp_objNum) && objTyp != 0x81 && objTyp != 0x105)
			didHit = __AttackTest(att_objNum, opp_objNum, weap_objNum);
		if(isRanged && !COMBAT_FireRangedWeapon(att_objNum, &x, &y, weap_objNum))
			didHit = -1;
		opp_objNum = COMBAT_getHead(MissileHitObj);
		if(!isRanged && COMBAT_TileDistance(att_objNum, MapX, MapY) < 6)
			MUS_PlaySfx(0);/*melee sound*/
		if(didHit == -1) {
			didHit = 1;
			if(MissileHitObj >= 0 && IsNPC(opp_objNum) && objTyp != 0x81 && objTyp != 0x105)
				didHit = __AttackTest(att_objNum, opp_objNum, weap_objNum);
		}
		if(didHit == 0)
			MUS_PlaySfx(3);

		if(didHit && MissileHitObj >= 0 && opp_objNum != att_objNum) {
			if(objTyp == 0xf1) {
				if((tmp = GetType(opp_objNum)) == 0xa0 && GetFrame(opp_objNum) == 1) {
					CON_printf(C_2BB4_20D6);
					SetFrame(opp_objNum, 2);
					VisibilityDirty = 1;
				} else if(
					(tmp = GetMonsterClass(tmp)) >= 0 && IsNPC(opp_objNum) &&
					((MonsterFlags[tmp] & 0x4000) || (MonsterFlags[tmp] & 0x80))
				) {
					NPCStatus[opp_objNum] |= 2;
					CON_printf(C_2BB4_13B1, GetObjectString(opp_objNum));
					if((MonsterFlags[tmp] & 0x80) && !(MonsterFlags[tmp] & 0x4000))
						__DoHit(att_objNum, opp_objNum, damage, 1);
					else
						C_07D1_1378(opp_objNum);
				} else {
					CON_printf(C_2BB4_02A4);
				}
			} else if(objTyp == 0x81 || objTyp == 0x105) {
				if(GetQual(weap_objNum)) {
					if(IsNPC(opp_objNum) && GetType(opp_objNum) == 0x91) {
						SetType(opp_objNum, 0x16c);
						CON_printf(C_2BB4_0FDE, GetObjectString(opp_objNum));
						ShowObjects();
					} else if(IsInvisible(opp_objNum)) {
						ClrInvisible(opp_objNum);
						CON_printf(C_2BB4_0FDE, GetObjectString(opp_objNum));
						ShowObjects();
					} else {
						CON_printf(C_2BB4_02A4);
					}
				} else {
					if(!(tmp = GetMonsterClass(GetType(opp_objNum))) || (MonsterFlags[tmp] & 0x4000) || !(MonsterFlags[tmp] & 0x80))
						CON_printf(C_2BB4_02A4);
					else
						__DoHit(att_objNum, opp_objNum, damage, 2);
				}
			} else if(objTyp == 0x28) {
				if(IsNPC(opp_objNum) && GetAlignment(opp_objNum) != 0x40) {
					C_07D1_1378(opp_objNum);
					NPCFlag[opp_objNum] = (NPCFlag[opp_objNum] & ~0x60) | GetAlignment(opp_objNum);
					SetAlignment(opp_objNum, 0x40);
					ObjStatus[opp_objNum] |= 4;
					CON_printf("%s charmed!\n", GetObjectString(opp_objNum));
				}
			} else {
				__DoHit(att_objNum, opp_objNum, damage, kind);
			}
		}
		if(!doSpray)
			return 0;
	}
	if(D_0704) {
		D_0704 = 0;
		return 0;
	}
	C_07D1_0D54(att_objNum, x, y, objTyp);
	if(objTyp == 0x2b || objTyp == 0x2d)
		TakeObj(att_objNum, 0x3a, 1);
	else
		SubQuan(weap_objNum, 4);
	if(StatusDisplay == 0x92 && Party[StatusMember] == att_objNum)
		StatusDirty = 1;

	return 0;
}

/*reset equiped weapon list: collect the (up to 8) equipped weapons of
  the active party member into EquipWeapons and print the first one*/
/*C_07D1_3028*/static COMBAT_InitWeaponList() {
	int16 invObj, attacker;

	attacker = Party[Active];
	NumEquipWeapons =
	CurEquipWeapon = 0;
	for(invObj = FindInv(attacker); invObj >= 0 && NumEquipWeapons < 8; invObj = NextInv()) {
		if(GetCoordUse(invObj) == 0x18 && GetDamagePoints(invObj) >= 0) {
			EquipWeapons[NumEquipWeapons] = invObj;
			NumEquipWeapons ++;
		}
	}
	if(NumEquipWeapons == 0)
		CON_printf(C_2BB4_00EB);
	else
		CON_printf(C_2BB4_00FC, GetObjectString(EquipWeapons[0]));
}

/*next equiped weapon list: attack the selected target with the current
  weapon (EquipWeapons[CurEquipWeapon], bare hands if none; a missing
  ammunition ends it with a sound), then prepare the targeting of the
  next equipped weapon, or end the command. The weapon 0x139 strikes
  through C_07D1_5FEF with the player's figure turned towards the target*/
/*C_07D1_30BA*/static COMBAT_AttackWithNextWeapon() {
	int16 attacker, weapTyp;
	int16 weapObj, hitResult, dir;

	attacker = Party[Active];
	if(NumEquipWeapons == 0)
		weapObj = attacker;
	else
		weapObj = EquipWeapons[CurEquipWeapon];
	weapTyp = GetType(weapObj);
	if(weapTyp == 0x30 && FindInvOfType(attacker, 0x40) < 0) {
		CON_printf(C_2BB4_0105);
		MUS_PlaySfx(5);
	} else if(weapTyp == 0x2f && FindInvType(attacker, 0x3f, -1) < 0) {
		CON_printf(C_2BB4_0115);
		MUS_PlaySfx(5);
	} else if((weapTyp == 0x29 || weapTyp == 0x2a) && FindInvType(attacker, 0x39, -1) < 0) {
		CON_printf(C_2BB4_0115);
		MUS_PlaySfx(5);
	} else if(weapTyp == 0x2e && FindInvType(attacker, 0x3c, -1) < 0) {
		CON_printf(C_2BB4_0115);
		MUS_PlaySfx(5);
	} else if(weapTyp == 0x2c && FindInvType(attacker, 0x3b, -1) < 0) {
		CON_printf(C_2BB4_0115);
		MUS_PlaySfx(5);
	} else if(weapTyp == 0x2b && FindInvType(attacker, 0x3a, -1) < 0) {
		CON_printf(C_2BB4_0115);
		MUS_PlaySfx(5);
	} else if(weapTyp == 0x2d &&
		(FindInvType(attacker, 0x3b, -1) < 0 || GetQual(weapObj) == 2) &&
		(FindInvType(attacker, 0x3a, -1) < 0 || GetQual(weapObj) == 1)) {
		CON_printf(C_2BB4_0115);
		MUS_PlaySfx(5);
	} else if((weapTyp == 0x81 || weapTyp == 0x105) && !GetQuan(weapObj)) {
		CON_printf(C_2BB4_0115);
		MUS_PlaySfx(5);
	} else if((weapTyp == 0xf0 || weapTyp == 0xf1) &&
		(GetQuan(weapObj) == 0 || (GetQual(weapObj) == 2 && GetQuan(weapObj) < 4))) {
		CON_printf(C_2BB4_0115);
		MUS_PlaySfx(5);
	} else {
		hitResult = 0;
		if(SelectionObj >= 0) {
			if(SelectionObj == attacker) {
				CON_printf(C_2BB4_0129);
			} else {
				CON_printf("%s.\n", GetObjectString(SelectionObj));
				dir = MkDirection(PointerX, PointerY);
				if(dir != -1) {
					UpdateMoveFrame(Party[Active], dir);
					VIS_UpdateView();
				}
			}
		} else {
			if(SelectionIsDark)
				CON_printf(C_2BB4_0402);
			else
				CON_printf("%s.\n", GetTileString(GetTileAtXYZ(SelectionX, SelectionY, MapZ)));
		}
		if(SelectionObj != attacker) {
			CON_printf("\n");
			SkipNewlines = 1;
			if(SelectionObj < 0)
				SelectionObj = COMBAT_GetTargetAt(attacker, SelectionX, SelectionY);
			if(weapTyp == 0x139) {
				ObjShapeType[0] = (D_16A8 ? 0x18d : 0x18c) | (dir >> 1 << 10);
				VIS_UpdateView();
				MapAnimEnabled = 1;
				DisplayTick();
				hitResult = C_07D1_5FEF(attacker, SelectionX, SelectionY, weapObj);
				ObjShapeType[0] = (ObjShapeType[1] & 0x3ff) | (dir >> 1 << 3 << 10);
				VIS_UpdateView();
				MapAnimEnabled = 1;
				DisplayTick();
			} else {
				hitResult = __TryHit(attacker, SelectionX, SelectionY, weapObj);
			}
			if(hitResult == 2) {
				CON_printf(C_2BB4_03A6);
				LastAttackTarget = -1;
				MUS_PlaySfx(5);
			} else if(hitResult == 3) {
				CON_printf(C_2BB4_034D);
				MUS_PlaySfx(5);
				LastAttackTarget = -1;
			}
		}
	}
	CurEquipWeapon ++;
	if(CurEquipWeapon < NumEquipWeapons) {
		SelectMode = 1;
		SelectRange = GetWeaponRange(EquipWeapons[CurEquipWeapon]);
		if(C_07D1_35F5((AimX + MapX - 5) & 0x3ff, (AimY + MapY - 5) & 0x3ff, attacker) > SelectRange) {
			AimX =
			AimY = 5;
		}
		LastNumberPlural = 0;
		CON_printf(C_2BB4_00D3, GetObjectString(EquipWeapons[CurEquipWeapon]));
	} else {
		MouseMode = 0;
		SelectMode = 0;
		CON_UpdateMousePointer();
	}
}

/*the player's "Attack" command: select a target for each equipped
  weapon in turn*/
/*C_07D1_3480*/COMBAT_attack() {
	int16 weapObj;
	int16 ch, attacker;

	attacker = Party[Active];
	CON_printf(C_2BB4_00E4);
	CON_printf(" ");
	MouseMode = 1;
	PendingCommand = 0x81;
	CON_UpdateMousePointer();
	COMBAT_InitWeaponList();
	if(NumEquipWeapons == 0)
		weapObj = attacker;
	else
		weapObj = EquipWeapons[CurEquipWeapon];
	SelectRange = GetWeaponRange(weapObj);
	if(C_07D1_35F5((AimX + MapX - 5) & 0x3ff, (AimY + MapY - 5) & 0x3ff, attacker) > SelectRange) {
		AimX =
		AimY = 5;
	}
	ch = CON_getch();
	LastAttackTarget = SelectionObj;
	if(ch == 0x8e && (SelectionObj < 0 || GetCoordUse(SelectionObj) == 0)) {
		do {
			COMBAT_AttackWithNextWeapon();
			if(MouseMode == 1) {
				RefreshStatus();
				ch = CON_getch();
				if(ch != 0x8e) {
					CON_printf(C_2BB4_03F1);
					break;
				}
			}
		} while(MouseMode == 1);
	} else {
		CON_printf(C_2BB4_03F1);
	}
	SubMov(Party[Active], 10);
	MouseMode = 0;
	SelectMode = 0;
	CON_UpdateMousePointer();
}

/*combat distance between two locations (RangeTable on the wrapping map;
  9 if too far)*/
/*C_07D1_35A8*/COMBAT_range(int16 x0, int16 y0, int16 x1, int16 y1) {
	int16 delta_x, delta_y;
	int16 range;

	delta_x = MapDistance(x0, x1);
	delta_y = MapDistance(y0, y1);
	if(delta_x < 8 && delta_y < 8)
		range = RangeTable[delta_x][delta_y];
	else
		range = 9;
	return range;
}

/*combat distance from an object to (x, y): COMBAT_range from its square;
  for the tiles of two squares (TileFlag 0x40 or 0x80: the second square
  above or to the left, by the frame / 4) from the nearer square; for the
  tiles of four squares (TileFlag 0xc0) 1 when (x, y) is next to them*/
/*C_07D1_35F5*/int16 C_07D1_35F5(int16 x, int16 y, int16 objNum) {
	int16 objX, objY, tile;
	int16 dx, dy;

	objX = GetX(objNum);
	objY = GetY(objNum);
	tile = TILE_FRAME(objNum);
	if((TileFlag[tile] & 0xc0) && (TileFlag[tile] & 0xc0) != 0xc0) {
		switch(GetFrame(objNum) >> 2) {
		case 0: return COMBAT_range(x, y, objX, (objY - 1) & 0x3ff);
		case 1: return COMBAT_range(x, y, (objX - 1) & 0x3ff, objY);
		case 2: case 3: return COMBAT_range(x, y, objX, objY);
		default: return x / (x - x);
		}
	} else if((TileFlag[tile] & 0xc0) == 0xc0) {
		dx = WrapDelta(x - objX);
		dy = WrapDelta(y - objY);
		if(dx >= -2 && dx <= 1 && dy >= -2 && dy <= 1)
			return 1;
		return COMBAT_range(x, y, objX, objY);
	}
	return COMBAT_range(x, y, objX, objY);
}

/*1 when no member of the party (an actor under the player's control)
  stands in the line of fire from objNum to target*/
/*C_07D1_3722*/C_07D1_3722(int16 objNum, int16 target) {
	int16 far *hits;

	for(hits = C_0E1A_1003(GetX(objNum), GetY(objNum), GetX(target), GetY(target), 0); *hits >= 0; hits ++)
		if(IsPlrControl(*hits))
			return 0;
	return 1;
}

/*the most damaging of the weapons att holds that reach opp (att itself:
  bare hands, if none). The 0x81/0x105 with a quality only against an
  invisible opponent, without one only against C_100B_0410's types or
  the classes with flag 0x80 and without 0x4000; a clever attacker (not
  hostile to the party, OSI_rand(1, 30) <= its intelligence) does not
  spray (0xf0, 0xf1, 0x2d, or fire a 0x2b) through the party: the
  sprayers then get the quality 1 (one target), else 0*/
/*C_07D1_37AF*/static __PickWeapon(int16 att_objNum, int16 opp_objNum) {
	int16 oppX, oppY, attX, attY;
	int16 range, best, damage, bestDamage;
	int16 weapTyp, oppTyp, oppClass;
	int16 clever, isSprayer;
	int16 invObj;

	best = -1;
	clever = !(NPCStatus[att_objNum] & 0x20) && OSI_rand(1, 30) <= GetInt(att_objNum);
	best = att_objNum;
	bestDamage = 0;
	oppX = GetX(opp_objNum);
	oppY = GetY(opp_objNum);
	range = C_07D1_35F5(oppX, oppY, att_objNum);
	oppTyp = GetType(opp_objNum);
	oppClass = GetMonsterClass(oppTyp);
	attX = GetX(att_objNum);
	attY = GetY(att_objNum);
	for(invObj = FindInv(att_objNum); invObj >= 0; invObj = NextInv()) {
		weapTyp = GetType(invObj);
		if(GetCoordUse(invObj) != 0x18)
			continue;
		if(GetWeaponRange(invObj) < range)
			continue;
		if(weapTyp == 0x81 || weapTyp == 0x105) {
			if(GetQual(invObj)) {
				if(!IsInvisible(opp_objNum))
					continue;
			} else if(!C_100B_0410(oppTyp)) {
				if(oppClass == 0 || !(MonsterFlags[oppClass] & 0x80) || (MonsterFlags[oppClass] & 0x4000))
					continue;
			}
		}
		isSprayer = weapTyp == 0xf0 || weapTyp == 0xf1 || weapTyp == 0x2d;
		if(clever && (isSprayer || weapTyp == 0x2b) && !C_07D1_3722(att_objNum, opp_objNum)) {
			if(!isSprayer)
				continue;
			SetQual(invObj, 1);
		} else if(isSprayer) {
			SetQual(invObj, 0);
		}
		damage = GetDamagePoints(invObj);
		if(damage > bestDamage) {
			bestDamage = damage;
			best = invObj;
		}
	}
	return best;
}

/*closest visible enemy within 11 tiles on the same level, according to
  the alignments (GOOD attacks the ones hostile to the party, EVIL the
  ones hostile to monsters, CHAOTIC everything but chaotic or neutral and
  its own kind (the creature 0x174 and its parts 0x172/0x173, 0x16a,
  0x175 and 0xfe together), NEUTRAL anything not neutral; never 0x17d);
  the strongest attack wins a tie. Retreating actors off-screen are
  ignored. returns -1 if none*/
/*C_07D1_398D*/COMBAT_pickOpponent(int16 objNum) {
	int16 i;
	int16 x, y, opp_x, opp_y, objAln;
	int16 dst2, ret, min_dst2;
	int16 objTyp, oppTyp;

	if(GetZ(objNum) != MapZ)
		return -1;
	ret = -1;
	objAln = GetAlignment(objNum);
	x = GetX(objNum);
	y = GetY(objNum);
	min_dst2 = 0x7fff;
	if(objAln == 0x60)
		objTyp = GetType(objNum);
	for(i = 0; i < 0x100; i++) {
		if(GetZ(i) != MapZ)
			continue;
		if(ObjShapeType[i] == 0)
			continue;
		if(IsDead(i))
			continue;
		if(i == objNum)
			continue;
		if(!COMBAT_canSee(objNum, i))
			continue;
		if(GetType(i) == 0x17d)
			continue;
		if(objAln == 0 && GetAlignment(i) == 0)
			continue;
		if(objAln == 0x60) {
			oppTyp = GetType(i);
			if(objTyp == oppTyp)
				continue;
			if(objTyp == 0x174 && (oppTyp == 0x172 || oppTyp == 0x173))
				continue;
			if(
				(objTyp == 0x16a || objTyp == 0x175 || objTyp == 0xfe) &&
				(oppTyp == 0x16a || oppTyp == 0x175 || oppTyp == 0xfe)
			)
				continue;
		}
		if(objAln == 0x40 && !Is_ATKPLR(i))
			continue;
		if(objAln == 0x20 && !Is_ATKMON(i))
			continue;
		opp_x = GetX(i);
		opp_y = GetY(i);
		if(COMBAT_TileDistance(objNum, opp_x, opp_y) > 11)
			continue;
		if(NPCMode[i] == 7 && (MapDistance(opp_x, MapX) > 5 || MapDistance(opp_y, MapY) > 5))
			continue;
		dst2 = MapDistance(opp_x, x) * MapDistance(opp_x, x) + MapDistance(opp_y, y) * MapDistance(opp_y, y);
		if(
			dst2 < min_dst2 ||
			(dst2 == min_dst2 && MonsterDamage[GetMonsterClass(GetType(i)) + 1] > MonsterDamage[GetMonsterClass(GetType(ret)) + 1])
		) {
			min_dst2 = dst2;
			ret = i;
		}
	}
	return ret;
}

/*AI_ASSAULT (also the fallback of the other modes): attack the closest
  enemy if in range and in line of fire, else step to its row or column;
  move toward it when out of range*/
/*C_07D1_3C26*/COMBAT_AI_Assault(int16 objNum) {
	int16 weapObj, opp_x, opp_y, x, y, range, dist;
	int16 weapTyp;

	SelectionObj = COMBAT_pickOpponent(objNum);
	weapObj = __PickWeapon(objNum, SelectionObj);
	weapTyp = GetType(weapObj);
	if(SelectionObj >= 0) {
		opp_x = GetX(SelectionObj);
		opp_y = GetY(SelectionObj);
		x = GetX(objNum);
		y = GetY(objNum);
		range = GetWeaponRange(weapObj);
		if((dist = C_07D1_35F5(opp_x, opp_y, objNum)) <= range) {
			if(dist <= 1 || COMBAT_Missile(objNum, &opp_x, &opp_y, range, 0)) {
				__TryHit(objNum, opp_x, opp_y, weapObj);
				SubMov(objNum, 10);
				return;
			}
			/*no line of fire: step to the target's row or column*/
			if(OSI_rand(0, 1)) {
				opp_x = x;
				opp_y = GetY(SelectionObj);
			} else {
				opp_x = GetX(SelectionObj);
				opp_y = y;
			}
		}
		TryMoveTo(objNum, opp_x, opp_y);
	} else {
		WanderRandomly(objNum);
	}
}

/*choose a ranged weapon (range 3 or more) for an NPC that keeps its
  distance: the longest range until one reaches the opponent, then the
  best damage; -1 if none*/
/*C_07D1_3D62*/C_07D1_3D62(int16 att_objNum, int16 opp_objNum) {
	int16 opp_x, opp_y, att_x, att_y, ret, dmg, max_dmg, itemTyp, range, wrange, oppTyp;
	int16 maxRange, i;

	maxRange = 0;
	ret = -1;
	max_dmg = 0;
	opp_x = GetX(opp_objNum);
	opp_y = GetY(opp_objNum);
	att_x = GetX(att_objNum);
	att_y = GetY(att_objNum);
	oppTyp = GetType(opp_objNum);
	range = C_07D1_35F5(opp_x, opp_y, att_objNum);
	i = FindInv(att_objNum);
	while(i >= 0) {
		itemTyp = GetType(i);
		if(GetWeaponRange(i) < 3 || GetCoordUse(i) != 0x18) {
			i = NextInv();
			continue;
		}
		if(maxRange < range) {
			if((wrange = GetWeaponRange(i)) > maxRange) {
				maxRange = wrange;
				max_dmg = GetDamagePoints(i);
				ret = i;
			}
		} else {
			if((dmg = GetDamagePoints(i)) > max_dmg) {
				maxRange = GetWeaponRange(i);
				max_dmg = dmg;
				ret = i;
			}
		}
		i = NextInv();
	}
	return ret;
}

/*AI_RANGE: keep away (3 tiles) from the closest enemy and shoot it with
  a ranged weapon; stay near the party*/
/*C_07D1_3E77*/COMBAT_AI_Range(int16 objNum) {
	int16 weapObj, opp_x, opp_y, x, range, dist, weapTyp;
	int16 y;

	x = GetX(objNum);
	y = GetY(objNum);
	SelectionObj = COMBAT_pickOpponent(objNum);
	weapObj = C_07D1_3D62(objNum, SelectionObj);
	if(SelectionObj >= 0 && MapDistance(GetX(SelectionObj), x) < 3 && MapDistance(GetY(SelectionObj), y) < 3) {
		TryMoveTo(objNum, (x - GetX(SelectionObj)) & 0x3ff, (y - GetY(SelectionObj)) & 0x3ff);
	} else if(weapObj == -1) {
		CON_printf("\n");
		CON_printf(GetObjectString(objNum));
		CON_printf(" has no ranged weapons!\n");
	} else if(MapDistance(x, MapX) > 5 || MapDistance(y, MapY) > 4) {
		TryMoveTo(objNum, MapX, MapY);
	} else {
		weapTyp = GetType(weapObj);
		if(SelectionObj >= 0) {
			opp_x = GetX(SelectionObj);
			opp_y = GetY(SelectionObj);
			range = GetWeaponRange(weapObj);
			if((dist = C_07D1_35F5(opp_x, opp_y, objNum)) <= range) {
				if(COMBAT_Missile(objNum, &opp_x, &opp_y, range, 0)) {
					__TryHit(objNum, opp_x, opp_y, weapObj);
					SubMov(objNum, 10);
					return;
				}
				/*no line of fire: step to the target's row or column*/
				if(OSI_rand(0, 1)) {
					opp_x = x;
					opp_y = GetY(SelectionObj);
				} else {
					opp_x = GetX(SelectionObj);
					opp_y = y;
				}
				TryMoveTo(objNum, opp_x, opp_y);
			}
			TryMoveTo(objNum, opp_x, opp_y);
		} else {
			WanderRandomly(objNum);
		}
	}
}

/*AI_BERSERK: attack the strongest (STR+DEX+INT) visible enemy within 8
  tiles (CHAOTIC: anyone but its own kind)*/
/*C_07D1_40A5*/COMBAT_AI_Berserk(int16 objNum) {
	int16 i;
	int16 weapObj, bestOpp, objAln, x, y, oppStats, maxStats;
	int16 oppX, oppY, range, delta_x, delta_y, mustMove, savedMovePts;
	int16 weapTyp, dist, objTyp;

	objTyp = GetType(objNum);
	bestOpp = -1;
	mustMove = 0;
	objAln = GetAlignment(objNum);
	x = GetX(objNum);
	y = GetY(objNum);
	maxStats = 0;
	for(i = 0; i < 0x100; i ++) {
		if(ObjShapeType[i] == 0)
			continue;
		if(IsDead(i))
			continue;
		if(i == objNum)
			continue;
		if(COMBAT_TileDistance(i, x, y) > 8)
			continue;
		if(!COMBAT_canSee(objNum, i))
			continue;
		if(objAln == 0x40 && GetAlignment(i) != 0x20)
			continue;
		if(objAln == 0x20 && GetAlignment(i) != 0x40)
			continue;
		if(objAln == 0x60) {
			int16 oppTyp;

			oppTyp = GetType(i);
			if(objTyp == oppTyp)
				continue;
			if(objTyp == 0x174 && (oppTyp == 0x172 || oppTyp == 0x173))
				continue;
			if(
				(objTyp == 0x16a || objTyp == 0x175 || objTyp == 0xfe) &&
				(oppTyp == 0x16a || oppTyp == 0x175 || oppTyp == 0xfe)
			)
				continue;
		}
		oppStats = GetStr(i) + GetDex(i) + GetInt(i);
		if(oppStats > maxStats) {
			maxStats = oppStats;
			bestOpp = i;
		}
	}
	if(bestOpp >= 0) {
		oppX = GetX(bestOpp);
		oppY = GetY(bestOpp);
		delta_x = MapDistance(oppX, x);
		delta_y = MapDistance(oppY, y);
		weapObj = __PickWeapon(objNum, bestOpp);
		weapTyp = GetType(weapObj);
		range = GetWeaponRange(weapObj);
		if((dist = C_07D1_35F5(oppX, oppY, objNum)) <= range) {
			SelectionObj = bestOpp;
			if(dist <= 1 || COMBAT_Missile(objNum, &oppX, &oppY, range, 0)) {
				__TryHit(objNum, oppX, oppY, weapObj);
				SubMov(objNum, 10);
				return;
			} else {
				if(OSI_rand(0, 1)) {
					oppX = x;
					oppY = GetY(SelectionObj);
				} else {
					oppX = GetX(SelectionObj);
					oppY = y;
				}
				mustMove = 1;
			}
		} else {
			mustMove = 1;
		}
		if(mustMove) {
			savedMovePts = MovePts[objNum];
			if(!TryMoveTo(objNum, oppX, oppY)) {
				MovePts[objNum] = savedMovePts;
				COMBAT_AI_Assault(objNum);
			}
		}
	} else {
		COMBAT_AI_Assault(objNum);
	}
	SubMov(objNum, 5);
}

/*AI_RETREAT: flee from the closest opponent (when within 8 tiles); good
  ones go back to the party. A retreating NPC not under the player's
  control regains 1 HP 1 time in 4 and goes back to its combat mode when
  no longer critical*/
/*C_07D1_4352*/COMBAT_AI_Retreat(int16 objNum) {
	int16 oppObjNum;
	int16 delta_x, delta_y, savedMovePts;

	if(COMBAT_TileDistance(objNum, MapX, MapY) > 7) {
		MovePts[objNum] = 0;
		return;
	}
	oppObjNum = COMBAT_pickOpponent(objNum);
	if(oppObjNum < 0) {
		WanderRandomly(objNum);
		return;
	}
	delta_x = WrapDelta(GetX(oppObjNum) - GetX(objNum));
	delta_y = WrapDelta(GetY(oppObjNum) - GetY(objNum));
	if(abs(delta_x) < 8 && abs(delta_y) < 8) {
		savedMovePts = MovePts[objNum];
		if(!TryMoveTo(objNum, (GetX(objNum) - delta_x) & 0x3ff, (GetY(objNum) - delta_y) & 0x3ff))
			MovePts[objNum] = savedMovePts;
	} else if(GetAlignment(objNum) == 0x40) {
		TryMoveTo(objNum, MapX, MapY);
	} else {
		WanderRandomly(objNum);
	}
	if(!IsPlrControl(objNum) && OSI_rand(0, 3) == 0) {
		if(MaxHP(objNum) > HitPoints[objNum])
			HitPoints[objNum] ++;
		if(HitPoints[objNum] * 4 / MaxHP(objNum) > 0)
			NPCMode[objNum] = NPCComMode[objNum];
	}
}

/*AI_LIKE: approach the party now and then when close, else wander*/
/*C_07D1_44D8*/COMBAT_AI_Like(int16 objNum) {
	int16 i;
	int16 x, y, nearParty;

	nearParty = 0;
	x = GetX(objNum);
	y = GetY(objNum);
	for(i = 0; i < PartySize; i ++) {
		if(MapDistance(GetX(Party[i]), x) < 3 && MapDistance(GetY(Party[i]), y) < 3) {
			nearParty = 1;
			break;
		}
	}
	if(nearParty && OSI_rand(0, 1) == 0) {
		TryMoveTo(objNum, MapX, MapY);
		return;
	}
	if(OSI_rand(0, 1)) {
		/*(x + OSI_rand(0, 1)) ? 10 : -10: a bug of the source*/
		if(OSI_rand(0, 1))
			x = (x + OSI_rand(0, 1)?10:-10) & 0x3ff;
		else
			y = (y + OSI_rand(0, 1)?10:-10) & 0x3ff;
		TryMoveTo(objNum, x, y);
		return;
	}
	SubMov(objNum, 5);
}

/*AI_UNFRIENDLY: wander; when close to the party, 1 in 6 to turn into
  AI_ASSAULT, hostile to the party (0x17a then becomes 0x17c, stronger)*/
/*C_07D1_4612*/COMBAT_AI_Unfriendly(int16 objNum) {
	int16 i;
	int16 x, y, nearParty;

	nearParty = 0;
	x = GetX(objNum);
	y = GetY(objNum);
	for(i = 0; i < PartySize; i ++) {
		if(MapDistance(GetX(Party[i]), x) < 3 && MapDistance(GetY(Party[i]), y) < 3) {
			nearParty = 1;
			break;
		}
	}
	if(nearParty && OSI_rand(0, 5) == 0) {
		NPCMode[objNum] = 8;
		NPCStatus[objNum] |= 0x20;
		if(GetType(objNum) == 0x17a) {
			SetType(objNum, 0x17c);
			SetFrame(objNum, 0);
			STREN[objNum] += 5;
			DEXTE[objNum] -= 5;
			MovePts[objNum] -= 5;
			INTEL[objNum] -= 7;
			HitPoints[objNum] += 20;
			VisibilityDirty = 1;
			DisplayTick();
		}
		COMBAT_AI_Assault(objNum);
		return;
	}
	if(OSI_rand(0, 1)) {
		if(OSI_rand(0, 1))
			x = (x + OSI_rand(0, 1)?10:-10) & 0x3ff;
		else
			y = (y + OSI_rand(0, 1)?10:-10) & 0x3ff;
		TryMoveTo(objNum, x, y);
		return;
	}
	SubMov(objNum, 5);
}

/*AI_IMMOBILE: never moves; attacks the closest enemy in range and line of
  fire. The creatures 0x175, 0x16a and 0x182 show a random frame (3 or 7);
  0x175 and 0x182 go invisible and turn to mode 0x12 when their enemy is
  8 squares away or more*/
/*C_07D1_47D8*/COMBAT_AI_Immobile(int16 objNum) {
	int16 objAln, weapObj, range, opp_x, opp_y, dist;
	int16 objTyp;

	objTyp = GetType(objNum);
	if(objTyp == 0x175 || objTyp == 0x16a || objTyp == 0x182)
		SetFrame(objNum, OSI_rand(0, 1) ? 3 : 7);
	objAln = GetAlignment(objNum);
	if(objAln != 0 && (SelectionObj = COMBAT_pickOpponent(objNum)) >= 0) {
		weapObj = __PickWeapon(objNum, SelectionObj);
		range = GetWeaponRange(weapObj);
		opp_x = GetX(SelectionObj);
		opp_y = GetY(SelectionObj);
		if((dist = C_07D1_35F5(opp_x, opp_y, objNum)) <= range) {
			if(dist <= 1 || COMBAT_Missile(objNum, &opp_x, &opp_y, range, 0)) {
				__TryHit(objNum, opp_x, opp_y, weapObj);
				SubMov(objNum, 10);
				return;
			}
		} else if((objTyp == 0x175 || objTyp == 0x182) && dist >= 8) {
			ObjStatus[objNum] |= 2;
			NPCMode[objNum] = NPCComMode[objNum] = 0x12;
			SubMov(objNum, 5);
			return;
		}
	}
	SubMov(objNum, 5);
}

/*A.I. of mode 0x13: moves at random and hurts everything on its new
  square (but the invulnerable ones); in dream stage 0x44 the 0x18e
  moves one turn in two, shaking the screen*/
/*C_07D1_4912*/C_07D1_4912(int16 objNum) {
	int16 objTyp;
	int16 i;

	objTyp = GetType(objNum);
	SubMov(objNum, 5);
	if(objTyp == 0x18e && DreamStage == 0x44) {
		if(OSI_rand(0, 1))
			return;
		ShakeScreen(4, 1);
	}
	TryStraightMove(objNum, OSI_rand(0, 7), 0);
	for(i = FindLoc(GetX(objNum), GetY(objNum), GetZ(objNum)); i >= 0; i = NextLoc())
		if(i != objNum && !IsInvulnerable(i))
			LoseHP(i, OSI_rand(1, 10) + OSI_rand(1, 10), 0);
}

/*A.I. of mode 0x16 (the dream): goes for the 0xa2 (carried by NPC 0, or
  the first one in the area), hurting everything on its way; it strikes
  NPC 0 when next to him; next to a 0xa2 on the map in the zone x >= 0x37,
  y <= 0x37 (TalkFlags[0x54] & 0x40 clear) the dream ends with NPC 0x54's
  talk. Without any 0xa2 as mode 0x13; gone in dream stage 0x60*/
/*C_07D1_4A00*/C_07D1_4A00(int16 objNum) {
	int16 target, tx, ty;
	int16 i;

	if(DreamStage == 0x60) {
		DeleteObj(objNum);
		return;
	}
	if((target = FindInvType(0, 0xa2, -1)) >= 0) {
		tx = GetX(0);
		ty = GetY(0);
		SelectionObj = 0;
	} else {
		for(i = SearchArea(AreaX, AreaY, (AreaX + 0x27) & 0x3ff, (AreaY + 0x27) & 0x3ff); i >= 0; i = NextArea()) {
			if(GetType(i) == 0xa2) {
				target = i;
				tx = GetX(target);
				ty = GetY(target);
				SelectionObj = target;
				break;
			}
		}
		if(target < 0) {
			C_07D1_4912(objNum);
			return;
		}
	}
	if(C_07D1_35F5(tx, ty, objNum) <= 1) {
		if(SelectionObj == 0) {
			__TryHit(objNum, tx, ty, objNum);
			SubMov(objNum, 10);
			return;
		}
		COMBAT_showHit(target);
		if(!(TalkFlags[0x54] & 0x40) && GetX(objNum) >= 0x37 && GetY(objNum) <= 0x37) {
			C_2F5F_0AC1(0x54);
			TalkFlags[0x54] |= 2;
			TALK_talkTo(0, 0x54, 0);
			DreamWakeUp();
			return;
		}
	}
	ShakeScreen(4, 1);
	TryMoveTo(objNum, tx, ty);
	for(i = FindLoc(GetX(objNum), GetY(objNum), GetZ(objNum)); i >= 0; i = NextLoc())
		if(i != objNum)
			LoseHP(i, OSI_rand(1, 10) + OSI_rand(1, 10), 0);
	SubMov(objNum, 5);
}

/*A.I. of mode 0x12 (hidden under the sand): when an enemy comes next to
  it, it bursts out (mode 0xe, visible, combat begins); the creature 0x175
  then brings up 10 to 30 parts 0x16a (mode 0x19, frame 3, its number in
  their quality) on free squares within 6 of it*/
/*C_07D1_4C0A*/C_07D1_4C0A(int16 objNum) {
	int16 opp, count, x, y, loc;
	int16 part, j;

	if(
		(opp = COMBAT_pickOpponent(objNum)) >= 0 &&
		(C_07D1_35F5(GetX(opp), GetY(opp), objNum) <= 1 || C_07D1_35F5(GetX(objNum), GetY(objNum), opp) <= 1)
	) {
		CON_printf("Suddenly, something bursts up through the sand!\n");
		MUS_PlaySfx(0x21);
		COMBAT_beginAttacked();
		NPCMode[objNum] = NPCComMode[objNum] = 0xe;
		ClrInvisible(objNum);
		if(GetType(objNum) == 0x175) {
			for(count = OSI_rand(0, 10) + OSI_rand(0, 10) + 10; count > 0; count --) {
				if((part = EGG_generate(0x16a, MapX, MapY, MapZ)) < 0)
					break;
				SetFrame(part, 3);
				NPCComMode[part] = NPCMode[part] = 0x19;
				for(j = 0; j < 8; j ++) {
					x = (GetX(objNum) + OSI_rand(0, 12) - 6) & 0x3ff;
					y = (GetY(objNum) + OSI_rand(0, 12) - 6) & 0x3ff;
					if(CanMoveTo(part, x, y) && ((loc = FindLoc(x, y, MapZ)) < 0 || (loc == 0x16a && LocPartOffset))) {
						MoveObj(part, x, y, MapZ);
						SetQual(part, objNum);
						break;
					}
				}
				if(j >= 8)
					DeleteObj(part);
			}
		}
		VisibilityDirty = 1;
		DisplayTick();
		COMBAT_AI_Immobile(objNum);
	} else {
		SubMov(objNum, 2);
	}
}

/*A.I. of mode 0x19 (the parts 0x16a of the creature 0x175, hidden in the
  sand): an enemy in range is struck, the part showing itself (frame 3 or
  7) if hidden; out of range a visible part sinks (0xfe, then hidden) one
  turn in two or for good when weak (a quarter of its hit points), a
  hidden one moves toward the enemy (gone when 8 squares from its
  creature) and shows itself now and then (more often when close)*/
/*C_07D1_4E34*/C_07D1_4E34(int16 objNum) {
	int16 weapObj, range, opp_x, opp_y, dist, weak;
	int16 head;

	SelectionObj = COMBAT_pickOpponent(objNum);
	weapObj = __PickWeapon(objNum, SelectionObj);
	range = GetWeaponRange(weapObj);
	opp_x = GetX(SelectionObj);
	opp_y = GetY(SelectionObj);
	if((dist = C_07D1_35F5(opp_x, opp_y, objNum)) <= range) {
		if(dist <= 1 || COMBAT_Missile(objNum, &opp_x, &opp_y, range, 0)) {
			if(IsInvisible(objNum)) {
				ClrInvisible(objNum);
				ShowObjects();
				DisplayTick();
				TimerDelay(0x745b8L);
				ObjShapeType[objNum] = 0x16a | ((OSI_rand(0, 2) ? 3 : 7) << 10);
				ShowObjects();
				DisplayTick();
			}
			__TryHit(objNum, opp_x, opp_y, weapObj);
			SubMov(objNum, 10);
		}
	} else if(!IsInvisible(objNum)) {
		SubMov(objNum, 5);
		weak = MaxHP(objNum) / 4 > HitPoints[objNum];
		if(weak || OSI_rand(1, 100) <= 50) {
			ObjShapeType[objNum] = 0xfe;
			ShowObjects();
			DisplayTick();
			TimerDelay(0x745b8L);
			if(weak)
				DeleteObj(objNum);
			else
				ObjStatus[objNum] |= 2;
			ShowObjects();
			DisplayTick();
		}
	} else {
		if((head = GetQual(objNum)) && COMBAT_range(GetX(objNum), GetY(objNum), GetX(head), GetY(head)) > 8) {
			DeleteObj(objNum);
		} else {
			TryMoveTo(objNum, opp_x, opp_y);
			if(OSI_rand(0, dist << 1) <= 1) {
				ClrInvisible(objNum);
				ShowObjects();
				DisplayTick();
				TimerDelay(0x745b8L);
				ObjShapeType[objNum] = 0x16a | ((OSI_rand(0, 2) ? 3 : 7) << 10);
				ShowObjects();
				DisplayTick();
			}
		}
	}
}

/*A.I. of mode 0x14 (a thief): next to the enemy (or, if it has none,
  another member next to it) it takes a 0x83 (oxium), leaving a random
  part of it, and flees (mode 0x1b) the more likely the more it took;
  next to an enemy without any it strikes; else it moves toward it*/
/*C_07D1_50B2*/C_07D1_50B2(int16 objNum) {
	int16 opp_x, opp_y, oxium, member;
	int16 i;

	if((SelectionObj = COMBAT_pickOpponent(objNum)) >= 0) {
		opp_x = GetX(SelectionObj);
		opp_y = GetY(SelectionObj);
		if(C_07D1_35F5(opp_x, opp_y, objNum) <= 1 || C_07D1_35F5(GetX(objNum), GetY(objNum), SelectionObj) <= 1) {
			if((oxium = FindInvOfType(SelectionObj, 0x83)) >= 0) {
			steal:
				COMBAT_showHit(SelectionObj);
				InsertObj(oxium, objNum, 0x10);
				i = GetQuan(oxium);
				SetQuan(oxium, OSI_rand(0, i));
				if(StatusDisplay == 0x92)
					StatusDirty = 1;
				CON_printf("Oxium stolen!\n");
				MUS_PlaySfx(0x24);
				SubMov(objNum, 10);
				if(OSI_rand(0, 50) <= i)
					NPCMode[objNum] = NPCComMode[objNum] = 0x1b;
			} else {
				for(i = 0; i < PartySize; i ++) {
					member = Party[i];
					if(member == SelectionObj)
						continue;
					opp_x = GetX(member);
					opp_y = GetY(member);
					if(
						(C_07D1_35F5(opp_x, opp_y, objNum) <= 1 || C_07D1_35F5(GetX(objNum), GetY(objNum), member) <= 1) &&
						(oxium = FindInvOfType(member, 0x83)) >= 0
					) {
						SelectionObj = member;
						goto steal;
					}
				}
				__TryHit(objNum, GetX(SelectionObj), GetY(SelectionObj), objNum);
				SubMov(objNum, 10);
			}
		} else {
			TryMoveTo(objNum, opp_x, opp_y);
			SubMov(objNum, 5);
		}
	} else {
		SubMov(objNum, 5);
	}
}

/*a worm (head objNum, its tail 0x173 holding the head's number in its
  quality) moves backward onto the square of its tail: the tail goes, the
  head turns to the direction the tail pointed to (from the tail's frame
  and the head's frame). Without a tail, with dieIfStuck the worm dies
  (its body actor, the head's quality, at that square, killed); returns
  the head's old direction, -1 if it did not move*/
/*C_07D1_52E6*/C_07D1_52E6(int16 objNum, int16 dieIfStuck) {
	int16 x, y, dir, bits, frame, tailFrame, oldDir;
	int16 tail;

	oldDir = GetFrame(objNum) & 6;
	dir = (GetFrame(objNum) + 4) & 6;
	x = (GetX(objNum) + DirIncrX[dir]) & 0x3ff;
	y = (GetY(objNum) + DirIncrY[dir]) & 0x3ff;
	for(tail = OBJ_FindTypeAt(0x173, x, y); tail >= 0 && GetQual(tail) != objNum; tail = C_1B09_0A5D(tail));
	if(tail < 0) {
		if(dieIfStuck) {
			if((tail = OBJ_FindNPCAt(GetQual(objNum), x, y)) >= 0) {
				if(LastAttackTarget == objNum)
					LastAttackTarget = -1;
				HitPoints[objNum] = 0;
				SetDead(objNum);
				NPCMode[objNum] = 0;
				ObjShapeType[objNum] = 0;
				MoveObj(objNum, GetX(objNum), GetY(objNum), 6);
				if(GetQual(objNum) != tail) {
					CON_printf("Worm error!\n");
				} else {
					SetQual(tail, 0);
					COMBAT_Kill(tail);
				}
			}
		} else {
			SubMov(objNum, 5);
		}
		return -1;
	}
	if(C_170D_0002(tail))
		return -1;
	tailFrame = ObjShapeType[tail] >> 10;
	frame = ObjShapeType[objNum] >> 10;
	if(tailFrame > 1) {
		bits = (tailFrame - 1) * 3 - (1 << (frame >> 1));
		for(dir = 0; bits >>= 1; dir += 2);
		dir = (dir + 4) & 6;
	} else {
		dir = frame & 6;
	}
	DeleteObj(tail);
	MoveObj(objNum, x, y, MapZ);
	SetFrame(objNum, dir + !(GetFrame(objNum) & 1));
	SetDirection(objNum, dir);
	return oldDir;
}

/*A.I. of mode 0x0d (the worms: the actor 0x172 is the body under the
  sand, its head 0x174 in its quality and the body's number in the head's;
  the tail segments 0x173 hold the head's number). The body dies with its
  head; without a head it raises one (mode 0x0d) next to it toward the
  enemy, or dies (D_16A3 counted down) when there is no room. The head
  strikes an enemy next to it, else moves toward it leaving a segment
  behind (its frame from the turn), or backs up (C_07D1_52E6, the quantity
  remembering the direction) when blocked or when too far from its body*/
/*C_07D1_5520*/C_07D1_5520(int16 objNum) {
	int16 x, y, dist, dir, diag, seg, back, segFrame, frame, r, objTyp, other, d;
	int16 tx, ty;

	objTyp = GetType(objNum);
	if((other = GetQual(objNum)) && (IsDead(other) || ObjShapeType[other] == 0)) {
		if(LastAttackTarget == objNum)
			LastAttackTarget = -1;
		HitPoints[objNum] = 0;
		SetDead(objNum);
		NPCMode[objNum] = 0;
		OBJ_MakeDeadBody(objNum);
		ObjShapeType[objNum] = 0;
		MoveObj(objNum, GetX(objNum), GetY(objNum), 6);
		SetQual(objNum, 0);
		return;
	}
	x = GetX(objNum);
	y = GetY(objNum);
	if(!C_170D_0002(objNum) || objTyp == 0x172) {
		if(
			objTyp == 0x174 && other &&
			(C_170D_0002(other) || MapDistance(x, GetX(other)) > 8 || MapDistance(y, GetY(other)) > 8)
		) {
			if((r = C_07D1_52E6(objNum, 0)) >= 0)
				SetQuan(objNum, r + 1);
			else
				SetQuan(objNum, 0xff);
			SubMov(objNum, 5);
			return;
		} else {
			frame = ObjShapeType[objNum] >> 10;
			SelectionObj = COMBAT_pickOpponent(objNum);
			x = GetX(objNum);
			y = GetY(objNum);
			tx = GetX(SelectionObj);
			ty = GetY(SelectionObj);
			dist = C_07D1_35F5(tx, ty, objNum);
			dir = C_07D1_000D(x, y, tx, ty);
			diag = dir & 1;
			if(diag)
				dir &= ~1;
			if(objTyp == 0x172) {
				if(other == 0 && dist > 1) {
					tx = (x + DirIncrX[dir]) & 0x3ff;
					ty = (y + DirIncrY[dir]) & 0x3ff;
					if((seg = EGG_generate(0x174, tx, ty, MapZ)) >= 0) {
						NPCComMode[seg] = NPCMode[seg] = 0xd;
						if(!CanMoveTo(seg, tx, ty)) {
							d = dir;
							do {
								d = (d + 2) & 7;
								tx = (x + DirIncrX[d]) & 0x3ff;
								ty = (y + DirIncrY[d]) & 0x3ff;
								if(CanMoveTo(seg, tx, ty)) {
									MoveObj(seg, tx, ty, MapZ);
									break;
								}
							} while(d != dir);
							if(d == dir) {
								DeleteObj(seg);
								seg = -1;
								DeleteObj(objNum);
								if(D_16A3)
									D_16A3 --;
							} else {
								dir = d;
							}
						}
						if(seg >= 0) {
							SetFrame(objNum, dir >> 1);
							SetFrame(seg, dir + OSI_rand(0, 1));
							SubMov(seg, 5);
							SetQual(seg, objNum);
							SetQual(objNum, seg);
						}
					}
				}
				SubMov(objNum, MovePts[objNum]);
				return;
			}
			if(dist < 2) {
				__TryHit(objNum, tx, ty, objNum);
				SubMov(objNum, 10);
				return;
			}
		retry:
			tx = (x + DirIncrX[dir]) & 0x3ff;
			ty = (y + DirIncrY[dir]) & 0x3ff;
			for(seg = OBJ_FindTypeAt(0x173, tx, ty); seg >= 0 && GetQual(seg) != objNum; seg = C_1B09_0A5D(seg));
			if(seg >= 0) {
				if((r = C_07D1_52E6(objNum, 0)) >= 0)
					SetQuan(objNum, r + 1);
				else
					SetQuan(objNum, 0xff);
				return;
			}
			if(
				GetQuan(objNum) - 1 == dir || !NPC_TryStep(objNum, tx, ty) ||
				C_170D_007E(tx, ty, MapZ) || !C_170D_0853(objNum, dir)
			) {
				if(diag) {
					diag = 0;
					dir = (dir + 2) & 6;
					goto retry;
				}
				if(OSI_rand(0, 1)) {
					if((r = C_07D1_52E6(objNum, 0)) >= 0)
						SetQuan(objNum, r + 1);
					else
						SetQuan(objNum, 0xff);
				}
				SubMov(objNum, 5);
				return;
			} else {
				back = (frame + 4) & 6;
				if(dir == back || (frame & 6) == dir)
					segFrame = dir == 0 || dir == 4;
				else
					segFrame = ((1 << (dir >> 1)) + (1 << (back >> 1))) / 3 + 1;
				if(AddMapObj(0x173 | (segFrame << 10), 0, objNum << 8, x, y, MapZ) >= 0) {
					MoveObj(objNum, tx, ty, MapZ);
					SetDirection(objNum, dir);
					SetQuan(objNum, 0xff);
				}
			}
		}
	}
	SubMov(objNum, 5);
}

/*A.I. of mode 0x11 (a worm fleeing, or critically hurt): the body 0x172
  stays, the head backs up (and the worm dies when it cannot)*/
/*C_07D1_5AD6*/C_07D1_5AD6(int16 objNum) {
	if(GetType(objNum) == 0x172)
		SubMov(objNum, MovePts[objNum]);
	else
		C_07D1_52E6(objNum, 1);
}

/*A.I. of mode 0x15: moves south while it can; when it cannot, it hurts
  (__DoHit, 6 points) an enemy next to it*/
/*C_07D1_5B09*/C_07D1_5B09(int16 objNum) {
	int16 opp_x, opp_y;

	if(!TryStraightMove(objNum, 4, 1) && (SelectionObj = COMBAT_pickOpponent(objNum)) >= 0) {
		opp_x = GetX(SelectionObj);
		opp_y = GetY(SelectionObj);
		if(C_07D1_35F5(opp_x, opp_y, objNum) < 2) {
			__DoHit(objNum, SelectionObj, 6, 0);
			PromptFlag = 1;
		}
	}
	SubMov(objNum, 3);
}

/*A.I. of mode 0x17: the 0x198 and the 0x1a8 go about together on the
  same square (a 0x198 joining a 0x1a8 already there vanishes; a 0x198
  alone becomes a 0x1a8 and makes itself a 0x198, frame 3), moving at
  random*/
/*C_07D1_5B8E*/C_07D1_5B8E(int16 objNum) {
	int16 x, y;
	int16 other;

	other = -1;
	x = GetX(objNum);
	y = GetY(objNum);
	switch(GetType(objNum)) {
	case 0x198:
		if((other = OBJ_FindTypeAt(0x1a8, x, y)) >= 0) {
			SubMov(objNum, MovePts[objNum]);
			DeleteObj(objNum);
			return;
		}
		ObjShapeType[objNum] = 0x1a8;
	case 0x1a8:
		if((other = OBJ_FindTypeAt(0x198, x, y)) < 0 && (other = AddMapObj(0xd98, 0, 0, x, y, MapZ)) < 0)
			CON_printf(C_2BB4_22F1);
	}
	TryStraightMove(objNum, OSI_rand(0, 7), 1);
	if(other >= 0)
		MoveObj(other, GetX(objNum), GetY(objNum), MapZ);
	SubMov(objNum, 5);
}

/*enter combat mode when attacked: as COMBAT_begin, but CombatOutcome is
  kept and the music is C_1FA9_032B*/
/*C_07D1_5CAF*/COMBAT_beginAttacked() {
	int16 partyIdx, mouseSave;

	InCombat = 1;
	for(partyIdx = 0; partyIdx < PartySize; partyIdx ++) {
		if(!IsCharmed(Party[partyIdx]))
			NPCMode[Party[partyIdx]] = NPCComMode[Party[partyIdx]];
		Leader[Party[partyIdx]] = Party[partyIdx];
	}
	if(SoloMember < 0) {
		for(
			partyIdx = 0;
			(IsIncapacitated(Party[partyIdx]) || IsDraggedUnder(Party[partyIdx])) && partyIdx < PartySize;
			partyIdx ++
		);
	} else {
		partyIdx = SoloMember;
	}
	SavedNPCMode = NPCMode[Party[partyIdx]];
	NPCMode[Party[partyIdx]] = 2;
	mouseSave = CON_mouseOffAt(0x8e, 0xa3, 0xa0, 0xc3);
	GR_DRAW_BITMAP(&Screen, 0x93, 0xa9, D_BA7E, 0);
	CON_mouseOn(mouseSave);
	C_1FA9_032B();
}

/*the 0x17b swallows the body (KillBody) of the actor it killed: its
  contents fall where the actor died (KillX, KillY), but the TileFlag2 8
  ones which vanish; on the screen the 0x17b shows itself gulping (frames
  0x23 to 0x2f)*/
/*C_07D1_5DB2*/C_07D1_5DB2(int16 attacker) {
	int16 oldFrame, frame;
	int16 obj;

	if(KillBody >= 0) {
		CON_printf("Gulp!\n");
		MUS_PlaySfx(0x24);
		for(obj = FindInv(KillBody); obj >= 0; obj = NextInv()) {
			if(!(TileFlag2[TILE_FRAME(obj)] & 8))
				MoveObj(obj, KillX, KillY, MapZ);
			else
				DeleteObj(obj);
		}
		DeleteObj(KillBody);
		if(C_1B09_090B(attacker)) {
			oldFrame = ObjShapeType[attacker] >> 10;
			SetFrame(attacker, (oldFrame >> 4) % 4 * 8 + 3);
			ShowObjects();
			DisplayTick();
			TimerDelay(0xdbba0L);
			for(frame = 0x23; frame <= 0x30; frame += 4) {
				SetFrame(attacker, frame);
				ShowObjects();
				DisplayTick();
				TimerDelay(0x927c0L);
			}
			SetFrame(attacker, oldFrame);
			ShowObjects();
			DisplayTick();
		} else {
			VisibilityDirty = 1;
		}
	}
}

/*the death of a class with flag 0x800: an explosion (0x107) at (x, y)
  hurting everything it reaches by 1-15 points, the 0x17a (up to 5) last,
  if still alive*/
/*C_07D1_5F2B*/C_07D1_5F2B(int16 x, int16 y) {
	int16 damage, list[5], objTyp;
	int16 far *hits;
	int16 n, obj;

	damage = OSI_rand(1, 15);
	n = 0;
	MUS_PlaySfx(0xb);
	for(hits = Explosion(x, y, 0x107); *hits >= 0; hits ++) {
		obj = *hits;
		objTyp = GetType(obj);
		if(objTyp != 0x17a)
			LoseHP(obj, damage, 0);
		else if(n < 5)
			list[n ++] = obj;
	}
	while(n --) {
		if(!IsDead(list[n]))
			LoseHP(list[n], damage, 0);
	}
	VIS_UpdateView();
	DisplayTick();
}

/*the strike of the weapon 0x139 (instead of __TryHit; same results:
  2 out of range, 1 missed or done, 0 the dream ended): where its missile
  stops short of the target on the tiles 0x90/0x91/0x95/0x96 (without a
  0xb6 there), it leaves an object there (0xb6 of frame 0, 1, 2 or 3) and
  a 0xda beside it; with a line of fire (the tiles 0x90-0x97 not blocking:
  TileFlag 0x20 cleared meanwhile) the missile 0x10b hits whoever is
  there (__DoHit, 0xfe points); hitting NPC 0x66 ends the dream (a cut
  scene, TalkFlags[0x46] |= 8)*/
/*C_07D1_5FEF*/C_07D1_5FEF(int16 attacker, int16 x, int16 y, int16 weapObj) {
	int16 range, target, mx, my;
	unsigned char savedFlags[8];
	int16 ret, tx, ty, tile, noLine, sx, sy;
	register int16 i, head;

	ret = 1;
	range = GetWeaponRange(weapObj);
	if(C_07D1_35F5(x, y, attacker) > range)
		return 2;
	MissileHitObj = SelectionObj;
	if((head = COMBAT_getHead(MissileHitObj)) >= 0) {
		tx = GetX(head);
		ty = GetY(head);
	} else {
		tx = SelectionX;
		ty = SelectionY;
	}
	mx = GetX(attacker);
	my = GetY(attacker);
	sx = 0;
	sy = 0;
	if(tx < mx)
		sx = -1;
	else if(tx > mx)
		sx = 1;
	if(ty < my)
		sy = -1;
	else if(ty > my)
		sy = 1;
	target = head;
	mx = tx;
	my = ty;
	noLine = !COMBAT_Missile(attacker, &mx, &my, range, 0);
	if(OBJ_FindTypeAt(0xb6, mx, my) < 0) {
		tile = GetTileAtXYZ(mx, my, MapZ);
		i = 0;
		switch(tile) {
		case 0x90: i = 0x0b6; break;
		case 0x91: i = 0x4b6; break;
		case 0x95: i = 0x8b6; break;
		case 0x96: i = 0xcb6; break;
		}
		if(i) {
			MUS_PlaySfx(8);
			TimerDelay(0x8ba1L);
			MUS_PlaySfx(6);
			COMBAT_Missile(attacker, &mx, &my, range, 0x10b);
			MUS_PlaySfx(0xb);
			AddMapObj(i, 0, 0, mx, my, MapZ);
			if((i = AddMapObj(0xda, 0, 0, mx + sx, y + sy, MapZ)) > 0)
				C_1B09_268D(i);
			TimerDelay(0xba2cL);
			MUS_PlaySfx(0x1a);
			VIS_UpdateView();
			ShowObjects();
			TimerDelay(0x5744aL);
		}
	}
	if(!noLine)
		return 1;
	for(i = 0x90; i <= 0x97; i ++) {
		savedFlags[i - 0x90] = TileFlag[i];
		TileFlag[i] &= ~0x20;
	}
	MUS_PlaySfx(8);
	TimerDelay(0x8ba1L);
	MUS_PlaySfx(6);
	target = head;
	MissileHitObj = head;
	mx = tx;
	my = ty;
	COMBAT_Missile(attacker, &mx, &my, range, 0x10b);
	for(i = 0x90; i <= 0x97; i ++)
		TileFlag[i] = savedFlags[i - 0x90];
	target = COMBAT_getHead(MissileHitObj);
	if(MissileHitObj >= 0 && target != attacker) {
		MUS_PlaySfx(0xb);
		TimerDelay(0xba2cL);
		if(target == 0x66) {
			COMBAT_showHit(0x66);
			TimerDelay(0x122e4cL);
			C_2F75_0B90();
			DreamWakeUp();
			TalkFlags[0x46] |= 8;
			return 0;
		}
		ret = target != SelectionObj;
		__DoHit(attacker, target, 0xfe, 0);
	}
	return ret;
}

/*NPC 0x40 hit (LoseHP): after its talk it hurls the player (NPC 1, shown
  as a 0x1a8 for a missile 0x101 from NPC 0x40, then flying as
  0x18e/0x18a) at least 10 squares further south, where the player falls
  unconscious and dies (D_0704 set: no spray pending)*/
/*C_07D1_632C*/C_07D1_632C() {
	int16 x1, y1, x0, deltaX, x2, y2, savedType;
	int16 deltaY, y0;

	TalkFlags[0x40] |= 8;
	TALK_talkTo(1, 0x40, 0);
	DisplayTick();
	x0 = GetX(0x40);
	y0 = GetY(0x40);
	x1 = GetX(1);
	y1 = GetY(1);
	deltaX = x1 - x0;
	deltaY = y1 - y0;
	if(deltaY <= 0)
		deltaY = 1;
	x2 = x1 + deltaX;
	y2 = y1 + deltaY;
	while(y2 - y0 < 10) {
		x2 += deltaX;
		y2 += deltaY;
	}
	Missiles(x0, y0, 1, &x1, &y1, 9, 0x101, 0);
	savedType = ObjShapeType[1];
	ObjShapeType[1] = 0x1a8;
	ShowObjects();
	DisplayTick();
	Missiles(x1, y1, 1, &x2, &y2, 9, D_16A8 ? 0x18e : 0x18a, 0);
	ObjShapeType[1] = savedType;
	SetTypeUnconscious(1);
	MoveObj(1, x2, y2 - 1, MapZ);
	ShowObjects();
	DisplayTick();
	D_0704 = 1;
	ObjShapeType[1] = savedType;
	COMBAT_Kill(1);
}

