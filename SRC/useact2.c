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
extern int16 C_100B_0410(int16);
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
extern int16 C_100B_038E(int16);/*breakable type*/
extern int16 OBJ_FindNPCAt(int16, int16, int16);/*OBJ_FindNPCAt(startNum, x, y)*/
extern int16 KBD_GetKey(int16);
extern MUS_PlaySong(int16);
extern MUS_PlayAreaSong();
extern C_1FA9_0C15();
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
extern int16 Class_20_3F_7C_7F(int16);
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
extern unsigned char D_0F12;/*StatusDirty*/
extern int16 D_0F18;/*MapAnimEnabled*/
extern unsigned char Active;
extern int16 MapX, MapY, MapZ;
extern char SoloMember;
extern int32 D_16AA;/*flags (0x10: the dream)*/
extern int16 StatusMember;
extern int16 StatusDisplay;
extern unsigned char far *D_3262;/*NPC flags: 0x80 ?, bits 0-7: talk flags 8-15*/
extern unsigned char far *D_46D0;/*NPCMode*/
extern unsigned char far *D_53AE;/*NPCComMode*/
extern int16 far *D_B508;/*OrigShapeType*/
extern int16 ColorText;
extern int16 D_0826;/*color of the player's input*/
extern unsigned char Time_M, Time_H;
extern int16 MouseMode;
extern unsigned char DirOriginX, DirOriginY;/*screen position of the party*/
extern char *DirNames[];
extern int16 D_EB72;/*AdvanceDir*/
extern unsigned char D_6DBE[];/*HitPoints*/
extern int16 D_EBCE;/*Selection.obj*/
extern struct { unsigned a:4, psychic:4, c:4, d:4; } D_16B0[];/*talents*/
extern char SelectMode;
extern char AimX, AimY;
extern int16 D_EBF6, D_EBF8;/*Selection.x, Selection.y*/
extern unsigned char IsResting;
extern struct tClipInfo *CurClip;
extern unsigned char D_3266[];/*ObjStatus*/
extern unsigned char D_4EA4[];/*Party*/
extern int16 D_54BA[];/*Amount: quantity (low byte), quality (high byte)*/
extern unsigned char D_6CBA[];/*NPCStatus*/
extern int16 far *D_6EBE;/*BaseTile*/
extern unsigned char D_6EC6[];/*TalkFlags*/
extern struct coord D_78AE[];/*ObjPos*/
extern unsigned char D_9CC0;/*PartySize*/
extern uint16 D_9CC4[];/*ObjShapeType*/

#define MapAnimEnabled D_0F18
#define ObjStatus D_3266
#define OrigShapeType D_B508
#define NPCMode D_46D0
#define NPCComMode D_53AE
#define Party D_4EA4
#define Amount D_54BA
#define NPCStatus D_6CBA
#define BaseTile D_6EBE
#define TalkFlags D_6EC6
#define ObjPos D_78AE
#define PartySize D_9CC0
#define ObjShapeType D_9CC4

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
#define HitPoints D_6DBE

