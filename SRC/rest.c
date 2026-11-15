/* flags: -d -mm -p -c -Z -O -y */
/* stub order: 0087 0131 0000 0397 */
/*
 * Martian Dreams - GAME.EXE overlay 2F7C: the "Rest" command.
 * From Savage Empire's rest.c (Ultima VI's), reworked.
 *
 * Resting is refused in combat, with foes near (a hostile NPC within 39
 * tiles), with an NPC within 5 tiles that can be seen ("%s is too near to
 * set up camp"), in solo mode and in the dream (Active == PartySize).
 * Poisoned members are named and the player may give up; members with a
 * talent (D_16B0 fields b and c: "inanimate things talking", "too much
 * light") cannot sleep. The party camps in a tent (five objects 0x86 put
 * around (x, y), the middle one animated) until sunrise or sunset, or for
 * 1-9 hours. A rest of more than 4 hours heals (2-8 + 2-8) * hours/2 hit
 * points when NextSleep allows it (then NextSleep = 6); while the time
 * passes (20 minutes per step) the members who have the experience for it
 * go up a level through a dream of three obelisks (REST_LevelUp: the sword
 * raises strength, the heart dexterity, the book intelligence, up to 30,
 * and 30 hit points come back). Poison may wear off (2-16 >= 15) or costs
 * hit points. At the end the morning song plays (C_1FA9_02CE) and the tent
 * is taken down.
 * MD changes from SE: no food (IsEdible, TryEat... are gone), no
 * gathering around the leader (CampOfsX/CampOfsY/CampFacing are still
 * here, unused), MapDistance (wrapping) instead of CLOSE_ENOUGH,
 * REST_OthersNear returns the NPC, no ship or turtle test, and the dream
 * and its questions are new.
 */
#include "se.h"

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;/*1 wrap, 2 center, 4 capitalize, 8 runes, 0x10 paginate, 0x20 scroll*/
};

/*OSI routines (asm)*/
extern int16 OSI_rand(int16, int16);/*OSI_rand(lo, hi)*/
extern cdecl MOUSE_SERVE();

/*library*/
extern void cdecl TimerDelay(int32);/*wait (2383 per millisecond)*/
extern int16 cdecl soundCard;

/*other game modules*/
extern PaletteAnimation();
extern ShowPortrait(int16);
extern int16 MaxHP(int16);
extern int16 COMBAT_canSee(int16, int16);
extern ClearMapWindow(int16);
extern DrawMapWindow(int16);
extern DisplayTick();
extern PassTime(int16);/*PassTime(minutes)*/
extern VIS_UpdateView();
extern MAP_LoadAround();
extern PartyEnter(int16);
extern PartyExit(int16, int16, int16, int16);
extern int16 MapDistance(int16, int16);/*MapDistance (wrapping)*/
extern int16 C_170D_2BB4(int16);/*true for a man (the dream's pronoun)*/
extern int16 NextArea(void);
extern int16 SearchArea(int16, int16, int16, int16);/*SearchArea(x1, y1, x2, y2)*/
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);/*AddMapObj(type, status, quantity, x, y, z)*/
extern DeleteObj(int16);
extern ShowObjects();
extern char *GetObjectString(int16);
extern C_1FA9_02CE(int16);/*the morning song*/
extern MUS_PlaySfx(int16);
extern int16 CON_mouseOffAt(int16, int16, int16, int16);
extern CON_mouseOn(int16);
extern int16 CON_getch();
extern int16 CON_prompt();
extern CON_setClip(struct tClipInfo *);
extern CON_gotoxy(int16, int16);
extern cdecl CON_printf(char far *, ...);
extern char *TALK_GetCharName(int16);
extern char far C_2BB4_02FA[];/*NotInCombatMsg*/
extern char far C_2BB4_0315[];/*NotInSoloMsg*/

extern unsigned char MouseOn;
extern int16 ScreenFade;
extern int16 ColorText;
extern int16 D_0826;/*text color of the dream's question*/
extern char D_0F12;/*StatusDirty*/
extern int16 D_0F16;/*IdleAnimTick*/
extern unsigned char NextSleep;
extern unsigned char Time_H;
extern unsigned char Active;
extern int16 MapX, MapY, MapZ;
extern int16 SoundFlag;
extern char InCombat;
extern char SoloMember;
extern struct { unsigned a:4, b:4, c:4, d:4; } D_16B0[];/*talents*/
extern char SelectMode;
extern char AnimateAtPrompt;
extern int16 EndPagingAtPrompt;
extern int16 StatusMember;
extern int16 StatusDisplay;/*0x90 a member, 0x91 the party, 0x92 the equipment*/
extern struct tClipInfo *CurClip;
extern char IsInConversation;
extern unsigned char D_45D0[];/*DEXTE*/
extern unsigned char D_4EA4[];/*Party*/
extern unsigned char far *D_4EB6;/*Level*/
extern char D_50CE[][14];/*Names*/
extern uint16 D_51AE[];/*ExpPoints*/
extern unsigned char D_53B2[];/*STREN*/
extern unsigned char D_6CBA[];/*NPCStatus*/
extern unsigned char D_6DBE[];/*HitPoints*/
extern unsigned char D_760E[];/*INTEL*/
extern struct coord D_78AE[];/*ObjPos*/
extern unsigned char D_9CC0;/*PartySize*/
extern uint16 D_9CC4[];/*ObjShapeType*/
extern struct tClipInfo D_EBD0[4];/*TextWindows*/
extern int16 D_EC42;/*LinesBeforeMore*/

