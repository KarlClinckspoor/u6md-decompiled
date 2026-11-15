/*
 * Martian Dreams - GAME.EXE segment 02CD: the main module.
 * From Savage Empire's GAME.EXE main.c (which comes from Ultima VI's).
 *
 * main() parses the configuration (CONFIG.MD + command line), sets up the
 * memory manager (MEM_init) and a work area for the .lzc loader, the mouse
 * and the screen surfaces (scanline offset tables), loads the graphic driver,
 * the palette, the font (system.lzc), the main screen, the tile tables and
 * animation data, loads the saved game and runs the game loop. Without a
 * character it goes back to MARTIAN.EXE.
 * Also here: InitError() (the fatal "initialization problem" exit), the
 * allocator __MemAlloc() (zeroed blocks from MEM_alloc), the loader callback
 * given to the graphic driver (LoadFileGr), the OSI file error handler
 * (FileErrorHandler) and the VGA palette cycling (PaletteAnimation).
 */
#include <dos.h>
#include <stdlib.h>
#include <process.h>
#include <ctype.h>

typedef int int16;
typedef unsigned uint16;
typedef long int32;
typedef unsigned long uint32;

union tHandler {
	struct {
		uint16 ofs, seg;
	} iii;
	void far *p;
	void far *(*f)();
};

/*text window (Ultima VI's)*/
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
	unsigned char textFg, textBg;
	unsigned char xorMode;
	unsigned char unknown0d;
	unsigned char animOn;
	unsigned char toBackBuffer;
	void (*pfnDiskConfirm)();/*_10*/
	unsigned char reserved14[8];
	void far *pWorkBuf;/*_1c*/
	void far *pDriverMem;/*_20*/
};

/*a palette entry*/
struct tRGB {
	unsigned char content[3];
};

/*the graphic driver: GrDriver's offset selects the entry*/
#define GR_SHUTDOWN()             (GrDriver.iii.ofs = 0x03, (*GrDriver.f)())
#define GR_INIT_PALETTE()         (GrDriver.iii.ofs = 0x24, (*GrDriver.f)())
#define GR_CYCLE_PALETTE(a)       (GrDriver.iii.ofs = 0x27, (*GrDriver.f)(a))
#define GR_INIT_MEMORY(seg,size)  (GrDriver.iii.ofs = 0x2a, (*GrDriver.f)(seg,size))
/*0x30: draw a bitmap on a surface (surface, 0, 0, bitmap, 0)*/
#define GR_DRAW_BITMAP(s,a,b,p,c) (GrDriver.iii.ofs = 0x30, (*GrDriver.f)(s,a,b,p,c))

/*declared before its definition: main stores its segment as a relocated
  immediate (mov [x+2],seg), not from CS*/
void FileErrorHandler();

/*OSI file routines (asm)*/
extern union tHandler OSI_ErrorHandler;
extern int16 OSI_ErrorCode;
extern int16 OSI_ErrorFunc;
extern char *OSI_ErrorFile;
extern int16 OSI_open(char *);
extern OSI_close(int16);
extern int32 OSI_read(int16, int32, int32, void far *);
extern int32 OSI_fileSize(int16);
extern int16 OSI_Started;
extern OSI_lowInit();
extern int16 OSI_rand(int16, int16);/*OSI_rand(lo, hi)*/
extern OSI_randomize(int16);
extern int16 KBD_GetKey(int16);
extern cdecl MOUSE_INIT(struct tMouse *);
extern cdecl MOUSE_POINTER(void far *, int16);
extern cdecl MOUSE_SHOW();

/*other game modules*/
extern int16 HatchingDisabled;
extern int16 SoundFlag;/*music on*/
extern char AvatarPortrait;/*0 = no character*/
extern char AnimateAtPrompt;
extern struct tClipInfo *CurClip;
extern GameLoop();
extern far CON_UpdateMousePointer();
extern GAME_Load();
extern int16 CON_getch();
extern CON_createClip(struct tClipInfo *, int16, int16, int16, int16);
extern CON_setClip(struct tClipInfo *);
extern cdecl CON_printf(char far *, ...);
extern MAP_InitChunkCache();
extern OBJ_DeleteTmpBlocks();
extern OBJ_InitLists();
extern MUS_InitDriver();
extern MUS_PlaySong(int16);
extern MUS_PlayAreaSong();
extern InitGraphicsDriver(int16);
extern AllocPaperDoll();
extern DISK_init();
extern DISK_full();

