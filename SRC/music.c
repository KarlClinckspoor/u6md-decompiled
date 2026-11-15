/* flags: -mm -c -O -G -Z -d */
/* compiler: Turbo C++ 1.01 */
/*
 * Martian Dreams, GAME.EXE segment 2710 (the same source as MARTIAN.EXE's music.c, segment
 * 1046; SRC_M/music.c): music and sound effects through the STRAX.DRV
 * sound driver. Plays standard MIDI files (MThd/MTrk, format 0-2, up to 32 tracks) from a
 * timer interrupt, with Origin's 0xFE loop events, and sound effects ("voices": sequences
 * of 8-byte steps) on MIDI channels 2-9, borrowed from the music and given back.
 * Carried over from The Savage Empire's SAVAGE.EXE music.c (segment 08E4); this version
 * was compiled by Turbo C++ 1.01 and does the same; the timer's previous routine
 * (prevTimer) is initialised data here. The source differs from SE's only where 1.01 or a
 * slightly different expression gives other code (VOICE, SfxStartStep, InitSound ...).
 */
#include <dos.h>

/*a step of a sound effect (8 bytes)*/
struct tSfxStep {
	unsigned char flags;/*_00*//*1: go on with the next step, 2: glide to target, 4: hold, 8: repeat*/
	unsigned char program;/*_01*//*1+: MIDI program + 1; 0: the end*/
	unsigned char note;/*_02*/
	unsigned char velocity;/*_03*/
	int duration;/*_04*//*in timer ticks*/
	unsigned char target;/*_06*//*the note a glide goes to*/
	unsigned char pad;/*_07*/
};

/*a sound effect being played (14 bytes)*/
struct tVoice {
	unsigned char flags;/*_00*//*1: stereo (two channels), 2: keep the channel at the end*/
	unsigned char chan1;/*_01*//*MIDI channel (left); 0: the voice is free*/
	unsigned char chan2;/*_02*//*right channel (stereo)*/
	unsigned char note;/*_03*/
	unsigned char volume;/*_04*/
	unsigned char pan;/*_05*/
	int id;/*_06*/
	int counter;/*_08*//*ticks left of the step*/
	struct tSfxStep far *step;/*_0A*/
};

/*what the driver's init entry returns*/
struct tDrvInfo {
	unsigned char unk00;/*_00*/
	unsigned char ok;/*_01*//*0: the timbre bank is not kept*/
	char unk02[8];/*_02*//*given to the interrupt hook with the driver's timer routine*/
	void far *cb1;/*_0A*//*callbacks given to the driver*/
	void far *cb2;/*_0E*/
};

typedef void (far pascal *tDrv3)(int, int, int);
typedef long (far pascal *tDrv3L)(int, int, int);/*see SetSoundParam*/
typedef void (far pascal *tDrv2)(int, int);
typedef void (far pascal *tDrv1)(void far *);
typedef void (far pascal *tDrv0)(void);
typedef struct tDrvInfo far *(far pascal *tDrvInit)(void);

/*other modules*/
extern void far * pascal LZC_load(char far *, int, long, int, long);/*(name, index, 0L, flags, 0L): load entry index of a file into memory*/
extern MEM_free(void far *);/*free memory loaded by LZC_load*/
extern void far *EMS_map(void far *);/*maps (expanded memory) data, returns its address*/
extern EMS_saveMap(void);/*saves the expanded memory mapping (0: failed)*/
extern EMS_restoreMap(void);/*restores it*/
extern HookVector(int, void far *, void *, void far *);/*(vector, routine, save, chain): hook an interrupt*/
extern SetTimerRate(int);/*sets the timer rate (Hz)*/
extern UnhookAll(void);/*unhooks the interrupts*/
extern LoopDelay();/*a callback given to the driver*/
extern int emsInUse;/*expanded memory in use*/

/*uninitialised data*/
/*EA52*/extern tDrv2 drvProgram;/*driver: program change (channel, program)*/
/*EA56*/extern void far *drvEntry14;/*driver entry +14h*/
/*EA5A*/extern tDrvInit drvInit;/*driver: init*/
/*EA66*/extern tDrv3 drvNote;/*driver: note (channel, note, velocity; 0: off)*/
/*EA72*/extern tDrv1 drvLoadTimbres;/*driver: load timbre bank*/
/*EA7C*/extern unsigned far *drvCode;/*STRAX.DRV in memory: entry offsets at +2..+14h*/
/*EA80*/extern tDrv0 drvShutdown;/*driver: shutdown*/
/*EA84*/extern void far *drvTimer;/*driver: timer routine*/
/*EA88*/extern void far *drvEntry12;/*driver entry +12h*/
/*EA8C*/extern struct tDrvInfo far *drvInfo;/*what the driver's init returned*/
/*EAD0*/extern tDrv3 drvController;/*driver: controller (channel, controller, value)*/
/*EAD4*/extern tDrv3 drvPitchBend;/*driver: pitch bend (channel, lsb, msb)*/
/*F204*/extern int trackEnded[32];/*the track has ended*/
/*F0E4*/extern unsigned char far *trackPtr[32];/*where each track is*/
/*F0DA*/extern char drvTimerHook[8];/*hook of the driver's timer routine*/
/*F0CC*/extern void far *timbreBank;/*the timbre bank in memory*/
/*F164*/extern unsigned char trackStatus[32];/*running status of each track*/
/*F248*/extern int ticksPerQuarter;/*MThd: ticks per quarter note*/
/*F0D0*/extern char musTimerHook[8];/*hook of the timer routine*/
/*F246*/extern int numTracks;/*MThd: number of tracks*/
/*F184*/extern long trackDelay[32];/*ticks until each track's next event*/
/*F244*/extern int midiFormat;/*MThd: format*/

