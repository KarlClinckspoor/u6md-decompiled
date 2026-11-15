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
extern int16 FindNonIgnoredObjAt(int16);
extern PrintTileArticle(int16);/*show a tile in the text window?*/
extern int16 COMBAT_getHead(int16);
extern int16 COMBAT_range(int16, int16, int16, int16);/*distance of (x1, y1) and (x2, y2)*/
extern DisplayTick();
extern UpdateTurnFrame(int16, int16);/*turns an NPC towards dir*/
extern SubMov(int16, int16);/*SubMov(npc, points)*/
extern char *GetTileString(int16);
extern char *GetObjectString(int16);
extern VIS_UpdateView();
extern unsigned char GetTileAtXYZ(int16, int16, int16);
extern int16 MapDistance(int16, int16);
extern MUS_PlaySfx(int16);/*play a sound effect?*/
extern DrawSlotCursor(int16);
extern int16 MkDirection(int16, int16);
extern int16 CON_getch();
extern CON_putch(int16);
extern cdecl CON_printf(char far *, ...);

extern char far OnMsg[];/*"On - "*/
extern char far NoEffectMsg[];/*"It has no effect.\n"*/
extern char far OutOfRangeMsg[];/*"Out of range!\n"*/
extern char far NothingMsg[];/*"nothing!\n"*/
extern char far CantBeUsedMsg[];/*"%s can't be used.\n"*/
extern char far HasNoEffectOnMsg[];/*"%s has no effect on "*/
extern char far HasNoEffectMsg[];/*"%s has no effect.\n"*/

extern unsigned char Active;
extern int16 MapZ;
extern struct { unsigned a:4, psychic:4, c:4, d:4; } Talents[];
extern unsigned char SelectMode;
extern char AimX, AimY, SelectRange;
extern int16 EndPagingAtPrompt;
extern int16 MouseMode;
extern struct { unsigned char originX, originY, cols, rows, curX, curY, fgColor, bgColor, attrFlags; } *CurClip;
extern unsigned char ObjStatus[];
extern unsigned char Party[];
extern unsigned char far *Level;
extern int16 far *BaseTile;
extern unsigned char far *TileFlag2;/*0x10 ignored*/
extern struct coord ObjPos[];
extern uint16 ObjShapeType[];
extern int16 PointerX, PointerY;
extern int16 SelectionObj;
extern int16 SelectionX, SelectionY;
/*the distance of the last target (GetUseTarget)*/
/*EC46*/extern int16 TargetDist;


#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))
#define IsTileIg(t) (TileFlag2[t] & 0x10)
#define GetCoordUse(i) (ObjStatus[i] & 0x18)
/*the active member's reach: its level when its first talent is set*/
#define REACH (Talents[Party[Active]].a ? Level[Party[Active]] : 0)

/*the tables, each a far variable (a paragraph-aligned segment of its own, after the code):
  UseObjects at 2E08, UseActions at 2E67, which points at the overlaid actions' stub entries*/
