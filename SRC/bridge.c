/* flags: -d -mm -p -c -Z -O -y */
/* stub order: 0000 0386 0237 */
/*
 * Martian Dreams - GAME.EXE overlay 2F58: the drawbridges over the canals,
 * new code with no Savage Empire counterpart.
 *
 * A drawbridge is worked by levers (type 0xc4, quality 10, quantity the
 * bridge's number) and spans the posts (type 0xc3 frame 0, quality the
 * bridge's number; the first post's quantity: bits 0-4 the bridge's
 * length, 0x80 set when it runs north-south). Using a lever (needs the
 * power, TalkFlags[0x73] & 0x10) finds the levers and posts, works out
 * the bridge's rectangle and extends the bridge (objects 0xc3 added row
 * by row) or retracts it (the 0xc3 objects with a frame deleted), and
 * turns the levers.
 */
#include "se.h"

/*other modules*/
extern void cdecl TimerDelay(int32);/*wait (2383 per millisecond)*/
extern DisplayTick();
extern int16 NextLoc();/*next object of FindLoc's search*/
extern int16 FindLoc(int16, int16, int16);/*first object at (x, y, z)?*/
extern int16 NextArea(void);
extern int16 SearchArea(int16, int16, int16, int16);/*SearchArea(x1, y1, x2, y2)*/
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);
extern DeleteObj(int16);
extern ShowObjects();
extern MUS_PlaySfx(int16);/*play a sound effect?*/
extern cdecl CON_printf(char far *, ...);

extern int16 MapZ;
extern unsigned char ObjStatus[];
extern int16 Amount[];/*quantity (low byte), quality (high byte)*/
extern unsigned char D_6EC6[];/*TalkFlags (per NPC)*/
extern struct coord ObjPos[];
extern uint16 ObjShapeType[];
extern int16 D_EC4C;/*BridgeNum: the bridge's number*/
extern int16 D_EC4E, D_EC50;/*BridgeX1, BridgeY1: the bridge's rectangle*/
extern int16 D_EC52;/*BridgeLen*/
extern int16 D_EC54, D_EC56;/*BridgeX2, BridgeY2*/
extern int16 D_EC58;/*BridgeNS: 0x80 when the bridge runs north-south*/

#define TalkFlags D_6EC6
#define BridgeNum D_EC4C
#define BridgeX1 D_EC4E
#define BridgeY1 D_EC50
#define BridgeLen D_EC52
#define BridgeX2 D_EC54
#define BridgeY2 D_EC56
#define BridgeNS D_EC58


#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)

/*a drawbridge's lever is used*/
/*C_2F58_0000*/Drawbridge(int16 objNum) {
	int16 y, extended, levers[2], numLevers, posts[5], numPosts;
	register int16 j, i;

	if(!(TalkFlags[0x73] & 0x10)) {
		CON_printf("There doesn't seem to be any power for the machinery.\n");
		MUS_PlaySfx(5);
		return;
	}
	BridgeNum = GetQuan(objNum);
	BridgeNS = 0;
	BridgeX1 = BridgeY1 = 0x3ff;
	BridgeX2 = BridgeY2 = 0;
	numLevers = numPosts = 0;
	for(i = 0x101; i < 0xc00; i++) {
		if(GetType(i) == 0xc4 && GetQual(i) == 10 && GetQuan(i) == BridgeNum && numLevers < 2) {
			levers[numLevers] = i;
			numLevers ++;
		} else if(GetQual(i) == BridgeNum && GetType(i) == 0xc3 && GetFrame(i) == 0 && numPosts < 5) {
			posts[numPosts] = i;
			numPosts ++;
			j = GetX(i);
			if(j < BridgeX1)
				BridgeX1 = j;
			if(j > BridgeX2)
				BridgeX2 = j;
			y = GetY(i);
			if(y < BridgeY1)
				BridgeY1 = y;
			if(y > BridgeY2)
				BridgeY2 = y;
		}
	}
	if(!numLevers || !numPosts) {
		CON_printf("Argh--drawbridge data inconsistent!\n");
		return;
	}
	BridgeLen = GetQuan(posts[0]);
	BridgeNS = BridgeLen & 0x80;
	BridgeLen &= 0x1f;
	if(BridgeNS) {
		BridgeY1 -= BridgeLen;
		BridgeY2 --;
	} else {
		BridgeX1 -= BridgeLen;
		BridgeX2 --;
	}
	extended = 0;
	for(i = SearchArea(BridgeX1, BridgeY1, BridgeX2, BridgeY2); i >= 0; i = NextArea()) {
		if(GetType(i) == 0xc3) {
			extended = 1;
		} else {
			CON_printf("The bridge doesn't move.\n");
			MUS_PlaySfx(5);
			return;
		}
	}
	for(j = 0; j < numLevers; j++)
		ObjShapeType[levers[j]] = GetType(levers[j]) + ((1 - extended) << 10);
	MUS_PlaySfx(0x12);
	if(extended)
		BridgeRetract();
	else
		BridgeExtend();
}

