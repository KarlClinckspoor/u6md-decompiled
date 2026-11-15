/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE segment 0E1A: graphic effects, missiles, the sky
 * bar, portraits, conversations, the paper doll, the peer map (Ultima VI's
 * gfx.c, by way of Savage Empire's: SE GAME.EXE 276B).
 *
 * - InitGraphicsDriver loads the graphic driver (an entry of system.lzc) and
 *   gives it its callbacks. The driver is called through the GR_xx macros:
 *   D_BA6A holds its far address, the offset part being set to the entry
 *   number before each call; D_BA66 is the driver's state block (tGr).
 * - Missiles: up to 16 sprites moving with a Bresenham line from one tile to
 *   several destination tiles, with rotation, collision with walls/objects
 *   and a list of the NPCs crossed (MissileHitList D_B9D6). Explosion and
 *   C_0E1A_1003 build on it. The animation is drawn in the offscreen map
 *   buffer; a "dirty rectangle" (D_BA6E left, D_BA70 top, D_BA74 right,
 *   D_BA76 bottom) is accumulated and copied to screen. The map window is
 *   x 8..167, y 16..159 of the buffer.
 * - the sky bar, the portraits (mdfaces.lzc over a background of
 *   mdback.lzc chosen by NPC), the dream pictures (mdream.lzc), the
 *   conversation scripts (talk.lzc), the paper doll (mdinv.lzc), the "peer"
 *   overview of the loaded area.
 *
 * Differences from Savage Empire's: there is no DISK_confirm (the driver's
 * disk callback is the empty DiskConfirm), no MagicalHalo, MagicWind,
 * AnimHailStorm, PortraitBg, ShowMidgame; the missile tables have 23 tiles;
 * coordinates wrap at 0x400; C_0E1A_1003 (a spray of 12 missiles in one of
 * eight directions) replaces AnimWebs; the sky bar is a strip of mdscreen.lzc
 * scrolled by the time of day, and it also sets the clocks' frames; the
 * portrait background comes from a table by NPC (C_0E1A_135B); the faces
 * and conversations share one NPC -> number choice (C_0E1A_16AA); scripts of
 * 0x36b0 bytes or more are refused; the paper doll is a picture of mdinv.lzc
 * by the NPC's type over four frame tiles that follow ColorPaper.
 */
#include "se.h"

#undef abs
extern int cdecl abs(int);

#define AREA_W 40
#define AREA_H 40

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;
};

/*a drawing surface (Ultima VI's)*/
struct tSurface {
	uint16 segment;/*_00*/
	uint16 far *pScanlines;/*_02*/
	uint16 right, left, top, bottom;/*_06,_08,_0a,_0c*/
};

/*the graphic driver's state (Ultima VI's tGr)*/
struct tGr {
	unsigned char header[6];/*_00*/
	void far *font;/*_06*/
	unsigned char textFg, textBg;
	unsigned char xorMode;
	unsigned char unknown0d;
	unsigned char animOn;
	unsigned char toBackBuffer;/*_0f: 1: color/fill/pixel primitives draw into the back buffer*/
	void (*pfnDiskConfirm)();/*_10*/
	unsigned char reserved14[8];
	void far *pWorkBuf;/*_1c*/
	void far *pDriverMem;/*_20*/
	void (*pfnLoadFile)();/*_24*/
	void (*pfnOpen)();/*_28*/
	void (*pfnRead)();/*_2c*/
	void (*pfnClose)();/*_30*/
};

/*the graphic driver: D_BA6A's offset selects the entry*/
#define GR_INIT(a)                         (D_BA6A.iii.ofs = 0x00, (struct tGr far *)(*D_BA6A.f)(a))
#define GR_SHUTDOWN()                      (D_BA6A.iii.ofs = 0x03, (*D_BA6A.f)())
#define GR_SET_COLOR(c)                    (D_BA6A.iii.ofs = 0x06, (*D_BA6A.f)(c))
#define GR_PUT_PIXEL(x,y)                  (D_BA6A.iii.ofs = 0x09, (*D_BA6A.f)(x,y))
#define GR_FILL_RECT(x0,y0,x1,y1)          (D_BA6A.iii.ofs = 0x12, (*D_BA6A.f)(x0,y0,x1,y1))
#define GR_PUT_TILE_SCREEN(tile,x,y)       (D_BA6A.iii.ofs = 0x2d, (*D_BA6A.f)(tile,x,y))
/*draw a bitmap on a surface (surface, x, y, bitmap, mode)*/
#define GR_DRAW_BITMAP(s,x,y,p,c)          (D_BA6A.iii.ofs = 0x30, (*D_BA6A.f)(s,x,y,p,c))
/*(slot, tile, rotation, pivotX, pivotY) build a rotated sprite*/
#define GR_DEFINE_SPRITE(i,a,b,c,d)        (D_BA6A.iii.ofs = 0x33, (*D_BA6A.f)(i,a,b,c,d))
/*draw sprite slot at x,y in the back buffer (also puts a saved background back)*/
#define GR_DRAW_SPRITE(i,x,y)              (D_BA6A.iii.ofs = 0x36, (*D_BA6A.f)(i,x,y))
/*save the back buffer under x,y into a sprite slot*/
#define GR_SAVE_SPRITE_BG(i,x,y)           (D_BA6A.iii.ofs = 0x39, (*D_BA6A.f)(i,x,y))
#define GR_PUT_TILE(tile,x,y)              (D_BA6A.iii.ofs = 0x42, (*D_BA6A.f)(tile,x,y))
/*copy back buffer rect x0,y0,x1,y1 to screen at x,y*/
#define GR_COPY_TO_SCREEN(x0,y0,x1,y1,x,y) (D_BA6A.iii.ofs = 0x45, (*D_BA6A.f)(x0,y0,x1,y1,x,y))
/*same with a fade-in effect*/
#define GR_FADE_TO_SCREEN(x0,y0,x1,y1,x,y) (D_BA6A.iii.ofs = 0x48, (*D_BA6A.f)(x0,y0,x1,y1,x,y))

/*OSI*/
extern int16 OSI_open(char *);
extern OSI_close(int16);
extern int32 OSI_read(int16, int32, int32, void far *);
extern cdecl MOUSE_SHOW();
extern cdecl MOUSE_HIDE();
extern int16 OSI_rand(int16, int16);/*OSI_rand(lo, hi)*/

/*main.c*/
extern void far *LoadFileGr(char far *, void far *, int16);
extern PaletteAnimation();
extern char DirIncrX[8], DirIncrY[8];
extern int16 GrDriverOption;
extern unsigned char MouseOn;
/*other game modules*/
extern AnimateTiles();
extern DrawMapWindow(int16);
extern DisplayTick();
extern MAP_BuildArea();
extern unsigned char GetTileAtXYZ(int16, int16, int16);
extern int16 WrapDelta(int16);
extern int16 MapDistance(int16, int16);
extern VIS_FloodFill(int16, int16, int16, int16);
extern VIS_UpdateView();
extern int16 NextLoc();
extern int16 FindLoc(int16, int16, int16);/*FindLoc(x, y, z)*/
extern int16 NextArea(void);
extern int16 SearchArea(int16, int16, int16, int16);/*SearchArea(x1, y1, x2, y2)*/
extern int16 IsBreakableType(int16);
extern int16 COMBAT_GetTargetAt(int16, int16, int16);/*COMBAT_GetTargetAt(shooter, x, y)*/
extern MUS_PlaySfx(int16);
extern int16 CON_mouseOffAt(int16, int16, int16, int16);
extern CON_mouseOn(int16);
extern CON_WaitKeyOrClick();
extern cdecl CON_printf(char far *, ...);
extern char far C_2BB4_22F1[];


