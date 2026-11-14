/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE overlay 2F55: SPELL_ReviveNPC, alone in its
 * overlay (Savage Empire's spell.c C_11FF_00B0, by way of Ultima VI).
 *
 * MD's changes: the coordinates tried around the body wrap (& 0x3ff),
 * OrigShapeType is a far pointer, and a revived party member also calls
 * C_170D_0976(1, 0).
 */
#include "se.h"

/*other modules*/
extern int16 C_07D1_064E(int16);/*MaxHP*/
extern int16 C_104E_0007(int16, int16, int16);/*CanMoveTo(npc, x, y)*/
extern int16 C_170D_0976(int16, int16);
extern int16 C_1B09_1872(int16);/*DeleteObj?*/
extern int16 C_1B09_1DF5(int16);/*OBJ_ReviveFromBody: the NPC*/
extern C_1B09_1F52(int16, int16, int16, int16);/*MoveObj(obj, x, y, z)*/
extern int16 C_2F83_0A85(int16);/*JoinParty*/
#define MaxHP C_07D1_064E
#define CanMoveTo C_104E_0007
#define DeleteObj C_1B09_1872
#define OBJ_ReviveFromBody C_1B09_1DF5
#define MoveObj C_1B09_1F52
#define JoinParty C_2F83_0A85

extern int16 D_1680;/*MapZ*/
extern int16 D_54BA[];/*Amount: quantity (low byte), quality (high byte)*/
extern unsigned char D_6CBA[];/*NPCStatus*/
extern unsigned char D_6DBE[];/*HitPoints*/
extern struct coord D_78AE[];/*ObjPos*/
extern uint16 D_9CC4[];/*ObjShapeType*/
extern int16 far *D_B508;/*OrigShapeType*/
#define MapZ D_1680
#define Amount D_54BA
#define NPCStatus D_6CBA
#define HitPoints D_6DBE
#define ObjShapeType D_9CC4
#define OrigShapeType D_B508

#define GetX(i) ((*(int16 *)&(D_78AE[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(D_78AE[i].raw[1]))>>2)&0x3ff)
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
		C_170D_0976(1, 0);
	}
	return npc;
}