/*C_2F58_0237*/BridgeExtend() {
	int16 x, amount;
	register int16 y, objNum;

	amount = BridgeNum << 8;
	if(BridgeNS) {
		for(y = BridgeY2; y >= BridgeY1; y--) {
			for(x = BridgeX2; x >= BridgeX1; x--) {
				if((objNum = AddMapObj(0xc3, 0, amount, x, y, MapZ)) >= 0) {
					ObjStatus[objNum] &= ~0x20;
					ObjShapeType[objNum] = GetType(objNum) + ((x == BridgeX2 ? 3 : 1) << 10);
				}
			}
			ShowObjects();
			DisplayTick();
			TimerDelay(0x249f0L);
		}
	} else {
		for(x = BridgeX2; x >= BridgeX1; x--) {
			for(y = BridgeY2; y >= BridgeY1; y--) {
				if((objNum = AddMapObj(0xc3, 0, amount, x, y, MapZ)) >= 0) {
					ObjStatus[objNum] &= ~0x20;
					ObjShapeType[objNum] = GetType(objNum) + ((y == BridgeY2 ? 2 : 1) << 10);
				}
			}
			ShowObjects();
			DisplayTick();
			TimerDelay(0x249f0L);
		}
	}
	CON_printf("The drawbridge slides out to cross the canal.\n");
}

/*C_2F58_0386*/BridgeRetract() {
	int16 x;
	register int16 y, objNum;

	if(BridgeNS) {
		for(x = BridgeX1; x <= BridgeX2; x++) {
			for(objNum = FindLoc(x, BridgeY1, MapZ); objNum >= 0; objNum = NextLoc()) {
				if(GetType(objNum) == 0xc3 && GetFrame(objNum) != 0)
					DeleteObj(objNum);
			}
		}
		for(y = BridgeY1 + 1; y <= BridgeY2; y++) {
			ShowObjects();
			DisplayTick();
			TimerDelay(0x249f0L);
			for(x = BridgeX1; x <= BridgeX2; x++) {
				for(objNum = FindLoc(x, y, MapZ); objNum >= 0; objNum = NextLoc()) {
					if(GetType(objNum) == 0xc3 && GetFrame(objNum) != 0)
						DeleteObj(objNum);
				}
			}
		}
	} else {
		for(y = BridgeY1; y <= BridgeY2; y++) {
			for(objNum = FindLoc(BridgeX1, y, MapZ); objNum >= 0; objNum = NextLoc()) {
				if(GetType(objNum) == 0xc3 && GetFrame(objNum) != 0)
					DeleteObj(objNum);
			}
		}
		for(x = BridgeX1 + 1; x <= BridgeX2; x++) {
			ShowObjects();
			DisplayTick();
			TimerDelay(0x249f0L);
			for(y = BridgeY1; y <= BridgeY2; y++) {
				for(objNum = FindLoc(x, y, MapZ); objNum >= 0; objNum = NextLoc()) {
					if(GetType(objNum) == 0xc3 && GetFrame(objNum) != 0)
						DeleteObj(objNum);
				}
			}
		}
	}
	ShowObjects();
	DisplayTick();
	CON_printf("The drawbridge retracts into its slot.\n");
}