/*library*/
extern int16 cdecl osiError;
extern void far * cdecl MEM_alloc(int32 size, int16 flags);
extern cdecl MEM_shutdown(int16);
extern int32 LIB_entrySize(char far *name, int16 index);
extern void far *LZC_loadType(char far *name, int16 index, void far *out, int16 memflags, char far *work);
extern void cdecl StopAllSounds(void);
extern void cdecl ShutdownSound(void);
extern void cdecl TimerDelay(int32);

extern int16 D_0F18;/*MapAnimEnabled*/
extern int16 VisibilityDirty;
extern int16 LocPartOffset;
extern unsigned char Time_M;
extern unsigned char Time_H;
extern int16 MapX;
extern int16 MapY;
extern int16 MapZ;
extern char InCombat;
extern unsigned char PlayerIsFemale;
extern int32 GameFlags;/*0x10: in a dream*/
extern int16 MissileShooter;
extern unsigned char RangeTable[8][8];
extern int16 ViewX, ViewY;
extern unsigned char AnimateAtPrompt;

/*uninitialised data*/
extern struct tSurface D_3232;/*Screen*/
extern unsigned char ObjStatus[];
extern char far *D_3E80;/*work area of the .lzc loader*/
extern unsigned char AreaFlags[AREA_H][AREA_W];
extern unsigned char AreaTiles[AREA_H][AREA_W];
extern char far *D_54B2;/*ScratchBuf + 0x36b0*/
extern int16 Amount[];/*quantity (low byte), quality (high byte)*/
extern unsigned char NPCStatus[];
extern unsigned char far *D_6DBA;/*TileFlag*/
extern int16 far *D_6EBE;/*BaseTile*/
extern unsigned char D_6EC6[];/*TalkFlags*/
extern unsigned char AreaLight[AREA_H][AREA_W];
extern int16 D_770E[];/*AnimFirstFrame*/
extern struct coord ObjPos[];
extern struct tSurface D_9CAE;/*MapSurface*/
extern int16 D_9CC2;/*NumAnimData*/
extern uint16 ObjShapeType[];
extern unsigned char far *D_B4C4;/*TerrainType*/
extern int16 D_B4C8[];/*AnimTile*/
extern int16 D_B9D6[];/*MissileHitList*/
extern struct tGr far *D_BA66;/*GrInfo*/
extern union tHandler D_BA6A;/*GrDriver*/
extern int16 D_BA6E, D_BA70, D_BA74, D_BA76;/*DirtyRect: left, top, right, bottom*/
extern int16 D_BA72;/*width - 1 of the shaken rectangle*/
extern void far *D_BA78;/*the paper doll picture (mdinv.lzc)*/
extern int16 D_BA7C;/*its entry, -1: none*/
extern void far *D_BA7E;/*mdscreen.lzc entry 2: the sky*/
extern int16 AreaX;
extern int16 AreaY;
extern unsigned char Obj_11x11[11][11];/*first entry of each cell's object tiles*/
extern int16 CellObjTile[];
extern unsigned char CellObjNext[];
extern int16 Tile_11x11[11][11];
extern int16 MissileHitObj;
extern int16 MissileEnd[2];

#define TileFlag D_6DBA
#define BaseTile D_6EBE
#define TalkFlags D_6EC6
#define TerrainType D_B4C4
#define MissileHitList D_B9D6
#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))
#define IsTileWin(tile) (TileFlag[tile] & 0x08)
#define IsTileNos(tile) (TileFlag[tile] & 0x20)
#define HideMouse if(MouseOn) MOUSE_HIDE();
#define ShowMouse if(MouseOn) MOUSE_SHOW();

/*====---- ----====*/
/*driver init parameter, indexed by adapter type:
  BIOS video mode (MCGA/EGA/Tandy) or video segment (CGA/Hercules)*/
/*0804*/int16 GrDriverParam[] = {
	0x13,
	0xD,
	0x9,
	0xB800,
	0xB000
};
/*080E*/int16 ScreenFade = 1;
/*colors*/
/*0810*/int16 ColorRed = 4;/*peer: damaging terrain*/
/*0812*/int16 ColorGreen = 2;/*peer: passable terrain*/
/*0814*/int16 ColorBlue = 9;/*peer: impassable wet terrain*/
/*0816*/int16 ColorMagenta = 0xD;
/*0818*/int16 ColorYellow = 0xE;
/*081A*/int16 ColorBlack = 0;
/*081C*/int16 ColorPaper = 0xDC;/*background of the map window and the panels*/
/*081E*/int16 ColorText = 0;/*normal text color*/
/*0820*/int16 D_0820 = 0;
/*0822*/int16 ColorGray = 7;
/*0824*/int16 ColorBrown = 6;
/*0826*/int16 D_0826 = 1;
/*0828*/int16 ColorLightGreen = 0xA;/*peer: impassable terrain*/
/*082A*/int16 D_082A = 0xF;
/*082C*/int16 D_082C = 0xC;
/*082E*/int16 D_082E = 0xDC;

/*the driver's disk request callback: nothing to do*/
static DiskConfirm(int16);

/*C_0E1A_000C*/static DiskConfirm(int16 disk) {
}

/*init gr:
  load the graphic driver for adapter type "adapter" (entry of system.lzc,
  0:MCGA 1:EGA 2:Tandy 3:CGA, 4 uses 3's), initialise it and give it the
  callbacks it needs (disk request, file loading/open/read/close)*/
/*C_0E1A_0013*/InitGraphicsDriver(int16 adapter) {
	char *name;
	int16 size;

	name = "system.lzc";
	size = LIB_entrySize(name, adapter == 4?3:adapter) + 0x10;
	D_BA6A.p = MEM_alloc(size, 2);
	if(D_BA6A.p == 0) {
		GR_SHUTDOWN();
		MEM_shutdown(8);
		exit(1);
	}
	D_BA6A.p = LZC_loadType(name, adapter == 4?3:adapter, D_BA6A.p, 2, D_3E80);
	D_BA66 = GR_INIT(GrDriverParam[adapter] + GrDriverOption);
	D_BA66->pfnDiskConfirm = DiskConfirm;
	D_BA66->pfnLoadFile = LoadFileGr;
	D_BA66->pfnOpen = OSI_open;
	D_BA66->pfnRead = OSI_read;
	D_BA66->pfnClose = OSI_close;
	if(adapter) {
		if(adapter == 1)
			D_BA72 = 0x8f;
		else
			D_BA72 = 0x95;
		D_082E = ColorPaper = ColorYellow;
	} else {
		D_BA72 = 0x96;
	}
}

/*redefineClipping:
  extend the dirty rectangle with the 16x16 square centered on x,y
  (map buffer coordinates), clamped to the map window (8..167, 16..159)*/
/*C_0E1A_0135*/static ExtendDirtyRect(int16 x, int16 y) {
	if(x < 16)
		D_BA6E = 8;
	else if(x - 8 < D_BA6E)
		D_BA6E = x - 8;

	if(y < 24)
		D_BA70 = 16;
	else if(y - 8 < D_BA70)
		D_BA70 = y - 8;

	if(x > 159)
		D_BA74 = 167;
	else if(x + 8 > D_BA74)
		D_BA74 = x + 8;

	if(y > 151)
		D_BA76 = 159;
	else if(y + 8 > D_BA76)
		D_BA76 = y + 8;
}

