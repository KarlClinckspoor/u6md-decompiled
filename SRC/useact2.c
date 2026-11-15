/* flags: -d -mm -p -c -Z -O -y */
/* stub order: 03AA 1E83 1BB6 0000 1EC7 0F88 0777 17B5 1A75 1334 19BD 166A 0CF3 0E53 1E08 0503 1237 1449 1088 1988 0267 1568 */
/*
 * Martian Dreams - GAME.EXE overlay 2F98: actions of the "use" command (the
 * second of three overlays of them, with 2F8D and 2FA1). New code.
 *
 * Each action is called by UseObject (use.c) through the UseActions table
 * (far 2E67) as f(obj, on, to): the used object, the second object ("On -")
 * and the third ("To -"), -1 when not asked; the numbers below are the
 * table's rows. Billiards, the pipe organ, the rollers, the cannon, the
 * panels of the cabinets 0x1ca (the dream machines, the rubber sprayer, the
 * ruby lenses, the canals), the spray guns, the headgear, the lever, the
 * position (latitude and longitude), the oxium bin, seeds and plants, the
 * spittoon, the ruby slippers (the Grand Finale), the dreamstuff and its
 * hoard, the valve and oxygenated air.
 */
#include "se.h"

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;/*1 wrap, 2 center, 4 capitalize, 8 runes, 0x10 paginate, 0x20 scroll*/
};

extern void cdecl TimerDelay(int32);/*wait (2383 per millisecond)*/
extern int16 cdecl toupper(int16);
extern int cdecl abs(int);

/*other modules*/
extern int16 GetDex(int16);
extern __TryHit(int16, int16, int16, int16);/*(attacker, x, y, weapon) (combat.c)*/
extern int16 IsPlantType(int16);
extern BargePumps(int16);
extern CON_gets(char *, int16);
extern int16 ParseNumberInput(unsigned char *, int16);
extern COMBAT_Kill(int16);
extern DreamSleep(int16, int16);
extern DreamEnter();
extern DreamWakeUp();
extern SetPartyMode();
extern STAT_ShowParty(int16);
extern TALK_talkTo(int16, int16, int16);
extern C_2F75_07AA();
extern C_2F75_07FA();
extern C_2F75_0951();
extern C_2F75_0A64();
extern C_2F75_1607();
extern C_2F75_1657();
extern C_2F75_16A8();
extern int16 COMBAT_Missile(int16, int16 *, int16 *, int16, int16);
extern int16 far *Explosion(int16, int16, int16);/*Explosion(x, y, missileTile): the objects hit, -1 terminated*/
extern int16 IsBreakableType(int16);
extern int16 OBJ_FindNPCAt(int16, int16, int16);/*OBJ_FindNPCAt(startNum, x, y)*/
extern int16 KBD_GetKey(int16);
extern MUS_PlaySong(int16);
extern MUS_PlayAreaSong();
extern MUS_PlayMelody();
extern int16 MaxHP(int16);
extern COMBAT_showHit(int16);
extern int16 LoseHP(int16, int16, int16);
extern DisplayTick();
extern PassTime(int16);
extern unsigned char GetTileAtXYZ(int16, int16, int16);
extern int16 NextLoc();
extern int16 FindLoc(int16, int16, int16);
extern int16 OBJ_FindTypeAt(int16, int16, int16);
extern int16 FindInv(int16);
extern int16 NextInv();
extern int16 NextArea();
extern int16 SearchArea(int16, int16, int16, int16);
extern int16 NextInvType();
extern int16 FindInvType(int16, int16, int16);
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);
extern int16 AddInvObj(int16, int16, int16, int16);
extern DeleteObj(int16);
extern MoveObj(int16, int16, int16, int16);
extern int16 InsertObj(int16, int16, int16);
extern ShowObjects();
extern char *GetObjectString(int16);
extern MUS_PlaySfx(int16);
extern int16 GetDigClass(int16);
extern int16 CON_prompt();
extern int16 CON_getch();
extern CON_putch(int16);
extern cdecl CON_printf(char far *, ...);
extern int16 OSI_rand(int16, int16);
extern int16 WrapDelta(int16);
extern int16 COMBAT_range(int16, int16, int16, int16);
extern int16 COMBAT_TryTeleport(int16, int16);
extern int16 CanMoveTo(int16, int16, int16);
extern int16 EGG_generate(int16, int16, int16, int16);/*EGG_generate(type, x, y, z)*/
extern int16 SPELL_ReviveNPC(int16);
extern C_2F8D_0476(int16, int16, int16);/*take and ready (useact1.c)*/
extern void PrintTileArticle(int16);
extern PrintObjCountOrArticle(int16);
extern DrawSlotCursor(int16);
extern far CON_UpdateMousePointer();

extern signed char DirIncrX[8], DirIncrY[8];
extern unsigned char StatusDirty;
extern int16 MapAnimEnabled;
extern unsigned char Active;
extern int16 MapX, MapY, MapZ;
extern char SoloMember;
extern int32 GameFlags;/*0x10: in a dream*/
extern int16 StatusMember;
extern int16 StatusDisplay;
extern unsigned char far *D_3262;/*NPC flags: 0x80 ?, bits 0-7: talk flags 8-15*/
extern unsigned char far *NPCMode;
extern unsigned char far *NPCComMode;
extern int16 far *OrigShapeType;
extern int16 ColorText;
extern int16 ColorDarkBlue;/*color of the player's input*/
extern unsigned char Time_M, Time_H;
extern int16 MouseMode;
extern unsigned char DirOriginX, DirOriginY;/*screen position of the party*/
extern char *DirNames[];
extern int16 AdvanceDir;
extern unsigned char HitPoints[];
extern int16 SelectionObj;
extern struct { unsigned a:4, psychic:4, c:4, d:4; } Talents[];
extern char SelectMode;
extern char AimX, AimY;
extern int16 SelectionX, SelectionY;
extern unsigned char IsResting;
extern struct tClipInfo *CurClip;
extern unsigned char ObjStatus[];
extern unsigned char Party[];
extern int16 Amount[];/*quantity (low byte), quality (high byte)*/
extern unsigned char NPCStatus[];
extern int16 far *BaseTile;
extern unsigned char TalkFlags[];
extern struct coord ObjPos[];
extern unsigned char PartySize;
extern uint16 ObjShapeType[];


