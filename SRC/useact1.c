/* flags: -d -mm -p -c -Z -O -y */
/* stub order: 0476 06C1 05DE 09C9 0B57 0849 1983 1145 133F 0DFE 1929 04E4 0895 1621 0A67 1B29 1545 0CFC 1F92 14BC 0B9D 1C20 0000 01C1 0EDB 0F1C */
/*
 * Martian Dreams - GAME.EXE overlay 2F8D: actions of the "use" command (the
 * first of three overlays of them, with 2F98 and 2FA1). New code: Savage
 * Empire's CmdUse tests types one by one in objcmd.c.
 *
 * Each action is called by UseObject (use.c) through the UseActions table
 * (far 2E67) as f(obj, on, to): the used object, the second object ("On -")
 * and the third ("To -"), -1 when not asked. The action numbers below are
 * the table's rows. Bed: sleep (REST_Command); books and the affidavit:
 * read (MDTEXT.LZC); readying; switches and panels fastened/loosened;
 * the drill on its cart; fixing rails and the power cable; crates; the
 * robot's heartstone; opening a pod; dirt and holes: digging, filling,
 * burying, loading wheelbarrows and rail cars (0x102 and 0x1ba-0x1bc are
 * dirt, 0x101/0xff holes; DirtLoadKind/LoadKindDirt map a dirt type to the load's
 * kind and back); the guns' setting; radium for weapons, the power
 * station and the mechanical man.
 */
#include "se.h"

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;/*1 wrap, 2 center, 4 capitalize, 8 runes, 0x10 paginate, 0x20 scroll*/
};

extern void far *LZC_loadType(char far *name, int16 index, void far *out, int16 memflags, char far *work);
extern void cdecl TimerDelay(int32);/*wait (2383 per millisecond)*/
extern int16 cdecl toupper(int16);

/*other modules*/
extern int16 AskHowMany(int16);
extern void SearchObj(int16);
extern int16 MaxHP(int16);
extern COMBAT_showHit(int16);
extern int16 LoseHP(int16, int16, int16);
extern ClearMapWindow(int16);
extern VIS_UpdateView();
extern DrawMapWindow(int16);
extern DisplayTick();
extern PassTime(int16);
extern unsigned char GetTileAtXYZ(int16, int16, int16);
extern int16 NextLoc();
extern int16 FindLoc(int16, int16, int16);
extern int16 OBJ_MapObjAt(int16, int16);
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
extern STAT_DrawInventory();
extern STAT_Ready();
extern int16 GetDigClass(int16);
extern int16 CON_prompt();
extern CON_WaitKeyOrClick();
extern CON_gotoxy(int16, int16);
extern CON_setClip(struct tClipInfo *);
extern cdecl CON_printf(char far *, ...);
extern int16 OSI_rand(int16, int16);
extern BeltStart();
extern DreamWakeUp();
extern DreamReleaseNpc(int16);
extern LampsAtNight();
extern Cutscene1();
extern Cutscene3();
extern REST_Command(int16, int16);
extern TALK_talkTo(int16, int16, int16);
extern int16 IsNpcOnScreen(int16);

