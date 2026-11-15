/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE segment 2238: the console and user input
 * (Ultima VI's console.c, by way of Savage Empire's: SE GAME.EXE 05D0).
 *
 * - Text output: a small "console" driver writing 8x8 characters into
 *   rectangular text windows (struct tClipInfo; CurClip is the current one,
 *   TextWindows[] holds the game's windows: [0] full screen, [1] status
 *   panel, [3] message scroll area). Characters are collected in a line
 *   buffer (LineBufChars / LineBufColors) and drawn one line at a time,
 *   which allows word wrap, centering, scrolling and "more" pagination
 *   according to the window's attrFlags:
 *     0x01 word wrap        0x02 center lines
 *     0x04 '`' capitalizes  0x08 runic font (char | 0x80)
 *     0x10 paginate         0x20 scroll at bottom
 *   CON_putch interprets the in-text escapes of conversations and messages
 *   (# $ / \ ^ @ * `), CON_printf is a minimal printf.
 * - Input: CON_prompt (animated cursor + one key), CON_ReadCommand (keys
 *   and mouse clicks to command codes), CON_getch (also runs the status
 *   panel commands itself), CON_gets (line input).
 * - Mouse: pointer shape, direction from the pointer position, hit-testing
 *   of the map view, the command icons and the status panel views.
 * - The keyboard "slot cursor" of the equipment/portrait views.
 * - Game load/save/restore (savegame\objlist).
 *
 * Martian Dreams' command codes: 0x80 direction, 0x81 attack, 0x82 talk,
 * 0x83 look, 0x84 get, 0x85 drop, 0x86 move, 0x87 use (the icons, from
 * x = 16, 18 pixels each; ALT+letter makes one the default command),
 * 0x8e target, 0x8f pass, 0x90 portrait view, 0x91 party view,
 * 0x92 equipment view, 0x99 next member, 0x9a previous member,
 * 0x9b/0x9c/0x9d inventory slot / equipment slot / map object hit,
 * 0x9e conversation view, 0x9f container up, 0xa0 open container,
 * 0xa6 refresh status, 0xa7 ready, 0xa8 unready, 0xa9 default action.
 * Differences from Savage Empire's: the map coordinates wrap (& 0x3ff),
 * the view can be rotated (ViewRotation), the flag GameFlags & 0x10 shows one
 * inventory only (no party/portrait views), no disk swapping in the
 * load/save, the empty slot tiles follow the panel color, locked
 * containers, the "more" prompt waits for any key or click.
 */
#include <dos.h>
#include <stdlib.h>
#include <alloc.h>
#include <process.h>
#include <ctype.h>
#include <string.h>

typedef int int16;
typedef unsigned uint16;
typedef long int32;
typedef unsigned long uint32;

/*a far pointer seen as offset:segment, as a data pointer or as a function*/
union tHandler {
	struct {
		uint16 ofs, seg;
	} iii;
	void far *p;
	void far *(*f)();
};

/*an object's packed position: x (10 bits), y (10 bits), z (4 bits)*/
struct coord {
	unsigned char raw[3];
};

#define SCR2TIL(i) ((i) >> 4)
#define TIL2SCR(i) ((i) << 4)

/*Turbo C's variable arguments: the address of "..." (each argument a word); written so
  that Turbo C++ 1.00 steps the pointer first, as Origin's code does*/
#define VA_LIST void **
#define VA_START(ap, last) ap = ...
#define VA_INT(ap) (((int16 *)++(ap))[-1])
#define VA_CHAR(ap) (((char *)++(ap))[-2])
#define VA_STR(ap) (((char **)++(ap))[-1])

/*a text window*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;/*1 wrap, 2 center, 4 capitalize, 8 runes, 0x10 paginate, 0x20 scroll*/
};

/*a drawing surface of the mouse code (Ultima VI's)*/
struct tSurface {
	uint16 segment;/*_00*/
	uint16 far *pScanlines;/*_02*/
	uint16 right, left, top, bottom;/*_06,_08,_0a,_0c*/
};

/*the mouse state (Ultima VI's)*/
struct tMouse {
	unsigned char buttonL, buttonR;
	int16 posX, posY;
	uint16 unused06;
	void far *shapes;/*_08 pointer shapes*/
	uint16 shapeIndex;
	struct tSurface *surface;/*_0e*/
	void far *saveBuf;/*_10 background save area*/
	void (far *onMove)();/*_14*/
	unsigned char installed;
};

/*the graphic driver's state (Ultima VI's tGr)*/
struct tGr {
	unsigned char header[6];/*_00*/
	void far *font;/*_06*/
	unsigned char textFg, textBg;/*_0a,_0b*/
	unsigned char xorMode;
	unsigned char unknown0d;
	unsigned char animOn;
	unsigned char toBackBuffer;/*_0f*/
};

/*mouse (OSI assembly)*/
extern cdecl MOUSE_POINTER(void far *, int16);
extern cdecl MOUSE_SERVE();
extern cdecl MOUSE_SHOW();
extern cdecl MOUSE_HIDE();
extern int16 cdecl MOUSE_IN_AREA(struct tSurface *);
/*OSI file routines (asm)*/
extern int16 OSI_open(char *);
extern OSI_close(int16);
extern int16 OSI_create(char *);
extern int32 OSI_read(int16, int32, int32, void far *);
extern int32 OSI_write(int16, int32, int32, void far *);
extern int16 KBD_GetKey(int16);

/*main.c*/
extern PaletteAnimation();
extern char DirIncrX[8], DirIncrY[8];
extern int16 GrAdapter;
extern unsigned char MouseOn;
/*other game modules*/
extern BargeFindLeader(int16);
extern DrawMapWindow(int16);/*DrawMapWindow?*/
extern DisplayTick();
extern PassTime(int16);
extern RefreshStatus();/*RefreshStatus?*/
extern MAP_LoadChunkIndexes(int16, int16, int16);
extern MAP_InitChunkCache();
extern MAP_LoadAround();
extern int16 MapDistance(int16, int16);/*distance between two map coordinates (wrapping)*/
extern VIS_UpdateView();
extern int16 OBJ_MakeDirFrame(int16, int16);/*OBJ_MakeDirFrame(type, direction)*/
extern int16 OBJ_FindTypeAt(int16, int16, int16);
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);
extern DeleteObj(int16);
extern OBJ_DeleteTmpBlocks();
extern LoadNewRegions(int16, int16, int16);
extern OBJ_CommitBlocks();
extern OBJ_InitLists();
extern OBJ_RelinkNPCs();
extern int16 IsContainerType(int16);
extern int16 QuanType(int16);/*4 quantity on 16 bits, 2 on 8 bits, else 0*/
extern STAT_refreshComMode();
extern STAT_RefreshInventory(int16);
extern int16 STAT_PickInvenSlot();
extern int16 STAT_PickEquipSlot();
extern STAT_Ready();
extern STAT_Unready();
extern InitNPCTracker();
extern int16 COMBAT_TileDistance(int16, int16, int16);/*COMBAT_TileDistance(objNum, x, y)*/
extern COMBAT_breakOff();
extern int16 COMBAT_GetTargetAt(int16, int16, int16);/*COMBAT_GetTargetAt(objNum, x, y)*/
extern MUS_PlayAreaSong();
extern MUS_PlaySfx(int16);/*play a sound effect?*/
extern int16 IsWaterTile(int16);


extern int16 ScreenFade;
extern int16 ColorRed;
extern int16 ColorPaper;
extern int16 ColorText;
extern int16 D_082E;/*default paper color*/
extern char D_0F12;/*StatusDirty*/
extern char D_0F13;/*PromptFlag?*/
extern char D_0F15;/*IsArmageddon?*/
extern int16 D_0F16;/*IdleAnimTick*/
extern int16 D_0F18;/*MapAnimEnabled*/
extern int16 BuiltAreaX, BuiltAreaY, BuiltAreaZ;/*BuiltAreaX, BuiltAreaY, BuiltAreaZ (map)*/
extern int16 HatchingDisabled;
extern int16 AdvanceBlocked;
extern int16 ObjBlkTmpPending[0x45];
extern int16 MoveAppliesTerrain;
extern unsigned char IsOnQuest;/*first byte of the saved game state*/
extern unsigned char Active;
extern int16 MapX, MapY, MapZ;
extern char InCombat;
extern unsigned char DefaultCommand;
extern int32 GameFlags;
extern int16 ViewRotation;/*rotation of the map view (eighths of a turn)?*/
extern char GameStateEnd;
extern unsigned char PartyListTop;
extern unsigned char InvenScroll;
extern int16 DiskFullChoice;
extern unsigned char IsInConversation;
extern int16 TextIdle;
extern char D_3240[];
extern char D_3232;/*a drawing surface (main.c)*/
extern char far *D_3262;/*0x100 bytes*/
extern unsigned char ObjStatus[];
extern struct tMouse D_3E66;/*Mouse*/
extern char MovePts[];
extern char far *SchedIndex;/*0x100 bytes*/
extern char far *PathData;/*0x100 bytes*/
extern char far *NPCFlag;/*0x100 bytes*/
extern unsigned char DEXTE[];
extern unsigned char far *NPCMode;
extern unsigned char Party[];
extern unsigned char far *Level;
extern char far *Schedule;/*5 bytes each*/
extern int16 SchedPointer[0x101];
extern void far *D_50C6;/*PointerShapes*/
extern char far *Leader;/*0x100 bytes*/
extern char D_50CE[][14];/*Names*/
extern uint16 ExpPoints[];
extern unsigned char far *NPCComMode;
extern unsigned char STREN[];
extern int16 Amount[];
extern unsigned char NPCStatus[];
extern unsigned char HitPoints[];
extern int16 far *D_6EBE;/*BaseTile*/
extern unsigned char D_6EC6[];/*TalkFlags*/
extern unsigned char INTEL[];
extern int16 D_770E[];
extern struct coord ObjPos[];
extern unsigned char PartySize;
extern int16 D_9CC2;/*NumAnimData*/
extern uint16 ObjShapeType[];
extern int16 D_B4C8[];
extern int16 far *OrigShapeType;
extern struct tGr far *D_BA66;/*GrInfo*/
extern union tHandler D_BA6A;/*GrDriver*/
extern void far *D_BA7E;/*mdscreen.lzc entry 2: default command mark*/
extern unsigned char D_BC8E[];/*StateAnimData*/
extern unsigned char Obj_11x11[11][11];/*objects shown in the map window*/
extern int16 Tile_11x11[11][11];/*the tiles of the map window*/
extern unsigned char D_EAF9[8];/*Party_2Handed*/
extern int16 D_EB00;/*ShowNPCInven*/
extern int16 D_EB02;/*NPC_2Handed*/
extern int16 D_EB18;/*InvenOwner*/
extern unsigned char D_EB1C;/*PromptCnt: animated cursor frame*/
extern int16 D_EB1D;/*MouseHeldCmd: command of the mouse button held*/
extern unsigned char D_EB1F[];/*LineBufChars*/
extern unsigned char D_EB48[];/*LineBufColors*/
extern int16 D_EB72;/*AdvanceDir*/
extern int16 D_EB74, D_EB76;/*PointerX, PointerY*/
extern struct tSurface D_EB78;/*MouseArea*/
extern int16 D_EB86[10+26];/*VarInt*/
extern int16 D_EBCE;/*Selection.obj*/
extern struct tClipInfo D_EBD0[4];/*TextWindows*/
extern int16 D_EBF4;/*MouseButtons: bit 0 left, bit 1 right*/
extern int16 D_EBF6, D_EBF8;/*Selection.x, Selection.y*/
extern char *D_EBFA[10+26];/*VarStr*/
extern int16 D_EC42;/*LinesBeforeMore*/
extern int16 D_EC44;/*SelectionIsDark*/

