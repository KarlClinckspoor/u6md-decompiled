/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE segment 120E: game loop and world clock
 * (Savage Empire's gameloop.c, Ultima VI's, reworked).
 *
 * GameLoop is the main command loop (called once from main.c): it reads a
 * key, turns letter keys into commands, runs the command, then the end of
 * turn. PassTime is the world clock. AnimateTiles / DrawMapWindow /
 * DisplayTick are the per frame display update. TerrainDamage (new: the
 * martian hazards, bombs and rays of object 0x1cf) / ApplyTerrainEffects
 * apply the effects of what a creature stands on (TileDamage, for map
 * tiles, is empty). GrowPlant / ShrinkWormsbane are PassTime's plants. SetActive / SetPartyMode
 * switch solo mode / party mode. From SE: DrawMapWindow, DisplayTick,
 * RedrawMapWithObjects, FlushViewIfDirty, ApplyTerrainEffects, PassTime's frame,
 * RefreshStatus, SetActive, SetPartyMode, ReadHexNumber, ReadDecNumber,
 * GameLoop's frame.
 */
#include "se.h"

#undef abs/*abs() is the library function here, not the inline __abs__*/

/*a drawing surface of the mouse code (Ultima VI's)*/
struct tSurface {
	uint16 segment;/*_00*/
	uint16 far *pScanlines;/*_02*/
	uint16 right, left, top, bottom;/*_06,_08,_0a,_0c*/
};

/*the mouse state (Ultima VI's)*/
struct tMouse {
	unsigned char buttonL, buttonR;
	int16 posX, posY;
	uint16 unused06;
	void far *shapes;/*_08 pointer shapes*/
	uint16 shapeIndex;
	struct tSurface *surface;/*_0e*/
	void far *saveBuf;/*_10 background save area*/
	void (far *onMove)();/*_14*/
	unsigned char installed;
};

/*the graphic driver's state (Ultima VI's tGr)*/
struct tGr {
	unsigned char header[6];/*_00*/
	void far *font;/*_06*/
	unsigned char textFg, textBg;
	unsigned char xorMode;
	unsigned char unknown0d;
	unsigned char animOn;
	unsigned char toBackBuffer;
};

/*the graphic driver: GrDriver's offset selects the entry*/
#define GR_SET_COLOR(c)               (GrDriver.iii.ofs = 0x06, (*GrDriver.f)(c))
#define GR_PUT_PIXEL(x,y)             (GrDriver.iii.ofs = 0x09, (*GrDriver.f)(x,y))
#define GR_PUT_TILE(tile,x,y)         (GrDriver.iii.ofs = 0x42, (*GrDriver.f)(tile,x,y))
#define GR_COPY_TO_SCREEN(a,b,c,d,e,g) (GrDriver.iii.ofs = 0x45, (*GrDriver.f)(a,b,c,d,e,g))
#define GR_FADE_TO_SCREEN(a,b,c,d,e,g) (GrDriver.iii.ofs = 0x48, (*GrDriver.f)(a,b,c,d,e,g))
#define GR_SET_ANIM_FRAME(a,b)        (GrDriver.iii.ofs = 0x4b, (*GrDriver.f)(a,b))

/*OSI library (asm)*/
extern int16 OSI_rand(int16, int16);/*OSI_rand(lo, hi)*/
extern cdecl MOUSE_SERVE();
extern cdecl MOUSE_DRAW();
extern cdecl MOUSE_SAVE_BG();
extern int16 cdecl MOUSE_IN_AREA(struct tSurface *);

/*far data*/
extern int16 far MonsterFlags[];/*by GetMonsterClass*/

/*other modules*/
extern int16 IsInvulnerable(int16);
extern int16 MaxHP(int16);
extern LoseHP(int16, int16, int16);/*(npc, damage, kind)*/
extern Missiles(int16, int16, int16, int16 *, int16 *, int16, int16, int16);
extern Explosion(int16, int16, int16);
extern VIS_UpdateView();
extern DrawSlotCursor(int16);
extern cdecl CON_printf(char far *, ...);
extern unsigned char GetTileAtXYZ(int16, int16, int16);
extern int16 GetMonsterClass(int16);
extern int16 NextLoc();
extern int16 FindLoc(int16, int16, int16);/*FindLoc(x, y, z)*/
extern ShowObjects();
extern MUS_PlaySfx(int16);
extern ShakeScreen(int16, int16);
extern BargeClear();
extern BargeAddController(int16);
extern BargeMoveAll();
extern UpdateJet(int16);
extern DrawSkyBar();
extern MAP_LoadAround();
extern CON_gets(char *, int16);
extern MUS_PlayAreaSong();
extern STAT_ShowParty(int16);
extern STAT_ShowPortrait(int16);
extern STAT_ShowInventory(int16);
extern char *TALK_GetCharName(int16);
extern ShowPortrait(int16);
extern PrintTileArticle(int16);/*shows a tile in the text window*/
extern COMBAT_stopCharm(int16);
extern int16 GetStr(int16);/*an NPC's strength*/
extern int16 GetInt(int16);/*an NPC's intelligence*/
extern int16 GetWarmth(int16);/*of a worn type*/
extern int16 IsLitLightType(int16);
extern int16 IsOutOfArea(int16);
extern UpdateSchedules();
extern int16 OBJ_IsLocLoaded(int16, int16, int16);/*OBJ_IsLocLoaded(x, y, z)*/
extern int16 FindInv(int16);
extern int16 NextInv();
extern int16 FindInvOfType(int16, int16);/*first object of a type in an inventory*/
extern int16 NextInvOfType();/*next one*/
extern char *GetObjectString(int16);
extern char far PsychicPowerFadesMsg[];
extern char far IceMeltedMsg[];
extern char far PoisonedMsg[];
extern int16 WrapDelta(int16);
extern int16 MapDistance(int16, int16);/*wrapping*/
extern InitNPCTracker();
extern PartyAdvance(int16);
extern MoveFollowers(int16, int16);
extern PTH_SearchPaths();
extern RunNPCTurns();
extern COMBAT_begin();
extern COMBAT_breakOff();
extern COMBAT_attack();
extern int16 COMBAT_Missile(int16, int16 *, int16 *, int16, int16);
extern CmdLook();
extern CmdDrop();
extern CmdGet();
extern CmdMove();
extern CmdUse();
extern TALK_talkTo(int16, int16, int16);
extern CON_mouseOffAt(int16, int16, int16, int16);
extern CON_mouseOn(int16);
extern far CON_UpdateMousePointer();
extern int16 CON_getch();
extern CON_EchoChar(int16);
extern GAME_Save();
extern GAME_Restore();
extern BargeFindLeader(int16);
extern BargeDump();
extern BeltUpdate(int16);
extern PeerMap();
extern PartyTeleport(int16, int16, int16, int16);
extern MUS_PlaySong(int16);
extern MUS_CombatOutcomeSong();
extern MUS_StopSfx();
extern MUS_PlayLoopSfx(int16);
extern MUS_SweepSfx();
extern MUS_MachinerySfx();
extern MUS_SplashSfx();
extern int16 QuanType(int16);
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);/*AddMapObj(shape, status, amount, x, y, z)*/
extern int16 AddInvObj(int16, int16, int16, int16);/*AddInvObj(shape, status, amount, owner)*/
extern C_2F75_02E1();
extern C_2F75_042D();
extern C_2F75_0532();
extern C_2F75_0757();
extern C_2F75_07AA();
extern C_2F75_07FA();
extern C_2F75_0951();
extern C_2F75_0A64();
extern C_2F75_0B90();
extern char far PassMsg[];/*pass*/
extern char far CombatMsg[];/*combat*/
extern char far BreakOffMsg[];/*break off*/
extern char far SoundOnMsg[];/*sound on*/
extern char far SoundOffMsg[];/*sound off*/
extern char far MusicOffMsg[];/*music off*/
extern char far MusicOnMsg[];/*music on*/
extern char far ExitToDOSMsg[];/*exit to DOS?*/
extern char far SaveMsg[];/*save game?*/
extern char far SoloModeMsg[];/*solo mode*/
extern char far PartyModeMsg[];/*party mode*/
extern char far NotAllHereMsg[];/*not all here*/
extern char far NotInCombatMsg[];/*not in combat*/
extern char far NotInSoloMsg[];/*not in solo mode*/
extern char far NotWhileDreamingMsg[];/*not on the barge*/
extern char far WhatMsg[];/*what?*/
extern char far FailsToRespondMsg[];/*fails to respond*/
extern char far KilledHelperMsg[];
extern char far HistoricalFigureDeadMsg[];
extern char far NoEmsMsg[];/*no EMS for the music*/
extern char far VersionMsg[];/*version*/
extern int16 OBJ_FindTypeAt(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 FindInvType(int16, int16, int16);/*FindInvType(owner, type, quality)*/
extern DeleteObj(int16);
extern char far AarghMsg[];/*failure message*/

/*variables of the other modules*/
extern unsigned char MouseOn;
extern unsigned char SelectMode;
extern char AimX, AimY;
extern char SelectRange;
extern char FreeAimMode;
extern int16 MouseMode;
extern int16 AllowMouseMov;
extern int16 LastAttackTarget;
extern char SlotCursorCol, SlotCursorRow;
extern int16 HelpOn;
extern int16 PendingCommand;
extern int16 DiskFullChoice;
extern int16 cdecl soundCard;
extern int16 MusicAdapter;
extern char LeaderOnBarge;
extern int16 CombatOutcome;
extern int16 HatchingDisabled;
extern int16 AllowNPCTeleport;
extern int16 SoundFlag;
extern int16 AvatarNotSolid;
extern unsigned char DefaultCommand;
extern int16 cdecl emsInUse;/*EMSINIT.ASM*/
extern int16 GrMemoryKB;
extern unsigned char far *D_3262;/*talk flags 8-15*/
extern char MovePts[];
extern unsigned char far *Level;
extern void far *ScreenParts;/*mdscreen.lzc entry 2: frames 0/1 mark/unmark a command icon*/
extern int16 AdvanceDir;
extern int16 SelectionObj;
extern int16 SelectionX, SelectionY;
extern int16 ScreenFade;
extern int16 VisibilityDirty;
extern unsigned char NextSleep;
extern unsigned char Time_M;
extern unsigned char Time_H;
extern unsigned char Date_D;
extern unsigned char Date_M;
extern int16 Date_Y;
extern unsigned char Active;
extern char AmbientLight;
extern unsigned char DrunkCounter;
extern char InCombat;
extern char SoloMember;
extern unsigned char SavedNPCMode;
extern struct { unsigned a:4, b:4, c:4, d:4; } Talents[];
extern int16 StatusMember;
extern int16 StatusDisplay;
extern unsigned char IsResting;
extern unsigned char ObjStatus[];
extern unsigned char far *NPCMode;
extern unsigned char Party[];
extern char Names[][14];
extern unsigned char far *TileFlag;
extern unsigned char TalkFlags[];
extern unsigned char PartySize;
extern uint16 far *OrigShapeType;
/*EC4A*/extern int16 CharmedNPC;/*an NPC charmed (never set in Martian Dreams): out of combat it turns hostile again (mode 0x1b)*/
/*BC84*/extern unsigned char HexInputBuf[5];
/*BC89*/extern unsigned char DecInputBuf[6];
extern int32 GameFlags;/*0x10: in a dream*/
extern int16 MapX, MapY, MapZ;
extern struct tSurface Screen;
extern unsigned char AnimFrameMask[0x20];
extern struct tMouse Mouse;
extern unsigned char far *NPCFlag;
extern int16 Amount[];
extern unsigned char NPCStatus[];
extern unsigned char HitPoints[];
extern int16 far *BaseTile;
extern unsigned char far *TileFlag2;
extern int16 AnimFirstFrame[0x20];
extern struct coord ObjPos[];
extern struct tSurface MapSurface;
extern int16 NumAnimData;
extern uint16 ObjShapeType[];
extern unsigned char far *TerrainType;
extern int16 AnimTile[0x20];
extern unsigned char AnimFrameShift[0x20];
extern union tHandler GrDriver;
/*BC8E*/extern unsigned char StateAnimData[0x20];/*animation of each animated tile: 1 normal, 2 reverse, 0 still*/
extern unsigned char Obj_11x11[11][11];
extern int16 CellObjTile[];
extern unsigned char CellObjNext[];
extern int16 Tile_11x11[11][11];
extern struct tSurface MouseArea;

#define IN_VEHICLE (Active == PartySize)
#define GR_DRAW_BITMAP(s,x,y,p,m)    (GrDriver.iii.ofs = 0x30, (*GrDriver.f)(s,x,y,p,m))
#define GetOwner(i) (*(int16 *)&(ObjPos[i].raw[0]))

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])
#define SetFrame(i, v) (ObjShapeType[i] = GetType(i) + ((v) << 10))
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))

