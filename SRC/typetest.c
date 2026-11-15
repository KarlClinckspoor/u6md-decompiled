/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE segment 222C: four small tests of a map tile
 * against fixed lists (the names are LOOK.LZC's). New in MD (no Savage
 * Empire counterpart). IsWaterTile is also used by console.c on the
 * animated tiles (D_B4C8).
 */
#include "se.h"

/*1 for the canal tiles 0x10-0x17 and the tiles 0xc0-0xc3, else 0*/
/*C_222C_0008*/IsWaterTile(register int16 n) {
	if(n >= 0x10 && n <= 0x17)
		return 1;
	if(n >= 0xc0 && n <= 0xc3)
		return 1;
	return 0;
}

/*1 for the rail track tiles 0x6c, 0x6d, 0x6e (track crossing), 0x4d and
  0x4f, else 0 (the rail car 0x19a runs on them, PushObj)*/
/*C_222C_0033*/IsTrackTile(register int16 n) {
	return n == 0x6c || n == 0x6d || n == 0x6e || n == 0x4d || n == 0x4f;
}

/*what digging there makes: 0x102 for the ground and dune tiles 0x20-0x24,
  0x0c, 0x0d and 0x28-0x3f (a hole 0x101), 0x100 for the polar ice 0x7c,
  0x7d and 0x7f (a hole in the ice 0xff); else 0 (nothing to dig)*/
/*C_222C_005F*/GetDigClass(register int16 n) {
	if((n >= 0x20 && n <= 0x24) || n == 0x0c || n == 0x0d || (n >= 0x28 && n <= 0x3f))
		return 0x102;
	if(n == 0x7c || n == 0x7d || n == 0x7f)
		return 0x100;
	return 0;
}

/*1 for the earthen wall tiles 0xf0-0xfb (mining them gives a pile of
  rocks 0x1ba), else 0*/
/*C_222C_00A4*/IsEarthenWallTile(int16 n) {
	return n >= 0xf0 && n <= 0xfb;
}