/*redraw, over the sprites, what must stay in front of them in the
  rectangle x0,y0-x1,y1 of the offscreen buffer: foreground tiles
  of the objects and hidden/dark tiles*/
/*C_0E1A_01C8*/static RedrawForeground(int16 x0, int16 y0, int16 x1, int16 y1) {
	int16 tx, ty;
	int16 entry, tile;

	for(tx = SCR2TIL(x0); tx <= SCR2TIL(x1); tx ++) {
		for(ty = SCR2TIL(y0); ty <= SCR2TIL(y1); ty ++) {
			/*objects' tiles of this cell: linked list CellObjNext, tiles in CellObjTile*/
			for(entry = Obj_11x11[ty][tx]; entry; entry = CellObjNext[entry]) {
				tile = CellObjTile[entry];
				if(tile && (TileFlag[tile & 0x7ff] & 0x10))
					GR_PUT_TILE(tile & 0x7ff, TIL2SCR(tx), TIL2SCR(ty));
			}
			if(Tile_11x11[ty][tx] == 0xff || Tile_11x11[ty][tx] == 0x10c)
				GR_PUT_TILE(0xff, TIL2SCR(tx), TIL2SCR(ty));
		}
	}
}

/*distance.rotation angles map:
  angle of the vector (dx,dy) indexed [dy + 7][dx + 7];
  0x80 is a full turn, 0x00=east, 0x20=north, 0x40=west, 0x60=south*/
