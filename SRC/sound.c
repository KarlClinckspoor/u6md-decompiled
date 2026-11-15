/* flags: -d -mm -p -c -Z -O -y */
/*
 * Martian Dreams - GAME.EXE segment 1FA9: the game's music and sound
 * effects. Savage Empire's GAME.EXE music.c (Ultima VI's), reworked.
 *
 * The music and sound library is SRC/music.c (segment 2710, MARTIAN.EXE's
 * source). This
 * module is the game's side: MUS_InitDriver() loads the 16 songs of
 * music.lzc into (expanded) memory (Songs, each with its volume in
 * SongVolume) and starts the sound driver with md.tim; MUS_PlaySong()
 * plays a song and remembers it (CurSong); MUS_AreaSong() chooses the
 * area's song: 3 on levels 2 and 3, 2 underground, 8 in combat, 0 by day
 * (6-17 h), else 1. Jingles start with PlaySong and queue the next song
 * with SetNextSong (no waiting for the music's marker as in SE).
 * MUS_PlaySfx() plays one of the 56 sound effects of Sfx; MUS_PlayLoopSfx
 * keeps one of them looping (LoopSfx, LoopVoice); a few functions play
 * their own sequences (MUS_SweepSfx, MUS_MachinerySfx, MUS_PlayMelody).
 * The combat music: MUS_CombatSong / MUS_AttackedSong at its start,
 * MUS_CombatOutcomeSong when a member falls or a foe dies, MUS_CombatEndSong.
 * MUS_MorningSong after a rest, MUS_EventJingle for story events.
 * PartyUnconscious is SE's "party knocked out" (C_2701_0556): in the dream the
 * dreamer wakes up (back to WakeX/WakeY, conversation 0x66) or, without
 * a place to go back to, shakes itself awake from the nightmare; out of it
 * the party wakes in the Olympus mine or the space capsule, unless the
 * capsule door is not open yet (TalkFlags[0x10] & 0x10), which ends the
 * game with a message on the text screen.
 * The game state block (SE's gamestat.c, MD's IsOnQuest-16D9) is a data-only
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

/*the graphic driver: GrDriver's offset selects the entry*/
#define GR_SHUTDOWN()             (GrDriver.iii.ofs = 0x03, (*GrDriver.f)())
#define GR_DRAW_BITMAP(s,a,b,p,c) (GrDriver.iii.ofs = 0x30, (*GrDriver.f)(s,a,b,p,c))

/*other game modules*/
extern int16 MusicAdapter;/*1: AdLib or Roland, 0: none*/
extern int16 CombatOutcome;
extern char StatusDirty;
extern int16 MapAnimEnabled;
extern unsigned char Time_H;
extern unsigned char Active;
extern int16 MapX, MapY, MapZ;
extern unsigned char WakeX, WakeY;/*where the dreamer wakes up (x, y)*/
extern int16 SoundFlag;
extern char DreamFoesLeft;
extern char InCombat;
extern int32 GameFlags;/*0x10: in a dream*/
extern int16 DreamStage;
extern char AnimateAtPrompt;
extern int16 EndPagingAtPrompt;
extern struct tClipInfo *CurClip;
extern unsigned char ObjStatus[];
extern unsigned char far *NPCMode;
extern unsigned char NPCStatus[];
extern unsigned char HitPoints[];
extern unsigned char TalkFlags[];/*per NPC*/
extern uint16 ObjShapeType[];
extern COMBAT_breakOff();
extern int16 MaxHP(int16);
extern ResetMapView();
extern RefreshStatus();
extern MAP_LoadAround();
extern ResurrectParty();/*the party wakes up after its death*/
extern int16 OBJ_FindTypeAt(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 OBJ_FindNPCAt(int16, int16, int16);/*next object from (start, ?, ?)*/
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);
extern MoveObj(int16, int16, int16, int16);/*MoveObj(obj, x, y, z)*/
extern OBJ_DeleteTmpBlocks();
extern int16 CON_mouseOffAt(int16, int16, int16, int16);
extern CON_mouseOn(int16);
extern CON_putch0(int16);
extern cdecl CON_printf(char far *, ...);
extern DreamWakeUp();
extern TALK_talkTo(int16, int16, int16);
extern char far UnconsciousMsg[];
extern char far AwakenAtMsg[];/*"... wake up in %s"*/
extern char far NoEmsMsg[];/*no EMS, no music*/

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
extern char far *LzcWorkBuf;/*work area of the .lzc loader*/
extern struct tSurface Screen;
extern union tHandler GrDriver;
extern void far *ScreenParts;/*mdscreen.lzc entry 2*/
/*EA76*/extern void far *LoopVoice;/*the looping sound effect's voice*/
/*EA90*/extern void far *Songs[16];/*the songs of music.lzc*/

