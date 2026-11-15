/* flags: -d -mm -p -c -Z -O -y */
/* stub order: 0210 039E 0000 */
/*
 * Martian Dreams - GAME.EXE overlay 2F72: the machinery's power going on
 * and off, and the lamps lit at night; new code with no Savage Empire
 * counterpart.
 *
 * MachineryOn starts the animated tiles of the BaseTile ranges of types
 * 0xbc, 0xbf, 0x1b0 and 0x1b3 (StateAnimData) and turns the objects of
 * the map (0x101-0xbff) on: type 0x9a to frame 1, 0xc9 frame | 4, 0xd6
 * becomes 0xd7, 0xe9 frames 1/3 become 5/7, and a 0xd0 gets an object
 * 0xd1 (frame 1) south of it. MachineryOff undoes it (0xd1 objects are
 * deleted).
 */
#include "se.h"

/*other modules*/
extern AnimateTiles();
extern int16 OBJ_FindTypeAt(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);
extern DeleteObj(int16);

extern int16 VisibilityDirty;
extern unsigned char Time_H;
extern int16 MapZ;
extern unsigned char ObjStatus[];
extern int16 far *BaseTile;
extern struct coord ObjPos[];
extern int16 NumAnimData;
extern uint16 ObjShapeType[];
extern int16 AnimTile[];/*the animated tiles*/
extern unsigned char StateAnimData[];


#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
/*is animated tile i one of the machinery's?*/
#define IS_MACHINE_TILE(i) (\
	(AnimTile[i] >= BaseTile[0xbc] && AnimTile[i] < BaseTile[0xbd]) ||\
	(AnimTile[i] >= BaseTile[0xbf] && AnimTile[i] < BaseTile[0xc0]) ||\
	(AnimTile[i] >= BaseTile[0x1b0] && AnimTile[i] < BaseTile[0x1b1]) ||\
	(AnimTile[i] >= BaseTile[0x1b3] && AnimTile[i] < BaseTile[0x1b4]))

/*the power goes on*/
/*C_2F72_0000*/MachineryOn() {
	int16 frame;
	register int16 objNum, i;

	for(i = 0; i < NumAnimData; i ++) {
		if(IS_MACHINE_TILE(i))
			StateAnimData[i] = 1;
	}
	for(i = 0x101; i < 0xc00; i ++) {
		switch(GetType(i)) {
			case 0x9a:
				ObjShapeType[i] = GetType(i) + 0x400;
			break;
			case 0xc9:
				ObjShapeType[i] = GetType(i) + ((GetFrame(i) | 4) << 10);
			break;
			case 0xd6:
				ObjShapeType[i] = (ObjShapeType[i] & 0xfc00) + 0xd7;
			break;
			case 0xd0:
				if(OBJ_FindTypeAt(0xd1, GetX(i), (GetY(i) + 1) & 0x3ff) < 0) {
					objNum = AddMapObj(0x4d1, 0, 0, GetX(i), (GetY(i) + 1) & 0x3ff, MapZ);
					ObjStatus[objNum] &= ~0x20;
				}
			break;
			case 0xe9:
				frame = GetFrame(i);
				if(frame == 1 || frame == 3)
					ObjShapeType[i] = GetType(i) + ((frame + 4) << 10);
			break;
		}
	}
	VisibilityDirty = 1;
}

/*the power goes off*/
/*C_2F72_0210*/MachineryOff() {
	register int16 frame, i;

	for(i = 0; i < NumAnimData; i ++) {
		if(IS_MACHINE_TILE(i))
			StateAnimData[i] = 0;
	}
	for(i = 0x101; i < 0xc00; i ++) {
		switch(GetType(i)) {
			case 0x9a:
				ObjShapeType[i] = GetType(i);
			break;
			case 0xc9:
				ObjShapeType[i] = GetType(i) + ((GetFrame(i) & 0xfb) << 10);
			break;
			case 0xd7:
				ObjShapeType[i] = (ObjShapeType[i] & 0xfc00) + 0xd6;
			break;
			case 0xe9:
				frame = GetFrame(i);
				if(frame == 5 || frame == 7)
					ObjShapeType[i] = GetType(i) + ((frame - 4) << 10);
			break;
			case 0xd1:
				DeleteObj(i);
			break;
		}
	}
	VisibilityDirty = 1;
}

/*after AnimateTiles: the lamps (type 0xe4) go to frame 7 at night
  (Time_H < 6 or > 17)*/
/*C_2F72_039E*/C_2F72_039E() {
	int16 night;
	register int16 type, i;

	AnimateTiles();
	night = Time_H < 6 || Time_H > 17;
	for(i = 0x101; i < 0xc00; i ++) {
		if((type = GetType(i)) == 0xe4 && night)
			ObjShapeType[i] = GetType(i) + 0x1c00;
	}
}