#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)
#define GetQual(o) (((unsigned char *)(Amount + (o)))[1])
#define SetQual(o, v) (GetQual(o) = (v))
#define GetQuan(o) (((unsigned char *)(Amount + (o)))[0])
#define SetQuan(o, v) (GetQuan(o) = (v))
#define GetAssoc(i) (*(int16 *)&(ObjPos[i]))

/*messages (segment 2BB4)*/
extern char far BlockedMsg[];/*"Blocked!\n"*/
extern char far TooMuchMsg[];/*"You are carrying too much already.\n"*/
extern char far NoEffectMsg[];/*"It has no effect.\n"*/
extern char far NotNowMsg[];/*"Not now.\n"*/
extern char far NotUndergroundMsg[];/*"Not while underground.\n"*/
extern char far NowhereMsg[];/*"nowhere.\n"*/
extern char far NothingMsg[];/*"nothing!\n"*/
extern char far FireMsg[];/*"\nFire!\n"*/
extern char far SplatMsg[];/*"Splat! There is no effect.\n"*/
extern char far KaboomMsg[];/*"Kaboom! You blew the door down!\n"*/
extern char far HasSteelBallsMsg[];/*"It already has steel cannon balls.\n"*/
extern char far SteelBallsMsg[];/*"The cannon will fire steel cannon balls.\n"*/
extern char far PressKeysToPlayMsg[];/*"Press keys 1-0 to play -"*/
extern char far ToccataMsg[];/*"Tocatta and Fugue in D minor (J.S. Bach).\n"*/
extern char far PanelBrokenMsg[];/*"The panel is broken.\n"*/
extern char far PanelNotInstalledMsg[];/*"The panel is not installed.\n"*/
extern char far PanelsOnCabinetsMsg[];/*"Panels are only installed onto cabinets.\n"*/
extern char far LeverStuckMsg[];/*"the lever seems stuck.\n"*/
extern char far PushLeverMsg[];/*"Push lever to "*/
extern char far LatLongMsg[];/*"You are somewhere near \nLat:%d %c\nLong:%d %c\n"*/
extern char far NeedCueStickMsg[];/*"You need a cue stick in your hands.\n"*/
extern char far NeedPipeOrganMsg[];/*"Organ music must be played on a pipe organ.\n"*/
extern char far HeadgearInstalledMsg[];/*"The headgear is installed.\n"*/
extern char far MachineDoesNotWorkMsg[];/*"The machine does not work.\n"*/
extern char far NobodyInMachineMsg[];/*"There is nobody sitting in the machine.\n"*/
extern char far DreamMachinesCeasedMsg[];/*"The dream machines seem to have ceased functioning.\n"*/
extern char far NobodyUnderSprayerMsg[];/*"Nobody is standing under the sprayer.\n"*/
extern char far NoRubberCoatMsg[];/*"You don't think it would be safe to coat an animal or plant with rubber.\n"*/
extern char far ChsheketTreatedMsg[];/*"Chsheket has already been treated.\n"*/
extern char far NoRubberMsg[];/*"There is no rubber in the sprayer system.\n"*/
extern char far RubberNotFleshMsg[];/*"The rubber in the sprayer system is not flesh-colored.\n"*/
extern char far KilledPlantMsg[];/*"You killed the plant.\n"*/
extern char far BecomesVisibleMsg[];/*"`%s becomes visible!\n"*/
extern char far RollersUnfixableMsg[];/*"Ooops. These rollers can never be fixed!\n"*/
extern char far OxiumFromBinMsg[];/*"You get two handfuls of oxium from the bin.\n"*/
extern char far ImagineMsg[];/*"You imagine "*/
extern char far PullDreamstuffMsg[];/*"You pull some of the dreamstuff from the hoard.\n"*/
extern char far LackMentalPowerMsg[];/*"You lack the mental power to affect the dreamstuff hoard.\n"*/
extern char far LensesNotRespondingMsg[];/*"Some of the lenses are not responding.\n"*/
extern char far NoWaterPressureMsg[];/*"There is no water pressure in the valve system.\n"*/
extern char far CanalsWorkingMsg[];/*"The canals are already working.\n"*/
extern char far LensesAlignedMsg[];/*"The ruby lenses are already aligned.\n"*/
extern char far GoodShotMsg[];/*"Good shot, old bean!\n"*/
extern char far HealedMsg[];/*"\n%s is healed.\n"*/
extern char far NoApparentEffectMsg[];/*"You actuate the mechanism to no apparent effect.\n"*/
extern char far StatusLightsChangeMsg[];/*"Status lights change but you see no other effect.\n"*/
extern char far LightsFlashMsg[];/*"Lights flash and change color but nothing else happens.\n"*/
extern char far UnsureWhatYouDidMsg[];/*"You are completely unsure what you just did.\n"*/
extern char far NoSprayerNearMsg[];/*"Funny, no sprayer near sprayer controls!\n"*/
extern char far AlignmentMsg[];/*"Alignment coordinate:\n"*/
extern char far WaitUntilDawnMsg[];/*"You wait until dawn...*\n"*/
extern char far NoSeedYetMsg[];/*"No seed to harvest yet.\n"*/
extern char far LocationMsg[];/*"Location: "*/
extern char far WontGrowMsg[];/*"Won't grow there!\n"*/
extern char far NeedsWateringMsg[];/*"Now it just needs watering.\n"*/
extern char far MouthTooDryMsg[];/*"Your mouth is too dry.\n"*/
extern char far SpitMsg[];/*"You spit into the spittoon.\n"*/
extern char far ClickMsg[];/*"Click.\n"*/
extern char far RubySlippersMsg[];/*"You may use the ruby slippers to go home. If you choose to, you may now view the Grand Finale.\n\n*"*/
extern char far FinaleExitMsg[];/*"Afterwards, you will exit to DOS. You will be able to resume play from your last saved game.\n\n*"*/
extern char far SeeFinaleMsg[];/*"Would you like to see the Grand Finale now? If so, press Y. To resume play now, hit N: (Y N)\n"*/
extern char far SprayGunRefillMsg[];/*"The spray gun has 10 more charges.\n"*/
extern char far SprayGunPaintMsg[];/*"The spray gun now contains 20 charges of green paint.\n"*/
extern char far SprayGunWeedKillerMsg[];/*"The spray gun now contains 10 charges of weed killer.\n"*/
extern char far SprayGunEmptyMsg[];/*"There is nothing in the gun.\n"*/
extern char far NoAirNeededMsg[];/*"The %s doesn't need oxygenated air.\n"*/
extern char far BeyondHelpMsg[];/*"%s is beyond help.\n"*/
extern char far RevivedMsg[];/*"%s is revived!\n"*/
extern char far AarghMsg[];/*"Aargh!\n"*/