extern char far PressAnyKeyMsg[];/*"\nPress any key..."*/
extern char far NoSleepInDreamMsg[];/*"You can't sleep in a dream.\n"*/
extern char far CantReadMsg[];/*"You can't read it.\n"*/
extern char far TooMuchMsg[];/*"You are carrying too much already.\n"*/
extern char far FullyChargedMsg[];/*"The %s is fully charged.\n"*/
extern char far NoEffectMsg[];/*"It has no effect.\n"*/
extern char far NotInSoloMsg[];/*"Not while in solo mode!\n"*/
extern char far NotPossibleMsg[];/*"That is not possible.\n"*/
extern char far AffidavitMsg[];/*"--Affidavit--\n\n"*/
extern char far AffidavitText1Msg[];/*"The undersigned do hereby vouch for the "*/
extern char far AffidavitText2Msg[];/*"trust-\nworthiness of $P, who bears "*/
extern char far AffidavitText3Msg[];/*"this chit, commending same to the members "*/
extern char far AffidavitText4Msg[];/*"of the Olympus Mons outpost."*/
extern char far SignedMsg[];/*"\n\n*Signed:\n"*/
extern char far ShermanMsg[];/*" Richard Sherman\n"*/
extern char far DupreyMsg[];/*" Capt. Gregory Duprey\n"*/
extern char far YellinMsg[];/*" Doctor David Yellin\n"*/
extern char far DrillPowerMsg[];/*"The drill's power is connected.\n"*/
extern char far SwitchUnfixableMsg[];/*"This switch cannot be fixed.\n"*/
extern char far SwitchFastenedMsg[];/*"The switch is fastened.\n"*/
extern char far SwitchLooseMsg[];/*"The switch is loose.\n"*/
extern char far RepairFirstMsg[];/*"It must be repaired first.\n"*/
extern char far PanelFastenedMsg[];/*"The panel is fastened.\n"*/
extern char far PanelLooseMsg[];/*"The panel is loose.\n"*/
extern char far PanelsOnCabinetsMsg[];/*"Panels are only installed onto cabinets.\n"*/
extern char far WrongPanelMsg[];/*"This cabinet requires a different type of panel.\n"*/
extern char far NothingToUnloadMsg[];/*"There is nothing to unload.\n"*/
extern char far NoMoreRoomMsg[];/*"There is no more room.\n"*/
extern char far IntoWheelbarrowMsg[];/*"You put the %s into the wheelbarrow.\n"*/
extern char far CantMixLoadMsg[];/*"You can't mix different things in the same load!\n"*/
extern char far IntoRailCarMsg[];/*"You put the %s into the rail car.\n"*/
extern char far CantBuryHereMsg[];/*"You can't bury anything here.\n"*/
extern char far PlantedSeedMsg[];/*"You planted the Martian seed.\n"*/
extern char far FilledHoleMsg[];/*"You filled in the hole.\n"*/
extern char far CantDigHereMsg[];/*"You can't dig here.\n"*/
extern char far UprootedPodMsg[];/*"You uprooted the pod plant.\n"*/
extern char far MartianFallsOutMsg[];/*"An unconscious Martian falls out of the pod.\n"*/
extern char far PriedNailsMsg[];/*"You pried the nails out of the crate.\n"*/
extern char far NotFullyGrownMsg[];/*"It is not fully grown.\n"*/
extern char far NewSettingMsg[];/*"  New setting (R%/S%/C)?\n"*/
extern char far WhichRailMsg[];/*"Which section of rail needs fixing -\n"*/
extern char far RadiumAlreadyMsg[];/*"Radium has already been installed.\n"*/
extern char far VeryHotMsg[];/*"OUCH. It is very hot.\n"*/
extern char far RadiumBlocksMsg[];/*"The %s only needed %d radium blocks.\n"*/
extern char far RadiumInstalledMsg[];/*"The radium has been installed.\n"*/
extern char far DontBuryPeopleMsg[];/*"`Don't bury people on Mars!\n"*/
extern char far TooPreciseMsg[];/*"The work is too precise to perform telekinetically.\n"*/
extern char far NoMoreDirtMsg[];/*"You can't pile any more dirt there.\n"*/
extern char far GroundNotFlatMsg[];/*"The ground is not flat enough.\n"*/
extern char far InTheWayMsg[];/*"The %s is in the way.\n"*/
extern char far DrillOnCartMsg[];/*"The drill must be installed onto a drill cart.\n"*/
extern char far NotStrongEnoughMsg[];/*"%s is not strong enough.\n"*/
extern char far HasHeartstoneAlreadyMsg[];/*"The robot already has a heartstone.\n"*/
extern char far HeartstoneInstalledMsg[];/*"The heartstone is installed.\n"*/
extern char far PodAlreadyOpenMsg[];/*"The pod is already open.\n"*/
extern char far CantDigDeeperMsg[];/*"You can't dig any deeper.\n"*/
extern char far StuckMsg[];/*"It is stuck.\n"*/
extern char far PanelOperatesDreamMachineMsg[];/*"The control panel operates the dream machine.\n"*/
extern char far NeedPliersMsg[];/*"You need to use pliers to attach the cable.\n"*/
extern char far ChewInstinctivelyMsg[];/*"You will chew it instinctively.\n"*/
extern char far GroundNotClearMsg[];/*"The ground is not clear for digging.\n"*/
extern char far BuryInHoleMsg[];/*"You must bury the %s in a hole.\n"*/
extern char far CoveredWithDirtMsg[];/*"It is covered with dirt.\n"*/
extern char far TheyCoveredWithDirtMsg[];/*"They have been covered with dirt.\n"*/
extern char far PlateExposedMsg[];/*"The plate in the camera has already been exposed.\n"*/
extern char far NeedShovelMsg[];/*"You must use a shovel to move that.\n"*/
extern char far CantReadTimeMsg[];/*"You can't determine how to read the time.\n"*/
extern char far NailCrateMsg[];/*"You nail the crate shut.\n"*/
extern char far CantBuryWithItselfMsg[];/*"You can't bury something using itself!\n"*/
extern char far CableFineMsg[];/*"The cable does not need replacement.\n"*/
extern char far BerriesNotEdibleMsg[];/*"The berries are not edible.\n"*/
extern char far DontKnowHowMsg[];/*"You don't know how it works.\n"*/
extern char far AarghMsg[];/*"Aargh!\n"*/
extern signed char DirIncrX[8], DirIncrY[8];
extern int16 ScreenFade;
extern unsigned char StatusDirty;
extern int16 MapAnimEnabled;
extern unsigned char Active;
extern int16 MapX, MapY, MapZ;
extern char SoloMember;
extern int32 GameFlags;/*0x10: in a dream*/
extern struct { unsigned a:4, psychic:4, c:4, d:4; } Talents[];
extern char SelectMode;
extern char AnimateAtPrompt;
extern int16 EndPagingAtPrompt;
extern int16 MouseMode;
extern int16 StatusDisplay;
extern int16 PromptCh;
extern struct tClipInfo *CurClip;
extern unsigned char ObjStatus[];
extern char far *LzcWorkBuf;/*work area of the .lzc loader*/
extern unsigned char Party[];
extern unsigned char far *Level;
extern char Names[][14];
extern int16 Amount[];/*quantity (low byte), quality (high byte)*/
extern unsigned char NPCStatus[];
extern int16 far *BaseTile;
extern char far *TalkBuf;
extern unsigned char TalkFlags[];
extern unsigned char far *TileFlag2;
extern struct coord ObjPos[];
extern unsigned char PartySize;
extern uint16 ObjShapeType[];
extern unsigned char far *TerrainType;
extern int16 Link[];
extern int16 InvenOwner;
extern int16 SelectionObj;
extern struct tClipInfo TextWindows[4];
extern int16 SelectionX, SelectionY;
extern char *VarStr[10+26];/*the conversations' string variables ($0-$9, $A-$Z)*/
extern int16 LinesBeforeMore;
extern int16 TargetDist;/*distance of the last target (use.c)*/
/*F0CA*/extern int16 DirtFailed;/*set when a dirt action failed (the load is not used up)*/

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)
#define GetQual(o) (((unsigned char *)(Amount + (o)))[1])
#define SetQual(o, v) (GetQual(o) = (v))
#define GetQuan(o) (((unsigned char *)(Amount + (o)))[0])
#define SetQuan(o, v) (GetQuan(o) = (v))

/*28F0*/char MdTextName[] = "mdtext.lzc";

/*
 * action 0 (bed 0x6a): sleep, unless in solo mode or in the dream; the bed's squares
 * (or the party's) must be free of objects and of tiles with TerrainType 2
 */