#define StatusDirty D_0F12
#define IdleAnimTick D_0F16
#define DEXTE D_45D0
#define Party D_4EA4
#define Level D_4EB6
#define Names D_50CE
#define ExpPoints D_51AE
#define STREN D_53B2
#define NPCStatus D_6CBA
#define HitPoints D_6DBE
#define INTEL D_760E
#define ObjPos D_78AE
#define PartySize D_9CC0
#define ObjShapeType D_9CC4
#define TextWindows D_EBD0
#define LinesBeforeMore D_EC42

#define NotInCombatMsg C_2BB4_02FA
#define NotInSoloMsg C_2BB4_0315

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)
#define SetCoordXYZ(o, x, y, z) {      \
	(o).raw[2] = ((z) << 4);           \
	*(int16 *)&((o).raw[0]) = (x);       \
	*(int16 *)&((o).raw[1]) |= (y) << 2; \
}

/*213E*/char IsResting = 0;/*1 while the party sleeps*/

/*check foes near:
  returns 1 if a hostile NPC (NPCStatus 0x20) is on this level within
  39 tiles of the party*/
/*C_2F7C_0000*/REST_FoesNear() {
	int16 objNum;

	for(objNum = 0x10; objNum < 0x100; objNum ++) {
		if(
			ObjShapeType[objNum] != 0 &&
			(NPCStatus[objNum] & 0x20) &&
			MapDistance(GetX(objNum), MapX) <= 39 &&
			MapDistance(GetY(objNum), MapY) <= 39 &&
			GetZ(objNum) == MapZ
		) return 1;
	}
	return 0;
}

/*check others near: returns the first NPC (NPCStatus without 0x10, 0x04,
  0x80) within 5 tiles on this level that COMBAT_canSee, else 0*/
/*C_2F7C_0087*/REST_OthersNear() {
	int16 objNum;

	for(objNum = 1; objNum < 0x100; objNum ++) {
		if(
			!(NPCStatus[objNum] & 0x10) &&
			!(NPCStatus[objNum] & 0x04) &&
			!(NPCStatus[objNum] & 0x80) &&
			MapDistance(GetX(objNum), MapX) <= 5 &&
			MapDistance(GetY(objNum), MapY) <= 5 &&
			GetZ(objNum) == MapZ &&
			COMBAT_canSee(1, objNum)
		) return objNum;
	}
	return 0;
}

/*while resting, NPC npc (1-15) goes up a level if it has the experience
  for it: 30 hit points come back, and in a dream the player touches the
  obelisk with the sword (strength), the heart (dexterity) or the book
  (intelligence). Attributes go up to 30*/