#define StatusDirty D_0F12
#define PromptFlag D_0F13
#define IsArmageddon D_0F15
#define IdleAnimTick D_0F16
#define MapAnimEnabled D_0F18
#define Mouse D_3E66
#define PointerShapes D_50C6
#define BaseTile D_6EBE
#define TalkFlags D_6EC6
#define NumAnimData D_9CC2
#define GrInfo D_BA66
#define GrDriver D_BA6A
#define StateAnimData D_BC8E
#define Party_2Handed D_EAF9
#define ShowNPCInven D_EB00
#define NPC_2Handed D_EB02
#define InvenOwner D_EB18
#define PromptCnt D_EB1C
#define MouseHeldCmd D_EB1D
#define LineBufChars D_EB1F
#define LineBufColors D_EB48
#define AdvanceDir D_EB72
#define PointerX D_EB74
#define PointerY D_EB76
#define MouseArea D_EB78
#define VarInt D_EB86
#define SelectionObj D_EBCE
#define TextWindows D_EBD0
#define MouseButtons D_EBF4
#define SelectionX D_EBF6
#define SelectionY D_EBF8
#define VarStr D_EBFA
#define LinesBeforeMore D_EC42
#define SelectionIsDark D_EC44

#define GR_SET_COLOR(c)              (GrDriver.iii.ofs = 0x06, (*GrDriver.f)(c))
#define GR_PUT_PIXEL(x,y)            (GrDriver.iii.ofs = 0x09, (*GrDriver.f)(x,y))
#define GR_FILL_RECT(x0,y0,x1,y1)    (GrDriver.iii.ofs = 0x12, (*GrDriver.f)(x0,y0,x1,y1))
#define GR_SCROLL_RECT_Y(x0,y0,x1,y1,dy) (GrDriver.iii.ofs = 0x15, (*GrDriver.f)(x0,y0,x1,y1,dy))
#define GR_PUT_CHAR(ch,x,y)          (GrDriver.iii.ofs = 0x1e, (*GrDriver.f)(ch,x,y))
#define GR_PUT_TILE_SCREEN(tile,x,y) (GrDriver.iii.ofs = 0x2d, (*GrDriver.f)(tile,x,y))
#define GR_PUT_TILE(tile,x,y)        (GrDriver.iii.ofs = 0x42, (*GrDriver.f)(tile,x,y))
#define GR_DRAW_BITMAP(s,x,y,p,m)    (GrDriver.iii.ofs = 0x30, (*GrDriver.f)(s,x,y,p,m))
#define GR_COPY_TO_SCREEN(x0,y0,x1,y1,a,b) (GrDriver.iii.ofs = 0x45, (*GrDriver.f)(x0,y0,x1,y1,a,b))

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetAssoc(i) (*(int16 *)&(ObjPos[i]))
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)
#define IsNPC(i) ((i) < 0x100 && (i) >= 0)
#define IN_VEHICLE (Active == PartySize)
/*background tile of an empty inventory slot (by the panel's paper color)*/
#define EMPTY_SLOT_TILE (ColorPaper == 7 ? 0x126 : 0x110)

/*isalpha with the ctype table read unsigned (Turbo C++ 1.00's ctype.h has it signed)*/
#undef isalpha
#define isalpha(c) (((unsigned char *)_ctype)[(c) + 1] & (_IS_UPP | _IS_LOW))

#define LEFT_CLICK (MouseButtons & 1)
#define RIGHT_CLICK (MouseButtons & 2)

/*1B84*/unsigned char SelectMode = 0;/*0 none, 1 aiming on the map, 2 slot cursor*/
/*1B85*/char AimX = 0;
/*1B86*/char AimY = 0;
/*1B87*/char SelectRange = 0;
/*1B88*/char FreeAimMode = 0;/*free aiming (TAB pressed with no command pending)*/
/*1B89*/static unsigned char CapitalizeNext = 0;/*'`' seen: capitalize the next char (if attrFlags & 4)*/
/*1B8A*/char SlotCursorCol = 3;/*slot cursor column (0~2 equipment, 3~6 inventory; bottom row: button #)*/
/*1B8B*/char SlotCursorRow = 0;/*slot cursor row (-1 container slot, 0~3 slots, 4 buttons row)*/
/*1B8C*/static unsigned char InGetString = 0;/*CON_gets() in progress*/
/*1B8D*/unsigned char AnimateAtPrompt = 1;/*run the animations while waiting at the prompt*/
/*1B8E*/int16 LastNumberPlural = 1;/*last number printed was != 1 (for '/' and '\\'); cleared at each new line*/
/*1B90*/int16 EndPagingAtPrompt = 0;/*clear the paginate attribute (0x10) at the next prompt*/
/*1B92*/int16 HelpOn = 1;/*highlight '@' keywords*/
/*1B94*/int16 InHiddenSection = 0;/*Ultima VI's '&' section flag: only cleared here*/
/*1B96*/int16 TextOn = 1;
/*1B98*/int16 LastAttackTarget = -1;/*not used in this module*/
/*1B9A*/int16 SkipNewlines = 0;/*ignore '\n' in a scrolling window until a printable char*/
/*1B9C*/static int16 KbdKeepLastOnly = 1;/*keyboard: keep only the last key in the BIOS buffer*/
/*1B9E*/int16 GetsAborted = 0;/*last CON_gets() was aborted with ESC*/
/*1BA0*/int16 CookedInput = 1;/*0: return raw keys (the "more" prompt), 1: full command processing*/
/*1BA2*/int16 InputToUpper = 1;/*convert input keys to uppercase*/
/*1BA4*/int16 StatusMember = 0;/*party member shown in the portrait/equipment view*/
/*1BA6*/int16 MousePress = 0;
/*1BA8*/int16 MouseTrigger = 0;
/*1BAA*/int16 MouseMode = 0;
/*1BAC*/int16 StatusDisplay = 0x91;
/*1BAE*/int16 PendingCommand = 0x8f;/*command being executed (waiting for a target)*/
/*1BB0*/int16 AllowMouseMov = 0;
/*1BB2*/int16 MouseMapX = -1;
/*1BB4*/int16 MouseMapY = -1;
/*screen pixel used as origin by MkDirection() (88,88: party leader in the map view)*/
/*1BB6*/unsigned char DirOriginX = 88;
/*1BB7*/unsigned char DirOriginY = 88;
/*rotation of the pointer around DirOriginX/Y by ViewRotation eighths of a turn
  (mkMouseCommand): x' = x*RotCos - y*RotSin, y' = x*RotSin + y*RotCos*/
/*1BB8*/char RotCos[8] = {1, 1, 0, -1, -1, -1, 0, 1};
/*1BC0*/char RotSin[8] = {0, 1, 1, 1, 0, -1, -1, -1};
void CON_DrawCharCb(int16 ch, int16 x, int16 y);
void CON_ScrollUpCb(int16 x, int16 y, int16 w, int16 h);
/*1BC8*/void (*CON_PutCharFn)(int16, int16, int16) = CON_DrawCharCb;/*"put char" callback*/
/*1BCC*/void (*CON_ScrollUpFn)(int16, int16, int16, int16) = CON_ScrollUpCb;/*"scroll up" callback*/
/*1BD0*/int16 PromptCh = 5;/*first char of the animated cursor (5~8; 1 for "more")*/
/*initial text window: full 40x25 screen, white on black*/
/*1BD2*/struct tClipInfo BootClip = {
	0, 0, 40, 25,
	0, 0,
	0xf, 0,
	0
};
/*1BDB*/struct tClipInfo *CurClip = &BootClip;/*current text window*/
/*1BDD*/int16 DefaultClipAttr = 1;/*attrFlags given to new windows by CON_createClip (word wrap)*/
/*3x5 font, for the quantities over inventory tiles
  (5 rows of 3 bits, MSB first; the low bit is unused)*/
/*1BDF*/uint16 TinyFont[] = {
	0xF6DE,/*'0'*/
	0x4924,/*'1'*/
	0xE7CE,/*'2'*/
	0xE59E,/*'3'*/
	0xB792,/*'4'*/
	0xF39E,/*'5'*/
	0xF3DE,/*'6'*/
	0xE4A4,/*'7'*/
	0xF7DE,/*'8'*/
	0xF79E,/*'9'*/
	0xF7DA,/*'A'*/
	0xD75C,/*'B'*/
	0xF24E,/*'C'*/
	0xD6DC,/*'D'*/
	0xF34E,/*'E'*/
	0xF348,/*'F'*/
	0xF2DE,/*'G'*/
	0xB7DA,/*'H'*/
	0xE92E,/*'I'*/
	0x24DE,/*'J'*/
	0xB75A,/*'K'*/
	0x924E,/*'L'*/
	0xBEDA,/*'M'*/
	0x03DA,/*'N'*/
	0x03DE,/*'O'*/
	0xF7C8 /*'P'*/
};
/*x position (in the 16x16 tile) of each digit, least significant first,
  for a number of 1~4 digits: right aligned*/
/*1C13*/unsigned char TinyDigitX[][4] = {
	{ 7,0,0,0},
	{ 9,4,0,0},
	{11,7,3,0},
	{13,9,5,1}
};
/*map view tile last pointed at by the mouse (for continuous walking)*/
/*1C23*/int16 LastMouseTileX = -1;
/*1C25*/int16 LastMouseTileY = -1;
/*mouse zones {left,top,right,bottom}, see mkMouseCommand()*/
/*1C27*/int16 MouseZones[][4] = {
	{  8, 16,167,159},/*map view*/
	{ 15,163,159,195},/*command icons*/
	{184,  2,311,119},/*status panel*/
	{  0,  0,319,199} /*the whole screen*/
};
/*equipment view slots: screen x [column] and y [row][column]
(columns 0~2: equipment, 3~6: inventory)*/
/*1C47*/int16 SlotScrX[] = {184,208,232,248,264,280,296};
/*1C55*/unsigned char SlotScrY[][7] = {
	{24,16,24,32,32,32,32},
	{40,40,40,48,48,48,48},
	{56,64,56,64,64,64,64},
	{80,80,80,80,80,80,80}
};
/*tiles of the buttons row (x = 184 + 16 * i, y = 102) of the portrait and
  equipment views, to erase the slot cursor (set by DrawSlotCursor)*/
/*1C71*/int16 SlotButtonTiles[] = {0x11a,0x117,0x118,0x11b,0x11d};
/*1C7B*/int16 PendingSelect = 0;/*next CON_getch() returns 0x8e (target) at once*/
/*1C7D*/unsigned char LineBufLen = 0;/*line buffer: length (= column of the next char)*/
/*1C7E*/unsigned char LineBufDrawn = 0;/*line buffer: the chars before this column are on screen*/
/*1C7F*/unsigned char LineAcceptSpace = 1;/*0: skip spaces (at the start of a wrapped line)*/
/*1C80*/unsigned char PendingEscape = 0;/*pending CON_putch() escape char or 0*/

int16 CON_putch0(int16 ch);
int16 CON_getch(void);
int16 GAME_Load(void);
int16 cdecl CON_printf(unsigned char far *format, ...);
static int16 setDefaultCommand(int16 cmd);

/*hide the mouse pointer if it is inside the screen rectangle
  (x0,y0)-(x1,y1), before drawing there.
  returns 1 if it was hidden: pass the result to CON_mouseOn() afterwards*/
/*C_2238_0000*/CON_mouseOffAt(int16 x0, int16 y0, int16 x1, int16 y1) {
	int16 inArea;

	if(MouseOn) {
		MouseArea.left = x0;
		MouseArea.top = y0;
		MouseArea.right = x1;
		MouseArea.bottom = y1;
		inArea = MOUSE_IN_AREA(&MouseArea);
		if(MOUSE_IN_AREA(&MouseArea))
			MOUSE_HIDE();
	}
	return inArea;
}