/*
 * action 35 (0x18f): billiards; needs a readied cue stick 0x191; the balls
 * 0x190 at (x - 1, y) and (x, y) (made with random frames) roll while
 * OSI_rand(0, 40) < GetDex
 */
/*C_2F98_0000*/C_2F98_0000(int16 obj, int16 on, int16 to) {
	register int16 x;
	register int16 b2;
	int16 b1, i, y, frames;

	frames = BaseTile[0x191] - BaseTile[0x190] - 1;
	for(x = FindInvType(Party[Active], 0x191, -1); x != -1 && (ObjStatus[x] & 0x18) != 0x18; x = NextInvType())
		;
	if(x == -1) {
		CON_printf(NeedCueStickMsg);
		return;
	}
	x = GetX(obj);
	y = GetY(obj);
	if((b1 = OBJ_FindTypeAt(0x190, (x - 1) & 0x3ff, y)) < 0)
		b1 = AddMapObj(0x190 | (OSI_rand(0, frames) << 10), 0, 0, (x - 1) & 0x3ff, y, MapZ);
	if(b1 < 0) {
		CON_printf(AarghMsg);
		return;
	}
	if((b2 = OBJ_FindTypeAt(0x190, x, y)) < 0)
		b2 = AddMapObj(0x190 | (OSI_rand(0, frames) << 10), 0, 0, x, y, MapZ);
	if(b2 < 0) {
		CON_printf(AarghMsg);
		return;
	}
	i = 0;
	do {
		if(i != 0)
			TimerDelay(OSI_rand(200, 500) * 2383L);
		MUS_PlaySfx(OSI_rand(0, 2) + 0x1c);
		TimerDelay(OSI_rand(10, 200) * 2383L);
		MUS_PlaySfx(OSI_rand(0, 2) + 0x1c);
		ObjShapeType[b1] = GetType(b1) + (OSI_rand(0, frames) << 10);
		ObjShapeType[b2] = GetType(b2) + (OSI_rand(0, frames) << 10);
		ShowObjects();
		MapAnimEnabled = 1;
		DisplayTick();
		i++;
	} while(OSI_rand(0, 40) < GetDex(Party[Active]));
	if(OSI_rand(0, 40) < GetDex(Party[Active]))
		CON_printf(GoodShotMsg);
}

/*
 * action 36 (pipe organ 0xa5, its seat 0x199): with the seat left of it,
 * Bach (MUS_PlayMelody); else keys 1-0 play sfx key - 10
 */
/*C_2F98_0267*/C_2F98_0267(register int16 obj, int16 on, int16 to) {
	register int16 seat = -1;
	int16 o, done, key;

	if(GetType(obj) == 0x199) {
		if((ObjStatus[obj] & 0x18) == 0) {
			seat = obj;
			if((o = OBJ_FindTypeAt(0xa5, (GetX(obj) + 1) & 0x3ff, GetY(obj))) < 0) {
				CON_printf(NeedPipeOrganMsg);
				return;
			}
			obj = o;
		} else {
			CON_printf(NeedPipeOrganMsg);
			return;
		}
	}
	if(seat == -1) {
		for(seat = FindLoc(GetX(obj) - 1, GetY(obj), MapZ); seat != -1 && GetType(seat) != 0x199; seat = NextLoc())
			;
	}
	if(seat >= 0) {
		CON_printf(ToccataMsg);
		MUS_PlayMelody();
	} else {
		MUS_PlaySong(-1);
		CON_printf(PressKeysToPlayMsg);
		done = 0;
		while(!done) {
			while((key = KBD_GetKey(1)) == 0)
				;
			if(key >= '0' && key <= '9')
				MUS_PlaySfx(key - 10);
			else
				done = 1;
		}
		MUS_PlayAreaSong();
	}
}

/*
 * action 46 (0x90 on the rollers 0xc0): fix the rollers (0xbc, a 0xbd every
 * fourth); TalkFlags[0x72] 4
 */
/*C_2F98_03AA*/C_2F98_03AA(int16 obj, int16 on, int16 to) {
	register int16 o;
	register int16 x;
	int16 y, n, cnt, r;

	x = GetX(on);
	y = GetY(on);
	for(o = on; GetFrame(o) != 0 && o != -1 && GetType(o) == 0xc0; o = OBJ_FindTypeAt(0xc0, x, y))
		x = (GetX(o) - 1) & 0x3ff;
	if(o == -1 || GetType(o) != 0xc0) {
		CON_printf(RollersUnfixableMsg);
		return;
	}
	for(r = o, cnt = 4; r >= 0; r = OBJ_FindTypeAt(0xc0, x, y)) {
		ObjShapeType[r] = (ObjShapeType[r] & 0xfc00) + 0xbc;
		if(cnt == 0) {
			cnt = 4;
			if((n = AddMapObj(0xbd, 1, 0, x, y, MapZ)) >= 0)
				ObjStatus[n] &= ~0x20;
			else
				CON_printf(AarghMsg);
		}
		cnt--;
		x = (x + 1) & 0x3ff;
	}
	DeleteObj(obj);
	ShowObjects();
	DisplayTick();
	TalkFlags[0x72] |= 4;
}

