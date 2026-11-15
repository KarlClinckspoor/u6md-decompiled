/* flags: -d -mm -p -c -Z -O -y */
/* stub order: 0A1B 017E 06C9 17D4 0F95 0562 1CB6 18F3 18C6 0000 1338 1BBC 11E0 00F7 1DAE 1B00 1B18 041A 1953 0C42 14D8 048E 1A5C 1C2E */
/*
 * Martian Dreams - GAME.EXE overlay 2FA1: actions of the "use" command (the
 * third of three overlays of them, with 2F8D and 2F98). New code.
 *
 * Each action is called by UseObject (use.c) through the UseActions table
 * (far 2E67) as f(obj, on, to): the used object, the second object ("On -")
 * and the third ("To -"), -1 when not asked; the numbers below are the
 * table's rows. Light sources (torches 0x6d/0x6e, candles, lamps and
 * lanterns 0x6f-0x76: light, put out, refuel, burn things), the switch
 * 0x19b, the drill 0x1b9, the telescope 0x5e, buckets 0xa0 (water, ice),
 * the chemicals 0xeb on a mound 0xfb, oil on rusted doors, the lenses on
 * their stands 0xca, berries 0x49-0x4c, a battle frenzy, mirrors, doors,
 * gates and chests, the map 0x96, the rubber sprayer's supplies and the
 * musical instruments.
 */
#include "se.h"

extern void cdecl TimerDelay(int32);/*wait (2383 per millisecond)*/

/*other modules*/
extern int16 IsUnlitLightType(int16);
extern int16 IsBurnableType(int16);
extern int16 QuanType(int16);/*4 quantity on 16 bits, 2 on 8 bits, else 0*/
extern C_2F63_0A13(int16);
extern C_2F75_032D(int16);/*a map of MDMAP.LZC*/
extern C_2F5F_0AC1(int16);
extern DreamWakeUp();
extern C_1FA9_0BE4();
extern MUS_PlaySfx(int16);
extern TALK_talkTo(int16, int16, int16);
extern STAT_Ready();
extern STAT_DrawInventory();
extern STAT_ShowInventory(int16);
extern STAT_ShowParty(int16);
extern STAT_ShowPortrait(int16);
extern RefreshStatus();
extern VIS_UpdateView();
extern MAP_LoadAround();
extern DisplayTick();
extern PassTime(int16);
extern int16 COMBAT_GetTargetAt(int16, int16, int16);/*(objNum, x, y)*/
extern __DoHit(int16, int16, int16, int16);/*(attacker, defender, hitPts, kind) (combat.c)*/
extern int16 LoseHP(int16, int16, int16);
extern unsigned char GetTileAtXYZ(int16, int16, int16);
extern int16 IsEarthenWallTile(int16);
extern int16 IsWaterTile(int16);
extern int16 GetDigClass(int16);
extern char *GetTileString(int16);
extern int16 OBJ_FindNPCAt(int16, int16, int16);/*OBJ_FindNPCAt(startNum, x, y)*/
extern int16 OBJ_FindTypeAt(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 OBJ_MapObjAt(int16, int16);
extern int16 FindLoc(int16, int16, int16);
extern int16 NextLoc();
extern int16 SearchArea(int16, int16, int16, int16);
extern int16 NextArea();
extern int16 FindInvType(int16, int16, int16);
extern int16 FindInvOfType(int16, int16);
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);
extern int16 AddInvObj(int16, int16, int16, int16);
extern DeleteObj(int16);
extern MoveObj(int16, int16, int16, int16);
extern SearchObj(int16);
extern ShowObjects();
extern char *GetObjectString(int16);
extern int16 CON_getch();
extern cdecl CON_printf(char far *, ...);
extern int16 OSI_rand(int16, int16);

extern signed char DirIncrX[8], DirIncrY[8];
extern char *DirNames[];
extern int16 ScreenFade;
extern int16 XRayView;
extern unsigned char D_0F12;/*StatusDirty*/
extern int16 VisibilityDirty;
extern unsigned char Time_H;
extern unsigned char Date_D, Date_M;
extern unsigned char Active;
extern char AmbientLight;
extern int16 MapX, MapY, MapZ;
extern unsigned char DrunkCounter;
extern char InCombat;
extern int32 GameFlags;/*0x10: in a dream*/
extern struct { unsigned a:4, psychic:4, c:4, d:4; } Talents[];
extern unsigned char PartyListTop;
extern int16 StatusMember;
extern int16 MouseMode;
extern int16 StatusDisplay;
extern unsigned char ObjStatus[];
extern unsigned char D_4EA4[];/*Party*/
extern unsigned char far *Level;
extern char D_50CE[][14];/*Names*/
extern int16 Amount[];/*quantity (low byte), quality (high byte)*/
extern unsigned char D_6CBA[];/*NPCStatus*/
extern unsigned char D_6EC6[];/*TalkFlags*/
extern struct coord ObjPos[];
extern uint16 ObjShapeType[];
extern unsigned char far *D_B4C4;/*TerrainType*/
extern int16 D_EB04[8];/*Equipment*/
extern int16 D_EB72;/*AdvanceDir*/
extern int16 D_EBCE;/*Selection.obj*/
extern int16 D_EBF6, D_EBF8;/*Selection.x, Selection.y*/

#define Party D_4EA4
#define Names D_50CE
#define NPCStatus D_6CBA
#define TalkFlags D_6EC6
#define TerrainType D_B4C4
#define Equipment D_EB04

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetQual(o) (((unsigned char *)(Amount + (o)))[1])
#define SetQual(o, v) (GetQual(o) = (v))
#define GetQuan(o) (((unsigned char *)(Amount + (o)))[0])
#define SetQuan(o, v) (GetQuan(o) = (v))
#define GetAssoc(i) (*(int16 *)&(ObjPos[i]))