/*show the mouse pointer again if mustShow (result of CON_mouseOffAt)*/
/*C_2238_0049*/CON_mouseOn(int16 mustShow) {
	if(MouseOn && mustShow)
		MOUSE_SHOW();
}

/*direction (0=N,1=NE,2=E ... 7=NW, -1 = none) pointed at by the screen
  pixel (x,y), relative to the origin (DirOriginX,DirOriginY).
  Near the origin (48x48 box) the 16x16 cell decides (-1 for the center cell);
  farther, the slope decides: mostly horizontal/vertical (ratio >= 2) gives
  E/W/N/S, else a diagonal.*/
/*C_2238_0062*/MkDirection(int16 x, int16 y) {
	int16 dir;
	int16 deltaX, deltaY;

	dir = -1;
	deltaX = abs(x - DirOriginX);
	deltaY = abs(y - DirOriginY);
	if(
		DirOriginX - 24 <= x && DirOriginX + 24 > x &&
		DirOriginY - 24 <= y && DirOriginY + 24 > y
	) {
		if(DirOriginX - 8 > x) {
			if(DirOriginY - 8 > y)
				dir = 7;
			else if(DirOriginY + 8 > y)
				dir = 6;
			else
				dir = 5;
		} else if(DirOriginX + 8 > x) {
			if(DirOriginY - 8 > y)
				dir = 0;
			else if(DirOriginY + 8 > y)
				dir = -1;
			else
				dir = 4;
		} else {
			if(DirOriginY - 8 > y)
				dir = 1;
			else if(DirOriginY + 8 > y)
				dir = 2;
			else
				dir = 3;
		}
	} else {
		if(deltaX > deltaY) {
			if(deltaY == 0 || (deltaX / deltaY) >= 2) {
				dir = (DirOriginX <= x)?2:6;
			}
		} else {
			if(deltaX == 0 || (deltaY / deltaX) >= 2) {
				dir = (DirOriginY <= y)?4:0;
			}
		}
		if(dir == -1)
			dir = (DirOriginX <= x)?((DirOriginY <= y)?3:1):((DirOriginY <= y)?5:7);
	}
	return dir;
}

/*mouse callback (Mouse.onMove), also called after mode changes:
update the mouse pointer shape according to its position.
shapes: 0 arrow (center of the map view, outside the active zones),
1 crosshair (MouseMode != 0, command icons, status panel),
2~9 direction arrows (2 + direction)*/
/*C_2238_01F1*/CON_UpdateMousePointer() {
	int16 dir;

	if(MouseOn == 0)
		return;
	if(MouseMode == 0) {
		if(Mouse.posX >= 8 && Mouse.posX < 168 && Mouse.posY >= 16 && Mouse.posY < 159) {
			if(
				DirOriginX - 8 <= Mouse.posX && DirOriginX + 8 > Mouse.posX &&
				DirOriginY - 8 <= Mouse.posY && DirOriginY + 8 > Mouse.posY
			) {
				MOUSE_POINTER(PointerShapes, 0);
			} else {
				dir = MkDirection(Mouse.posX, Mouse.posY);
				if(dir < 0)
					MOUSE_POINTER(PointerShapes, 1);
				else
					MOUSE_POINTER(PointerShapes, dir + 2);
			}
		} else if(Mouse.posX >= 15 && Mouse.posX <= 159 && Mouse.posY >= 163 && Mouse.posY <= 195) {
			MOUSE_POINTER(PointerShapes, 1);
		} else if(Mouse.posX >= 184 && Mouse.posX <= 311 && Mouse.posY >= 2 && Mouse.posY <= 119) {
			MOUSE_POINTER(PointerShapes, 1);
		} else {
			MOUSE_POINTER(PointerShapes, 0);
		}
	} else {
		MOUSE_POINTER(PointerShapes, 1);
	}
}

/*"put char" callback (CON_PutCharFn):
draw character ch at text cell (x,y) with the current window's colors*/
/*C_2238_02FF*/void CON_DrawCharCb(int16 ch, int16 x, int16 y) {
	int16 mouseHidden;

	x <<= 3;
	y <<= 3;
	mouseHidden = CON_mouseOffAt(x, y, x + 7, y + 7);
	GrInfo->textBg = CurClip->bgColor;
	GrInfo->textFg = CurClip->fgColor;
	GR_PUT_CHAR(ch, x, y);
	CON_mouseOn(mouseHidden);
}

/*"scroll up" callback (CON_ScrollUpFn):
scroll the text window at cell (x,y) of w x h cells up by one line (8 pixels)
and clear its last line with the background color*/
/*C_2238_0371*/void CON_ScrollUpCb(int16 x, int16 y, int16 w, int16 h) {
	int16 mouseHidden;

	x <<= 3;
	y <<= 3;
	w <<= 3;
	h <<= 3;
	mouseHidden = CON_mouseOffAt(x, y, x + w - 1, y + h - 1);
	GR_SCROLL_RECT_Y(x, y, x + w - 1, y + h - 1, -8);
	GR_SET_COLOR(CurClip->bgColor);
	GR_FILL_RECT(x, y + h - 8, x + w - 1, y + h - 1);
	CON_mouseOn(mouseHidden);
}

/*restore the game: throw away the current state and reload the saved game*/
/*C_2238_0416*/GAME_Restore() {
	int16 i;

	if(InCombat)
		COMBAT_breakOff();
	IsArmageddon = 0;
	MapAnimEnabled = 1;
	SelectMode = 0;
	StatusDirty = 1;
	DiskFullChoice = ' ';
	OBJ_DeleteTmpBlocks();
	for(i = 0; i < 0x45; i++)
		ObjBlkTmpPending[i] = 0;
	OBJ_InitLists();
	MAP_InitChunkCache();
	setDefaultCommand(0xff);
	StatusDirty = 1;
	HatchingDisabled = 1;
	GAME_Load();
	HatchingDisabled = 0;
	InitNPCTracker();
	MUS_PlayAreaSong();
	PassTime(0);
	VIS_UpdateView();
	ScreenFade = 1;
	DrawMapWindow(1);
	PromptFlag = 1;
}

