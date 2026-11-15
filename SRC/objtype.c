/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE segment 100B: the object type tables and the
 * functions that look a type up in them. New as a module in MD: Savage
 * Empire has these functions spread over combat.c (GetDamagePoints,
 * __GetWeaponRange, GetArmorPoints), objcmd.c (IsContainerType), object.c
 * (QuanType) and status.c (STAT_GetEquipSlot, IsWeightPer10Type); the
 * others are new. Each table is defined just before its function.
 *
 * MD's changes to SE's functions: GetDamagePoints and GetWeaponRange have
 * no special case for type 0x15 quality 1; STAT_GetEquipSlot tests ranges
 * of types (1-5 head, 6-8 and 0xea neck, 9-0xf feet, 0x11-0x17 and 0x5a
 * chest, 0x19-0x1e left finger, 0x21-0x27 right finger) before its three
 * lists (two-handed, right hand, left hand), searched from the end.
 */
#include "se.h"

extern int16 GetMonsterClass(int16);/*GetMonsterClass(type): < 0 if none*/

extern unsigned char far C_2DE5_012E[];/*MonsterDamage (by GetMonsterClass)*/
#define MonsterDamage C_2DE5_012E

extern uint16 D_9CC4[];/*ObjShapeType*/
#define ObjShapeType D_9CC4

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define IsNPC(i) ((i) < 0x100 && (i) >= 0)

/*0BB2*/int16 WeaponTypes[] = {
	0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x36,0x37,0x38,0x41,
	0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x191,0xf0,0xf1,0x66,0x81,0x105,0x139,0x28,0x10
};
/*0BF2*/unsigned char WeaponDamages[] = {
	15,18,20,30,30,45,8,12,15,20,10,25,10,20,25,15,
	8,10,10,15,12,20,10,12,20,15,12,60,60,254,1,30
};

/*maximum damage of a weapon, or of an actor's natural attack (from its
  monster class); -1 if the object is not a weapon*/
/*C_100B_000E*/GetDamagePoints(int16 objNum) {
	int16 i, damage;
	int16 objCls;

	damage = -1;
	if(IsNPC(objNum))
		objCls = GetMonsterClass(GetType(objNum));
	else
		objCls = -1;
	if(objCls >= 0) {
		damage = MonsterDamage[objCls];
	} else {
		for(i = 0; i < 32; i ++)
			if(GetType(objNum) == WeaponTypes[i]) {
				damage = WeaponDamages[i];
				break;
			}
	}
	return damage;
}

/*0C12*/int16 RangedWeaponTypes[] = {
	0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0xf0,0xf1,0x81,0x105,0x139,0x28,0x16e,0x182,
	0x16c,0x180
};
/*0C36*/unsigned char WeaponRanges[] = {4,5,4,9,9,9,4,5,6,6,2,2,9,6,3,4,3,2};

/*range of a weapon (or of an actor's natural attack): 1 by default*/
/*C_100B_0086*/GetWeaponRange(int16 objNum) {
	int16 i, range;
	int16 objType;

	range = 1;
	objType = GetType(objNum);
	for(i = 0; i < 18; i ++) {
		if(RangedWeaponTypes[i] == objType) {
			range = WeaponRanges[i];
			break;
		}
	}
	return range;
}

/*0C48*/int16 Types0C48[] = {0x3,0x4,0xb,0xe,0xd,0x15,0x16,0x17,0x1b,0x1e,0x22,0x24,0x26,0x27};
/*0C64*/unsigned char Values0C64[] = {1,2,1,3,2,1,2,2,1,3,1,2,1,3};

/*a value of an object by its type, 0 if not in Types0C48 (SE's
  GetArmorPoints; clothes, but not armour: see C_100B_015C)*/
/*C_100B_00C4*/C_100B_00C4(int16 objNum) {
	int16 i, points;

	points = 0;
	for(i = 0; i < 14; i ++) {
		if(Types0C48[i] == GetType(objNum)) {
			points = Values0C64[i];
			break;
		}
	}
	return points;
}

/*0C72*/int16 Types0C72[] = {
	0x29,0x2a,0x2c,0x2d,0x2e,0x2f,0x30,0x28,0xf0,0xf1,0x81,0x105,0x139,0x16e,0x182,0x16c,
	0x180
};
/*0C94*/int16 Tiles0C94[] = {
	0x103,0x103,0x103,0x103,0x103,0x23e,0x23f,0x23f,0x16b,0x16a,0x10a,0x10a,0x10b,0x16f,0x16c,0x16d,
	0x16e
};
/*0CB6*/int16 Values0CB6[] = {7,7,8,8,8,6,6,6,0xa,0xa,6,6,0x32,0x22,0x22,6,6};
/*0CD8*/int16 Values0CD8[] = {0x39,0x39,0x3b,0x3b,0x3c,0x3f,0x40,-3,-2,-2,-2,-2,-3,-3,-3,-3,-3};