/*
 * action 25 (cannon 0x14d): fire in its direction at the first actor within 6;
 * steel balls (quality) blow a door 0xad open (TalkFlags[0x41] 4); explosion
 * 0x107
 */
/*C_2F98_0503*/C_2F98_0503(int16 obj, int16 on, int16 to) {
	register int16 dir;
	register int16 i;
	int16 x, y;
	int16 far *hits;
	int16 typ, res, frame;

	res = 0;
	CON_printf(FireMsg);
	MUS_PlaySfx(0x30);
	dir = GetFrame(obj) << 1;
	x = (GetX(obj) + DirIncrX[dir]) & 0x3ff;
	y = (GetY(obj) + DirIncrY[dir]) & 0x3ff;
	for(i = 0; i < 6; i++) {
		if(OBJ_FindNPCAt(0, x, y) >= 0)
			break;
		x = (x + DirIncrX[dir]) & 0x3ff;
		y = (y + DirIncrY[dir]) & 0x3ff;
	}
	if(COMBAT_Missile(obj, &x, &y, 6, 0x105) == 0) {
		for(i = FindLoc(x, y, MapZ); i >= 0; i = NextLoc()) {
			if(GetType(i) == 0xad && (frame = GetFrame(i)) < 4) {
				if(GetQual(obj) == 0) {
					res = 1;
				} else {
					ObjShapeType[i] = GetType(i) + ((frame + 2) << 10);
					if(frame > 1)
						res = -1;
				}
			}
		}
		if(res != -1) {
			x = (x + DirIncrX[(dir + 4) & 7]) & 0x3ff;
			y = (y + DirIncrY[(dir + 4) & 7]) & 0x3ff;
		}
	}
	MUS_PlaySfx(0xb);
	for(hits = Explosion(x, y, 0x107); *hits >= 0; hits++) {
		typ = GetType(*hits);
		if((*hits < 0x100 && *hits >= 0) || (IsBreakableType(typ) && GetQuan(*hits) != 0))
			LoseHP(*hits, OSI_rand(1, 0xf) + OSI_rand(1, 0xf), 0);
	}
	if(res != 0) {
		if(res > 0) {
			CON_printf(SplatMsg);
			MUS_PlaySfx(5);
		} else {
			CON_printf(KaboomMsg);
			MUS_PlaySfx(4);
			TalkFlags[0x41] |= 4;
		}
	}
}

/*
 * action 41 (0x8a on the cannon 0x14d): load steel cannon balls (quality 1)
 */
/*C_2F98_0777*/C_2F98_0777(int16 obj, int16 on, int16 to) {
	register int16 q;

	if((q = GetQual(on)) != 0) {
		CON_printf(HasSteelBallsMsg);
	} else {
		SetQual(on, 1);
		CON_printf(SteelBallsMsg);
		DeleteObj(obj);
	}
}

/*
 * panel 4: the dream machines (chair 0x122, headgear 0x121/0x64, setting
 * 0x120; machine 0x123: NPC 0x21, robot 0x11f: NPC 6)
 */
/*C_2F98_07C3*/static C_2F98_07C3(int16 obj, int16 panel) {
	register int16 o;
	int16 typ, frame, x, y, chair = -1, mach = -1, c = -1;
	register int16 npc = -1;
	int16 q = -1, solo = -1;

	if(TalkFlags[0x46] & 8) {
		CON_printf(DreamMachinesCeasedMsg);
		return;
	}
	for(o = SearchArea((GetX(panel) - 5) & 0x3ff, (GetY(panel) - 5) & 0x3ff, (GetX(panel) + 5) & 0x3ff, (GetY(panel) + 5) & 0x3ff); o != -1; o = NextArea()) {
		typ = GetType(o);
		frame = GetFrame(o);
		if(typ == 0x122) {
			if(GetFrame(o) == 1)
				chair = o;
		} else if(typ == 0x121) {
			x = GetX(o);
			y = GetY(o);
		} else if(typ == 0x64) {
			c = o;
		} else if(typ == 0x120) {
			q = GetQual(o);
		}
	}
	npc = OBJ_FindNPCAt(1, x, y);
	if(chair == -1 || q == -1) {
		CON_printf(MachineDoesNotWorkMsg);
		return;
	}
	if(c == -1) {
		CON_printf(MachineDoesNotWorkMsg);
		return;
	}
	if((mach = OBJ_FindTypeAt(0x123, x, y)) >= 0) {
		npc = 0x21;
		if(TalkFlags[0x21] & 4 || !(TalkFlags[0x61] & 0x10)) {
			CON_printf(NoEffectMsg);
		} else {
			C_2F75_07AA();
			TALK_talkTo(1, 0x21, 0);
			TalkFlags[0x21] |= 4;
			MoveObj(0x61, 0x61, 0x61, 6);
			NPCMode[0x61] = NPCComMode[0x61] = 0;
			D_3262[0x61] |= 0xcb;
			MoveObj(npc, x, (y + 1) & 0x3ff, MapZ);
			COMBAT_Kill(npc);
			DeleteObj(mach);
		}
		return;
	}
	if((mach = OBJ_FindTypeAt(0x11f, x, y)) >= 0) {
		if(!GetQual(mach)) {
			CON_printf(NoEffectMsg);
			return;
		}
		npc = 6;
		C_2F75_1607();
		if(PartySize == 5) {
			if(SoloMember >= 0 && Party[SoloMember] == 5)
				SetPartyMode();
			if(Party[StatusMember] == 5) {
				StatusDisplay = 0x91;
				STAT_ShowParty(0);
			}
			solo = SoloMember;
			SoloMember = -1;
			TalkFlags[5] |= 4;
			TALK_talkTo(1, 5, 0);
			TalkFlags[5] &= ~4;
			SoloMember = solo;
		}
		TALK_talkTo(1, 6, 0);
		MoveObj(npc, x, (y + 1) & 0x3ff, MapZ);
		MoveObj(0x63, 0x63, 0x63, 6);
		NPCMode[0x63] = NPCComMode[0x63] = 0;
		D_3262[0x63] |= 0xcb;
		DeleteObj(mach);
		return;
	}
	if(npc == -1) {
		CON_printf(NobodyInMachineMsg);
		return;
	}
	if(MapZ != 0) {
		DreamWakeUp();
		return;
	}
	if(q < 2)
		TalkFlags[0x60] |= 8;
	if(q != 3) {
		DreamSleep(npc, q);
		return;
	}
	if(TalkFlags[0x46] & 1) {
		DreamEnter();
		return;
	}
	CON_printf(MachineDoesNotWorkMsg);
}