/*C_2F8D_0000*/UseBed(int16 obj, int16 on, int16 to) {
	int16 x, y, i, j, onMap;
	register int16 o, found;

	found = 0;
	if(SoloMember != -1) {
		CON_printf(NotInSoloMsg);
		MUS_PlaySfx(5);
		return;
	}
	if(Active == PartySize) {
		CON_printf(NoSleepInDreamMsg);
		MUS_PlaySfx(5);
		return;
	}
	if(ObjStatus[obj] & 0x18) {
		onMap = 0;
		x = MapX;
		y = MapY;
	} else {
		onMap = 1;
		x = GetX(obj);
		y = GetY(obj);
	}
	MouseMode = 0;
	ShowObjects();
	DisplayTick();
	if((o = SearchArea((x - 1) & 0x3ff, (y - 2) & 0x3ff, (x + 1) & 0x3ff, y)) != -1) do {
		if((NPCStatus[o] & 0x80) || (TileFlag2[BaseTile[GetType(o)] + GetFrame(o)] & 0x10) || GetType(o) == 0x6a)
			o = NextArea();
		else
			found = 1;
	} while(!found && o != -1);
	if(!found) {
		for(i = -1; i < 1; i++) {
			for(j = -2; j <= 0; j++) {
				if(TerrainType[GetTileAtXYZ((x + i) & 0x3ff, (y + j) & 0x3ff, MapZ)] & 2) {
					found = 1;
					break;
				}
			}
			if(found)
				break;
		}
	}
	if(found) {
		if(o == -1)
			CON_printf(GroundNotFlatMsg);
		else
			CON_printf(InTheWayMsg, GetObjectString(o));
		MUS_PlaySfx(5);
	} else
		REST_Command(x, y);
}

/*
 * action 1 (0x94, 0x97, 0xf3): read; quality 0 is the affidavit (signed by
 * those whose TalkFlags 8 is set), else text quality - 1 of MDTEXT.LZC in text window 2
 */
/*C_2F8D_01C1*/UseRead(int16 obj, int16 on, int16 to) {
	struct tClipInfo *clip;
	int16 lines, book, done;
	char line[90];
	register int16 n, pos;

	ClearMapWindow(1);
	lines = LinesBeforeMore;
	clip = CurClip;
	CON_setClip(&TextWindows[2]);
	CurClip->attrFlags |= 0x20;
	CurClip->attrFlags |= 0x10;
	CON_gotoxy(0, 0);
	LinesBeforeMore = CurClip->rows;
	if((book = GetQual(obj)) == 0) {
		VarStr['P' - 0x37] = Names[0];/*$P*/
		MapAnimEnabled = 0;
		CON_printf(AffidavitMsg);
		CON_printf(AffidavitText1Msg);
		CON_printf(AffidavitText2Msg);
		CON_printf(AffidavitText3Msg);
		CON_printf(AffidavitText4Msg);
		CON_printf(SignedMsg);
		if(TalkFlags[5] & 8)
			CON_printf(ShermanMsg);
		else
			CON_printf("\n");
		if(TalkFlags[0x19] & 8)
			CON_printf(DupreyMsg);
		else
			CON_printf("\n");
		if(TalkFlags[0x1a] & 8)
			CON_printf(YellinMsg);
		else
			CON_printf("\n");
		CON_printf("\n*");
		MapAnimEnabled = 1;
	} else if(LZC_loadType(MdTextName, book - 1, TalkBuf, 0, LzcWorkBuf) != 0) {
		done = 0;
		pos = 0;
		while(!done) {
			n = 0;
			do {
				line[n] = TalkBuf[pos];
				if(TalkBuf[pos] == 0) {
					done = 1;
				} else if(line[n] == '\n' || n == 0x4f) {
					line[n + 1] = 0;
					MapAnimEnabled = 0;
					CON_printf("%s", line);
					MapAnimEnabled = 1;
					n = 0;
				} else
					n++;
				pos++;
			} while(!done);
		}
	} else
		CON_printf(CantReadMsg);
	CON_printf(PressAnyKeyMsg);
	SelectMode = 0;
	PromptCh = 1;
	MapAnimEnabled = 0;
	CON_WaitKeyOrClick();
	MapAnimEnabled = 1;
	PromptCh = 5;
	CON_setClip(clip);
	EndPagingAtPrompt = 1;
	LinesBeforeMore = lines;
	AnimateAtPrompt = 1;
	VIS_UpdateView();
	ScreenFade = 1;
	DrawMapWindow(0);
	CON_printf("\n");
}

/*
 * the load kind of a dirt type (0x102: 1, 0x1ba: 2, 0x1bb: 3, 0x1bc: 4)
 */
/*C_2F8D_03FC*/static unsigned char DirtLoadKind(int16 type) {
	switch(type) {
	case 0x102:
		return 1;
	case 0x1ba:
		return 2;
	case 0x1bb:
		return 3;
	case 0x1bc:
		return 4;
	default:
		return 1;
	}
}

/*
 * the dirt type of a load kind (the reverse of DirtLoadKind)
 */
/*C_2F8D_043F*/static LoadKindDirt(unsigned char n) {
	switch(n) {
	case 1:
		return 0x102;
	case 2:
		return 0x1ba;
	case 3:
		return 0x1bb;
	case 4:
		return 0x1bc;
	default:
		return 0x102;
	}
}

/*
 * action 2 (0x191, 0x26, 0x88): take into the inventory and ready
 */