/*C_2F7C_0131*/REST_LevelUp(register int16 npc) {
	char *name;
	char pronoun[4] = "She";
	register int16 done = 0;
	int16 key;
	int16 savedDisplay, savedMember, savedLines;
	struct tClipInfo *savedClip;
	uint16 expTable[9] = {0, 100, 200, 400, 800, 1600, 3200, 6400, 0x7fff};

	if(npc < 1 || npc > 15)
		return;
	if(expTable[Level[npc]] > ExpPoints[npc])
		return;
	Level[npc] ++;
	name = TALK_GetCharName(npc);
	if(C_170D_2BB4(npc)) {
		pronoun[2] = 0;
		pronoun[1] = 'e';
		pronoun[0] = 'H';
	}
	IsInConversation = 1;
	AnimateAtPrompt = 0;
	ClearMapWindow(1);
	ShowPortrait(npc);
	savedDisplay = StatusDisplay;
	savedMember = StatusMember;
	savedLines = LinesBeforeMore;
	savedClip = CurClip;
	CON_setClip(&(TextWindows[2]));
	CurClip->attrFlags |= 0x20;
	CurClip->attrFlags |= 0x10;
	CON_gotoxy(0, 0);
	LinesBeforeMore = CurClip->rows;
	HitPoints[npc] = (MaxHP(npc) <= HitPoints[npc] + 30)?MaxHP(npc):(HitPoints[npc] + 30);
	CON_printf("%s has a dream...\n\n", name);
	CON_printf("%s sees three stone obelisks, one inscribed with a book, ", pronoun);
	CON_printf("one a heart, and one a sword.\n\n*");
	CON_printf("%s feels drawn to one of the obelisks.\n\n", pronoun);
	CON_printf("Does %s touch the obelisk with the @book, the @heart, ", name);
	CON_printf("or the @sword?\n");
	CurClip->fgColor = D_0826;
	CON_printf("Which (B H S):\n");
	while(!done) {
		while((key = CON_prompt()) == 0);
		switch(key) {
			case 'S': case 's':/*sword*/
				if(STREN[npc] < 30)
					STREN[npc] = STREN[npc] + 1;
				done = 1;
				break;
			case 'H': case 'h':/*heart*/
				if(DEXTE[npc] < 30)
					DEXTE[npc] = DEXTE[npc] + 1;
				done = 1;
				break;
			case 'B': case 'b':/*book*/
				if(INTEL[npc] < 30)
					INTEL[npc] = INTEL[npc] + 1;
				done = 1;
				break;
			default:/*twice: the original clears DI twice*/
				done = 0;
				done = 0;
		}
	}
	CON_printf("\n\n");
	CurClip->fgColor = ColorText;
	IsInConversation = 0;
	CON_setClip(savedClip);
	EndPagingAtPrompt = 1;
	LinesBeforeMore = savedLines;
	AnimateAtPrompt = 1;
	VIS_UpdateView();
	ScreenFade = 1;
	DrawMapWindow(0);
	StatusDisplay = savedDisplay;
	StatusMember = savedMember;
	StatusDirty ++;
}

/*SE's places around the leader for party members #0..#7 (not used in MD):
  CampOfsX/CampOfsY = x/y offset, CampFacing = direction the member
  faces (0=N 2=E 4=S 6=W)*/
/*2155*/char CampOfsX[] = { 0, 0, 1,-1, 1,-1, 1,-1};
/*215D*/char CampOfsY[] = {-1, 1, 0, 0,-1, 1, 1,-1};
/*2165*/char CampFacing[] = { 4, 0, 6, 2, 4, 0, 6, 2};