/*messages (segment 2BB4)*/
extern char far C_2BB4_01B4[];/*"Direction-\n"*/
extern char far C_2BB4_01FD[];/*"It's already lit.\n"*/
extern char far C_2BB4_0230[];/*"Blocked!\n"*/
extern char far C_2BB4_0295[];/*"It is locked.\n"*/
extern char far C_2BB4_02A4[];/*"It has no effect.\n"*/
extern char far C_2BB4_02CF[];/*"No.\n"*/
extern char far C_2BB4_02EB[];/*"Not at night!\n"*/
extern char far C_2BB4_032E[];/*"Not now.\n"*/
extern char far C_2BB4_0364[];/*"Not while underground.\n"*/
extern char far C_2BB4_037C[];/*"That would be a waste outside of combat.\n"*/
extern char far C_2BB4_03F8[];/*"nowhere.\n"*/
extern char far C_2BB4_0415[];/*"\nNothing to light the %s with.\n"*/
extern char far C_2BB4_0435[];/*"\nThere is no fuel.\n"*/
extern char far C_2BB4_0449[];/*"\nYour hands are full.\n"*/
extern char far C_2BB4_0460[];/*"The %s is extinguished.\n"*/
extern char far C_2BB4_0479[];/*"\nYou incinerated the %s.\n"*/
extern char far C_2BB4_0493[];/*"\nIt's already full.\n"*/
extern char far C_2BB4_069A[];/*"There is nothing in the bucket.\n"*/
extern char far C_2BB4_06BB[];/*"You pour the water onto the ground.\n"*/
extern char far C_2BB4_06E0[];/*"You pour the water onto the seed.\n"*/
extern char far C_2BB4_0703[];/*"%s is all wet.\n"*/
extern char far C_2BB4_0713[];/*"The %s is all wet.\n"*/
extern char far C_2BB4_0727[];/*"You pour the nitrogen compound onto the mound.\n"*/
extern char far C_2BB4_0757[];/*"You pour the phosphorous solution onto the mound.\n"*/
extern char far C_2BB4_078A[];/*"You pour the potash mixture onto the mound.\n"*/
extern char far C_2BB4_07B7[];/*"The seed is destroyed by acid.\n"*/
extern char far C_2BB4_07D7[];/*"The seed is poisoned by oil.\n"*/
extern char far C_2BB4_07F5[];/*"There is nothing to drill into.\n"*/
extern char far C_2BB4_0816[];/*"There is no room left for the ore.\n"*/
extern char far C_2BB4_085B[];/*"You eat a Martian berry. You feel %s strange "*/
extern char far C_2BB4_0889[];/*"relationship with things around you.\n"*/
extern char far C_2BB4_08AF[];/*"sensitivity to the feelings of objects around you.\n"*/
extern char far C_2BB4_08E3[];/*"sense of your spatial location.\n"*/
extern char far C_2BB4_0904[];/*"feelings, like the berries were really odd.\n"*/
extern char far C_2BB4_0931[];/*"%s enters a battle frenzy!\n"*/
extern char far C_2BB4_0C32[];/*"It turns loosely.\n"*/
extern char far C_2BB4_0E6B[];/*"The sprayer system now has rubber.\n"*/
extern char far C_2BB4_0E8F[];/*"The sprayer system now has dye.\n"*/
extern char far C_2BB4_104B[];/*"This door is not rusted.\n"*/
extern char far C_2BB4_1065[];/*"The door is unstuck.\n"*/
extern char far C_2BB4_1171[];/*"`(using %s)"*/
extern char far C_2BB4_117D[];/*"`You fill the bucket with water.\n"*/
extern char far C_2BB4_119F[];/*"`Dump out the ice? "*/
extern char far C_2BB4_11F4[];/*"`It is automatic and can't be opened by hand.\n"*/
extern char far C_2BB4_1223[];/*"It is apparently rusted shut.\n"*/
extern char far C_2BB4_1242[];/*"It is apparently locked.\n"*/
extern char far C_2BB4_125C[];/*"It has no power.\n"*/
extern char far C_2BB4_1325[];/*"Wheeeeze\n"*/
extern char far C_2BB4_132F[];/*"Ching!\n"*/
extern char far C_2BB4_1337[];/*"Thump! Thump!\n"*/
extern char far C_2BB4_1346[];/*"Shing!\n"*/
extern char far C_2BB4_137F[];/*"A torch burns out.\n"*/
extern char far C_2BB4_1BE8[];/*"The %s is broken.\n"*/
extern char far C_2BB4_1BFB[];/*"You cannot install it there.\n"*/
extern char far C_2BB4_1C54[];/*"It is nailed shut.\n"*/
extern char far C_2BB4_1ECD[];/*"\n\nAn item can't be placed inside itself!\n"*/
extern char far C_2BB4_1F26[];/*"A mechanical person can't eat berries!\n"*/
extern char far C_2BB4_1FA6[];/*"There is already something in the bucket.\n"*/
extern char far C_2BB4_2138[];/*"sudden flush, dizziness, and nausea.\n"*/
extern char far C_2BB4_2261[];/*"There are too many moving trees nearby.\n"*/

/*
 * action 63 (a lit light source on itself): put it out (the lit type
 * becomes the unlit one); a lit torch 0x6e burns out (deleted) unless it is
 * readied ("No."); also called by status.c on unready as (obj, obj, -1)
 */