/*C_2F8D_0476*/UseTakeAndReady(register int16 obj, int16 on, int16 to) {
	if((ObjStatus[obj] & 0x18) == 0 && InsertObj(obj, Party[Active], 0x10) == -1) {
		CON_printf(TooMuchMsg);
		MUS_PlaySfx(5);
		return;
	}
	if((ObjStatus[obj] & 0x18) != 0x18) {
		SelectionObj = obj;
		STAT_Ready();
	}
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*
 * action 4 (0x87 on 0x19b): fasten (quality 1) or loosen a switch; fastening
 * takes the frame of the 0x19d up-left of it
 */
/*C_2F8D_04E4*/UseFastenSwitch(int16 obj, register int16 sw, int16 to) {
	int16 x, y;
	register int16 o;

	if(GetQual(sw) == 1) {
		x = (GetX(sw) - 1) & 0x3ff;
		y = (GetY(sw) - 1) & 0x3ff;
		o = -1;
		if((o = OBJ_FindTypeAt(0x19d, x, y)) == -1) {
			CON_printf(SwitchUnfixableMsg);
			MUS_PlaySfx(5);
		} else {
			ObjShapeType[sw] = GetType(sw) + (GetFrame(o) << 10);
			SetQual(sw, 0);
			MUS_PlaySfx(0x1f);
			CON_printf(SwitchFastenedMsg);
		}
	} else {
		SetQual(sw, 1);
		MUS_PlaySfx(0x1f);
		CON_printf(SwitchLooseMsg);
	}
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*
 * action 5 (0x87 on drill 0x1b8): install the drill on a drill cart 0x1b7 (makes 0x1b9)
 */
/*C_2F8D_05DE*/UseInstallDrill(int16 obj, int16 drill, int16 to) {
	int16 cart;
	register int16 n;

	if((cart = OBJ_FindTypeAt(0x1b7, GetX(drill), GetY(drill))) == -1)
		CON_printf(DrillOnCartMsg);
	else if((n = AddMapObj(0x1b9, 0, 1, GetX(drill), GetY(drill), MapZ)) == -1)
		CON_printf(AarghMsg);
	else {
		ObjStatus[n] &= 0xdf;
		ObjShapeType[n] = GetType(n) + 0x400;
		DeleteObj(drill);
		DeleteObj(cart);
		MUS_PlaySfx(0x1f);
		CON_printf(DrillPowerMsg);
	}
}

/*
 * action 6 (0x87 on panel 0x1ca): fasten a panel onto a cabinet 0x1c9 (quality:
 * the panel kind it takes) or loosen it; the fourth sets TalkFlags[0x74] 8
 */
/*C_2F8D_06C1*/UseFastenPanel(int16 obj, register int16 panel, int16 to) {
	int16 q, cab;
	register int16 n;

	if(ObjStatus[panel] & 0x18) {
		CON_printf(NoEffectMsg);
		return;
	}
	q = GetQual(panel);
	n = GetQuan(panel);
	if(q != 0) {
		if(q & 2) {
			CON_printf(RepairFirstMsg);
			MUS_PlaySfx(5);
		} else if((cab = OBJ_FindTypeAt(0x1c9, GetX(panel), GetY(panel))) == -1) {
			CON_printf(PanelsOnCabinetsMsg);
			MUS_PlaySfx(5);
		} else {
			if((q = GetQual(cab)) == 0) {
				if(n != 0) {
					CON_printf(WrongPanelMsg);
					MUS_PlaySfx(5);
					return;
				}
			} else if(q <= 3 && n == 0) {
				if(GetFrame(panel) != q - 1) {
					CON_printf(WrongPanelMsg);
					MUS_PlaySfx(5);
					return;
				}
			} else if(q != n) {
				CON_printf(WrongPanelMsg);
				MUS_PlaySfx(5);
				return;
			}
			q = GetQual(panel);
			SetQual(panel, q - 1);
			MUS_PlaySfx(0x1f);
			CON_printf(PanelFastenedMsg);
			if(q == 4) {
				TalkFlags[0x74] |= 8;
				if(TalkFlags[0x74] & 1)
					TalkFlags[0x60] |= 4;
			}
		}
	} else {
		SetQual(panel, q | 1);
		MUS_PlaySfx(0x1f);
		CON_printf(PanelLooseMsg);
	}
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*
 * action 7 (0x34 on 0x194 to 0x195): fix a section of rail
 */
/*C_2F8D_0849*/UseFixRail(int16 obj, int16 on, int16 to) {
	MUS_PlaySfx(0x20);
	CON_printf(WhichRailMsg);
	DeleteObj(on);
	ObjShapeType[to] = (ObjShapeType[to] & 0xfc00) + 0x19e;
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*
 * action 8 (0x35 on cable 0xc7 to 0xc9/0xd7): replace the power cable (only the
 * avatar, wearing rubber gloves 0x26, else a shock)
 */
/*C_2F8D_0895*/UseReplaceCable(int16 obj, int16 cable, int16 to) {
	int16 solo;
	register int16 npc, o;

	if(TargetDist > 1) {
		CON_printf(TooPreciseMsg);
		return;
	}
	if(!(TalkFlags[0x73] & 4) || (TalkFlags[0x71] & 8)) {
		CON_printf(CableFineMsg);
		return;
	}
	if((npc = Party[Active]) != 1) {
		solo = SoloMember;
		SoloMember = -1;
		TalkFlags[2] |= 0x10;
		TALK_talkTo(1, npc, 0);
		TalkFlags[2] &= 0xef;
		SoloMember = solo;
		return;
	}
	for(o = FindInvType(npc, 0x26, -1); o != -1 && (ObjStatus[o] & 0x18) != 0x18; o = NextInvType())
		;
	if(o == -1) {
		COMBAT_showHit(npc);
		LoseHP(npc, OSI_rand(0, MaxHP(npc) >> 1), 0);
		if(IsNpcOnScreen(2)) {
			TalkFlags[2] |= 4;
			TALK_talkTo(npc, 2, 0);
			TalkFlags[2] &= 4;
		}
	} else {
		MUS_PlaySfx(0x1f);
		DeleteObj(cable);
		if(StatusDisplay == 0x92)
			STAT_DrawInventory();
		TalkFlags[0x73] |= 0x10;
		LampsAtNight();
		Cutscene3();
	}
}

/*
 * actions 17, 22 (crowbars, hammer on crates): pry a crate open, nail it shut (0x111)
 */
/*C_2F8D_09C9*/UseOnCrate(int16 obj, register int16 crate, int16 to) {
	if(GetFrame(crate) == 2) {
		ObjShapeType[crate] = GetType(crate) + 0x400;
		MUS_PlaySfx(0x1f);
		CON_printf(PriedNailsMsg);
	} else if(GetFrame(crate) == 1 && GetType(obj) == 0x111) {
		ObjShapeType[crate] = GetType(crate) + 0x800;
		MUS_PlaySfx(0x20);
		CON_printf(NailCrateMsg);
	} else
		CON_printf(NoEffectMsg);
}

/*
 * action 18 (chisel 0x110, pry bar 0x66 on the hatch 0x1ab): only the avatar;
 * once TalkFlags[0x10] has 0x10 the hatch opens (cut scene 1, NPC 0x12
 * talks, TalkFlags[0x10] 0x20), else NPC 0x10 talks
 */
/*C_2F8D_0A67*/UseOpenHatch(int16 obj, register int16 o, int16 to) {
	register int16 npc;

	if((npc = Party[Active]) != 1) {
		CON_printf(NotStrongEnoughMsg, GetObjectString(npc));
		return;
	}
	if(!(TalkFlags[0x10] & 0x10)) {
		TalkFlags[0x10] |= 4;
		TALK_talkTo(1, 0x10, 0);
		return;
	}
	Cutscene1();
	TalkFlags[0x12] |= 0x48;
	TALK_talkTo(1, 0x12, 0);
	ObjShapeType[o] = (ObjShapeType[o] & 0xfc00) + 0x1ac;
	TalkFlags[0x10] |= 0x20;
	ObjShapeType[o] = GetType(o);
	MoveObj(o, (GetX(o) + 1) & 0x3ff, (GetY(o) + 1) & 0x3ff, MapZ);
	PassTime(1);
}

/*
 * action 65 (heartstone 0x1aa on robot 0x11f)
 */
/*C_2F8D_0B57*/UseInstallHeartstone(int16 obj, int16 robot, int16 to) {
	if(GetQual(robot) & 1)
		CON_printf(HasHeartstoneAlreadyMsg);
	else {
		SetQual(robot, 1);
		MUS_PlaySfx(0x1f);
		DeleteObj(obj);
		CON_printf(HeartstoneInstalledMsg);
	}
}

/*
 * action 20 (on pod 0xfb): open a ripe pod (frame 3); in the dream NPC 0x56 comes out
 */
/*C_2F8D_0B9D*/UseOpenPod(int16 obj, register int16 pod, int16 to) {
	register int16 o;

	if(GetFrame(pod) == 3) {
		ObjShapeType[pod] = GetType(pod) + 0x1000;
		if(GameFlags & 0x10) {
			MoveObj(0x56, GetX(pod), GetY(pod), MapZ);
			ShowObjects();
			MapAnimEnabled = 1;
			DisplayTick();
			TimerDelay(0x3a2dcL);
			TalkFlags[0x56] |= 0x88;
			TALK_talkTo(1, 0x56, 0);
			DreamReleaseNpc(0x56);
			DreamWakeUp();
		} else {
			o = AddMapObj(0x123, 0, 1, GetX(pod), GetY(pod), MapZ);
			CON_printf(MartianFallsOutMsg);
			ObjStatus[o] &= 0xdf;
			ObjStatus[pod] |= 0x20;
			TalkFlags[0x61] |= 8;
		}
		VIS_UpdateView();
		MapAnimEnabled = 1;
		DisplayTick();
	} else if(GetFrame(pod) < 3)
		CON_printf(NotFullyGrownMsg);
	else
		CON_printf(PodAlreadyOpenMsg);
}

/*
 * action 21 (on plant 0x82): pick berries 0x83
 */
/*C_2F8D_0CFC*/UsePickBerries(int16 obj, register int16 o, int16 to) {
	register int16 n;

	if(GetFrame(o) == 0) {
		MUS_PlaySfx(0x20);
		ObjShapeType[o] = GetType(o) + 0x400;
		if((ObjStatus[o] & 0x18) == 0)
			n = AddMapObj(0x83, 0, OSI_rand(1, 0x32) + OSI_rand(1, 0x32), GetX(o), GetY(o), GetZ(o));
		else
			n = AddInvObj(0x83, ObjStatus[o] & 0x18, OSI_rand(1, 0x32) + OSI_rand(1, 0x32), *(int16 *)&ObjPos[o]);
	} else
		CON_printf(NoEffectMsg);
}

/*
 * actions 9, 64: the messages of tools used on the wrong things
 */
/*C_2F8D_0DFE*/UseWrongTool(int16 obj, int16 on, int16 to) {
	switch(GetType(obj)) {
	case 0x41: case 0x42: case 0x43: case 0x107: case 0x10b: case 0x147:
		CON_printf(CantDigDeeperMsg);
		break;
	case 0x1ab:
		CON_printf(StuckMsg);
		break;
	case 0x120:
		CON_printf(PanelOperatesDreamMachineMsg);
		break;
	case 0xc7:
		CON_printf(NeedPliersMsg);
		break;
	case 0x83:
		CON_printf(ChewInstinctivelyMsg);
		break;
	case 0xb8:
		CON_printf(PlateExposedMsg);
		break;
	case 0x1ba: case 0x1bb: case 0x1bc:
		CON_printf(NeedShovelMsg);
		break;
	case 0x125:
		CON_printf(CantReadTimeMsg);
		break;
	case 0x4d: case 0x4e:
		CON_printf(BerriesNotEdibleMsg);
		break;
	case 0x143:
		CON_printf(DontKnowHowMsg);
		break;
	default:
		CON_printf(NoEffectMsg);
		break;
	}
}

/*
 * search (empty) the hole 0x101 at (x, y)
 */
/*C_2F8D_0EDB*/SearchHole(int16 x, int16 y) {
	register int16 o;

	for(o = OBJ_FindTypeAt(0x101, x, y); o != -1 && GetType(o) != 0x101; o = NextLoc())
		;
	if(o != -1)
		SearchObj(o);
}

/*
 * action 10 (shovels on pod plant 0xfb): uproot it (dirt 0x102 next to the party)
 */
/*C_2F8D_0F1C*/UseUprootPlant(int16 obj, int16 plant, int16 to) {
	int16 npc, hole, pile, x, y;
	register int16 d, o;

	hole = -1;
	npc = Party[Active];
	for(d = 0; d < 8; d++) {
		x = (GetX(npc) + DirIncrX[d]) & 0x3ff;
		y = (GetY(npc) + DirIncrY[d]) & 0x3ff;
		if(OBJ_MapObjAt(x, y) == -1 && !(TerrainType[GetTileAtXYZ(x, y, MapZ)] & 2))
			break;
	}
	if(d == 8 || (hole = AddMapObj(0x102, 0, 1, (GetX(npc) + DirIncrX[d]) & 0x3ff, (GetY(npc) + DirIncrY[d]) & 0x3ff, MapZ)) == -1) {
		CON_printf(CantDigHereMsg);
		return;
	}
	ObjStatus[hole] &= 0xdf;
	pile = -1;
	if((pile = OBJ_FindTypeAt(0x101, GetX(plant), GetY(plant))) == -1) {
		CON_printf(NoEffectMsg);
		return;
	}
	if((o = FindInv(pile)) != -1 && GetType(o) == 0xfc) {
		if(GetFrame(o) == 0)
			ObjShapeType[o] = GetType(o) + 0x400;
		MoveObj(o, GetX(plant), GetY(plant), MapZ);
	}
	while((o = NextInv()) != -1)
		DeleteObj(o);
	if(GetFrame(plant) > 0)
		CON_printf(UprootedPodMsg);
	MUS_PlaySfx(0x1b);
	DeleteObj(plant);
}

/*
 * action 11 (shovels on a square): dig a hole, the dirt next to the party
 */
/*C_2F8D_1145*/UseDigHole(int16 obj, int16 on, int16 to) {
	int16 cls, shape, pile, npc, x, y;
	register int16 d, hole;

	if(on != -1 && !(TileFlag2[BaseTile[GetType(on)] + GetFrame(on)] & 0x10)) {
		CON_printf(GroundNotClearMsg);
		return;
	}
	npc = Party[Active];
	if((cls = GetDigClass(GetTileAtXYZ(SelectionX, SelectionY, MapZ))) != 0) {
		if(cls == 0x102)
			shape = 0x101;
		else if(cls == 0x100)
			shape = 0xff;
		else
			shape = 0x102;
		if((hole = AddMapObj(shape, 0, 1, SelectionX, SelectionY, MapZ)) >= 0)
			ObjStatus[hole] &= 0xdf;
		pile = -1;
		for(d = 0; d < 8; d++) {
			x = (GetX(npc) + DirIncrX[d]) & 0x3ff;
			y = (GetY(npc) + DirIncrY[d]) & 0x3ff;
			if(OBJ_MapObjAt(x, y) == -1 && !(TerrainType[GetTileAtXYZ(x, y, MapZ)] & 2))
				break;
		}
		if(d == 8 || (pile = AddMapObj(cls, 0, 1, (GetX(npc) + DirIncrX[d]) & 0x3ff, (GetY(npc) + DirIncrY[d]) & 0x3ff, MapZ)) == -1) {
			CON_printf(CantDigHereMsg);
			DeleteObj(hole);
			hole = -1;
		} else {
			MUS_PlaySfx(0x1b);
			ObjStatus[pile] &= 0xdf;
		}
	} else
		CON_printf(NoEffectMsg);
}

/*
 * action 13 (shovels: dirt into wheelbarrow 0x10c or rail car 0x19a)
 */
/*C_2F8D_133F*/UseLoadDirt(int16 obj, int16 on, register int16 cart) {
	int16 ct, n;
	register int16 t;

	t = GetType(on);
	ct = GetType(cart);
	DirtFailed = 0;
	if(ct == 0x10c) {
		if(GetQuan(cart) == 1) {
			CON_printf(NoMoreRoomMsg);
			MUS_PlaySfx(5);
			DirtFailed = 1;
		} else {
			MUS_PlaySfx(0x1b);
			SetQuan(cart, 1);
			SetQual(cart, DirtLoadKind(t));
			CON_printf(IntoWheelbarrowMsg, GetObjectString(on));
			DeleteObj(on);
		}
	} else if(ct == 0x19a) {
		if((n = GetQuan(cart)) == 7) {
			CON_printf(NoMoreRoomMsg);
			MUS_PlaySfx(5);
			DirtFailed = 1;
		} else if(n != 0 && DirtLoadKind(t) != GetQual(cart)) {
			CON_printf(CantMixLoadMsg);
			MUS_PlaySfx(5);
			DirtFailed = 1;
		} else {
			n++;
			SetQual(cart, DirtLoadKind(t));
			if(n == 7)
				ObjShapeType[cart] = GetType(cart) + (((GetFrame(cart) & 1) + 4) << 10);
			else
				ObjShapeType[cart] = GetType(cart) + (((GetFrame(cart) & 1) + 2) << 10);
			SetQuan(cart, n);
			MUS_PlaySfx(0x1b);
			CON_printf(IntoRailCarMsg, GetObjectString(on));
			DeleteObj(on);
		}
	}
}

/*
 * action 62 (shovels: dirt into a hole 0x101): fill the hole
 */
/*C_2F8D_14BC*/UseFillHole(int16 obj, int16 hole, int16 pile) {
	register int16 o;

	DirtFailed = 0;
	if((o = FindInv(pile)) >= 0) {
		SearchObj(pile);
		DirtFailed = 1;
		return;
	}
	SearchHole(GetX(hole), GetY(hole));
	DeleteObj(hole);
	DeleteObj(pile);
	MUS_PlaySfx(0x1b);
	MUS_PlaySfx(0x1b);
	CON_printf(FilledHoleMsg);
}

/*
 * action 12 (shovels on dirt 0x1ba-0x1bc, to a square): move the dirt there
 */
/*C_2F8D_1545*/UseMoveDirt(int16 obj, register int16 dirt, int16 to) {
	int16 x, y, tx;
	register int16 ty;

	tx = SelectionX;
	ty = SelectionY;
	x = GetX(dirt);
	y = GetY(dirt);
	DirtFailed = 0;
	if(TerrainType[GetTileAtXYZ(tx, ty, MapZ)] & 2) {
		CON_printf(NotPossibleMsg);
		MUS_PlaySfx(5);
		DirtFailed = -1;
	} else if(OBJ_MapObjAt(tx, ty) != -1) {
		MUS_PlaySfx(5);
		DirtFailed = -1;
	} else {
		MUS_PlaySfx(0x1b);
		MoveObj(dirt, tx, ty, MapZ);
		if(GetFrame(dirt) != 0)
			ObjShapeType[dirt] = GetType(dirt);
		SearchHole(x, y);
	}
}

/*
 * action 15 (shovels: dirt onto objects): bury them in the hole under them (a seed
 * 0xfc makes a pod plant 0xfb)
 */
/*C_2F8D_1621*/UseBury(int16 obj, register int16 dirt, int16 target) {
	int16 hole, seed, fail, shape, t, count, x, y, tx, ty;
	register int16 o;

	seed = -1;
	fail = 0;
	x = GetX(dirt);
	y = GetY(dirt);
	shape = 0x102;
	if(target == -1) {
		UseMoveDirt(obj, dirt, target);
		return;
	}
	if(target < 0x100 && target >= 0) {
		MUS_PlaySfx(5);
		CON_printf(DontBuryPeopleMsg);
		DirtFailed = 1;
		return;
	}
	if(obj == target) {
		MUS_PlaySfx(5);
		CON_printf(CantBuryWithItselfMsg);
		DirtFailed = 1;
		return;
	}
	tx = GetX(target);
	ty = GetY(target);
	if((o = OBJ_FindTypeAt(0x102, tx, ty)) == -1)
		o = OBJ_FindTypeAt(0xfb, tx, ty);
	if(o != -1) {
		MUS_PlaySfx(5);
		CON_printf(NoMoreDirtMsg);
		DirtFailed = 1;
		return;
	}
	if((hole = OBJ_FindTypeAt(0x101, tx, ty)) == -1) {
		CON_printf(BuryInHoleMsg, GetObjectString(target));
		DirtFailed = 1;
		return;
	}
	shape = 0x102;
	count = 0;
	MUS_PlaySfx(0x1b);
	for(o = FindLoc(tx, ty, MapZ); o != -1 && GetType(o) != 0x101; o = NextLoc()) {
		count++;
		if(o < 0x100 && o >= 0) {
			CON_printf(DontBuryPeopleMsg);
			fail = 1;
			break;
		}
		if((t = GetType(o)) == 0xfc) {
			if(seed != -1) {
				CON_printf(CantBuryHereMsg);
				fail = 1;
				break;
			}
			seed = o;
			shape = 0xfb;
		}
	}
	if(fail) {
		MUS_PlaySfx(5);
		DirtFailed = 1;
		return;
	}
	for(o = FindLoc(tx, ty, MapZ); o != -1 && GetType(o) != 0x101; o = NextLoc()) {
		if(!(TileFlag2[BaseTile[GetType(o)] + GetFrame(o)] & 0x10) && InsertObj(o, hole, 8) == -1) {
			CON_printf(AarghMsg);
			SearchObj(hole);
			DirtFailed = 1;
			return;
		}
	}
	MoveObj(dirt, tx, ty, MapZ);
	if(GetFrame(dirt) != 0)
		ObjShapeType[dirt] = GetType(dirt);
	ObjStatus[dirt] &= 0xdf;
	MUS_PlaySfx(0x1b);
	if(shape == 0xfb) {
		ObjShapeType[dirt] = (ObjShapeType[dirt] & 0xfc00) + 0xfb;
		CON_printf(PlantedSeedMsg);
	} else if(count == 1)
		CON_printf(CoveredWithDirtMsg);
	else
		CON_printf(TheyCoveredWithDirtMsg);
	SearchHole(x, y);
}

/*
 * action 16 (shovels: dirt 0x1bc onto the coal belt 0xe9)
 */
/*C_2F8D_1929*/UseDirtOnBelt(int16 obj, int16 dirt, int16 to) {
	register int16 f, t;

	t = GetType(dirt);
	f = GetFrame(to);
	MUS_PlaySfx(0x1b);
	DeleteObj(dirt);
	if(t != 0x1bc)
		CON_printf(NoEffectMsg);
	else if(f < 4)
		BeltStart();
}

/*
 * action 14 (shovels on wheelbarrow or rail car): unload one load
 */
/*C_2F8D_1983*/UseUnloadCart(int16 obj, register int16 cart, int16 target) {
	int16 shape, tt, n;
	register int16 dirt;

	DirtFailed = 0;
	if(GetQuan(cart) == 0) {
		CON_printf(NothingToUnloadMsg);
		MUS_PlaySfx(5);
		return;
	}
	shape = LoadKindDirt(GetQual(cart));
	if((dirt = AddMapObj(shape, 0, 1, GetX(cart), GetY(cart), MapZ)) == -1) {
		CON_printf(AarghMsg);
		MUS_PlaySfx(5);
		return;
	}
	ObjStatus[dirt] &= 0xdf;
	if(target == -1)
		UseMoveDirt(obj, dirt, target);
	else {
		switch(tt = GetType(target)) {
		case 0x10c: case 0x19a:
			UseLoadDirt(obj, dirt, target);
			break;
		case 0xe9:
			UseDirtOnBelt(obj, dirt, target);
			break;
		case 0x101:
			UseFillHole(obj, dirt, target);
			break;
		default:
			UseBury(obj, dirt, target);
			break;
		}
	}
	if(DirtFailed) {
		DeleteObj(dirt);
		return;
	}
	n = GetQuan(cart) - 1;
	SetQuan(cart, n);
	if(GetType(cart) == 0x19a) {
		if(n == 0)
			ObjShapeType[cart] = GetType(cart) + ((GetFrame(cart) & 1) << 10);
		else
			ObjShapeType[cart] = GetType(cart) + (((GetFrame(cart) & 1) + 2) << 10);
	}
}

/*
 * action 40 (guns 0x2d, 0xf0, 0xf1): show and set the setting (quality)
 */
/*C_2F8D_1B29*/UseGunSetting(register int16 gun, int16 on, int16 to) {
	int16 q;
	register int16 ch;

	CON_printf("It is set to \"%s\".", (q = GetQual(gun)) != 0 ? (q > 1 ? "Shotgun" : "Rifle") : "Combination");
	if(GetType(gun) != 0x2d)
		CON_printf("  It has %d charges left.", GetQuan(gun));
	CON_printf(NewSettingMsg);
	do
		ch = toupper(CON_prompt());
	while(ch != 'S' && ch != 'R' && ch != 'C' && ch != '\n' && ch != '\r');
	switch(ch) {
	case 'S':
		CON_printf("Shotgun\n");
		SetQual(gun, 2);
		break;
	case 'R':
		CON_printf("Rifle\n");
		SetQual(gun, 1);
		break;
	case 'C':
		CON_printf("Combination\n");
		SetQual(gun, 0);
		break;
	}
}

/*
 * actions 47, 61 (radium 0x1c1 on guns, 0x1c0 on 0x122/0x1c0): charge a weapon,
 * install radium (TalkFlags[0x74] 4); hurts without gloves 0x88 or a talent
 */
/*C_2F8D_1C20*/UseRadium(register int16 rad, register int16 on, int16 to) {
	int16 ot, charge, npc, n, installed, owner, glove;

	npc = Party[Active];
	ot = GetType(on);
	if(ObjStatus[rad] & 0x18)
		StatusDirty = 1;
	if(on == rad && ot == 0x1c0) {
		if((ObjStatus[on] & 0x18) == 0) {
			owner = Link[on];
			if((ObjStatus[owner] & 0x18) || GetX(owner) != GetX(on) || GetY(owner) != GetY(on) || GetZ(owner) != MapZ) {
				CON_printf(NoEffectMsg);
				return;
			}
			on = owner;
			ot = GetType(on);
		} else {
			CON_printf(NoEffectMsg);
			return;
		}
	}
	for(glove = FindInvType(npc, 0x88, -1); glove != -1 && (ObjStatus[glove] & 0x18) != 0x18; glove = NextInvType())
		;
	if(!(Talents[Party[Active]].a ? Level[Party[Active]] : 0) && glove == -1 && npc != 6) {
		COMBAT_showHit(Party[Active]);
		NPCStatus[Party[Active]] |= 8;
		CON_printf(VeryHotMsg);
	}
	CON_printf("\n");
	if(GetType(rad) == 0x1c0) {
		if(ot == 0x122) {
			if(installed == 1)
				CON_printf(RadiumAlreadyMsg);
			else {
				DeleteObj(rad);
				MUS_PlaySfx(4);
				CON_printf(RadiumInstalledMsg);
				ObjShapeType[on] = GetType(on) + 0x400;
				TalkFlags[0x74] |= 4;
				if(TalkFlags[0x74] & 1)
					TalkFlags[0x60] |= 4;
			}
		} else
			CON_printf(NoEffectMsg);
	} else if(GetType(rad) == 0x1c1) {
		if((charge = GetQuan(on)) > 0xd2)
			CON_printf(FullyChargedMsg, GetObjectString(on));
		else {
			n = 0;
			if((n = AskHowMany(rad)) == 0)
				return;
			if(charge + n * 0x1e > 0xf0) {
				n = (0xf0 - charge) / 0x1e;
				CON_printf(RadiumBlocksMsg, GetObjectString(on), n);
				CON_printf(FullyChargedMsg, GetObjectString(on));
			}
			charge += n * 0x1e;
			SetQuan(on, charge);
			if((ObjStatus[rad] & 0x18) == 0)
				MapAnimEnabled = 1;
			else if(StatusDisplay == 0x92 && *(int16 *)&ObjPos[rad] == InvenOwner)
				StatusDirty = 1;
			if(GetQuan(rad) == n)
				DeleteObj(rad);
			else
				SetQuan(rad, GetQuan(rad) - n);
		}
	} else
		CON_printf(NoEffectMsg);
}

/*
 * action 73 (radium on the mechanical man 0x166, NPC 0x75)
 */
/*C_2F8D_1F92*/UseRadiumOnMechMan(register int16 rad, register int16 man, int16 to) {
	int16 n;

	if(ObjStatus[rad] & 0x18)
		StatusDirty = 1;
	if(man != 0x75) {
		CON_printf("This mechanical man already has radium.\n");
		return;
	}
	if((n = GetQuan(rad)) == 1)
		DeleteObj(rad);
	else
		SetQuan(rad, n - 1);
	ObjShapeType[man] = GetType(man) + 0x2400;
	TalkFlags[0x75] |= 8;
}
