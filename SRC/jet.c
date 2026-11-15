/* flags: -d -mm -p -c -Z -O -y */
/* stub order: 0000 01C2 */
/*
 * Martian Dreams - GAME.EXE overlay 2F80: "jetters" (object type 0x1d0,
 * jets of steam or gas?), new code with no Savage Empire counterpart.
 *
 * A jetter's quality: bits 0-3 its delay, bits 4-7 the length of the jet;
 * its quantity: bits 0-3 the countdown, bits 5-7 the direction (as
 * DirIncrX/DirIncrY). TriggerJet shows the jet (Missiles) along the
 * tiles in front of it and hurts what is hit (MissileHitList): NPCs and
 * C_100B_038E types. UpdateJet counts a jetter down (or, with no delay,
 * triggers it at random) while talk flag TalkFlags[0x73] & 4 is set.
 */
#include "se.h"

/*other modules*/
extern int16 LoseHP(int16, int16, int16);/*hurt (objNum, damage, ?)*/
extern int16 Missiles(int16, int16, int16, int16 *, int16 *, int16, int16, int16);/*show a missile/jet (x, y, length, xs, ys, ?, tile?, ?)*/
extern int16 C_100B_038E(int16);
extern FlushViewIfDirty();
extern MUS_PlaySfx(int16);/*play a sound effect?*/
extern cdecl CON_printf(char far *, ...);
extern int16 OSI_rand(int16, int16);/*OSI_rand(lo, hi)*/

extern char DirIncrX[8], DirIncrY[8];
extern int16 D_54BA[];/*Amount: quantity (low byte), quality (high byte)*/
extern unsigned char D_6EC6[];/*TalkFlags*/
extern struct coord D_78AE[];/*ObjPos*/
extern uint16 D_9CC4[];/*ObjShapeType*/
extern int16 D_B9D6[0x40];/*MissileHitList: what the last missile hit (-1 ends)*/
#define Amount D_54BA
#define TalkFlags D_6EC6
#define ObjPos D_78AE
#define ObjShapeType D_9CC4
#define MissileHitList D_B9D6

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)

/*fire the jetter objNum*/
/*C_2F80_0000*/TriggerJet(register int16 objNum) {
	int16 delay, countdown, len, dir;
	int16 x, y;
	int16 far *p;
	int16 xs[16];
	int16 ys[16];
	register int16 i;

	if(objNum == -1) {
		CON_printf("trigger_jet on a non-object!\r\n");
		return;
	}
	if(GetType(objNum) != 0x1d0) {
		CON_printf("trigger_jet on a non-jetter!\r\n");
		return;
	}
	delay = GetQual(objNum);
	len = delay >> 4;
	delay &= 0xf;
	countdown = GetQuan(objNum);
	dir = (countdown & 0xe0) >> 5;
	countdown &= 0xf;
	if(delay > 0 && countdown > 0) {
		CON_printf("Nothing comes out.\r\n");
		return;
	}
	if(delay > 0)
		GetQuan(objNum) = delay | (dir << 5);
	x = GetX(objNum);
	y = GetY(objNum);
	for(i = 0; i < len; i ++) {
		x = (x + DirIncrX[dir]) & 0x3ff;
		xs[i] = x;
		y = (y + DirIncrY[dir]) & 0x3ff;
		ys[i] = y;
	}
	FlushViewIfDirty();
	MUS_PlaySfx(0x19);
	Missiles(GetX(objNum), GetY(objNum), len, xs, ys, 8, 0x104, 6);
	for(p = MissileHitList; *p >= 0; p ++) {
		if((*p < 0x100 && *p >= 0) || C_100B_038E(GetType(*p)))
			LoseHP(*p, OSI_rand(10, 50), 0);
	}
}

/*count the jetter objNum down: fire it when its countdown is over; with
  no delay, fire it at random*/
/*C_2F80_01C2*/UpdateJet(register int16 objNum) {
	int16 counter;
	register int16 delay;

	if(!(TalkFlags[0x73] & 4))
		return;
	if(GetType(objNum) != 0x1d0) {
		CON_printf("update-jetting a non-jetter!\r\n");
		return;
	}
	delay = GetQual(objNum) & 0xf;
	counter = GetQuan(objNum) != 0;
	if(delay) {
		if(counter) {
			counter --;
			GetQuan(objNum) = (GetQuan(objNum) & 0xf0) | counter;
		} else {
			TriggerJet(objNum);
		}
	} else if(counter && OSI_rand(1, counter) == 1) {
		TriggerJet(objNum);
	}
}