/*library*/
extern int16 cdecl soundCard;
extern int16 cdecl osiError;
extern char far * cdecl lzcWorkArea;
extern cdecl MEM_init(int16);
extern cdecl MEM_shutdown(int16);
extern void far * cdecl MEM_alloc(int32 size, int16 flags);
extern cdecl MEM_free(void far *);
extern int32 cdecl MEM_largestFree(int16);
extern cdecl ShutdownSound(void);
extern cdecl UnhookAll(void);
extern int32 LIB_entrySize(char far *name, int16 index);
extern void far *LZC_loadType(char far *name, int16 index, void far *out, int16 memflags, char far *work);
extern int32 PtrToLinear(void far *);
extern void far *LinearToPtr(int32);

/*uninitialised data*/
/*3232*/extern struct tSurface Screen;/*the screen's surface (scanline table ScreenScanlines), for the mouse code*/
/*3240*/extern unsigned char AnimFrameMask[0x20];/*animdata: the frame mask of each animated tile*/
/*3260*/extern int16 GrMemoryKB;/*memory given to the graphic driver for the tiles, in KB*/
extern char far *D_3262;/*0x100 bytes*/
/*3E66*/extern struct tMouse Mouse;/*the mouse state*/
/*3E80*/extern char far *LzcWorkBuf;/*work area of the .lzc loader (0x3500 bytes)*/
extern char far *SchedIndex;/*0x100 bytes*/
extern char far *PathData;/*0x100 bytes*/
extern char far *NPCFlag;/*0x100 bytes*/
extern char far *NPCMode;/*0x100 bytes*/
/*4D14*/extern uint16 ScreenScanlines[200];
extern char far *Level;/*0x100 bytes*/
extern char far *Schedule;/*3000 bytes*/
extern char far *ScratchBuf;/*0x7900 bytes, also the map view back buffer*/
/*50C4*/extern uint16 GrMemorySeg;/*segment of the graphic driver's memory*/
/*50C6*/extern char far *PointerShapes;/*the mouse pointer shapes (mdcursor.ptr)*/
extern char far *Leader;/*0x100 bytes*/
extern char far *NPCComMode;/*0x100 bytes*/
/*54B2*/extern char far *ScratchBufHigh;/*ScratchBuf + 0x36b0 (after the room of a conversation script): pictures, look.lzc*/
/*54B6*/extern uint32 GrMemorySize;/*memory given to the graphic driver*/
/*6DBA*/extern char far *TileFlag;/*tileflag: 0x03 light, 0x04 opaque, 0x08 window, 0x10 drawn over sprites, 0x40 double height, 0x80 double width*/
/*6EBE*/extern char far *BaseTile;/*the first tile of each object type*/
/*6EC2*/extern char far *TalkBuf;/*ScratchBuf again: the conversation script, books*/
/*7606*/extern char far *TypeWeight;/*tileflag: the weight of each object type*/
/*760A*/extern char far *TileFlag2;/*tileflag: the second flags of each tile*/
/*770E*/extern int16 AnimFirstFrame[0x20];/*animdata: the first frame of each animated tile*/
/*774E*/extern uint16 MapScanlines[176];/*the map view buffer's scanline offsets*/
/*9CAE*/extern struct tSurface MapSurface;/*the map view buffer's surface (scanline table MapScanlines)*/
/*9CBC*/extern void huge *GrMemory;/*the graphic driver's memory*/
/*9CC2*/extern int16 NumAnimData;/*animdata: the number of animated tiles*/
/*B4C4*/extern char far *TerrainType;/*tileflag: the terrain of each tile (bits 4-7 movement cost, 8 damaging)*/
/*B4C8*/extern int16 AnimTile[0x20];/*animdata: the tile of each animated tile*/
extern int16 far *OrigShapeType;/*0x100 words*/
/*B50C*/extern unsigned char AnimFrameShift[0x20];/*animdata: the frame shift of each animated tile*/
/*B52C*/extern struct tRGB far *VGAPalette;/*mdpal*/
extern struct tGr far *GrInfo;
extern union tHandler GrDriver;
/*BA7E*/extern void far *ScreenParts;/*mdscreen.lzc entry 2: drawn in modes 0/1 (the command and combat marks) and 2/3 (the sky)*/
extern char far *ChunkIndexBuf;/*0x600 bytes*/
/*EBD0*/extern struct tClipInfo TextWindows[4];/*[0] whole screen, [1] status panel, [2] over the map view, [3] message area*/

