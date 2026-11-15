/* flags: -d -mm -p -c -Z -O -y */
/* stub order: 0000 0A85 098F 0CC0 0167 0D2A 0952 0B82 08BD */
/*
 * Martian Dreams - GAME.EXE overlay 2F83: conversation entry points and
 * the scripted events (Ultima VI's talk.c, by way of Savage Empire's:
 * SE GAME.EXE 0F79, plus the helpers of SE's talkdr.c 0F8D that MD moved
 * here).
 *
 * TALK_talkTo is the "talk" command: talking to an object (not an NPC)
 * gives a psychic impression when the active character has the talent,
 * some objects stand for an NPC (0x72-0x74), talking to yourself wakes you
 * up from the dream (D_16AA & 0x10). TALK_initTalk sets the script
 * variables ($G $N $P $T, #A #D #E #G #H #I #M #N #O #P #S #T #W #X #Y #Z;
 * clears $0-$9/#0-#9) and runs the script interpreter (TalkDriver, overlay
 * 2F88). A script's OP_EVENT runs TALK_Event, which may leave an event
 * number in TalkEvent for TALK_talkTo to run after the conversation.
 * Variables are indexed 0-9 for digits, then 'A' - 0x37 = 10 ... 'Z' = 35.
 */
#include "se.h"

/*other modules*/
extern int16 CON_mouseOffAt(int16, int16, int16, int16);
extern CON_mouseOn(int16);
extern cdecl CON_printf(char far *, ...);
extern CON_itoa(uint16, char far *, int16, int16);/*CON_itoa(number, str, max length, base)*/
extern TalkDriver(int16, int16);/*TalkDriver(talker, npc): overlay 2F88*/
extern OSI_resetVideo();
extern cdecl MEM_shutdown(int16);
extern void cdecl TimerDelay(int32);/*wait (2383 per millisecond)*/
extern void cdecl exit(int16);
extern int16 COMBAT_TryTeleport(int16, int16);/*(npc, ?): 0 makes TALK_Event delete the new NPC*/
extern int16 GetStr(int16);/*strength of an NPC (with modifiers?)*/
extern int16 GetDex(int16);/*an NPC attribute (#A)*/
extern int16 GetInt(int16);/*an NPC attribute (#I)*/
extern int16 MaxHP(int16);
extern SetTypeUnconscious(int16);
extern COMBAT_begin();/*called when not InCombat (COMBAT_begin?)*/
extern COMBAT_Missile(int16, int16 *, int16 *, int16, int16);
extern COMBAT_Kill(int16);
extern int16 HasConversation(int16);
extern LoadConversation(int16, unsigned char far *);/*LoadConversation(npc, buffer)*/
extern DisplayTick();
extern VIS_UpdateView();
extern MAP_LoadAround();
extern PartyTeleport(int16, int16, int16, int16);
extern int16 WrapDelta(int16);/*wraps a map distance*/
extern int16 MapDistance(int16, int16);/*distance of two map coordinates (wrapping)*/
extern int16 EGG_generate(int16, int16, int16, int16);/*EGG_generate(type, x, y, z)*/
extern C_1FA9_04ED();
extern MUS_PlaySfx(int16);
extern int16 OBJ_FindInContainer(int16, int16, int16);/*OBJ_FindInContainer(npc, type, quality)*/
extern int16 C_1B09_090B(int16);
extern int16 OBJ_FindTypeAt(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 FindInv(int16);
extern int16 NextInv();
extern int16 FindInvOfType(int16, int16);/*first object of a type in an inventory*/
extern int16 NextInvOfType();/*next one*/
extern int16 NextArea();/*next object of an area search*/
extern int16 SearchArea(int16, int16, int16, int16);/*first object of an area search*/
extern int16 OBJ_FindNPCAt(int16, int16, int16);
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);
extern int16 AddInvObj(int16, int16, int16, int16);
extern DeleteObj(int16);/*DeleteObj?*/
extern OBJ_PurgeFarLocals(int16, int16, int16);
extern OBJ_ReviveFromBody(int16);
extern MoveObj(int16, int16, int16, int16);/*MoveObj(obj, x, y, z)*/
extern OBJ_DeleteTmpBlocks();
extern ShowObjects();
extern char *GetObjectString(int16);
extern OBJ_DeleteContents(int16);
extern DreamWakeUp();
extern C_2F5F_0AC1(int16);
extern C_2F75_1429();
extern C_2F75_16A8();
extern char far C_2BB4_0000[];
extern char far C_2BB4_000B[];
extern char far C_2BB4_0022[];
extern char far C_2BB4_0045[];
extern char far C_2BB4_0315[];/*NotInSoloMsg*/
extern char far C_2BB4_114D[];
extern char far C_2BB4_22F1[];
#define NotInSoloMsg C_2BB4_0315