/*2710's initialised data*/
/*2B4E*/int soundCard = 0;/*0: none, 1: Roland, 2: AdLib, 3: the third driver (timer at 7680 Hz); STRAX.DRV entry soundCard - 1*/
/*2B50*/int sfxReady = 0;/*the sound effects may be updated (0 while they are changed)*/
/*2B52*/int musicMarker = 0;/*the last 0xFE type 3 event's value*/
/*2B54*/int musicCondition = 0;/*value the 0xFE type 4 and 5 events test*/
/*2B56*/int noSongQueued = 1;/*1 when a song starts, 0 when the next song is set (not read here)*/
/*2B58*/int musicNotesOn = 1;/*music notes are played*/
/*2B5A*/int musicVolume = 0x100;/*music volume (0x100: full)*/
/*2B5C*/int musicVolumeSet = 0x100;/*music volume set (fades go back to it)*/
/*2B5E*/int fadeStep = 0;/*fade step*/
/*2B60*/int fadeLevel = 0;/*fade level*/
/*2B62*/int fadeNoStop = 0;/*the fade does not stop the music*/
/*2B64*/int nextSongSet = 0;/*the next song is set: 0xFE loops of type 0 are skipped, type 1 ends the song*/
/*2B66*/void far *nextSong = 0;/*the song to play when the music stops*/
/*2B6A*/void far *curSong = 0;/*the song playing*/
/*2B6E*/struct tVoice voices[32] = {0};/*the sound effects*/
/*2D2E*/unsigned char chanProgram[11] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};/*program of each channel*/
/*2D39*/unsigned char chanVolume[11] = {0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f};/*the music's volume of each channel*/
/*2D44*/unsigned char chanPan[11] = {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40};/*the music's pan of each channel*/
/*2D4F*/unsigned char chanMusicProgram[11] = {0};/*the music's program of each channel*/
/*2D5A*/unsigned char chanList[13] = {0, 1, 3, 4, 5, 6, 7, 8, 9, 11, 10, 2, 12};/*channels 2-9 in two lists: free (from 11), in use (from 12), oldest first*/
/*2D67*/signed char chanPriority[13] = {0};/*priority of the sound effect on each channel*/
/*2D74*/unsigned char chanUser[13] = {0};/*channel: 0 free, 1 music, 2 sound effect*/
/*2D81*/unsigned long tickTime = 10000;/*time of a MIDI tick*/
/*2D85*/long midiTime = 0;/*MIDI time to play (0x10000 a timer tick)*/
/*2D89*/unsigned char drumChan[77] = {/*AdLib: the channel (10-26, see InitSound) of each percussion note*/
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 10, 10, 18, 11, 0, 12, 13, 17, 13, 16, 13, 14, 13, 13, 15, 13, 19, 0, 0,
	0, 0, 21, 0, 0, 0, 26, 26, 25, 20, 20, 0, 0, 21, 21, 22, 23, 0, 0, 24, 0, 20, 0
};
/*2DD6*/unsigned char drumNote[77] = {/*AdLib: the note played for each percussion note*/
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 48, 48, 48, 48, 0, 48, 42, 71, 42, 71, 47, 71, 47, 52, 79, 52, 77, 0, 0,
	0, 0, 71, 0, 0, 0, 72, 79, 79, 64, 58, 0, 0, 89, 84, 48, 72, 0, 0, 36, 0, 96, 0
};
/*2E23*/int musicStopped = 1;/*the music is stopped*/
/*2E25*/int musicFlags = 0;/*music flags: 1 looped or ended, 2 started, 4 keep the next song*/
/*2E27*/void interrupt (far *prevTimer)() = 0;/*the timer interrupt's previous routine*/

/*the address of sound effect n, computed as a number (mov dx,offset voices / add dx,ax;
  &voices[n] gives add ax,offset)*/
#define VOICE(n) ((struct tVoice *)((unsigned)voices + (n) * sizeof(struct tVoice)))

void interrupt SoundTimer(void);
PlaySong(void far *);