/*====---- ----====*/
/*00A8*/int16 Unused00A8 = 0x2000;
/*00AA*/int16 IsZFile = 0;/*Ultima VI's LoadFile() flag (LZW files); not used in this module*/
/*00AC*/char *CompressedFileList[] = {/*LOW.ASM's list of compressed files*/
	""
};
/*00AE*/char DirIncrX[] = { 0, 1, 1, 1, 0,-1,-1,-1};
/*00B6*/char DirIncrY[] = {-1,-1, 0, 1, 1, 1, 0,-1};
/*00BE*/char *DirNames[] = {
	"North",
	"Northeast",
	"East",
	"Southeast",
	"South",
	"Southwest",
	"West",
	"Northwest"
};
/*00CE*/int16 Unused00CE = 0;
/*00D0*/int16 Unused00D0 = 0;
/*00D2*/int16 IgnoreShortRead = 1;/*1: ignore "short read" errors (end of file)*/
/*00D4*/int16 GrAdapter = 0;/*graphic adapter: 0 VGA/MCGA, 1 EGA, 2 Tandy (3 CGA, 4 Hercules: Ultima VI's)*/
/*00D6*/int16 MusicAdapter = 0;/*1: AdLib or Roland, 0: none*/
/*00D8*/int16 GrDriverOption = 0;/*'$' config flag*/
/*00DA*/unsigned char MouseOn = 1;
/*00DB*/char ConsoleReady = 0;/*1 once the console can display messages (error handler)*/
/*00DC*/unsigned char PaletteCycle = 0;/*palette cycling counter*/
/*00DD*/char GrAdapterLetters[] = "vetch";/*config letters for GrAdapter*/
/*00E3*/char MusicAdapterLetters[] = "narp";/*config letters (None/Adlib/Roland/Pc speaker); indexed by MusicAdapter, so only n/a come out*/
/*====---- ----====*/

/*fatal error during the initialization: back to text mode, print the
  message with the BIOS and exit to DOS (exit code 3)*/
/*C_02CD_000D*/InitError() {
	char *msg1, *msg2;
	union REGS regs, out;
	register char *msg3;
	register char *p;

	msg1 = "Initialization problem.  Check your configuration information\r\n";
	msg2 = "and free memory as shown in your reference card.\r\n";
	msg3 = "(It may be a good idea to turn your machine off, then back on again now.)\r\n";
	MEM_shutdown(8);
	UnhookAll();
	/*text mode 3, page 0, cursor at 0,0*/
	regs.x.ax = 0x3;
	int86(0x10, &regs, &out);
	regs.x.ax = 0x500;
	int86(0x10, &regs, &out);
	regs.x.ax = 0x200;
	regs.x.bx = 0;
	regs.x.dx = 0;
	int86(0x10, &regs, &out);
	/*teletype output*/
	regs.x.ax = 0xe00;
	regs.x.bx = 1;
	p = msg1;
	while(*p) {
		regs.h.al = *p++;
		int86(0x10, &regs, &out);
	}
	p = msg2;
	while(*p) {
		regs.h.al = *p++;
		int86(0x10, &regs, &out);
	}
	p = msg3;
	while(*p) {
		regs.h.al = *p++;
		int86(0x10, &regs, &out);
	}
	/*terminate, exit code 3*/
	regs.x.ax = 0x4c03;
	int86(0x21, &regs, &out);
}

/*allocate "size" zeroed bytes with MEM_alloc (InitError if there is not
  enough memory)*/