extern int16 D_0F18;/*MapAnimEnabled*/
extern int16 ForceHatching;
extern int16 VisibilityDirty;
extern unsigned char Time_H;
extern unsigned char Date_D;
extern unsigned char Date_M;
extern unsigned char Active;
extern int16 MapX, MapY, MapZ;
extern unsigned char D_1684, D_1685;
extern unsigned char DrunkCounter;
extern unsigned char D_16A3;
extern char InCombat;
extern char SoloMember;
extern unsigned char D_16A8;/*the player is a woman*/
extern int32 D_16AA;/*flags (0x10: ?)*/
extern struct { unsigned a:4, psychic:4, b:8; } D_16B0[];/*talents? (psychic: can read objects)*/
extern int16 DreamStage;
extern unsigned char far *D_3262;/*NPC flags: 0x80 ?, bits 0-7: talk flags 8-15*/
extern unsigned char D_3266[];/*ObjStatus*/
extern unsigned char far *D_46D0;/*NPCMode*/
extern unsigned char D_4EA4[];/*Party*/
extern char D_50CE[][14];/*Names*/
extern uint16 D_51AE[];/*ExpPoints*/
extern unsigned char far *D_53AE;/*NPCComMode*/
extern unsigned char far *D_54B2;/*ScratchBuf*/
extern int16 D_54BA[];/*Amount: quantity (low byte), quality (high byte)*/
extern unsigned char D_6CBA[];/*NPCStatus*/
extern unsigned char D_6DBE[];/*HitPoints*/
extern unsigned char D_6EC6[];/*TalkFlags*/
extern struct coord D_78AE[];/*ObjPos*/
extern unsigned char D_9CC0;/*PartySize*/
extern uint16 D_9CC4[];/*ObjShapeType*/
extern int16 far *D_B508;/*OrigShapeType*/
extern unsigned char D_C130[11][11];/*Obj_11x11*/
extern int16 D_C4AA[11][11];/*Tile_11x11*/
extern int16 D_EB86[10+26];/*VarInt*/
extern char *D_EBFA[10+26];/*VarStr*/
extern unsigned char D_EC5C[];/*TalkNpcName*/
extern char D_ECE2[10][100];/*TalkStrBuf: storage of the string variables $0-$9*/

#define MapAnimEnabled D_0F18
#define ObjStatus D_3266
#define NPCMode D_46D0
#define Party D_4EA4
#define Names D_50CE
#define ExpPoints D_51AE
#define NPCComMode D_53AE
#define ScratchBuf D_54B2
#define Amount D_54BA
#define NPCStatus D_6CBA
#define HitPoints D_6DBE
#define TalkFlags D_6EC6
#define PartySize D_9CC0
#define ObjShapeType D_9CC4
#define OrigShapeType D_B508
#define VarInt D_EB86
#define VarStr D_EBFA
#define TalkNpcName D_EC5C
#define TalkStrBuf D_ECE2

#define IN_VEHICLE (Active == PartySize)
#define IsPlrControl(i)  (NPCStatus[i] & 0x80)
#define GetAlignment(i)  (NPCStatus[i] & 0x60)
#define SetAlignment(i, v) (NPCStatus[i] = (NPCStatus[i] & ~0x60) | (v))
#define NEUTRAL 0
#define EVIL 0x20
#define GOOD 0x40
#define CHAOTIC 0x60
#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetX(i) ((*(int16 *)&(D_78AE[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(D_78AE[i].raw[1]))>>2)&0x3ff)
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])
#define CanReadObjects() (D_16B0[Party[Active]].psychic)

char *TALK_GetCharName(int16);

/*====---- ----====*/
/*event left by TALK_Event, run by TALK_talkTo after the conversation*/
/*247A*/int16 TalkEvent = -1;
/*====---- ----====*/