/*
 * panel 5: the rubber sprayer 0x114 coats NPC 6 (0x165 becomes 0x162)
 */
/*C_2F98_0B2F*/static C_2F98_0B2F(int16 obj, int16 panel) {
	register int16 spr, npc;
	int16 o, q;

	npc = -1;
	spr = -1;
	for(o = SearchArea((GetX(panel) - 5) & 0x3ff, (GetY(panel) - 5) & 0x3ff, (GetX(panel) + 5) & 0x3ff, (GetY(panel) + 5) & 0x3ff); o >= 0; o = NextArea())
		if(GetType(o) == 0x114)
			spr = o;
	if(spr == -1) {
		CON_printf(NoSprayerNearMsg);
		return;
	}
	if(!((q = GetQual(spr)) & 1)) {
		CON_printf(NoRubberMsg);
		return;
	}
	if(!(q & 2)) {
		if(TalkFlags[0x3e] & 0x10) {
			CON_printf(RubberNotFleshMsg);
			return;
		}
		TalkFlags[6] |= 4;
		TALK_talkTo(1, 6, 0);
		return;
	}
	if((npc = OBJ_FindNPCAt(0, GetX(spr), GetY(spr))) < 0) {
		CON_printf(NobodyUnderSprayerMsg);
		return;
	}
	if(npc != 6) {
		CON_printf(NoRubberCoatMsg);
		return;
	}
	if(GetType(npc) != 0x165) {
		CON_printf(ChsheketTreatedMsg);
		return;
	}
	ObjShapeType[npc] = (ObjShapeType[npc] & 0xfc00) + 0x162;
	TalkFlags[6] |= 0x22;
	OrigShapeType[6] = 0x162 | (OrigShapeType[6] >> 10 << 10);
	C_2F75_0A64();
	TALK_talkTo(1, 6, 0);
	TalkFlags[6] &= ~0x20;
}

/*
 * panel 6: align the ruby lenses (a coordinate within 5 of the time
 * H * 100 + M; at night wait until dawn)
 */
/*C_2F98_0CF3*/C_2F98_0CF3() {
	register int16 t;
	int16 n;
	char buf[10];

	if(TalkFlags[0x76] & 0x20) {
		CON_printf(LensesAlignedMsg);
	} else if(TalkFlags[0x2e] & 0x80 && TalkFlags[0x39] & 0x80 && TalkFlags[0x30] & 0x80) {
		CON_printf(AlignmentMsg);
		CurClip->fgColor = ColorDarkBlue;
		CON_gets(buf, 10);
		n = ParseNumberInput(buf, 0);
		CurClip->fgColor = ColorText;
		if((t = Time_H * 100 + Time_M) - 5 < n && t + 5 > n) {
			if(Time_H >= 18 || Time_H <= 5) {
				CON_printf(WaitUntilDawnMsg);
				IsResting = 1;
				PassTime((28 - Time_H) * 60 + (60 - Time_M));
				IsResting = 0;
			}
			TalkFlags[0x2e] |= 0x80;
			TalkFlags[0x39] |= 0x80;
			TalkFlags[0x30] |= 0x80;
			TalkFlags[0x76] |= 0x20;
			BargePumps(1);
			C_2F75_07FA();
			TalkFlags[5] |= 0x10;
			TALK_talkTo(1, 5, 0);
			TalkFlags[5] &= ~0x10;
		} else {
			CON_printf(NoEffectMsg);
		}
	} else {
		CON_printf(LensesNotRespondingMsg);
	}
}

/*
 * action 45 (panel 0x1ca on a cabinet 0x1c9): by the cabinet's quality
 */
/*C_2F98_0E53*/C_2F98_0E53(int16 obj, int16 on, int16 to) {
	register int16 cab = -1;
	register int16 q;

	if((q = GetQual(obj)) & 2) {
		CON_printf(PanelBrokenMsg);
		return;
	}
	if(q & 1) {
		CON_printf(PanelNotInstalledMsg);
		return;
	}
	if((cab = OBJ_FindTypeAt(0x1c9, GetX(obj), GetY(obj))) == -1) {
		CON_printf(PanelsOnCabinetsMsg);
		return;
	}
	q = GetQual(cab);
	switch(q) {
	case 0:
		CON_printf(UnsureWhatYouDidMsg);
		break;
	case 1:
		CON_printf(NoApparentEffectMsg);
		break;
	case 2:
		CON_printf(StatusLightsChangeMsg);
		break;
	case 3:
		CON_printf(LightsFlashMsg);
		break;
	case 4:
		C_2F98_07C3(obj, cab);
		break;
	case 5:
		C_2F98_0B2F(obj, cab);
		break;
	case 6:
		C_2F98_0CF3();
		break;
	case 7:
		if(TalkFlags[0x76] & 0x40) {
			CON_printf(CanalsWorkingMsg);
		} else if(TalkFlags[0x76] & 0x20) {
			TalkFlags[0x76] |= 0x40;
			BargePumps(0);
			C_2F75_0951();
		} else {
			CON_printf(NoWaterPressureMsg);
		}
		break;
	default:
		CON_printf(NoEffectMsg);
	}
}