/*the missile of a ranged weapon type? fills *tile, *b and *c from the
  tables (0, 0, -1 if objType is not in Types0C72)*/
/*C_100B_00FF*/C_100B_00FF(int16 objType, int16 *tile, int16 *b, register int16 *c) {
	register int16 i;

	for(i = 0; i < 17; i ++) {
		if(Types0C72[i] == objType) {
			*tile = Tiles0C94[i];
			*b = Values0CB6[i];
			*c = Values0CD8[i];
			return;
		}
	}
	*tile = 0;
	*b = 0;
	*c = -1;
}

/*0CFA*/int16 Types0CFA[] = {
	0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xe,0xd,0x11,0x12,0x13,
	0x14,0x15,0x16,0x17,0x5a,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x21,0x22,0x23,0x24,0x25,
	0x26,0x27
};
/*0D3E*/unsigned char Values0D3E[] = {
	1,2,3,3,2,1,1,2,1,1,2,4,3,2,3,3,
	4,4,3,5,12,2,2,2,3,3,3,1,1,1,2,3,
	1,3
};

/*a value of an object type that can be worn (armour points?), 0 if not
  in Types0CFA*/
/*C_100B_015C*/C_100B_015C(int16 objType) {
	int16 i;

	for(i = 33; i >= 0; i --)
		if(Types0CFA[i] == objType)
			return Values0D3E[i];
	return 0;
}

/*container types*/
/*0D60*/int16 ContainerTypes[] = {
	0x50,0x51,0x52,0x53,0x55,0x56,0x57,0x59,0x130,0x8b,0x155,0x101,0x68,0x11c,0x11d
};

/*is typeObject a container (one of ContainerTypes)? returns 1/0*/
/*C_100B_0186*/IsContainerType(int16 typeObject) {
	int16 i;

	for(i = 0; i < 15; i ++)
		if(ContainerTypes[i] == typeObject)
			return 1;
	return 0;
}

/*0D7E*/int16 Types0D7E[] = {0x133};

/*is objType 0x133? returns 1/0*/
/*C_100B_01AD*/C_100B_01AD(int16 objType) {
	int16 i;

	for(i = 0; i < 1; i ++)
		if(Types0D7E[i] == objType)
			return 1;
	return 0;
}

/*the unlit light sources? (0x6d is an unlit torch; 0x6e is lit)*/
/*0D80*/int16 Types0D80[] = {0x6d,0x6f,0x71,0x73,0x75};

/*is objType one of Types0D80? returns 1/0*/
/*C_100B_01D4*/C_100B_01D4(int16 objType) {
	int16 i;

	for(i = 0; i < 5; i ++)
		if(Types0D80[i] == objType)
			return 1;
	return 0;
}

/*the lit light sources? (0x6e is a lit torch)*/
/*0D8A*/int16 Types0D8A[] = {0x6e,0x70,0x72,0x74,0x76};

/*is objType one of Types0D8A? returns 1/0 (STAT_CheckUnready)*/
/*C_100B_01FB*/C_100B_01FB(int16 objType) {
	int16 i;

	for(i = 0; i < 5; i ++)
		if(Types0D8A[i] == objType)
			return 1;
	return 0;
}

/*is objType 0x19a, 0x10c or 0x1b9? returns 1/0*/
/*C_100B_0222*/C_100B_0222(register int16 objType) {
	return objType == 0x19a || objType == 0x10c || objType == 0x1b9;
}

/*0D94*/int16 Types0D94[] = {
	0x18,0x84,0xb9,0x3e,0x65,0x6b,0x94,0x96,0x97,0x9e,0xf3,0xfc,0x1bc,0x1bf,0x100
};

/*is objType one of Types0D94? returns 1/0*/
/*C_100B_0247*/C_100B_0247(int16 objType) {
	int16 i;

	for(i = 0; i < 15; i ++)
		if(Types0D94[i] == objType)
			return 1;
	return 0;
}

/*types whose TypeWeight is for 10 units*/
/*0DB2*/int16 WeightPer10Types[] = {
	0x39,0x3a,0x3b,0x3c,0x3f,0x40,0x5b,0x5d,0x5c,0x6b,0x83,0x49,0x4a,0x4b,0x4d,0x14b,
	0x18,0x84
};