/*start a conversation between the player and NPC objNum*/
/*C_2F83_0000*/TALK_initTalk(int16 objNum) {
	register int16 i;
	int16 handle;

	/*set variables #*/
	VarInt['A' - 0x37] = GetDex(1);
	VarInt['D' - 0x37] = Date_D;/*[d]ay*/
	VarInt['E' - 0x37] = ExpPoints[1];/*[e]xperience*/
	VarInt['H' - 0x37] = Time_H;/*[h]our*/
	VarInt['I' - 0x37] = GetInt(1);
	VarInt['M' - 0x37] = Date_M;/*[m]onth*/
	VarInt['N' - 0x37] = PartySize;
	VarInt['P' - 0x37] = HitPoints[1];
	VarInt['W' - 0x37] = NPCMode[objNum];/*NPC's [w]ork type*/
	VarInt['S' - 0x37] = GetStr(1);/*[s]trength*/
	VarInt['T' - 0x37] = 0;
	VarInt['W' - 0x37] = NPCMode[objNum];
	VarInt['X' - 0x37] = MapX;
	VarInt['Y' - 0x37] = MapY;
	VarInt['Z' - 0x37] = MapZ;
	VarInt['O' - 0x37] = 0;
	for(i = 0; i < 0x100; i ++)
		if(C_1B09_090B(i))
			VarInt['O' - 0x37] ++;
	/*set strings $*/
	VarStr['N' - 0x37] = "Winona Ryder is a really hot babe";/*[n]ame of NPC (a buffer)*/
	VarStr['P' - 0x37] = Names[0];/*name of [p]layer*/
	if(Time_H < 12)
		VarStr['T' - 0x37] = "morning";
	else if(Time_H < 18)
		VarStr['T' - 0x37] = "afternoon";
	else
		VarStr['T' - 0x37] = "evening";
	if(D_16A8) {
		VarStr['G' - 0x37] = "Madam";
		VarInt['G' - 0x37] = 1;
	} else {
		VarStr['G' - 0x37] = "Sir";
		VarInt['G' - 0x37] = 0;
	}
	/*-- reset $0~$9 --*/
	for(i = 0; i < 10; i ++)
		VarStr[i] = TalkStrBuf[i];
	/*-- reset #0~#9 --*/
	for(i = 0; i < 10; i ++)
		VarInt[i] = 0;
	/*--*/
	handle = CON_mouseOffAt(0, 0, 319, 199);
	TalkDriver((D_16AA & 0x10)?0:1, objNum);
	CON_mouseOn(handle);
}

/*"talk" command: party member #partyId (unused) talks to objNum;
  showName: print the target's name first*/