/*C_02CD_011A*/static __MemAlloc(void far **pptr, uint32 size) {
	uint32 offset;

	if(MEM_largestFree(0) < size) {
		InitError();
		return;
	}

	*pptr = MEM_alloc(size, 2);
	if(*pptr == 0)
		InitError();
	/*zero memory*/
	for(offset = 0; offset < size; offset ++)
		*((char huge *)*pptr + offset) = 0;
}

/*file loader callback given to the graphic driver: the file name is a far
  string (copied locally); isZ: an .lzc file, loaded with LZC_loadType*/
/*C_02CD_01A6*/void far *LoadFileGr(char far *fn, void far *pBuffer, int16 isZ) {
	void far *pRetBuff;
	char fname[14];
	int16 i, fileHandle;

	for(i = 0; i < 13; i++)
		fname[i] = fn[i];

	pRetBuff = pBuffer;
	if(isZ == 0) {
		fileHandle = OSI_open(fname);
		OSI_read(fileHandle, 0, OSI_fileSize(fileHandle), pBuffer);
		OSI_close(fileHandle);
	} else {
		LZC_loadType(fname, 0, pBuffer, 0, LzcWorkBuf);
	}
	return 0;
}

/*OSI file error handler (installed in OSI_ErrorHandler by main).
  Called by the OSI file functions with OSI_ErrorCode = DOS error code (-1 = fewer
  bytes read/written than asked) and OSI_ErrorFunc = function (1 open, 2 close,
  3 create, 4 delete, 5 read, 6 write, 7 rename, 8 copy).
  Leaving OSI_ErrorCode != 0 makes the operation fail, clearing it means "retry".
  A short write is reported as "disk full", other errors are displayed and
  a key is awaited: <CTRL+C>/<ESC> give up, any other key retries.*/
/*C_02CD_0228*/void FileErrorHandler() {
	int16 key;
	struct tClipInfo *savedClip;

	/*end of file reached: not an error*/
	if(OSI_ErrorCode == -1 && OSI_ErrorFunc == 5/*OSI_read*/)
		if(IgnoreShortRead)
			return;
	/*deleting a file that does not exist: not an error*/
	if(OSI_ErrorCode == 2 && OSI_ErrorFunc == 4/*OSI_delete*/)
		return;
	if(ConsoleReady) {
		savedClip = CurClip;
		CON_setClip(&(TextWindows[3]));
		if(OSI_ErrorCode == -1 && OSI_ErrorFunc == 6/*OSI_write*/) {
			DISK_full();
		} else {
			CON_printf("DISK ERROR!!!! Function: %d, error: %d, file: ", OSI_ErrorFunc, OSI_ErrorCode);
			CON_printf(OSI_ErrorFile);
			CON_printf("\n");
			/*wait for a key (flushing the keyboard buffer)*/
			while(!(key = KBD_GetKey(1)));
		}
		CON_setClip(savedClip);
	}
	/*not <CTRL+C>/<ESC>: retry*/
	if(key != 3 && key != 27)
		OSI_ErrorCode = 0;
}

/*give all the remaining memory (but 16 bytes) to the graphic driver (which
  loads its tile graphics there), then load the tile animation table
  "animdata" and the tile/type property tables "tileflag"*/
