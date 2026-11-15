/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE segment 222C: four small tests of a number
 * (a tile or an object type) against fixed lists. New in MD (no Savage
 * Empire counterpart). InRange_10_17_C0_C3 is used by console.c on the animated
 * tiles (D_B4C8).
 */
#include "se.h"

/*1 for 0x10-0x17 and 0xc0-0xc3, else 0*/
/*C_222C_0008*/InRange_10_17_C0_C3(register int16 n) {
	if(n >= 0x10 && n <= 0x17)
		return 1;
	if(n >= 0xc0 && n <= 0xc3)
		return 1;
	return 0;
}

/*1 for 0x6c, 0x6d, 0x6e, 0x4d and 0x4f, else 0*/
/*C_222C_0033*/Is_6C_6D_6E_4D_4F(register int16 n) {
	return n == 0x6c || n == 0x6d || n == 0x6e || n == 0x4d || n == 0x4f;
}

/*0x102 for 0x20-0x24, 0x0c, 0x0d and 0x28-0x3f; 0x100 for 0x7c, 0x7d and
  0x7f; else 0*/
/*C_222C_005F*/Class_20_3F_7C_7F(register int16 n) {
	if((n >= 0x20 && n <= 0x24) || n == 0x0c || n == 0x0d || (n >= 0x28 && n <= 0x3f))
		return 0x102;
	if(n == 0x7c || n == 0x7d || n == 0x7f)
		return 0x100;
	return 0;
}

/*1 for 0xf0-0xfb, else 0*/
/*C_222C_00A4*/InRange_F0_FB(int16 n) {
	return n >= 0xf0 && n <= 0xfb;
}