/*puts channel a in a list before channel b*/
/*C_2710_000F*/void ChanInsert(register int a, register int b)
{
	register int i;

	for (i = 0; i < 13; i++) {
		if (chanList[i] == b) {
			chanList[i] = a;
			chanList[a] = b;
			break;
		}
	}
}

/*a sound effect of priority prio takes channel ch: moves it to the end of the list in use*/
/*C_2710_0040*/void ChanTake(int ch, int prio)
{
	register int i;

	if (ch >= 2 && ch <= 9) {
		for (i = 0; i < 13; i++) {
			if (chanList[i] == ch) {
				chanList[i] = chanList[ch];
				chanList[ch] = ch;
				ChanInsert(ch, 12);
				chanPriority[ch] = prio;
				break;
			}
		}
		if (sfxReady) {
			sfxReady = 0;
			for (i = 0; i < 32; i++) {
				if (voices[i].chan1 == ch)
					voices[i].chan1 = 0;
			}
			sfxReady = 1;
		}
	}
}

/*frees channel ch: notes off, back to the end of the free list*/
/*C_2710_00D9*/void ChanFree(int ch)
{
	register int i;

	if (ch >= 2 && ch <= 9) {
		disable();
		(*drvController)(ch, 0x7b, 0);
		enable();
		chanUser[ch] = 0;
		for (i = 0; i < 13; i++) {
			if (chanList[i] == ch) {
				chanList[i] = chanList[ch];
				chanList[ch] = ch;
				ChanInsert(ch, 11);
				break;
			}
		}
	}
}

/*finds a channel for a sound effect of priority prio: a free one, or the oldest of no
  higher priority (0: none)*/
/*C_2710_013B*/ChanFind(int prio)
{
	register int i;
	register int ch;

	ch = 0;
	if (chanList[11] != 11) {
		ch = chanList[11];
	} else {
		for (i = chanList[12]; i != 12; i = chanList[i]) {
			if (chanPriority[i] <= prio) {
				ch = i;
				break;
			}
		}
	}
	ChanTake(ch, prio);
	return ch;
}

/*starts the current step of sound effect n (or ends the effect). program is in DI: a
  byte store of program - 1 would keep it off the registers, the >> 0 keeps the
  subtraction a word one*/
/*C_2710_0183*/void SfxStartStep(int n)
{
	struct tSfxStep far *step;
	int note;
	int velocity;
	int duration;
	int vol1;
	int vol2;
	int pan1;
	int pan2;
	register int program;
	register struct tVoice *v;

	if (soundCard) {
		v = VOICE(n);
		step = v->step;
		program = step->program;
		if (program != 0 && v->chan1 != 0) {
			note = step->note;
			velocity = step->velocity;
			duration = step->duration;
			if (!!(v->flags & 1)) {
				pan1 = 0;
				pan2 = 0x7f;
				vol1 = ((v->pan < 0x20 ? 0x7f : 0x9f - v->pan) * v->volume) >> 7;
				vol2 = ((v->pan > 0x5f ? 0x7f : v->pan + 0x20) * v->volume) >> 7;
			} else {
				vol1 = vol2 = v->volume;
				pan1 = pan2 = v->pan;
			}
			disable();
			if (chanUser[v->chan1] != 2) {
				(*drvController)(v->chan1, 0x7b, 0);
				chanUser[v->chan1] = 2;
			}
			(*drvProgram)(v->chan1, program - 1);
			chanProgram[v->chan1] = (program - 1) >> 0;
			(*drvController)(v->chan1, 7, vol1);
			(*drvController)(v->chan1, 10, pan1);
			(*drvNote)(v->chan1, note, velocity);
			if ((!!(v->flags & 1)) && v->chan2 != 0) {
				if (chanUser[v->chan2] != 2) {
					(*drvController)(v->chan2, 0x7b, 0);
					chanUser[v->chan2] = 2;
				}
				(*drvProgram)(v->chan2, program - 1);
				chanProgram[v->chan2] = (program - 1) >> 0;
				(*drvController)(v->chan2, 7, vol2);
				(*drvController)(v->chan2, 10, pan2);
				(*drvNote)(v->chan2, note, velocity);
			}
			enable();
			v->note = note;
			v->counter = duration;
		} else {
			ChanFree(v->chan1);
			if (!!(v->flags & 1))
				ChanFree(v->chan2);
			if (!(v->flags & (unsigned char)2))
				v->chan1 = 0;
		}
	}
}