/*0830*/unsigned char MissileAngles[][16] = {
	{0x30,0x2E,0x2D,0x2B,0x28,0x26,0x23,0x20,0x1D,0x1A,0x18,0x15,0x13,0x12,0x10,0x0F},
	{0x32,0x30,0x2E,0x2C,0x29,0x26,0x23,0x20,0x1D,0x1A,0x17,0x14,0x12,0x10,0x0E,0x0D},
	{0x34,0x32,0x30,0x2E,0x2B,0x27,0x24,0x20,0x1C,0x19,0x15,0x12,0x10,0x0E,0x0D,0x0B},
	{0x35,0x34,0x32,0x30,0x2D,0x29,0x25,0x20,0x1B,0x17,0x13,0x10,0x0E,0x0C,0x0B,0x09},
	{0x38,0x37,0x35,0x33,0x30,0x2C,0x26,0x20,0x1A,0x14,0x10,0x0D,0x0B,0x09,0x08,0x07},
	{0x3A,0x3A,0x39,0x37,0x34,0x30,0x29,0x20,0x17,0x10,0x0C,0x09,0x07,0x06,0x06,0x05},
	{0x3D,0x3D,0x3C,0x3B,0x3A,0x37,0x30,0x20,0x10,0x09,0x06,0x05,0x04,0x03,0x03,0x03},
	{0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	{0x43,0x43,0x44,0x45,0x46,0x49,0x50,0x60,0x70,0x77,0x7A,0x7B,0x7C,0x7D,0x7D,0x7D},
	{0x46,0x46,0x47,0x49,0x4C,0x50,0x57,0x60,0x69,0x70,0x74,0x77,0x79,0x7A,0x7A,0x7B},
	{0x48,0x49,0x4B,0x4D,0x50,0x54,0x5A,0x60,0x66,0x6C,0x70,0x73,0x75,0x77,0x78,0x79},
	{0x4B,0x4C,0x4E,0x50,0x53,0x57,0x5B,0x60,0x65,0x69,0x6D,0x70,0x72,0x74,0x75,0x77},
	{0x4D,0x4E,0x50,0x52,0x55,0x59,0x5C,0x60,0x64,0x67,0x6B,0x6E,0x70,0x72,0x73,0x75},
	{0x4E,0x50,0x52,0x54,0x57,0x5A,0x5D,0x60,0x63,0x66,0x69,0x6C,0x6E,0x70,0x72,0x73},
	{0x50,0x52,0x53,0x55,0x58,0x5A,0x5D,0x60,0x63,0x66,0x68,0x6B,0x6D,0x6E,0x70,0x71},
	{0x51,0x53,0x55,0x57,0x59,0x5B,0x5D,0x60,0x63,0x65,0x67,0x69,0x6B,0x6D,0x6F,0x70}
};
/*missile tiles; the following tables are indexed by the position of the
  projectile tile in this list ("misType", 0 = unknown tile)*/
/*0930*/uint16 MissileTiles[] = {
	0x0FF,
	0x101,0x104,0x105,0x107,0x103,
	0x106,0x14E,0x14F,0x16B,0x16A,
	0x10A,0x23E,0x23F,0x235,0x230,
	0x18A,0x18E,0x10B,0x16F,0x16C,
	0x16D,0x16E
};
/*angle at which the tile is drawn (subtracted from the flight angle)*/
/*095E*/unsigned char MissileTileAngle[] = {0x20,0x00,0x20,0x00,0x00,0x00,0x50,0x00,0x00,0x20,0x20,0x00,0x00,0x00,0x20,0x20,0x00,0x00,0x20,0x00,0x00,0x00,0x00};
/*spin added at each redraw*/
/*0975*/unsigned char MissileSpin[] = {0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x00,0x07,0x00,0x00,0x02,0x02,0x02,0x02,0x00,0x02,0x00,0x00,0x00};
/*rotation pivot x in the tile (0x7f = center)*/
/*098C*/unsigned char MissilePivotX[] = {0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F};
/*rotation pivot y in the tile (0x7f = center)*/
/*09A3*/unsigned char MissilePivotY[] = {0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F};
/*the sprite is redrawn every N pixels of movement*/
/*09BA*/unsigned char MissileRefresh[] = {0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x0A,0x0A,0x07,0x02,0x01,0x02,0x02};

#define MAX_MISSILES 16
/*world pixel -> tile, wrapping*/
#define MSCR2TIL(i) (((i) >> 4) & 0x3ff)




/*animate num_dst projectiles (tile projTile, 0 = invisible) flying from
  tile src_x,src_y to the tiles pX_dst[],pY_dst[], one pixel per step
  (Bresenham). A projectile stops when it goes out of max_range (RangeTable
  distance), hits a tile that blocks missiles (windows are crossed when
  adjacent to the source) or an object whose tile blocks missiles.
  attr: 1 = boomerang return (flies from the destination back to the source)
        2 = "trail" mode: the sprites are never erased; refresh every 6 pixels
            (else every pixel: MissileRefresh is only the delay of a lone missile)
        4 = (U6: explosion-like sound)
        8 = one non rotated sprite (slot 15) drawn at every position
  Out:
   - pX_dst/pY_dst = tile where each projectile stopped,
   - MissileEnd = last tile reached by projectile #0,
   - MissileHitObj = object hit at the stop tile of #0 (if MissileShooter, the shooter, is set),
   - MissileHitList = list of the objects of IsBreakableType types (with a quantity)
     or type 0xa0 hit, then of the NPCs (< 0x100) crossed/hit (NPCs of
     ObjStatus 2 with the types 0x175, 0x16a, 0xfe, 0x182 count as objects),
     -1 terminated (at most 0x3f).
  Coordinates wrap at 0x400 (WrapDelta, MapDistance); a lone missile waits
  (MissileRefresh[misType] + 1) * 0x94f (TimerDelay) at each pixel.
  returns 1 if every projectile reached its destination*/
/*C_0E1A_02B3*/Missiles(
	int16 src_x, int16 src_y,
	int16 num_dst, int16 *pX_dst, int16 *pY_dst,
	int16 max_range, int16 projTile,
	int16 attr
) {
	int16 i;
	int16 dst;
	int16 objNum,refreshRate,temp,misType,delta_max;
	int16 tile_delta_x,tile_delta_y;
	int16 ret,whichDir;
	int16 spr_new_x,spr_new_y;
	int16 mustDisplay,numTile;
	int16 unused;
	int16 numHits;/*objects (not NPCs) already put in MissileHitList*/
	/*current position in world pixels (tile * 16 + 8)*/
	int16 cur_x[MAX_MISSILES],cur_y[MAX_MISSILES];
	/*Bresenham: major/minor deltas, increments [0]=straight [1]=diagonal step*/
	int16 delta_x[MAX_MISSILES],delta_y[MAX_MISSILES];
	int16 incr_x[MAX_MISSILES][2],incr_y[MAX_MISSILES][2];
	int16 b_error[MAX_MISSILES],berr_incr[MAX_MISSILES][2];
	/*last tile entered*/
	int16 tile_x[MAX_MISSILES],tile_y[MAX_MISSILES];
	int16 spr_rot[MAX_MISSILES];
	/*1 bit per NPC (0x100 NPCs)*/
	int16 collision_mask[0x100/8];
	/*sprite position in the map buffer*/
	int16 spr_x[MAX_MISSILES],spr_y[MAX_MISSILES];

	numHits = 0;
	unused = 0;
	DrawMapWindow(0);

	for(i = 0; i < 0x100/8; i ++)
		collision_mask[i] = 0;

	HideMouse
	ret = 1;

	MissileEnd[0] =
	tile_x[0] = src_x;
	MissileEnd[1] =
	tile_y[0] = src_y;

	if(attr & 1) {
		temp = tile_x[0]; tile_x[0] = pX_dst[0]; pX_dst[0] = temp;
		temp = tile_y[0]; tile_y[0] = pY_dst[0]; pY_dst[0] = temp;
	}

	cur_x[0] = TIL2SCR(tile_x[0]) + 8;
	cur_y[0] = TIL2SCR(tile_y[0]) + 8;

	misType = 0;
	for(i = 0; i < 23; i ++) {
		if(MissileTiles[i] == projTile) {
			misType = i;
			break;
		}
	}

	if(attr & 2)
		refreshRate = 6;
	else
		refreshRate = 1;

	/*define the sprites: oriented toward each destination*/
	if(projTile) {
		if(attr & 8) {
			GR_DEFINE_SPRITE(15, projTile, 0, 0x7f, 0x7f);
		} else {
			for(dst = 0; dst < num_dst; dst ++) {
				spr_rot[dst] =
					MissileAngles[pY_dst[dst] - tile_y[0] + 7][pX_dst[dst] - tile_x[0] + 7] - MissileTileAngle[misType];
				if(attr & 2)
					GR_DEFINE_SPRITE(dst, projTile, spr_rot[dst], MissilePivotX[misType], MissilePivotY[misType]);
				else
					GR_DEFINE_SPRITE(dst + 8, projTile, spr_rot[dst], MissilePivotX[misType], MissilePivotY[misType]);
			}
		}
	}

	/*Bresenham setup*/
	delta_max = 0;
	for(dst = 0; dst < num_dst; dst ++) {
		cur_x[dst] = cur_x[0];
		cur_y[dst] = cur_y[0];
		tile_x[dst] = tile_x[0];
		tile_y[dst] = tile_y[0];
		delta_x[dst] = TIL2SCR(WrapDelta(pX_dst[dst] - tile_x[0]));
		delta_y[dst] = TIL2SCR(WrapDelta(pY_dst[dst] - tile_y[0]));
		incr_y[dst][0] = 0;
		if(delta_x[dst] < 0) {
			delta_x[dst] = -delta_x[dst];
			incr_x[dst][0] =
			incr_x[dst][1] = -1;
		} else {
			incr_x[dst][0] =
			incr_x[dst][1] = 1;
		}
		if(delta_y[dst] < 0) {
			delta_y[dst] = -delta_y[dst];
			incr_y[dst][1] = -1;
		} else {
			incr_y[dst][1] = 1;
		}
		/*y major: swap the roles*/
		if(delta_x[dst] < delta_y[dst]) {
			temp = delta_x[dst];
			delta_x[dst] = delta_y[dst];
			delta_y[dst] = temp;
			incr_y[dst][0] = incr_y[dst][1];
			incr_x[dst][1] = incr_x[dst][0];
			incr_x[dst][0] = 0;
		}
		if(delta_x[dst] > delta_max)
			delta_max = delta_x[dst];
		b_error[dst] = delta_y[dst] + delta_y[dst] - delta_x[dst];
		berr_incr[dst][0] = delta_y[dst] + delta_y[dst];
		berr_incr[dst][1] = (delta_y[dst] - delta_x[dst]) << 1;
	}

	/*one pixel per step; delta_x[dst] is the number of steps of
	  projectile dst (set to the current step when it stops)*/
	for(i = 0; i < delta_max; i ++) {
		mustDisplay = 0;
		/*empty dirty rectangle*/
		D_BA6E = 168;
		D_BA70 = 160;
		D_BA74 = 7;
		D_BA76 = 15;
		for(dst = 0; dst < num_dst; dst ++) {
			if(delta_x[dst] <= i) {
				/*arrived/stopped: its last place must still be refreshed*/
				if(i)
					ExtendDirtyRect(spr_x[dst], spr_y[dst]);
				continue;
			}
			whichDir = (b_error[dst] >= 0);
			b_error[dst] += berr_incr[dst][whichDir];
			cur_x[dst] += incr_x[dst][whichDir];
			cur_y[dst] += incr_y[dst][whichDir];

			if(projTile && refreshRate && (i % refreshRate) == 0) {
				spr_new_x = cur_x[dst] - TIL2SCR(MapX - 5);
				spr_new_y = cur_y[dst] - TIL2SCR(MapY - 5);
				if(spr_new_x >= 8 && spr_new_x < 168 && spr_new_y >= 8 && spr_new_y < 168) {
					if(!mustDisplay) {
						if(dst == 0) {
							/*(U6 played the flight sound here; the test is left)*/
						}
						mustDisplay = 1;
					}
					ExtendDirtyRect(spr_new_x, spr_new_y);
					if(attr & 2) {
						/*trail: draw directly, never erased*/
						GR_DRAW_SPRITE(dst, spr_new_x, spr_new_y);
					} else {
						if(i)
							ExtendDirtyRect(spr_x[dst], spr_y[dst]);
						spr_x[dst] = spr_new_x;
						spr_y[dst] = spr_new_y;
					}
				}
				/*spinning projectile*/
				if(MissileSpin[misType]) {
					 spr_rot[dst] += MissileSpin[misType];
					 if(!(attr & 2) && !(attr & 8))
						 GR_DEFINE_SPRITE(dst + 8, projTile, spr_rot[dst], MissilePivotX[misType], MissilePivotY[misType]);
				}
			}

			/*still in the same tile?*/
			if(MSCR2TIL(cur_x[dst]) == tile_x[dst] && MSCR2TIL(cur_y[dst]) == tile_y[dst])
				continue;

			/*entering a new tile: check range and obstacles*/
			tile_delta_x = MapDistance(MSCR2TIL(cur_x[dst]), src_x);
			tile_delta_y = MapDistance(MSCR2TIL(cur_y[dst]), src_y);
			if(tile_delta_x > 7 || tile_delta_y > 7 || RangeTable[tile_delta_x][tile_delta_y] > max_range) {
stop:
				/*blocked or out of range: this projectile stops here.
				  note: the break also leaves the loop for the remaining
				  projectiles of this step*/
				delta_x[dst] = i;
				break;
			}
			tile_x[dst] = MSCR2TIL(cur_x[dst]);
			tile_y[dst] = MSCR2TIL(cur_y[dst]);
			numTile = GetTileAtXYZ(tile_x[dst], tile_y[dst], MapZ);
			if((tile_delta_x > 1 || tile_delta_y > 1 || !IsTileWin(numTile)) && IsTileNos(numTile))
				goto stop;
			/*mark the NPCs of the tile; stop at the first object blocking missiles*/
			for(objNum = FindLoc(tile_x[dst], tile_y[dst], MapZ); objNum >= 0; objNum = NextLoc()) {
				if(objNum < 0x100 && objNum >= 0 && (
					!(ObjStatus[objNum] & 2) ||
					(GetType(objNum) != 0x175 && GetType(objNum) != 0x16a && GetType(objNum) != 0xfe && GetType(objNum) != 0x182)
				)) {
					collision_mask[objNum >> 3] |= (1 << (objNum & 7));
				} else if(numHits <= 0x3e && ((GetQuan(objNum) && IsBreakableType(GetType(objNum))) || GetType(objNum) == 0xa0)) {
					register int16 k;

					for(k = 0; k < numHits; k ++)
						if(MissileHitList[k] == objNum)
							break;
					if(k >= numHits)
						MissileHitList[numHits ++] = objNum;
				}
				if(IsTileNos(TILE_FRAME(objNum) - LocPartOffset))
					break;
			}
			if(objNum >= 0)
				goto stop;
			if(dst == 0) {
				MissileEnd[0] = tile_x[0];
				MissileEnd[1] = tile_y[0];
			}
		}

		if(mustDisplay) {
		if(!(i & 3))
			PaletteAnimation();

		/*save the background under the sprites (slots 0-7)*/
		if(!(attr & 2)) {
			for(dst = 0; dst < num_dst; dst ++) {
				if(delta_x[dst] > i || (attr & 8))
					GR_SAVE_SPRITE_BG(dst, spr_x[dst], spr_y[dst]);
			}
		}
		/*draw the projectiles*/
		if(!(attr & 2)) {
			for(dst = 0; dst < num_dst; dst ++) {
				if(attr & 8)
					GR_DRAW_SPRITE(15, spr_x[dst], spr_y[dst]);
				else if(delta_x[dst] > i)
					GR_DRAW_SPRITE(dst + 8, spr_x[dst], spr_y[dst]);
			}
		}
		/*foreground + copy the dirty rectangle to screen*/
		if(D_BA6E <= D_BA74 && D_BA70 <= D_BA76) {
			RedrawForeground(D_BA6E, D_BA70, D_BA74, D_BA76);
			GR_COPY_TO_SCREEN(D_BA6E, D_BA70, D_BA74, D_BA76, D_BA6E, D_BA70);
		}
		/*restore the background in the offscreen buffer*/
		if(!(attr & 2)) {
			for(dst = 0; dst < num_dst; dst ++) {
				if(delta_x[dst] > i || (attr & 8))
					GR_DRAW_SPRITE(dst, spr_x[dst], spr_y[dst]);
			}
		}
		}
		if(num_dst == 1)
			TimerDelay((MissileRefresh[misType] + 1) * 0x94fL);
	}

	if(projTile && !(attr & 2) && !(attr & 8))
		DisplayTick();
	/*report where the projectiles stopped*/
	for(dst = 0; dst < num_dst; dst ++) {
		if(
			tile_x[dst] == pX_dst[dst] &&
			(tile_y[dst] == pY_dst[dst] || (attr & 1))
		) continue;
		ret = 0;
		pX_dst[dst] = tile_x[dst];
		pY_dst[dst] = tile_y[dst];
		if(dst == 0 && MissileShooter >= 0)
			MissileHitObj = COMBAT_GetTargetAt(MissileShooter, tile_x[0], tile_y[0]);
	}
	ShowMouse

	/*collision bitmap -> list of NPCs after the objects (max 0x3f);
	  when the list is full, NPCs replace the objects at its start*/
	objNum = 0;
	for(i = 0; i < 0x100/8; i ++) {
		for(dst = 0; dst < 8; dst ++) {
			if(collision_mask[i] & (1 << dst)) {
				if(numHits > 0x3e) {
					if(objNum > 0x3e || (MissileHitList[objNum] < 0x100 && MissileHitList[objNum] >= 0))
						break;
					MissileHitList[objNum] = (i << 3) + dst;
					objNum ++;
				} else {
					MissileHitList[numHits] = (i << 3) + dst;
					numHits ++;
				}
			}
		}
	}
	MissileHitList[numHits] = -1;
	return ret;
}

/*explosion pattern: the 3x3 square around the center*/
/*09D1*/char ExplosionDX[] = {-1, 0, 1,-1, 0, 1,-1, 0, 1};
/*09DA*/char ExplosionDY[] = {-1,-1,-1, 0, 0, 0, 1, 1, 1};

/*explosion centered on x,y: 9 trails of missileTile toward the pattern
  above (range 3); returns the -1 terminated list (MissileHitList) of the NPCs
  hit, including those on the center tile and the IsBreakableType objects there*/
int16 far /*C_0E1A_0EEA*/*Explosion(int16 x, int16 y, int16 missileTile) {
	register int16 count, objNum;
	int16 x_dst[9], y_dst[9];

	for(count = 0; count < 9; count ++) {
		x_dst[count] = (x + ExplosionDX[count]) & 0x3ff;
		y_dst[count] = (y + ExplosionDY[count]) & 0x3ff;
	}
	Missiles(x, y, 9, x_dst, y_dst, 3, missileTile, 4|2);
	/*append the NPCs of the center tile*/
	for(count = 0; count < 0x3e; count ++)
		if(MissileHitList[count] < 0)
			break;
	for(objNum = FindLoc(x, y, MapZ); count < 0x3f && objNum >= 0; objNum = NextLoc()) {
		if((objNum < 0x100 && objNum >= 0) || (GetQuan(objNum) && IsBreakableType(GetType(objNum))))
			MissileHitList[count ++] = objNum;
		if(IsTileNos(TILE_FRAME(objNum) - LocPartOffset))
			break;
	}
	MissileHitList[count] = -1;
	return MissileHitList;
}

/*spray: offsets of the 12 destinations by direction (0 north .. 7)*/
/*09E3*/char SprayDX[8][12] = {
	{ 0, 0,-1, 1, 0,-1, 1, 0,-1, 1,-2, 2},
	{ 1, 1, 2, 2, 2, 3, 3, 2, 4, 3, 4, 4},
	{ 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 4},
	{ 1, 1, 2, 2, 2, 3, 3, 2, 4, 3, 4, 4},
	{ 0, 0,-1, 1, 0,-1, 1, 0,-1, 1,-2, 2},
	{-1,-1,-2,-2,-2,-3,-3,-2,-4,-3,-4,-4},
	{-1,-2,-2,-2,-3,-3,-3,-4,-4,-4,-4,-4},
	{-1,-1,-2,-2,-2,-3,-3,-2,-4,-3,-4,-4}
};
/*0A43*/char SprayDY[8][12] = {
	{-1,-2,-2,-2,-3,-3,-3,-4,-4,-4,-4,-4},
	{-1,-2,-1,-2,-3,-2,-3,-4,-2,-4,-3,-4},
	{ 0, 0,-1, 1, 0,-1, 1, 0,-1, 1,-2, 2},
	{ 1, 2, 1, 2, 3, 2, 3, 4, 2, 4, 3, 4},
	{ 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 4},
	{ 1, 2, 1, 2, 3, 2, 3, 4, 2, 4, 3, 4},
	{ 0, 0,-1, 1, 0,-1, 1, 0,-1, 1,-2, 2},
	{-1,-2,-1,-2,-3,-2,-3,-4,-2,-4,-3,-4}
};
/*direction by [dy + 4][dx + 4]*/
/*0AA3*/unsigned char SprayDir[9][9] = {
	{7,7,0,0,0,0,1,1,1},
	{7,7,7,0,0,0,1,1,1},
	{7,7,7,0,0,1,1,1,2},
	{6,6,7,7,0,1,2,2,2},
	{6,6,6,6,0,2,2,2,2},
	{6,6,6,5,4,3,3,2,2},
	{6,5,5,5,4,4,3,3,3},
	{5,5,5,4,4,4,3,3,3},
	{5,5,5,4,4,4,4,3,3}
};

/*a spray of 12 missiles (tile missileTile, range 6) from x,y toward x2,y2:
  the destinations come from the direction (SprayDir, SprayDX/SprayDY).
  returns the list of NPCs hit (MissileHitList)*/
int16 far /*C_0E1A_1003*/*C_0E1A_1003(int16 x, int16 y, int16 x2, int16 y2, int16 missileTile) {
	int16 dx;
	register int16 dy;
	int16 dir;
	register int16 i;
	int16 x_dst[12], y_dst[12];

	dx = WrapDelta(x2 - x) + 4;
	dy = WrapDelta(y2 - y) + 4;
	if(dx < 0)
		dx = 0;
	if(dx > 8)
		dx = 8;
	if(dy < 0)
		dy = 0;
	if(dy > 8)
		dy = 8;
	dir = SprayDir[dy][dx];
	for(i = 0; i < 12; i ++) {
		x_dst[i] = (x + SprayDX[dir][i]) & 0x3ff;
		y_dst[i] = (y + SprayDY[dir][i]) & 0x3ff;
	}
	Missiles(x, y, 12, x_dst, y_dst, 6, missileTile, 8);
	return MissileHitList;
}

/*shake the map window "duration" times: the inner part of the
  offscreen map is copied to screen at a random offset of +/-ampl pixels*/
/*C_0E1A_10E9*/ShakeScreen(int16 duration, int16 ampl) {
	register int16 i;
	int16 ofs_x;
	register int16 ofs_y;

	DrawMapWindow(0);
	for(i = 0; i < duration; i ++) {
		MUS_PlaySfx(0);
		ofs_x = OSI_rand(8 - ampl, ampl + 8);
		ofs_y = OSI_rand(16 - ampl, ampl + 16);
		GR_COPY_TO_SCREEN(ofs_x, ofs_y, ofs_x + D_BA72, ofs_y + 0x8f, 8, 16);
		StopAllSounds();
	}
	DisplayTick();
}

/*clear the 11x11 view: all tiles hidden, no object, and rebuild the
  free list of the object tile entries (CellObjNext[i] = i + 1)*/
/*C_0E1A_116B*/static ClearMapView() {
	int16 i, j;

	for(i = 0; i < 11; i ++) {
		for(j = 0; j < 11; j ++)
			Tile_11x11[i][j] = 0xff;
	}
	for(i = 0; i < 0x100; i++)
		CellObjNext[i] = i + 1;
	for(i = 0; i < 11; i ++) {
		for(j = 0; j < 11; j ++)
			Obj_11x11[i][j] = 0;
	}
}

/*blank the map window (displayed), then rebuild the view
  (tile map and visible tiles/objects) without displaying it*/
/*C_0E1A_11D2*/ResetMapView() {
	ClearMapView();
	VisibilityDirty = 0;
	ScreenFade = 1;
	DrawMapWindow(1);
	MAP_BuildArea();
	VIS_UpdateView();
}

/*the strip above the map window: on MapZ 2/3 gray, underground black
  (and the map animations are enabled), on the surface a strip of the sky
  picture (mdscreen.lzc entry 2) scrolled by the time of day; then the
  animated tiles that show BaseTile[0x62] (clocks) get the hour as frame*/
/*C_0E1A_11FA*/DrawSkyBar() {
	register int16 i;
	register int16 mouseState;

	mouseState = CON_mouseOffAt(0x20, 2, 0x8f, 0xc);
	if(MapZ == 2 || MapZ == 3) {
		GR_SET_COLOR(ColorGray);
		GR_FILL_RECT(0x20, 2, 0x8f, 0xc);
		D_0F18 = 1;
	} else if(MapZ > 0) {
		GR_SET_COLOR(ColorBlack);
		GR_FILL_RECT(0x20, 2, 0x8f, 0xc);
		D_0F18 = 1;
	} else {
		if(Time_H >= 12)
			GR_DRAW_BITMAP(&D_9CAE, 8 - (((Time_H - 12) << 4) + (Time_M / 15 << 2)), 0x10, D_BA7E, 3);
		else
			GR_DRAW_BITMAP(&D_9CAE, 8 - ((Time_H << 4) + (Time_M / 15 << 2)), 0x10, D_BA7E, 2);
		GR_COPY_TO_SCREEN(8, 0x10, 0x77, 0x1a, 0x20, 2);
	}
	CON_mouseOn(mouseState);
	for(i = 0; i < D_9CC2; i ++) {
		if(D_B4C8[i] == BaseTile[0x62])
			D_770E[i] = Time_H % 12 + 0x1a0;
	}
	AnimateTiles();
}

/*0AF4*/unsigned char PortraitBgs[] = {
	0x22,0x17,0x50,0x00,0x00,0x00,0x00,0x00,0x56,0x27,0x00,0x00,0x55,0x45,0x70,0x00,
	0x53,0x25,0x45,0x15,0x17,0x37,0x45,0x32,0x24,0x53,0x21,0x42,0x13,0x66,0x61,0x20,
	0x67,0x23,0x15,0x60,0x00,0x00,0x00,0x00,0x37,0x45,0x32,0x24,0x75,0x73,0x50,0x12,
	0x51,0x02,0x65,0x61,0x45,0x46,0x31,0x00,0x24,0x77,0x06,0x50,0x00
};
/*0B31*/char FacesFile[] = "mdfaces.lzc";
/*0B3D*/char BkgrndFile[] = "mdback.lzc";
/*0B48*/char DreamFile[] = "mdream.lzc";

/*the portrait background (entry of mdback.lzc) of NPC npc: a 4-bit
  field of PortraitBgs (high nibble for even npc), random for NPCs above
  0x79, 0 in the dream*/
/*C_0E1A_135B*/C_0E1A_135B(register int16 npc) {
	if(GameFlags & 0x10)
		return 0;
	if(npc > 0x79)
		return OSI_rand(0, 7);
	return (PortraitBgs[npc >> 1] >> ((npc & 1) ? 0 : 4)) & 0xf;
}

/*draw the portrait of NPC objNum at screen x,y + 2: a face of mdfaces.lzc
  over its background (C_0E1A_135B, mdback.lzc). NPCs 10-15 use 0x51's;
  the avatar (0, 1) has face 1, or 0 for a woman (PlayerIsFemale); NPC 6 with
  TalkFlags[6] & 2 is 0x16; NPCs with ObjStatus & 1 are 0x72/0x73 (by
  PlayerIsFemale); NPC 0x21 without TalkFlags[0x21] & 4 is 0x61; then some NPCs
  share faces. Nothing is drawn for NPCs 0xbf and up (the test for a
  temporary NPC of type 0x179 behind it cannot succeed)*/
/*C_0E1A_13A8*/DrawPortrait(register int16 objNum, int16 x, int16 y) {
	register int16 bg;

	if(objNum >= 10 && objNum <= 15)
		objNum = 0x51;
	if(objNum < 0xbf) {
		if(objNum >= 0xc0) {
			if(GetType(objNum) == 0x179)
				objNum = 0x1d;
		} else {
			if(objNum <= 1)
				objNum = PlayerIsFemale ? 0 : 1;
			else if(objNum == 6 && (TalkFlags[objNum] & 2))
				objNum = 0x16;
			else if(ObjStatus[objNum] & 1)
				objNum = PlayerIsFemale ? 0x73 : 0x72;
			else if(objNum == 0x21 && !(TalkFlags[0x21] & 4))
				objNum = 0x61;
			else
				objNum = objNum;/*(a statement the compiler drops: it leaves the jmp before it)*/
			bg = C_0E1A_135B(objNum);
			if(objNum == 0x15 || (objNum >= 0x47 && objNum <= 0x4a) || objNum == 0x4e)
				objNum = 0x14;
			else if(objNum >= 0x4b && objNum <= 0x4d)
				objNum = 0x16;
			else if(objNum == 0x1e)
				objNum = 0x18;
			else if(objNum == 0x46)
				objNum = 0x40;
			else if(objNum == 0x50)
				objNum = 0x2a;
			else if(objNum == 0x51)
				objNum = 0x2b;
			else if(objNum == 0x52)
				objNum = 0x2c;
			else if(objNum == 0x53)
				objNum = 0x2d;
			else if(objNum == 0x54)
				objNum = 0x2e;
			else if(objNum == 0x55)
				objNum = 0x2f;
			else if(objNum == 0x56)
				objNum = 0x30;
			else if(objNum == 0x57)
				objNum = 0x31;
			else if(objNum == 0x5e)
				objNum = 6;
			else if(objNum == 0x68 || objNum == 0x77 || objNum == 0x78)
				objNum = 0x67;
			else if(objNum == 0x69)
				objNum = 0x19;
			else if(objNum == 0x6d)
				objNum = 2;
			LZC_loadType(BkgrndFile, bg, D_54B2, 0, D_3E80);
			GR_DRAW_BITMAP(&D_3232, x + 1, y + 2, D_54B2, 0);
			LZC_loadType(FacesFile, objNum, D_54B2, 0, D_3E80);
			GR_DRAW_BITMAP(&D_3232, x, y + 2, D_54B2, 0);
		}
	}
}

/*draw the status area background (mdscreen.lzc entry 1) at 184,2*/
/*C_0E1A_158A*/C_0E1A_158A() {
	LZC_loadType("mdscreen.lzc", 1, D_54B2, 0, D_3E80);
	GR_DRAW_BITMAP(&D_3232, 0xb8, 2, D_54B2, 0);
}

/*show picture n (1-) of mdream.lzc in the status area, over a
  ColorPaper background with its two corner tiles*/
/*C_0E1A_15D1*/C_0E1A_15D1(unsigned char n) {
	register int16 mouseState;

	mouseState = CON_mouseOffAt(0xb8, 2, 0x137, 0x77);
	GR_SET_COLOR(ColorPaper);
	GR_FILL_RECT(0xb8, 2, 0x137, 0x77);
	GR_PUT_TILE_SCREEN(ColorPaper == 7 ? 0x12d : 0x124, 0xb8, 0);
	GR_PUT_TILE_SCREEN(ColorPaper == 7 ? 0x12e : 0x125, 0x128, 0);
	if(LZC_loadType(DreamFile, n - 1, D_54B2, 0, D_3E80))
		GR_DRAW_BITMAP(&D_3232, 0xb8, 2, D_54B2, 0);
	CON_mouseOn(mouseState);
}

/*the number of NPC objNum's face and conversation (entry + 1 of
  mdfaces.lzc, entry of talk.lzc): NPCs 10-15 use 0x51's, temporary NPCs
  (0xc0 and up) of type 0x179 0x1d; 0 if none (0xbf, other temporary NPCs)*/
/*C_0E1A_16AA*/C_0E1A_16AA(register int16 objNum) {
	register int16 objType;

	if(objNum >= 10 && objNum <= 15)
		objNum = 0x51;
	objType = GetType(objNum);
	if(objNum >= 0xc0) {
		switch(objType) {
			case 0x179: return 0x1d;
			default: return 0;
		}
	} else if(objNum >= 0xbf) {
		return 0;
	}
	return objNum;
}

/*load the conversation script of NPC objNum (an entry of talk.lzc) into
  dest. returns objNum, or 0 if the NPC has no conversation or the script
  is too big (message C_2BB4_22F1)*/
/*C_0E1A_16F2*/LoadConversation(int16 objNum, void far *dest) {
	/*0B53*/static char loadName[] = "talk.lzc";
	register int16 convIndex;

	if(!(convIndex = C_0E1A_16AA(objNum)))
		return 0;
	if(LIB_entrySize(loadName, convIndex) >= 0x36b0L) {
		CON_printf(C_2BB4_22F1);
		return 0;
	}
	LZC_loadType(loadName, convIndex, dest, 0, D_3E80);
	if(osiError)
		return 0;
	return objNum;
}

/*returns objNum if NPC objNum has a conversation script (a non empty
  entry of talk.lzc, smaller than 0x36b0 bytes), else 0*/
/*C_0E1A_175B*/HasConversation(int16 objNum) {
	/*0B5C*/static char checkName[] = "talk.lzc";
	register int16 convIndex;
	int32 size;

	if(!(convIndex = C_0E1A_16AA(objNum)))
		return 0;
	size = LIB_entrySize(checkName, convIndex);
	if(size >= 0x36b0L) {
		CON_printf(C_2BB4_22F1);
		return 0;
	}
	if(size <= 0 || osiError)
		return 0;
	return objNum;
}

/*allocate the paper doll buffer (D_BA78): the biggest of mdinv.lzc's
  entries 0-0x12; the game stops if there is not enough memory*/
/*C_0E1A_17C3*/C_0E1A_17C3() {
	register int16 i;
	int32 size, max;

	max = 0;
	for(i = 0; i <= 0x12; i ++) {
		size = LIB_entrySize("mdinv.lzc", i);
		if(size > max)
			max = size;
	}
	D_BA78 = MEM_alloc(max, 0);
	if(D_BA78 == 0) {
		CON_printf("Insufficient memory.  See your Ref Card.\n");
		CON_WaitKeyOrClick();
		GR_SHUTDOWN();
		ShutdownSound();
		MEM_shutdown(8);
		exit(1);
	}
	D_BA7C = -1;
}

/*the paper doll at screen x,y: four frame tiles (following ColorPaper),
  then the picture of mdinv.lzc for NPC objNum's type (0x126: 1, 0x127: 2,
  0x156-0x166: 0-0x10, else 0x11), loaded once in D_BA78*/
/*C_0E1A_186B*/DrawPaperDoll(int16 x, register int16 y, int16 objNum) {
	register int16 n;

	GR_PUT_TILE_SCREEN(ColorPaper == 7 ? 0x129 : 0x113, x, y);
	GR_PUT_TILE_SCREEN(ColorPaper == 7 ? 0x12a : 0x114, x + 16, y);
	GR_PUT_TILE_SCREEN(ColorPaper == 7 ? 0x12b : 0x115, x, y + 16);
	GR_PUT_TILE_SCREEN(ColorPaper == 7 ? 0x12c : 0x116, x + 16, y + 16);
	if((n = GetType(objNum)) == 0x126)
		n = 1;
	else if(n == 0x127)
		n = 2;
	else if(n < 0x156 || n > 0x166)
		n = 0x11;
	else
		n = n - 0x156;
	if(n != D_BA7C) {
		D_BA7C = -1;
		if(LZC_loadType("mdinv.lzc", n, D_BA78, 0, D_3E80))
			D_BA7C = n;
	}
	if(D_BA7C != -1)
		GR_DRAW_BITMAP(&D_3232, x + 0xd, y + 2, D_BA78, 0);
}

/*peer: draw one map tile as a 4x4 dot at x,y:
  tileNum < 0 = an NPC (white with green corners), 0xff = black,
  damaging = red, impassable = blue (wet) or light green, other terrain
  (< 0x100) = green; passable object tiles are not drawn*/
/*C_0E1A_199C*/static peer_tile(int16 tileNum, int16 x, int16 y) {
	register int16 color;
	int16 i;
	register int16 j;

	if(tileNum < 0) {
		color = 15;
	} else if(tileNum == 0xff) {
		color = 0;
	} else if(TerrainType[tileNum] & 8) {
		color = ColorRed;
	} else if(TerrainType[tileNum] & 2) {
		if(TerrainType[tileNum] & 1)
			color = ColorBlue;
		else
			color = ColorLightGreen;
	} else {
		color = -1;
	}
	if(color < 0 && tileNum >= 0x100)
		return;
	if(color < 0)
		color = ColorGreen;
	GR_SET_COLOR(color);
	for(i = 0; i < 4; i ++) {
		for(j = 0; j < 4; j ++)
			GR_PUT_PIXEL(i + x, j + y);
	}
	if(color == 15) {
		GR_SET_COLOR(ColorGreen);
		GR_PUT_PIXEL(x, y);
		GR_PUT_PIXEL(x + 3, y);
		GR_PUT_PIXEL(x, y + 3);
		GR_PUT_PIXEL(x + 3, y + 3);
	}
}

/*peer:
  overview of the whole loaded 40x40 area, 4x4 pixels per tile, drawn
  over the map window. Underground only the tiles reachable from the party
  (AreaFlags bit 0x80, computed by VIS_FloodFill) are shown. The party is a
  white square. Waits for a key or click (without animations), then
  recomputes the view*/
/*C_0E1A_1AB2*/PeerMap() {
	int16 row, col;
	int16 rowStart, rowEnd, colStart, colEnd;
	int16 tile, objNum, mouseState, isUnderground;

	isUnderground = (MapZ > 0 && MapZ < 5);
	for(row = 0; row < AREA_H; row ++) {
		for(col = 0; col < AREA_W; col ++)
			AreaFlags[row][col] =
			AreaLight[row][col] = 0;
	}
	rowStart = colStart = 0;
	rowEnd = colEnd = 40;
	if(isUnderground) {
		VIS_FloodFill(ViewX + 5, ViewY + 5, 40, 2);
	}
	D_BA66->toBackBuffer = 1;
	GR_SET_COLOR(0);
	GR_FILL_RECT(8, 16, 167, 159);
	/*terrain*/
	for(row = rowStart; row < rowEnd; row ++) {
		for(col = colStart; col < colEnd; col ++) {
			tile = AreaTiles[row][col];
			if(isUnderground && !(AreaFlags[row][col] & 0x80))
				tile = 0xff;
			peer_tile(tile, (col << 2) + 8, (row << 2) + 8);
		}
	}
	/*objects (NPC #0 only if under player control)*/
	for(objNum = SearchArea(AreaX, AreaY, (AreaX + (AREA_W-1)) & 0x3ff, (AreaY + (AREA_H-1)) & 0x3ff); objNum >= 0; objNum = NextArea()) {
		if(objNum == 0 && (NPCStatus[objNum] & 0x80) == 0)
			continue;
		col = WrapDelta(GetX(objNum) - AreaX);
		row = WrapDelta(GetY(objNum) - AreaY);
		if(isUnderground && (AreaFlags[row][col] & 0x80) == 0)
			continue;
		if(objNum < 0x100 && objNum >= 0) {
			peer_tile(-1, (col << 2) + 8, (row << 2) + 8);
		} else {
			tile = TILE_FRAME(objNum);
			peer_tile(tile, (col << 2) + 8, (row << 2) + 8);
			/*multi-tile objects extend to the left/up*/
			if((TileFlag[tile] & 0x80) && col) {
				if((TileFlag[tile] & 0x40) && row) {
					peer_tile(tile - 2, (col << 2) + 8, (row << 2) + 4);
					peer_tile(tile - 3, (col << 2) + 4, (row << 2) + 4);
				}
				peer_tile(tile - 1, (col << 2) + 4, (row << 2) + 8);
			} else if((TileFlag[tile] & 0x40) && row) {
				peer_tile(tile - 1, (col << 2) + 8, (row << 2) + 4);
			}
		}
	}
	/*the party (center of the view)*/
	GR_SET_COLOR(15);
	GR_FILL_RECT((ViewX << 2) + 28, (ViewY << 2) + 28, (ViewX << 2) + 31, (ViewY << 2) + 31);
	GR_SET_COLOR(0);
	D_BA66->toBackBuffer = 0;
	mouseState = CON_mouseOffAt(8, 16, 167, 159);
	GR_FADE_TO_SCREEN(8, 16, 167, 159, 8, 16);
	CON_mouseOn(mouseState);
	AnimateAtPrompt = 0;
	CON_WaitKeyOrClick();
	AnimateAtPrompt = 1;
	ScreenFade = 1;
	VIS_UpdateView();
}

/*fill the map window with ColorPaper and show it (fading in if fade)*/
/*C_0E1A_1E2B*/ClearMapWindow(int16 fade) {
	int16 mouseState;

	mouseState = CON_mouseOffAt(8, 16, 167, 159);
	D_BA66->toBackBuffer = 1;
	GR_SET_COLOR(ColorPaper);
	GR_FILL_RECT(8, 16, 167, 159);
	if(fade)
		GR_FADE_TO_SCREEN(8, 16, 167, 159, 8, 16);
	else
		GR_COPY_TO_SCREEN(8, 16, 167, 159, 8, 16);
	D_BA66->toBackBuffer = 0;
	CON_mouseOn(mouseState);
}