/*C_02CD_02D2*/static InitGraphicsData() {
	uint16 aSegment;
	union tHandler memBP;/*the block, paragraph aligned (unused)*/
	int32 linearAddr;
	void far *pBlock;
	int16 animFile, flagFile;

	GrMemorySize = MEM_largestFree(0) - 0x10;
	pBlock = MEM_alloc(GrMemorySize, 2);
	linearAddr = ((uint32)FP_SEG(pBlock) << 4) + FP_OFF(pBlock);
	if(linearAddr & 0xf) {
		GrMemorySize -= 0x10 - (linearAddr & 0xf);
		linearAddr = (linearAddr & ~0xf) + 0x10;
		memBP.iii.seg = linearAddr >> 4;
		memBP.iii.ofs = linearAddr & 0xf;
	}
	aSegment = linearAddr >> 4;
	GR_INIT_MEMORY(aSegment, (uint16)(GrMemorySize >> 4));
	GrMemoryKB = GrMemorySize >> 10;
	GrMemory = GrInfo->pDriverMem;
	if(GrMemory == 0)
		InitError();
	GrMemorySeg = ((int32)GrMemory >> 16) + (((int32)GrMemory & 0xffff) >> 4);

	/*animated tiles: count, tile number, first frame, frame mask, shift*/
	animFile = OSI_open("animdata");
	OSI_read(animFile, 0, 2, &NumAnimData);
	OSI_read(animFile, -1, 0x20 * sizeof(int16), AnimTile);
	OSI_read(animFile, -1, 0x20 * sizeof(int16), AnimFirstFrame);
	OSI_read(animFile, -1, 0x20, AnimFrameMask);
	OSI_read(animFile, -1, 0x20, AnimFrameShift);
	OSI_close(animFile);

	/*0x800 tiles terrain flags, 0x800 tiles flags, 0x400 types weight,
	  0x800 tiles flags #2*/
	flagFile = OSI_open("tileflag");
	OSI_read(flagFile, 0, 0x800, TerrainType);
	OSI_read(flagFile, -1, 0x800, TileFlag);
	OSI_read(flagFile, -1, 0x400, TypeWeight);
	OSI_read(flagFile, -1, 0x800, TileFlag2);
	OSI_close(flagFile);
}

/*display the main screen background (mdscreen.lzc entry 0, in a temporary
  block), then load its entry 2 (ScreenParts)*/
/*C_02CD_0509*/ShowMainScreen() {
	void far *pixmap;

	pixmap = MEM_alloc(LIB_entrySize("mdscreen.lzc", 0), 0);
	if(pixmap == 0)
		InitError();
	if(LZC_loadType("mdscreen.lzc", 0, pixmap, 0, LzcWorkBuf) == 0)
		InitError();
	GR_DRAW_BITMAP(&Screen, 0, 0, pixmap, 0);
	MEM_free(pixmap);
	ScreenParts = LZC_loadType("mdscreen.lzc", 2, 0L, 0, LzcWorkBuf);
}

/*VGA only: send the whole 256 colors palette VGAPalette to the DAC
  (BIOS int 10h/1012h)*/
/*C_02CD_05AC*/SetVGAPalette() {
	union REGS regs;
	struct SREGS sregs;

	if(GrAdapter != 0)
		return;
	regs.x.ax = 0x1012;
	regs.x.bx = 0;
	regs.x.cx = 0x100;
	regs.x.dx = FP_OFF(VGAPalette);
	segread(&sregs);
	sregs.es = FP_SEG(VGAPalette);
	int86x(0x10, &regs, &regs, &sregs);
}

/*load and set the palette ("mdpal" for VGA, by the driver otherwise)*/
/*C_02CD_05F7*/LoadPalette() {
	int16 fileHandle;

	if(GrAdapter == 0) {
		__MemAlloc(&VGAPalette, 0x100 * 3);
		fileHandle = OSI_open("mdpal");
		OSI_read(fileHandle, 0, 0x100 * 3, VGAPalette);
		OSI_close(fileHandle);
		SetVGAPalette();
	} else {
		GR_INIT_PALETTE();
	}
}

/*palette cycling for fire/water/lava colors.
  VGA: color 0xff pulses in grey levels 12..19, colors 0xe0-0xe7 and
  0xe8-0xef rotate by one step per call, colors 0xf0-0xf3, 0xf4-0xf7 and
  0xf8-0xfb every second call (values taken from the VGAPalette palette).
  Other adapters: done by the graphic driver*/