/*a timer tick of the sound effects: ends steps, glides, starts the next steps*/
/*C_2710_039E*/void SfxTick()
{
	int flags;
	int changed;
	register struct tVoice *v;
	register int n;

	if (soundCard && sfxReady) {
		for (n = 0; n < 32; n++) {
			changed = 0;
			if ((v = VOICE(n))->step == 0 || v->chan1 == 0)
				continue;
			flags = v->step->flags;
			if (flags & 4)
				continue;
			if (--v->counter)
				continue;
			(*drvNote)(v->chan1, v->note, 0);
			if (!!(v->flags & 1))
				(*drvNote)(v->chan2, v->note, 0);
			if (flags & 2) {
				if (v->step->target > v->note) {
					v->note++;
					changed = 1;
				} else if (v->step->target < v->note) {
					v->note--;
					changed = 1;
				} else {
					v->note = v->step->note;
				}
			}
			if (flags & 8)
				changed = 1;
			if (changed) {
				(*drvNote)(v->chan1, v->note, v->step->velocity);
				v->counter = v->step->duration;
			} else if (flags & 1) {
				v->step++;
				SfxStartStep(n);
			} else {
				ChanFree(v->chan1);
				if (!!(v->flags & 1))
					ChanFree(v->chan2);
				if (!(v->flags & (unsigned char)2))
					v->chan1 = 0;
			}
		}
	}
}

/*plays a sound effect (replacing the one of the same id); returns it (0: no channel)*/
/*C_2710_04FB*/struct tVoice far *PlaySound(struct tSfxStep far *steps, int flags, int volume, int pan, int id, int prio)
{
	struct tVoice far *v;
	int ch1 = 0;
	int ch2 = 0;
	register int i;
	register int slot = -1;

	if (soundCard) {
		for (i = 0; i < 32; i++) {
			if (voices[i].chan1 != 0 && voices[i].id == id) {
				ch1 = voices[i].chan1;
				ch2 = voices[i].chan2;
				disable();
				(*drvNote)(ch1, voices[i].note, 0);
				if (!!(voices[i].flags & 1))
					(*drvNote)(ch2, voices[i].note, 0);
				enable();
				voices[i].chan1 = 0;
				break;
			}
			if (voices[i].chan1 == 0)
				slot = i;
		}
		if (ch1 == 0 && slot >= 0) {
			i = slot;
			ch1 = ChanFind(prio);
			if (ch1 != 0 && (flags & 1))
				ch2 = ChanFind(prio);
		}
		if (ch1 != 0 && (ch2 != 0 || !(flags & 1))) {
			v = VOICE(i);
			v->flags = flags;
			v->chan1 = ch1;
			v->chan2 = ch2;
			v->volume = volume;
			v->pan = pan;
			v->id = id;
			v->step = steps;
			SfxStartStep(i);
			return v;
		}
	}
	return 0;
}

/*changes the volume (ctl 7) or pan (ctl 10) of sound effect v (if its id is id, or id 0).
  The last call of the stereo branch must not be merged by the compiler with the
  else branch's: the only way found is a call returning long (DX:AX), hence the cast*/
/*C_2710_0686*/void SetSoundParam(struct tVoice far *v, register int ctl, int val, int id)
{
	int vol1;
	register int vol2;

	if (!soundCard)
		return;
	if (v == 0)
		return;
	if (id != 0 && v->id != id)
		return;
	if (v->chan1 == 0)
		return;
	if (ctl == 7)
		v->volume = val;
	if (ctl == 10)
		v->pan = val;
	if ((!!(v->flags & 1)) && ctl == 10) {
		vol1 = ((v->pan < 0x20 ? 0x7f : 0x9f - v->pan) * v->volume) >> 7;
		vol2 = ((v->pan > 0x5f ? 0x7f : v->pan + 0x20) * v->volume) >> 7;
		disable();
		if (chanUser[v->chan1] == 2)
			(*drvController)(v->chan1, 7, vol1);
		if (chanUser[v->chan2] == 2)
			(*(tDrv3L)drvController)(v->chan2, 7, vol2);/*see above*/
		enable();
	} else {
		disable();
		if (chanUser[v->chan1] == 2)
			(*drvController)(v->chan1, ctl, val);
		if ((!!(v->flags & 1)) && chanUser[v->chan2] == 2)
			(*drvController)(v->chan2, ctl, val);
		enable();
	}
}

/*gives channel ch back to the music*/
/*C_2710_07FC*/void ChanToMusic(register int ch)
{
	if (soundCard && chanUser[ch] == 2) {
		ChanFree(ch);
		disable();
		(*drvController)(ch, 0x7b, 0);
		(*drvProgram)(ch, chanMusicProgram[ch]);
		chanProgram[ch] = chanMusicProgram[ch];
		(*drvController)(ch, 7, (chanVolume[ch] * musicVolume) >> 8);
		(*drvController)(ch, 10, chanPan[ch]);
		chanUser[ch] = 1;
		enable();
	}
}

/*stops sound effect v (if its id is id, or id 0)*/
/*C_2710_086A*/void SfxStop(struct tVoice far *v, int id)
{
	if (soundCard && sfxReady) {
		sfxReady = 0;
		if (v != 0 && (id == 0 || v->id == id) && v->chan1 != 0) {
			ChanToMusic(v->chan1);
			if (!!(v->flags & 1))
				ChanToMusic(v->chan2);
			v->chan1 = 0;
		}
		sfxReady = 1;
	}
}