/*C_2F83_0167*/TALK_talkTo(int16 partyId, register int16 objNum, int16 showName) {
	int16 obj2;
	int16 x, y;
	int16 savedSolo;
	register int16 k;

	savedSolo = SoloMember;
	if(showName == 0 && SoloMember >= 0)
		SoloMember = -1;
	if((objNum >= 0x100 || objNum < 0) && CanReadObjects()) {
		if(ObjStatus[objNum] & 1) {
			switch(GetType(objNum)) {
				case 0xbc: case 0xbf: case 0xc0:
					objNum = 0x72;
				break;
				case 0xc8: case 0xc9: case 0xd6: case 0xd7:
					objNum = 0x73;
				break;
				case 0x64: case 0x120: case 0x121: case 0x122:
					objNum = 0x74;
				break;
				case 0xf7:
					CON_printf("This saw was used by a Martian doctor, or \"Arborist&, for surgery.\n");
				break;
				case 0xf9:
					CON_printf("This device was used by a Martian doctor, or \"Arborist&, on patients.\n");
				break;
				case 0x5c:
					CON_printf("This was worn last by a dying man.\n");
				break;
				case 0xc:
					CON_printf("You receive a momentary impression of a house falling on you.\n");
				break;
				case 0xe7:
					CON_printf("This was used by a Martian whose entire life focussed on measuring time.\n");
				break;
				case 0xea:
					CON_printf("This jewelry was worn by a distinguished Martian, a mayor or \"Agrarian&.\n");
				break;
				case 0xf6:
					CON_printf("These footbags were worn by an elderly, distinguished Martian whose job, ");
					CON_printf("\"Gatherer&, has no human analog - a combination of undertaker, dustman, ");
					CON_printf("and treasurer.\n");
				break;
				case 0x113:
					CON_printf("This sculpture had great ritual or religious significance to a city, or ");
					CON_printf("\"Grove&, of Martians.  The image is of \"very proud soil&.");
				break;
			}
		} else {
			CON_printf("You receive no psychic impressions.\n");
			if(savedSolo >= 0)
				SoloMember = savedSolo;
			return;
		}
	}
	if(objNum < 0x100 && objNum >= 0 && (!(ObjStatus[objNum] & 1) || CanReadObjects())) {
		if(objNum <= 1) {
			if(showName)
				CON_printf("yourself\n");
			if(D_16AA & 0x10) {
				CON_printf("You try to wake yourself up.\n");
				if(MapZ != 3)
					DreamWakeUp();
			} else if(SoloMember >= 0) {
				CON_printf(NotInSoloMsg);
			} else {
				CON_printf("You aren't yet that insane.\n");
			}
		} else if(GetType(objNum) == 0x187) {
			CON_printf("%s\n", GetObjectString(objNum));
			CON_printf(C_2BB4_114D, D_16A8?"wo":"");
			if(savedSolo >= 0)
				SoloMember = savedSolo;
			return;
		} else {
			if(showName)
				CON_printf("%s\n", (TalkFlags[objNum] & 1)?
					/*met already*/TALK_GetCharName(objNum):
					/*first time*/GetObjectString(objNum)
				);
			TALK_initTalk(objNum);
			if(TalkEvent >= 0) {
				DisplayTick();
				switch(TalkEvent) {
					case 9:
						C_2F75_16A8();
					break;
					case 10:
						C_2F75_1429();
					break;
					case 0x38:
						if((TalkFlags[0x55] & 0xc) != 8)
							break;
						C_2F5F_0AC1(0x55);
					case 0x34: case 0x36:
						DreamWakeUp();
					break;
					case 0x65:
						k = AddMapObj(0xd24, 0, 0xe00, 0x34, 0x20, MapZ);
						obj2 = AddMapObj(0x5cd, 0, 0x340b, 0x34, 0x21, MapZ);
						if(k < 0 || obj2 < 0) {
							CON_printf(C_2BB4_22F1);
							return;
						}
						ObjStatus[obj2] = 0xc1;
						if((k = OBJ_FindNPCAt(0, 0x34, 0x20)) == 0) {
							MoveObj(0, 0x34, 0x22, MapZ);
							MapX = 0x34;
							MapY = 0x22;
							MAP_LoadAround();
						}
						VisibilityDirty = 1;
					break;
					case 0x66:
						k = AddMapObj(0xd24, 0, 0xf00, 0x12, 0x22, MapZ);
						obj2 = AddMapObj(0x5cd, 0, 0x1b1a, 0x12, 0x23, MapZ);
						if(k < 0 || obj2 < 0) {
							CON_printf(C_2BB4_22F1);
							return;
						}
						ObjStatus[obj2] = 0xc5;
						if((k = OBJ_FindNPCAt(0, 0x12, 0x22)) == 0) {
							MoveObj(0, 0x13, 0x23, MapZ);
							MapX = 0x13;
							MapY = 0x23;
							MAP_LoadAround();
						}
						VisibilityDirty = 1;
						TalkFlags[0x68] &= 1;
						if((k = OBJ_FindTypeAt(0x155, 0x14, 0xb)) >= 0)
							ObjShapeType[k] = 0x189;
						if((k = OBJ_FindTypeAt(0x155, 0x16, 0xb)) >= 0)
							ObjShapeType[k] = 0x589;
						if((k = OBJ_FindTypeAt(0x155, 0x18, 0xb)) >= 0)
							ObjShapeType[k] = 0x989;
					break;
					case 0x67:
						if(TalkFlags[0x68] & 4) {
							if(TalkFlags[0x68] & 8) {
								if(TalkFlags[0x68] & 0x10) {
									x = GetX(0);
									y = GetY(0);
									k = 1;
								} else {
									TalkFlags[0x68] |= 0x10;
									x = 0x18;
									y = 0xb;
									k = 5;
								}
							} else {
								TalkFlags[0x68] |= 8;
								x = 0x16;
								y = 0xb;
								k = 3;
							}
						} else {
							TalkFlags[0x68] |= 4;
							x = 0x14;
							y = 0xb;
							k = 4;
						}
						MUS_PlaySfx(2);
						CON_printf("`%s ", GetObjectString(0x68));
						if(!(TalkFlags[0x68] & 8)) {
							CON_printf(C_2BB4_000B);
							CON_printf(C_2BB4_0022);
						} else {
							CON_printf(C_2BB4_0000);
						}
						MUS_PlaySfx(10);
						COMBAT_Missile(0x68, &x, &y, 9, 0x16a);
						MUS_PlaySfx(1);
						CON_printf(C_2BB4_0045, GetObjectString(k));
						if(k == 1) {
							SetTypeUnconscious(0);
							SetTypeUnconscious(0x68);
							VisibilityDirty = 1;
							DisplayTick();
							TimerDelay(0x122e4cL);
							ObjShapeType[0] = OrigShapeType[1];
							ObjShapeType[0x68] = OrigShapeType[0x68];
							PartyTeleport(D_1684, D_1685, MapZ, 0);
						} else {
							if((obj2 = OBJ_FindTypeAt(0x189, x, y)) >= 0)
								ObjShapeType[obj2] = 0x155 | ((GetType(k) - 0x156) << 10);
							else
								CON_printf(C_2BB4_22F1);
							if((obj2 = OBJ_FindTypeAt(0xae, x, y)) >= 0)
								MoveObj(obj2, x, y, MapZ);
							else
								CON_printf(C_2BB4_22F1);
						}
					break;
					case 0x68:
						k = AddMapObj(0xd24, 0, 0x1000, 0x6a, 0x2a, MapZ);
						obj2 = AddMapObj(0x5cd, 0, 0x5328, 0x6a, 0x2b, MapZ);
						if(k < 0 || obj2 < 0) {
							CON_printf(C_2BB4_22F1);
							return;
						}
						ObjStatus[obj2] = 0xe0;
						if((k = OBJ_FindNPCAt(0, 0x6a, 0x2a)) == 0) {
							MoveObj(0, 0x6b, 0x2b, MapZ);
							MapX = 0x6b;
							MapY = 0x2b;
							MAP_LoadAround();
						}
						VisibilityDirty = 1;
						TalkFlags[0x69] |= 1;
					break;
					case 0x69:
						ShowObjects();
				}
				TalkEvent = -1;
			}
		}
	} else {
		CON_printf("nothing!\n");
	}
	if(savedSolo >= 0)
		SoloMember = savedSolo;
}