/*
 * action 48 (paint 0x77, weed killer 0x80 on spray guns 0x81/0x105): charge
 */
/*C_2F98_0F88*/C_2F98_0F88(int16 obj, register int16 on, int16 to) {
	register int16 t;
	int16 x, y, p, quan, qual;

	p = Party[Active];
	x = GetX(p);
	y = GetY(p);
	t = GetType(obj);
	quan = GetQuan(on);
	qual = GetQual(on);
	if(t == 0x77 && qual == 0) {
		CON_printf(SprayGunPaintMsg);
		SetQuan(on, 20);
		SetQual(on, 1);
	} else if(t == 0x80 && qual != 0) {
		CON_printf(SprayGunWeedKillerMsg);
		SetQuan(on, 10);
		SetQual(on, 0);
	} else {
		quan += 10;
		if(quan > 30)
			quan = 30;
		SetQuan(on, quan);
		CON_printf(SprayGunRefillMsg);
	}
	DeleteObj(obj);
}

/*
 * action 49 (spray guns 0x105, 0x81): paint (quality 1) shows invisible
 * actors, weed killer hurts actors and kills plants (IsPlantType)
 */
/*C_2F98_1088*/C_2F98_1088(int16 obj, register int16 on, int16 to) {
	register int16 t;
	int16 quan, qual;

	quan = GetQuan(obj);
	qual = GetQual(obj);
	if(quan == 0) {
		CON_printf(SprayGunEmptyMsg);
		return;
	}
	t = GetType(on);
	if(qual != 0) {
		SetQuan(obj, quan - 1);
		if(on < 0x100 && on >= 0 && t == 0x91) {
			ObjShapeType[on] = (ObjShapeType[on] & 0xfc00) + 0x16c;
			CON_printf(BecomesVisibleMsg, GetObjectString(on));
		} else if(ObjStatus[on] & 2) {
			ObjStatus[on] &= ~2;
			CON_printf(BecomesVisibleMsg, GetObjectString(on));
		} else {
			CON_printf(NoEffectMsg);
		}
	} else {
		if(on < 0x100 && on >= 0) {
			__TryHit(Party[Active], GetX(on), GetY(on), obj);
		} else if(IsPlantType(t)) {
			SetQuan(obj, quan - 1);
			COMBAT_showHit(on);
			CON_printf(KilledPlantMsg);
			if(t == 0xcd) {
				TalkFlags[0x30] |= 0x80;
				DeleteObj(on);
			} else if(t == 0x198) {
				ObjShapeType[on] = (ObjShapeType[on] & 0xfc00) + 0xa6;
			} else {
				DeleteObj(on);
			}
		} else {
			SetQuan(obj, quan - 1);
			CON_printf(NoEffectMsg);
		}
	}
}

/*
 * action 52 (headgear 0x128 on 0x120/0x121): install it (0x64, frame 1);
 * TalkFlags[0x74] 0x10
 */
/*C_2F98_1237*/C_2F98_1237(int16 obj, register int16 on, int16 to) {
	register int16 o;

	if(GetType(on) == 0x121) {
		if((o = OBJ_FindTypeAt(0x120, GetX(on), GetY(on))) == -1) {
			CON_printf(NoEffectMsg);
			return;
		}
		on = o;
	}
	if((o = AddMapObj(0x64, 1, 0, GetX(on), GetY(on), MapZ)) == -1) {
		CON_printf(NotNowMsg);
		return;
	}
	ObjStatus[o] &= ~0x20;
	DeleteObj(obj);
	ObjShapeType[o] = GetType(o) + 0x400;
	CON_printf(HeadgearInstalledMsg);
	if((TalkFlags[0x74] |= 0x10) & 1)
		TalkFlags[0x60] |= 4;
}

/*
 * action 23 (lever 0x1a2): set the frame to a direction (AdvanceDir) or 4
 * (center); needs TalkFlags[0x76] 0x20 and 0x40, or the dream
 */
/*C_2F98_1334*/C_2F98_1334(register int16 obj, int16 on, int16 to) {
	register int16 key;

	if((!(TalkFlags[0x76] & 0x20) || !(TalkFlags[0x76] & 0x40)) && !(GameFlags & 0x10)) {
		CON_printf(LeverStuckMsg);
		return;
	}
	MouseMode = 0;
	DirOriginX = (WrapDelta(GetX(obj) - MapX) << 4) + 0x58;
	DirOriginY = (WrapDelta(GetY(obj) - MapY) << 4) + 0x58;
	CON_printf(PushLeverMsg);
	key = CON_getch();
	MUS_PlaySfx(0x11);
	DirOriginX = 0x58;
	DirOriginY = 0x58;
	CON_UpdateMousePointer();
	if(key != 0x80) {
		CON_printf("center.\n");
		ObjShapeType[obj] = GetType(obj) + 0x1000;
	} else {
		CON_printf("%s.\n", DirNames[AdvanceDir]);
		ObjShapeType[obj] = GetType(obj) + (AdvanceDir >> 1 << 10);
	}
}

/*
 * action 33 (0x60): latitude and longitude of the party (not underground)
 */
/*C_2F98_1449*/C_2F98_1449(int16 obj, int16 on, int16 to) {
	register int16 lon, lat;
	unsigned char ns, ew;
	int16 p;

	if(MapZ != 0) {
		CON_printf(NotUndergroundMsg);
		return;
	}
	ns = 'N';
	ew = 'W';
	p = Party[Active];
	lon = (GetX(p) - 0x200L) * 0x168 / 0x400;
	lat = (GetY(p) - 0x200L) * 0xf0 / 0x400;
	if(lat > 0)
		ns = 'S';
	else if(lat == 0)
		ns = ' ';
	if(lon == 0xb4 || lon == -0xb4 || lon == 0)
		ew = ' ';
	else if(lon < 0)
		ew = 'E';
	lon = 0xb4 - (abs)(lon);
	lat = (abs)(lat);
	MUS_PlaySfx(4);
	CON_printf(LatLongMsg, lat, ns, lon, ew);
}