/*stops sound effect v, or (v 0) all of them, if their id is id (or id 0)*/
/*C_2710_08DC*/void StopSound(struct tVoice far *v, register int id)
{
	register int i;

	if (soundCard) {
		if (v == 0) {
			for (i = 0; i < 32; i++)
				SfxStop(VOICE(i), id);
		} else {
			SfxStop(v, id);
		}
	}
}

/*stops all the sound effects*/
/*C_2710_0927*/void StopAllSounds()
{
	register int i;

	if (soundCard && sfxReady) {
		sfxReady = 0;
		for (i = 0; i < 32; i++)
			voices[i].chan1 = 0;
		for (i = 2; i < 10; i++)
			ChanToMusic(i);
		sfxReady = 1;
	}
}

/*reads a MIDI variable-length number from track t*/
/*C_2710_0972*/long MidiVarLen(register int t)
{
	long n;

	n = 0;
	while (*trackPtr[t] & 0x80)
		n = (n << 7) + (*trackPtr[t]++ & 0x7f);
	n = (n << 7) + (unsigned)*trackPtr[t]++;
	return n;
}

/*p begins with the 4 characters of tag*/
/*C_2710_0A06*/IsChunk(char far *p, register char *tag)
{
	register int i;

	for (i = 0; i < 4; i++) {
		if (*p++ != *tag++)
			break;
	}
	return i == 4;
}

/*the length of the chunk at p (only 16 bits of it)*/
/*C_2710_0A38*/ChunkLength(unsigned char far *p)
{
	long n;

	n = (p[4] << 24) + (p[5] << 16) + (p[6] << 8) + p[7];
	return n;
}

/*a meta event of track t: sets the tempo*/
/*C_2710_0A79*/void MidiMetaEvent(int t, int type, register int len)
{
	long tempo;
	register int i;

	switch (type) {
	case 0x51:
		tempo = 0;
		for (i = 0; i < len; i++) {
			tempo <<= 8;
			tempo += trackPtr[t][i];
		}
		tickTime = tempo / ticksPerQuarter;
		tickTime *= 0x1317;
		tickTime /= 0x4db;
		break;
	}
	trackPtr[t] += len;
}

/*adds a delta time to track t*/
/*C_2710_0B24*/void TrackAddDelta(int t, long delta)
{
	trackDelay[t] += delta;
}

/*the track whose event comes next; advances the time to it*/
/*C_2710_0B3E*/NextTrack()
{
	long min;
	register int i;
	register int t;

	t = -1;
	min = 0x7fffffffL;
	for (i = 0; i < numTracks; i++) {
		if (trackEnded[i] == 0 && trackDelay[i] < min) {
			t = i;
			min = trackDelay[i];
		}
	}
	if (min != 0)
		midiTime += (long)(min * tickTime);
	for (i = 0; i < numTracks; i++)
		trackDelay[i] -= min;
	return t;
}