/*0F10*/int16 AnimFrameCounter = 0;/*counts the animation frames (AnimateTiles)*/
/*0F12*/unsigned char StatusDirty = 1;/*the status area must be redrawn*/
/*0F13*/unsigned char PromptFlag = 0;/*the prompt must be printed again*/
/*0F14*/unsigned char CyclopsFlag = 0;/*Ultima VI's; not used*/
/*0F15*/unsigned char IsArmageddon = 0;/*Ultima VI's name: 1 the game cannot be won any more, 2 (combat.c); no saving then*/
/*0F16*/int16 IdleAnimTick = 0;/*cleared by AnimateTiles*/
/*0F18*/int16 MapAnimEnabled = 1;/*DisplayTick animates and redraws the map window*/
/*0F1A*/int16 Erupting = 0;/*SE's name (object.c clears it)*/
/*wall flag (TerrainType) per direction N,NE,E,SE,S,SW,W (Ultima VI's; not used here)*/
/*0F1C*/unsigned char DirWallFlags[] = {0x80,0,0x40,0,0x20,0,0x10};


/*animate the tiles*/
/*C_120E_0009*/AnimateTiles() {
	int16 i, frm;
	int16 chance;

	chance = 0;
	AnimFrameCounter ++;
	for(i = 0; i < NumAnimData; i++) {
		frm = AnimFirstFrame[i];
		if(StateAnimData[i] == 1/*normal*/)
			frm += (AnimFrameCounter & AnimFrameMask[i]) >> AnimFrameShift[i];
		else if(StateAnimData[i] == 2/*reverse*/)
			frm += (~AnimFrameCounter & AnimFrameMask[i]) >> AnimFrameShift[i];
		GR_SET_ANIM_FRAME(AnimTile[i], frm);
	}
	IdleAnimTick = 0;
}

/*draw the 11x11 map window into the back buffer (tiles + objects).
If showOnScreen: also draw the target cursor and the mouse pointer, and
copy (or fade in) the window to the screen.*/
/*C_120E_0084*/DrawMapWindow(int16 showOnScreen/*0:offScreen,1:onScreen*/) {
	int16 i, j;
	int16 cellObj, scr_x, scr_y, tile, mouseInMap;

	for(j = 0; j < 11; j++) {
		for(i = 0; i < 11; i++) {
			scr_x = TIL2SCR(i);
			scr_y = TIL2SCR(j);
			if(MouseOn) MOUSE_SERVE();
			/*tiles*/
			GR_PUT_TILE(Tile_11x11[j][i], scr_x, scr_y);
			/*objects: linked list of tiles to draw in this cell*/
			for(cellObj = Obj_11x11[j][i]; cellObj; cellObj = CellObjNext[cellObj]) {
				tile = CellObjTile[cellObj];
				GR_PUT_TILE(tile, scr_x, scr_y);
			}
		}
	}
	if(!showOnScreen)
		return;
	if(MouseMode == 0 && !FreeAimMode) {
		DrawSlotCursor(0);
		SelectMode = 0;
	}
	if(SelectMode == 1) {
		/*target cursor*/
		if(SelectRange < 0)
			tile = 0x108;/*direction*/
		else
			tile = 0x109;/*select*/
		GR_PUT_TILE(tile, AimX * 16, AimY * 16);
	}
	/*mouse pointer over the map: draw it in the back buffer too*/
	if(MouseOn) {
		MouseArea.left = 8;
		MouseArea.right = 167;
		MouseArea.top = 16;
		MouseArea.bottom = 159;
		mouseInMap = MOUSE_IN_AREA(&MouseArea);
		if(mouseInMap) {
			Mouse.surface = &MapSurface;
			MOUSE_SAVE_BG();
			MOUSE_DRAW();
			Mouse.surface = &Screen;
		}
	}
	/*display the map part*/
	if(ScreenFade) {
		GR_FADE_TO_SCREEN(8, 16, 167, 159, 8, 16);
		ScreenFade = 0;
	} else {
		GR_COPY_TO_SCREEN(8, 16, 167, 159, 8, 16);
	}
}

/*one display "tick": recompute the view if needed, animate, redraw the map
window*/
/*C_120E_0221*/DisplayTick() {
	if(VisibilityDirty) {
		VIS_UpdateView();
		VisibilityDirty = 0;
	}
	if(MapAnimEnabled) {
		AnimateTiles();
		DrawMapWindow(1);
	}
}