/*name of an NPC from its conversation script (the bytes after the
OP_SCRIPT_MARK/npc number) into TalkNpcName, or "NPC 0x<num>";
returns TalkNpcName*/
/*C_2F83_08BD*/unsigned char *TALK_GetNpcName(int16 objNum) {
	int16 i, j;

	i = 0;
	j = 2;
	if(HasConversation(objNum)) {
		LoadConversation(objNum, ScratchBuf);
		for(j = 2; i < 50; i ++, j ++)
			TalkNpcName[i] = ScratchBuf[j];
	} else {
		CON_printf(C_2BB4_22F1);
		strcpy(TalkNpcName, "NPC 0x");
		CON_itoa(objNum, TalkNpcName + 6, 4, 16);
		return TalkNpcName;
	}
	i = 0;
	while((TalkNpcName[i] & 0x80) == 0 && TalkNpcName[i++]);
	TalkNpcName[i] = 0;

	return TalkNpcName;
}

/*name of a character: party members use the party name table (Names[]),
other npcs the name from their conversation script*/
/*C_2F83_0952*/char *TALK_GetCharName(register int16 objNum) {
	register int16 i;

	if(IsPlrControl(objNum)) {
		for(i = 0; i < 8; i ++)
			if(Party[i] == objNum)
				break;
		return Names[i];
	}
	TALK_GetNpcName(objNum);

	return (char *)TalkNpcName;
}

/*is object onscreen? (within the visible 9x9 part of the 11x11 view
and not on a hidden/dark tile)*/
/*C_2F83_098F*/IsNpcOnScreen(int16 objNum) {
	register int16 x;
	register int16 y;
	int16 tile;

	if(MapDistance(GetX(objNum), MapX) < 5 && MapDistance(GetY(objNum), MapY) < 5) {
		x = WrapDelta(GetX(objNum) - (MapX - 5));
		y = WrapDelta(GetY(objNum) - (MapY - 5));
		/*objects displayed there, or an object tile*/
		if(D_C130[y][x] || D_C4AA[y][x] >= 0x200)
			return 1;
		tile = D_C4AA[y][x];
		return (tile != 0xff && tile != 0x10c);
	}
	return 0;
}

/*add npc to the party: at most 5 members
returns 0:ok (or already in the party), 1:party is in a vehicle, 2:party is full*/
/*C_2F83_0A85*/JoinParty(int16 objNum) {
	unsigned char *name;
	int16 nameLen;
	register int16 i;
	register int16 freeSlots;

	freeSlots = 5;
	for(i = 0; i < PartySize; i ++) {
		freeSlots --;
		if(Party[i] == objNum)
			return 0;
	}
	if(freeSlots < 1)
		return 2;
	if(IN_VEHICLE)
		return 1;
	PartySize ++;
	Party[PartySize - 1] = objNum;
	name = TALK_GetNpcName(objNum);
	nameLen = strlen(name);
	if(nameLen >= 14)
		strncpy(Names[PartySize - 1], name, 13);
	else
		strcpy(Names[PartySize - 1], name);
	NPCStatus[objNum] |= 0x80;
	D_3262[objNum] &= 0x7f;
	if(GetAlignment(objNum) == NEUTRAL)
		SetAlignment(objNum, GOOD);
	NPCMode[objNum] = 1;
	NPCComMode[objNum] = 8;
	return 0;
}