/*plays the MIDI events due in this timer tick; 1: the song has ended*/
/*C_2710_0BE2*/PlayMidiEvents()
{
	int i;
	int done;
	int d1;
	int d2;
	int type;
	int len;
	int ofs;
	long delta;
	register int t;
	int ch;

	done = 0;
	while (!done) {
		t = NextTrack();
		if (midiTime > 0xffffL)
			break;
		d1 = *trackPtr[t]++;
		if (d1 >= 0x80 && d1 < 0xf0) {
			trackStatus[t] = d1;
			d1 = *trackPtr[t]++;
		}
		if (d1 < 0xf0) {
			ch = (trackStatus[t] & 0xf) + 1;
			if (chanUser[ch] == 0 || chanPriority[ch] < 0) {
				(*drvController)(ch, 0x7b, 0);
				if (chanProgram[ch] != chanMusicProgram[ch]) {
					if (ch != 10)
						(*drvProgram)(ch, chanMusicProgram[ch]);
					chanProgram[ch] = chanMusicProgram[ch];
				}
				(*drvController)(ch, 7, (chanVolume[ch] * musicVolume) >> 8);
				(*drvController)(ch, 10, chanPan[ch]);
				chanUser[ch] = 1;
			}
			if (chanUser[ch] == 1)
				ChanTake(ch, 0);
			switch (trackStatus[t] & 0xf0) {
			case 0x80:
				d2 = *trackPtr[t]++;
				if (musicNotesOn && chanUser[ch] == 1) {
					if (ch == 10 && soundCard == 2)
						(*drvNote)(drumChan[d1], drumNote[d1], 0);
					else
						(*drvNote)(ch, d1, 0);
				}
				break;
			case 0x90:
				d2 = *trackPtr[t]++;
				if (musicNotesOn && chanUser[ch] == 1) {
					if (ch == 10 && soundCard == 2)
						(*drvNote)(drumChan[d1], drumNote[d1], d2);
					else
						(*drvNote)(ch, d1, d2);
				}
				break;
			case 0xa0:
				d2 = *trackPtr[t]++;
				break;
			case 0xb0:
				d2 = *trackPtr[t]++;
				if (musicNotesOn && chanUser[ch] == 1) {
					if (d1 == 7)
						(*drvController)(ch, d1, (d2 * musicVolume) >> 8);
					else
						(*drvController)(ch, d1, d2);
				}
				if (d1 == 7)
					chanVolume[ch] = d2;
				if (d1 == 10)
					chanPan[ch] = d2;
				break;
			case 0xc0:
				if (musicNotesOn && chanUser[ch] == 1) {
					(*drvProgram)(ch, d1);
					chanProgram[ch] = d1;
				}
				chanMusicProgram[ch] = d1;
				break;
			case 0xe0:
				d2 = *trackPtr[t]++;
				if (musicNotesOn && chanUser[ch] == 1)
					(*drvPitchBend)(ch, d1, d2);
				break;
			}
		} else {
			switch (d1) {
			case 0xf0:
			case 0xf7:
				len = MidiVarLen(t);
				trackPtr[t] += len;
				break;
			case 0xfe:/*Origin's: type, length, data; types 0, 2, 4, 5 jump back by the
				  first data word (0: always, 2: when the next song is set, 4/5: when data[2]
				  differs from/equals musicCondition), 1 ends the song, 3 sets musicMarker*/
				ch = (trackStatus[t] & 0xf) + 1;
				type = *trackPtr[t]++;
				len = *trackPtr[t]++;
				if (type == 0) {
					if (nextSongSet) {
						trackPtr[t] += len;
					} else {
						musicFlags |= 1;
						(*drvController)(ch, 0x7b, 0);
						ofs = trackPtr[t][0] + (trackPtr[t][1] << 8);
						trackPtr[t] -= ofs;
					}
				} else if (type == 1) {
					if (nextSongSet) {
						musicFlags |= 1;
						for (i = 2; i <= 10; i++) {
							if (musicNotesOn && chanUser[i] == 1)
								(*drvController)(i, 0x7b, 0);
						}
						return 1;
					}
				} else if (type == 2) {
					if (nextSongSet) {
						musicFlags |= 1;
						(*drvController)(ch, 0x7b, 0);
						ofs = trackPtr[t][0] + (trackPtr[t][1] << 8);
						trackPtr[t] -= ofs;
					} else {
						trackPtr[t] += len;
					}
				} else if (type == 3) {
					musicMarker = trackPtr[t][0];
					trackPtr[t] += len;
				} else if (type == 4) {
					if (trackPtr[t][2] != musicCondition) {
						musicFlags |= 1;
						(*drvController)(ch, 0x7b, 0);
						ofs = trackPtr[t][0] + (trackPtr[t][1] << 8);
						trackPtr[t] -= ofs;
					} else {
						trackPtr[t] += len;
					}
				} else if (type == 5) {
					if (trackPtr[t][2] == musicCondition) {
						musicFlags |= 1;
						(*drvController)(ch, 0x7b, 0);
						ofs = trackPtr[t][0] + (trackPtr[t][1] << 8);
						trackPtr[t] -= ofs;
					} else {
						trackPtr[t] += len;
					}
				} else {
					trackPtr[t] += len;
				}
				break;
			case 0xff:
				type = *trackPtr[t]++;
				len = MidiVarLen(t);
				MidiMetaEvent(t, type, len);
				if (type == 0x2f) {
					trackEnded[t] = 1;
					for (i = 0; i < numTracks; i++) {
						if (trackEnded[i] == 0)
							break;
					}
					if (i == numTracks)
						done = 1;
				}
				break;
			}
		}
		if (!done) {
			delta = MidiVarLen(t);
			TrackAddDelta(t, delta);
		}
	}
	return done;
}

/*stops the music: notes off*/
/*C_2710_126D*/void MusicNotesOff()
{
	register int ch;

	musicStopped = 1;
	curSong = 0;
	musicFlags |= 1;
	for (ch = 1; ch <= 16; ch++) {
		if (chanUser[ch] == 1)
			(*drvController)(ch, 0x7b, 0);
	}
}

/*sets the volume of the music's channels*/
/*C_2710_12AA*/void SetChanVolumes(int vol)
{
	register int ch;

	musicVolume = vol;
	for (ch = 2; ch <= 10; ch++) {
		if (chanUser[ch] == 1)
			(*drvController)(ch, 7, (chanVolume[ch] * musicVolume) >> 8);
	}
}

/*the timer interrupt: fades, plays the music, starts the next song, the sound effects*/
/*C_2710_12E1*/void interrupt SoundTimer()
{
	void far *song;

	if (soundCard) {
		if (fadeStep && !musicStopped) {
			fadeLevel += fadeStep;
			if (fadeLevel < 0)
				fadeStep = 0;
			else
				SetChanVolumes(((fadeLevel >> 8) * musicVolumeSet) >> 7);
			if (fadeStep == 0) {
				if (!fadeNoStop)
					MusicNotesOff();
				SetChanVolumes(musicVolumeSet);
			}
		}
		if (!musicStopped) {
			if (emsInUse == 0 || EMS_saveMap() != 0) {
				EMS_map(curSong);
				musicStopped = PlayMidiEvents();
				EMS_restoreMap();
			}
			if (musicStopped) {
				musicFlags |= 1;
				curSong = 0;
			}
			midiTime -= 0x10000L;
		} else if (nextSong) {
			song = curSong;
			PlaySong(nextSong);
			if (musicFlags & 4) {
				nextSong = song;
				musicFlags &= ~4;
			}
		}
		SfxTick();
	}
	(*prevTimer)();
}

