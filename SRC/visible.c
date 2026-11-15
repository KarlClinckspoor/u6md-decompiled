/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE segment 1157: what part of the area is visible
 * (Ultima VI's visible.c, by way of Savage Empire's: SE GAME.EXE 09DA).
 *
 * VIS_UpdateView builds the 11x11 ground tiles shown around the party
 * (Tile_11x11): it marks in the area flags (AreaFlags) what the objects of
 * the area do to sight and light, then runs a breadth-first flood fill
 * (VIS_FloodFill) from the party's square and from every visible light
 * source, keeping the brightest light in AreaLight. Squares never reached are
 * drawn as tile 0xff (hidden), reached but unlit ones as tile 0x10c
 * (darkness), and wall tiles seen from one side only are replaced by a
 * variant showing only their visible faces.
 *
 * Area flag bits (per area square):
 *   0x03 light level of the brightest object on the square
 *   0x04 a "background" object is there (does not block sight)
 *   0x08 a wall object is there
 *   0x10 a window object is there
 *   0x20 an opaque object is there
 *   0x40 already queued by the current flood fill
 *   0x80 visible from the party
 *
 * Differences from Savage Empire's: map coordinates wrap (& 0x3ff, distances
 * by MapDistance); no VIS_IsInViewSquare; a square keeps the
 * brightest light reaching it instead of the sum, and a light source of
 * level n spreads n + 1 squares; XRayView, or the active member's third
 * talent (Talents bits 8-11), shows the whole view lit and unhidden; objects
 * of type 0x184 near the party (within 1 square) are set to frame 0 with a
 * random quality 6-10 (sound 0x22 when they change), far ones count their
 * quality down and then turn to frame 1; no type 0x0b light exception; the
 * wall shapes exist only for wall tiles 0xa0-0xa5 (0x150), 0xb0-0xb2 (0x160,
 * replaced by 0x150 unless S and E and SE are visible) and 0xe0-0xe5
 * (0x140), tiles 0x28-0x2d are never replaced, nor a wall whose 4 sides and
 * SE square are visible; WallShapeOfs differs.
 */
#include "se.h"

#define AREA_W 40
#define AREA_H 40

/*other modules*/
extern char AmbientLight;/*sun light level 0..7*/
extern unsigned char Active;
extern int16 MapX;
extern int16 MapY;
extern struct { unsigned a:4, psychic:4, c:4, d:4; } Talents[];
/*3E84*/extern unsigned char AreaFlags[AREA_H][AREA_W];
extern unsigned char AreaTiles[AREA_H][AREA_W];/*the ground tiles of the area*/
extern unsigned char Party[];
extern int16 Amount[];/*quantity (low byte), quality (high byte)*/
extern unsigned char far *D_6DBA;/*TileFlag: 0x03 light, 0x04 opaque, 0x08 window, 0x40 double height, 0x80 double width*/
extern int16 far *D_6EBE;/*BaseTile: first tile of each object type*/
/*6FC6*/extern unsigned char AreaLight[AREA_H][AREA_W];
extern unsigned char far *D_760A;/*TileFlag2: 0x20 background*/
extern struct coord ObjPos[];
extern uint16 ObjShapeType[];
extern unsigned char far *D_B4C4;/*TerrainType: 0x0f kind (0x04 wall), 0x10 W, 0x20 S, 0x40 E, 0x80 N*/
/*BA82*/extern int16 AreaOffset;
/*BA84*/extern unsigned char VisQueueX[256];/*flood fill ring-buffer queue, x*/
/*BB84*/extern unsigned char VisQueueY[256];/*flood fill ring-buffer queue, y*/
/*C4AA*/extern int16 Tile_11x11[11][11];/*the tiles of the map window*/

extern int16 MapDistance(int16, int16);/*distance of two map coordinates (wrapping)*/
extern MUS_PlaySfx(int16);/*play a sound effect?*/
extern int16 SearchArea(int16, int16, int16, int16);/*SearchArea(x1, y1, x2, y2): first object in the rectangle*/
extern int16 NextArea(void);/*next object of SearchArea*/
extern ShowObjects(void);/*draw the map window*/
extern int16 OSI_rand(int16, int16);/*OSI_rand(lo, hi)*/

