/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE overlay 2F6E: using a lever (type 0xc4 and
 * others), new code with no Savage Empire counterpart.
 *
 * UseLever, a "use" action (obj, on, to), goes by the lever's quality:
 * 1 and 2 work the objects 0xb3/0xe3 whose quality (bits 0-6) is the
 * lever's quantity, and turn the levers 0x137/0x138/0xc4 of quality <= 2
 * with the same quantity (LeverObjects); 10 is bridge.c's Drawbridge;
 * 20 the levers of the power station and the canal pumps (LeverMachinery,
 * by quantity 1-6); 21 the dream's levers (LeverDream: quantity 1 turns
 * the map view, 2 and 3 call overlay 2F5F).
 * Talk flags: TalkFlags[0x73] & 4 the underground power, & 0x10 the
 * surface power, TalkFlags[0x71] & 8 the power is not yet permanent,
 * TalkFlags[0x76] & 0x20 / & 0x40 the canal pumps, TalkFlags[0x54] & 0x40
 * and 2, TalkFlags[6] & 2.
 */
#include "se.h"

/*other modules*/
extern BargePumps(int16);
extern C_1B09_3E18();/*ShowObjects*/
extern C_1FA9_0919(int16);/*MUS_PlaySfx*/
extern C_1FA9_0AFD();/*five beeps (machinery)*/
extern C_2F52_0473();/*BeltStart*/
extern Drawbridge(int16);
extern C_2F5F_0000(int16, int16);
extern C_2F5F_02CE();
extern C_2F5F_0477();
extern C_2F5F_0AC1(int16);
extern C_2F72_0000();/*MachineryOn*/
extern C_2F72_0210();/*MachineryOff*/
extern C_2F72_039E();
extern C_2F75_042D();
extern C_2F75_0532();
extern C_2F75_07FA();
extern C_2F75_0951();
extern C_2F75_0A64();
extern C_2F83_0167(int16, int16, int16);/*TALK_talkTo*/
extern cdecl CON_printf(char far *, ...);
extern char far C_2BB4_0C5F[];
extern char far C_2BB4_0F21[];
extern char far C_2BB4_111C[];
extern char far C_2BB4_12C8[];
extern char far C_2BB4_12E9[];

extern unsigned char D_167A;/*Active*/
extern int16 D_1680;/*MapZ*/
extern int32 D_16AA;/*flags (0x10: the dream)*/
extern int16 D_16AE;/*rotation of the map view (eighths of a turn)?*/
extern int16 D_16D4;
extern unsigned char D_4EA4[];/*Party*/
extern int16 D_54BA[];/*Amount: quantity (low byte), quality (high byte)*/
extern unsigned char D_6EC6[];/*TalkFlags (per NPC)*/
extern struct coord D_78AE[];/*ObjPos*/
extern uint16 D_9CC4[];/*ObjShapeType*/

#define Active D_167A
#define MapZ D_1680
#define IN_DREAM (D_16AA & 0x10)
#define Party D_4EA4
#define Amount D_54BA
#define TalkFlags D_6EC6
#define ObjPos D_78AE
#define ObjShapeType D_9CC4

#define ShowObjects C_1B09_3E18
#define MUS_PlaySfx C_1FA9_0919
#define BeltStart C_2F52_0473
#define MachineryOn C_2F72_0000
#define MachineryOff C_2F72_0210
#define TALK_talkTo C_2F83_0167

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)

/*a lever is used ("use" action: the lever, the second and third objects)*/
/*C_2F6E_0000*/UseLever(register int16 objNum, int16 on, int16 to) {
	int16 qual;

	qual = GetQual(objNum);
	switch(qual) {
		case 1: case 2:
			LeverObjects(objNum);
			break;
		case 10:
			Drawbridge(objNum);
			break;
		case 20:
			LeverMachinery(objNum);
			break;
		case 21:
			LeverDream(objNum);
			break;
		default:
			CON_printf("What an odd lever!\n");
	}
}

/*levers of quality 1 and 2: the object 0xb3/0xe3 whose quality (bits 0-6)
  is the lever's quantity; quality 1 toggles its quality's bit 7, quality 2
  (needs the power) opens or closes it (frames 1/3 and 9/11 through 5/7);
  the levers 0x137/0x138/0xc4 with the same quantity turn (frame 0 or 1).
  In the dream, a 0x18e NPC (0xc0-0xff) at x >= 0x37, y <= 0x37 on this
  level starts conversation 0x54*/