/*rewinds the song's tracks (to play only track only; -1: all)*/
/*C_2710_1402*/void RewindTracks(register int only)
{
	register int i;

	for (i = 2; i <= 10; i++)
		(*drvController)(i, 0x79, 0);
	for (i = 0; i < numTracks; i++) {
		trackDelay[i] = 0;
		if (only == -1 || only == i)
			trackEnded[i] = 0;
		else
			trackEnded[i] = 1;
		trackPtr[i] += 8;
		TrackAddDelta(i, MidiVarLen(i));
	}
	musicStopped = nextSongSet = musicMarker = 0;
	noSongQueued = 1;
	musicFlags &= ~1;
	musicFlags |= 2;
	nextSong = 0;
}

/*fades the music out in time ticks (0: stops it now)*/
/*C_2710_14A6*/void FadeOutMusic(int time)
{
	musicFlags &= ~2;
	if (soundCard) {
		if (!fadeStep)
			fadeLevel = 0x7fff;
		nextSong = 0;
		if (time) {
			fadeStep = -(0x8000 / time);
			fadeNoStop = 0;
		} else {
			fadeStep = 0;
			MusicNotesOff();
			SetChanVolumes(musicVolumeSet);
		}
	}
}

/*loads the sound card's driver from STRAX.DRV and its timbre bank from file name*/
/*C_2710_1500*/LoadSoundDriver(char far *name)
{
	register unsigned seg;
	register int card;

	if (soundCard) {
		card = soundCard - 1;
		drvCode = LZC_load("strax.drv", card, 0L, 0x12, 0L);
		timbreBank = LZC_load(name, card, 0L, 0x10, 0L);
		if (timbreBank == 0 && soundCard != 1) {
			MEM_free(drvCode);
			drvCode = 0;
			soundCard = 0;
		}
		if (drvCode) {
			seg = FP_SEG(drvCode);
			drvInit = (tDrvInit)MK_FP(seg, drvCode[1]);
			drvLoadTimbres = (tDrv1)MK_FP(seg, drvCode[2]);
			drvShutdown = (tDrv0)MK_FP(seg, drvCode[3]);
			drvTimer = MK_FP(seg, drvCode[4]);
			drvNote = (tDrv3)MK_FP(seg, drvCode[5]);
			drvController = (tDrv3)MK_FP(seg, drvCode[6]);
			drvPitchBend = (tDrv3)MK_FP(seg, drvCode[7]);
			drvProgram = (tDrv2)MK_FP(seg, drvCode[8]);
			drvEntry12 = MK_FP(seg, drvCode[9]);
			drvEntry14 = MK_FP(seg, drvCode[10]);
			drvInfo = (*drvInit)();
			drvInfo->cb1 = HookVector;
			drvInfo->cb2 = LoopDelay;
			(*drvLoadTimbres)(timbreBank);
			if (!drvInfo->ok)
				MEM_free(timbreBank);
			return 1;
		}
	}
	return 0;
}

/*starts the music and sound: loads the driver and the timbres, hooks the timer*/
/*C_2710_1666*/void InitSound(char far *name)
{
	if (!soundCard)
		return;
	LoadSoundDriver(name);
	if (soundCard == 2) {
		(*drvProgram)(10, 0x80);
		(*drvProgram)(11, 0x72);
		(*drvProgram)(12, 0x83);
		(*drvProgram)(13, 0x71);
		(*drvProgram)(14, 0x86);
		(*drvProgram)(15, 0x87);
		(*drvProgram)(16, 0x85);
		(*drvProgram)(17, 0x84);
		(*drvProgram)(18, 0x81);
		(*drvProgram)(19, 0x88);
		(*drvProgram)(20, 0x8d);
		(*drvProgram)(21, 0x8f);
		(*drvProgram)(22, 0x90);
		(*drvProgram)(23, 0x91);
		(*drvProgram)(24, 0x93);
		(*drvProgram)(25, 0x8c);
		(*drvProgram)(26, 0x8b);
	}
	SetTimerRate(soundCard == 3 ? 0x1e00 : 0x3c);
	musicStopped = 1;
	curSong = 0;
	nextSong = 0;
	musicFlags = 0;
	disable();
	HookVector(8, (void far *)(long)SoundTimer,/*(long): seg and offset loaded into AX, DX before the pushes*/ musTimerHook, (void far *)&prevTimer);
	HookVector(8, drvTimer, drvTimerHook, drvInfo->unk02);
	enable();
	sfxReady = 1;
}