#define TileFlag D_6DBA
#define TileFlag2 D_760A
#define TerrainType D_B4C4
#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define TILE_FRAME(i) (D_6EBE[GetType(i)] + GetFrame(i))

/*0EA0*/int16 ViewX = 12;/*the 11x11 view's corner in the area*/
/*0EA2*/int16 ViewY = 12;
/*0EA4*/int16 XRayView = 0;/*all of the view shown and lit*/

/*rounded euclidean distance: DistanceTable[|dx|][|dy|] ~ sqrt(dx*dx + dy*dy)*/
/*0EA6*/static char DistanceTable[][8] = {
	{0,1,2,3,4,5,6,7},
	{1,1,2,3,4,5,6,7},
	{2,2,3,4,5,6,6,7},
	{3,3,4,4,5,6,7,8},
	{4,4,5,5,6,7,7,8},
	{5,5,6,6,7,7,8,9},
	{6,6,6,7,7,8,8,9},
	{7,7,7,8,8,9,9,10}
};

/*flood fill queue (ring buffer of 256 in VisQueueX/VisQueueY)*/
/*0EE6*/static unsigned char visited_w = 0;
/*0EE7*/static unsigned char visited_r = 0;

/*cumulative steps that walk around the 8 neighbours of a square,
  starting from the center: N, NE, E, SE, S, SW, W, NW*/
/*0EE8*/static char NeighbourStepX[] = {      0, 1,      0,      0, -1, -1,       0,       0};
/*0EF0*/static char NeighbourStepY[] = {-     1, 0,      1,      1,  0,  0, -     1, -     1};
/*0EF8*/static char NeighbourStepOfs[] = {-AREA_W, 1, AREA_W, AREA_W, -1, -1, -AREA_W, -AREA_W};

/*wall shapes: tile offset from the wall group's base, indexed by the
  wall's connections restricted to the visible sides
  (bit 8 = N, 4 = E, 2 = S, 1 = W); -1: keep the tile*/
/*0F00*/static char WallShapeOfs[] = {
	-1,/*----*/
	-1,/*---W*/
	-1,/*--S-*/
	 2,/*--SW*/
	-1,/*-E--*/
	 1,/*-E-W*/
	 0,/*-ES-*/
	 0,/*-ESW*/
	-1,/*N---*/
	 4,/*N--W*/
	 3,/*N-S-*/
	 3,/*N-SW*/
	 5,/*NE--*/
	 4,/*NE-W*/
	 0,/*NES-*/
	-1 /*NESW*/
};

/*can sight/light go through the square AreaOffset (x, y are its area
  coordinates)? opaque objects block, windows are only see-through
  from an orthogonally adjacent square of the party, opaque ground
  tiles block unless a background object is on them*/
static /*C_1157_000A*/VIS_IsSeeThrough(int16 x, int16 y) {
	int16 flag, tile;

	tile = AreaTiles[0][AreaOffset];
	flag = AreaFlags[0][AreaOffset];
	if(flag & 0x20)
		return 0;
	if((TileFlag[tile] & 0x08) || (flag & 0x10)) {
		return
			(((ViewX + 5) & 0x3ff) == x && MapDistance(ViewY + 5, y) < 2) ||
			(((ViewY + 5) & 0x3ff) == y && MapDistance(ViewX + 5, x) < 2)
		;
	}
	if(flag & 0x04)
		return 1;
	if(TileFlag[tile] & 0x04)
		return 0;
	return 1;
}

/*queue area square (x, y) for the flood fill and mark it 0x40
  (the queue silently drops squares when full)*/
static /*C_1157_00A6*/VIS_Enqueue(int16 x, int16 y) {
	if(x >= 0 && x < AREA_W && y >= 0 && y < AREA_H) {
		VisQueueX[visited_w] = x;
		VisQueueY[visited_w] = y;
		if(++visited_w == visited_r)
			visited_w --;
		else
			AreaFlags[y][x] |= 0x40;
	}
}