/*C_2F6E_0076*/LeverObjects(int16 objNum) {
	int16 quan, n, levers[4], type, q, qual, frame, state;
	register int16 found, i;

	n = 0;
	found = -1;
	qual = GetQual(objNum);
	quan = GetQuan(objNum);
	for(i = 0x101; i < 0xc00; i ++) {
		if((type = GetType(i)) == 0xb3 || type == 0xe3) {
			if((GetQual(i) & 0x7f) == quan)
				found = i;
		} else if((type == 0x137 || type == 0x138 || type == 0xc4) && GetQual(i) <= 2 && GetQuan(i) == quan && n < 4) {
			levers[n ++] = i;
		}
	}
	if(found == -1) {
		CON_printf("Strangely, nothing happens.\n");
		return;
	}
	if(n == 0) {
		CON_printf("Strangely, it doesn't move.\n");
		return;
	}
	if(qual == 1) {
		if((q = GetQual(found)) & 0x80) {
			GetQual(found) = q & 0x7f;
			state = 1;
		} else {
			GetQual(found) = q | 0x80;
			state = 0;
		}
	} else if(qual == 2) {
		/*(type is the last object's of the loop)*/
		if((type == 0xe3 && !(TalkFlags[0x73] & 4)) || (type == 0xb3 && !(TalkFlags[0x73] & 0x10))) {
			CON_printf(C_2BB4_0C5F);
			return;
		}
		frame = GetFrame(found);
		ObjShapeType[found] = GetType(found) + (((frame & 2) + 5) << 10);
		C_1FA9_0AFD();
		ShowObjects();
		if(frame < 4) {
			ObjShapeType[found] = GetType(found) + (((frame & 2) + 9) << 10);
			state = 1;
		} else {
			ObjShapeType[found] = GetType(found) + (((frame & 2) + 1) << 10);
			state = 0;
			if(IN_DREAM && D_16D4 == 0x44) {
				for(i = 0xc0; i < 0x100; i ++) {
					if(GetType(i) == 0x18e) {
						if(!(TalkFlags[0x54] & 0x40) && GetZ(i) == MapZ && GetX(i) >= 0x37 && GetY(i) <= 0x37) {
							C_2F5F_0AC1(0x54);
							TalkFlags[0x54] |= 2;
							TALK_talkTo(0, 0x54, 0);
							C_2F5F_0477();
							return;
						}
						break;
					}
				}
			}
		}
		C_1FA9_0AFD();
	}
	for(i = 0; i < n; i ++)
		ObjShapeType[levers[i]] = GetType(levers[i]) + (state << 10);
	ShowObjects();
	MUS_PlaySfx(0x11);
}

/*levers of quality 20 (by quantity): 1 the underground power, 2 the
  surface power, 3 and 4 the canal pumps, 5 the conveyor belts or cutting
  the underground power, 6 object 6 (type 0x165 becomes 0x162). The lever
  turns (frame ^ 1)*/
/*C_2F6E_034B*/LeverMachinery(register int16 objNum) {
	register int16 quan;

	quan = GetQuan(objNum);
	switch(quan) {
		case 1:
			if(TalkFlags[0x73] & 4) {
				if(TalkFlags[0x71] & 8) {
					TalkFlags[0x73] &= ~4;
					TalkFlags[0x71] &= ~8;
					MachineryOff();
					TalkFlags[0x73] |= 4;
					MachineryOn();
					C_2F75_042D();
				} else {
					CON_printf("Sorry, boss, the power is on for good.\n");
				}
			} else {
				TalkFlags[0x73] |= 4;
				MachineryOn();
				C_2F75_042D();
			}
			break;
		case 2:
			if(!(TalkFlags[0x73] & 4) || TalkFlags[0x71] & 8) {
				CON_printf("You can't turn the surface power on until the underground");
				CON_printf("power is permanently on.\n");
				return;
			}
			if(TalkFlags[0x73] & 0x10) {
				CON_printf("Sorry, boss.  The surface power can't be turned off.\n");
				return;
			}
			TalkFlags[0x73] |= 0x10;
			C_2F72_039E();
			C_2F75_0532();
			break;
		case 3:
			if(TalkFlags[0x76] & 0x20) {
				CON_printf(C_2BB4_12E9);
				break;
			}
			TalkFlags[0x2e] |= 0x80;
			TalkFlags[0x39] |= 0x80;
			TalkFlags[0x30] |= 0x80;
			TalkFlags[0x76] |= 0x20;
			BargePumps(1);
			C_2F75_07FA();
			break;
		case 4:
			if(TalkFlags[0x76] & 0x40) {
				CON_printf(C_2BB4_12C8);
				break;
			}
			if(!(TalkFlags[0x76] & 0x20)) {
				CON_printf(C_2BB4_111C);
				break;
			}
			TalkFlags[0x76] |= 0x40;
			BargePumps(0);
			C_2F75_0951();
			break;
		case 5:
			if(TalkFlags[0x73] & 4) {
				if(TalkFlags[0x71] & 8) {
					CON_printf("Power prematurely cut off.\n");
					TalkFlags[0x73] &= ~4;
					TalkFlags[0x71] &= ~8;
					MachineryOff();
					break;
				} else {
					CON_printf("Power is already permanently on.\n");
					return;
				}
			}
			BeltStart();
			break;
		case 6:
			if(GetType(6) != 0x165) {
				CON_printf(C_2BB4_0F21);
				break;
			}
			ObjShapeType[6] = (ObjShapeType[6] & 0xfc00) + 0x162;
			TalkFlags[6] |= 2;
			C_2F75_0A64();
			break;
		default:
			CON_printf("The lever doesn't do anything!\n");
			return;
	}
	ObjShapeType[objNum] = GetType(objNum) + ((GetFrame(objNum) ^ 1) << 10);
	MUS_PlaySfx(0x11);
}

/*the dream's levers (quality 21; by quantity): 1 turns the map view
  (D_16AE 4) or back, 2 and 3 call overlay 2F5F*/
/*C_2F6E_0556*/LeverDream(register int16 objNum) {
	register int16 quan;

	switch(quan = GetQuan(objNum)) {
		case 1:
			if(D_16AE) {
				CON_printf("The world returns to normal.\n");
				D_16AE = 0;
				ObjShapeType[objNum] = GetType(objNum);
			} else {
				CON_printf("You feel a strange disorientation.\n");
				D_16AE = 4;
				ObjShapeType[objNum] = GetType(objNum) + 0x400;
			}
			break;
		case 2:
			C_2F5F_0000(Party[Active], 0);
			return;
		case 3:
			C_2F5F_02CE();
			return;
		default:
			CON_printf("Strangely, the lever does nothing at all!\n");
			return;
	}
	MUS_PlaySfx(0x11);
}