/*messages (segment 2BB4)*/
extern char far C_2BB4_0230[];/*"Blocked!\n"*/
extern char far C_2BB4_024E[];/*"You are carrying too much already.\n"*/
extern char far C_2BB4_02A4[];/*"It has no effect.\n"*/
extern char far C_2BB4_032E[];/*"Not now.\n"*/
extern char far C_2BB4_0364[];/*"Not while underground.\n"*/
extern char far C_2BB4_03F8[];/*"nowhere.\n"*/
extern char far C_2BB4_0402[];/*"nothing!\n"*/
extern char far C_2BB4_04D3[];/*"\nFire!\n"*/
extern char far C_2BB4_04DB[];/*"Splat! There is no effect.\n"*/
extern char far C_2BB4_04F7[];/*"Kaboom! You blew the door down!\n"*/
extern char far C_2BB4_0518[];/*"It already has steel cannon balls.\n"*/
extern char far C_2BB4_053C[];/*"The cannon will fire steel cannon balls.\n"*/
extern char far C_2BB4_0566[];/*"Press keys 1-0 to play -"*/
extern char far C_2BB4_057F[];/*"Tocatta and Fugue in D minor (J.S. Bach).\n"*/
extern char far C_2BB4_09E3[];/*"The panel is broken.\n"*/
extern char far C_2BB4_09F9[];/*"The panel is not installed.\n"*/
extern char far C_2BB4_0A16[];/*"Panels are only installed onto cabinets.\n"*/
extern char far C_2BB4_0C5F[];/*"the lever seems stuck.\n"*/
extern char far C_2BB4_0C77[];/*"Push lever to "*/
extern char far C_2BB4_0C86[];/*"You are somewhere near \nLat:%d %c\nLong:%d %c\n"*/
extern char far C_2BB4_0CB4[];/*"You need a cue stick in your hands.\n"*/
extern char far C_2BB4_0CD9[];/*"Organ music must be played on a pipe organ.\n"*/
extern char far C_2BB4_0DD5[];/*"The headgear is installed.\n"*/
extern char far C_2BB4_0DF1[];/*"The machine does not work.\n"*/
extern char far C_2BB4_0E0D[];/*"There is nobody sitting in the machine.\n"*/
extern char far C_2BB4_0E36[];/*"The dream machines seem to have ceased functioning.\n"*/
extern char far C_2BB4_0EB0[];/*"Nobody is standing under the sprayer.\n"*/
extern char far C_2BB4_0ED7[];/*"You don't think it would be safe to coat an animal or plant with rubber.\n"*/
extern char far C_2BB4_0F21[];/*"Chsheket has already been treated.\n"*/
extern char far C_2BB4_0F45[];/*"There is no rubber in the sprayer system.\n"*/
extern char far C_2BB4_0F70[];/*"The rubber in the sprayer system is not flesh-colored.\n"*/
extern char far C_2BB4_0FC7[];/*"You killed the plant.\n"*/
extern char far C_2BB4_0FDE[];/*"`%s becomes visible!\n"*/
extern char far C_2BB4_0FF4[];/*"Ooops. These rollers can never be fixed!\n"*/
extern char far C_2BB4_101E[];/*"You get two handfuls of oxium from the bin.\n"*/
extern char far C_2BB4_107B[];/*"You imagine "*/
extern char far C_2BB4_1088[];/*"You pull some of the dreamstuff from the hoard.\n"*/
extern char far C_2BB4_10B9[];/*"You lack the mental power to affect the dreamstuff hoard.\n"*/
extern char far C_2BB4_10F4[];/*"Some of the lenses are not responding.\n"*/
extern char far C_2BB4_111C[];/*"There is no water pressure in the valve system.\n"*/
extern char far C_2BB4_12C8[];/*"The canals are already working.\n"*/
extern char far C_2BB4_12E9[];/*"The ruby lenses are already aligned.\n"*/
extern char far C_2BB4_130F[];/*"Good shot, old bean!\n"*/
extern char far C_2BB4_13A1[];/*"\n%s is healed.\n"*/
extern char far C_2BB4_166B[];/*"You actuate the mechanism to no apparent effect.\n"*/
extern char far C_2BB4_169D[];/*"Status lights change but you see no other effect.\n"*/
extern char far C_2BB4_16D0[];/*"Lights flash and change color but nothing else happens.\n"*/
extern char far C_2BB4_1709[];/*"You are completely unsure what you just did.\n"*/
extern char far C_2BB4_19DD[];/*"Funny, no sprayer near sprayer controls!\n"*/
extern char far C_2BB4_1A07[];/*"Alignment coordinate:\n"*/
extern char far C_2BB4_1A1E[];/*"You wait until dawn...*\n"*/
extern char far C_2BB4_1A37[];/*"No seed to harvest yet.\n"*/
extern char far C_2BB4_1A50[];/*"Location: "*/
extern char far C_2BB4_1A5B[];/*"Won't grow there!\n"*/
extern char far C_2BB4_1A6E[];/*"Now it just needs watering.\n"*/
extern char far C_2BB4_1A8B[];/*"Your mouth is too dry.\n"*/
extern char far C_2BB4_1AA3[];/*"You spit into the spittoon.\n"*/
extern char far C_2BB4_1AC0[];/*"Click.\n"*/
extern char far C_2BB4_1AC8[];/*"You may use the ruby slippers to go home. If you choose to, you may now view the Grand Finale.\n\n*"*/
extern char far C_2BB4_1B2A[];/*"Afterwards, you will exit to DOS. You will be able to resume play from your last saved game.\n\n*"*/
extern char far C_2BB4_1B8A[];/*"Would you like to see the Grand Finale now? If so, press Y. To resume play now, hit N: (Y N)\n"*/
extern char far C_2BB4_2016[];/*"The spray gun has 10 more charges.\n"*/
extern char far C_2BB4_203A[];/*"The spray gun now contains 20 charges of green paint.\n"*/
extern char far C_2BB4_2071[];/*"The spray gun now contains 10 charges of weed killer.\n"*/
extern char far C_2BB4_20A8[];/*"There is nothing in the gun.\n"*/
extern char far C_2BB4_228A[];/*"The %s doesn't need oxygenated air.\n"*/
extern char far C_2BB4_22AF[];/*"%s is beyond help.\n"*/
extern char far C_2BB4_22C3[];/*"%s is revived!\n"*/
extern char far C_2BB4_22F1[];/*"Aargh!\n"*/

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
		CON_printf(C_2BB4_0CB4);
		return;
	}
	x = GetX(obj);
	y = GetY(obj);
	if((b1 = OBJ_FindTypeAt(0x190, (x - 1) & 0x3ff, y)) < 0)
		b1 = AddMapObj(0x190 | (OSI_rand(0, frames) << 10), 0, 0, (x - 1) & 0x3ff, y, MapZ);
	if(b1 < 0) {
		CON_printf(C_2BB4_22F1);
		return;
	}
	if((b2 = OBJ_FindTypeAt(0x190, x, y)) < 0)
		b2 = AddMapObj(0x190 | (OSI_rand(0, frames) << 10), 0, 0, x, y, MapZ);
	if(b2 < 0) {
		CON_printf(C_2BB4_22F1);
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
		CON_printf(C_2BB4_130F);
}