/*remove npc from the party; he drops his whole inventory at his feet
returns 0:ok, 1:party is in a vehicle, 2:not in party*/
/*C_2F83_0B82*/LeaveParty(register int16 objNum) {
	int16 slot, invObj;
	register int16 i;

	if(IN_VEHICLE)
		return 1;
	for(slot = 0; slot < PartySize; slot ++)
		if(Party[slot] == objNum)
			break;
	if(PartySize == slot)
		return 2;
	for(i = slot; i < PartySize; i ++) {
		Party[i] = Party[i + 1];
		strcpy(Names[i], Names[i + 1]);
	}
	PartySize --;
	NPCStatus[objNum] &= ~0x80;
	if(GetAlignment(objNum) == GOOD)
		SetAlignment(objNum, NEUTRAL);
	if(D_3262[objNum] & 0x80) {
		NPCMode[objNum] = 0x92;
	} else {
		NPCMode[objNum] = 0x8f;
		for(invObj = FindInv(objNum); invObj >= 0; invObj = FindInv(objNum))
			MoveObj(invObj, GetX(objNum), GetY(objNum), MapZ);
		MoveObj(objNum, GetX(objNum), GetY(objNum), MapZ);
	}
	return 0;
}

/*returns the first onscreen party member carrying an object of type
<objType> (and quality <objQual>), or -1*/
/*C_2F83_0CC0*/FindPartyMemberWithObj(int16 objType, int16 objQual) {
	int16 i;

	if(D_16AA & 0x10) {/*in the dream: only the player*/
		if(OBJ_FindInContainer(0, objType, objQual) != -1)
			return 0;
	} else {
		for(i = 0; i < PartySize; i ++) {
			if(IsNpcOnScreen(Party[i]) && OBJ_FindInContainer(Party[i], objType, objQual) != -1)
				return Party[i];
		}
	}
	return -1;
}

