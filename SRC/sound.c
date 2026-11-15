/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE segment 1FA9: the game's music and sound
 * effects. Savage Empire's GAME.EXE music.c (Ultima VI's), reworked.
 *
 * The music and sound library is SRC/music.c (2710, MARTIAN.EXE's). This
 * module is the game's side: MUS_InitDriver() loads the 16 songs of
 * music.lzc into (expanded) memory (Songs, each with its volume in
 * SongVolume) and starts the sound driver with md.tim; MUS_PlaySong()
 * plays a song and remembers it (CurSong); MUS_AreaSong() chooses the
 * area's song: 3 on levels 2 and 3, 2 underground, 8 in combat, 0 by day
 * (6-17 h), else 1. Jingles start with PlaySong and queue the next song
 * with SetNextSong (no waiting for the music's marker as in SE).
 * MUS_PlaySfx() plays one of the 56 sound effects of Sfx; C_1FA9_09A5
 * keeps one of them looping (LoopSfx, LoopVoice); a few functions play
 * their own sequences (a sweep, five beeps, a melody).
 * C_1FA9_0603 is SE's "party knocked out" (C_2701_0556): in the dream the
 * dreamer wakes up (back to D_1684/D_1685, conversation 0x66) or, without
 * a place to go back to, shakes itself awake from the nightmare; out of it
 * the party wakes in the Olympus mine or the space capsule, unless the
 * capsule door is not open yet (TalkFlags[0x10] & 0x10), which ends the
 * game with a message on the text screen.
 * The game state block (SE's gamestat.c, MD's D_1672-16D9) is a data-only
 * module just before this one (its empty code segment is at 1FA9:2).
 */
#include "se.h"

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;/*1 wrap, 2 center, 4 capitalize, 8 runes, 0x10 paginate, 0x20 scroll*/
};

/*a step of a sound effect (8 bytes, the library's)*/
struct tSfxStep {
	unsigned char flags;/*_00*//*1: go on with the next step, 2: glide to target, 4: hold, 8: repeat*/
	unsigned char program;/*_01*//*1+: MIDI program + 1; 0: the end*/
	unsigned char note;/*_02*/
	unsigned char velocity;/*_03*/
	int16 duration;/*_04*//*in timer ticks*/
	unsigned char target;/*_06*//*the note a glide goes to*/
	unsigned char pad;/*_07*/
};

/*the graphic driver: D_BA6A's offset selects the entry*/
#define GR_SHUTDOWN()             (D_BA6A.iii.ofs = 0x03, (*D_BA6A.f)())
#define GR_DRAW_BITMAP(s,a,b,p,c) (D_BA6A.iii.ofs = 0x30, (*D_BA6A.f)(s,a,b,p,c))

/*other game modules*/
extern int16 MusicAdapter;/*1: AdLib or Roland, 0: none*/
extern int16 D_06BE;/*CombatOutcome*/
extern char D_0F12;/*StatusDirty*/
extern int16 D_0F18;/*MapAnimEnabled*/
extern unsigned char D_1675;/*Time_H*/
extern unsigned char D_167A;/*Active*/
extern int16 D_167C, D_167E, D_1680;/*MapX, MapY, MapZ*/
extern unsigned char D_1684, D_1685;/*where the dreamer wakes up (x, y)*/
extern int16 D_1687;/*SoundFlag*/
extern char D_16A3;
extern char D_16A4;/*InCombat*/
extern int32 D_16AA;/*flags (0x10: the dream)*/
extern int16 D_16D4;
extern char D_1B8D;/*AnimateAtPrompt*/
extern int16 D_1B90;/*EndPagingAtPrompt*/
extern struct tClipInfo *D_1BDB;/*CurClip*/
extern unsigned char D_3266[];/*ObjStatus*/
extern unsigned char far *D_46D0;/*NPCMode*/
extern unsigned char D_6CBA[];/*NPCStatus*/
extern unsigned char D_6DBE[];/*HitPoints*/
extern unsigned char D_6EC6[];/*TalkFlags (per NPC)*/
extern uint16 D_9CC4[];/*ObjShapeType*/
extern C_07D1_02C4();
extern int16 C_07D1_064E(int16);/*MaxHP*/
extern C_0E1A_11D2();/*ResetMapView*/
extern C_120E_18B4();/*RefreshStatus*/
extern MAP_LoadAround();
extern C_1534_0C51();/*the party wakes up after its death*/
extern int16 C_1B09_09C7(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 C_1B09_1081(int16, int16, int16);/*next object from (start, ?, ?)*/
extern int16 C_1B09_116E(int16, int16, int16, int16, int16, int16);/*AddMapObj*/
extern C_1B09_1F52(int16, int16, int16, int16);/*MoveObj(obj, x, y, z)*/
extern C_1B09_2CD2();/*OBJ_DeleteTmpBlocks*/
extern int16 CON_mouseOffAt(int16, int16, int16, int16);
extern CON_mouseOn(int16);
extern CON_putch0(int16);
extern cdecl CON_printf(char far *, ...);
extern C_2F5F_0477();
extern C_2F83_0167(int16, int16, int16);/*TALK_talkTo*/
extern char far C_2BB4_006E[];/*UnconsciousMsg*/
extern char far C_2BB4_00A4[];/*"... wake up in %s"*/
extern char far C_2BB4_215E[];/*no EMS, no music*/

/*library (cdecl)*/
extern int16 cdecl soundCard;
extern int16 cdecl emsInUse;
extern int16 cdecl osiError;
extern int cdecl musicStopped;
extern void far * cdecl PlaySound(struct tSfxStep far *steps, int16 flags, int16 volume, int16 pan, int16 id, int16 prio);
extern void cdecl SfxStop(void far *v, int16 id);
extern void cdecl StopAllSounds(void);
extern void cdecl FadeOutMusic(int16 time);
extern void cdecl InitSound(char far *name);
extern void cdecl ShutdownSound(void);
extern int16 cdecl PlaySong(void far *song);
extern void cdecl FadeToSong(void far *song, int16 time, int16 flags);
extern void cdecl SetNextSong(void far *song, int16 flags);
extern void cdecl SetMusicVolume(int16 vol);
extern void cdecl TimerDelay(int32 t);/*wait (2383 per millisecond)*/
extern void far * cdecl MEM_alloc(int32 size, int16 flags);
extern cdecl MEM_shutdown(int16);
extern cdecl UnhookAll(void);
extern int32 LIB_entrySize(char far *name, int16 index);
extern void far *LZC_loadType(char far *name, int16 index, void far *out, int16 memflags, char far *work);

/*uninitialised data*/
extern char far *D_3E80;/*work area of the .lzc loader*/
extern struct tSurface D_3232;/*Screen*/
extern union tHandler D_BA6A;/*GrDriver*/
extern void far *D_BA7E;/*mdscreen.lzc entry 2*/
extern void far *D_EA76;/*LoopVoice: the looping sound effect's voice*/
extern void far *D_EA90[16];/*Songs: the songs of music.lzc*/

#define CombatOutcome D_06BE
#define StatusDirty D_0F12
#define MapAnimEnabled D_0F18
#define Time_H D_1675
#define Active D_167A
#define MapX D_167C
#define MapY D_167E
#define MapZ D_1680
#define SoundFlag D_1687
#define InCombat D_16A4
#define IN_DREAM (D_16AA & 0x10)
#define AnimateAtPrompt D_1B8D
#define EndPagingAtPrompt D_1B90
#define CurClip D_1BDB
#define ObjStatus D_3266
#define NPCMode D_46D0
#define NPCStatus D_6CBA
#define HitPoints D_6DBE
#define TalkFlags D_6EC6
#define ObjShapeType D_9CC4
#define LoopVoice D_EA76
#define Songs D_EA90

#define MaxHP C_07D1_064E
#define ResetMapView C_0E1A_11D2
#define RefreshStatus C_120E_18B4
#define OBJ_FindTypeAt C_1B09_09C7
#define AddMapObj C_1B09_116E
#define MoveObj C_1B09_1F52
#define OBJ_DeleteTmpBlocks C_1B09_2CD2
#define TALK_talkTo C_2F83_0167
#define UnconsciousMsg C_2BB4_006E

#define GetType(i) (ObjShapeType[i] & 0x3ff)

/*16DA*/int16 CurSong = -1;/*the song being played (-1: none)*/

/*the sound effects (MUS_PlaySfx): one step each*/
/*16DC*/struct tSfxStep Sfx[56] = {
	{0, 0x04, 0x30, 0x7F, 0x3C, 0, 0},
	{0, 0x06, 0x30, 0x7F, 0x3C, 0, 0},
	{0, 0x07, 0x30, 0x7F, 0x3C, 0, 0},
	{0, 0x08, 0x30, 0x7F, 0x1E, 0, 0},
	{0, 0x02, 0x30, 0x7F, 0x1E, 0, 0},
	{0, 0x36, 0x30, 0x7F, 0x14, 0, 0},
	{0, 0x0B, 0x30, 0x60, 0x1E, 0, 0},
	{0, 0x0C, 0x30, 0x7F, 0x3C, 0, 0},
	{0, 0x10, 0x18, 0x7F, 0x3C, 0, 0},
	{0, 0x1D, 0x30, 0x60, 0x3C, 0, 0},
	{0, 0x61, 0x30, 0x7F, 0x14, 0, 0},
	{0, 0x13, 0x30, 0x7F, 0x64, 0, 0},
	{0, 0x14, 0x30, 0x7F, 0x3C, 0, 0},
	{0, 0x15, 0x30, 0x7F, 0x3C, 0, 0},
	{2, 0x16, 0x30, 0x7F, 0x19, 0, 0},
	{0, 0x17, 0x40, 0x7F, 0x3C, 0, 0},
	{0, 0x18, 0x30, 0x60, 0x5A, 0, 0},
	{0, 0x1B, 0x30, 0x60, 0x3C, 0, 0},
	{0, 0x38, 0x30, 0x7F, 0x5A, 0, 0},
	{0, 0x70, 0x30, 0x54, 0x0A, 0, 0},
	{0, 0x29, 0x30, 0x40, 0x0A, 0, 0},
	{0, 0x2A, 0x30, 0x60, 0x3C, 0, 0},
	{0, 0x63, 0x18, 0x7F, 0x3C, 0, 0},
	{0, 0x37, 0x30, 0x7F, 0x0A, 0, 0},
	{0, 0x2D, 0x30, 0x7F, 0x0A, 0, 0},
	{0, 0x2E, 0x30, 0x7F, 0x0A, 0, 0},
	{0, 0x2F, 0x30, 0x7F, 0x1E, 0, 0},
	{0, 0x30, 0x30, 0x7F, 0x3C, 0, 0},
	{0, 0x3C, 0x30, 0x7F, 0x3C, 0, 0},
	{0, 0x3D, 0x30, 0x7F, 0x3C, 0, 0},
	{0, 0x3E, 0x30, 0x7F, 0x3C, 0, 0},
	{0, 0x3F, 0x30, 0x7F, 0x3C, 0, 0},
	{0, 0x40, 0x30, 0x7F, 0x3C, 0, 0},
	{0, 0x64, 0x30, 0x7F, 0x1E, 0, 0},
	{0, 0x65, 0x30, 0x7F, 0x17, 0, 0},
	{0, 0x75, 0x30, 0x7F, 0x1E, 0, 0},
	{0, 0x35, 0x30, 0x7F, 0x0A, 0, 0},
	{0, 0x6A, 0x30, 0x7F, 0x3C, 0, 0},
	{0, 0x6B, 0x41, 0x40, 0x28, 0, 0},
	{0, 0x6B, 0x32, 0x40, 0x28, 0, 0},
	{0, 0x6B, 0x34, 0x40, 0x28, 0, 0},
	{0, 0x6B, 0x35, 0x40, 0x28, 0, 0},
	{0, 0x6B, 0x37, 0x40, 0x28, 0, 0},
	{0, 0x6B, 0x39, 0x40, 0x28, 0, 0},
	{0, 0x6B, 0x3A, 0x40, 0x28, 0, 0},
	{0, 0x6B, 0x3C, 0x40, 0x28, 0, 0},
	{0, 0x6B, 0x3E, 0x40, 0x28, 0, 0},
	{0, 0x6B, 0x40, 0x40, 0x28, 0, 0},
	{0, 0x74, 0x18, 0x7F, 0x64, 0, 0},
	{0, 0x28, 0x30, 0x7F, 0x0F, 0, 0},
	{0, 0x6E, 0x30, 0x60, 0x5A, 0, 0},
	{0, 0x6F, 0x30, 0x60, 0x4B, 0, 0},
	{0, 0x52, 0x30, 0x60, 0x3C, 0, 0},
	{0, 0x51, 0x30, 0x60, 0x3C, 0, 0},
	{0, 0x50, 0x30, 0x60, 0x1E, 0, 0},
	{0, 0x58, 0x30, 0x60, 0x1E, 0, 0}
};
/*189C*/int16 LoopSfx = -1;/*the looping sound effect (C_1FA9_09A5), -1: none*/
/*189E*/unsigned char SongVolume[16] = {
	0xD7, 0xD7, 0xE1, 0xE1, 0xE6, 0xA5, 0xA5, 0xA5,
	0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0, 0, 0
};
/*the melody of C_1FA9_0C15: (note, duration) pairs, note 0 a rest*/
/*18AE*/unsigned char Melody[46] = {
	0x1C, 0x0C, 0x1A, 0x0C, 0x1C, 0x18, 0x00, 0x0C,
	0x1C, 0x0C, 0x1A, 0x0C, 0x18, 0x0C, 0x17, 0x0C,
	0x15, 0x0C, 0x14, 0x18, 0x15, 0x18, 0x00, 0x18,
	0x34, 0x0C, 0x32, 0x0C, 0x34, 0x18, 0x00, 0x0C,
	0x34, 0x0C, 0x32, 0x0C, 0x30, 0x0C, 0x2F, 0x0C,
	0x2D, 0x0C, 0x2C, 0x18, 0x2D, 0x18
};

extern int16 MUS_LoadSongs();

/*init the sound: with a music card, loads the songs (needs expanded
  memory), then starts the sound driver with the timbres of md.tim*/
/*C_1FA9_0002*/MUS_InitDriver() {
	if(!SoundFlag) {
		soundCard = 0;
	} else {
		if(soundCard == 2 || soundCard == 1) {
			if(emsInUse == 0) {
				MusicAdapter = 0;
				CON_printf(C_2BB4_215E);
			} else if(MUS_LoadSongs() == 0) {
				MusicAdapter = 0;
				SoundFlag = 0;
				soundCard = 0;
				CON_printf("No music loaded.\n");
			}
		}
		if(soundCard == 1) {
			CON_printf("Initializing Roland...");
			CON_putch0(-1);
		}
		InitSound("md.tim");
		if(soundCard == 1)
			CON_printf("done!\n");
	}
}

/*loads the 16 songs of music.lzc into Songs (expanded memory); a song
  that fails is left 0 (with a message). Returns 0 if one failed*/
/*C_1FA9_009D*/int16 MUS_LoadSongs() {
	uint32 size;
	register int16 failed = 0;
	register int16 i;

	for(i = 0; i < 16; i++) {
		if(failed)
			Songs[i] = 0;
		size = LIB_entrySize("music.lzc", i);
		if(osiError || size == -1L) {
			CON_printf("Music error: getsize, song %d, fe %d, size %dK\n", i, osiError, size >> 10);
			failed = 1;
			Songs[i] = 0;
			continue;
		}
		if((Songs[i] = MEM_alloc(size, 4)) == 0) {
			CON_printf("Music error: Insufficient EMS, song %d, size %dK\n", i, size >> 10);
			failed = 1;
			Songs[i] = 0;
			continue;
		}
		LZC_loadType("music.lzc", i, Songs[i], 0, D_3E80);
		if(osiError) {
			CON_printf("Music error: readpacket, song %d, fe %d\n", i, osiError);
			failed = 1;
			Songs[i] = 0;
		}
	}
	return !failed;
}

/*plays song <song> at its volume (fading from the current one), or fades
  the music out (-1); the same song is started again only if the music
  stopped*/
/*C_1FA9_01B4*/MUS_PlaySong(register int16 song) {
	if(MusicAdapter) {
		if(song == -1) {
			FadeOutMusic(60);
			CurSong = -1;
		} else if(CurSong == -1) {
			SetMusicVolume(SongVolume[song]);
			PlaySong(Songs[song]);
			CurSong = song;
		} else if(CurSong != song) {
			SetMusicVolume(SongVolume[song]);
			FadeToSong(Songs[song], 60, 0);
			CurSong = song;
		} else {
			if(musicStopped) {
				SetMusicVolume(SongVolume[song]);
				PlaySong(Songs[song]);
				CurSong = song;
			}
			return;/*(keeps this branch from being merged with the second)*/
		}
	}
}

/*the song of the area: 3 on levels 2 and 3, 2 underground, 8 in combat,
  0 by day (6 to 17 h), else 1*/
/*C_1FA9_0270*/int16 MUS_AreaSong() {
	if(MapZ == 2 || MapZ == 3)
		return 3;
	if(MapZ > 0)
		return 2;
	if(InCombat)
		return 8;
	if(Time_H > 5 && Time_H < 18)
		return 0;
	return 1;
}

/*plays the song of the area*/
/*C_1FA9_02B5*/MUS_PlayAreaSong() {
	register int16 song;

	if(MusicAdapter) {
		song = MUS_AreaSong();
		MUS_PlaySong(song);
	}
}

/*the morning song (4), then song 0: always with <force>, else on the
  surface between 5 and 11 o'clock*/
/*C_1FA9_02CE*/C_1FA9_02CE(int16 force) {
	if(MusicAdapter && (force || (Time_H <= 11 && Time_H >= 5 && MapZ == 0))) {
		SetMusicVolume(SongVolume[4]);
		PlaySong(Songs[4]);
		SetNextSong(Songs[0], 0);
		CurSong = 0;
	}
}

/*jingle 7 then song 8 on the surface; song 14 (area 2) or 13 (area 3)
  underground*/
/*C_1FA9_032B*/C_1FA9_032B() {
	int16 area;
	register int16 jingle, song;

	area = MUS_AreaSong();
	if(!MusicAdapter)
		return;
	if(area == 2) {
		jingle = -1;
		song = 14;
	} else if(area == 3) {
		jingle = -1;
		song = 13;
	} else {
		jingle = 7;
		song = 8;
	}
	if(jingle >= 0) {
		SetMusicVolume(SongVolume[jingle]);
		PlaySong(Songs[jingle]);
		SetNextSong(Songs[song], 0);
	} else {
		SetMusicVolume(SongVolume[song]);
		FadeToSong(Songs[song], 60, 0);
	}
	CurSong = song;
}

/*end of a battle: underground the area song; on the surface <how> < 0
  plays jingle 11, > 0 jingle 12, then the area song; 0 fades to it*/
/*C_1FA9_03DA*/C_1FA9_03DA(int16 how) {
	register int16 jingle, song;

	if(MusicAdapter) {
		if(MapZ != 0) {
			MUS_PlayAreaSong();
		} else {
			song = MUS_AreaSong();
			if(how < 0)
				jingle = 11;
			else if(how > 0)
				jingle = 12;
			else
				jingle = -1;
			SetMusicVolume(SongVolume[song]);
			if(jingle >= 0) {
				PlaySong(Songs[jingle]);
				SetNextSong(Songs[song], 0);
			} else {
				FadeToSong(Songs[song], 60, 0);
			}
			CurSong = song;
		}
	}
}

/*doubles CombatOutcome; on the surface plays song 9 or 10 by its sign,
  then song 8*/
/*C_1FA9_047E*/C_1FA9_047E() {
	CombatOutcome *= 2;
	if(MapZ != 0 || !MusicAdapter)
		return;
	if(CombatOutcome < 0) {
		SetMusicVolume(SongVolume[9]);
		PlaySong(Songs[9]);
	} else {
		SetMusicVolume(SongVolume[10]);
		PlaySong(Songs[10]);
	}
	SetNextSong(Songs[8], 0);
	CurSong = 8;
}

/*out of combat: song 5 (surface) or 15, then the area song*/
/*C_1FA9_04ED*/C_1FA9_04ED() {
	register int16 song;

	if(MusicAdapter && InCombat != 1) {
		if(MapZ == 0)
			song = 5;
		else
			song = 15;
		SetMusicVolume(SongVolume[song]);
		PlaySong(Songs[song]);
		CurSong = MUS_AreaSong();
		SetNextSong(Songs[CurSong], 0);
	}
}

/*jingle 6 then song 8 on the surface; song 14 (area 2) or 13 (area 3)
  underground*/
/*C_1FA9_0555*/C_1FA9_0555() {
	int16 area;
	register int16 jingle, song;

	if(MusicAdapter) {
		area = MUS_AreaSong();
		if(area == 2) {
			jingle = -1;
			song = 14;
		} else if(area == 3) {
			jingle = -1;
			song = 13;
		} else {
			jingle = 6;
			song = 8;
		}
		if(jingle >= 0) {
			SetMusicVolume(SongVolume[jingle]);
			PlaySong(Songs[jingle]);
			SetNextSong(Songs[song], 0);
		} else {
			SetMusicVolume(SongVolume[song]);
			FadeToSong(Songs[song], 60, 0);
		}
		CurSong = song;
	}
}

/*the party is knocked out (see the header)*/
/*C_1FA9_0603*/C_1FA9_0603() {
	int16 song;
	char *msg;
	int16 mouseState;
	union REGS regs, out;
	register int16 obj;
	register char *p;

	msg = "Your character cannot be resurrected before the capsule door is open.\n\rPlease try again!\r\n";
	if(Active == 0x10)
		return;
	if(IN_DREAM) {
		NPCStatus[0] &= ~0x10;
		D_16A3 = 0;
		if(D_1684) {
			HitPoints[0] = MaxHP(1);
			NPCStatus[0] &= ~8;
			MoveObj(0, D_1684, D_1685, MapZ);
			MapX = D_1684;
			MapY = D_1685;
			MAP_LoadAround();
			C_07D1_02C4();
			CON_printf("You feel your dream consciousness returning.\n");
			if(D_16D4 == 0xc0) {
				if((TalkFlags[0x66] & 0x3c) == 0x18) {
					for(obj = C_1B09_1081(0xc0, 0x87, 0x17); obj >= 0; obj = C_1B09_1081(obj + 1, 0x87, 0x17)) {
						if(GetType(obj) == 0x175 && NPCMode[obj] == 0x12) {
							NPCStatus[obj] |= 0x10;
							ObjShapeType[obj] = 0;
						}
					}
					if((obj = OBJ_FindTypeAt(0xe0, 0x7f, 0x18)) < 0) {
						if((obj = AddMapObj(0xce0, 0, 0, 0x7f, 0x18, MapZ)) < 0)
							CON_printf("Aargh!\n");
						else
							ObjStatus[obj] &= ~0x20;
					}
				}
				TalkFlags[0x66] |= 0x80;
				TALK_talkTo(Active, 0x66, 0);
			}
			MapAnimEnabled = 1;
			StatusDirty = 1;
			return RefreshStatus();/*(a plain return here is merged with the one below)*/
		}
		if(InCombat) {
			mouseState = CON_mouseOffAt(0x8d, 0xa3, 0x9e, 0xc4);
			GR_DRAW_BITMAP(&D_3232, 0x93, 0xa9, D_BA7E, 1);
			CON_mouseOn(mouseState);
		}
		InCombat = 0;
		CurClip->attrFlags |= 0x10;
		CON_printf("You shake yourself awake from the nightmare.\n");
		C_2F5F_0477();
		MapAnimEnabled = 1;
		StatusDirty = 1;
		RefreshStatus();
		return;
	}
	if(MusicAdapter)
		FadeOutMusic(90);
	CurClip->attrFlags |= 0x10;
	CON_printf(UnconsciousMsg);
	MapAnimEnabled = 1;
	StatusDirty = 1;
	RefreshStatus();
	ResetMapView();
	AnimateAtPrompt = 0;
	if(MusicAdapter) {
		SetMusicVolume(SongVolume[11]);
		PlaySong(Songs[11]);
		song = MUS_AreaSong();
		SetNextSong(Songs[song], 0);
		CurSong = song;
	}
	if(!(TalkFlags[0x10] & 0x10)) {
		/*the capsule door is not open: the end*/
		ShutdownSound();
		OBJ_DeleteTmpBlocks();
		MEM_shutdown(8);
		UnhookAll();
		GR_SHUTDOWN();
		regs.x.ax = 0xe00;
		regs.x.bx = 1;
		p = msg;
		while(*p) {
			regs.h.al = *p++;
			int86(0x10, &regs, &out);
		}
		exit(4);
	}
	CON_printf(C_2BB4_00A4, (TalkFlags[0x46] & 8)?"the Olympus mine":"the space capsule");
	C_1534_0C51();
	EndPagingAtPrompt = 1;
	AnimateAtPrompt = 1;
}

/*plays sound effect <sfx> (0-55) when the sound is on (13 twice); a
  looping effect is started again*/
/*C_1FA9_0919*/MUS_PlaySfx(register int16 sfx) {
	if(SoundFlag && sfx < 56) {
		PlaySound(&(Sfx[sfx]), 0, 0xff, 0x3f, 1, 0xc);
		if(sfx == 13) {
			TimerDelay(0x745b8L);
			PlaySound(&(Sfx[sfx]), 0, 0xff, 0x3f, 1, 0xc);
		}
		if(LoopSfx != -1)
			C_1FA9_09A5(-2);
	}
}

/*keeps sound effect <sfx> looping (flag 8): -1 stops it, -2 starts the
  current one again*/
/*C_1FA9_09A5*/C_1FA9_09A5(register int16 sfx) {
	struct tSfxStep step;

	if(SoundFlag && sfx < 56) {
		if(sfx != LoopSfx) {
			if(sfx == -2)
				sfx = LoopSfx;
			else if(LoopSfx != -1)
				SfxStop(LoopVoice, 2);
			if(sfx != -1) {
				step.flags = Sfx[sfx].flags | 8;
				step.program = Sfx[sfx].program;
				step.note = Sfx[sfx].note;
				step.velocity = Sfx[sfx].velocity;
				step.duration = Sfx[sfx].duration;
				step.target = Sfx[sfx].target;
				step.pad = 0;
				LoopVoice = PlaySound(&step, 0, 0xff, 0x3f, 2, 0xc);
			}
		}
		LoopSfx = sfx;
	}
}

/*a rising sweep of sound effect 14 (notes 0x2f-0x60, panned along), then
  sound effect 11*/
/*C_1FA9_0A8A*/C_1FA9_0A8A() {
	int16 note;

	if(SoundFlag) {
		for(note = 0x2f; note <= 0x60; note++) {
			Sfx[14].note = note;
			PlaySound(&(Sfx[14]), 0, 0xff, note, 1, 0xc);
			TimerDelay(0xe8b7L);
		}
		PlaySound(&(Sfx[11]), 0, 0xff, 0x60, 1, 0x14);
	}
}

/*five times sound effect 19 (machinery)*/
/*C_1FA9_0AFD*/C_1FA9_0AFD() {
	if(SoundFlag) {
		PlaySound(&(Sfx[19]), 0, 0xff, 0x3f, 1, 0xc);
		TimerDelay(0xba2cL);
		PlaySound(&(Sfx[19]), 0, 0xff, 0x3f, 1, 0xc);
		TimerDelay(0xba2cL);
		PlaySound(&(Sfx[19]), 0, 0xff, 0x3f, 1, 0xc);
		TimerDelay(0xba2cL);
		PlaySound(&(Sfx[19]), 0, 0xff, 0x3f, 1, 0xc);
		TimerDelay(0xba2cL);
		PlaySound(&(Sfx[19]), 0, 0xff, 0x3f, 1, 0xc);
	}
}

/*sound effect 35 at note 0x30*/
/*C_1FA9_0BE4*/C_1FA9_0BE4() {
	if(SoundFlag) {
		Sfx[35].note = 0x30;
		PlaySound(&(Sfx[35]), 0, 0xff, 0x3f, 1, 0xc);
	}
}

/*plays Melody with sound effect 38's instrument (the music is faded out
  first, the area song comes back after)*/
/*C_1FA9_0C15*/C_1FA9_0C15() {
	struct tSfxStep step;
	register int16 i;

	if(SoundFlag) {
		if(MusicAdapter) {
			MUS_PlaySong(-1);
			TimerDelay(0x245c98L);
		}
		step.flags = 0;
		step.program = Sfx[38].program;
		step.velocity = Sfx[38].velocity;
		step.target = 0;
		step.pad = 0;
		for(i = 0; i < 46; i += 2) {
			if(Melody[i]) {
				step.note = Melody[i];
				step.duration = Melody[i + 1] - 1;
				PlaySound(&step, 0, 0xff, 0x3f, 1, 0xc);
			}
			TimerDelay(Melody[i + 1] * 39717L);
		}
		StopAllSounds();
		if(MusicAdapter)
			MUS_PlayAreaSong();
	}
}

/*stops the sound effects*/
/*C_1FA9_0CD2*/C_1FA9_0CD2() {
	if(soundCard)
		StopAllSounds();
}