/*
 * action 36 (pipe organ 0xa5, its seat 0x199): with the seat left of it,
 * Bach (C_1FA9_0C15); else keys 1-0 play sfx key - 10
 */
/*C_2F98_0267*/C_2F98_0267(register int16 obj, int16 on, int16 to) {
	register int16 seat = -1;
	int16 o, done, key;

	if(GetType(obj) == 0x199) {
		if((ObjStatus[obj] & 0x18) == 0) {
			seat = obj;
			if((o = OBJ_FindTypeAt(0xa5, (GetX(obj) + 1) & 0x3ff, GetY(obj))) < 0) {
				CON_printf(C_2BB4_0CD9);
				return;
			}
			obj = o;
		} else {
			CON_printf(C_2BB4_0CD9);
			return;
		}
	}
	if(seat == -1) {
		for(seat = FindLoc(GetX(obj) - 1, GetY(obj), MapZ); seat != -1 && GetType(seat) != 0x199; seat = NextLoc())
			;
	}
	if(seat >= 0) {
		CON_printf(C_2BB4_057F);
		C_1FA9_0C15();
	} else {
		MUS_PlaySong(-1);
		CON_printf(C_2BB4_0566);
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
		CON_printf(C_2BB4_0FF4);
		return;
	}
	for(r = o, cnt = 4; r >= 0; r = OBJ_FindTypeAt(0xc0, x, y)) {
		ObjShapeType[r] = (ObjShapeType[r] & 0xfc00) + 0xbc;
		if(cnt == 0) {
			cnt = 4;
			if((n = AddMapObj(0xbd, 1, 0, x, y, MapZ)) >= 0)
				ObjStatus[n] &= ~0x20;
			else
				CON_printf(C_2BB4_22F1);
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
	CON_printf(C_2BB4_04D3);
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
		if((*hits < 0x100 && *hits >= 0) || (C_100B_038E(typ) && GetQuan(*hits) != 0))
			LoseHP(*hits, OSI_rand(1, 0xf) + OSI_rand(1, 0xf), 0);
	}
	if(res != 0) {
		if(res > 0) {
			CON_printf(C_2BB4_04DB);
			MUS_PlaySfx(5);
		} else {
			CON_printf(C_2BB4_04F7);
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
		CON_printf(C_2BB4_0518);
	} else {
		SetQual(on, 1);
		CON_printf(C_2BB4_053C);
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
		CON_printf(C_2BB4_0E36);
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
		CON_printf(C_2BB4_0DF1);
		return;
	}
	if(c == -1) {
		CON_printf(C_2BB4_0DF1);
		return;
	}
	if((mach = OBJ_FindTypeAt(0x123, x, y)) >= 0) {
		npc = 0x21;
		if(TalkFlags[0x21] & 4 || !(TalkFlags[0x61] & 0x10)) {
			CON_printf(C_2BB4_02A4);
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
			CON_printf(C_2BB4_02A4);
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
		CON_printf(C_2BB4_0E0D);
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
	CON_printf(C_2BB4_0DF1);
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
		CON_printf(C_2BB4_19DD);
		return;
	}
	if(!((q = GetQual(spr)) & 1)) {
		CON_printf(C_2BB4_0F45);
		return;
	}
	if(!(q & 2)) {
		if(TalkFlags[0x3e] & 0x10) {
			CON_printf(C_2BB4_0F70);
			return;
		}
		TalkFlags[6] |= 4;
		TALK_talkTo(1, 6, 0);
		return;
	}
	if((npc = OBJ_FindNPCAt(0, GetX(spr), GetY(spr))) < 0) {
		CON_printf(C_2BB4_0EB0);
		return;
	}
	if(npc != 6) {
		CON_printf(C_2BB4_0ED7);
		return;
	}
	if(GetType(npc) != 0x165) {
		CON_printf(C_2BB4_0F21);
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
		CON_printf(C_2BB4_12E9);
	} else if(TalkFlags[0x2e] & 0x80 && TalkFlags[0x39] & 0x80 && TalkFlags[0x30] & 0x80) {
		CON_printf(C_2BB4_1A07);
		CurClip->fgColor = D_0826;
		CON_gets(buf, 10);
		n = ParseNumberInput(buf, 0);
		CurClip->fgColor = ColorText;
		if((t = Time_H * 100 + Time_M) - 5 < n && t + 5 > n) {
			if(Time_H >= 18 || Time_H <= 5) {
				CON_printf(C_2BB4_1A1E);
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
			CON_printf(C_2BB4_02A4);
		}
	} else {
		CON_printf(C_2BB4_10F4);
	}
}

/*
 * action 45 (panel 0x1ca on a cabinet 0x1c9): by the cabinet's quality
 */
/*C_2F98_0E53*/C_2F98_0E53(int16 obj, int16 on, int16 to) {
	register int16 cab = -1;
	register int16 q;

	if((q = GetQual(obj)) & 2) {
		CON_printf(C_2BB4_09E3);
		return;
	}
	if(q & 1) {
		CON_printf(C_2BB4_09F9);
		return;
	}
	if((cab = OBJ_FindTypeAt(0x1c9, GetX(obj), GetY(obj))) == -1) {
		CON_printf(C_2BB4_0A16);
		return;
	}
	q = GetQual(cab);
	switch(q) {
	case 0:
		CON_printf(C_2BB4_1709);
		break;
	case 1:
		CON_printf(C_2BB4_166B);
		break;
	case 2:
		CON_printf(C_2BB4_169D);
		break;
	case 3:
		CON_printf(C_2BB4_16D0);
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
			CON_printf(C_2BB4_12C8);
		} else if(TalkFlags[0x76] & 0x20) {
			TalkFlags[0x76] |= 0x40;
			BargePumps(0);
			C_2F75_0951();
		} else {
			CON_printf(C_2BB4_111C);
		}
		break;
	default:
		CON_printf(C_2BB4_02A4);
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
		CON_printf(C_2BB4_203A);
		SetQuan(on, 20);
		SetQual(on, 1);
	} else if(t == 0x80 && qual != 0) {
		CON_printf(C_2BB4_2071);
		SetQuan(on, 10);
		SetQual(on, 0);
	} else {
		quan += 10;
		if(quan > 30)
			quan = 30;
		SetQuan(on, quan);
		CON_printf(C_2BB4_2016);
	}
	DeleteObj(obj);
}

/*
 * action 49 (spray guns 0x105, 0x81): paint (quality 1) shows invisible
 * actors, weed killer hurts actors and kills plants (C_100B_0410)
 */
/*C_2F98_1088*/C_2F98_1088(int16 obj, register int16 on, int16 to) {
	register int16 t;
	int16 quan, qual;

	quan = GetQuan(obj);
	qual = GetQual(obj);
	if(quan == 0) {
		CON_printf(C_2BB4_20A8);
		return;
	}
	t = GetType(on);
	if(qual != 0) {
		SetQuan(obj, quan - 1);
		if(on < 0x100 && on >= 0 && t == 0x91) {
			ObjShapeType[on] = (ObjShapeType[on] & 0xfc00) + 0x16c;
			CON_printf(C_2BB4_0FDE, GetObjectString(on));
		} else if(ObjStatus[on] & 2) {
			ObjStatus[on] &= ~2;
			CON_printf(C_2BB4_0FDE, GetObjectString(on));
		} else {
			CON_printf(C_2BB4_02A4);
		}
	} else {
		if(on < 0x100 && on >= 0) {
			__TryHit(Party[Active], GetX(on), GetY(on), obj);
		} else if(C_100B_0410(t)) {
			SetQuan(obj, quan - 1);
			COMBAT_showHit(on);
			CON_printf(C_2BB4_0FC7);
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
			CON_printf(C_2BB4_02A4);
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
			CON_printf(C_2BB4_02A4);
			return;
		}
		on = o;
	}
	if((o = AddMapObj(0x64, 1, 0, GetX(on), GetY(on), MapZ)) == -1) {
		CON_printf(C_2BB4_032E);
		return;
	}
	ObjStatus[o] &= ~0x20;
	DeleteObj(obj);
	ObjShapeType[o] = GetType(o) + 0x400;
	CON_printf(C_2BB4_0DD5);
	if((TalkFlags[0x74] |= 0x10) & 1)
		TalkFlags[0x60] |= 4;
}

/*
 * action 23 (lever 0x1a2): set the frame to a direction (AdvanceDir) or 4
 * (center); needs TalkFlags[0x76] 0x20 and 0x40, or the dream
 */
/*C_2F98_1334*/C_2F98_1334(register int16 obj, int16 on, int16 to) {
	register int16 key;

	if((!(TalkFlags[0x76] & 0x20) || !(TalkFlags[0x76] & 0x40)) && !(D_16AA & 0x10)) {
		CON_printf(C_2BB4_0C5F);
		return;
	}
	MouseMode = 0;
	DirOriginX = (WrapDelta(GetX(obj) - MapX) << 4) + 0x58;
	DirOriginY = (WrapDelta(GetY(obj) - MapY) << 4) + 0x58;
	CON_printf(C_2BB4_0C77);
	key = CON_getch();
	MUS_PlaySfx(0x11);
	DirOriginX = 0x58;
	DirOriginY = 0x58;
	CON_UpdateMousePointer();
	if(key != 0x80) {
		CON_printf("center.\n");
		ObjShapeType[obj] = GetType(obj) + 0x1000;
	} else {
		CON_printf("%s.\n", DirNames[D_EB72]);
		ObjShapeType[obj] = GetType(obj) + (D_EB72 >> 1 << 10);
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
		CON_printf(C_2BB4_0364);
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
	CON_printf(C_2BB4_0C86, lat, ns, lon, ew);
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
		CON_printf(C_2BB4_0230);
		return;
	}
	if(AddInvObj(0x83, 0x10, 0x14, p) == -1) {
		CON_printf(C_2BB4_024E);
		return;
	}
	if(d > 1)
		COMBAT_Missile(p, &x, &y, 9, BaseTile[0x83]);
	CON_printf(C_2BB4_101E);
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
		CON_printf(C_2BB4_0230);
		return;
	}
	if(GetFrame(obj) < 1) {
		CON_printf(C_2BB4_1A37);
		return;
	}
	if((o = AddInvObj(0x9e, 0x10, 1, p)) < 0) {
		CON_printf(C_2BB4_024E);
		return;
	}
	if(d > 1)
		COMBAT_Missile(p, &x, &y, 9, BaseTile[0x9e]);
	ObjShapeType[obj] = GetType(obj);
	SetQual(obj, OSI_rand(0xa, 0xf));
	ShowObjects();
	if(StatusDisplay == 0x92)
		D_0F12 = 1;
	MapAnimEnabled = 1;
}

/*
 * action 59 (seed 0x9e): plant it on the selected square
 */
/*C_2F98_17B5*/C_2F98_17B5(register int16 obj, int16 on, int16 to) {
	register int16 p;
	int16 key, x, y, d;

	p = Party[Active];
	CON_printf(C_2BB4_1A50);
	MouseMode = 1;
	if(SelectMode) {
		DrawSlotCursor(0);
		SelectMode = 1;
		AimX = AimY = 5;
	}
	if((key = CON_getch()) != 0x8e || D_EBF6 < 0 || D_EBF8 < 0) {
		CON_printf(C_2BB4_03F8);
		return;
	}
	CON_putch('\n');
	x = D_EBF6;
	y = D_EBF8;
	if((d = COMBAT_range(x, y, GetX(p), GetY(p))) > 1 && COMBAT_Missile(p, &x, &y, 9, 0) == 0) {
		CON_printf(C_2BB4_0230);
		MUS_PlaySfx(5);
		return;
	}
	if(!Class_20_3F_7C_7F(GetTileAtXYZ(x, y, MapZ))) {
		CON_printf(C_2BB4_1A5B);
		MUS_PlaySfx(5);
		return;
	}
	if(d > 1)
		COMBAT_Missile(p, &x, &y, 9, BaseTile[0x9e]);
	if(ObjStatus[obj] & 0x18)
		D_0F12 = 1;
	if(GetQuan(obj) > 1) {
		SetQuan(obj, GetQuan(obj) - 1);
		if((obj = AddMapObj(0x9e, 0, 1, x, y, MapZ)) < 0)
			CON_printf(C_2BB4_22F1);
	} else {
		MoveObj(obj, x, y, MapZ);
	}
	ShowObjects();
	MapAnimEnabled = 1;
	CON_printf(C_2BB4_1A6E);
}

/*
 * action 66 (spittoon 0x11e)
 */
/*C_2F98_1988*/C_2F98_1988(int16 obj, int16 on, int16 to) {
	if(ObjStatus[Party[Active]] & 0x40)
		CON_printf(C_2BB4_1A8B);
	else
		CON_printf(C_2BB4_1AA3);
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
		CON_printf(C_2BB4_1AC0);
		return;
	}
	CON_printf(C_2BB4_1AC8);
	CON_printf(C_2BB4_1B2A);
	CON_printf(C_2BB4_1B8A);
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

	if(!D_16B0[Party[Active]].psychic) {
		CON_printf(C_2BB4_10B9);
		return;
	}
	p = Party[Active];
	if((d = COMBAT_range(D_EBF6, D_EBF8, GetX(p), GetY(p))) > 1 && COMBAT_Missile(p, &D_EBF6, &D_EBF8, 9, 0) == 0) {
		CON_printf(C_2BB4_0230);
		return;
	}
	if((o = AddInvObj(0x14b, 0x10, 0, p)) == -1) {
		CON_printf(C_2BB4_024E);
		return;
	}
	if(d > 1)
		COMBAT_Missile(p, &D_EBF6, &D_EBF8, 9, BaseTile[0x14b]);
	CON_printf(C_2BB4_1088);
	if(AddInvObj(GetQual(obj) ? 0x139 : OSI_rand(0x31, 0x38), 8, 0, o) >= 0)
		SetQual(obj, 0);
	else
		CON_printf(C_2BB4_22F1);
}

/*
 * action 53 (dreamstuff 0x14b): imagine what it holds: an object (< 0x156) in
 * its place, else an actor next to the party (alignment from the quality)
 */
/*C_2F98_1BB6*/C_2F98_1BB6(int16 obj, int16 on, int16 to) {
	register int16 npc, inner;
	int16 t, i, x, y;

	MUS_PlaySfx(0x32);
	CON_printf(C_2BB4_107B);
	if((inner = FindInv(obj)) < 0) {
		CON_printf(C_2BB4_0402);
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
			CON_printf(C_2BB4_0402);
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
			CON_printf(C_2BB4_0402);
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
	if(D_EBCE == 0x68) {
		TalkFlags[0x68] |= 0x40;
		TALK_talkTo(Active, 0x68, 0);
	} else if(D_EBCE == 0 && MaxHP(1) > HitPoints[0]) {
		CON_printf(C_2BB4_13A1, GetObjectString(1));
		HitPoints[0] = MaxHP(1);
	} else {
		CON_printf(C_2BB4_02A4);
	}
}

/*
 * action 72 (valve 0xdc): start the canals (as panel 7)
 */
/*C_2F98_1E83*/C_2F98_1E83(int16 obj, int16 on, int16 to) {
	if(TalkFlags[0x76] & 0x40) {
		CON_printf(C_2BB4_12C8);
	} else if(TalkFlags[0x76] & 0x20) {
		TalkFlags[0x76] |= 0x40;
		BargePumps(0);
		C_2F75_0951();
	} else {
		CON_printf(C_2BB4_111C);
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
		CON_printf(C_2BB4_228A, GetObjectString(on));
		MUS_PlaySfx(5);
		return;
	}
	if(t == 0x155) {
		if(n < 0x10) {
			if((n = SPELL_ReviveNPC(on)) < 0) {
				CON_printf(C_2BB4_22AF, GetObjectString(on));
				MUS_PlaySfx(5);
			} else {
				CON_printf(C_2BB4_22C3, GetObjectString(n));
				MUS_PlaySfx(4);
			}
		} else {
			CON_printf(C_2BB4_22AF, GetObjectString(on));
			MUS_PlaySfx(5);
		}
	} else {
		CON_printf(C_2BB4_22C3, GetObjectString(on));
		MUS_PlaySfx(4);
		HitPoints[on] = MaxHP(on);
	}
	if(StatusDisplay == 0x91 || StatusDisplay == 0x90 && Party[StatusMember] == on)
		D_0F12 = 1;
	if(GetQuan(obj) == 1) {
		DeleteObj(obj);
		return;
	}
	SetQuan(obj, GetQuan(obj) - 1);
}