/*load the game: read "schedule", "basetile" and the saved state in
"savegame\\objlist" (same layout as written by GAME_Save), then rebuild
the map around the party, the animations and the objects that depend on
the game's flags, and the status panel colors*/
/*C_2238_04B1*/GAME_Load() {
	int16 fd, i;
	struct tClipInfo *savedClip;
	int16 obj;
	int16 frame;

	BuiltAreaX = BuiltAreaY = BuiltAreaZ = -1;
	if(TalkFlags[0x76] & 0x20) {
		if(TalkFlags[0x76] & 0x40) {
			for(i = 0; i < NumAnimData; i ++)
				if(IsWaterTile(D_B4C8[i]))
					D_770E[i] --;
		}
		for(i = 0; i < NumAnimData; i ++)
			if(D_B4C8[i] >= 1 && D_B4C8[i] <= 4)
				D_770E[i] --;
		TalkFlags[0x76] &= ~0x20;
	}

	fd = OSI_open(/*1C81*/"schedule");
	OSI_read(fd, 0, (0x100 + 1) * sizeof(int16), SchedPointer);
	OSI_read(fd, -1, (int32)SchedPointer[0x100] * 5, Schedule);
	OSI_close(fd);

	fd = OSI_open(/*1C8A*/"basetile");
	OSI_read(fd, 0, 0x800, BaseTile);
	OSI_close(fd);

	fd = OSI_open(/*1C93*/"savegame\\objlist");
	OSI_read(fd, 0, 0x100, ObjStatus);
	OSI_read(fd, -1, 0x100 * sizeof(struct coord), ObjPos);
	OSI_read(fd, -1, 0x200, ObjShapeType);
	OSI_read(fd, -1, 0x200, Amount);
	OSI_read(fd, -1, 0x100, NPCStatus);
	OSI_read(fd, -1, 0x100, STREN);
	OSI_read(fd, -1, 0x100, DEXTE);
	OSI_read(fd, -1, 0x100, INTEL);
	OSI_read(fd, -1, 0x200, ExpPoints);
	OSI_read(fd, -1, 0x100, HitPoints);
	OSI_read(fd, -1, 16 * 14, D_50CE);
	OSI_read(fd, -1, 0x10, Party);
	OSI_read(fd, -1, 1, &PartySize);
	OSI_read(fd, -1, 0x100, Level);
	OSI_read(fd, -1, 0x100, SchedIndex);
	OSI_read(fd, -1, 0x100, NPCMode);
	OSI_read(fd, -1, 0x100, D_3262);
	OSI_read(fd, -1, 0x100, NPCComMode);
	OSI_read(fd, -1, 0x100, MovePts);
	OSI_read(fd, -1, 0x100, MovePts);
	OSI_read(fd, -1, 0x100 * sizeof(int16), OrigShapeType);
	OSI_read(fd, -1, 0x100, TalkFlags);
	OSI_read(fd, -1, 0x100, Leader);
	OSI_read(fd, -1, 0x100, NPCFlag);
	OSI_read(fd, -1, 0x100, PathData);
	OSI_read(fd, -1, (int32)((char *)&GameStateEnd - (char *)&IsOnQuest), &IsOnQuest);
	OSI_close(fd);

	OBJ_RelinkNPCs();
	if(TalkFlags[6] & 2)
		OrigShapeType[6] = 0x162 | ((OrigShapeType[6] >> 10) << 10);
	else
		OrigShapeType[6] = 0x165 | ((OrigShapeType[6] >> 10) << 10);
	VarInt['Q' - 0x37] = IsOnQuest;
	InCombat = 0;
	MapX = GetX(Party[Active]);
	MapY = GetY(Party[Active]);
	MapZ = GetZ(Party[Active]);

	for(i = 0; i < NumAnimData; i ++) {
		if(!(TalkFlags[0x73] & 4) && (
			(D_B4C8[i] >= BaseTile[0xbc] && D_B4C8[i] < BaseTile[0xbd]) ||
			(D_B4C8[i] >= BaseTile[0xbf] && D_B4C8[i] < BaseTile[0xc0]) ||
			(D_B4C8[i] >= BaseTile[0x1b0] && D_B4C8[i] < BaseTile[0x1b1]) ||
			(D_B4C8[i] >= BaseTile[0x1b3] && D_B4C8[i] < BaseTile[0x1b4])
		)) {
			StateAnimData[i] = 0;
		} else if(IsWaterTile(D_B4C8[i])) {
			D_3240[i] = 0;
			if((TalkFlags[0x76] & 0x20) && (TalkFlags[0x76] & 0x40))
				D_770E[i] ++;
		} else if(D_B4C8[i] >= 1 && D_B4C8[i] <= 4) {
			D_3240[i] = 0;
			if(TalkFlags[0x76] & 0x20)
				D_770E[i] ++;
		} else if(D_B4C8[i] == BaseTile[0x62]) {
			D_3240[i] = 0;
		} else {
			StateAnimData[i] = 1;
		}
	}

	setDefaultCommand(DefaultCommand);
	LoadNewRegions(MapX, MapY, MapZ);
	MAP_LoadChunkIndexes(MapX, MapY, MapZ);
	MoveAppliesTerrain = 1;
	MAP_LoadAround();

	if(!(TalkFlags[0x73] & 4)) {
		for(i = 0x101; i < 0xc00; i ++) {
			switch(GetType(i)) {
				case 0x9a:
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
	} else {
		for(i = 0x101; i < 0xc00; i ++) {
			switch(GetType(i)) {
				case 0x9a:
					ObjShapeType[i] = GetType(i) + 0x400;
				break;
				case 0xd0:
					if(OBJ_FindTypeAt(0xd1, GetX(i), (GetY(i) + 1) & 0x3ff) < 0) {
						obj = AddMapObj(0x4d1, 0, 0, GetX(i), (GetY(i) + 1) & 0x3ff, MapZ);
						ObjStatus[obj] &= ~0x20;
					}
				break;
				case 0xe9:
					frame = GetFrame(i);
					if(frame == 1 || frame == 3)
						ObjShapeType[i] = GetType(i) + ((frame + 4) << 10);
				break;
			}
		}
	}

	if(GameFlags & 0x10) {
		ColorPaper = 7;
		StatusDisplay = 0x92;
		StatusMember = PartySize;
	} else {
		ColorPaper = D_082E;
		StatusDisplay = 0x91;
		StatusMember = 0;
	}
	GR_SET_COLOR(ColorPaper);
	i = CON_mouseOffAt(184, 128, 311, 195);
	GR_FILL_RECT(184, 128, 311, 195);
	CON_mouseOn(i);
	savedClip = CurClip;
	for(i = 0; i < 4; i ++) {
		CON_setClip(&TextWindows[i]);
		CurClip->bgColor = ColorPaper;
	}
	CON_setClip(savedClip);
	StatusDirty = 1;
	BargeFindLeader(1);
}

/*save the game: write the state to "savegame\\objlist"*/
/*C_2238_0CBE*/GAME_Save() {
	int16 fd, i;

	if(TalkFlags[0x76] & 0x20) {
		if(TalkFlags[0x76] & 0x40) {
			for(i = 0; i < NumAnimData; i ++)
				if(IsWaterTile(D_B4C8[i]))
					D_770E[i] --;
		}
		for(i = 0; i < NumAnimData; i ++)
			if(D_B4C8[i] >= 1 && D_B4C8[i] <= 4)
				D_770E[i] --;
	}
	DiskFullChoice = ' ';
	BuiltAreaX = BuiltAreaY = BuiltAreaZ = -1;
	OBJ_CommitBlocks();
	IsOnQuest = VarInt['Q' - 0x37];

	fd = OSI_create(/*1C93*/"savegame\\objlist");
	OSI_write(fd, 0, 0x100, ObjStatus);
	OSI_write(fd, -1, 0x100 * sizeof(struct coord), ObjPos);
	OSI_write(fd, -1, 0x200, ObjShapeType);
	OSI_write(fd, -1, 0x200, Amount);
	OSI_write(fd, -1, 0x100, NPCStatus);
	OSI_write(fd, -1, 0x100, STREN);
	OSI_write(fd, -1, 0x100, DEXTE);
	OSI_write(fd, -1, 0x100, INTEL);
	OSI_write(fd, -1, 0x200, ExpPoints);
	OSI_write(fd, -1, 0x100, HitPoints);
	OSI_write(fd, -1, 16 * 14, D_50CE);
	OSI_write(fd, -1, 0x10, Party);
	OSI_write(fd, -1, 1, &PartySize);
	OSI_write(fd, -1, 0x100, Level);
	OSI_write(fd, -1, 0x100, SchedIndex);
	OSI_write(fd, -1, 0x100, NPCMode);
	OSI_write(fd, -1, 0x100, D_3262);
	OSI_write(fd, -1, 0x100, NPCComMode);
	OSI_write(fd, -1, 0x100, MovePts);
	OSI_write(fd, -1, 0x100, MovePts);
	OSI_write(fd, -1, 0x100 * sizeof(int16), OrigShapeType);
	OSI_write(fd, -1, 0x100, TalkFlags);
	OSI_write(fd, -1, 0x100, Leader);
	OSI_write(fd, -1, 0x100, NPCFlag);
	OSI_write(fd, -1, 0x100, PathData);
	OSI_write(fd, -1, (int32)((char *)&GameStateEnd - (char *)&IsOnQuest), &IsOnQuest);
	OSI_close(fd);

	MAP_LoadAround();
	StatusDirty ++;
	TalkFlags[0x76] &= ~0x20;
	TalkFlags[0x73] &= ~0x10;
}

/*draw tile ftile over background tile btile at screen (x,y)
(composed in the backbuffer's top-left 16x16 corner, then copied)*/
/*C_2238_1026*/DrawTileOnBg(int16 ftile, int16 btile, int16 x, int16 y) {
	GR_PUT_TILE(btile, 0, 0);
	GR_PUT_TILE(ftile, 0, 0);
	GR_COPY_TO_SCREEN(0, 0, 15, 15, x, y);
}

/*draw tile ftile over tile mtile over background tile btile at screen (x,y)*/
/*C_2238_1068*/DrawTile2OnBg(int16 ftile, int16 mtile, int16 btile, int16 x, int16 y) {
	GR_PUT_TILE(btile, 0, 0);
	GR_PUT_TILE(mtile, 0, 0);
	GR_PUT_TILE(ftile, 0, 0);
	GR_COPY_TO_SCREEN(0, 0, 15, 15, x, y);
}

/*draw a 3x5 character in the backbuffer's 16x16 work tile, at
column xOfs, rows 11~15 (bottom of the tile), in text color.
glyph: 0~9 = '0'-'9', 10~25 = 'A'-'P'*/
/*C_2238_10BB*/DrawTinyGlyph(int16 glyph, int16 xOfs) {
	int16 px, py;
	uint16 bits;

	GrInfo->toBackBuffer = 1;
	bits = TinyFont[glyph];
	GR_SET_COLOR(ColorText);
	for(py = 11; py < 16; py++) {
		for(px = xOfs; px < xOfs + 3; px ++) {
			if(bits & 0x8000) {
				GR_PUT_PIXEL(px, py);
			}
			bits <<= 1;
		}
	}
	GrInfo->toBackBuffer = 0;
}

/*draw tile ftile on an empty slot at screen (x,y) with the number val
(0~9999, right aligned, 3x5 font) printed over its bottom*/
/*C_2238_1127*/DrawTileWithNumber(int16 ftile, register int16 val, int16 x, int16 y) {
	register int16 numDigits;
	int16 digit;

	GR_PUT_TILE(EMPTY_SLOT_TILE, 0, 0);
	GR_PUT_TILE(ftile, 0, 0);
	if(val < 10)
		numDigits = 1;
	else if(val < 100)
		numDigits = 2;
	else if(val < 1000)
		numDigits = 3;
	else if(val < 10000)
		numDigits = 4;
	else
		numDigits = 0;
	for(digit = 0; digit < numDigits; digit ++) {
		DrawTinyGlyph(val % 10, TinyDigitX[numDigits - 1][digit]);
		val /= 10;
	}
	GR_COPY_TO_SCREEN(0, 0, 15, 15, x, y);
}

/*draw an inventory object in its slot at screen (x,y): objects counted
on 16 bits (QuanType 4) show their amount, stackable ones (QuanType 2)
their quantity (an amount below 1 is made 1)*/
/*C_2238_11E3*/DrawInvObject(int16 objNum, int16 x, int16 y) {
	int16 quanType, ftile;

	quanType = QuanType(ObjShapeType[objNum]);
	ftile = TILE_FRAME(objNum);
	if(quanType && Amount[objNum] < 1)
		Amount[objNum] = 1;
	if(quanType == 4) {
		GR_PUT_TILE_SCREEN(EMPTY_SLOT_TILE, x, y);
		DrawTileWithNumber(ftile, Amount[objNum], x, y);
	} else if(quanType == 2) {
		GR_PUT_TILE_SCREEN(EMPTY_SLOT_TILE, x, y);
		DrawTileWithNumber(ftile, GetQuan(objNum), x, y);
	} else {
		DrawTileOnBg(ftile, EMPTY_SLOT_TILE, x, y);
	}
}

/*same as DrawTileOnBg (entry point used by the status views)*/
/*C_2238_12D0*/DrawSlotTile(int16 ftile, int16 btile, int16 x, int16 y) {
	DrawTileOnBg(ftile, btile, x, y);
}

/*same as DrawTile2OnBg (entry point used by the status views)*/
/*C_2238_12E7*/DrawSlotTile2(int16 ftile, int16 mtile, int16 btile, int16 x, int16 y) {
	DrawTile2OnBg(ftile, mtile, btile, x, y);
	return 0;
}

/*poll for one key at the text cursor of the current window.
while no key is available, draw one frame of the animated cursor
(PromptCh + 0~3) and run the idle animations once.
returns the key code (KBD_GetKey: ASCII, or 0x100 + scan code)
or 0 if none*/
/*C_2238_1303*/CON_prompt() {
	int16 ch;
	int16 promptX, promptY;

	/*the player is reading: reset the "more" line counter*/
	LinesBeforeMore = CurClip->rows;
	if(EndPagingAtPrompt) {
		CurClip->attrFlags &= ~0x10;/*end of paginated text*/
		EndPagingAtPrompt = 0;
	}
	TextIdle = 0;
	if(CurClip->curX >= CurClip->cols)
		CON_putch0(0);/*line full: new line*/
	promptX = CurClip->curX + CurClip->originX;
	promptY = CurClip->curY + CurClip->originY;
	ch = KBD_GetKey(KbdKeepLastOnly);
	if(ch == 0) {/*animated cursor*/
		(*CON_PutCharFn)(((PromptCnt ++) & 3) + PromptCh, promptX, promptY);
		IdleAnimTick = 1;
		if(AnimateAtPrompt) {
			DisplayTick();
			PaletteAnimation();
		} else {
			if(GrAdapter == 0)
				PaletteAnimation();
		}
		if(MouseOn)
			MOUSE_SERVE();
	}
	(*CON_PutCharFn)(' ', promptX, promptY);
	TextIdle = 1;

	return ch;
}

/*echo one typed character for CON_gets(), directly at the cursor
(no line buffer); '\b' erases the previous char, also across lines*/
/*C_2238_13D0*/CON_EchoChar(int16 ch) {
	int16 scrX, scrY;

	scrX = CurClip->curX + CurClip->originX;
	scrY = CurClip->curY + CurClip->originY;
	TextIdle = 0;
	if(ch == '\b') {/*backspace*/
		if(CurClip->curX) {
			CurClip->curX --;
			scrX --;
		} else if(CurClip->curY) {
			CurClip->curY --;
			scrY --;
			CurClip->curX = CurClip->cols - 1;
			scrX = CurClip->curX + CurClip->originX;
		}
		(*CON_PutCharFn)(' ', scrX, scrY);
	} else {
		(*CON_PutCharFn)(ch, scrX, scrY);
		if(++CurClip->curX >= CurClip->cols) {
			CurClip->curX = 0;
			if(CurClip->curY < CurClip->rows - 1) {
				CurClip->curY ++;
			} else if(CurClip->attrFlags & 0x20) {
				TextIdle = 1;
				(*CON_ScrollUpFn)(CurClip->originX, CurClip->originY, CurClip->cols, CurClip->rows);
				TextIdle = 0;
			}
		}
	}
	TextIdle = 1;
}

/*draw string str directly at text cell (x,y), no clipping (uncalled?)*/
/*C_2238_14AA*/CON_PutStrAt(unsigned char *str, int16 x, int16 y) {
	while(*str) {
		(*CON_PutCharFn)(*str++, x++, y);
	}
}

/*mouse held over the map view without command: return the walking
direction (MkDirection, -1 = center) and, if AllowMouseMov, update the
walk target MouseMapX/Y (map coordinates of the pointed tile). The target
is kept while the pointer stays on the same tile, unless the party is next
to it on the same row/column or the button was just pressed.*/
/*C_2238_14D3*/mkMouseDir() {
	int16 tileX, tileY;
	int16 dir;

	tileX = SCR2TIL(PointerX);
	tileY = SCR2TIL(PointerY);
	dir = MkDirection(PointerX, PointerY);

	if(AllowMouseMov) {
		if(
			MouseTrigger || MouseMapX == -1 ||
			(MouseMapX == MapX && MapDistance(MouseMapY, MapY) < 2) ||
			(MouseMapY == MapY && MapDistance(MouseMapX, MapX) < 2) ||
			tileX != LastMouseTileX || tileY != LastMouseTileY
		) {
			MouseMapX = (tileX + MapX - 5) & 0x3ff;
			MouseMapY = (tileY + MapY - 5) & 0x3ff;
			LastMouseTileX = tileX;
			LastMouseTileY = tileY;
		}
	} else {
		MouseMapX =
		MouseMapY = -1;
	}

	return dir;
}

/*fill Selection (x, y, obj) from the pointer position PointerX/Y.
- map view: Selection.x/y = map coordinates; returns 0x9d if there is
  an object (COMBAT_GetTargetAt), else Selection.obj = -1
  and SelectionIsDark tells if the tile is hidden/darkness.
- equipment view: returns 0x9b for an inventory slot or the container
  slot (STAT_PickInvenSlot), 0x9c for an equipment slot (STAT_PickEquipSlot).
returns 0 when nothing selectable is pointed at*/
static /*C_2238_158A*/mkMouseSelection() {
	int16 tile;

	SelectionIsDark = 0;
	if(PointerX < 168) {
		SelectionX = (SCR2TIL(PointerX) & 0x3ff) + MapX - 5;
		SelectionY = (SCR2TIL(PointerY) & 0x3ff) + MapY - 5;
		if(Obj_11x11[SCR2TIL(PointerY)][SCR2TIL(PointerX)] || Tile_11x11[SCR2TIL(PointerY)][SCR2TIL(PointerX)] >= 0x200) {
			SelectionObj = COMBAT_GetTargetAt(Party[Active], SelectionX, SelectionY);
			return 0x9d;
		}
		SelectionObj = -1;
		tile = Tile_11x11[SCR2TIL(PointerY)][SCR2TIL(PointerX)];
		SelectionIsDark = (tile == 0xff || tile == 0x10c);
	} else if(StatusDisplay == 0x92) {
		SelectionX = -1;
		SelectionY = -1;
		if(STAT_PickInvenSlot())
			return 0x9b;
		if(STAT_PickEquipSlot())
			return 0x9c;
	} else {
		SelectionX = -1;
		SelectionY = -1;
		SelectionObj = -1;
	}
	return 0;
}

/*mouse click in the party view: returns the command to execute (0 = none).
Each member line is 16 pixels high; clicking the portrait column (x > 200)
opens his portrait, the column before it (184 < x <= 200) his equipment.
Selection.obj = party index*/
static /*C_2238_16A2*/MouseInPartyView() {
	int16 cmd;

	if(GameFlags & 0x10) {
		SelectionObj = PartySize;
		return 0x92;
	}
	cmd = 0;
	if(PointerY >= 8) {
		if(PointerX > 208) {/*to portrait view*/
			cmd = 0x90;
			SelectionObj = PartyListTop + (PointerY - 8) / 24;
			if(PartySize <= SelectionObj)
				cmd = 0;
		} else if(PointerX > 192) {/*to equipment view*/
			cmd = 0x92;
			SelectionObj = PartyListTop + (PointerY - 8) / 24;
			if(PartySize <= SelectionObj)
				cmd = 0;
		}
	}

	return cmd;
}

/*mouse click in the portrait view: only the buttons row (y >= 102) reacts.
returns the command (0 = none)*/
static /*C_2238_172A*/MouseInPortraitView() {
	int16 cmd;

	if(GameFlags & 0x10) {
		SelectionObj = PartySize;
		return 0x92;
	}
	cmd = 0;
	if(PointerY >= 88) {
		if(PointerX >= 200 && PointerX < 216) {/*to party view*/
			cmd = 0x91;
		} else if(PointerX >= 216 && PointerX < 232) {/*to equipment view*/
			cmd = 0x92;
			SelectionObj = -1;
		} else if(PointerX >= 232 && PointerX < 248) {/*next member*/
			cmd = 0x99;
		} else if(PointerX >= 184 && PointerX < 200) {/*previous member*/
			cmd = 0x9a;
		}
	}
	return cmd;
}

/*mouse click (or slot cursor validation) in the equipment/inventory view.
returns the command (0 = none). The combat mode button (248,102; not for
the player nor an NPC out of the party's control) cycles the member's
combat mode here directly*/
static /*C_2238_17AC*/MouseInEquipView() {
	int16 cmd, selKind;
	int16 objType;

	if(IsInConversation)
		return 0;
	cmd = 0;

	/*combat mode button: cycles 2, 4, 5, 7, 8*/
	if(
		!(GameFlags & 0x10) &&
		StatusMember != 0 &&
		PointerX >= 248 && PointerX < 264 &&
		PointerY >= 102 &&
		!(ObjStatus[Party[StatusMember]] & 0x80)
	) {
		NPCComMode[Party[StatusMember]] = NPCComMode[Party[StatusMember]] + 1;
		if(NPCComMode[Party[StatusMember]] > 8)
			NPCComMode[Party[StatusMember]] = 2;
		else if(NPCComMode[Party[StatusMember]] == 3)
			NPCComMode[Party[StatusMember]] = 4;
		else if(NPCComMode[Party[StatusMember]] == 5)
			NPCComMode[Party[StatusMember]] = 7;
		if(
			InCombat &&
			Active != StatusMember &&
			!(ObjStatus[Party[StatusMember]] & 4)
		) NPCMode[Party[StatusMember]] = NPCComMode[Party[StatusMember]];
		STAT_refreshComMode();
	} else if(PointerY >= 102) {
		if(GameFlags & 0x10) {
			cmd = 0x92;
			SelectionObj = -1;
		} else if(PointerX >= 200 && PointerX < 216) {/*to party view*/
			cmd = 0x91;
		} else if(PointerX >= 216 && PointerX < 232) {/*to portrait view*/
			cmd = 0x90;
			SelectionObj = -1;
		} else if(PointerX >= 232 && PointerX < 248) {/*next member*/
			cmd = 0x99;
		} else if(PointerX >= 184 && PointerX < 200) {/*previous member*/
			cmd = 0x9a;
		}
	} else if(
		PointerX >= 272 && PointerX < 288 &&
		PointerY >= 16 && PointerY < 32 &&
		MouseMode == 0
	) {/*container slot (top right): back to the parent container/owner*/
		if((GameFlags & 0x10) && IsNPC(InvenOwner))
			cmd = 0x92;
		else
			cmd = 0x9f;
	} else if(MouseMode == 0 && PendingCommand != 0x86) {
		selKind = mkMouseSelection();
		if(SelectionObj != -1) {
			if(selKind == 0x9b) {
				/*inventory object: open it if it is a container, else ready it*/
				objType = GetType(SelectionObj);
				if(IsContainerType(objType))
					cmd = 0xa0;
				else
					cmd = 0xa7;
			} else if(selKind == 0x9c) {
				/*equipped object: unready it*/
				cmd = 0xa8;
			}
		}
	}

	return cmd;
}

/*set the default command (0x81~0x89, or 0xff = none), executed by a
right click; it is shown on its icon (icons are 18 pixels wide from x = 16)
by drawing the bitmap D_BA7E (mdscreen.lzc entry 2) there: mode 1 erases
the mark of the previous one, mode 0 marks the new one*/
static /*C_2238_1A2E*/setDefaultCommand(int16 cmd) {
	int16 x;

	if(MouseOn)
		MOUSE_HIDE();
	if(DefaultCommand != 0xff) {
		x = TIL2SCR(DefaultCommand - 0x81) + ((DefaultCommand - 0x81) << 1);
		GR_DRAW_BITMAP(&D_3232, x + 21, 169, D_BA7E, 1);
	}
	if((DefaultCommand = cmd) != 0xff) {
		x = TIL2SCR(DefaultCommand - 0x81) + ((DefaultCommand - 0x81) << 1);
		GR_DRAW_BITMAP(&D_3232, x + 21, 169, D_BA7E, 0);
	}
	if(MouseOn)
		MOUSE_SHOW();
}

/*translate the current mouse button state (MouseButtons, MouseTrigger = just
pressed) at PointerX/Y into a command code (0 = none), by zone (MouseZones):
- map view: walk (0x80 + AdvanceDir; repeated while held unless the last
  step was blocked, AdvanceBlocked), pass (center), target (0x8e when a command
  waits for one, MouseMode != 0), right click = default command (0xa9)
- icons: left click = command icon, right click = set the default command
- status panel: depends on StatusDisplay
- elsewhere: pass*/
static /*C_2238_1ADC*/mkMouseCommand() {
	int16 mouseCmd, zone;
	int16 mouseDir;

	mouseCmd = 0;

	/*(zone 3 is the whole screen: always found)*/
	for(zone = 0; ; zone ++) {
		if(
			MouseZones[zone][0] <= PointerX && MouseZones[zone][2] >= PointerX &&
			MouseZones[zone][1] <= PointerY && MouseZones[zone][3] >= PointerY
		) break;
	}
	switch(zone) {
		case 0:/*map view*/
			if(ViewRotation) {
				/*rotated view: rotate the pointer around the party*/
				int16 dx, dy;

				dx = PointerX - DirOriginX;
				dy = PointerY - DirOriginY;
				PointerX = DirOriginX + RotCos[ViewRotation] * dx - RotSin[ViewRotation] * dy;
				PointerY = DirOriginY + RotSin[ViewRotation] * dx + RotCos[ViewRotation] * dy;
			}
			if(MouseMode == 0) {
				if(DefaultCommand != 0xff && RIGHT_CLICK) {
					if(MouseTrigger) {
						mouseCmd = 0xa9;
						mkMouseSelection();
					}
				} else {
					mouseDir = mkMouseDir();
					if(mouseDir == -1) {
						/*(on one line, merged with case 0x9e below)*/
						if(MouseTrigger) mouseCmd = 0x8f;
						else
							mouseCmd = 0;
					} else if(MouseTrigger || AdvanceBlocked == 0 || mouseDir != AdvanceDir) {
						mouseCmd = 0x80;
						AdvanceDir = mouseDir;
					}
				}
			} else if(MouseTrigger) {
				mouseCmd = 0x8e;
				mkMouseSelection();
			}
		break;
		case 1:/*icons: 18 pixels wide from x = 16, 0x81~0x87 can be the default*/
			if(MouseTrigger) {
				if(LEFT_CLICK)
					mouseCmd = 0x81 + (PointerX - 16) / 18;
				else if(PointerX < 142)
					setDefaultCommand(0x81 + (PointerX - 16) / 18);
			}
		break;
		case 2:/*status panel*/
			if(MouseTrigger) {
				if(DefaultCommand != 0xff && MouseMode == 0 && RIGHT_CLICK) {
					mouseCmd = 0xa9;
					mkMouseSelection();
				} else {
					switch(StatusDisplay) {
						case 0x91: mouseCmd = MouseInPartyView(); break;
						case 0x90: mouseCmd = MouseInPortraitView(); break;
						case 0x92:
							mouseCmd = MouseInEquipView();
							if(mouseCmd == 0 && MouseMode == 1 && mkMouseSelection())
								mouseCmd = 0x8e;
						break;
						case 0x9e: mouseCmd = 0x8f; break;
					}
				}
			}
		break;
		case 3:/*the whole screen*/
			if(MouseTrigger) {
				mouseCmd = 0x8f;
				SelectionX =
				SelectionY = -1;
			}
		break;
	}

	return mouseCmd;
}

/*keyboard "slot cursor" of the equipment/portrait views (SelectMode 2),
at column SlotCursorCol / row SlotCursorRow:
 mode 1: draw the cursor,
 mode 0: erase it (redraw the slot's object or button),
 mode 2: move PointerX/Y on the slot (to validate it like a mouse click).
In every mode PointerX/Y are first set inside the slot, so that
mkMouseSelection()/MouseInEquipView() can hit-test it.*/
/*C_2238_1D17*/DrawSlotCursor(int16 mode) {
	int16 x, y;
	int16 selKind, objType, mouseHidden;

	if(SelectMode != 2)
		return;
	if(StatusDisplay != 0x92 && StatusDisplay != 0x90)
		return;
	if(StatusMember == 0) {
		SlotButtonTiles[0] = EMPTY_SLOT_TILE;
		SlotButtonTiles[4] = 0x11c;
	} else if(ObjStatus[Party[StatusMember]] & 0x80) {
		SlotButtonTiles[0] = 0x11a;
		SlotButtonTiles[4] = 0x11c;
	} else {
		SlotButtonTiles[0] = 0x11a;
		SlotButtonTiles[4] = 0x11d;
	}
	if(PartySize - 1 == StatusMember)
		SlotButtonTiles[3] = EMPTY_SLOT_TILE;
	else
		SlotButtonTiles[3] = 0x11b;
	if(StatusDisplay == 0x90)
		SlotButtonTiles[2] = 0x119;
	else
		SlotButtonTiles[2] = 0x118;
	if(StatusDisplay == 0x90) {
		SlotCursorRow = 4;
		if(SlotCursorCol > 3)
			SlotCursorCol = 3;
	}
	if(SlotCursorRow < 0) {
		/*container/owner slot*/
		x = PointerX = 272;
		y = PointerY = 16;
	} else if(SlotCursorRow == 3) {
		/*last inventory row*/
		if(SlotCursorCol < 3)
			SlotCursorCol = 3;
		PointerX = SlotCursorCol * 16 + 200;
		PointerY = SlotCursorRow * 16 + 32;
	} else if(SlotCursorRow > 3) {
		/*buttons row*/
		if(SlotCursorCol > 4)
			SlotCursorCol = 4;
		x = PointerX = SlotCursorCol * 16 + 184;
		y = PointerY = 102;
	} else if(SlotCursorCol < 3) {
		/*equipment slots*/
		PointerX = SlotCursorCol * 24 + 184;
		PointerY = SlotCursorRow * 20 + 24;
	} else {
		/*inventory slots*/
		PointerX = SlotCursorCol * 16 + 200;
		PointerY = SlotCursorRow * 16 + 32;
	}
	if(SlotCursorRow >= 0 && SlotCursorRow < 4) {
		x = SlotScrX[SlotCursorCol];
		y = SlotScrY[SlotCursorRow][SlotCursorCol];
	}
	mouseHidden = CON_mouseOffAt(x, y, x + 15, y + 15);
	if(mode == 1) {
		GR_PUT_TILE_SCREEN(0x109, x, y);
	} else if(mode == 0) {
		selKind = mkMouseSelection();
		if(SlotCursorRow > 3) {
			if(!(GameFlags & 0x10))
				GR_PUT_TILE_SCREEN(SlotButtonTiles[SlotCursorCol], x, y);
			else
				GR_PUT_TILE_SCREEN(EMPTY_SLOT_TILE, x, y);
		} else if((selKind == 0x9b || selKind == 0x9c) && SelectionObj >= 0) {
			if(SlotCursorRow < 0) {
				/*container slot: object, or character facing south*/
				if(IsNPC(SelectionObj)) {
					objType = GetType(SelectionObj);
					DrawTileOnBg(OBJ_MakeDirFrame(objType, 4) + BaseTile[objType], EMPTY_SLOT_TILE, x, y);
				} else {
					DrawTileOnBg(TILE_FRAME(SelectionObj), EMPTY_SLOT_TILE, x, y);
				}
			} else {
				DrawInvObject(SelectionObj, x, y);
			}
		} else if(
			SlotCursorRow == 1 && SlotCursorCol == 2 &&
			((!ShowNPCInven && Party_2Handed[StatusMember]) || (ShowNPCInven && NPC_2Handed))
		) {
			/*left hand slot taken by a two-handed weapon*/
			GR_PUT_TILE_SCREEN(ColorPaper == 7 ? 0x128 : 0x112, x, y);
		} else {
			/*empty slot*/
			GR_PUT_TILE_SCREEN(ColorPaper == 7 ? 0x127 : 0x111, x, y);
		}
	} else if(mode == 2) {
		PointerX = x;
		PointerY = y;
	}
	CON_mouseOn(mouseHidden);
}

/*wait for a key or a mouse click (and its release)*/
/*C_2238_2069*/CON_WaitKeyOrClick() {
	int16 key;

	key = 0;
	while(!key) {
		if(MouseOn) {
			MOUSE_SERVE();
			if(Mouse.buttonL || Mouse.buttonR) {
				do
					MOUSE_SERVE();
				while(Mouse.buttonL || Mouse.buttonR);
				break;
			}
		}
		key = CON_prompt();
	}
}

/*wait for the next input and translate it into a key or a command code.
- mouse: mkMouseCommand(); while the button is held, the command produced
  by the press (MouseHeldCmd) is repeated (e.g. walking), others are ignored.
- keyboard: direction keys -> 0x80 + AdvanceDir, <ALT+letter> sets the
  default command; then according to SelectMode
  (0 none, 1 aiming cursor AimX/AimY on the map, 2 slot cursor in the
  equipment/portrait view):
  the command letter again or <ENTER> validates, <SPACE>/<ESC> cancels,
  <TAB> switches between map aiming and slot cursor, '1'~'9' selects a
  party member; plus the status view hotkeys ('*' '/' F10 '+' '-' F1~F8 '`').
CookedInput == 0 returns the raw key (used for "more" prompts).
returns a key code or a command code, never 0*/
/*C_2238_20AD*/CON_ReadCommand() {
	int16 ch, buttons;
	int16 mouseX, mouseY, newX, newY;

	if(CookedInput)
		CON_putch0(-1);
	ch = 0;
	LinesBeforeMore = CurClip->rows;
	while(!ch) {
		/* */
		if(MouseOn) {
			MOUSE_SERVE();
			buttons = (Mouse.buttonL?1:0) | (Mouse.buttonR?2:0);
			mouseX = Mouse.posX;
			mouseY = Mouse.posY;
			if(buttons & 3) {
				if(!MousePress) {
					MousePress =
					MouseTrigger = 1;
				} else {
					MouseTrigger = 0;
				}
				PointerX = mouseX;
				PointerY = mouseY;
				MouseButtons = buttons;
				ch = mkMouseCommand();
				if(MouseTrigger)
					MouseHeldCmd = ch;
				else if(MouseHeldCmd != ch)
					ch = 0;
			} else {
				MousePress =
				MouseTrigger = 0;
			}
		}
		if(ch)
			continue;
		/* */
		ch = CON_prompt();
		if(ch) {
			MouseMapX =
			MouseMapY = -1;
		}
		if(InputToUpper && ch < 0x100)
			ch = toupper(ch);
		/*0x1xx = 0x100 + BIOS scan code (arrows/keypad, <ALT+key>)*/
		switch(ch) {
			case 0x148: ch = 0x80; AdvanceDir = 0; break;/*up*/
			case 0x150: ch = 0x80; AdvanceDir = 4; break;/*down*/
			case 0x14b: ch = 0x80; AdvanceDir = 6; break;/*left*/
			case 0x14d: ch = 0x80; AdvanceDir = 2; break;/*right*/
			case 0x147: ch = 0x80; AdvanceDir = 7; break;/*up left*/
			case 0x149: ch = 0x80; AdvanceDir = 1; break;/*up right*/
			case 0x14f: ch = 0x80; AdvanceDir = 5; break;/*down left*/
			case 0x151: ch = 0x80; AdvanceDir = 3; break;/*down right*/
			case 0x11e: setDefaultCommand(0x81); ch = 0; break;/*<ALT A>*/
			case 0x114: setDefaultCommand(0x82); ch = 0; break;/*<ALT T>*/
			case 0x126: setDefaultCommand(0x83); ch = 0; break;/*<ALT L>*/
			case 0x122: setDefaultCommand(0x84); ch = 0; break;/*<ALT G>*/
			case 0x120: setDefaultCommand(0x85); ch = 0; break;/*<ALT D>*/
			case 0x132: setDefaultCommand(0x86); ch = 0; break;/*<ALT M>*/
			case 0x116: setDefaultCommand(0x87); ch = 0; break;/*<ALT U>*/
		}
		/*rotated view: the direction keys turn with it*/
		if(ViewRotation && ch == 0x80)
			AdvanceDir = (AdvanceDir + ViewRotation) & 7;
		if(MouseMode == 0 && !FreeAimMode) {
			DrawSlotCursor(0);
			SelectMode = 0;
		}
		/*raw key wanted: the loop ends since ch != 0 (unless no key)*/
		if(CookedInput == 0 || (InGetString && ch == '\r'))
			continue;
		if(SelectMode != 0) {
			/*pressing the pending command's letter again = <ENTER>*/
			if(
				(PendingCommand == 0x81 && ch == 'A') ||
				(PendingCommand == 0x82 && ch == 'T') ||
				(PendingCommand == 0x83 && ch == 'L') ||
				(PendingCommand == 0x84 && ch == 'G') ||
				(PendingCommand == 0x85 && ch == 'D') ||
				(PendingCommand == 0x86 && ch == 'M') ||
				(PendingCommand == 0x87 && ch == 'U')
			) ch = '\r';

			if(ch == 9) {
				/*<TAB>: map aiming <-> slot cursor in the equipment view*/
				if(SelectMode == 1) {
					SelectMode = 2;
					if(StatusDisplay != 0x92) {
						if(IN_VEHICLE)
							StatusMember = 0;
						else
							StatusMember = Active;
						StatusDisplay = 0x92;
						StatusDirty = 1;
						RefreshStatus();
					}
					DrawSlotCursor(1);
				} else if(StatusDisplay != 0x9e && !IsInConversation) {
					DrawSlotCursor(0);
					SelectMode = 1;
				}
				DrawMapWindow(1);
				ch = 0;
			} else if(ch == ' ' || ch == 27) {
				DrawSlotCursor(0);
				SelectMode = 0;
				ch = 0;
			} else if(ch == '\r') {
				if(SelectMode == 1) {
					/*target = aimed map tile; free aiming
					(no command) runs the default command*/
					ch = 0x8e;
					if(MouseMode == 0) {
						if(FreeAimMode && DefaultCommand != 0xff)
							ch = 0xa9;
						FreeAimMode = 0;
					}
					SelectMode = 0;
					PointerX = TIL2SCR(AimX);
					PointerY = TIL2SCR(AimY);
					mkMouseSelection();
				} else if(SelectMode == 2) {
					if(StatusDisplay == 0x92 || StatusDisplay == 0x90) {
						DrawSlotCursor(2);
						if(StatusDisplay == 0x92)
							ch = MouseInEquipView();
						else
							ch = MouseInPortraitView();
						if(ch == 0 && MouseMode == 1 && mkMouseSelection())
							ch = 0x8e;
					}
				}
			} else if(ch == 0x80) {
				if(SelectMode == 1) {
					/*move the aiming cursor inside the map view, within
					SelectRange of the party member; SelectRange -1 means
					"adjacent tile": the direction selects at once*/
					ch = 0;
					newX = AimX + DirIncrX[AdvanceDir];
					newY = AimY + DirIncrY[AdvanceDir];
					if(
						newX >= 0 && newX < 11 &&
						newY >= 1 && newY < 10 &&
						COMBAT_TileDistance(Party[Active], (newX + MapX - 5) & 0x3ff, (newY + MapY - 5) & 0x3ff) <= SelectRange
					) {
						AimX = newX;
						AimY = newY;
						DrawMapWindow(1);
					} else if(SelectRange == -1) {
						ch = 0x8e;
						PointerX = TIL2SCR(newX);
						PointerY = TIL2SCR(newY);
						mkMouseSelection();
					}
				} else if(SelectMode == 2 && StatusDisplay == 0x92) {
					/*move the slot cursor; past the edges it goes to
					the container slot (up) or to the buttons row (down)*/
					ch = 0;
					newX = SlotCursorCol + DirIncrX[AdvanceDir];
					newY = SlotCursorRow + DirIncrY[AdvanceDir];
					if((GameFlags & 0x10) && newY > 3)
						newY = 3;
					if(
						newX >= 0 && newX < 7 &&
						newY >= 0 && newY < 4
					) {
						DrawSlotCursor(0);
						if(newX == 1 && newY == 1) {/*skip the figure's body*/
							newX += DirIncrX[AdvanceDir];
							newY += DirIncrY[AdvanceDir];
						}
						SlotCursorCol = newX;
						SlotCursorRow = newY;
						DrawSlotCursor(1);
					} else if((newX > 2 && newX < 7) || newY > 3) {
						if(newY < 0) {
							DrawSlotCursor(0);
							SlotCursorRow = 0xff;
							DrawSlotCursor(1);
						} else if(newY > 3) {
							DrawSlotCursor(0);
							SlotCursorRow = 4;
							SlotCursorCol = newX;
							if(SlotCursorCol > 4)
								SlotCursorCol = 4;
							else if(SlotCursorCol < 0)
								SlotCursorCol = 0;
							DrawSlotCursor(1);
						}
					}
				} else if(SelectMode == 2 && StatusDisplay == 0x90) {
					ch = 0;
					newX = SlotCursorCol + DirIncrX[AdvanceDir];
					if(newX < 0)
						newX = 0;
					else if(newX > 3)
						newX = 3;
					DrawSlotCursor(0);
					SlotCursorCol = newX;
					SlotCursorRow = 3;
					DrawSlotCursor(1);
				}
			} else if(ch >= '1' && ch - '1' < PartySize && SelectMode == 1) {
				/*'1'~: select a party member as target*/
				SelectionIsDark = 0;
				SelectMode = 0;
				SelectionObj = Party[ch - '1'];
				if(IN_VEHICLE) {
					SelectionX = MapX;
					SelectionY = MapY;
				} else {
					SelectionX = GetX(SelectionObj);
					SelectionY = GetY(SelectionObj);
				}
				ch = 0x8e;
			}
		} else if(ch == 9 && StatusDisplay != 0x9e && !IsInConversation) {
			/*<TAB> with no command pending: free aiming (FreeAimMode)*/
			if(StatusDisplay == 0x92) {
				SelectMode = 2;
				DrawSlotCursor(1);
			} else if(StatusDisplay != 0xa1) {
				SelectMode = 1;
			}
			if(SelectMode != 0) {
				AimX =
				AimY = 5;
				SelectRange = 7;
				FreeAimMode = 1;
				DrawMapWindow(1);
				ch = 0;
			}
		}
		if(StatusDisplay == 0x92 && !IsInConversation) {
			/*equipment view*/
			if(ch == '*' && !(GameFlags & 0x10)) {
				ch = 0x90;
				SelectionObj = -1;
			} else if((ch == '/' || ch == 0x144/*F10*/) && !(GameFlags & 0x10)) {
				ch = 0x91;
			} else if(ch == '+' && !(GameFlags & 0x10)) {
				ch = 0x99;
			} else if(ch == '-' && !(GameFlags & 0x10)) {
				ch = 0x9a;
			} else if((ch == '`' || ch == '~') && !(GameFlags & 0x10)) {
				/*"click" on the combat mode button*/
				PointerX = 248;
				PointerY = 102;
				MouseInEquipView();
				ch = 0;
			} else if(ch >= 0x13b/*F1*/ && PartySize + 0x13b > ch && !(GameFlags & 0x10)) {
				SelectionObj = ch - 0x13b;
				ch = 0x92;
			}
		} else if(StatusDisplay == 0x90 && !IsInConversation) {
			/*portrait view*/
			if(ch == '*') {
				ch = 0x92;
				SelectionObj = -1;
			} else if(ch == '/' || ch == 0x144/*F10*/) {
				ch = 0x91;
			} else if(ch == '+') {
				ch = 0x99;
			} else if(ch == '-') {
				ch = 0x9a;
			} else if(ch >= 0x13b && PartySize + 0x13b > ch) {
				SelectionObj = ch - 0x13b;
				ch = 0x90;
			}
		} else if(StatusDisplay == 0x91 && !IsInConversation) {
			/*party view*/
			if(ch >= 0x13b/*F1*/ && PartySize + 0x13b > ch) {
				SelectionObj = ch - 0x13b;
				ch = 0x92;
			}
		}
	}

	return ch;
}

/*get char:
wait for a key or a command (CON_ReadCommand), the cursor in text color
(the "more" cursor, PromptCh 1) . The status panel commands (view changes,
containers, ready/unready) are executed here and the wait goes on; any
other key/command is returned.
0xa9 (default command on a right-clicked target) returns DefaultCommand
and, except for 0x8a, 0x89 and 0xaa, makes the next call return 0x8e at
once since Selection is already filled*/
/*C_2238_28CC*/CON_getch() {
	int16 ch, endLoop;
	int16 savedFg;

	savedFg = CurClip->fgColor;
	if(PromptCh == 1)
		CurClip->fgColor = ColorText;
	if(PendingSelect) {
		PendingSelect = 0;
		return 0x8e;
	}
	do {
		endLoop = 0;
		ch = CON_ReadCommand();
		switch(ch) {
			case 0xa9:/*default action*/
				ch = DefaultCommand;
				if(ch != 0x8a && ch != 0x89 && ch != 0xaa)
					PendingSelect = 1;
				endLoop = 1;
			break;
			case 0x91:/*party view*/
				if(!IN_VEHICLE) {
					SelectMode = 0;
					StatusDisplay = 0x91;
					StatusMember = 0;
					StatusDirty = 1;
				}
			break;
			case 0x90:/*portrait view*/
				if(!IN_VEHICLE) {
					StatusDisplay = 0x90;
					if(SelectionObj >= 0)
						StatusMember = SelectionObj;
					StatusDirty = 1;
				}
			break;
			case 0x92:/*equipment view*/
				StatusDisplay = 0x92;
				if(SelectionObj >= 0)
					StatusMember = SelectionObj;
				StatusDirty = 1;
			break;
			case 0x9a:/*previous member*/
				if(StatusMember > 0) {
					StatusMember --;
					StatusDirty = 1;
					SelectionObj = -1;
				}
			break;
			case 0x99:/*next member*/
				if(PartySize - 1 > StatusMember) {
					StatusMember ++;
					StatusDirty = 1;
					SelectionObj = -1;
				}
			break;
			case 0x9f:/*leave the container shown: back to its owner/parent container, or from a character to the party view*/
				if(IsNPC(InvenOwner)) {
					StatusDisplay = 0x91;
					StatusMember = 0;
					StatusDirty = 1;
				} else {
					InvenOwner = GetAssoc(InvenOwner);
					InvenScroll = 0;
					STAT_RefreshInventory(InvenOwner);
					DrawSlotCursor(1);
				}
			break;
			case 0xa0:/*show the content of container Selection.obj (frame 2: locked)*/
				if(GetFrame(SelectionObj) == 2 && GetType(SelectionObj) != 0x155) {
					MUS_PlaySfx(5);
					CON_printf(/*1CA4*/"Locked!\n");
				} else {
					InvenOwner = SelectionObj;
					InvenScroll = 0;
					STAT_RefreshInventory(InvenOwner);
					DrawSlotCursor(1);
				}
			break;
			case 0xa7:/*Ready*/
				STAT_Ready();
				DrawSlotCursor(1);
			break;
			case 0xa8:/*Unready*/
				STAT_Unready();
				DrawSlotCursor(1);
			break;
			default:
				endLoop = 1;
			case 0xa6: ;/*only refreshes the status panel*/
		}
		if(!endLoop)
			RefreshStatus();
	} while(!endLoop);
	CurClip->fgColor = savedFg;

	return ch;
}

/*read a line of at most maxLen printable chars into buf, echoing it.
<ENTER> validates, <ESC> erases it and aborts (GetsAborted = 1, buf = "").
returns the length*/
/*C_2238_2AF1*/CON_gets(char *buf, int16 maxLen) {
	int16 len, done;
	int16 ch;

	InGetString = 1;
	InputToUpper = 0;
	CON_putch0(-1);
	len =
	done = 0;
	KbdKeepLastOnly = 0;
	while(!done) {
		while(!(ch = CON_prompt()));
		switch(ch) {
			case '\r':
				buf[len] = 0;
				GetsAborted = 0;
				done ++;
			break;
			case 27:
				buf[0] = 0;
				while(len --)
					CON_EchoChar(8);
				GetsAborted = 1;
				done ++;
			break;
			case 0x10e:/*backspace*/
				if(len) {
					len --;
					CON_EchoChar(8);
				}
			break;
			default:
				if(len < maxLen && ch >= ' ' && ch <= '~') {
					buf[len] = ch;
					len ++;
					CON_EchoChar(ch);
				}
		}
	}
	KbdKeepLastOnly = 1;
	LinesBeforeMore = CurClip->rows;
	InputToUpper = 1;
	InGetString = 0;
	return len;
}

/*initialize text window clip on the text cells (x0,y0)-(x1,y1)
(inclusive) with the default colors/attributes, and make it current*/
/*C_2238_2BBF*/CON_createClip(struct tClipInfo *clip, int16 x0, int16 y0, int16 x1, int16 y1) {
	CurClip = clip;
	clip->originX = x0;
	clip->originY = y0;
	clip->cols = x1 - x0 + 1;
	clip->rows = y1 - y0 + 1;
	clip->curX = clip->curY = 0;
	clip->bgColor = ColorPaper;
	clip->fgColor = ColorText;
	clip->attrFlags = DefaultClipAttr;
}

/*make clip the current text window (the pending text of the previous one
is drawn first)*/
/*C_2238_2C0A*/CON_setClip(struct tClipInfo *clip) {
	CON_putch0(-1);
	CurClip = clip;
	GrInfo->textBg = CurClip->bgColor;
	GrInfo->textFg = CurClip->fgColor;
	CON_putch0(-1);
}

/*move the text cursor of the current window to (x,y), clamped to it*/
/*C_2238_2C41*/CON_gotoxy(int16 x, int16 y)
{
	if(x < 0)
		x = 0;
	if(y < 0)
		y = 0;
	if(x > CurClip->cols)
		x = CurClip->cols;
	if(y > CurClip->rows)
		y = CurClip->rows;
	CON_putch0(-1);
	CurClip->curX = x;
	CurClip->curY = y;
	CON_putch0(-1);
}

/*low level character output to the current window (no escapes).
ch >= 0: the char is appended to the line buffer (LineBufChars/LineBufColors);
  0 or '\n' ends the line. When the line is complete (newline, or buffer
  full = window width) it is drawn: with word wrap (attrFlags & 1) the line is
  cut after the last ' ' or '-' and the rest is carried to the next line,
  with attrFlags & 2 it is centered. Then the cursor goes to the next line,
  scrolling (attrFlags & 0x20) at the bottom. With attrFlags & 0x10 a "more"
  prompt waits for a key when the window is full of new text (LinesBeforeMore).
ch < 0 (-1): flush: draw the pending part of the line without ending it
  (done before moving the cursor, changing the window, or reading input)*/
/*C_2238_2CAC*/CON_putch0(int16 ch) {
	register int16 row;
	int16 i, breakPos, unusedMidX, savedFg;

	if(!TextOn)
		return;
	/*skip empty lines after an attack message (see SkipNewlines)*/
	if(SkipNewlines && (CurClip->attrFlags & 0x20)) {
		if(ch == '\n')
			return;
		if(ch >= 0)
			SkipNewlines = 0;
	}
	TextIdle = 0;
	if((CurClip->attrFlags & 4) && CapitalizeNext)
		ch = toupper(ch);
	CapitalizeNext = 0;
	if(ch == 0)
		ch = '\n';
	/*runic font: upper half of the character set*/
	if((CurClip->attrFlags & 8) && ch > ' ')
		ch |= 0x80;
	if(CurClip->cols == LineBufLen || ch == '\n' || ch < 0) {
		/*-- line complete (or flush): draw it --*/
		if(ch == '\n' || ch < 0)
			LastNumberPlural = 0;
		if(LinesBeforeMore && ch >= 0 && (CurClip->attrFlags & 0x10))
			LinesBeforeMore --;
		if((CurClip->attrFlags & 0x10) && LinesBeforeMore == 0 && ch >= 0) {
			/*"more": other cursor shape (PromptCh 1), raw keys*/
			unusedMidX = (CurClip->cols >> 1) - 2;
			row = (CurClip->rows + CurClip->originY) - 1;
			CookedInput = 0;
			PromptCh = 1;
			CON_WaitKeyOrClick();
			PromptCh = 5;
			CookedInput = 1;
			TextIdle = 0;
			/*erase the bottom line (where the "more" cursor was)*/
			for(i = 0; i < CurClip->cols; i ++)
				(*CON_PutCharFn)(' ', i + CurClip->originX, row);
			LinesBeforeMore = CurClip->rows - 1;
		}
		/*word wrap: find where to cut the full line (not before the part
		already drawn; no break found: cut at the window width)*/
		breakPos = LineBufLen;
		if((CurClip->attrFlags & 1) && CurClip->cols == LineBufLen && ch >= 0) {
			LineBufChars[LineBufLen] = (ch == '\n')?' ':ch;
			LineBufColors[LineBufLen] = CurClip->fgColor;
			while(LineBufChars[breakPos] != ' ' && LineBufChars[breakPos - 1] != '-' && LineBufDrawn < breakPos)
				breakPos --;
			if(breakPos == 0)
				breakPos = LineBufLen;
		}
		row = CurClip->originY + CurClip->curY;
		if(CurClip->attrFlags & 2) {
			for(i = LineBufDrawn; i < LineBufDrawn + ((CurClip->cols - breakPos) >> 1); i++)
				(*CON_PutCharFn)(' ', i + CurClip->originX, row);
		}
		savedFg = CurClip->fgColor;
		for(i = LineBufDrawn; i < breakPos; i ++) {
			CurClip->fgColor = LineBufColors[i];
			(*CON_PutCharFn)(LineBufChars[i], i + CurClip->originX, row);
		}
		CurClip->fgColor = savedFg;
		if(LineBufDrawn < breakPos)
			CurClip->curX = breakPos;
		if((CurClip->attrFlags & 1) && CurClip->cols == LineBufLen && ch >= 0) {
			/*carry the cut word to the start of the buffer (dropping the space)*/
			if(LineBufChars[breakPos] == ' ')
				breakPos ++;
			for(i = breakPos; i <= LineBufLen; i ++) {
				LineBufChars[i - breakPos] = LineBufChars[i];
				LineBufColors[i - breakPos] = LineBufColors[i];
			}
			LineBufDrawn = 0;
			LineBufLen = i - breakPos;
			if(ch == ' ')
				LineAcceptSpace = 0;
		} else if(ch >= 0) {
			/*new empty line, then the char that did not fit (if any)*/
			LineBufLen = LineBufDrawn = 0;
			if(ch >= 0 && ch != '\n') {
				LineBufColors[LineBufLen] = CurClip->fgColor;
				LineBufChars[LineBufLen] = ch;
				LineBufLen ++;
			}
		}
		if(ch >= 0) {
			CurClip->curX = 0;
			if(CurClip->curY < CurClip->rows - 1) {
				CurClip->curY ++;
			} else if(CurClip->attrFlags & 0x20) {
				TextIdle = 1;
				(*CON_ScrollUpFn)(CurClip->originX, CurClip->originY, CurClip->cols, CurClip->rows);
				TextIdle = 0;
			}
		} else {
			/*flush: the line stays open at the cursor*/
			LineBufDrawn = LineBufLen = CurClip->curX;
			InHiddenSection = 0;
		}
	} else if(!(CurClip->attrFlags & 1) || ch != ' ' || LineAcceptSpace) {
		/*buffer the char (leading spaces of a wrapped line are dropped)*/
		LineBufColors[LineBufLen] = CurClip->fgColor;
		LineBufChars[LineBufLen] = ch;
		LineBufLen ++;
	}
	if(ch != ' ')
		LineAcceptSpace = 1;
	TextIdle = 1;
}

/*output ch count times (no escapes)*/
/*C_2238_30AF*/CON_PutRepeat(int16 count, int16 ch) {
	int16 i;

	for(i = 0; i < count; i ++)
		CON_putch0(ch);
}

/*output string str (no escapes)*/
/*C_2238_30C9*/CON_PutsRaw(unsigned char far *str) {
	while(*str)
		CON_putch0(*str++);
}

/*convert number to a string in str, in base |base| (2~36); a negative
base means signed (number taken as a signed int, '-' counts in maxLen).
at most maxLen digits are kept (the most significant ones are lost).
returns the length*/
/*C_2238_30EB*/CON_itoa(uint16 number, char far *str, int16 maxLen, int16 base) {
	int16 len;
	char digits[8];
	int16 i, isNegative, digit;

	isNegative = 0;
	len = 0;
	if(base < 0) {
		base = -base;
		if(number > 0x7fff) {
			number = -number;
			isNegative = 1;
			maxLen --;
		}
	}
	/*digits, least significant first*/
	if(base) {
		while(number && len < maxLen) {
			digit = number % base;
			digits[len++] = (digit < 10)?(digit + '0'):(digit - 10 + 'A');
			number /= base;
		}
	}
	if(len == 0)
		digits[len++] = '0';
	if(isNegative)
		digits[len++] = '-';
	/*reverse*/
	for(i = 0; i < len; i ++)
		str[i] = digits[len - i - 1];
	str[i] = 0;
	return len;
}

/*character output with the in-text escapes (used by CON_puts/CON_printf,
so by all game messages, books, signs and conversations):
  #c   print VarInt[c] (c = '0'~'9' or 'A'~'Z')
  $c   print VarStr[c]
  /    following letters printed only if the last number was 1 (singular)
  \    following letters printed only if the last number was not 1 (plural)
       (both end at the next non letter)
  ^c   print control char c & 0x1f (special glyphs)
  @    following word is a keyword: highlighted (ColorRed) if help is on
  %    cancels a pending escape (no effect by itself)
  *    flush and wait for a key
  `    capitalize the next char*/
/*C_2238_3197*/CON_putch(int16 ch) {
	int16 varIdx;
	char numBuf[8];

	if(!TextOn)
		return;
	switch(ch) {
		case '#': case '$': case '%':
		case '/': case '@': case '\\':
		case '^': PendingEscape = ch; return;
		case '*':
			CON_putch0(-1);
			PromptCh = 1;
			CON_WaitKeyOrClick();
			PromptCh = 5;
		return;
		case '`': CapitalizeNext = 1; return;
	}
	switch(PendingEscape) {
		case '#':
			ch = toupper(ch);
			if(ch >= '0' && ch <= '9')
				varIdx = ch - '0';
			else if(ch >= 'A' && ch <= 'Z')
				varIdx = (ch - 'A') + 10;
			else
				varIdx = 0;
			LastNumberPlural = (VarInt[varIdx] != 1);
			CON_itoa(VarInt[varIdx], numBuf, 6, -10);
			CON_PutsRaw(numBuf);
			PendingEscape = 0;
		break;
		case '$':
			ch = toupper(ch);
			if(ch >= '0' && ch <= '9')
				varIdx = ch - '0';
			else if(ch >= 'A' && ch <= 'Z')
				varIdx = (ch - 'A') + 10;
			else
				varIdx = 0;
			CON_PutsRaw(VarStr[varIdx]);
			PendingEscape = 0;
		break;
		case '/':
			if(!isalpha(ch) || !LastNumberPlural) {
				CON_putch0(ch);
				if(!isalpha(ch))
					PendingEscape = 0;
			}
		break;
		case '\\':
			if(!isalpha(ch) || LastNumberPlural) {
				CON_putch0(ch);
				if(!isalpha(ch))
					PendingEscape = 0;
			}
		break;
		case '^':
			if(CurClip->attrFlags & 8)
				CON_putch0((ch & 0x1f) | 0x80);
			else
				CON_putch0(ch & 0x1f);
			PendingEscape = 0;
		break;
		case '@':
			/*keyword ends with punctuation/space*/
			if(strchr(/*04B5*/" ,.:;!?'-\"\n", ch)) {
				PendingEscape = 0;
				CurClip->fgColor = ColorText;
				CON_putch0(ch);
			} else {
				if(HelpOn)
					CurClip->fgColor = ColorRed;
				CON_putch0(ch);
			}
		break;
		default:
			CON_putch0(ch);
	}
}

/*output string str with escapes (CON_putch)*/
/*C_2238_33D6*/CON_puts(unsigned char far *str) {
	while(*str)
		CON_putch(*str++);
}

/*minimal printf: %[-][0][width] followed by d (signed), o, x, u, c, s;
an unknown conversion prints its char raw ("%>" prints the '>' prompt);
the numbers set LastNumberPlural (plural flag); literal chars go through
CON_putch() so escapes apply, the converted values (%s too) also do*/
/*C_2238_33F8*/cdecl CON_printf(unsigned char far *format, ...) {
	VA_LIST argp;
	uint16 num;
	char numBuf[8];
	int16 c, fmtCh;
	int16 fieldWidth, len, padCh, leftAlign;
	char *str;

	if(!TextOn)
		return;
	VA_START(argp, format);
	while(*format) {
		c = *format++;
		fieldWidth = len = leftAlign = 0;
		padCh = ' ';
		str = numBuf;
		switch(c) {
		case '%':
			fmtCh = *format++;
			while(fmtCh == '0' || fmtCh == '-') {
				switch(fmtCh) {
					case '0': padCh = '0'; break;
					case '-': leftAlign = 1; break;
				}
				fmtCh = *format++;
			}
			while(fmtCh >= '0' && fmtCh <= '9') {
				fieldWidth *= 10;
				fieldWidth += fmtCh - '0';
				fmtCh = *format++;
			}
			switch(tolower(fmtCh)) {
				case 'd':
					num = VA_INT(argp);
					len = CON_itoa(num, numBuf, 6, -10);
					LastNumberPlural = (num != 1);
				break;
				case 'o':
					num = VA_INT(argp);
					len = CON_itoa(num, numBuf, 6, 8);
					LastNumberPlural = (num != 1);
				break;
				case 'x':
					num = VA_INT(argp);
					len = CON_itoa(num, numBuf, 4, 16);
					LastNumberPlural = (num != 1);
				break;
				case 'u':
					num = VA_INT(argp);
					len = CON_itoa(num, numBuf, 5, 10);
					LastNumberPlural = (num != 1);
				break;
				case 'c':
					numBuf[0] = VA_CHAR(argp);
					numBuf[1] = 0;
				break;
				case 's':
					str = VA_STR(argp);
					len = strlen(str);
				break;
				default:
					numBuf[0] = 0;
					CON_putch0(fmtCh);
			}
			if(leftAlign)
				CON_puts(str);
			CON_PutRepeat(fieldWidth - len, padCh);
			if(leftAlign == 0)
				CON_puts(str);
		break;
		default:
			CON_putch(c);
		}
	}
}