extern void UseBed(int16, int16, int16);
extern void UseRead(int16, int16, int16);
extern void UseTakeAndReady(int16, int16, int16);
extern void UseInstrument(int16, int16, int16);
extern void UseFastenSwitch(int16, int16, int16);
extern void UseInstallDrill(int16, int16, int16);
extern void UseFastenPanel(int16, int16, int16);
extern void UseFixRail(int16, int16, int16);
extern void UseReplaceCable(int16, int16, int16);
extern void UseWrongTool(int16, int16, int16);
extern void UseUprootPlant(int16, int16, int16);
extern void UseDigHole(int16, int16, int16);
extern void UseMoveDirt(int16, int16, int16);
extern void UseLoadDirt(int16, int16, int16);
extern void UseUnloadCart(int16, int16, int16);
extern void UseBury(int16, int16, int16);
extern void UseDirtOnBelt(int16, int16, int16);
extern void UseOnCrate(int16, int16, int16);
extern void UseOpenHatch(int16, int16, int16);
extern void UseBurn(int16, int16, int16);
extern void UseOpenPod(int16, int16, int16);
extern void UsePickBerries(int16, int16, int16);
extern void UseBargeController(int16, int16, int16);
extern void UseLever(int16, int16, int16);
extern void UseCannon(int16, int16, int16);
extern void UseEatBerries(int16, int16, int16);
extern void UseBattleFrenzy(int16, int16, int16);
extern void UseLightWithMatch(int16, int16, int16);
extern void UseLight(int16, int16, int16);
extern void UseRefuelLantern(int16, int16, int16);
extern void UseSwitch(int16, int16, int16);
extern void UseTelescope(int16, int16, int16);
extern void UseSextant(int16, int16, int16);
extern void UseChest(int16, int16, int16);
extern void UseBilliards(int16, int16, int16);
extern void UseOrgan(int16, int16, int16);
extern void UseChemicals(int16, int16, int16);
extern void UseInstallLenses(int16, int16, int16);
extern void UseDrill(int16, int16, int16);
extern void UseGunSetting(int16, int16, int16);
extern void UseLoadCannon(int16, int16, int16);
extern void UseBucket(int16, int16, int16);
extern void UseMap(int16, int16, int16);
extern void UseGate(int16, int16, int16);
extern void UseCabinetPanel(int16, int16, int16);
extern void UseFixRollers(int16, int16, int16);
extern void UseRadium(int16, int16, int16);
extern void UseChargeSprayGun(int16, int16, int16);
extern void UseSprayGun(int16, int16, int16);
extern void UseOxiumBin(int16, int16, int16);
extern void UseLoadSprayer(int16, int16, int16);
extern void UseInstallHeadgear(int16, int16, int16);
extern void UseDreamstuff(int16, int16, int16);
extern void UseDreamstuffHoard(int16, int16, int16);
extern void UseHealingElixir(int16, int16, int16);
extern void UseAutoDoor(int16, int16, int16);
extern void UseHarvestSeed(int16, int16, int16);
extern void UsePlantSeed(int16, int16, int16);
extern void UseOil(int16, int16, int16);
extern void UseFillHole(int16, int16, int16);
extern void UsePutOutLight(int16, int16, int16);
extern void UseInstallHeartstone(int16, int16, int16);
extern void UseSpittoon(int16, int16, int16);
extern void UseRubySlippers(int16, int16, int16);
extern void UseManuscript(int16, int16, int16);
extern void UseMirror(int16, int16, int16);
extern void UseGateB5(int16, int16, int16);
extern void UsePourBucket(int16, int16, int16);
extern void UseValve(int16, int16, int16);
extern void UseRadiumOnMechMan(int16, int16, int16);
extern void UseOxygenatedAir(int16, int16, int16);