/*flood fill from area square (x, y): every square reached gets
  light = clamp(4 - distance + lightBase, 0..4) if brighter than its own;
  the fill spreads from see-through squares to their neighbours.
  mode 1: party's view (marks squares visible 0x80, limited to the
          11x11 view plus a 1-square border),
  mode 2: "peer" (marks visible, whole area),
  mode 0: light source (spreads up to distance lightBase + 4).
  If (x, y) itself blocks sight (e.g. a torch on a wall), the fill
  starts from its east and south neighbours instead.*/
/*C_1157_0118*/VIS_FloodFill(int16 x, int16 y, int16 lightBase, int16 mode) {
	int16 curX, curY;
	int16 dir, light;

	/*AreaOffset = y * AREA_W + x*/
	AreaOffset = y << 3;
	AreaOffset += AreaOffset << 2;
	AreaOffset += x;
	if(VIS_IsSeeThrough(x, y)) {
		VIS_Enqueue(x, y);
	} else {
		AreaOffset ++;
		if(VIS_IsSeeThrough(x + 1, y))
			VIS_Enqueue(x + 1, y);
		AreaOffset += (AREA_W-1);
		if(VIS_IsSeeThrough(x, y + 1))
			VIS_Enqueue(x, y + 1);
	}
	while(visited_w != visited_r) {
		curX = VisQueueX[visited_r];
		curY = VisQueueY[visited_r];
		visited_r ++;
		AreaOffset = curY << 3;
		AreaOffset += AreaOffset << 2;
		AreaOffset += curX;

		light = 4 - DistanceTable[MapDistance(curX, x)][MapDistance(curY, y)] + lightBase;
		if(light < 0)
			light = 0;
		else if(light > 4)
			light = 4;
		if(AreaLight[0][AreaOffset] < light)
			AreaLight[0][AreaOffset] = light;

		if(mode)
			AreaFlags[0][AreaOffset] |= 0x80;
		if(VIS_IsSeeThrough(curX, curY)) {
			for(dir = 0; dir < 8; dir ++) {
				curX = curX + NeighbourStepX[dir];
				curY = curY + NeighbourStepY[dir];
				AreaOffset += NeighbourStepOfs[dir];
				if(!(AreaFlags[0][AreaOffset] & 0x40)) {
					if(mode == 1) {
						if(
							curX - ViewX >= -1 && curX - ViewX <= 11 &&
							curY - ViewY >= -1 && curY - ViewY <= 11
						) VIS_Enqueue(curX, curY);
					} else if(mode == 2) {/*for peer*/
						VIS_Enqueue(curX, curY);
					} else if(DistanceTable[MapDistance(curX, x)][MapDistance(curY, y)] <= lightBase + 4) {
						VIS_Enqueue(curX, curY);
					}
				}
			}
		}
	}
}

/*objects of type 0x184 next to the party go to frame 0 with a quality
  of 6-10; away from it, they count their quality down, then go to frame 1*/
#define UPDATE_0184(objNum, q) \
		if(GetType(objNum) == 0x184) {\
			if(MapDistance(GetX(objNum), MapX) <= 1 && MapDistance(GetY(objNum), MapY) <= 1) {\
				if(GetFrame(objNum))\
					MUS_PlaySfx(0x22);\
				ObjShapeType[objNum] = GetType(objNum);\
				GetQual(objNum) = OSI_rand(6, 10);\
			} else if(GetFrame(objNum) == 0) {\
				if(q = GetQual(objNum))\
					GetQual(objNum) = q - 1;\
				else\
					ObjShapeType[objNum] = GetType(objNum) + 0x400;\
			}\
		}

/*compute the visible 11x11 tiles (Tile_11x11) and the light of the
  area around the party, then redraw the map window*/