/*
 * action 50 (bin 0xd4): 20 oxium 0x83 into the inventory
 */
/*C_2F98_1568*/C_2F98_1568(int16 obj, int16 on, int16 to) {
	register int16 p, d;
	int16 x, y;

	p = Party[Active];
	x = GetX(obj);
	y = GetY(obj);
	if((d = COMBAT_range(x, y, GetX(p), GetY(p))) > 1 && COMBAT_Missile(p, &x, &y, 9, 0) == 0) {
		CON_printf(BlockedMsg);
		return;
	}
	if(AddInvObj(0x83, 0x10, 0x14, p) == -1) {
		CON_printf(TooMuchMsg);
		return;
	}
	if(d > 1)
		COMBAT_Missile(p, &x, &y, 9, BaseTile[0x83]);
	CON_printf(OxiumFromBinMsg);
	TalkFlags[0x12] |= 0x10;
}

/*
 * action 58 (plant 0x9d): harvest a seed 0x9e (frame >= 1; frame 0 and
 * quality 10-15 again)
 */
/*C_2F98_166A*/C_2F98_166A(register int16 obj, int16 on, int16 to) {
	register int16 p;
	int16 o, d, x, y;

	p = Party[Active];
	x = GetX(obj);
	y = GetY(obj);
	if((d = COMBAT_range(x, y, GetX(p), GetY(p))) > 1 && COMBAT_Missile(p, &x, &y, 9, 0) == 0) {
		CON_printf(BlockedMsg);
		return;
	}
	if(GetFrame(obj) < 1) {
		CON_printf(NoSeedYetMsg);
		return;
	}
	if((o = AddInvObj(0x9e, 0x10, 1, p)) < 0) {
		CON_printf(TooMuchMsg);
		return;
	}
	if(d > 1)
		COMBAT_Missile(p, &x, &y, 9, BaseTile[0x9e]);
	ObjShapeType[obj] = GetType(obj);
	SetQual(obj, OSI_rand(0xa, 0xf));
	ShowObjects();
	if(StatusDisplay == 0x92)
		StatusDirty = 1;
	MapAnimEnabled = 1;
}

/*
 * action 59 (seed 0x9e): plant it on the selected square
 */
/*C_2F98_17B5*/C_2F98_17B5(register int16 obj, int16 on, int16 to) {
	register int16 p;
	int16 key, x, y, d;

	p = Party[Active];
	CON_printf(LocationMsg);
	MouseMode = 1;
	if(SelectMode) {
		DrawSlotCursor(0);
		SelectMode = 1;
		AimX = AimY = 5;
	}
	if((key = CON_getch()) != 0x8e || SelectionX < 0 || SelectionY < 0) {
		CON_printf(NowhereMsg);
		return;
	}
	CON_putch('\n');
	x = SelectionX;
	y = SelectionY;
	if((d = COMBAT_range(x, y, GetX(p), GetY(p))) > 1 && COMBAT_Missile(p, &x, &y, 9, 0) == 0) {
		CON_printf(BlockedMsg);
		MUS_PlaySfx(5);
		return;
	}
	if(!GetDigClass(GetTileAtXYZ(x, y, MapZ))) {
		CON_printf(WontGrowMsg);
		MUS_PlaySfx(5);
		return;
	}
	if(d > 1)
		COMBAT_Missile(p, &x, &y, 9, BaseTile[0x9e]);
	if(ObjStatus[obj] & 0x18)
		StatusDirty = 1;
	if(GetQuan(obj) > 1) {
		SetQuan(obj, GetQuan(obj) - 1);
		if((obj = AddMapObj(0x9e, 0, 1, x, y, MapZ)) < 0)
			CON_printf(AarghMsg);
	} else {
		MoveObj(obj, x, y, MapZ);
	}
	ShowObjects();
	MapAnimEnabled = 1;
	CON_printf(NeedsWateringMsg);
}

/*
 * action 66 (spittoon 0x11e)
 */
/*C_2F98_1988*/C_2F98_1988(int16 obj, int16 on, int16 to) {
	if(ObjStatus[Party[Active]] & 0x40)
		CON_printf(MouthTooDryMsg);
	else
		CON_printf(SpitMsg);
}

/*
 * action 67 (ruby slippers 0xc): readied, click three times (quality), then
 * the Grand Finale (C_2F75_16A8) or not (C_2F75_1657)
 */
/*C_2F98_19BD*/C_2F98_19BD(register int16 obj, int16 on, int16 to) {
	register int16 ch;
	int16 q;

	if((ObjStatus[obj] & 0x18) != 0x18) {
		C_2F8D_0476(obj, -1, -1);
		return;
	}
	if((q = GetQual(obj)) < 2) {
		SetQual(obj, q + 1);
		CON_printf(ClickMsg);
		return;
	}
	CON_printf(RubySlippersMsg);
	CON_printf(FinaleExitMsg);
	CON_printf(SeeFinaleMsg);
	for(ch = 0; ch != 'Y' && ch != 'N'; ch = toupper(CON_prompt()))
		;
	if(ch == 'N')
		C_2F75_1657();
	else
		C_2F75_16A8();
}

/*
 * action 54 (dreamstuff hoard 0x1c7): with the psychic talent, a dreamstuff
 * 0x14b holding 0x139 (the hoard's quality) or a random 0x31-0x38
 */