/*C_2E08_0000*/struct tUseObject far UseObjects[] = {
	{0x11b, {57, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x1b9, {39, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x32, {20, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x33, {21, 22, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x1a2, {23, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x68, {34, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x2d, {40, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x1c7, {54, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x1c0, {61, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x57, {34, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x143, {64, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x94, {1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xeb, {37, 60, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x53, {34, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x130, {34, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x4d, {64, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xa0, {42, 71, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x4b, {26, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xc7, {64, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xb8, {64, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x7c, {30, 60, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x71, {28, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x6f, {28, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x14d, {25, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xde, {44, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x1bc, {64, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x46, {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x118, {57, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x120, {64, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x14b, {53, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x1b8, {64, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x11a, {57, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x79, {55, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x90, {46, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xf1, {40, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x98, {44, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xb5, {70, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xe7, {3, 21, 22, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x77, {48, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x1ab, {64, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x93, {69, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x1aa, {65, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xf0, {40, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x43, {9, 10, 11, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x128, {52, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x1bb, {64, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x36, {20, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x75, {28, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x72, {29, 19, 63, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x70, {29, 19, 63, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x76, {29, 19, 63, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x74, {29, 19, 63, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x6e, {29, 19, 63, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xc4, {24, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x110, {17, 18, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x125, {64, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x11c, {34, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x111, {21, 22, 17, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x107, {9, 10, 11, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x108, {20, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x10b, {9, 10, 11, 12, 13, 14, 15, 16, 62, 0xff}},
	{0x147, {9, 10, 11, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x109, {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xf2, {20, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x10a, {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x105, {49, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x37, {20, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xe3, {56, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x58, {68, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x96, {43, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x6b, {29, 19, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x97, {1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x55, {34, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x73, {28, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x83, {64, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xd4, {50, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x144, {74, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x4a, {26, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x1ca, {45, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x41, {9, 10, 11, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xa5, {36, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x45, {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x35, {8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x191, {2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x18f, {35, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x66, {17, 18, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x1c1, {47, 73, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x44, {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x7b, {51, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x1ba, {64, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x4e, {64, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x14c, {51, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x26, {2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xc, {67, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x48, {20, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xf3, {1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x47, {20, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x60, {33, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x199, {36, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x1a5, {44, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x42, {9, 10, 11, 12, 13, 14, 15, 16, 62, 0xff}},
	{0x34, {7, 21, 22, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xb3, {56, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x4c, {26, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x11e, {66, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x5e, {32, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x8a, {41, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x137, {24, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x138, {24, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x19b, {31, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x6a, {0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x49, {26, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x119, {57, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xce, {38, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x6c, {29, 19, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x88, {2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x6d, {28, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x13a, {38, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0xdc, {72, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x81, {49, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x80, {48, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x56, {34, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x9d, {58, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x9e, {59, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{0x87, {4, 5, 6, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{-1, {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}}
};
/*C_2E67_0000*/struct tUseAction far UseActions[] = {
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseBed},/*0*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseRead},/*1*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseTakeAndReady},/*2*/
	{{0x12a, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseInstrument},/*3*/
	{{0x19b, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseFastenSwitch},/*4*/
	{{0x1b8, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseInstallDrill},/*5*/
	{{0x1ca, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseFastenPanel},/*6*/
	{{0x194, -1, -1, -1, -1, -1, -1, -1}, {0x195, -1, -1, -1, -1, -1, -1, -1}, UseFixRail},/*7*/
	{{0xc7, -1, -1, -1, -1, -1, -1, -1}, {0xc9, 0xd7, -1, -1, -1, -1, -1, -1}, UseReplaceCable},/*8*/
	{{0x101, 0xff, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseWrongTool},/*9*/
	{{0xfb, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseUprootPlant},/*10*/
	{{-2, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseDigHole},/*11*/
	{{0x1ba, 0x1bb, 0x1bc, -1, -1, -1, -1, 0}, {-2, -1, -1, -1, -1, -1, -1, -1}, UseMoveDirt},/*12*/
	{{0x102, 0x1ba, 0x1bb, 0x1bc, -1, -1, -1, -1}, {0x10c, 0x19a, -1, -1, -1, -1, -1, -1}, UseLoadDirt},/*13*/
	{{0x10c, 0x19a, -1, -1, -1, -1, -1, -1}, {-2, -1, -1, -1, -1, -1, -1, -1}, UseUnloadCart},/*14*/
	{{0x102, -1, -1, -1, -1, -1, -1, -1}, {-2, -1, -1, -1, -1, -1, -1, -1}, UseBury},/*15*/
	{{0x1bc, -1, -1, -1, -1, -1, -1, -1}, {0xe9, -1, -1, -1, -1, -1, -1, -1}, UseDirtOnBelt},/*16*/
	{{0x56, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseOnCrate},/*17*/
	{{0x1ab, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseOpenHatch},/*18*/
	{{-2, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseBurn},/*19*/
	{{0xfb, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseOpenPod},/*20*/
	{{0x82, -1, -1, -1, -1, -1, -1, 0}, {-1, -1, -1, -1, -1, -1, -1, -1}, UsePickBerries},/*21*/
	{{0x53, 0x55, 0x57, 0x130, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseOnCrate},/*22*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseBargeController},/*23*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseLever},/*24*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseCannon},/*25*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseEatBerries},/*26*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseBattleFrenzy},/*27*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseLightWithMatch},/*28*/
	{{0x6f, 0x71, 0x75, 0x73, 0x6d, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseLight},/*29*/
	{{0x75, 0x76, 0x73, 0x74, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseRefuelLantern},/*30*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseSwitch},/*31*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseTelescope},/*32*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseSextant},/*33*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseChest},/*34*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseBilliards},/*35*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseOrgan},/*36*/
	{{0xfb, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseChemicals},/*37*/
	{{0xca, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseInstallLenses},/*38*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseDrill},/*39*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseGunSetting},/*40*/
	{{0x14d, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseLoadCannon},/*41*/
	{{-2, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseBucket},/*42*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseMap},/*43*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseGate},/*44*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseCabinetPanel},/*45*/
	{{0xc0, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseFixRollers},/*46*/
	{{0xf0, 0xf1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseRadium},/*47*/
	{{0x81, 0x105, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseChargeSprayGun},/*48*/
	{{-2, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseSprayGun},/*49*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseOxiumBin},/*50*/
	{{0x114, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseLoadSprayer},/*51*/
	{{0x120, 0x121, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseInstallHeadgear},/*52*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseDreamstuff},/*53*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseDreamstuffHoard},/*54*/
	{{-2, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseHealingElixir},/*55*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseAutoDoor},/*56*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseInstrument},/*57*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseHarvestSeed},/*58*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UsePlantSeed},/*59*/
	{{-2, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseOil},/*60*/
	{{0x122, 0x1c0, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseRadium},/*61*/
	{{0x102, -1, -1, -1, -1, -1, -1, -1}, {0x101, -1, -1, -1, -1, -1, -1, -1}, UseFillHole},/*62*/
	{{0x76, 0x70, 0x72, 0x74, 0x6e, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UsePutOutLight},/*63*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseWrongTool},/*64*/
	{{0x11f, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseInstallHeartstone},/*65*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseSpittoon},/*66*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseRubySlippers},/*67*/
	{{0x54, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseManuscript},/*68*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseMirror},/*69*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseGateB5},/*70*/
	{{0xa0, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UsePourBucket},/*71*/
	{{-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseValve},/*72*/
	{{0x166, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseRadiumOnMechMan},/*73*/
	{{-2, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, UseOxygenatedAir}/*74*/
};

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
			MUS_PlaySfx(5);
			return -2;
		}
		TargetDist = COMBAT_range(SelectionX, SelectionY, x, y);
	} else {
		CON_printf(GetObjectString(SelectionObj));
		CON_putch('\n');
		if(GetCoordUse(SelectionObj) == 0)
			TargetDist = COMBAT_range(SelectionX, SelectionY, x, y);
		else
			TargetDist = 1;
	}
	if(REACH + 1 < TargetDist && (MapDistance(SelectionX, x) > 1 || MapDistance(SelectionY, y) > 1)) {
		CON_printf(OutOfRangeMsg);
		MUS_PlaySfx(5);
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
		CON_printf(CantBeUsedMsg, GetObjectString(objNum));
		return;
	}
	if(WantsTarget(1, list)) {
		on = PromptUseTarget(OnMsg);
		if(on == -2)
			return;
		onType = GetType(on);
		if(!FilterActions(onType, 1, list)) {
			CON_printf("\n");
			PrintTileArticle(TILE_FRAME(objNum));
			CON_printf(HasNoEffectOnMsg, GetObjectString(objNum));
			PrintTileArticle(TILE_FRAME(on));
			CON_printf("%s.\n", GetObjectString(on));
			return;
		}
		if(TargetDist > 1)
			MUS_PlaySfx(0x32);
		if(WantsTarget(2, list)) {
			to = PromptUseTarget("To -\n");
			if(to == -2)
				return;
			toType = GetType(to);
			if(!FilterActions(toType, 2, list)) {
				CON_printf("\n");
				PrintTileArticle(TILE_FRAME(objNum));
				CON_printf("%s on ", GetObjectString(objNum));
				PrintTileArticle(TILE_FRAME(on));
				CON_printf("%s to ", GetObjectString(on));
				PrintTileArticle(TILE_FRAME(to));
				CON_printf(HasNoEffectMsg, GetObjectString(to));
				return;
			}
			if(TargetDist > 1)
				MUS_PlaySfx(0x32);
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
		dist = COMBAT_range(x, y, SelectionX, SelectionY);
		if(REACH + 1 < dist) {
			CON_printf(OutOfRangeMsg);
			return;
		}
		if(dist > 1)
			MUS_PlaySfx(0x32);
	}
	if(GetCoordUse(SelectionObj) == 0) {
		dir = MkDirection(PointerX, PointerY);
		if(dir != -1) {
			UpdateTurnFrame(Party[Active], dir);
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