/*is objType one of WeightPer10Types (weight counted per 10 units)?*/
/*C_100B_026E*/IsWeightPer10Type(int16 objType) {
	int16 i;

	for(i = 0; i < 18; i ++)
		if(WeightPer10Types[i] == objType)
			return 1;
	return 0;
}

/*stackable types*/
/*0DD6*/int16 StackableTypes[] = {
	0x39,0x3a,0x3b,0x3c,0x3f,0x40,0x7c,0x6b,0x6d,0x83,0x49,0x4a,0x4b,0x4d,0x1c1,0x18,
	0x84,0x9e,0x7a,0x4c,0x144
};

/*how an object type counts: 2 = quantity byte (stackable, see
  StackableTypes), 0 = one per object*/
/*C_100B_0295*/QuanType(int16 objTyp) {
	int16 i, ret;

	ret = 0;
	for(i = 0; i < 21; i++)
		if(StackableTypes[i] == objTyp) {
			ret = 2;
			break;
		}
	return ret;
}

/*0E00*/int16 TwoHandedTypes[] = {
	0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x34,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x191,
	0xf0,0xf1,0x139,0xa2,0xa1,0x28
};
/*0E2C*/int16 OneHandedTypes[] = {0x29,0x2a,0x31,0x32,0x33,0x36,0x37,0x38,0x10,0xf2,0x81,0x105,0x88,0x66};
/*0E48*/int16 LeftHandTypes[] = {0x6d,0x6f,0x71,0x73,0x75,0x6e,0x70,0x72,0x74,0x76};

/*equipment slot an object goes to (by its type): 0 head, 1 neck, 7 feet,
  4 chest, 6 left finger, 3 right finger, 8 two-handed weapon, 2 right
  hand, 5 left hand, or -1 if it can't be readied*/
/*C_100B_02C0*/STAT_GetEquipSlot(int16 objNum) {
	register int16 objTyp, i;

	objTyp = GetType(objNum);
	if(objTyp >= 1 && objTyp <= 5)
		return 0;
	if((objTyp >= 6 && objTyp <= 8) || objTyp == 0xea)
		return 1;
	if(objTyp >= 9 && objTyp <= 0xf)
		return 7;
	if((objTyp >= 0x11 && objTyp <= 0x17) || objTyp == 0x5a)
		return 4;
	if(objTyp >= 0x19 && objTyp <= 0x1e)
		return 6;
	if(objTyp >= 0x21 && objTyp <= 0x27)
		return 3;
	for(i = 21; i >= 0; i --)
		if(TwoHandedTypes[i] == objTyp)
			return 8;
	for(i = 13; i >= 0; i --)
		if(OneHandedTypes[i] == objTyp)
			return 2;
	for(i = 9; i >= 0; i --)
		if(LeftHandTypes[i] == objTyp)
			return 5;
	return -1;
}

/*0E5C*/int16 Types0E5C[] = {0xb3,0x184,0x133,0x12c,0xd9};

/*is objType one of Types0E5C? returns 1/0*/
/*C_100B_038E*/C_100B_038E(int16 objType) {
	int16 i;

	for(i = 0; i < 5; i ++)
		if(Types0E5C[i] == objType)
			return 1;
	return 0;
}

/*0E66*/int16 Types0E66[] = {
	0x184,0x101,0x102,0xfb,0xfc,0x155,0x151,0x152,0x154,0x153,0x173,0x172,0x1ba,0x1bb,0x1bc,0x12c
};

/*1 for types 1-0xb7 (the test for 0x11-0x3b and 0x3f-0x5a and 0x5d-0x86
  that follows is then always false), and the types of Types0E66; else 0*/
/*C_100B_03B5*/C_100B_03B5(register int16 objType) {
	int16 i;

	if(objType >= 1 && objType < 0xb8)
		return 1;
	if(objType >= 0x11 && objType < 0x3c)
		return 1;
	if(objType >= 0x3f && objType < 0x5b)
		return 1;
	if(objType >= 0x5d && objType < 0x87)
		return 1;
	for(i = 0; i < 16; i ++)
		if(Types0E66[i] == objType)
			return 1;
	return 0;
}

/*0E86*/int16 Types0E86[] = {0xcd,0x198,0xa8,0xaa,0xab,0x12c,0x133,0x12b};

/*is objType one of Types0E86? returns 1/0*/
/*C_100B_0410*/C_100B_0410(int16 objType) {
	int16 i;

	for(i = 0; i < 8; i ++)
		if(Types0E86[i] == objType)
			return 1;
	return 0;
}
