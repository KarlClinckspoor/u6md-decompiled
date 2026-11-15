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
 * apply the effects of what a creature stands on. SetActive / SetPartyMode
 * switch solo mode / party mode. From SE: DrawMapWindow, DisplayTick,
 * C_0359_0D58, FlushViewIfDirty, ApplyTerrainEffects, PassTime's frame,
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

/*the graphic driver: D_BA6A's offset selects the entry*/
#define GR_SET_COLOR(c)               (D_BA6A.iii.ofs = 0x06, (*D_BA6A.f)(c))
#define GR_PUT_PIXEL(x,y)             (D_BA6A.iii.ofs = 0x09, (*D_BA6A.f)(x,y))
#define GR_PUT_TILE(tile,x,y)         (D_BA6A.iii.ofs = 0x42, (*D_BA6A.f)(tile,x,y))
#define GR_COPY_TO_SCREEN(a,b,c,d,e,g) (D_BA6A.iii.ofs = 0x45, (*D_BA6A.f)(a,b,c,d,e,g))
#define GR_FADE_TO_SCREEN(a,b,c,d,e,g) (D_BA6A.iii.ofs = 0x48, (*D_BA6A.f)(a,b,c,d,e,g))
#define GR_SET_ANIM_FRAME(a,b)        (D_BA6A.iii.ofs = 0x4b, (*D_BA6A.f)(a,b))

/*OSI library (asm)*/
extern int16 OSI_rand(int16, int16);/*OSI_rand(lo, hi)*/
extern cdecl MOUSE_SERVE();
extern cdecl MOUSE_DRAW();
extern cdecl MOUSE_SAVE_BG();
extern int16 cdecl MOUSE_IN_AREA(struct tSurface *);

/*far data*/
extern int16 far C_2DE5_01C6[];/*MonsterFlags (by GetMonsterClass)*/