#define IN_DREAM (GameFlags & 0x10)


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
/*189C*/int16 LoopSfx = -1;/*the looping sound effect (MUS_PlayLoopSfx), -1: none*/
/*189E*/unsigned char SongVolume[16] = {
	0xD7, 0xD7, 0xE1, 0xE1, 0xE6, 0xA5, 0xA5, 0xA5,
	0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0, 0, 0
};
/*the melody of MUS_PlayMelody: (note, duration) pairs, note 0 a rest*/
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
				CON_printf(NoEmsMsg);
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
		LZC_loadType("music.lzc", i, Songs[i], 0, LzcWorkBuf);
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
/*C_1FA9_02CE*/MUS_MorningSong(int16 force) {
	if(MusicAdapter && (force || (Time_H <= 11 && Time_H >= 5 && MapZ == 0))) {
		SetMusicVolume(SongVolume[4]);
		PlaySong(Songs[4]);
		SetNextSong(Songs[0], 0);
		CurSong = 0;
	}
}

/*jingle 7 then song 8 on the surface; song 14 (area 2) or 13 (area 3)
  underground*/
/*C_1FA9_032B*/MUS_AttackedSong() {
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
/*C_1FA9_03DA*/MUS_CombatEndSong(int16 how) {
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
/*C_1FA9_047E*/MUS_CombatOutcomeSong() {
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
/*C_1FA9_04ED*/MUS_EventJingle() {
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
/*C_1FA9_0555*/MUS_CombatSong() {
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
/*C_1FA9_0603*/PartyUnconscious() {
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
		DreamFoesLeft = 0;
		if(WakeX) {
			HitPoints[0] = MaxHP(1);
			NPCStatus[0] &= ~8;
			MoveObj(0, WakeX, WakeY, MapZ);
			MapX = WakeX;
			MapY = WakeY;
			MAP_LoadAround();
			COMBAT_breakOff();
			CON_printf("You feel your dream consciousness returning.\n");
			if(DreamStage == 0xc0) {
				if((TalkFlags[0x66] & 0x3c) == 0x18) {
					for(obj = OBJ_FindNPCAt(0xc0, 0x87, 0x17); obj >= 0; obj = OBJ_FindNPCAt(obj + 1, 0x87, 0x17)) {
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
			GR_DRAW_BITMAP(&Screen, 0x93, 0xa9, ScreenParts, 1);
			CON_mouseOn(mouseState);
		}
		InCombat = 0;
		CurClip->attrFlags |= 0x10;
		CON_printf("You shake yourself awake from the nightmare.\n");
		DreamWakeUp();
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
	CON_printf(AwakenAtMsg, (TalkFlags[0x46] & 8)?"the Olympus mine":"the space capsule");
	ResurrectParty();
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
			MUS_PlayLoopSfx(-2);
	}
}

/*keeps sound effect <sfx> looping (flag 8): -1 stops it, -2 starts the
  current one again*/
/*C_1FA9_09A5*/MUS_PlayLoopSfx(register int16 sfx) {
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
/*C_1FA9_0A8A*/MUS_SweepSfx() {
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
/*C_1FA9_0AFD*/MUS_MachinerySfx() {
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
/*C_1FA9_0BE4*/MUS_SplashSfx() {
	if(SoundFlag) {
		Sfx[35].note = 0x30;
		PlaySound(&(Sfx[35]), 0, 0xff, 0x3f, 1, 0xc);
	}
}

/*plays Melody with sound effect 38's instrument (the music is faded out
  first, the area song comes back after)*/
/*C_1FA9_0C15*/MUS_PlayMelody() {
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
/*C_1FA9_0CD2*/MUS_StopSfx() {
	if(soundCard)
		StopAllSounds();
}