/*redraw the map window, with the objects rebuilt; fade it in if mode is 1*/
/*C_120E_024B*/RedrawMapWithObjects(int16 mode) {
	DrawMapWindow(1);
	if(mode == 1) {
		ShowObjects();
		ScreenFade = 1;
	} else {
		ShowObjects();
		ScreenFade = 0;
	}
	DrawMapWindow(1);
}

/*if the view is dirty, recompute it and redisplay the map now*/
/*C_120E_0280*/FlushViewIfDirty() {
	if(VisibilityDirty) {
		VIS_UpdateView();
		VisibilityDirty = 0;
		DisplayTick();
	}
}

/*the effect of a map tile (nothing in Martian Dreams)*/
/*C_120E_029B*/TileDamage(int16 npc, int16 tile) {
}

/*the effect of object obj on creature npc (it stands on it)*/
/*C_120E_02A2*/TerrainDamage(register int16 npc, int16 obj) {
	int16 objTyp, objCls, kind;
	int16 a, b;
	int16 dmg;

	dmg = 0;
	kind = 0;
	NPCFlag[npc] |= 8;/*terrain handled for this turn*/
	if(IsInvulnerable(npc))
		return;
	if(!(NPCStatus[npc] & 0x10)) {/*not dead*/
	objCls = GetMonsterClass(GetType(npc));
	switch(objTyp = GetType(obj)) {
		case 0xd1:
			kind = 3;
			if(npc != 6)
				dmg = OSI_rand(HitPoints[npc] / 2, MaxHP(npc) / 2);
			else
				dmg = 0;
		break;
		case 0x17d:
			dmg = OSI_rand(1, 10) + OSI_rand(1, 10);
		break;
		case 0xd7:
			MUS_PlaySfx(0x15);
			dmg = OSI_rand(1, 20) + OSI_rand(1, 20);
		break;
		case 0xa8:
			dmg = OSI_rand(1, 20);
		break;
		case 0x1cf:
			a = GetQual(obj);
			if(a == 0)
				a = OSI_rand(1, 8);
			if(a == 0x14) {
				CON_printf("It's hot here!\n");
				dmg = OSI_rand(1, 20);
				PromptFlag = 1;
				break;
			}
			if(HitPoints[0] < 2)
				break;
			switch(a) {
				case 1:
					MUS_PlaySfx(9);
					CON_printf("Fire!\n");
					kind = 3;
				break;
				case 2:
					MUS_PlaySfx(10);
					CON_printf("Frost!\n");
					kind = 1;
				break;
				case 3:
					CON_printf("Fire bomb!\n");
					MUS_PlaySfx(0xb);
					kind = 3;
					Explosion(MapX + OSI_rand(0, 2) - 1, MapY + OSI_rand(0, 1) - 1, 0x107);
				break;
				case 4:
					CON_printf("Fragmentation bomb!\n");
					MUS_PlaySfx(0xb);
					Explosion(MapX + OSI_rand(0, 2) - 1, MapY + OSI_rand(0, 1) - 1, 0x106);
				break;
				case 5:
					CON_printf("Steam jet!\n");
					MUS_PlaySfx(0x19);
					kind = 3;
					a = MapX;
					b = MapY;
					Missiles(MapX + OSI_rand(0, 2) + OSI_rand(0, 2) - 2, MapY + OSI_rand(0, 4) - 5, 1, &a, &b, 8, 0x104, 0);
				break;
				case 6:
					CON_printf("Heat Ray!\n");
					MUS_PlaySfx(10);
					kind = 3;
					a = MapX;
					b = MapY;
					Missiles(MapX + OSI_rand(0, 2) + OSI_rand(0, 2) - 2, MapY + OSI_rand(0, 4) - 5, 1, &a, &b, 8, 0x16b, 0);
				break;
				case 7:
					CON_printf("Freeze Ray!\n");
					MUS_PlaySfx(10);
					a = MapX;
					b = MapY;
					Missiles(MapX + OSI_rand(0, 2) + OSI_rand(0, 2) - 2, MapY + OSI_rand(0, 4) - 5, 1, &a, &b, 8, 0x16a, 0);
					kind = 1;
				break;
				case 8:
					CON_printf("Acid!\n");
					MUS_PlaySfx(0x1a);
				break;
			}
			PromptFlag = 1;
			dmg = OSI_rand(1, HitPoints[npc] - 2);
		break;
	}
	if(dmg)
		LoseHP(npc, dmg, kind);
	}
}

/*check whether creature objNum stands at (x, y) on a damaging object or
map tile and apply it. Flying ethereal monsters, creatures already handled
this turn, on another level or outside the loaded 40x40 area are skipped.*/
/*C_120E_06A0*/ApplyTerrainEffects(int16 objNum, int16 x, int16 y) {
	register int16 otherObj;
	register int16 tileNum;
	int16 area_x, area_y, wrapMask, objTyp, objCls;

	objTyp = GetType(objNum);
	objCls = GetMonsterClass(objTyp);
	if(objCls >= 0 && (MonsterFlags[objCls] & 0x100) && (MonsterFlags[objCls] & 0x400))
		return;
	if(!(NPCFlag[objNum] & 8) && GetZ(objNum) == MapZ) {
		/*coordinates wrap at 1024 on the surface, 256 underground*/
		if(MapZ)
			wrapMask = 0xff;
		else
			wrapMask = 0x3ff;
		area_x = (MapX - 0x10) & 0x3f8;
		area_y = (MapY - 0x10) & 0x3f8;
		if((((x - area_x) & wrapMask) & 0x3ff) < 40 && (((y - area_y) & wrapMask) & 0x3ff) < 40) {
			for(otherObj = FindLoc(x, y, MapZ); otherObj >= 0; otherObj = NextLoc()) {
				if(otherObj == objNum)
					continue;
				tileNum = TILE_FRAME(otherObj);
				if(TileFlag2[tileNum] & 4)
					return;
				if(TerrainType[tileNum] & 8) {
					TerrainDamage(objNum, otherObj);
					return;
				}
			}
			tileNum = GetTileAtXYZ(x, y, MapZ);
			if(TerrainType[tileNum] & 8)
				TileDamage(objNum, tileNum);
		}
	}
}

/*a growing plant obj (quality 0xf, frames 0-2) grows one stage when the date
kept by the object 0xfc in the container 0x101 under it is old enough (in
the dream: at random)*/
/*C_120E_0800*/GrowPlant(register int16 obj) {
	register int16 inv;
	int16 cont, qual, day, month, frm;

	inv = -1;
	qual = GetQual(obj);
	if(qual == 0xf && GetFrame(obj) < 3) {
		if((cont = OBJ_FindTypeAt(0x101, GetX(obj), GetY(obj))) < 0) {
			CON_printf(AarghMsg);
			return;
		}
		if((inv = FindInvType(cont, 0xfc, -1)) < 0) {
			CON_printf(AarghMsg);
			return;
		}
		day = GetQual(inv);
		month = GetQuan(inv);
		if(GameFlags & 0x10) {
			month += OSI_rand(1, 2);
			if(month > 12) {
				frm = GetFrame(obj) + 1;
				SetFrame(obj, frm);
				VisibilityDirty = 1;
				if(frm >= 3)
					GetQual(obj) = 0x10;
				GetQuan(inv) = 0;
			} else {
				GetQuan(inv) = month;
			}
		} else if(month && day) {
			if(Date_M * 28 + Date_D > month * 28 + day + 3 || Date_M < month) {
				frm = GetFrame(obj) + 1;
				SetFrame(obj, frm);
				if(frm >= 3)
					GetQual(obj) = 0x10;
				Amount[inv] = (Date_D << 8) | Date_M;
			}
		}
	}
}

/*an object that shrinks: its quality counts down, then its frame (back to
quality 10-15), then it is deleted*/
/*C_120E_09C7*/ShrinkWormsbane(register int16 obj) {
	int16 q;
	register int16 frm;

	if((q = GetQual(obj) - 1) != 0) {
		GetQual(obj) = q;
	} else if((frm = GetFrame(obj)) != 0) {
		SetFrame(obj, frm - 1);
		GetQual(obj) = OSI_rand(10, 15);
		VisibilityDirty = 1;
	} else {
		DeleteObj(obj);
	}
}

/*wind direction names (Ultima VI's; not used here)*/
/*0F23*/char *WindDirNames[] = {
	"C ",
	"N ",
	"NE",
	"E ",
	"SE",
	"S ",
	"SW",
	"W ",
	"NW"
};
/*0F35*/unsigned char LastHourUpdate = -1;/*hour of the last hourly update (-1: none yet)*/