/*C_02CD_064E*/PaletteAnimation() {
	int16 step;
	union REGS regs;
	int16 rgb;

	if(GrAdapter == 0) {
		/*triangle wave 0..7..0 -> grey level 12..19*/
		step = (PaletteCycle ++) & 0xf;
		if(step > 7)
			step = 0xf - step;
		step += 0xc;
		/*BIOS: set DAC register bx to (dh,ch,cl)*/
		regs.x.ax = 0x1010;
		regs.x.bx = 0xff;
		regs.h.dh =
		regs.h.ch =
		regs.h.cl = step;
		int86(0x10, &regs, &regs);
		/*0x3c8: DAC write index, 0x3c9: r,g,b*/
		for(step = 0; step < 8; step ++) {
			outportb(0x3c8, ((PaletteCycle - step) & 7) + 0xe0);
			for(rgb = 0; rgb < 3; rgb ++)
				outportb(0x3c9, VGAPalette[0xe7 - step].content[rgb]);

			outportb(0x3c8, ((PaletteCycle - step) & 7) + 0xe8);
			for(rgb = 0; rgb < 3; rgb ++)
				outportb(0x3c9, VGAPalette[0xef - step].content[rgb]);
		}
		for(step = 0; step < 4; step ++) {
			outportb(0x3c8, (((PaletteCycle >> 1) - step) & 3) + 0xf0);
			for(rgb = 0; rgb < 3; rgb ++)
				outportb(0x3c9, VGAPalette[0xf3 - step].content[rgb]);

			outportb(0x3c8, (((PaletteCycle >> 1) - step) & 3) + 0xf4);
			for(rgb = 0; rgb < 3; rgb ++)
				outportb(0x3c9, VGAPalette[0xf7 - step].content[rgb]);

			outportb(0x3c8, (((PaletteCycle >> 1) - step) & 3) + 0xf8);
			for(rgb = 0; rgb < 3; rgb ++)
				outportb(0x3c9, VGAPalette[0xfb - step].content[rgb]);
		}
	} else {
		GR_CYCLE_PALETTE((unsigned char)(PaletteCycle ++));
	}
}

/*build the configuration string (same format as CONFIG.MD): adapter
  letter, 'M'ouse/'K'eyboard, music letter*/
/*C_02CD_0836*/BuildConfigString(char *dst) {
	*(dst++) = GrAdapterLetters[GrAdapter];
	*(dst++) = MouseOn?'M':'K';
	*(dst++) = MusicAdapterLetters[MusicAdapter];
	*dst = 0;
}

/*parse a configuration string (CONFIG.MD or a command line argument):
  V/E/T graphic adapter, M/K mouse or keyboard, N/A/R/P music (none, AdLib,
  Roland, the third driver), '$' see GrDriverOption*/
/*C_02CD_086B*/cfg_parse(unsigned char *str) {
	while(*str) {
		switch(toupper(*str++)) {
			case '$': GrDriverOption = 1; break;
			case 'V': GrAdapter = 0; break;
			case 'E': GrAdapter = 1; break;
			case 'T': GrAdapter = 2; break;
			case 'M': MouseOn = 1; break;
			case 'K': MouseOn = 0; break;
			case 'N': MusicAdapter = 0; soundCard = 0; SoundFlag = 0; break;
			case 'A': MusicAdapter = 1; soundCard = 2; SoundFlag = 1; break;
			case 'R': MusicAdapter = 1; soundCard = 1; SoundFlag = 1; break;
			case 'P': MusicAdapter = 0; soundCard = 3; SoundFlag = 1; break;
		}
	}
}