/*C_1157_0312*/VIS_UpdateView() {
	unsigned char *pTiles,*pLight;
	int16 *pView;
	int16 cellX,x,y;
	int16 tile,terrain,wallBase,visibleSides,wallSides,nbTile,nbTerrain,objNum;
	int16 light,radius;
	register unsigned char *pFlags;
	register int16 cellY;

	if(XRayView || Talents[Party[Active]].c) {
		/*everything visible and lit*/
		for(cellX = 8; cellX < 32; cellX ++) {
			for(cellY = 8; cellY < 32; cellY ++) {
				AreaLight[cellX][cellY] = 4;
				AreaFlags[cellX][cellY] = 3;
			}
		}
		for(cellY = 0; cellY < 11; cellY ++) {
			for(cellX = 0; cellX < 11; cellX ++) {
				Tile_11x11[cellY][cellX] = AreaTiles[cellY + ViewY][cellX + ViewX];
				AreaFlags[cellY + ViewY][cellX + ViewX] |= 0x80;
			}
		}
		for(
			objNum = SearchArea((MapX - 15) & 0x3ff, (MapY - 15) & 0x3ff, (MapX + 15) & 0x3ff, (MapY + 15) & 0x3ff);
			objNum >= 0;
			objNum = NextArea()
		) {
			UPDATE_0184(objNum, radius)
		}
	} else {
		/*clear flags and light of the area*/
		for(cellX = 0; cellX < AREA_W; cellX ++) {
			for(cellY = 0; cellY < AREA_H; cellY ++) {
				AreaFlags[cellY][cellX] =
				AreaLight[cellY][cellX] = 0;
			}
		}
		/*cellX, cellY = area origin*/
		cellX = ((MapX - 0x10) & 0x3ff) & 0xfff8;
		cellY = ((MapY - 0x10) & 0x3ff) & 0xfff8;
		/*flag what the objects of the central squares do to sight and light*/
		for(
			objNum = SearchArea((cellX + 8) & 0x3ff, (cellY + 8) & 0x3ff, (cellX + 31) & 0x3ff, (cellY + 31) & 0x3ff);
			objNum >= 0;
			objNum = NextArea()
		) {
			x = (GetX(objNum) - cellX) & 0x3ff;
			y = (GetY(objNum) - cellY) & 0x3ff;
			pFlags = &(AreaFlags[y][x]);
			UPDATE_0184(objNum, radius)
			tile = TILE_FRAME(objNum);

			if(TileFlag[tile] & 0x08)
				*pFlags |= 0x10;
			else if(TileFlag[tile] & 0x04)
				*pFlags |= 0x20;
			else if(TileFlag2[tile] & 0x20)
				*pFlags |= 0x04;

			/*double-size objects also cover the square above/left*/
			if(TileFlag[tile - 1] & 0x04) {
				if((TileFlag[tile] & 0x40) && y)
					*(pFlags - AREA_W) |= 0x20;
				if((TileFlag[tile] & 0x80) && x)
					*(pFlags - 1) |= 0x20;
			}

			if(TerrainType[tile] & 0x04) {
				*pFlags |= 0x08;
				if((TileFlag[tile] & 0x40) && y)
					*(pFlags - AREA_W) |= 0x08;
				if((TileFlag[tile] & 0x80) && x)
					*(pFlags - 1) |= 0x08;
			}

			if(light = TileFlag[tile] & 0x03) {
				if((*pFlags & 0x03) < light)
					*pFlags = (*pFlags & ~0x03) | light;
			}
		}

		/*what the party sees, lit by the ambient light*/
		VIS_FloodFill((ViewX + 5) & 0x3ff, (ViewY + 5) & 0x3ff, AmbientLight, 1);

		/*every visible light source adds its own light*/
		if(AmbientLight < 7) {
			pFlags = &(AreaFlags[8][8]);
			pTiles = &(AreaTiles[8][8]);
			for(cellY = 8; cellY < 32; cellY ++) {
				for(cellX = 8; cellX < 32; cellX ++) {
					if((*pFlags & 0x03) && (*pFlags & 0x80)) {
						/*a light on a wall only counts if the square south of it is visible*/
						if(
							(TerrainType[*pTiles] & 0x04) &&
							(!(*(pFlags + AREA_W) & 0x80) || !(*(pFlags + AREA_W) & 0x80))/*twice, as in U6*/
						) continue;
						/*forget the "queued" marks around it*/
						radius = (*pFlags & 0x03) + 1;
						for(x = cellX - radius; x <= cellX + radius; x ++) {
							for(y = cellY - radius; y <= cellY + radius; y ++)
								AreaFlags[y][x] &= ~0x40;
						}
						VIS_FloodFill(cellX, cellY, *pFlags & 0x03, 0);
					}
					pFlags ++;
					pTiles ++;
				}
				pFlags += 16;
				pTiles += 16;
			}
		}

		/*build the 11x11 view*/
		pView = &(Tile_11x11[0][0]);
		pLight = &(AreaLight[ViewY][ViewX]);
		pFlags = &(AreaFlags[ViewY][ViewX]);
		pTiles = &(AreaTiles[ViewY][ViewX]);
		for(cellY = 0; cellY < 11; cellY ++) {
			for(cellX = 0; cellX < 11; cellX ++) {
				x = (cellX + ViewX) & 0x3ff;
				y = (cellY + ViewY) & 0x3ff;
				if(!(*pFlags & 0x80)) {
					tile = 0xff;/*hidden*/
				} else if(*pLight == 0) {
					tile = 0x10c;/*darkness*/
				} else {
					tile = *pTiles;
					/*a plain wall tile (wall + impassable) not seen from all
					  4 sides is replaced by the wall shape showing only the
					  sides that are visible*/
					terrain = TerrainType[tile] & 0x0f;
					if(terrain == 6 && (tile < 0x28 || tile > 0x2d)) {
						if(tile >= 0xa0 && tile <= 0xa5)
							wallBase = 0x150;
						else if(tile >= 0xb0 && tile <= 0xb2)
							wallBase = 0x160;
						else if(tile >= 0xe0 && tile <= 0xe5)
							wallBase = 0x140;
						else
							wallBase = 0;
						/*which neighbours are visible (8 N, 4 E, 2 S, 1 W)*/
						visibleSides = 0;
						if(*(pFlags - AREA_W) & 0x80)
							visibleSides |= 8;
						if(*(pFlags + 1) & 0x80)
							visibleSides |= 4;
						if(*(pFlags + AREA_W) & 0x80)
							visibleSides |= 2;
						if(*(pFlags - 1) & 0x80)
							visibleSides |= 1;
						if(wallBase == 0x160 && (!(visibleSides & 2) || !(visibleSides & 4) || !(*(pFlags + AREA_W + 1) & 0x80)))
							wallBase = 0x150;
						if(wallBase != 0 && (visibleSides != 0xf || !(*(pFlags + AREA_W + 1) & 0x80))) {
							/*the wall's connections: upper nibble of TerrainType*/
							wallSides = TerrainType[tile] >> 4;
							/*with bit 4, the N connection is dropped unless the
							  north square has a wall object or is a wall tile
							  connected south*/
							if(wallSides & 4) {
								nbTile = *(pTiles - AREA_W);
								nbTerrain = TerrainType[nbTile] & 0x0f;
								if(
									!(*(pFlags - AREA_W) & 0x08) &&
									(!(nbTerrain & 0x04) || !(TerrainType[nbTile] & 0x20))
								) wallSides &= ~8;
							}
							/*with bit 2, the same for the W connection with the
							  west square, connected east*/
							if(wallSides & 2) {
								nbTile = *(pTiles - 1);
								nbTerrain = TerrainType[nbTile] & 0x0f;
								if(
									!(*(pFlags - 1) & 0x08) &&
									(!(nbTerrain & 0x04) || !(TerrainType[nbTile] & 0x40))
								) wallSides &= ~1;
							}
							wallSides &= visibleSides;
							if(WallShapeOfs[wallSides] >= 0)
								tile = wallBase + WallShapeOfs[wallSides];
						}
					}
				}
				*pView++ = tile;
				pFlags ++;
				pTiles ++;
				pLight ++;
			}
			pFlags += 29;/*AREA_W - 11*/
			pTiles += 29;
			pLight += 29;
		}
	}
	ShowObjects();
}