/*OP_EVENT: scripted events of the story*/
/*C_2F83_0D2A*/TALK_Event(int16 event) {
	int16 count;
	int16 found;
	int16 obj;
	register int16 k;
	register int16 npc;

	switch(event) {
		case 1:
			count = 0;
			for(k = 0; k < PartySize; k ++) {
				for(npc = FindInvOfType(Party[k], 0x1ca); npc >= 0; npc = NextInvOfType()) {
					count ++;
					MUS_PlaySfx(4);
					GetQual(npc) &= 0xfd;
					GetQuan(npc) = 4;
				}
			}
			if(count > 1)
				TalkFlags[0x39] |= 8;
			else
				TalkFlags[0x39] &= ~8;
		break;
		case 2:
			COMBAT_Kill(0x3c);
		break;
		case 3:
		break;
		case 4:
			D_3262[0x1e] &= 0x7f;
			D_3262[0x18] &= 0x7f;
		break;
		case 5:
			D_3262[0x18] |= 0x80;
			NPCMode[0x18] = 0;
			MoveObj(0x18, 0x18, 0x18, 6);
		break;
		case 6:
			if((k = OBJ_FindTypeAt(0xb5, 0x2c3, 0x1f3)) < 0) {
				CON_printf(C_2BB4_22F1);
			} else {
				ObjShapeType[k] = GetType(k) + 0xc00;
				MoveObj(k, 0x2c2, 0x1f3, 0);
			}
			if((k = OBJ_FindTypeAt(0xb5, 0x2c4, 0x1f3)) < 0)
				CON_printf(C_2BB4_22F1);
			else
				ObjShapeType[k] = GetType(k) + 0x1c00;
		break;
		case 7:
			if((k = OBJ_FindTypeAt(0x98, 0x2c7, 0x1dc)) < 0)
				CON_printf(C_2BB4_22F1);
			else
				ObjShapeType[k] = GetType(k) + 0x1c00;
		break;
		case 8:
			for(k = SearchArea(0x9a, 0xbb, 0xaa, 0xcb); k >= 0; k = NextArea()) {
				if(GetType(k) == 0x19a && (ObjShapeType[k] >> 10) > 3 && GetQual(k) == 3) {
					TalkFlags[0x3a] |= 4;
					break;
				}
			}
		break;
		case 9:
			TalkEvent = 9;
		break;
		case 10:
			TalkEvent = 10;
		break;
		case 0xb:
			if((k = OBJ_FindTypeAt(0x98, 0x13f, 0x194)) < 0)
				CON_printf(C_2BB4_22F1);
			else {
				GetQual(k) = 0;
				ObjShapeType[k] = GetType(k) + 0x1c00;
			}
		break;
		case 0x33:
			if((k = OBJ_FindTypeAt(0x4f, 9, 0x1f)) < 0)
				CON_printf(C_2BB4_22F1);
			else
				ObjShapeType[k] = GetType(k) + 0x400;
		break;
		case 0x34:
			C_2F5F_0AC1(0x5c);
			TalkEvent = 0x34;
		break;
		case 0x35:
			C_1FA9_04ED();
			D_16A3 = 0;
			for(k = 0; k < 6; k ++) {
				if((npc = EGG_generate(0x91, MapX, MapY, MapZ)) >= 0) {
					if(COMBAT_TryTeleport(npc, 0)) {
						NPCComMode[npc] = NPCMode[npc] = 8;
						D_16A3 ++;
					} else {
						DeleteObj(npc);
					}
				}
			}
			if(!D_16A3)
				CON_printf(C_2BB4_22F1);
		break;
		case 0x36:
			C_2F5F_0AC1(0x52);
			TalkEvent = 0x36;
		break;
		case 0x37:
			MoveObj(0x56, 0x56, 0x56, 6);
			OBJ_PurgeFarLocals(-1, -1, -1);
			if((k = AddInvObj(0x9e, 0x10, 4, 0)) < 0) {
				CON_printf(C_2BB4_22F1);
				break;
			}
			if((k = AddInvObj(0xf2, 0x10, 0, 0)) < 0) {
				CON_printf(C_2BB4_22F1);
				break;
			}
			if((k = AddInvObj(0x4a0, 0x10, 0, 0)) < 0) {
				CON_printf(C_2BB4_22F1);
				break;
			}
			if((k = AddMapObj(0x101, 0, 0, 0x19, 0x6f, MapZ)) < 0) {
				CON_printf(C_2BB4_22F1);
				break;
			}
			if((npc = AddMapObj(0xfb, 0, 0xf00, 0x19, 0x6f, MapZ)) < 0) {
				CON_printf(C_2BB4_22F1);
				break;
			}
			if((npc = AddInvObj(0xfc, 8, 0, k)) < 0) {
				CON_printf(C_2BB4_22F1);
				break;
			}
			for(k = 0; k < 4; k ++) {
				if((npc = EGG_generate(0x56e, 0xa, 0x6f, MapZ)) >= 0) {
					NPCComMode[npc] = NPCMode[npc] = 0x18;
					if(!COMBAT_TryTeleport(npc, 1))
						DeleteObj(npc);
				} else {
					CON_printf(C_2BB4_22F1);
				}
			}
			for(k = 0; k < 4; k ++) {
				if((npc = EGG_generate(0x56e, 0x19, 0x60, MapZ)) >= 0) {
					NPCComMode[npc] = NPCMode[npc] = 0x18;
					if(!COMBAT_TryTeleport(npc, 1))
						DeleteObj(npc);
				} else {
					CON_printf(C_2BB4_22F1);
				}
			}
			for(k = 0; k < 4; k ++) {
				if((npc = EGG_generate(0x56e, 0x28, 0x6f, MapZ)) >= 0) {
					NPCComMode[npc] = NPCMode[npc] = 0x18;
					if(!COMBAT_TryTeleport(npc, 1))
						DeleteObj(npc);
				} else {
					CON_printf(C_2BB4_22F1);
				}
			}
			VIS_UpdateView();
			MapAnimEnabled = 1;
			C_1FA9_04ED();
		break;
		case 0x38:
			TalkFlags[0x55] |= 0x18;
			TalkFlags[0x55] &= 0xfb;
			count = 0;
			found = 0;
			for(obj = SearchArea(0x49, 0x30, 0x57, 0x51); obj >= 0; obj = NextArea()) {
				switch(GetType(obj)) {
					case 0x84:
						count ++;
						TalkFlags[0x55] &= 0xef;
						if(found != 0 && GetQuan(obj) != found) {
							TalkFlags[0x55] &= 0xf7;
							found = -1;
						} else if(found == 0) {
							found = GetQuan(obj);
						}
					break;
					case 0x1a9: case 0x1aa:
						TalkFlags[0x55] |= 4;
					break;
				}
			}
			for(obj = FindInv(0); obj >= 0; obj = NextInv()) {
				switch(GetType(obj)) {
					case 0x84:
						TalkFlags[0x55] &= 0xe7;
						TalkFlags[0x55] |= 0x20;
					break;
					case 0x1a9: case 0x1aa:
						TalkFlags[0x55] |= 4;
					break;
				}
			}
			if(count != 0 && count != 0x19)
				TalkFlags[0x55] &= 0xf7;
			if(found != 0 && found != 10)
				TalkFlags[0x55] &= 0xf7;
			TalkEvent = 0x38;
		break;
		case 0x39:
			for(k = SearchArea(8, 0x18, 0x16, 0x39); k >= 0; k = NextArea()) {
				if(GetType(k) == 0x155 && GetQual(k) == 0x5b)
					break;
			}
			if(k < 0) {
				CON_printf(C_2BB4_22F1);
				break;
			}
			OBJ_ReviveFromBody(k);
			NPCStatus[0x5b] &= 0xef;
			HitPoints[0x5b] = MaxHP(0x5b);
			NPCStatus[0x5b] &= 0xf7;
			ObjShapeType[0x5b] = OrigShapeType[0x5b];
			NPCStatus[0x5b] &= 0x9f;
			TalkFlags[0x5b] &= 0xef;
		break;
		case 0x65:
			C_1FA9_04ED();
			TalkEvent = 0x65;
		break;
		case 0x66:
			C_1FA9_04ED();
			TalkEvent = 0x66;
		break;
		case 0x67:
			TalkEvent = 0x67;
		break;
		case 0x68:
			C_1FA9_04ED();
			TalkEvent = 0x68;
		break;
		case 0x69:
			MUS_PlaySfx(0x32);
			if(GetX(0x67) == 0x2c) {
				if((k = OBJ_FindTypeAt(0xdb, 0x3b, 0xc)) < 0) {
					CON_printf(C_2BB4_22F1);
					break;
				}
				ObjShapeType[k] = GetType(k) + 0xc00;
			} else {
				if((k = OBJ_FindTypeAt(0xdb, 0x2b, 0xf)) < 0) {
					CON_printf(C_2BB4_22F1);
					break;
				}
				ObjShapeType[k] = GetType(k) + 0x1c00;
			}
			ObjShapeType[0x67] = (ObjShapeType[0x67] & 0xfc00) + 0x171;
			TalkEvent = 0x69;
		break;
		case 0x6a:
			PartyTeleport(D_1684, D_1685, MapZ, 0);
		break;
		case 0x6b:
			MUS_PlaySfx(4);
			OBJ_PurgeFarLocals(-1, -1, -1);
			PartyTeleport(0x6a, 0x2c, MapZ, 0);
			OBJ_DeleteContents(0);
			D_1684 = 0x6a;
			D_1685 = 0x2c;
			DreamStage = 0xe4;
		break;
		case 0x6c:
			if(!InCombat)
				COMBAT_begin();
			D_16A3 = 6;
			OBJ_PurgeFarLocals(-1, -1, -1);
			ForceHatching = 1;
			PartyTeleport(0x6e, 0x17, MapZ, 0);
			ForceHatching = 0;
			if((k = OBJ_FindTypeAt(0xe0, 0x75, 0x17)) >= 0)
				break;
			if((k = AddMapObj(0xce0, 0, 0, 0x75, 0x17, MapZ)) < 0)
				CON_printf(C_2BB4_22F1);
			else
				ObjStatus[k] &= 0xdf;
		break;
		case 0x6d:
			if(!InCombat)
				COMBAT_begin();
			D_16A3 = 6;
			ForceHatching = 1;
			OBJ_PurgeFarLocals(-1, -1, -1);
			PartyTeleport(0x7a, 0xd, MapZ, 0);
			ForceHatching = 0;
			if((k = OBJ_FindTypeAt(0xe0, 0x7a, 0x12)) >= 0)
				break;
			if((k = AddMapObj(0x1ce0, 0, 0, 0x7a, 0x12, MapZ)) < 0)
				CON_printf(C_2BB4_22F1);
			else
				ObjStatus[k] &= 0xdf;
		break;
		case 0x6e:
			if(!InCombat)
				COMBAT_begin();
			ForceHatching = 1;
			OBJ_PurgeFarLocals(-1, -1, -1);
			PartyTeleport(0x82, 0x18, MapZ, 0);
			ForceHatching = 0;
		break;
		case 0x6f:
			HitPoints[0] = MaxHP(1);
		break;
		case 0xfe:
			DrunkCounter ++;
		break;
		case 0xff:/*end of the game: back to DOS*/
			OBJ_DeleteTmpBlocks();
			OSI_resetVideo();
			MEM_shutdown(8);
			exit(0);
		default:
			CON_printf("CallGuards(%d)!\n", event);
	}
}