/*"rest" command: the party camps at (x, y)*/
/*C_2F7C_0397*/REST_Command(int16 x, int16 y) {
	struct coord xyz;
	int16 sleepTime, i;
	int16 someoneCannotMove, cannot, gatherTries, obj, n;
	int16 mouseState, dream, wasSelectMode;
	int16 tent[5], found;
	register int16 ch, npc;

	someoneCannotMove = 1;
	gatherTries = 20;
	obj = 0x100;
	dream = 0;
	CON_printf("Rest");
	wasSelectMode = SelectMode;
	if(InCombat) {
		CON_printf(" - ");
		CON_printf(NotInCombatMsg);
		MUS_PlaySfx(5);
		return;
	}
	if(REST_FoesNear()) {
		CON_printf(" - Not while foes are near!\n");
		MUS_PlaySfx(5);
		return;
	}
	if(i = REST_OthersNear()) {
		CON_printf(" - %s is too near to set up camp.\n", GetObjectString(i));
		MUS_PlaySfx(5);
		return;
	}
	if(SoloMember != -1) {
		CON_printf(" - ");
		CON_printf(NotInSoloMsg);
		MUS_PlaySfx(5);
		return;
	}
	if(Active == PartySize) {
		CON_printf(" - ");
		CON_printf("You can't sleep while dreaming!\n");
		MUS_PlaySfx(5);
		return;
	}
	cannot = 0;
	for(i = 0; i < PartySize; i ++) {
		if(NPCStatus[Party[i]] & 8) {
			CON_printf("\n%s is poisoned!", Names[i]);
			cannot = 1;
		}
	}
	if(cannot) {
		CON_printf("\nDo you really want to sleep anyway? ");
		ch = CON_getch();
		CON_printf("%c\n", ch);
		if(ch != 'y' && ch != 'Y')
			return;
	} else {
		CON_printf("\n");
	}
	cannot = 0;
	for(i = 0; i < PartySize; i ++) {
		if(D_16B0[Party[i]].c || D_16B0[Party[i]].b) {
			cannot = 1;
			if(D_16B0[Party[i]].c && D_16B0[Party[i]].b)
				CON_printf("\n%s complains of too much light and inanimate things keep talking.*\n", Names[i]);
			else if(D_16B0[Party[i]].b)
				CON_printf("\n%s complains of inanimate things talking.*\n", Names[i]);
			else
				CON_printf("\n%s complains of too much light.*\n", Names[i]);
		}
	}
	if(cannot) {
		if(PartySize > 1)
			CON_printf("\nNobody can sleep.\n");
		else
			CON_printf("\nYou can't sleep.\n");
		return;
	}

	/*the tent*/
	MapX = x;
	MapY = y;
	MAP_LoadAround();
	tent[0] = AddMapObj(0xc86, 0, 1, x, (y - 1) & 0x3ff, MapZ);
	tent[1] = AddMapObj(0x1486, 0, 1, (x + 1) & 0x3ff, (y - 1) & 0x3ff, MapZ);
	tent[2] = AddMapObj(0x1886, 0, 1, (x - 1) & 0x3ff, y, MapZ);
	tent[3] = AddMapObj(0x2486, 0, 1, (x + 1) & 0x3ff, y, MapZ);
	tent[4] = AddMapObj(0x2086, 0, 1, x, y, MapZ);
	DisplayTick();
	PartyEnter(1);
	TimerDelay(0xe8b70L);
	ObjShapeType[tent[4]] = GetType(tent[4]) + 0x1c00;
	ShowObjects();

	CON_printf("\nRest until ");
	if(Time_H < 7 || Time_H > 16) {
		CON_printf("sunrise? ");
		sleepTime = (Time_H < 7)?(7 - Time_H):(24 - Time_H + 7);
	} else {
		CON_printf("sunset? ");
		sleepTime = 18 - Time_H;
	}
	ch = CON_getch();
	if(ch == 'N' || ch == 'n') {
		sleepTime = 0;
		CON_printf("N\nHow many hours? ");
		ch = CON_getch();
		if(ch < '1' || ch > '9') {
			CON_printf("0\n");
			mouseState = CON_mouseOffAt(0, 0, 311, 191);
			goto wakeUp;
		}
		CON_printf("%c\n", ch);
		sleepTime = ch - '0';
	} else {
		CON_printf("Y\n");
	}
	StatusDisplay = 0x91;
	StatusMember = 0;
	StatusDirty = 1;
	DisplayTick();
	IsResting = 1;
	mouseState = CON_mouseOffAt(0, 0, 311, 191);
	SetCoordXYZ(xyz, MapX, MapY, MapZ);

	/*more than 4 hours: with NextSleep 0 everybody heals*/
	if(!NextSleep && sleepTime > 4) {
		for(n = 0; n < PartySize; n ++) {
			npc = Party[n];
			i = (OSI_rand(3, 8) + OSI_rand(3, 8)) * (sleepTime >> 1);
			if(MaxHP(npc) >= HitPoints[npc] + i) {
				HitPoints[npc] += i;
				if(StatusDisplay == 0x91 || (StatusDisplay == 0x90 && Party[StatusMember] == npc))
					StatusDirty = 1;
			} else {
				HitPoints[npc] = MaxHP(npc);
			}
		}
		NextSleep = 6;
	}

	/*20 minutes per step; while dreaming, member #i may go up a level*/
	dream = sleepTime * 3 > PartySize;
	for(i = 0; i < sleepTime * 3; i ++) {
		PassTime(20);
		ShowObjects();
		IdleAnimTick = 1;
		DisplayTick();
		PaletteAnimation();
		if(MouseOn)
			MOUSE_SERVE();
		if(dream && i < PartySize)
			REST_LevelUp(Party[i]);
	}

	/*poison wears off, or costs hit points*/
	n = sleepTime * 2;
	for(i = 0; i < PartySize; i ++) {
		if(NPCStatus[Party[i]] & 8) {
			if(OSI_rand(1, 8) + OSI_rand(1, 8) >= 15) {
				NPCStatus[Party[i]] = NPCStatus[Party[i]] & ~8;
				CON_printf("%s feels better.\n", Names[Party[i]]);
			} else {
				if(HitPoints[Party[i]] < n + 5)
					HitPoints[Party[i]] = 5;
				else
					HitPoints[Party[i]] = HitPoints[Party[i]] - n;
				StatusDirty = 1;
			}
		}
	}

wakeUp:
	IsResting = 0;
	CON_mouseOn(mouseState);
	TimerDelay(0xe8b70L);
	ObjShapeType[tent[4]] = GetType(tent[4]) + 0x2000;
	ShowObjects();
	DisplayTick();
	if(SoundFlag && soundCard != 3)
		C_1FA9_02CE(0);
	else
		TimerDelay(0xe8b70L);
	PartyExit(x, y, MapZ, 1);

	/*the tent is taken down*/
	n = 0;
	for(found = SearchArea((x - 6) & 0x3ff, (y - 6) & 0x3ff, (x + 6) & 0x3ff, (y + 6) & 0x3ff); found != -1 && n < 5; found = NextArea()) {
		if(GetType(found) == 0x86)
			tent[n ++] = found;
	}
	for(i = 0; i < n; i ++)
		DeleteObj(tent[i]);
}