/*C_2FA1_0000*/C_2FA1_0000(int16 obj, register int16 on, int16 to) {
	register int16 t;

	if(obj == on) {
		if(GetType(obj) == 0x6e) {
			if((ObjStatus[obj] & 0x18) == 0x18) {
				CON_printf(C_2BB4_02CF);
			} else {
				DeleteObj(obj);
				CON_printf(C_2BB4_137F);
			}
		} else {
			switch(GetType(on)) {
				case 0x70: t = 0x6f; break;
				case 0x76: t = 0x75; break;
				case 0x72: t = 0x71; break;
				case 0x74: t = 0x73; break;
				default: t = GetType(on);
			}
			ObjShapeType[on] = (ObjShapeType[on] & 0xfc00) + t;
			CON_printf(C_2BB4_0460, GetObjectString(on));
		}
	} else {
		CON_printf(C_2BB4_01FD);
	}
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
	PassTime(0);
}

/*
 * action 19 (a lit light source on anything): 0x84 C_2F63_0A13, a burnable
 * type (IsBurnableType) is incinerated
 */
/*C_2FA1_00F7*/C_2FA1_00F7(int16 obj, int16 on, int16 to) {
	register int16 t;

	if((t = GetType(on)) == 0x84) {
		C_2F63_0A13(on);
	} else if(IsBurnableType(t)) {
		CON_printf(C_2BB4_0479, GetObjectString(on));
		MUS_PlaySfx(9);
		DeleteObj(on);
	} else {
		CON_printf(C_2BB4_02A4);
		MUS_PlaySfx(5);
	}
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
	PassTime(0);
}

/*
 * action 29 (a lit source or a match 0x6b on an unlit one): light it; a torch
 * from a stack 0x6d is split off and readied, others are readied first
 * (hands full); quality 0 sets the fuel: lanterns 0x73/0x75 take a can of oil
 * 0x7c (0xff / 0xb4), candles 0x6f/0x71 100, others 0xb4; a match is used
 * up; the view is drawn with AmbientLight 7
 */