/*world clock: advance the game time by "minutes" (0 = just refresh the
time-dependent state) and process everything that depends on it*/
/*C_120E_0A47*/PassTime(int16 minutes) {
	int16 newMinutes, oldLight, inv, torchLit, objTyp, unused, warmth, cold, light;
	register int16 i, j;

	if((TalkFlags[0x46] & 8) && OSI_rand(0, 4) == 0)
		ShakeScreen(OSI_rand(4, 20), 1);
	BargeClear();
	if(minutes > 0) {
		for(i = 0x101; i < 0xc00; i++) {
			if(!IsOutOfArea(i)) {
				switch(GetType(i)) {
					case 0x1a2: BargeAddController(i); break;
					case 0x1d0: UpdateJet(i); break;
					case 0xfb: GrowPlant(i); break;
					case 0x9d: ShrinkWormsbane(i); break;
				}
			}
		}
		BargeMoveAll();
		FlushViewIfDirty();
	}
	torchLit = unused = 0;
	for(i = 0; i < 0x100; i++) {
		if(ObjShapeType[i] == 0)
			continue;
		/*-- talents at most 10 --*/
		if(i <= 0xf) {
			if(Talents[i].a > 10)
				Talents[i].a = 10;
			if(Talents[i].b > 10)
				Talents[i].b = 10;
			if(Talents[i].c > 10)
				Talents[i].c = 10;
		}
		if(!(GameFlags & 0x10) && (NPCStatus[i] & 0x80) && !(NPCStatus[i] & 0x10)) {
			/*-- party members: air, lights, cold --*/
			if(i == 0)
				continue;
			warmth = 0;
			if(minutes && !(NPCStatus[i] & 4) && !IsOutOfArea(i)) {
				if(i == 6 || !(TalkFlags[0x10] & 0x20) || FindInvOfType(i, 0x83) >= 0) {
					if(ObjStatus[i] & 0x40) {
						ObjStatus[i] &= ~0x40;
						CON_printf("%s breathes easier!\n", GetObjectString(i));
						PromptFlag = 1;
						StatusDirty = 1;
					}
				} else if(!(ObjStatus[i] & 0x40)) {
					ObjStatus[i] |= 0x40;
					CON_printf("%s gasps for air!\n", GetObjectString(i));
					PromptFlag = 1;
					StatusDirty = 1;
				}
			}
			for(inv = FindInv(i); inv >= 0; inv = NextInv()) {
				objTyp = GetType(inv);
				if((ObjStatus[inv] & 0x18) != 0x18)
					continue;
				light = TileFlag[TILE_FRAME(inv)] & 3;
				objTyp = GetType(inv);
				warmth += GetWarmth(objTyp);
				if(!IsLitLightType(objTyp))
					continue;
				if(OSI_rand(0, 1)) {
					if(GetQual(inv) <= minutes) {
						if(objTyp == 0x74 || objTyp == 0x76) {
							ObjShapeType[inv] = objTyp == 0x74?(ObjShapeType[inv] & 0xfc00) + 0x73:(ObjShapeType[inv] & 0xfc00) + 0x75;
							GetQual(inv) = 0;
							CON_printf("The %s is out of fuel.\n", GetObjectString(inv));
							PromptFlag = 1;
						} else {
							PrintTileArticle(TILE_FRAME(inv));
							CON_printf("%s went out.\n", GetObjectString(inv));
							PromptFlag = 1;
							DeleteObj(inv);
						}
						StatusDirty = 1;
						VisibilityDirty = 1;
					} else {
						if(torchLit < light)
							torchLit = light;
						GetQual(inv) -= minutes;
					}
				} else {
					if(torchLit < light)
						torchLit = light;
				}
			}
			if(!IsResting && !MapZ) {
				if(Time_H <= 3 || Time_H >= 22)
					cold = (warmth < 10?1:0) + 1;
				else if(Time_H <= 6 || Time_H >= 18)
					cold = warmth < 10?1:0;
				else
					cold = 0;
				if(i == 6)
					cold = 0;
				if(cold) {
					if(!(NPCStatus[i] & 1)) {
						CON_printf("%s is freezing!\n", GetObjectString(i));
						StatusDirty = 1;
						PromptFlag = 1;
						NPCStatus[i] |= 1;
					}
					if(minutes > 0) {
						for(j = 0; j < minutes; j++) {
							if(OSI_rand(0, 1)) {
								CON_printf("%s is freezing!\n", GetObjectString(i));
								PromptFlag = 1;
								LoseHP(i, OSI_rand(1, 2), 1);
								if(cold > 1)
									LoseHP(i, OSI_rand(1, 2), 1);
							} else if(cold > 1) {
								CON_printf("%s is freezing!\n", GetObjectString(i));
								PromptFlag = 1;
								LoseHP(i, OSI_rand(1, 2), 1);
							}
						}
					}
				} else if(NPCStatus[i] & 1) {
					CON_printf("%s feels warmer!\n", GetObjectString(i));
					StatusDirty = 1;
					PromptFlag = 1;
					NPCStatus[i] &= ~1;
				}
			} else if(NPCStatus[i] & 1) {
				CON_printf("%s feels warmer!\n", GetObjectString(i));
				StatusDirty = 1;
				PromptFlag = 1;
				NPCStatus[i] &= ~1;
			}
		}
		/*-- status effects wearing off, one roll per minute --*/
		for(j = 0; j < minutes; j++) {
			int16 owner;

			if(i == CharmedNPC && i && !InCombat) {
				NPCMode[CharmedNPC] = 0x1b;
				NPCStatus[CharmedNPC] = (NPCStatus[CharmedNPC] & ~0x60) | 0x60;
				CharmedNPC = 0;
			}
			if((ObjStatus[i] & 0x80) && !InCombat)
				ObjStatus[i] &= ~0x80;
			if((NPCStatus[i] & 8) && OSI_rand(0, 25) == 0) {
				NPCStatus[i] &= ~8;
				if(NPCStatus[i] & 0x80)
					StatusDirty = 1;
			}
			if((ObjStatus[i] & 4) && OSI_rand(0, 25) == 0) {
				COMBAT_stopCharm(i);
				if(NPCStatus[i] & 0x80)
					StatusDirty = 1;
			}
			if((NPCStatus[i] & 2) && (i == 6 || OSI_rand(0, 3) == 0 && GetStr(i) >= OSI_rand(1, 30))) {
				NPCStatus[i] &= ~2;
				if(NPCStatus[i] & 0x80)
					StatusDirty = 1;
			}
			if(
				(NPCStatus[i] & 4) && NPCMode[i] != 0x91 &&
				(!IsResting || !(NPCStatus[i] & 0x80)) &&
				OBJ_IsLocLoaded(GetX(i), GetY(i), GetZ(i)) && OSI_rand(0, 20) == 0
			) {
				NPCStatus[i] &= ~4;
				NPCFlag[i] |= 8;
				ObjShapeType[i] = OrigShapeType[i];
			}
			if((NPCStatus[i] & 8) && i != 6 && OSI_rand(0, 7) == 0)
				LoseHP(i, 1, 0);
			if(i > 7)
				continue;
			/*-- talents wear off --*/
			if(Talents[i].a && OSI_rand(1, GetInt(i) << 2) == 1) {
				if(--Talents[i].a == 0 && (NPCStatus[i] & 0x80)) {
					if(!i && (GameFlags & 0x10) || i && !(GameFlags & 0x10)) {
						MUS_PlaySfx(0x32);
						CON_printf(PsychicPowerFadesMsg);
						PromptFlag = 1;
						StatusDirty = 1;
					}
				}
			}
			if(Talents[i].b && OSI_rand(1, GetInt(i) << 2) == 1) {
				if(--Talents[i].b == 0) {
					if((NPCStatus[i] & 0x80) && (!i && (GameFlags & 0x10) || i && !(GameFlags & 0x10))) {
						MUS_PlaySfx(0x32);
						CON_printf(PsychicPowerFadesMsg);
						PromptFlag = 1;
						StatusDirty = 1;
					}
					if(Party[Active] == i)
						VisibilityDirty = 1;
				}
			}
			if(Talents[i].c && OSI_rand(1, GetInt(i) << 2) == 1) {
				if(--Talents[i].c == 0) {
					if((NPCStatus[i] & 0x80) && (!i && (GameFlags & 0x10) || i && !(GameFlags & 0x10))) {
						MUS_PlaySfx(0x32);
						CON_printf(PsychicPowerFadesMsg);
						PromptFlag = 1;
						StatusDirty = 1;
					}
					if(Party[Active] == i)
						VisibilityDirty = 1;
				}
			}
			for(inv = FindInvOfType(i, 0xa0); inv >= 0; inv = NextInvOfType()) {
				if(GetFrame(inv) > 1 && OSI_rand(0, 100) < 10) {
					CON_printf(IceMeltedMsg);
					PromptFlag = 1;
					SetFrame(inv, 1);
					if(StatusDisplay == 0x92 && Party[StatusMember] == i)
						StatusDirty = 1;
				}
			}
			for(inv = FindInvOfType(i, 0x100); inv >= 0; inv = NextInvOfType()) {
				if(OSI_rand(0, 100) < 10) {
					CON_printf(IceMeltedMsg);
					PromptFlag = 1;
					DeleteObj(inv);
					if(StatusDisplay == 0x92 && Party[StatusMember] == i)
						StatusDirty = 1;
				}
			}
			if(!(NPCStatus[i] & 8) && i != 6) {
				for(inv = FindInvOfType(i, 0x1c1); inv >= 0; inv = NextInvOfType()) {
					for(owner = GetOwner(inv); (ObjStatus[owner] & 0x18) && GetType(owner) != 0x8b; owner = GetOwner(owner))
						;
					if((ObjStatus[owner] & 0x18) == 0) {
						CON_printf(PoisonedMsg, TALK_GetCharName(i));
						PromptFlag = 1;
						NPCStatus[i] |= 8;
						StatusDirty = 1;
					}
				}
			}
			for(inv = FindInvOfType(i, 0x1c0); inv >= 0; inv = NextInvOfType()) {
				for(owner = GetOwner(inv); (ObjStatus[owner] & 0x18) && GetType(owner) != 0x8b; owner = GetOwner(owner))
					;
				if((ObjStatus[owner] & 0x18) == 0) {
					CON_printf(PoisonedMsg, TALK_GetCharName(i));
					PromptFlag = 1;
					NPCStatus[i] |= 8;
					StatusDirty = 1;
				}
			}
		}
		/*-- damaging terrain --*/
		ApplyTerrainEffects(i, GetX(i), GetY(i));
		NPCFlag[i] &= ~8;
	}

	/*-- calendar: 60 minutes, 24 hours, 28 days, 12 months --*/
	for(newMinutes = Time_M + minutes; newMinutes > 59; ) {
		newMinutes -= 60;
		if(++Time_H > 23) {
			Time_H = 0;
			if(++Date_D > 28) {
				Date_D = 1;
				if(++Date_M > 12) {
					Date_M = 1;
					Date_Y ++;
				}
			}
		}
		if(NextSleep)
			NextSleep --;
		/*-- the air bottles (0x83) of the party, one hour each --*/
		if(!IsResting && !(GameFlags & 0x10) && (TalkFlags[0x10] & 0x20)) {
			for(i = 0; i < PartySize; i++) {
				if((j = Party[i]) == 6)
					continue;
				if(NPCStatus[j] & 4)
					continue;
				if((inv = FindInvOfType(j, 0x83)) < 0) {
					if(!(ObjStatus[j] & 0x40)) {
						ObjStatus[j] |= 0x40;
						CON_printf("%s gasps for air!\n", GetObjectString(j));
						StatusDirty = 1;
						PromptFlag = 1;
					}
				} else if(GetQuan(inv) > 1) {
					GetQuan(inv) = GetQuan(inv) - 1;
					if(ObjStatus[j] & 0x40) {
						ObjStatus[j] &= ~0x40;
						CON_printf("%s breathes easier!\n", GetObjectString(j));
						StatusDirty = 1;
						PromptFlag = 1;
					}
				} else {
					DeleteObj(inv);
					if((inv = FindInvOfType(j, 0x83)) < 0) {
						if(!(ObjStatus[j] & 0x40)) {
							ObjStatus[j] |= 0x40;
							CON_printf("%s gasps for air!\n", GetObjectString(j));
							StatusDirty = 1;
							PromptFlag = 1;
						}
					} else if(ObjStatus[j] & 0x40) {
						ObjStatus[j] &= ~0x40;
						CON_printf("%s breathes easier!\n", GetObjectString(j));
						StatusDirty = 1;
						PromptFlag = 1;
					}
				}
			}
		}
		/*-- objects 0xe4: frame 7 at night once TalkFlags[0x73] & 0x10, else 3 --*/
		j = (TalkFlags[0x73] & 0x10) && (Time_H < 6 || Time_H > 17)?7:3;
		for(i = 0x101; i < 0xc00; i++) {
			if(GetType(i) == 0xe4)
				SetFrame(i, j);
		}
		if(DrunkCounter)
			DrunkCounter --;
		UpdateSchedules();
	}
	Time_M = newMinutes;
	j = Time_H * 60 + Time_M;/*time of day, in minutes*/
	if(minutes || LastHourUpdate == 0xff)
		DrawSkyBar();
	/*hourly updates*/
	if(LastHourUpdate != Time_H) {
		if(StatusDisplay == 0x92)
			STAT_ShowInventory(StatusMember);
		LastHourUpdate = Time_H;
		MUS_PlayAreaSong();
	}
	if(!(GameFlags & 0x10)) {
		/*ambient light*/
		oldLight = AmbientLight;
		if(MapZ > 0)
			AmbientLight = 0;
		else if(j < 400 || j > 1080)
			AmbientLight = 0;
		else if(j < 440)
			AmbientLight = (j - 400) / 10 + 2;
		else if(j > 1040)
			AmbientLight = (1080 - j) / 10 + 2;
		else
			AmbientLight = 7;
		if(AmbientLight < 4 && torchLit)
			AmbientLight = torchLit;
		if(AmbientLight != oldLight)
			VIS_UpdateView();
	}
}