/*other modules*/
extern int16 C_07D1_0075(int16);
extern int16 C_07D1_064E(int16);
extern C_07D1_1D64(int16, int16, int16);/*(npc, damage, kind)*/
extern Missiles(int16, int16, int16, int16 *, int16 *, int16, int16, int16);
extern Explosion(int16, int16, int16);
extern VIS_UpdateView();
extern DrawSlotCursor(int16);
extern cdecl CON_printf(char far *, ...);
extern unsigned char GetTileAtXYZ(int16, int16, int16);
extern int16 GetMonsterClass(int16);
extern int16 C_1B09_032A();/*NextLoc*/
extern int16 C_1B09_07D8(int16, int16, int16);/*FindLoc(x, y, z)*/
extern C_1B09_3E18();/*ShowObjects*/
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
extern C_047A_024B(int16);/*ShowPortrait*/
extern C_047A_0416(int16);/*shows a tile in the text window*/
extern C_07D1_014C(int16);/*COMBAT_stopCharm*/
extern int16 C_07D1_0521(int16);/*an NPC's strength*/
extern int16 C_07D1_05C0(int16);/*an NPC's intelligence*/
extern int16 C_100B_015C(int16);/*warmth of a worn type*/
extern int16 C_100B_01FB(int16);/*a burning light?*/
extern int16 C_170D_0002(int16);
extern C_170D_3D9D();/*UpdateSchedules*/
extern int16 C_1B09_005C(int16, int16, int16);/*OBJ_IsLocLoaded(x, y, z)*/
extern int16 C_1B09_0B1D(int16);/*FindInv*/
extern int16 C_1B09_0B76();/*NextInv*/
extern int16 C_1B09_0BEE(int16, int16);/*first object of a type in an inventory*/
extern int16 C_1B09_0CA4();/*next one*/
extern char *C_1B09_4759(int16);/*GetObjectString*/
extern char far C_2BB4_21E9[];
extern char far C_2BB4_1737[];
extern char far C_2BB4_136D[];
extern int16 C_1534_1101(int16);/*WrapDelta*/
extern int16 C_1534_1129(int16, int16);/*MapDistance (wrapping)*/
extern C_170D_3D71();/*InitNPCTracker*/
extern C_170D_1097(int16);/*PartyAdvance*/
extern C_170D_0976(int16, int16);/*MoveFollowers*/
extern C_170D_3685();/*PTH_SearchPaths*/
extern C_170D_3A6D();/*RunNPCTurns*/
extern C_07D1_01BB();/*COMBAT_begin*/
extern C_07D1_02C4();/*COMBAT_breakOff*/
extern C_07D1_3480();/*COMBAT_attack*/
extern int16 C_07D1_0CFC(int16, int16 *, int16 *, int16, int16);
extern C_047A_0734();/*CmdLook*/
extern C_047A_145E();/*CmdDrop*/
extern C_047A_1C0E();/*CmdGet*/
extern C_047A_26A5();/*CmdMove*/
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
extern C_1FA9_047E();
extern C_1FA9_0CD2();
extern C_1FA9_09A5(int16);
extern C_1FA9_0A8A();
extern C_1FA9_0AFD();
extern C_1FA9_0BE4();
extern int16 C_100B_0295(int16);/*QuanType*/
extern int16 C_1B09_116E(int16, int16, int16, int16, int16, int16);/*AddMapObj(shape, status, amount, x, y, z)*/
extern int16 C_1B09_12EE(int16, int16, int16, int16);/*AddInvObj(shape, status, amount, owner)*/
extern C_2F75_02E1();
extern C_2F75_042D();
extern C_2F75_0532();
extern C_2F75_0757();
extern C_2F75_07AA();
extern C_2F75_07FA();
extern C_2F75_0951();
extern C_2F75_0A64();
extern C_2F75_0B90();
extern char far C_2BB4_0130[];/*pass*/
extern char far C_2BB4_0137[];/*combat*/
extern char far C_2BB4_0146[];/*break off*/
extern char far C_2BB4_0159[];/*sound on*/
extern char far C_2BB4_0163[];/*sound off*/
extern char far C_2BB4_016E[];/*music off*/
extern char far C_2BB4_0179[];/*music on*/
extern char far C_2BB4_0183[];/*exit to DOS?*/
extern char far C_2BB4_0191[];/*save game?*/
extern char far C_2BB4_019D[];/*solo mode*/
extern char far C_2BB4_01A8[];/*party mode*/
extern char far C_2BB4_02D4[];/*not all here*/
extern char far C_2BB4_02FA[];/*not in combat*/
extern char far C_2BB4_0315[];/*not in solo mode*/
extern char far C_2BB4_0338[];/*not on the barge*/
extern char far C_2BB4_03F1[];/*what?*/
extern char far C_2BB4_13C1[];/*fails to respond*/
extern char far C_2BB4_174D[];
extern char far C_2BB4_178B[];
extern char far C_2BB4_215E[];/*no EMS for the music*/
extern char far C_2BB4_22F9[];/*version*/
extern int16 C_1B09_09C7(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 C_1B09_1026(int16, int16, int16);/*FindInvType(owner, type, quality)*/
extern C_1B09_1872(int16);/*DeleteObj*/
extern char far C_2BB4_22F1[];/*failure message*/

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
extern int16 D_00D6;/*MusicAdapter*/
extern char D_028C;
extern int16 D_06BE;/*CombatOutcome*/
extern int16 D_13F2;/*HatchingDisabled*/
extern int16 D_1422;/*AllowNPCTeleport*/
extern int16 D_1687;/*SoundFlag*/
extern int16 D_1689;/*the avatar is not solid (debug)*/
extern unsigned char D_16A7;/*DefaultCommand*/
extern int16 D_2E5C;/*emsInUse*/
extern int16 D_3260;/*free memory for the tiles, in KB*/
extern unsigned char far *D_3262;/*talk flags 8-15*/
extern char D_44C4[];/*MovePts*/
extern unsigned char far *D_4EB6;/*Level*/
extern void far *D_BA7E;/*mdscreen.lzc entry 2: default command mark*/
extern int16 D_EB72;/*AdvanceDir*/
extern int16 D_EBCE;/*Selection.obj*/
extern int16 D_EBF6, D_EBF8;/*Selection.x, Selection.y*/
extern int16 D_080E;/*ScreenFade*/
extern int16 D_1431;/*VisibilityDirty*/
extern unsigned char D_1673;/*NextSleep*/
extern unsigned char D_1674;/*Time_M*/
extern unsigned char D_1675;/*Time_H*/
extern unsigned char D_1676;/*Date_D*/
extern unsigned char D_1677;/*Date_M*/
extern int16 D_1678;/*Date_Y*/
extern unsigned char D_167A;/*Active*/
extern char D_167B;/*AmbientLight*/
extern unsigned char D_1686;/*DrunkCounter*/
extern char D_16A4;/*InCombat*/
extern char D_16A5;/*SoloMember*/
extern unsigned char D_16A6;/*SavedNPCMode*/
extern struct { unsigned a:4, b:4, c:4, d:4; } D_16B0[];/*talents*/
extern int16 StatusMember;
extern int16 StatusDisplay;
extern unsigned char D_213E;/*IsResting*/
extern unsigned char D_3266[];/*ObjStatus*/
extern unsigned char far *D_46D0;/*NPCMode*/
extern unsigned char D_4EA4[];/*Party*/
extern char D_50CE[][14];/*Names*/
extern unsigned char far *D_6DBA;/*TileFlag*/
extern unsigned char D_6EC6[];/*TalkFlags*/
extern unsigned char D_9CC0;/*PartySize*/
extern uint16 far *D_B508;/*OrigShapeType*/
extern int16 D_EC4A;
extern unsigned char D_BC84[5];/*HexInputBuf*/
extern unsigned char D_BC89[6];/*DecInputBuf*/
extern int32 D_16AA;/*flags (0x10: the dream)*/
extern int16 D_167C, D_167E, D_1680;/*MapX, MapY, MapZ*/
extern struct tSurface D_3232;/*Screen*/
extern unsigned char D_3240[0x20];/*AnimFrameMask*/
extern struct tMouse D_3E66;/*Mouse*/
extern unsigned char far *D_45CC;/*NPCFlag*/
extern int16 D_54BA[];/*Amount*/
extern unsigned char D_6CBA[];/*NPCStatus*/
extern unsigned char D_6DBE[];
extern int16 far *D_6EBE;/*BaseTile*/
extern unsigned char far *D_760A;/*TileFlag2*/
extern int16 D_770E[0x20];/*AnimFirstFrame*/
extern struct coord D_78AE[];/*ObjPos*/
extern struct tSurface D_9CAE;/*MapSurface*/
extern int16 D_9CC2;/*NumAnimData*/
extern uint16 D_9CC4[];/*ObjShapeType*/
extern unsigned char far *D_B4C4;/*TerrainType*/
extern int16 D_B4C8[0x20];/*AnimTile*/
extern unsigned char D_B50C[0x20];/*AnimFrameShift*/
extern union tHandler D_BA6A;/*GrDriver*/
extern unsigned char D_BC8E[0x20];/*StateAnimData*/
extern unsigned char D_C130[11][11];/*Obj_11x11*/
extern int16 D_C1AA[];/*CellObjTile*/
extern unsigned char D_C3AA[];/*CellObjNext*/
extern int16 D_C4AA[11][11];/*Tile_11x11*/
extern struct tSurface D_EB78;/*MouseArea*/

#define ScreenFade D_080E
#define MusicAdapter D_00D6
#define CombatOutcome D_06BE
#define HatchingDisabled D_13F2
#define AllowNPCTeleport D_1422
#define SoundFlag D_1687
#define DefaultCommand D_16A7
#define emsInUse D_2E5C
#define MovePts D_44C4
#define Level D_4EB6
#define AdvanceDir D_EB72
#define Selection_obj D_EBCE
#define Selection_x D_EBF6
#define Selection_y D_EBF8
#define WrapDelta C_1534_1101
#define MapDistance C_1534_1129
#define InitNPCTracker C_170D_3D71
#define PartyAdvance C_170D_1097
#define MoveFollowers C_170D_0976
#define PTH_SearchPaths C_170D_3685
#define RunNPCTurns C_170D_3A6D
#define COMBAT_begin C_07D1_01BB
#define COMBAT_breakOff C_07D1_02C4
#define COMBAT_attack C_07D1_3480
#define CmdLook C_047A_0734
#define CmdDrop C_047A_145E
#define CmdGet C_047A_1C0E
#define CmdMove C_047A_26A5
#define QuanType C_100B_0295
#define AddMapObj C_1B09_116E
#define AddInvObj C_1B09_12EE
#define PassMsg C_2BB4_0130
#define CombatMsg C_2BB4_0137
#define BreakOffMsg C_2BB4_0146
#define SoundOnMsg C_2BB4_0159
#define SoundOffMsg C_2BB4_0163
#define MusicOffMsg C_2BB4_016E
#define MusicOnMsg C_2BB4_0179
#define ExitToDOSMsg C_2BB4_0183
#define SaveMsg C_2BB4_0191
#define SoloModeMsg C_2BB4_019D
#define PartyModeMsg C_2BB4_01A8
#define NotAllHereMsg C_2BB4_02D4
#define NotInCombatMsg C_2BB4_02FA
#define NotInSoloMsg C_2BB4_0315
#define NotOnShipMsg C_2BB4_0338
#define WhatMsg C_2BB4_03F1
#define FailsToRespondMsg C_2BB4_13C1
#define NoEmsMsg C_2BB4_215E
#define VersionMsg C_2BB4_22F9
#define IN_VEHICLE (Active == PartySize)
#define GR_DRAW_BITMAP(s,x,y,p,m)    (D_BA6A.iii.ofs = 0x30, (*D_BA6A.f)(s,x,y,p,m))
#define VisibilityDirty D_1431
#define NextSleep D_1673
#define Time_M D_1674
#define Time_H D_1675
#define Date_D D_1676
#define Date_M D_1677
#define Date_Y D_1678
#define Active D_167A
#define AmbientLight D_167B
#define DrunkCounter D_1686
#define InCombat D_16A4
#define SoloMember D_16A5
#define SavedNPCMode D_16A6
#define HexInputBuf D_BC84
#define DecInputBuf D_BC89
#define IsResting D_213E
#define ObjStatus D_3266
#define NPCMode D_46D0
#define Party D_4EA4
#define Names D_50CE
#define TileFlag D_6DBA
#define TalkFlags D_6EC6
#define PartySize D_9CC0
#define OrigShapeType D_B508
#define MapX D_167C
#define MapY D_167E
#define MapZ D_1680
#define Screen D_3232
#define AnimFrameMask D_3240
#define Mouse D_3E66
#define NPCFlag D_45CC
#define Amount D_54BA
#define NPCStatus D_6CBA
#define BaseTile D_6EBE
#define TileFlag2 D_760A
#define AnimFirstFrame D_770E
#define ObjPos D_78AE
#define MapSurface D_9CAE
#define NumAnimData D_9CC2
#define ObjShapeType D_9CC4
#define TerrainType D_B4C4
#define AnimTile D_B4C8
#define AnimFrameShift D_B50C
#define StateAnimData D_BC8E
#define Obj_11x11 D_C130
#define CellObjTile D_C1AA
#define CellObjNext D_C3AA
#define Tile_11x11 D_C4AA
#define MouseArea D_EB78
#define MonsterFlags C_2DE5_01C6
#define NextLoc C_1B09_032A
#define FindLoc C_1B09_07D8
#define ShowObjects C_1B09_3E18
#define OBJ_FindTypeAt C_1B09_09C7
#define FindInvType C_1B09_1026
#define DeleteObj C_1B09_1872
#define COMBAT_stopCharm C_07D1_014C
#define GetStr C_07D1_0521
#define GetInt C_07D1_05C0
#define UpdateSchedules C_170D_3D9D
#define OBJ_IsLocLoaded C_1B09_005C
#define FindInv C_1B09_0B1D
#define NextInv C_1B09_0B76
#define FindInvOfType C_1B09_0BEE
#define NextInvOfType C_1B09_0CA4
#define GetObjectString C_1B09_4759
#define ShowPortrait C_047A_024B
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

/*0F10*/int16 D_0F10 = 0;/*AnimFrameCounter*/
/*0F12*/unsigned char D_0F12 = 1;/*StatusDirty: status area must be redrawn*/
/*0F13*/unsigned char D_0F13 = 0;/*PromptFlag: the prompt must be printed again*/
/*0F14*/unsigned char D_0F14 = 0;/*CyclopsFlag (Ultima VI's)*/
/*0F15*/unsigned char D_0F15 = 0;/*IsArmageddon (Ultima VI's)*/
/*0F16*/int16 D_0F16 = 0;/*IdleAnimTick*/
/*0F18*/int16 D_0F18 = 1;/*MapAnimEnabled*/
/*0F1A*/int16 D_0F1A = 0;/*Erupting (SE's)*/
/*wall flag (TerrainType) per direction N,NE,E,SE,S,SW,W (Ultima VI's; not used here)*/
/*0F1C*/unsigned char DirWallFlags[] = {0x80,0,0x40,0,0x20,0,0x10};

#define AnimFrameCounter D_0F10
#define StatusDirty D_0F12
#define PromptFlag D_0F13
#define IdleAnimTick D_0F16
#define MapAnimEnabled D_0F18

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
/*C_120E_024B*/C_120E_024B(int16 mode) {
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
/*C_120E_029B*/C_120E_029B(int16 npc, int16 tile) {
}

/*the effect of object obj on creature npc (it stands on it)*/
/*C_120E_02A2*/TerrainDamage(register int16 npc, int16 obj) {
	int16 objTyp, objCls, kind;
	int16 a, b;
	int16 dmg;

	dmg = 0;
	kind = 0;
	NPCFlag[npc] |= 8;/*terrain handled for this turn*/
	if(C_07D1_0075(npc))
		return;
	if(!(NPCStatus[npc] & 0x10)) {/*not dead*/
	objCls = GetMonsterClass(GetType(npc));
	switch(objTyp = GetType(obj)) {
		case 0xd1:
			kind = 3;
			if(npc != 6)
				dmg = OSI_rand(D_6DBE[npc] / 2, C_07D1_064E(npc) / 2);
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
			if(D_6DBE[0] < 2)
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
			dmg = OSI_rand(1, D_6DBE[npc] - 2);
		break;
	}
	if(dmg)
		C_07D1_1D64(npc, dmg, kind);
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
				C_120E_029B(objNum, tileNum);
		}
	}
}

/*a growing plant obj (quality 0xf, frames 0-2) grows one stage when the date
kept by the object 0xfc in the container 0x101 under it is old enough (in
the dream: at random)*/
/*C_120E_0800*/C_120E_0800(register int16 obj) {
	register int16 inv;
	int16 cont, qual, day, month, frm;

	inv = -1;
	qual = GetQual(obj);
	if(qual == 0xf && GetFrame(obj) < 3) {
		if((cont = OBJ_FindTypeAt(0x101, GetX(obj), GetY(obj))) < 0) {
			CON_printf(C_2BB4_22F1);
			return;
		}
		if((inv = FindInvType(cont, 0xfc, -1)) < 0) {
			CON_printf(C_2BB4_22F1);
			return;
		}
		day = GetQual(inv);
		month = GetQuan(inv);
		if(D_16AA & 0x10) {
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
/*C_120E_09C7*/C_120E_09C7(register int16 obj) {
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
			if(!C_170D_0002(i)) {
				switch(GetType(i)) {
					case 0x1a2: BargeAddController(i); break;
					case 0x1d0: UpdateJet(i); break;
					case 0xfb: C_120E_0800(i); break;
					case 0x9d: C_120E_09C7(i); break;
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
			if(D_16B0[i].a > 10)
				D_16B0[i].a = 10;
			if(D_16B0[i].b > 10)
				D_16B0[i].b = 10;
			if(D_16B0[i].c > 10)
				D_16B0[i].c = 10;
		}
		if(!(D_16AA & 0x10) && (NPCStatus[i] & 0x80) && !(NPCStatus[i] & 0x10)) {
			/*-- party members: air, lights, cold --*/
			if(i == 0)
				continue;
			warmth = 0;
			if(minutes && !(NPCStatus[i] & 4) && !C_170D_0002(i)) {
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
				warmth += C_100B_015C(objTyp);
				if(!C_100B_01FB(objTyp))
					continue;
				if(OSI_rand(0, 1)) {
					if(GetQual(inv) <= minutes) {
						if(objTyp == 0x74 || objTyp == 0x76) {
							ObjShapeType[inv] = objTyp == 0x74?(ObjShapeType[inv] & 0xfc00) + 0x73:(ObjShapeType[inv] & 0xfc00) + 0x75;
							GetQual(inv) = 0;
							CON_printf("The %s is out of fuel.\n", GetObjectString(inv));
							PromptFlag = 1;
						} else {
							C_047A_0416(TILE_FRAME(inv));
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
								C_07D1_1D64(i, OSI_rand(1, 2), 1);
								if(cold > 1)
									C_07D1_1D64(i, OSI_rand(1, 2), 1);
							} else if(cold > 1) {
								CON_printf("%s is freezing!\n", GetObjectString(i));
								PromptFlag = 1;
								C_07D1_1D64(i, OSI_rand(1, 2), 1);
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

			if(i == D_EC4A && i && !InCombat) {
				NPCMode[D_EC4A] = 0x1b;
				NPCStatus[D_EC4A] = (NPCStatus[D_EC4A] & ~0x60) | 0x60;
				D_EC4A = 0;
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
				C_07D1_1D64(i, 1, 0);
			if(i > 7)
				continue;
			/*-- talents wear off --*/
			if(D_16B0[i].a && OSI_rand(1, GetInt(i) << 2) == 1) {
				if(--D_16B0[i].a == 0 && (NPCStatus[i] & 0x80)) {
					if(!i && (D_16AA & 0x10) || i && !(D_16AA & 0x10)) {
						MUS_PlaySfx(0x32);
						CON_printf(C_2BB4_21E9);
						PromptFlag = 1;
						StatusDirty = 1;
					}
				}
			}
			if(D_16B0[i].b && OSI_rand(1, GetInt(i) << 2) == 1) {
				if(--D_16B0[i].b == 0) {
					if((NPCStatus[i] & 0x80) && (!i && (D_16AA & 0x10) || i && !(D_16AA & 0x10))) {
						MUS_PlaySfx(0x32);
						CON_printf(C_2BB4_21E9);
						PromptFlag = 1;
						StatusDirty = 1;
					}
					if(Party[Active] == i)
						VisibilityDirty = 1;
				}
			}
			if(D_16B0[i].c && OSI_rand(1, GetInt(i) << 2) == 1) {
				if(--D_16B0[i].c == 0) {
					if((NPCStatus[i] & 0x80) && (!i && (D_16AA & 0x10) || i && !(D_16AA & 0x10))) {
						MUS_PlaySfx(0x32);
						CON_printf(C_2BB4_21E9);
						PromptFlag = 1;
						StatusDirty = 1;
					}
					if(Party[Active] == i)
						VisibilityDirty = 1;
				}
			}
			for(inv = FindInvOfType(i, 0xa0); inv >= 0; inv = NextInvOfType()) {
				if(GetFrame(inv) > 1 && OSI_rand(0, 100) < 10) {
					CON_printf(C_2BB4_1737);
					PromptFlag = 1;
					SetFrame(inv, 1);
					if(StatusDisplay == 0x92 && Party[StatusMember] == i)
						StatusDirty = 1;
				}
			}
			for(inv = FindInvOfType(i, 0x100); inv >= 0; inv = NextInvOfType()) {
				if(OSI_rand(0, 100) < 10) {
					CON_printf(C_2BB4_1737);
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
						CON_printf(C_2BB4_136D, TALK_GetCharName(i));
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
					CON_printf(C_2BB4_136D, TALK_GetCharName(i));
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
		if(!IsResting && !(D_16AA & 0x10) && (TalkFlags[0x10] & 0x20)) {
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
	if(!(D_16AA & 0x10)) {
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
#define TALENT_RANGE(n) (D_16B0[n].a?Level[n]:0)

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
					CON_printf(NotOnShipMsg);
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
					CON_printf(NotOnShipMsg);
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
					GR_DRAW_BITMAP(&D_3232, 21, 169, D_BA7E, 0);
					CON_mouseOn(mouseHidden);
				}
				BargeFindLeader(1);
				COMBAT_attack();
				if(DefaultCommand != ch) {
					mouseHidden = CON_mouseOffAt(15, 163, 32, 196);
					GR_DRAW_BITMAP(&D_3232, 21, 169, D_BA7E, 1);
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
					GR_DRAW_BITMAP(&D_3232, member + 21, 169, D_BA7E, 0);
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
							TALK_talkTo(Active, Selection_obj, 1);
						break;
					}
				} else {
					CON_printf(WhatMsg);
				}
				if(DefaultCommand != ch) {
					member = TIL2SCR(ch - 0x81) + ((ch - 0x81) << 1);
					mouseHidden = CON_mouseOffAt(member + 15, 163, member + 32, 196);
					GR_DRAW_BITMAP(&D_3232, member + 21, 169, D_BA7E, 1);
					CON_mouseOn(mouseHidden);
				}
				MouseMode = 0;
				PendingCommand = 0x8f;
				DrawSlotCursor(0);
				SelectMode = 0;
				CON_UpdateMousePointer();
			break;
			case 0xaa:/*"save game"*/
				if(D_0F15) {
					switch(D_0F15) {
						case 1: CON_printf(C_2BB4_174D); break;
						case 2: CON_printf(C_2BB4_178B); break;
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
					C_1FA9_0CD2();
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
						C_1FA9_047E();
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
							num2 = Selection_x;
							num3 = Selection_y;
							CON_printf("from (%03x,%03x) to (%03x,%03x):", GetX(num), GetY(num), num2, num3);
							MUS_PlaySfx(0x32);
							MUS_PlaySfx(6);
							if(C_07D1_0CFC(num, &num2, &num3, 8, 0x101)) {
								CON_printf("successful! (%03x,%03x)\n", num2, num3);
								MUS_PlaySfx(4);
							} else {
								CON_printf("failed! (%03x,%03x)\n", num2, num3);
								MUS_PlaySfx(0);
							}
						}
						break;
						case 0xde:/*"solid" avatar*/
							if(D_1689) {
								CON_printf("Avatar solidifies!\n");
								MUS_PlaySfx(0x32);
								D_1689 = 0;
							} else {
								CON_printf("Avatar desolidifies!\n");
								MUS_PlaySfx(0x32);
								D_1689 = 1;
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
									C_1FA9_0A8A();
								else if(num == 0x13)
									C_1FA9_0AFD();
								else if(num == 0x23)
									C_1FA9_0BE4();
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
								C_1FA9_09A5(num);
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
							CON_printf("LB: %d\n", D_028C);
						break;
						case 0xf9:/*analyze an object*/
							CON_printf("Analyze what? ");
							MouseMode = 1;
							DrawSlotCursor(0);
							SelectMode = 1;
							AimX =
							AimY = 5;
							SelectRange = 9;
							if(CON_getch() == 0x8e && Selection_obj >= 0) {
								CON_printf("Obj%x\n(%x,%x,%x)\n", Selection_obj, GetX(Selection_obj), GetY(Selection_obj), GetZ(Selection_obj));
								CON_printf("T%03x F%x A%x(%d)\n", GetType(Selection_obj), GetFrame(Selection_obj), Amount[Selection_obj], Amount[Selection_obj]);
								CON_printf("QQ%x(%d)/%x(%d)\n", GetQual(Selection_obj), GetQual(Selection_obj), GetQuan(Selection_obj), GetQuan(Selection_obj));
								if(ObjStatus[Selection_obj] & 1)
									CON_printf("Au ");
								if(ObjStatus[Selection_obj] & 2)
									CON_printf("In ");
								if(ObjStatus[Selection_obj] & 4)
									CON_printf("Ch ");
								if(ObjStatus[Selection_obj] & 0x40)
									CON_printf("Cu ");
								if(ObjStatus[Selection_obj] & 0x80)
									CON_printf("Fn ");
								if(Selection_obj < 0x100 && Selection_obj >= 0) {
									if(NPCStatus[Selection_obj] & 1)
										CON_printf("Fe ");
									if(NPCStatus[Selection_obj] & 2)
										CON_printf("Pa ");
									if(NPCStatus[Selection_obj] & 4)
										CON_printf("As ");
									if(NPCStatus[Selection_obj] & 8)
										CON_printf("Po ");
									if(NPCStatus[Selection_obj] & 0x10)
										CON_printf("De ");
									if(NPCStatus[Selection_obj] & 0x80)
										CON_printf("Pt ");
									switch(NPCStatus[Selection_obj] & 0x60) {
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
								D_6DBE[0] = C_07D1_064E(1);
							} else {
								for(num = 0; num < PartySize; num++)
									D_6DBE[Party[num]] = C_07D1_064E(Party[num]);
							}
							StatusDirty = 1;
						}
						break;
						case 0xfc:/*memory*/
							CON_printf("%dK free for tiles\n\n", D_3260);
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
				C_1FA9_047E();
			if(ch != 0x80)
				PromptFlag = 1;
		}
	} while(ch != 0x12D);
}