/*C_2FA1_017E*/C_2FA1_017E(int16 obj, register int16 on, int16 to) {
	register int16 t;
	int16 p, fuel, old, n, nt;

	t = GetType(on);
	if(ObjStatus[on] & 0x18) {
		if((ObjStatus[on] & 0x18) != 0x18) {
			if(t == 0x6d) {
				n = GetQuan(on);
				old = on;
				if((on = AddInvObj(0x6d, 0x18, 1, Party[StatusMember])) < 0) {
					CON_printf(C_2BB4_0449);
					MUS_PlaySfx(5);
					return;
				}
				if(n > 1)
					SetQuan(old, n - 1);
				else
					DeleteObj(old);
			} else {
				D_EBCE = on;
				STAT_Ready();
				if(Equipment[2] != on && Equipment[5] != on) {
					CON_printf(C_2BB4_0449);
					MUS_PlaySfx(5);
					return;
				}
			}
		}
		p = GetAssoc(on);
	} else {
		p = Party[Active];
	}
	CON_printf(C_2BB4_1171, GetType(obj) != 0x6b ? GetObjectString(obj) : "a match");
	if(IsUnlitLightType(t)) {
		switch(GetType(on)) {
			case 0x6d: nt = 0x6e; break;
			case 0x6f: nt = 0x70; break;
			case 0x75: nt = 0x76; break;
			case 0x71: nt = 0x72; break;
			case 0x73: nt = 0x74; break;
			default: nt = GetType(on);
		}
		if(GetQual(on) == 0) {
			if(t == 0x73 || t == 0x75) {
				if((fuel = FindInvOfType(p, 0x7c)) == -1) {
					CON_printf(C_2BB4_0435);
					MUS_PlaySfx(5);
					return;
				}
				n = GetQuan(fuel);
				if(n > 1)
					SetQuan(fuel, n - 1);
				else
					DeleteObj(fuel);
				if(t == 0x73)
					SetQual(on, 0xff);
				else
					SetQual(on, 0xb4);
			} else if(t == 0x6f) {
				SetQual(on, 100);
			} else if(t == 0x71) {
				SetQual(on, 100);
			} else {
				SetQual(on, 0xb4);
			}
		}
		ObjShapeType[on] = (ObjShapeType[on] & 0xfc00) + nt;
	}
	if(GetType(obj) == 0x6b) {
		n = GetQuan(obj);
		if(n > 1)
			SetQuan(obj, n - 1);
		else
			DeleteObj(obj);
	}
	n = AmbientLight;
	AmbientLight = 7;
	VIS_UpdateView();
	DisplayTick();
	AmbientLight = n;
	PassTime(0);
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*
 * action 28 (an unlit light source): light it (C_2FA1_017E) with a
 * match 0x6c or 0x6b from the active member's inventory
 */
/*C_2FA1_041A*/C_2FA1_041A(int16 obj, int16 on, int16 to) {
	register int16 m = -1, p = Party[Active];

	if(m == -1)
		m = FindInvOfType(p, 0x6c);
	if(m == -1)
		m = FindInvOfType(p, 0x6b);
	if(m == -1) {
		CON_printf(C_2BB4_0415, GetObjectString(obj));
		MUS_PlaySfx(5);
	} else {
		C_2FA1_017E(m, obj, -1);
	}
}

/*
 * action 30 (oil 0x7c on a lantern 0x73-0x76): refuel it (0xb4 / 0xff)
 */
/*C_2FA1_048E*/C_2FA1_048E(int16 obj, int16 on, int16 to) {
	register int16 t, q;
	int16 p, n;

	p = Party[Active];
	t = GetType(on);
	q = GetQual(on);
	if((t == 0x75 || t == 0x76) && q >= 0x7e || (t == 0x73 || t == 0x74) && q >= 0xb2) {
		CON_printf(C_2BB4_0493);
		MUS_PlaySfx(5);
		return;
	}
	n = GetQuan(obj);
	if(n > 1)
		SetQuan(obj, n - 1);
	else
		DeleteObj(obj);
	if(t == 0x75 || t == 0x76)
		SetQual(on, 0xb4);
	else
		SetQual(on, 0xff);
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
	PassTime(0);
}

/*
 * action 31 (switch 0x19b): toggle it and the 0x19d at (x - 1, y - 1);
 * quality 1 "It turns loosely."; a 0x19a there flips its frame bit 0
 */
/*C_2FA1_0562*/C_2FA1_0562(int16 obj, int16 on, int16 to) {
	register int16 o = -1, f;
	int16 a = -1, b = -1, t, x, y, q;

	f = GetFrame(obj) ? 0 : 1;
	q = GetQual(obj);
	x = (GetX(obj) - 1) & 0x3ff;
	y = (GetY(obj) - 1) & 0x3ff;
	for(o = FindLoc(x, y, MapZ); o != -1; o = NextLoc()) {
		if((t = GetType(o)) == 0x19d)
			a = o;
		else if(t == 0x19a)
			b = o;
	}
	if(a == -1) {
		CON_printf(C_2BB4_02A4);
		MUS_PlaySfx(5);
		return;
	}
	ObjShapeType[obj] = GetType(obj) + (f << 10);
	if(q != 1) {
		ObjShapeType[a] = GetType(a) + (f << 10);
		if(b != -1) {
			f = GetFrame(b);
			ObjShapeType[b] = GetType(b) + ((f % 2 ? f - 1 : f + 1) << 10);
		}
	} else {
		CON_printf(C_2BB4_0C32);
	}
}

/*
 * action 39 (drill 0x1b9): drill in the direction of its frame: ore
 * 0x1bd / 0x1be (quality = loads left; a 0xd5 is only used up) or rock
 * (tiles 0xf0-0xfb) gives the dirt 0x1bb / 0x1bc / 0x1ba on a free square
 * around the drill; an actor there is hit for 0x28
 */
/*C_2FA1_06C9*/C_2FA1_06C9(int16 obj, int16 on, int16 to) {
	register int16 o = -1, y;
	int16 f, x, nt, m, a, k, d, q, t;

	MUS_PlaySfx(0x10);
	if((f = GetFrame(obj)) == 1) {
		x = GetX(obj) - 1;
		y = GetY(obj);
	} else if(f == 3) {
		x = GetX(obj);
		y = GetY(obj) - 1;
	} else if(f == 4) {
		x = GetX(obj) + 1;
		y = GetY(obj);
	} else {
		x = GetX(obj);
		y = GetY(obj) + 1;
	}
	for(o = FindLoc(x, y, MapZ); o != -1 && GetType(o) != 0x1bd && GetType(o) != 0x1be && GetType(o) != 0xd5; o = NextLoc())
		;
	if(o == -1) {
		if(IsEarthenWallTile(GetTileAtXYZ(x, y, MapZ))) {
			nt = 0x1ba;
		} else {
			if((o = COMBAT_GetTargetAt(Party[Active], x, y)) >= 0) {
				__DoHit(Party[Active], o, 0x28, 0);
				return;
			}
			CON_printf(C_2BB4_07F5);
			MUS_PlaySfx(5);
			return;
		}
	} else if(GetType(o) == 0x1bd) {
		nt = 0x1bb;
	} else if(GetType(o) == 0x1be) {
		nt = 0x1bc;
	} else {
		nt = -1;
	}
	if(nt != -1) {
		switch(f) {
			case 1: d = 0; break;
			case 3: d = 2; break;
			case 4: d = 4; break;
			default: d = 6;
		}
		k = -1;
		for(;;) {
			if(k == 8)
				break;
			k ++;
			x = (GetX(obj) + DirIncrX[(k + d) % 8]) & 0x3ff;
			y = (GetY(obj) + DirIncrY[(k + d) % 8]) & 0x3ff;
			a = OBJ_MapObjAt(x, y);
			if((t = GetType(a)) == 0x14e || t == 0x14f || t == 0x150)
				a = -1;
			if((TerrainType[GetTileAtXYZ(x, y, MapZ)] & 2) == 0 && a == -1)
				break;
		}
		if(k == 8) {
			CON_printf(C_2BB4_0816);
			return;
		}
		if((m = AddMapObj(nt, 0, 1, x, y, MapZ)) == -1) {
			CON_printf(C_2BB4_032E);
			return;
		}
		ObjStatus[m] &= ~0x20;
	}
	if(o != -1) {
		if((q = GetQual(o)) == 1)
			DeleteObj(o);
		else
			SetQual(o, q - 1);
	}
	if(nt == -1)
		DeleteObj(o);
}

/*
 * action 32 (telescope 0x5e): on the surface by day and in caves 2/3, look
 * up to 40 squares in a direction (XRayView) from where it stands; not
 * with moving trees 0x198 (invisible) near
 */
/*C_2FA1_0A1B*/C_2FA1_0A1B(int16 obj, int16 on, int16 to) {
	register int16 o, n = 40;
	int16 sx, sy, tile = 0, xr;

	if(MapZ != 0 && MapZ != 2 && MapZ != 3) {
		CON_printf(C_2BB4_0364);
		return;
	}
	if(MapZ == 0 && (Time_H < 5 || Time_H > 17)) {
		CON_printf(C_2BB4_02EB);
		return;
	}
	for(o = SearchArea((MapX - 6) & 0x3ff, (MapY - 5) & 0x3ff, (MapX + 7) & 0x3ff, (MapY + 5) & 0x3ff); o >= 0; o = NextArea()) {
		if(GetType(o) == 0x198 && (ObjStatus[o] & 0x20)) {
			CON_printf(C_2BB4_2261);
			return;
		}
	}
	sx = MapX;
	sy = MapY;
	xr = XRayView;
	if((ObjStatus[obj] & 0x18) == 0) {
		MapX = GetX(obj);
		MapY = GetY(obj);
	} else {
		for(o = GetAssoc(obj); (ObjStatus[o] & 0x18) != 0; o = GetAssoc(o))
			;
		MapX = GetX(o);
		MapY = GetY(o);
	}
	MAP_LoadAround();
	ShowObjects();
	DisplayTick();
	CON_printf(C_2BB4_01B4);
	MouseMode = 0;
	if(CON_getch() != 0x80) {
		CON_printf(C_2BB4_03F8);
		return;
	}
	CON_printf("%s.\n", DirNames[D_EB72]);
	XRayView = 1;
	ScreenFade = 1;
	MAP_LoadAround();
	ShowObjects();
	DisplayTick();
	while(n >= 0 && tile < 0xff) {
		MapX = (MapX + DirIncrX[D_EB72]) & 0x3ff;
		MapY = (MapY + DirIncrY[D_EB72]) & 0x3ff;
		MAP_LoadAround();
		ShowObjects();
		DisplayTick();
		n --;
		tile = GetTileAtXYZ(MapX, MapY, MapZ);
	}
	XRayView = xr;
	MapX = sx;
	MapY = sy;
	MAP_LoadAround();
	ShowObjects();
	D_0F12 = 1;
	RefreshStatus();
	ScreenFade = 1;
	DisplayTick();
}

/*
 * action 42 (bucket 0xa0 on anything): frame 1 water, 2 ice; fills from
 * the canals (tiles 1-4, TalkFlags[0x76] 0x20; with 0x40 water tiles) or
 * the pump 0x112; waters a mound 0xfb (quality |= 8; all four: the seed
 * gets the date), plants 0x9e (becomes 0x9d) and 0x9d (frame 1); the bucket
 * stays full in the dream. Bug kept: with on == -1 it goes on to GetType(-1)
 */
/*C_2FA1_0C42*/C_2FA1_0C42(register int16 obj, register int16 on, int16 to) {
	int16 f, t, q, seed, hole, k;

	f = GetFrame(obj);
	if(ObjStatus[obj] & 0x18)
		D_0F12 = 1;
	if(f == 2) {
		CON_printf(C_2BB4_119F);
		if((k = CON_getch()) == 'y' || k == 'Y')
			ObjShapeType[obj] = GetType(obj);
		return;
	}
	if(on == -1 && (TalkFlags[0x76] & 0x20) && (GetTileAtXYZ(D_EBF6, D_EBF8, MapZ) >= 1 && GetTileAtXYZ(D_EBF6, D_EBF8, MapZ) <= 4 || (TalkFlags[0x76] & 0x40) && IsWaterTile(GetTileAtXYZ(D_EBF6, D_EBF8, MapZ)))) {
		CON_printf(C_2BB4_117D);
		ObjShapeType[obj] = GetType(obj) + 0x400;
		return;
	}
	if(f == 0) {
		CON_printf(C_2BB4_069A);
		MUS_PlaySfx(5);
		return;
	}
	if(on == -1) {
		CON_printf(C_2BB4_0713, GetTileString(GetTileAtXYZ(D_EBF6, D_EBF8, MapZ)));
		C_1FA9_0BE4();
		if((GameFlags & 0x10) == 0)
			ObjShapeType[obj] = GetType(obj);
	}
	if((t = GetType(on)) == 0xfb) {
		if((seed = OBJ_FindTypeAt(0xfc, D_EBF6, D_EBF8)) < 0 && (hole = OBJ_FindTypeAt(0x101, D_EBF6, D_EBF8)) >= 0)
			seed = FindInvType(hole, 0xfc, -1);
		if(seed == -1) {
			CON_printf(C_2BB4_06BB);
			C_1FA9_0BE4();
		} else {
			q = GetQual(on);
			SetQual(on, q |= 8);
			MUS_PlaySfx(4);
			CON_printf(C_2BB4_06E0);
			if(q == 0xf && GetFrame(seed) == 0) {
				SetQual(seed, Date_D);
				SetQuan(seed, Date_M);
			}
		}
	} else if(t == 0x9e) {
		if((ObjStatus[D_EBCE] & 0x18) == 0 && GetDigClass(GetTileAtXYZ(D_EBF6, D_EBF8, MapZ))) {
			MUS_PlaySfx(4);
			ObjShapeType[on] = 0x9d;
			SetQual(on, OSI_rand(10, 15));
			ShowObjects();
		} else {
			CON_printf(C_2BB4_02A4);
		}
	} else if(t == 0x9d) {
		if(GetFrame(D_EBCE) == 0) {
			MUS_PlaySfx(4);
			ObjShapeType[on] = GetType(on) + 0x400;
			SetQual(on, OSI_rand(10, 15));
			ShowObjects();
		} else {
			CON_printf(C_2BB4_02A4);
		}
	} else if(t == 0x112) {
		CON_printf(C_2BB4_117D);
		ObjShapeType[obj] = GetType(obj) + 0x400;
		return;
	} else {
		if(on < 0x100 && on >= 0)
			CON_printf(C_2BB4_0703, GetObjectString(on));
		else
			CON_printf(C_2BB4_0713, GetObjectString(on));
		C_1FA9_0BE4();
	}
	if((GameFlags & 0x10) == 0)
		ObjShapeType[obj] = GetType(obj);
}

/*
 * action 37 (chemicals 0xeb on a mound 0xfb): by frame 3 nitrogen, 1
 * phosphorous, 0 potash (quality bits 1, 2, 4; all of them and water:
 * the seed 0xfc gets the date), 2 acid kills the seed (the mound becomes
 * 0x102), 4 oil poisons it
 */
/*C_2FA1_0F95*/C_2FA1_0F95(int16 obj, int16 on, int16 to) {
	register int16 seed;
	int16 f, q, hole;

	f = GetFrame(obj);
	if((seed = OBJ_FindTypeAt(0xfc, GetX(on), GetY(on))) < 0 && (hole = OBJ_FindTypeAt(0x101, GetX(on), GetY(on))) >= 0)
		seed = FindInvType(hole, 0xfc, -1);
	if(seed == -1) {
		DeleteObj(obj);
		CON_printf(C_2BB4_02A4);
		return;
	}
	q = GetQual(on);
	switch(f) {
		case 3:
			if(!(q & 1)) {
				CON_printf(C_2BB4_0727);
				q |= 1;
			} else {
				CON_printf(C_2BB4_02A4);
			}
			break;
		case 1:
			if(!(q & 2)) {
				CON_printf(C_2BB4_0757);
				q |= 2;
			} else {
				CON_printf(C_2BB4_02A4);
			}
			break;
		case 0:
			if(!(q & 4)) {
				CON_printf(C_2BB4_078A);
				q |= 4;
			} else {
				CON_printf(C_2BB4_02A4);
			}
			break;
		case 2:
		case 4:
			if(f == 2 && seed != -1) {
				CON_printf(C_2BB4_07B7);
				MUS_PlaySfx(5);
				DeleteObj(seed);
				ObjShapeType[on] = (ObjShapeType[on] & 0xfc00) + 0x102;
				SetQual(seed, 0);
				SetQuan(seed, 0);
			} else if(seed != -1) {
				CON_printf(C_2BB4_07D7);
				MUS_PlaySfx(5);
				ObjShapeType[seed] = GetType(seed) + 0x400;
				SetQual(seed, 0);
				SetQuan(seed, 0);
			}
			q = 0;
			break;
		default:
			CON_printf(C_2BB4_02A4);
	}
	SetQual(on, q);
	DeleteObj(obj);
	if(q == 0xf && GetFrame(seed) == 0) {
		SetQual(seed, Date_D);
		SetQuan(seed, Date_M);
	}
}

/*
 * action 60 (oil 0xeb of frame 4 or 0x7c on anything): unstick a rusted
 * door (quality 0xc0 becomes 0x40); on tile 8 NPC 0x51 talks (the dream)
 */
/*C_2FA1_11E0*/C_2FA1_11E0(int16 obj, int16 on, int16 to) {
	register int16 t;
	int16 q, n;

	if(GetType(obj) != 0xeb || GetFrame(obj) == 4) {
		if(on == -1) {
			if(GetTileAtXYZ(D_EBF6, D_EBF8, MapZ) == 8) {
				TalkFlags[0x51] |= 0x40;
				TALK_talkTo(1, 0x51, 0);
				C_2F5F_0AC1(0x51);
				DreamWakeUp();
			} else {
				CON_printf(C_2BB4_02A4);
			}
		} else if((t = GetType(on)) == 0xb3 || t == 0xe3 || t == 0x1a5 || t == 0x98 || t == 0xdb || t == 0xde) {
			if((q = GetQual(on)) - 0x80 >= 0x40) {
				SetQual(on, q - 0x80);
				CON_printf(C_2BB4_1065);
				MUS_PlaySfx(4);
				if(QuanType(obj)) {
					if((n = GetQuan(obj)) > 1)
						SetQuan(obj, GetQuan(obj) - 1);
					else
						DeleteObj(obj);
				} else {
					DeleteObj(obj);
				}
			} else {
				CON_printf(C_2BB4_104B);
			}
		} else {
			goto fail;
		}
	} else {
fail:
		CON_printf(C_2BB4_02A4);
	}
}

/*
 * action 38 (lenses 0xce / 0x13a on a stand 0xca): install them on the
 * stand of frame 0 / 1 (TalkFlags[0x2e] / [0x39] 0x80). Bug kept: the stand
 * is searched around GetX(-1)
 */
/*C_2FA1_1338*/C_2FA1_1338(int16 obj, register int16 on, int16 to) {
	register int16 o;
	int16 t, f, k;

	if(GetQual(obj) & 2) {
		CON_printf(C_2BB4_1BE8, GetObjectString(obj));
		return;
	}
	if(GetType(obj) == 0xce) {
		if(TalkFlags[0x2e] & 0x80) {
			CON_printf(C_2BB4_02A4);
			return;
		}
		k = 0;
	} else {
		if(TalkFlags[0x39] & 0x80) {
			CON_printf(C_2BB4_02A4);
			return;
		}
		k = 1;
	}
	t = GetType(on);
	if((f = GetFrame(on)) != k) {
		on = -1;
		o = SearchArea((GetX(on) - 1) & 0x3ff, (GetY(on) - 1) & 0x3ff, (GetX(on) + 1) & 0x3ff, (GetY(on) + 1) & 0x3ff);
		while(o != -1) {
			t = GetType(o);
			if(GetFrame(o) == k && t == 0xca) {
				on = o;
				o = -1;
			} else {
				o = NextArea();
			}
		}
		if(on == -1) {
			CON_printf(C_2BB4_1BFB);
			return;
		}
	}
	MoveObj(obj, GetX(on), GetY(on), MapZ);
	SetQual(obj, 0);
	if(k != 0)
		TalkFlags[0x39] |= 0x80;
	else
		TalkFlags[0x2e] |= 0x80;
}

/*
 * action 26 (berries 0x49-0x4c): raise a talent (Level, psychic, c) or
 * hurt and make drunk; not for the mechanical man (NPC 6)
 */
/*C_2FA1_14D8*/C_2FA1_14D8(int16 obj, int16 on, int16 to) {
	register int16 v;
	int16 k, inv;

	k = GetType(obj) - 0x49;
	inv = ObjStatus[obj] & 0x18 ? 1 : 0;
	if(Party[Active] == 6) {
		CON_printf(C_2BB4_1F26);
		return;
	}
	MUS_PlaySfx(0x32);
	switch(k) {
		case 0: v = Talents[Party[Active]].a ? Level[Party[Active]] : 0; break;
		case 1: v = Talents[Party[Active]].psychic; break;
		case 2: v = Talents[Party[Active]].c; break;
		case 3: v = 0; break;
		default: v = 0;
	}
	CON_printf(C_2BB4_085B, v ? "an increase in the" : "a");
	switch(k) {
		case 0:
			CON_printf(C_2BB4_0889);
			v = ++Talents[Party[Active]].a;
			break;
		case 1:
			CON_printf(C_2BB4_08AF);
			if(v == 0)
				VisibilityDirty = 1;
			v = ++Talents[Party[Active]].psychic;
			break;
		case 2:
			CON_printf(C_2BB4_08E3);
			if(v == 0)
				VisibilityDirty = 1;
			v = ++Talents[Party[Active]].c;
			break;
		case 3:
			CON_printf(C_2BB4_2138);
			LoseHP(Party[Active], OSI_rand(5, 20) + OSI_rand(5, 20), 2);
			DrunkCounter += OSI_rand(1, 2);
			NPCStatus[Party[Active]] = NPCStatus[Party[Active]] & ~8;
			break;
		default:
			CON_printf(C_2BB4_0904);
			return;
	}
	if(GetQuan(obj) > 1)
		SetQuan(obj, GetQuan(obj) - 1);
	else
		DeleteObj(obj);
	if(inv || (GameFlags & 0x10))
		STAT_ShowInventory(StatusMember);
	if(StatusDisplay == 0x91)
		STAT_ShowParty(PartyListTop);
}

/*
 * action 27: a battle frenzy (ObjStatus 0x80) in combat
 */
/*C_2FA1_17D4*/C_2FA1_17D4(int16 obj, int16 on, int16 to) {
	register int16 p;
	int16 inv;

	inv = ObjStatus[obj] & 0x18 ? 1 : 0;
	if(!InCombat) {
		CON_printf(C_2BB4_037C);
		MUS_PlaySfx(5);
		return;
	}
	p = Party[Active];
	if(!(ObjStatus[p] & 0x80)) {
		CON_printf(C_2BB4_0931, Names[Active]);
		MUS_PlaySfx(0x32);
	}
	ObjStatus[p] |= 0x80;
	if(GetQuan(obj) > 1)
		SetQuan(obj, GetQuan(obj) - 1);
	else
		DeleteObj(obj);
	if(inv)
		STAT_DrawInventory();
	if(StatusDisplay == 0x91)
		STAT_ShowParty(PartyListTop);
	else if(StatusDisplay == 0x90 && Active == StatusMember)
		STAT_ShowPortrait(StatusMember);
}

/*
 * action 68 (0x58 on 0x54): NPC 0x57 talks (the dream)
 */
/*C_2FA1_18C6*/C_2FA1_18C6(int16 obj, int16 on, int16 to) {
	TalkFlags[0x57] |= 0x40;
	TALK_talkTo(1, 0x57, 0);
	C_2F5F_0AC1(0x57);
	DreamWakeUp();
}

/*
 * action 69 (mirror 0x93): at (0x76-0x78, 0xca-0xcc, 2) NPC 0x50 talks
 * (the dream), else "You see yourself."
 */
/*C_2FA1_18F3*/C_2FA1_18F3(int16 obj, int16 on, int16 to) {
	if(MapX >= 0x76 && MapX <= 0x78 && MapY >= 0xca && MapY <= 0xcc && MapZ == 2) {
		TalkFlags[0x50] |= 0x40;
		TALK_talkTo(1, 0x50, 0);
		C_2F5F_0AC1(0x50);
		DreamWakeUp();
	} else {
		CON_printf("You see yourself.\n");
	}
}

/*
 * action 70 (door 0xb5): open or close it (frames 0-7, quality 0x80 locked)
 */
/*C_2FA1_1953*/C_2FA1_1953(register int16 obj, int16 on, int16 to) {
	register int16 x;
	int16 y;

	x = GetX(obj);
	y = GetY(obj);
	if(GetQual(obj) & 0x80) {
		CON_printf(C_2BB4_1242);
		return;
	}
	switch(GetFrame(obj)) {
		case 0:
			x = (x + 1) & 0x3ff;
		case 1:
			ObjShapeType[obj] = GetType(obj) + 0xc00;
			MoveObj(obj, (x - 1) & 0x3ff, y, MapZ);
			break;
		case 2: case 3:
			ObjShapeType[obj] = GetType(obj) + 0x400;
			MoveObj(obj, (x + 1) & 0x3ff, y, MapZ);
			break;
		case 4: case 5:
			ObjShapeType[obj] = GetType(obj) + 0x1c00;
			break;
		case 6: case 7:
			ObjShapeType[obj] = GetType(obj) + 0x1400;
			break;
	}
}

/*
 * action 34 (chests and such): frame 2 locked, 1 closed (open it and
 * SearchObj), else close it
 */
/*C_2FA1_1A5C*/C_2FA1_1A5C(register int16 obj, int16 on, int16 to) {
	register int16 f;
	int16 t;

	t = GetType(obj);
	if((f = GetFrame(obj)) == 2) {
		if(t == 0x56 || t == 0x11c)
			CON_printf(C_2BB4_1C54);
		else
			CON_printf(C_2BB4_0295);
	} else if(f == 1) {
		ObjShapeType[obj] = GetType(obj);
		SearchObj(obj);
	} else {
		ObjShapeType[obj] = GetType(obj) + 0x400;
	}
	if(ObjStatus[obj] & 0x18)
		D_0F12 = 1;
}

/*
 * action 43 (map 0x96): show map quality (C_2F75_032D)
 */
/*C_2FA1_1B00*/C_2FA1_1B00(int16 obj, int16 on, int16 to) {
	C_2F75_032D(GetQual(obj));
}

/*
 * action 44 (gates 0xde, 0x98, 0x1a5): open or close (frame bit 1)
 * unless someone stands there
 */
/*C_2FA1_1B18*/C_2FA1_1B18(register int16 obj, int16 on, int16 to) {
	if(GetQual(obj) & 0x80) {
		CON_printf(C_2BB4_0295);
		return;
	}
	if(OBJ_FindNPCAt(0, GetX(obj), GetY(obj)) >= 0) {
		CON_printf(C_2BB4_0230);
		MUS_PlaySfx(5);
		return;
	}
	MUS_PlaySfx(0x31);
	ObjShapeType[obj] = GetType(obj) + ((GetFrame(obj) ^ 2) << 10);
}

/*
 * action 51 (rubber 0x14c or dye 0x7b on the sprayer 0x114): quality |= 1 / 2
 */
/*C_2FA1_1BBC*/C_2FA1_1BBC(int16 obj, int16 on, int16 to) {
	register int16 t;

	if((t = GetType(obj)) == 0x14c) {
		DeleteObj(obj);
		SetQual(on, GetQual(on) | 1);
		CON_printf(C_2BB4_0E6B);
	} else if(t == 0x7b) {
		DeleteObj(obj);
		SetQual(on, GetQual(on) | 2);
		CON_printf(C_2BB4_0E8F);
	}
}

/*
 * action 56 (doors 0xe3, 0xb3): automatic; why they don't open
 */
/*C_2FA1_1C2E*/C_2FA1_1C2E(int16 obj, int16 on, int16 to) {
	register int16 t;

	t = GetType(obj);
	CON_printf(C_2BB4_11F4);
	if((GetQual(obj) & 0xc0) == 0xc0)
		CON_printf(C_2BB4_1223);
	else if(GetQual(obj) & 0x80)
		CON_printf(C_2BB4_1242);
	else if(t == 0xe3 && !(TalkFlags[0x73] & 4) || t == 0xb3 && !(TalkFlags[0x73] & 0x10))
		CON_printf(C_2BB4_125C);
}

/*
 * actions 3 and 57 (the instruments 0x118-0x11b, a stick 0xe7 on the
 * gong 0x12a)
 */
/*C_2FA1_1CB6*/C_2FA1_1CB6(register int16 obj, int16 on, int16 to) {
	register int16 tmp;

	if(GetType(obj) == 0xe7) {
		tmp = obj;
		obj = on;
		on = tmp;
	}
	switch(GetType(obj)) {
		case 0x11a:
			CON_printf(C_2BB4_1337);
			MUS_PlaySfx(0x34);
			TimerDelay(476600L);
			MUS_PlaySfx(0x34);
			break;
		case 0x118:
			CON_printf(C_2BB4_132F);
			MUS_PlaySfx(0x36);
			break;
		case 0x119:
			CON_printf(C_2BB4_1346);
			MUS_PlaySfx(0x35);
			break;
		case 0x11b:
			CON_printf(C_2BB4_1325);
			MUS_PlaySfx(0x37);
			break;
		case 0x12a:
			CON_printf("GONG!!!!\n");
			MUS_PlaySfx(0xf);
			break;
		default:
			CON_printf("You play the %s.\n", GetObjectString(obj));
	}
}

/*
 * action 71 (bucket 0xa0 on a bucket): pour one into the other
 */
/*C_2FA1_1DAE*/C_2FA1_1DAE(int16 obj, int16 on, int16 to) {
	register int16 tmp;
	int16 fo, fn;

	fo = GetFrame(obj);
	fn = GetFrame(on);
	if(obj == on) {
		CON_printf(C_2BB4_1ECD);
		MUS_PlaySfx(5);
		return;
	}
	if(fo == 0 && fn > 0) {
		tmp = on;
		on = obj;
		obj = tmp;
		tmp = fn;
		fn = fo;
		fo = tmp;
	}
	if(fn > 0) {
		CON_printf(C_2BB4_1FA6);
		MUS_PlaySfx(5);
		return;
	}
	if(fo == 0) {
		CON_printf(C_2BB4_069A);
		MUS_PlaySfx(5);
		return;
	}
	ObjShapeType[on] = GetType(on) + (fo << 10);
	ObjShapeType[obj] = GetType(obj);
	if((ObjStatus[on] & 0x18) || (ObjStatus[obj] & 0x18))
		D_0F12 = 1;
	if((ObjStatus[on] & 0x18) == 0 || (ObjStatus[obj] & 0x18) == 0)
		ShowObjects();
}
