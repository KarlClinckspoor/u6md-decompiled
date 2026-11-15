/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE overlay 2F55: SPELL_ReviveNPC, alone in its
 * overlay (Savage Empire's spell.c C_11FF_00B0, by way of Ultima VI).
 *
 * MD's changes: the coordinates tried around the body wrap (& 0x3ff),
 * OrigShapeType is a far pointer, and a revived party member also calls
 * MoveFollowers(1, 0).
 */
#include "se.h"

/*other modules*/
extern int16 MaxHP(int16);
extern int16 CanMoveTo(int16, int16, int16);/*CanMoveTo(npc, x, y)*/
extern int16 MoveFollowers(int16, int16);
extern int16 DeleteObj(int16);/*DeleteObj?*/
extern int16 OBJ_ReviveFromBody(int16);/*the NPC*/
extern MoveObj(int16, int16, int16, int16);/*MoveObj(obj, x, y, z)*/
extern int16 JoinParty(int16);

extern int16 MapZ;
extern int16 Amount[];/*quantity (low byte), quality (high byte)*/
extern unsigned char NPCStatus[];
extern unsigned char HitPoints[];
extern struct coord ObjPos[];
extern uint16 ObjShapeType[];
extern int16 far *OrigShapeType;

#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])

/*brings the NPC of a dead body back to life on a free spot within 4
  tiles of it (or where it is); returns the NPC, or -1 for no body or
  a body of quality >= 0x10 (not an NPC's: it is deleted)*/
/*C_2F55_0000*/SPELL_ReviveNPC(int16 corpse) {
	int16 npc, radius;
	int16 placed, step, new_x, new_y;

	placed = 0;
	if(corpse < 0)
		return -1;
	if(GetQual(corpse) >= 0x10) {
		DeleteObj(corpse);
		return -1;
	}
	npc = OBJ_ReviveFromBody(corpse);
	NPCStatus[npc] &= ~0x10;
	HitPoints[npc] = MaxHP(npc);
	NPCStatus[npc] &= ~0x08;
	ObjShapeType[npc] = OrigShapeType[npc];
	NPCStatus[npc] = (NPCStatus[npc] & 0x9f) | 0x40;
	for(radius = 1; radius < 5; radius ++) {
		for(step = 0; step < (radius << 1) + 1; step ++) {
			new_x = (GetX(npc) - radius + step) & 0x3ff;
			new_y = (GetY(npc) - radius) & 0x3ff;
			if(CanMoveTo(npc, new_x, new_y)) {
				MoveObj(npc, new_x, new_y, MapZ);
				placed = 1;
				break;
			}
			new_x = (GetX(npc) - radius) & 0x3ff;
			new_y = (GetY(npc) - radius + step) & 0x3ff;
			if(CanMoveTo(npc, new_x, new_y)) {
				MoveObj(npc, new_x, new_y, MapZ);
				placed = 1;
				break;
			}
			new_x = (GetX(npc) + radius - step) & 0x3ff;
			new_y = (GetY(npc) + radius) & 0x3ff;
			if(CanMoveTo(npc, new_x, new_y)) {
				MoveObj(npc, new_x, new_y, MapZ);
				placed = 1;
				break;
			}
			new_x = (GetX(npc) + radius) & 0x3ff;
			new_y = (GetY(npc) + radius - step) & 0x3ff;
			if(CanMoveTo(npc, new_x, new_y)) {
				MoveObj(npc, new_x, new_y, MapZ);
				placed = 1;
				break;
			}
		}
		if(placed)
			break;
	}
	if(!placed)
		MoveObj(npc, GetX(npc), GetY(npc), MapZ);
	if(NPCStatus[npc] & 0x80) {
		JoinParty(npc);
		MoveFollowers(1, 0);
	}
	return npc;
}