/*stops the music and sound, shuts the driver down, unhooks the timer*/
/*C_2710_17DA*/void ShutdownSound()
{
	register int i;

	if (soundCard) {
		sfxReady = 0;
		musicStopped = 1;
		curSong = 0;
		nextSong = 0;
		musicFlags = 0;
		for (i = 0; i < 32; i++)
			voices[i].chan1 = 0;
		for (i = 2; i < 10; i++)
			ChanToMusic(i);
		for (i = 1; i <= 16; i++)
			(*drvController)(i, 0x7b, 0);
		(*drvShutdown)();
	}
	UnhookAll();
}

/*starts the MIDI file song (0: not a MIDI file)*/
/*C_2710_185C*/StartSong(void far *song)
{
	int ok;
	unsigned char far *p;
	register unsigned ofs;
	register int t;

	ok = 1;
	musicStopped = 1;
	curSong = song;
	if (emsInUse == 0 || EMS_saveMap() != 0)
		p = EMS_map(song);
	else
		p = 0;
	if (p == 0) {
		ok = 0;
	} else {
		for (ofs = 0; !IsChunk(p + ofs, "MThd"); ofs += ChunkLength(p + ofs) + 8) {
			if (IsChunk(p + ofs, "MTrk")) {
				ok = 0;
				break;
			}
		}
		if (ChunkLength(p + ofs) != 6)
			ok = 0;
		if (ok) {
			midiFormat = (p[ofs + 8] << 8) + p[ofs + 9];
			numTracks = (p[ofs + 10] << 8) + p[ofs + 11];
			ticksPerQuarter = (p[ofs + 12] << 8) + p[ofs + 13];
			if (midiFormat > 2 || midiFormat < 0)
				ok = 0;
			if (numTracks > 32 || numTracks < 0)
				ok = 0;
		}
		if (ok) {
			for (t = 0; t < numTracks; t++) {
				while (!IsChunk(&p[ofs + 14], "MTrk"))
					ofs += ChunkLength(&p[ofs + 14]) + 8;
				trackPtr[t] = &p[ofs + 14];
				ofs += ChunkLength(&p[ofs + 14]) + 8;
			}
			RewindTracks(-1);
		}
	}
	EMS_restoreMap();
	return ok;
}

/*plays song now (0: no sound card or not a MIDI file)*/
/*C_2710_1A62*/PlaySong(void far *song)
{
	if (!soundCard)
		return 0;
	FadeOutMusic(0);
	fadeStep = 0;
	return StartSong(song);
}

/*fades the music out in time ticks, then plays song (flags: musicFlags)*/
/*C_2710_1A8E*/void FadeToSong(void far *song, int time, int flags)
{
	musicFlags = flags;
	if (soundCard) {
		if (!fadeStep)
			fadeLevel = 0x7fff;
		if (time) {
			fadeStep = -(0x8000 / time);
			nextSong = song;
			fadeNoStop = 0;
			noSongQueued = 0;
		} else {
			fadeStep = 0;
			nextSong = 0;
			StartSong(song);
		}
	}
}

/*song is played when the music reaches its end*/
/*C_2710_1AFA*/void SetNextSong(void far *song, int flags)
{
	nextSong = song;
	nextSongSet = 1;
	noSongQueued = 0;
	musicFlags = flags;
}

/*waits until the music sets one of the flags mask, and clears them*/
/*C_2710_1B1E*/void WaitMusicFlags(register int mask)
{
	if (soundCard) {
		while (!(musicFlags & mask))
			;
		musicFlags &= ~mask;
	}
}

/*sets the music volume (0x100: full)*/
/*C_2710_1B3D*/void SetMusicVolume(register int vol)
{
	register int ch;

	if (soundCard) {
		musicVolumeSet = vol;
		if (!fadeStep)
			musicVolume = vol;
		for (ch = 2; ch <= 10; ch++) {
			if (chanUser[ch] == 1)
				(*drvController)(ch, 7, (chanVolume[ch] * vol) >> 8);
		}
	}
}

/*turns the music's notes on or off*/
/*C_2710_1B87*/void SetMusicOn(int on)
{
	register int ch;

	if (!soundCard)
		return;
	if ((on != 0) != musicNotesOn) {
		musicNotesOn = on;
		if (musicNotesOn) {
			for (ch = 2; ch <= 10; ch++) {
				if (chanUser[ch] == 1) {
					if (ch != 10)
						(*drvProgram)(ch, chanMusicProgram[ch]);
					chanProgram[ch] = chanMusicProgram[ch];
					(*drvController)(ch, 7, (chanVolume[ch] * musicVolume) >> 8);
					(*drvController)(ch, 10, chanPan[ch]);
				}
			}
		} else {
			for (ch = 2; ch <= 10; ch++) {
				if (chanUser[ch] == 1)
					(*drvController)(ch, 0x7b, 0);
			}
		}
	}
}
