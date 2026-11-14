/* flags: -d -mm -p -c -Z -O -y */
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
extern int16 CON_mouseOffAt(int16, int16, int16, int16);/*CON_mouseOffAt*/
extern CON_mouseOn(int16);/*CON_mouseOn*/
extern cdecl CON_printf(char far *, ...);/*CON_printf*/
extern CON_itoa(uint16, char far *, int16, int16);/*CON_itoa(number, str, max length, base)*/
extern TalkDriver(int16, int16);/*TalkDriver(talker, npc): overlay 2F88*/
extern OSI_resetVideo();/*OSI_resetVideo*/
extern cdecl MEM_shutdown(int16);
extern void cdecl TimerDelay(int32);/*wait (2383 per millisecond)*/
extern void cdecl exit(int16);
extern int16 C_07D1_0416(int16, int16);/*(npc, ?): 0 makes TALK_Event delete the new NPC*/
extern int16 C_07D1_0521(int16);/*strength of an NPC (with modifiers?)*/
extern int16 C_07D1_0576(int16);/*an NPC attribute (#A)*/
extern int16 C_07D1_05C0(int16);/*an NPC attribute (#I)*/
extern int16 C_07D1_064E(int16);/*MaxHP*/
extern C_07D1_06C1(int16);/*SetTypeUnconscious*/
extern C_07D1_01BB();/*called when not InCombat (COMBAT_begin?)*/
extern C_07D1_0CFC(int16, int16 *, int16 *, int16, int16);
extern C_07D1_15F0(int16);
extern int16 C_0E1A_175B(int16);/*HasConversation*/
extern C_0E1A_16F2(int16, unsigned char far *);/*LoadConversation(npc, buffer)*/
extern C_120E_0221();/*DisplayTick*/
extern C_1157_0312();/*VIS_UpdateView*/
extern C_1534_056F();
extern C_1534_0906(int16, int16, int16, int16);
extern int16 C_1534_1101(int16);/*wraps a map distance*/
extern int16 C_1534_1129(int16, int16);/*distance of two map coordinates (wrapping)*/
extern int16 C_1649_009C(int16, int16, int16, int16);/*EGG_generate(type, x, y, z)*/
extern C_1FA9_04ED();
extern C_1FA9_0919(int16);
extern int16 C_1B09_0002(int16, int16, int16);/*OBJ_FindInContainer(npc, type, quality)*/
extern int16 C_1B09_090B(int16);
extern int16 C_1B09_09C7(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 C_1B09_0B1D(int16);/*FindInv*/
extern int16 C_1B09_0B76();/*NextInv*/
extern int16 C_1B09_0BEE(int16, int16);/*first object of a type in an inventory*/
extern int16 C_1B09_0CA4();/*next one*/
extern int16 C_1B09_0DD7();/*next object of an area search*/
extern int16 C_1B09_0EDE(int16, int16, int16, int16);/*first object of an area search*/
extern int16 C_1B09_1081(int16, int16, int16);
extern int16 C_1B09_116E(int16, int16, int16, int16, int16, int16);/*AddMapObj*/
extern int16 C_1B09_12EE(int16, int16, int16, int16);
extern C_1B09_1872(int16);/*DeleteObj?*/
extern C_1B09_1A1D(int16, int16, int16);
extern C_1B09_1DF5(int16);
extern C_1B09_1F52(int16, int16, int16, int16);/*MoveObj(obj, x, y, z)*/
extern C_1B09_2CD2();/*OBJ_DeleteTmpBlocks*/
extern C_1B09_3E18();
extern char *C_1B09_4759(int16);/*GetObjectString*/
extern C_1B09_4963(int16);
extern C_2F5F_0477();
extern C_2F5F_0AC1(int16);
extern C_2F75_1429();
extern C_2F75_16A8();
#define MaxHP C_07D1_064E
#define SetTypeUnconscious C_07D1_06C1
#define HasConversation C_0E1A_175B
#define LoadConversation C_0E1A_16F2
#define DisplayTick C_120E_0221
#define VIS_UpdateView C_1157_0312
#define EGG_generate C_1649_009C
#define OBJ_FindInContainer C_1B09_0002
#define OBJ_FindTypeAt C_1B09_09C7
#define FindInv C_1B09_0B1D
#define NextInv C_1B09_0B76
#define AddMapObj C_1B09_116E
#define DeleteObj C_1B09_1872
#define MoveObj C_1B09_1F52
#define OBJ_DeleteTmpBlocks C_1B09_2CD2
#define GetObjectString C_1B09_4759
extern char far C_2BB4_0000[];
extern char far C_2BB4_000B[];
extern char far C_2BB4_0022[];
extern char far C_2BB4_0045[];
extern char far C_2BB4_0315[];/*NotInSoloMsg*/
extern char far C_2BB4_114D[];
extern char far C_2BB4_22F1[];
#define NotInSoloMsg C_2BB4_0315

extern int16 D_0F18;/*MapAnimEnabled*/
extern int16 D_13F0;
extern int16 D_1431;
extern unsigned char D_1675;/*Time_H*/
extern unsigned char D_1676;/*Date_D*/
extern unsigned char D_1677;/*Date_M*/
extern unsigned char D_167A;/*Active*/
extern int16 D_167C, D_167E, D_1680;/*MapX, MapY, MapZ*/
extern unsigned char D_1684, D_1685;
extern unsigned char D_1686;
extern unsigned char D_16A3;
extern char D_16A4;/*InCombat*/
extern char D_16A5;/*SoloMember*/
extern unsigned char D_16A8;/*the player is a woman*/
extern int32 D_16AA;/*flags (0x10: ?)*/
extern struct { unsigned a:4, psychic:4, b:8; } D_16B0[];/*talents? (psychic: can read objects)*/
extern int16 D_16D4;
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
#define Time_H D_1675
#define Date_D D_1676
#define Date_M D_1677
#define Active D_167A
#define MapX D_167C
#define MapY D_167E
#define MapZ D_1680
#define InCombat D_16A4
#define SoloMember D_16A5
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
	VarInt['A' - 0x37] = C_07D1_0576(1);
	VarInt['D' - 0x37] = Date_D;/*[d]ay*/
	VarInt['E' - 0x37] = ExpPoints[1];/*[e]xperience*/
	VarInt['H' - 0x37] = Time_H;/*[h]our*/
	VarInt['I' - 0x37] = C_07D1_05C0(1);
	VarInt['M' - 0x37] = Date_M;/*[m]onth*/
	VarInt['N' - 0x37] = PartySize;
	VarInt['P' - 0x37] = HitPoints[1];
	VarInt['W' - 0x37] = NPCMode[objNum];/*NPC's [w]ork type*/
	VarInt['S' - 0x37] = C_07D1_0521(1);/*[s]trength*/
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
					C_2F5F_0477();
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
						C_2F5F_0477();
					break;
					case 0x65:
						k = AddMapObj(0xd24, 0, 0xe00, 0x34, 0x20, MapZ);
						obj2 = AddMapObj(0x5cd, 0, 0x340b, 0x34, 0x21, MapZ);
						if(k < 0 || obj2 < 0) {
							CON_printf(C_2BB4_22F1);
							return;
						}
						ObjStatus[obj2] = 0xc1;
						if((k = C_1B09_1081(0, 0x34, 0x20)) == 0) {
							MoveObj(0, 0x34, 0x22, MapZ);
							MapX = 0x34;
							MapY = 0x22;
							C_1534_056F();
						}
						D_1431 = 1;
					break;
					case 0x66:
						k = AddMapObj(0xd24, 0, 0xf00, 0x12, 0x22, MapZ);
						obj2 = AddMapObj(0x5cd, 0, 0x1b1a, 0x12, 0x23, MapZ);
						if(k < 0 || obj2 < 0) {
							CON_printf(C_2BB4_22F1);
							return;
						}
						ObjStatus[obj2] = 0xc5;
						if((k = C_1B09_1081(0, 0x12, 0x22)) == 0) {
							MoveObj(0, 0x13, 0x23, MapZ);
							MapX = 0x13;
							MapY = 0x23;
							C_1534_056F();
						}
						D_1431 = 1;
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
						C_1FA9_0919(2);
						CON_printf("`%s ", GetObjectString(0x68));
						if(!(TalkFlags[0x68] & 8)) {
							CON_printf(C_2BB4_000B);
							CON_printf(C_2BB4_0022);
						} else {
							CON_printf(C_2BB4_0000);
						}
						C_1FA9_0919(10);
						C_07D1_0CFC(0x68, &x, &y, 9, 0x16a);
						C_1FA9_0919(1);
						CON_printf(C_2BB4_0045, GetObjectString(k));
						if(k == 1) {
							SetTypeUnconscious(0);
							SetTypeUnconscious(0x68);
							D_1431 = 1;
							DisplayTick();
							TimerDelay(0x122e4cL);
							ObjShapeType[0] = OrigShapeType[1];
							ObjShapeType[0x68] = OrigShapeType[0x68];
							C_1534_0906(D_1684, D_1685, MapZ, 0);
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
						if((k = C_1B09_1081(0, 0x6a, 0x2a)) == 0) {
							MoveObj(0, 0x6b, 0x2b, MapZ);
							MapX = 0x6b;
							MapY = 0x2b;
							C_1534_056F();
						}
						D_1431 = 1;
						TalkFlags[0x69] |= 1;
					break;
					case 0x69:
						C_1B09_3E18();
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

	if(C_1534_1129(GetX(objNum), MapX) < 5 && C_1534_1129(GetY(objNum), MapY) < 5) {
		x = C_1534_1101(GetX(objNum) - (MapX - 5));
		y = C_1534_1101(GetY(objNum) - (MapY - 5));
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
				for(npc = C_1B09_0BEE(Party[k], 0x1ca); npc >= 0; npc = C_1B09_0CA4()) {
					count ++;
					C_1FA9_0919(4);
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
			C_07D1_15F0(0x3c);
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
			for(k = C_1B09_0EDE(0x9a, 0xbb, 0xaa, 0xcb); k >= 0; k = C_1B09_0DD7()) {
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
					if(C_07D1_0416(npc, 0)) {
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
			C_1B09_1A1D(-1, -1, -1);
			if((k = C_1B09_12EE(0x9e, 0x10, 4, 0)) < 0) {
				CON_printf(C_2BB4_22F1);
				break;
			}
			if((k = C_1B09_12EE(0xf2, 0x10, 0, 0)) < 0) {
				CON_printf(C_2BB4_22F1);
				break;
			}
			if((k = C_1B09_12EE(0x4a0, 0x10, 0, 0)) < 0) {
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
			if((npc = C_1B09_12EE(0xfc, 8, 0, k)) < 0) {
				CON_printf(C_2BB4_22F1);
				break;
			}
			for(k = 0; k < 4; k ++) {
				if((npc = EGG_generate(0x56e, 0xa, 0x6f, MapZ)) >= 0) {
					NPCComMode[npc] = NPCMode[npc] = 0x18;
					if(!C_07D1_0416(npc, 1))
						DeleteObj(npc);
				} else {
					CON_printf(C_2BB4_22F1);
				}
			}
			for(k = 0; k < 4; k ++) {
				if((npc = EGG_generate(0x56e, 0x19, 0x60, MapZ)) >= 0) {
					NPCComMode[npc] = NPCMode[npc] = 0x18;
					if(!C_07D1_0416(npc, 1))
						DeleteObj(npc);
				} else {
					CON_printf(C_2BB4_22F1);
				}
			}
			for(k = 0; k < 4; k ++) {
				if((npc = EGG_generate(0x56e, 0x28, 0x6f, MapZ)) >= 0) {
					NPCComMode[npc] = NPCMode[npc] = 0x18;
					if(!C_07D1_0416(npc, 1))
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
			for(obj = C_1B09_0EDE(0x49, 0x30, 0x57, 0x51); obj >= 0; obj = C_1B09_0DD7()) {
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
			for(k = C_1B09_0EDE(8, 0x18, 0x16, 0x39); k >= 0; k = C_1B09_0DD7()) {
				if(GetType(k) == 0x155 && GetQual(k) == 0x5b)
					break;
			}
			if(k < 0) {
				CON_printf(C_2BB4_22F1);
				break;
			}
			C_1B09_1DF5(k);
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
			C_1FA9_0919(0x32);
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
			C_1534_0906(D_1684, D_1685, MapZ, 0);
		break;
		case 0x6b:
			C_1FA9_0919(4);
			C_1B09_1A1D(-1, -1, -1);
			C_1534_0906(0x6a, 0x2c, MapZ, 0);
			C_1B09_4963(0);
			D_1684 = 0x6a;
			D_1685 = 0x2c;
			D_16D4 = 0xe4;
		break;
		case 0x6c:
			if(!InCombat)
				C_07D1_01BB();
			D_16A3 = 6;
			C_1B09_1A1D(-1, -1, -1);
			D_13F0 = 1;
			C_1534_0906(0x6e, 0x17, MapZ, 0);
			D_13F0 = 0;
			if((k = OBJ_FindTypeAt(0xe0, 0x75, 0x17)) >= 0)
				break;
			if((k = AddMapObj(0xce0, 0, 0, 0x75, 0x17, MapZ)) < 0)
				CON_printf(C_2BB4_22F1);
			else
				ObjStatus[k] &= 0xdf;
		break;
		case 0x6d:
			if(!InCombat)
				C_07D1_01BB();
			D_16A3 = 6;
			D_13F0 = 1;
			C_1B09_1A1D(-1, -1, -1);
			C_1534_0906(0x7a, 0xd, MapZ, 0);
			D_13F0 = 0;
			if((k = OBJ_FindTypeAt(0xe0, 0x7a, 0x12)) >= 0)
				break;
			if((k = AddMapObj(0x1ce0, 0, 0, 0x7a, 0x12, MapZ)) < 0)
				CON_printf(C_2BB4_22F1);
			else
				ObjStatus[k] &= 0xdf;
		break;
		case 0x6e:
			if(!InCombat)
				C_07D1_01BB();
			D_13F0 = 1;
			C_1B09_1A1D(-1, -1, -1);
			C_1534_0906(0x82, 0x18, MapZ, 0);
			D_13F0 = 0;
		break;
		case 0x6f:
			HitPoints[0] = MaxHP(1);
		break;
		case 0xfe:
			D_1686 ++;
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