/*redraw the status area (party list, portrait, equipment or the
conversation portrait) if StatusDirty, and the prompt if PromptFlag; in solo
mode the displayed member is forced to the solo character*/
/*C_120E_18B4*/RefreshStatus() {
	if(StatusDirty) {
		if(SoloMember >= 0)
			StatusMember = SoloMember;
		switch(StatusDisplay) {
			case 0x91: STAT_ShowParty(0); break;
			case 0x90: STAT_ShowPortrait(StatusMember); break;
			case 0x92: STAT_ShowInventory(StatusMember); break;
			case 0x9e: ShowPortrait(Party[StatusMember]); break;
		}
		DrawSlotCursor(1);
	}
	StatusDirty = 0;
	if(PromptFlag) {
		CON_printf("\n%s%>", Names[Active]);
		PromptFlag = 0;
	}
}

/*make party member #partyIdx the controlled character and centre the map
on him (partyIdx == PartySize: the vehicle)*/
/*C_120E_195E*/SetActive(int16 partyIdx) {
	if(Active == partyIdx)
		return;
	Active = partyIdx;
	MapX = GetX(Party[partyIdx]);
	MapY = GetY(Party[partyIdx]);
	MapZ = GetZ(Party[partyIdx]);
	MAP_LoadAround();
	MapAnimEnabled = 1;
	PromptFlag = 1;
}

/*back to party mode: restore the AI mode saved for the solo character,
the leader (Party[0]) is commanded, the others follow*/
/*C_120E_19DA*/SetPartyMode() {
	int16 objNum, i;

	if(SoloMember < 0)
		NPCMode[Party[0]] = SavedNPCMode;
	else
		NPCMode[Party[SoloMember]] = SavedNPCMode;
	SavedNPCMode = NPCMode[Party[0]];
	NPCMode[Party[0]] = 2;/*command*/
	for(i = 1; i < PartySize; i ++) {
		objNum = Party[i];
		if(!(ObjStatus[objNum] & 4))
			NPCMode[objNum] = 1;/*follow*/
		if(InCombat && (ObjStatus[objNum] & 0x80))
			NPCMode[objNum] = 6;
	}
	SetActive(0);
	SoloMember = 0xff;/*i.e. -1: party mode*/
	DrawSlotCursor(0);
}

/*read an hexadecimal number of up to "len" (max 4) digits from the
keyboard (debug commands); no validation of the digits*/
/*C_120E_1A7B*/ReadHexNumber(int16 len) {
	int16 i, value;

	if(len > 4)
		len = 4;
	CON_gets(HexInputBuf, len);
	value = 0;
	for(i = 0; i < len; i++) {
		if(HexInputBuf[i] == 0)
			break;
		value <<= 4;
		if(HexInputBuf[i] >= '0' && HexInputBuf[i] <= '9')
			value += HexInputBuf[i] - '0';
		else
			value += toupper(HexInputBuf[i]) - ('A' - 10);
	}
	return value;
}