/*program entry: initialization, game loop, exit*/
/*C_02CD_0942*/cdecl main(int16 argc, char **argv) {
	char cfgString[7];
	int16 hercRow;
	int32 linearAddr;
	char far *pFont;
	unsigned now[2];
	int16 i, fileHandle;

	/*seed the random generator from the time of day*/
	gettime((struct time *)now);
	MEM_init(8);
	OSI_randomize(now[1]);
	for(i = 0; i < now[1]; i++)
		OSI_rand(0, 2);
	/*config: CONFIG.MD (6 characters) then the command line arguments*/
	fileHandle = OSI_open("config.md");
	if(fileHandle >= 0) {
		OSI_read(fileHandle, 0, 6, cfgString);
		cfgString[6] = 0;
		cfg_parse(cfgString);
		OSI_close(fileHandle);
	}
	for(i = 1; i < argc; i ++)
		cfg_parse(argv[i]);
	/*the .lzc loader's work area*/
	LzcWorkBuf = MEM_alloc(0x3500, 2);
	if(LzcWorkBuf == 0)
		InitError();
	lzcWorkArea = LzcWorkBuf;

	DISK_init();
	OSI_lowInit();
	/*install the file error handler*/
	OSI_ErrorHandler.f = FileErrorHandler;
	/*the surfaces (scanline offset tables) are only used by the mouse code:
	  Screen = the 320x200 screen, MapSurface = the map view*/
	Screen.pScanlines = ScreenScanlines;
	Screen.left = 0;
	Screen.right = 319;
	Screen.top = 0;
	Screen.bottom = 199;

	MapSurface.pScanlines = MapScanlines;
	MapSurface.left = 8;
	MapSurface.right = 167;
	MapSurface.top = 16;
	MapSurface.bottom = 159;

	if(GrAdapter == 0) {/*VGA*/
		Screen.segment = 0xa000;
		ScreenScanlines[0] = 0;
		for(i = 1; i < 200; i++)
			ScreenScanlines[i] = ScreenScanlines[i - 1] + 0x140;
	} else
	if(GrAdapter == 1) {/*EGA*/
		Screen.segment = 0xa000;
		ScreenScanlines[0] = 0;
		for(i = 1; i < 200; i++)
			ScreenScanlines[i] = ScreenScanlines[i - 1] + 40;
		Screen.pScanlines = ScreenScanlines;
		MapScanlines[0] = 0;
		for(i = 1; i < 176; i++)
			MapScanlines[i] = MapScanlines[i - 1] + 40;
		MapSurface.segment = 0xa200;
	} else
	if(GrAdapter == 2) {/*TANDY*/
		Screen.segment = 0xb800;
		ScreenScanlines[0] = 0;
		ScreenScanlines[1] = 0x2000;
		ScreenScanlines[2] = 0x4000;
		ScreenScanlines[3] = 0x6000;
		for(i = 4; i < 200; i++)
			ScreenScanlines[i] = ScreenScanlines[i - 4] + 0xa0;
		Screen.pScanlines = ScreenScanlines;
	} else
	if(GrAdapter == 3) {/*CGA*/
		Screen.segment = 0xb800;
		ScreenScanlines[0] = 0;
		ScreenScanlines[1] = 0x2000;
		for(i = 2; i < 200; i++)
			ScreenScanlines[i] = ScreenScanlines[i - 2] + 80;
		Screen.pScanlines = ScreenScanlines;
	} else
	if(GrAdapter == 4) {/*HERCULES*/
		Screen.segment = 0xb000;
		/*200 lines stretched to 300 (x1.5), 4 interleaved banks
		  of 90 bytes per line*/
		for(i = 0; i < 200; i++) {
			hercRow = (i * 3 >> 1) + 0xf;
			ScreenScanlines[i] = ((hercRow & 3) << 13) + (hercRow >> 2) * 90 + 5;
		}
		Screen.pScanlines = ScreenScanlines;
	}
	if(MouseOn) {
		/*background save area and pointer shapes*/
		__MemAlloc(&Mouse.saveBuf, 0x400);
		__MemAlloc(&PointerShapes, 0x800);
		LZC_loadType("mdcursor.ptr", 0, PointerShapes, 0, LzcWorkBuf);
		if(osiError)
			InitError();
		Mouse.shapes = PointerShapes;
		Mouse.surface = &Screen;
		MOUSE_INIT(&Mouse);
		Mouse.onMove = CON_UpdateMousePointer;
		MOUSE_POINTER(PointerShapes, 0);
	}
	InitGraphicsDriver(GrAdapter);/*init gr*/
	LoadPalette();
	/*font*/
	__MemAlloc(&pFont, LIB_entrySize("system.lzc", 3));
	GrInfo->font = LZC_loadType("system.lzc", 3, pFont, 0, LzcWorkBuf);
	ConsoleReady = 1;
	/*text windows: [0] whole screen, [1] status area, [3] message area*/
	CON_createClip(&(TextWindows[0]), 0, 0, 39, 24);
	CON_setClip(&(TextWindows[0]));
	CurClip->attrFlags |= 1;
	CurClip->attrFlags |= 0x20;
	CurClip->attrFlags |= 4;
	ShowMainScreen();
	CON_createClip(&(TextWindows[1]), 23, 1, 38, 13);
	CON_createClip(&(TextWindows[3]), 23, 16, 38, 23);
	CON_createClip(&(TextWindows[2]), 2, 3, 19, 18);
	CON_setClip(&(TextWindows[3]));
	/*message area: word wrap, scroll, capitalize*/
	CurClip->attrFlags |= 1;
	CurClip->attrFlags |= 0x20;
	CurClip->attrFlags |= 4;
	MUS_InitDriver();

	__MemAlloc(&TerrainType, 0x800);
	__MemAlloc(&TileFlag, 0x800);
	__MemAlloc(&BaseTile, 0x800);
	__MemAlloc(&TypeWeight, 0x400);
	__MemAlloc(&TileFlag2, 0x800);
	__MemAlloc(&Schedule, 3000);
	__MemAlloc(&NPCMode, 0x100);
	__MemAlloc(&D_3262, 0x100);
	__MemAlloc(&NPCComMode, 0x100);
	__MemAlloc(&SchedIndex, 0x100);
	__MemAlloc(&Leader, 0x100);
	__MemAlloc(&NPCFlag, 0x100);
	__MemAlloc(&OrigShapeType, 0x100 * sizeof(int16));
	__MemAlloc(&Level, 0x100);
	__MemAlloc(&PathData, 0x100);
	__MemAlloc(&ChunkIndexBuf, 0x600);

	/*ScratchBuf is also the 176x176 (0x7900 bytes) map view back buffer:
	  MapScanlines gives the offsets of its inner part (8 pixels border)*/
	__MemAlloc(&ScratchBuf, 0x7900);
	TalkBuf = GrInfo->pWorkBuf = ScratchBuf;
	linearAddr = PtrToLinear(ScratchBuf);
	ScratchBufHigh = LinearToPtr(linearAddr + 0x36b0);
	if(GrAdapter == 0) {/*VGA*/
		MapSurface.segment = FP_SEG(ScratchBuf);
		MapScanlines[0] = FP_OFF(ScratchBuf);
		for(i = 1; i < 0xa0; i++)
			MapScanlines[i] = MapScanlines[i - 1] + 0xb0;
	} else
	if(GrAdapter == 2) {/*TANDY*/
		MapSurface.segment = FP_SEG(ScratchBuf);
		MapScanlines[0] = FP_OFF(ScratchBuf);
		for(i = 1; i < 0xa0; i++)
			MapScanlines[i] = MapScanlines[i - 1] + 0x80;
	} else
	if(GrAdapter == 3 || GrAdapter == 4) {/*CGA/HERCULES*/
		MapSurface.segment = FP_SEG(ScratchBuf);
		MapScanlines[0] = FP_OFF(ScratchBuf);
		for(i = 1; i < 0xa0; i++)
			MapScanlines[i] = MapScanlines[i - 1] + 0x50;
	}

	OBJ_InitLists();
	AllocPaperDoll();

	InitGraphicsData();

	OSI_Started = 1;
	MAP_InitChunkCache();/*empty the map chunk cache*/
	HatchingDisabled = 1;
	GAME_Load();/*load the game*/

	/*no character in the saved game: go back to MARTIAN.EXE*/
	if(AvatarPortrait == 0) {
		CON_printf("You must first create a character. ");
		AnimateAtPrompt = 0;
		CON_getch();
		ShutdownSound();
		BuildConfigString(cfgString);
		MEM_shutdown(8);
		UnhookAll();
		GR_SHUTDOWN();
		while(1)
			execl("martian.exe", "martian.exe", 0L);
	}

	if(MouseOn)
		MOUSE_SHOW();
	MUS_PlayAreaSong();

	/*-- game loop --*/
	GameLoop();
	/*-- --*/

	/*exit: stop the music, delete the temporary files*/
	MUS_PlaySong(0);
	OBJ_DeleteTmpBlocks();
	ShutdownSound();
	MEM_shutdown(8);
	UnhookAll();
	GR_SHUTDOWN();
	exit(0);
}