/*C_2F98_1A75*/C_2F98_1A75(int16 obj, int16 on, int16 to) {
	register int16 p, d;
	int16 o;

	if(!Talents[Party[Active]].psychic) {
		CON_printf(LackMentalPowerMsg);
		return;
	}
	p = Party[Active];
	if((d = COMBAT_range(SelectionX, SelectionY, GetX(p), GetY(p))) > 1 && COMBAT_Missile(p, &SelectionX, &SelectionY, 9, 0) == 0) {
		CON_printf(BlockedMsg);
		return;
	}
	if((o = AddInvObj(0x14b, 0x10, 0, p)) == -1) {
		CON_printf(TooMuchMsg);
		return;
	}
	if(d > 1)
		COMBAT_Missile(p, &SelectionX, &SelectionY, 9, BaseTile[0x14b]);
	CON_printf(PullDreamstuffMsg);
	if(AddInvObj(GetQual(obj) ? 0x139 : OSI_rand(0x31, 0x38), 8, 0, o) >= 0)
		SetQual(obj, 0);
	else
		CON_printf(AarghMsg);
}

/*
 * action 53 (dreamstuff 0x14b): imagine what it holds: an object (< 0x156) in
 * its place, else an actor next to the party (alignment from the quality)
 */
/*C_2F98_1BB6*/C_2F98_1BB6(int16 obj, int16 on, int16 to) {
	register int16 npc, inner;
	int16 t, i, x, y;

	MUS_PlaySfx(0x32);
	CON_printf(ImagineMsg);
	if((inner = FindInv(obj)) < 0) {
		CON_printf(NothingMsg);
		return;
	}
	if((t = GetType(inner)) < 0x156) {
		PrintObjCountOrArticle(inner);
		CON_printf("%s\n", GetObjectString(inner));
		if((ObjStatus[obj] & 0x18) == 0)
			MoveObj(inner, GetX(obj), GetY(obj), GetZ(obj));
		else
			InsertObj(inner, GetAssoc(obj), ObjStatus[obj] & 0x18);
		DeleteObj(obj);
		return;
	} else {
		if((npc = EGG_generate(t, MapX, MapY, MapZ)) < 0) {
			CON_printf(NothingMsg);
			return;
		}
		ObjShapeType[npc] = GetType(npc) + (GetFrame(inner) << 10);
		if(!(ObjStatus[inner] & 0x20))
			ObjStatus[npc] &= ~0x20;
		for(i = 0; i <= 7; i++) {
			x = MapX + DirIncrX[i];
			y = MapY + DirIncrY[i];
			if(CanMoveTo(npc, x, y)) {
				MoveObj(npc, x, y, MapZ);
				break;
			}
		}
		if(i > 7 && !COMBAT_TryTeleport(npc, 0)) {
			CON_printf(NothingMsg);
			DeleteObj(npc);
			return;
		}
		switch(GetQual(obj)) {
		case 1:
			NPCStatus[npc] &= 0x9f;
			break;
		case 2:
			NPCStatus[npc] = NPCStatus[npc] & 0x9f | 0x20;
			break;
		case 3:
			NPCStatus[npc] = NPCStatus[npc] & 0x9f | 0x40;
			break;
		case 4:
			NPCStatus[npc] = NPCStatus[npc] & 0x9f | 0x60;
			break;
		}
		NPCComMode[npc] = NPCMode[npc] = GetQual(inner);
		if(ObjStatus[inner] & 0x80)
			ObjStatus[npc] |= 0x80;
		PrintTileArticle(inner);
		CON_printf("%s\n", GetObjectString(inner));
		DeleteObj(inner);
	}
	DeleteObj(obj);
}

/*
 * action 55 (0x79 on an object): NPC 0x68 talks; on object 0 heals (NPC 1's
 * MaxHP and name, NPC 0's HitPoints, as the original)
 */
/*C_2F98_1E08*/C_2F98_1E08(int16 obj, int16 on, int16 to) {
	if(SelectionObj == 0x68) {
		TalkFlags[0x68] |= 0x40;
		TALK_talkTo(Active, 0x68, 0);
	} else if(SelectionObj == 0 && MaxHP(1) > HitPoints[0]) {
		CON_printf(HealedMsg, GetObjectString(1));
		HitPoints[0] = MaxHP(1);
	} else {
		CON_printf(NoEffectMsg);
	}
}

/*
 * action 72 (valve 0xdc): start the canals (as panel 7)
 */
/*C_2F98_1E83*/C_2F98_1E83(int16 obj, int16 on, int16 to) {
	if(TalkFlags[0x76] & 0x40) {
		CON_printf(CanalsWorkingMsg);
	} else if(TalkFlags[0x76] & 0x20) {
		TalkFlags[0x76] |= 0x40;
		BargePumps(0);
		C_2F75_0951();
	} else {
		CON_printf(NoWaterPressureMsg);
	}
}

/*
 * action 74 (oxygenated air 0x144 on an object): revive a body 0x155, heal an
 * actor (NPCStatus 0x80); n is tested uninitialised (the original's bug)
 */
/*C_2F98_1EC7*/C_2F98_1EC7(int16 obj, register int16 on, int16 to) {
	register int16 n;
	int16 t;

	if((t = GetType(on)) != 0x155 && (on >= 0x100 || on < 0 || !(NPCStatus[on] & 0x80))) {
		CON_printf(NoAirNeededMsg, GetObjectString(on));
		MUS_PlaySfx(5);
		return;
	}
	if(t == 0x155) {
		if(n < 0x10) {
			if((n = SPELL_ReviveNPC(on)) < 0) {
				CON_printf(BeyondHelpMsg, GetObjectString(on));
				MUS_PlaySfx(5);
			} else {
				CON_printf(RevivedMsg, GetObjectString(n));
				MUS_PlaySfx(4);
			}
		} else {
			CON_printf(BeyondHelpMsg, GetObjectString(on));
			MUS_PlaySfx(5);
		}
	} else {
		CON_printf(RevivedMsg, GetObjectString(on));
		MUS_PlaySfx(4);
		HitPoints[on] = MaxHP(on);
	}
	if(StatusDisplay == 0x91 || StatusDisplay == 0x90 && Party[StatusMember] == on)
		StatusDirty = 1;
	if(GetQuan(obj) == 1) {
		DeleteObj(obj);
		return;
	}
	SetQuan(obj, GetQuan(obj) - 1);
}