/*read a decimal number of up to "len" (max 5) digits from the keyboard;
stops at the first non digit*/
/*C_120E_1AE1*/ReadDecNumber(int16 len) {
	int16 i, value;

	if(len > 5)
		len = 5;
	CON_gets(DecInputBuf, len);
	value = 0;
	for(i = 0; i < len; i++) {
		if(DecInputBuf[i] == 0 || DecInputBuf[i] < '0' || DecInputBuf[i] > '9')
			break;
		value = DecInputBuf[i] + value * 10 - '0';
	}
	return value;
}

/*commands names, indexed by command code - 0x81*/
/*0F36*/static char *CommandNames[] = {
	"Attack",
	"Talk",
	"Look",
	"Get",
	"Drop",
	"Move",
	"Use",
	"Combat",
	"REST",
	"CAST",
	"Save"
};

/*range of the talent a of npc (its level), 0 without it*/
#define TALENT_RANGE(n) (Talents[n].a?Level[n]:0)

/*game loop:
one iteration per player turn: read a key (CON_getch also returns mouse
generated command codes and runs the animations while waiting), translate
letter keys to commands, execute, then do the end of turn processing.
Exits on <ALT+X>/<CTRL+Q>/<CTRL+E> confirmed (ch == 0x12D).*/
/*C_120E_1B3A*/GameLoop() {
	register int16 ch;
	register int16 member;
	int16 key, mouseHidden;

	HatchingDisabled = 0;
	InitNPCTracker();
	PassTime(0);
	VIS_UpdateView();
	ScreenFade = 1;
	DrawMapWindow(1);
	CON_printf("%s%>", Names[Active]);
	do {
		RefreshStatus();
		if(MapAnimEnabled || VisibilityDirty)
			DisplayTick();
		if(MouseMode == 0)
			AllowMouseMov = 1;
		ch = CON_getch();
		AllowMouseMov = 0;

		/*keyboard commands: start a target selection (cursor at the
		  centre of the map window, SelectRange < 0 = direction only)*/
		switch(ch) {
			case 'A'/*ttack*/:
				ch = 0x81;
				DrawSlotCursor(0);
				FreeAimMode = 0;
				SelectMode = 1;
				AimX =
				AimY = 5;
				SelectRange = 7;
				/*aim at the previous target if still on screen*/
				if(LastAttackTarget >= 0) {
					AimX = WrapDelta(GetX(LastAttackTarget) - MapX) + 5;
					AimY = WrapDelta(GetY(LastAttackTarget) - MapY) + 5;
					if(AimX >= 0 && AimX <= 10 && AimY >= 0 && AimY <= 10)
						break;
					AimX =
					AimY = 5;
				}
			break;
			case 'T'/*alk*/:
				ch = 0x82;
				DrawSlotCursor(0);
				FreeAimMode = 0;
				SelectMode = 1;
				AimX =
				AimY = 5;
				SelectRange = 7;
			break;
			case 'L'/*ook*/:
				ch = 0x83;
				DrawSlotCursor(0);
				FreeAimMode = 0;
				SelectMode = 1;
				AimX =
				AimY = 5;
				SelectRange = 7;
			break;
			case 'G'/*et*/:
				ch = 0x84;
				DrawSlotCursor(0);
				FreeAimMode = 0;
				SelectMode = 1;
				AimX =
				AimY = 5;
				if(TALENT_RANGE(Party[Active]))
					SelectRange = TALENT_RANGE(Party[Active]) + 1;
				else
					SelectRange = -1;
			break;
			case 'D'/*rop*/:
				/*selection in the inventory: show the equipment of the
				  active character*/
				ch = 0x85;
				DrawSlotCursor(0);
				FreeAimMode = 0;
				SelectMode = 2;
				AimX =
				AimY = 5;
				SelectRange = 7;
				SlotCursorCol = 3;
				SlotCursorRow = 0;
				if(StatusDisplay != 0x92 || Active != StatusMember) {
					StatusDisplay = 0x92;
					StatusMember = Active;
					StatusDirty = 1;
					RefreshStatus();
				}
				DrawSlotCursor(1);
			break;
			case 'M'/*ove*/:
				ch = 0x86;
				DrawSlotCursor(0);
				FreeAimMode = 0;
				SelectMode = 1;
				AimX =
				AimY = 5;
				if(TALENT_RANGE(Party[Active]))
					SelectRange = TALENT_RANGE(Party[Active]) + 1;
				else
					SelectRange = -1;
			break;
			case 'U'/*se*/:
				ch = 0x87;
				DrawSlotCursor(0);
				FreeAimMode = 0;
				SelectMode = 1;
				AimX =
				AimY = 5;
				if(TALENT_RANGE(Party[Active]))
					SelectRange = TALENT_RANGE(Party[Active]) + 1;
				else
					SelectRange = -1;
			break;
			case 'R'/*est*/:
				ch = 0x89;
				DrawSlotCursor(0);
				FreeAimMode = 0;
				SelectMode = 1;
				AimX =
				AimY = 5;
				SelectRange = 7;
			break;
			case 19:/*<CTRL+S>*/
				ch = 0xaa;
			break;
			case 'B'/*egin/break combat*/:
				ch = 0x88;
			break;
		}

		/*character switch: '0' = party mode, '1'..'9' = solo mode*/
		if(ch >= '0' && ch <= '9' && ch < PartySize + '1') {
			if(ch == '0') {/*Party Mode*/
				if(IN_VEHICLE) {
					CON_printf(NotWhileDreamingMsg);
				} else if(InCombat) {
					CON_printf(NotInCombatMsg);
				} else {
					/*everybody must be within 8 tiles*/
					for(member = 0; member < PartySize; member ++) {
						if(MapDistance(MapX, GetX(Party[member])) > 8 || MapDistance(MapY, GetY(Party[member])) > 8) {
							CON_printf(NotAllHereMsg);
							break;
						}
					}
					if(PartySize == member) {
						CON_printf(PartyModeMsg);
						SetPartyMode();
						MouseMode = 0;
						SelectMode = 0;
						PendingCommand = 0x8f;
						CON_UpdateMousePointer();
					}
				}
			} else {/*Solo Mode*/
				if(
					(NPCStatus[Party[ch - '1']] & 0x16) ||
					(NPCFlag[Party[ch - '1']] & 0x10) ||
					(ObjStatus[Party[ch - '1']] & 4)
				) {
					CON_printf(FailsToRespondMsg, Names[ch - '1']);
					MUS_PlaySfx(5);
				} else if(IN_VEHICLE) {
					CON_printf(NotWhileDreamingMsg);
					MUS_PlaySfx(5);
				} else if(InCombat) {
					CON_printf(NotInCombatMsg);
					MUS_PlaySfx(5);
				} else {
					/*restore the AI mode of the previously commanded
					  character and save the one of the new solo character*/
					if(SoloMember < 0)
						NPCMode[Party[Active]] = SavedNPCMode;
					else
						NPCMode[Party[SoloMember]] = SavedNPCMode;
					SavedNPCMode = NPCMode[Party[ch - '1']];
					/*followers stay where they are*/
					for(member = 0; member < PartySize; member ++) {
						if(NPCMode[Party[member]] == 1)
							NPCMode[Party[member]] = 0;
					}
					SetActive(ch - '1');
					NPCMode[Party[Active]] = 2;
					SoloMember = Active;
					if(StatusDisplay != 0x91)
						StatusDirty = 1;
					CON_printf(SoloModeMsg);
					MouseMode = 0;
					DrawSlotCursor(0);
					SelectMode = 0;
					PendingCommand = 0x8f;
					CON_UpdateMousePointer();
				}
			}
		} else/*the other commands*/
		switch(ch) {
			case 0x80:/*[advance]*/
				PartyAdvance(AdvanceDir);
			break;
			case 5: case 17: case 0x12D:/*<CTRL+E> <CTRL+Q> <ALT+X> --"exit to dos?"*/
				CON_printf(ExitToDOSMsg);
				while((key = CON_getch()) != 'Y' && key != 'N');
				CON_printf("%c\n", key);
				if(key == 'N')
					ch = 0;
				else
					ch = 0x12D;
			break;
			case 0x81:/*"attack"*/
				if(DefaultCommand != ch) {
					mouseHidden = CON_mouseOffAt(15, 163, 32, 196);
					GR_DRAW_BITMAP(&Screen, 21, 169, ScreenParts, 0);
					CON_mouseOn(mouseHidden);
				}
				BargeFindLeader(1);
				COMBAT_attack();
				if(DefaultCommand != ch) {
					mouseHidden = CON_mouseOffAt(15, 163, 32, 196);
					GR_DRAW_BITMAP(&Screen, 21, 169, ScreenParts, 1);
					CON_mouseOn(mouseHidden);
				}
				PendingCommand = 0x8f;
			break;
			case 0x88:/*begin/break combat*/
				if(InCombat) {/*break off*/
					CON_printf(BreakOffMsg);
					COMBAT_breakOff();
				} else if(SoloMember >= 0) {
					CON_printf(NotInSoloMsg);
				} else {
					CON_printf(CombatMsg);
					COMBAT_begin();
				}
			break;
			case 0x82:
			case 0x83:
			case 0x84:
			case 0x85:
			case 0x86:
			case 0x87:
				/*mark the command's icon, wait for the target (0x8e =
				  selection done)*/
				if(DefaultCommand != ch) {
					member = TIL2SCR(ch - 0x81) + ((ch - 0x81) << 1);
					mouseHidden = CON_mouseOffAt(member + 15, 163, member + 32, 196);
					GR_DRAW_BITMAP(&Screen, member + 21, 169, ScreenParts, 0);
					CON_mouseOn(mouseHidden);
				}
				MouseMode = 1;
				PendingCommand = ch;
				CON_UpdateMousePointer();
				CON_printf("%s-", CommandNames[ch - 0x81]);
				key = CON_getch();
				if(key == 0x8e) {
					switch(PendingCommand) {
						case 0x83: CmdLook(); break;/*"look"*/
						case 0x85: CmdDrop(); break;/*"drop"*/
						case 0x84: CmdGet(); break;/*"get"*/
						case 0x86: CmdMove(); break;/*"move"*/
						case 0x87: CmdUse(); break;/*"use"*/
						case 0x82:/*"talk"*/
							MouseMode = 0;
							TALK_talkTo(Active, SelectionObj, 1);
						break;
					}
				} else {
					CON_printf(WhatMsg);
				}
				if(DefaultCommand != ch) {
					member = TIL2SCR(ch - 0x81) + ((ch - 0x81) << 1);
					mouseHidden = CON_mouseOffAt(member + 15, 163, member + 32, 196);
					GR_DRAW_BITMAP(&Screen, member + 21, 169, ScreenParts, 1);
					CON_mouseOn(mouseHidden);
				}
				MouseMode = 0;
				PendingCommand = 0x8f;
				DrawSlotCursor(0);
				SelectMode = 0;
				CON_UpdateMousePointer();
			break;
			case 0xaa:/*"save game"*/
				if(IsArmageddon) {
					switch(IsArmageddon) {
						case 1: CON_printf(KilledHelperMsg); break;
						case 2: CON_printf(HistoricalFigureDeadMsg); break;
					}
					break;
				}
				if(InCombat) {
					CON_printf(NotInCombatMsg);
					MUS_PlaySfx(5);
					break;
				}
				CON_printf(SaveMsg);
				while((key = CON_getch()) != 'Y' && key != 'N');
				CON_printf("%c\n", key);
				if(key == 'Y') {
					GAME_Save();
					GAME_Restore();
					DrawSlotCursor(0);
					MouseMode = 0;
					SelectMode = 0;
					CON_UpdateMousePointer();
				}
			break;
			case 18:/*<CTRL+R> "restore game"*/
				CON_printf("Restore game? ");
				while((key = CON_getch()) != 'Y' && key != 'N');
				CON_printf("%c\n", key);
				if(key == 'Y') {
					GAME_Restore();
					DrawSlotCursor(0);
					MouseMode = 0;
					SelectMode = 0;
					CON_UpdateMousePointer();
				}
			break;
			case 27: case ' ': case 0x8f:/*"pass"*/
				CON_printf(PassMsg);
				MovePts[Party[Active]] = 0;
				DrawSlotCursor(0);
				MouseMode = 0;
				SelectMode = 0;
				CON_UpdateMousePointer();
				BeltUpdate(1);
				if(Party[Active] == 1)
					BargeFindLeader(1);
				MoveFollowers(Party[Active], 1);
			break;
			case 26:/*<CTRL+Z> sound effects*/
				if(soundCard == 0) {
					CON_printf("No sound available!\n");
				} else if(SoundFlag) {
					SoundFlag = 0;
					MUS_StopSfx();
					CON_printf(SoundOffMsg);
				} else {
					SoundFlag = 1;
					CON_printf(SoundOnMsg);
				}
			break;
			case 1:/*<CTRL+A> music*/
				if(soundCard == 0 || soundCard == 3) {
					CON_printf("No music available!\n");
				} else if(MusicAdapter) {
					CON_printf(MusicOffMsg);
					MUS_PlaySong(-1);
					MusicAdapter = 0;
				} else if(emsInUse) {
					CON_printf(MusicOnMsg);
					MusicAdapter = 1;
					if(InCombat) {
						MUS_PlaySong(8);
						MUS_CombatOutcomeSong();
					} else {
						MUS_PlayAreaSong();
					}
				} else {
					CON_printf(NoEmsMsg);
				}
			break;
			case 8:/*<CTRL+H>*/
				CON_printf("Help ");
				HelpOn = !HelpOn;
				CON_printf(HelpOn?"on":"off");
				CON_printf("!\n");
			break;
			case 22:/*<CTRL+V> version string*/
				CON_printf(VersionMsg);
			break;
			case 0xd5: case 0xd6: case 0xd7: case 0xdd: case 0xde:
			case 0xdf: case 0xe0: case 0xe1: case 0xe2: case 0xe3:
			case 0xe4: case 0xe5: case 0xe8: case 0xe9: case 0xea:
			case 0xf2: case 0xf3: case 0xf5: case 0xf6: case 0xf9:
			case 0xfa: case 0xfb: case 0xfc:/*<alt 2 x x> debug*/
				if(TalkFlags[1] & 0x40) {
					switch(ch) {
						case 0xd5:/*time, position, peer*/
							CON_printf("\nTIME %02d:%02d\nX%03x Y%03x Z%1x\n", Time_H, Time_M, MapX, MapY, MapZ);
							PeerMap();
						break;
						case 0xd6:/*go to coordinate (hexadecimal x,y,z)*/
						{
							int16 x, y, z;

							CON_printf("\nGoto x: ");
							x = ReadHexNumber(3);
							CON_printf("\n     y: ");
							y = ReadHexNumber(3);
							CON_printf("\n     z: ");
							if((z = ReadHexNumber(1)) < 0)
								z = 0;
							if(z) {
								x &= 0xff;
								y &= 0xff;
							} else {
								x &= 0x3ff;
								y &= 0x3ff;
							}
							if(z > 6)
								z = 6;
							PartyTeleport(x, y, z, 1);
							MUS_PlayAreaSong();
						}
						break;
						case 0xd7:/*time += 60 minutes*/
							PassTime(60);
							AllowNPCTeleport = 1;
							PTH_SearchPaths();
							AllowNPCTeleport = 0;
						break;
						case 0xdd:/*a missile of the active character to a target*/
						{
							int16 num, num2, num3;

							num = Party[Active];
							CON_printf("Divine Flare--");
							MouseMode = 1;
							DrawSlotCursor(0);
							SelectMode = 1;
							AimX =
							AimY = 5;
							SelectRange = 8;
							CON_getch();
							num2 = SelectionX;
							num3 = SelectionY;
							CON_printf("from (%03x,%03x) to (%03x,%03x):", GetX(num), GetY(num), num2, num3);
							MUS_PlaySfx(0x32);
							MUS_PlaySfx(6);
							if(COMBAT_Missile(num, &num2, &num3, 8, 0x101)) {
								CON_printf("successful! (%03x,%03x)\n", num2, num3);
								MUS_PlaySfx(4);
							} else {
								CON_printf("failed! (%03x,%03x)\n", num2, num3);
								MUS_PlaySfx(0);
							}
						}
						break;
						case 0xde:/*"solid" avatar*/
							if(AvatarNotSolid) {
								CON_printf("Avatar solidifies!\n");
								MUS_PlaySfx(0x32);
								AvatarNotSolid = 0;
							} else {
								CON_printf("Avatar desolidifies!\n");
								MUS_PlaySfx(0x32);
								AvatarNotSolid = 1;
							}
						break;
						case 0xdf:/*go to an NPC*/
						{
							int16 num, num2;

							CON_printf("\nNPC: ");
							num = ReadHexNumber(2);
							if(!(NPCStatus[num] & 0x10) && num <= 0xbf) {
								num2 = (int16)TALK_GetCharName(num);
								CON_printf("%s\n", num2);
								PartyTeleport(GetX(num) + 1, GetY(num) + 1, GetZ(num), 0);
								MUS_PlayAreaSong();
							} else {
								CON_printf("invalid");
								MUS_PlaySfx(5);
							}
						}
						break;
						case 0xe8:/*show an NPC*/
						{
							int16 num, num2;

							CON_printf("NPC: ");
							num = ReadHexNumber(2);
							num2 = (int16)TALK_GetCharName(num);
							CON_printf("%s\n", num2);
							ShowPortrait(num);
							break;
						}
						case 0xe9:/*sound effect*/
						{
							int16 num;

							if(SoundFlag) {
								CON_printf("SFX number: ");
								num = ReadDecNumber(2);
								if(num == 0xe)
									MUS_SweepSfx();
								else if(num == 0x13)
									MUS_MachinerySfx();
								else if(num == 0x23)
									MUS_SplashSfx();
								else
									MUS_PlaySfx(num);
							}
						}
						break;
						case 0xea:
						{
							int16 num;

							if(SoundFlag) {
								CON_printf("SFX number: ");
								num = ReadDecNumber(2);
								MUS_PlayLoopSfx(num);
							}
						}
						break;
						case 0xf2:/*talk flag bits*/
						{
							int16 num, num2, num3, num4, num5;

							CON_printf("NPC: ");
							num = ReadHexNumber(2);
							num5 = (int16)TALK_GetCharName(num);
							CON_printf("%s\n", num5);
							CON_printf("flags 0x%x\n", TalkFlags[num]);
							CON_printf("\nBit: ");
							if((num2 = ReadHexNumber(1)) < 8)
								CON_printf("%d is ", num2);
							else
								CON_printf("WAIT flag ");
							num4 = num2 < 8 ? TalkFlags[num] & (1 << num2) : D_3262[num] & 0x80;
							if(num4)
								CON_printf("on.");
							else
								CON_printf("off.");
							CON_printf("\nNew value? ");
							if((num3 = ReadHexNumber(1)) != 0) {
								if(num2 < 8)
									TalkFlags[num] |= 1 << num2;
								else
									D_3262[num] |= 0x80;
								CON_printf("on.");
							} else {
								if(num2 < 8)
									TalkFlags[num] &= ~(1 << num2);
								else
									D_3262[num] &= ~0x80;
								CON_printf("off.");
							}
						}
						break;
						case 0xf3:/*talk to an NPC*/
						{
							int16 num;

							CON_printf("NPC: ");
							num = ReadHexNumber(2);
							CON_printf("%s\n", TALK_GetCharName(num));
							TALK_talkTo(Active, num, 1);
						}
						break;
						case 0xf5:/*a cut scene*/
						{
							int16 num;

							CON_printf("Do midgame: ");
							num = ReadDecNumber(2);
							switch(num) {
								case 1: C_2F75_02E1(); break;
								case 2: C_2F75_042D(); break;
								case 3: C_2F75_0532(); break;
								case 4: C_2F75_0757(); break;
								case 5: C_2F75_07AA(); break;
								case 6: C_2F75_07FA(); break;
								case 7: C_2F75_0951(); break;
								case 8: C_2F75_0A64(); break;
								case 9: C_2F75_0B90(); break;
								default: CON_printf("None!\n");
							}
						}
						break;
						case 0xf6:/*the barges*/
							BargeDump();
							CON_printf("LB: %d\n", LeaderOnBarge);
						break;
						case 0xf9:/*analyze an object*/
							CON_printf("Analyze what? ");
							MouseMode = 1;
							DrawSlotCursor(0);
							SelectMode = 1;
							AimX =
							AimY = 5;
							SelectRange = 9;
							if(CON_getch() == 0x8e && SelectionObj >= 0) {
								CON_printf("Obj%x\n(%x,%x,%x)\n", SelectionObj, GetX(SelectionObj), GetY(SelectionObj), GetZ(SelectionObj));
								CON_printf("T%03x F%x A%x(%d)\n", GetType(SelectionObj), GetFrame(SelectionObj), Amount[SelectionObj], Amount[SelectionObj]);
								CON_printf("QQ%x(%d)/%x(%d)\n", GetQual(SelectionObj), GetQual(SelectionObj), GetQuan(SelectionObj), GetQuan(SelectionObj));
								if(ObjStatus[SelectionObj] & 1)
									CON_printf("Au ");
								if(ObjStatus[SelectionObj] & 2)
									CON_printf("In ");
								if(ObjStatus[SelectionObj] & 4)
									CON_printf("Ch ");
								if(ObjStatus[SelectionObj] & 0x40)
									CON_printf("Cu ");
								if(ObjStatus[SelectionObj] & 0x80)
									CON_printf("Fn ");
								if(SelectionObj < 0x100 && SelectionObj >= 0) {
									if(NPCStatus[SelectionObj] & 1)
										CON_printf("Fe ");
									if(NPCStatus[SelectionObj] & 2)
										CON_printf("Pa ");
									if(NPCStatus[SelectionObj] & 4)
										CON_printf("As ");
									if(NPCStatus[SelectionObj] & 8)
										CON_printf("Po ");
									if(NPCStatus[SelectionObj] & 0x10)
										CON_printf("De ");
									if(NPCStatus[SelectionObj] & 0x80)
										CON_printf("Pt ");
									switch(NPCStatus[SelectionObj] & 0x60) {
										case 0x40: CON_printf("G\n"); break;
										case 0x20: CON_printf("E\n"); break;
										case 0: CON_printf("N\n"); break;
										case 0x60: CON_printf("C\n"); break;
									}
								}
							}
							DrawSlotCursor(0);
						break;
						case 0xfa:/*create an object*/
						{
							int16 num, num2, num3, num4, num5, num6, obj;

							CON_printf("Create Item:\nType:0x");
							num = ReadHexNumber(3);
							if(BaseTile[num + 1] - BaseTile[num] > 1) {
								CON_printf("\nFrame:0x");
								num2 = ReadHexNumber(1);
							} else {
								num2 = 0;
							}
							CON_printf("\nQual:0x");
							num3 = ReadHexNumber(2);
							CON_EchoChar('\n');
							if(QuanType(num) || num == 0xc4 || num == 0x137 || num == 0x138) {
								CON_printf("\nQuan:0x");
								num4 = ReadHexNumber(2);
							} else {
								num4 = 0;
							}
							num5 = num | (num2 << 10);
							num6 = (num3 << 8) | num4;
							if((obj = AddInvObj(num5, 0x10, num6, Party[Active])) < 0) {
								if((obj = AddMapObj(num5, 0, num6, MapX, MapY, MapZ)) < 0) {
									CON_printf("\nFailed!\n");
								} else {
									CON_printf("\n(dropped)\n");
									VisibilityDirty = 1;
								}
							} else {
								CON_printf("\n");
								StatusDirty = 1;
							}
						}
						break;
						case 0xfb:/*heal the party*/
						{
							int16 num;

							CON_printf("Healing party\n\n");
							if(IN_VEHICLE) {
								HitPoints[0] = MaxHP(1);
							} else {
								for(num = 0; num < PartySize; num++)
									HitPoints[Party[num]] = MaxHP(Party[num]);
							}
							StatusDirty = 1;
						}
						break;
						case 0xfc:/*memory*/
							CON_printf("%dK free for tiles\n\n", GrMemoryKB);
						break;
						default:
							CON_printf(WhatMsg);
							MUS_PlaySfx(5);
							DrawSlotCursor(0);
							MouseMode = 0;
							SelectMode = 0;
							CON_UpdateMousePointer();
					}
				} else if(ch == 0xf2) {
					CON_printf("\nTIME %02d:%02d\nX%03x Y%03x Z%1x\n", Time_H, Time_M, MapX, MapY, MapZ);
				} else if(ch == 0xd5) {
					CON_printf("That's an old, worn-out magic word!\n");
				} else {
					CON_printf("Are you a god?\n");
					MUS_PlaySfx(5);
					DrawSlotCursor(0);
					MouseMode = 0;
					SelectMode = 0;
					CON_UpdateMousePointer();
				}
			break;
			default:
				CON_printf(WhatMsg);
				DrawSlotCursor(0);
				MouseMode = 0;
				SelectMode = 0;
				CON_UpdateMousePointer();
		}

		/*-- end of turn --*/
		if(ch != 0x12D) {
			/*"disk full" recovery choice*/
			if(DiskFullChoice == 'R')
				GAME_Restore();/*Restore game?*/
			else if(DiskFullChoice == 'G')
				GAME_Save();/*Save game?*/
			/*NPCs and the rest of the world take their turn*/
			RunNPCTurns();
			if(InCombat && (CombatOutcome == 1 || CombatOutcome == -1))
				MUS_CombatOutcomeSong();
			if(ch != 0x80)
				PromptFlag = 1;
		}
	} while(ch != 0x12D);
}
