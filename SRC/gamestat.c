/*
	Game state block saved in the savegame: everything from IsOnQuest to
	GameStateEnd (excluded) is read and written as one block, so the order and
	sizes of these variables must not change. A data-only module (a zero-length
	code segment at 1FA9:2), linked between object.c and sound.c. Savage
	Empire's gamestat.c (Ultima VI's) reworked: no KARMA, WindDir, SpellFx,
	moonstones, powder keg, Gargish, moons; new: the dream's variables
	(D_1684/D_1685, D_16AA, D_16AE, the wake date, DreamStage), the player's
	sex and the talents.
*/
#include "se.h"

/*1672*/unsigned char IsOnQuest = 0;
/*1673*/unsigned char NextSleep = 0;
/*1674*/unsigned char Time_M = 0;/*minutes*/
/*1675*/unsigned char Time_H = 8;/*hour of day*/
/*1676*/unsigned char Date_D = 6;/*day of month*/
/*1677*/unsigned char Date_M = 1;/*month*/
/*1678*/uint16 Date_Y = 1895;/*year*/
/*167A*/unsigned char Active = 0;
/*167B*/char AmbientLight = 7;/*ambient (sun) light level 0..7, see PassTime*/
/*167C*/int16 MapX = 0x198;
/*167E*/int16 MapY = 0x270;
/*1680*/int16 MapZ = 0;
/*1682*/int16 D_1682 = 0;/*not used*/
/*1684*/unsigned char D_1684 = 0;/*where the dreamer wakes up: x*/
/*1685*/unsigned char D_1685 = 0;/*y*/
/*1686*/unsigned char DrunkCounter = 0;
/*1687*/int16 SoundFlag = 1;
/*1689*/int16 D_1689 = 0;/*the avatar is not solid (debug)*/
/*168B*/unsigned char PathObject[8] = {0};
/*1693*/unsigned char PathCounter[8] = {0};
/*169B*/unsigned char PathTries[8] = {0};
/*16A3*/unsigned char D_16A3 = 0;/*creatures that could not be placed (egg.c)*/
/*16A4*/char InCombat = 1;
/*16A5*/char SoloMember = -1;/*solo mode: party index of the solo member, -1 = party mode*/
/*16A6*/char SavedNPCMode = 0;/*NPCMode of the commanded member before it got AI_COMMAND*/
/*16A7*/unsigned char DefaultCommand = 0xFF;
/*16A8*/unsigned char D_16A8 = 0;/*the player is a woman*/
/*16A9*/unsigned char AvatarPortrait = 0;/*0: no character created yet*/
/*16AA*/int32 D_16AA = 0;/*flags (0x10: the dream)*/
/*16AE*/int16 D_16AE = 0;/*rotation of the map view*/
/*16B0*/struct { unsigned a:4, psychic:4, c:4, d:4; } D_16B0[16] = {0};/*talents of NPCs 0-15*/
/*16D0*/unsigned char D_16D0 = 0;/*the day to wake up from the dream*/
/*16D1*/unsigned char D_16D1 = 0;/*the month*/
/*16D2*/int16 D_16D2 = 0;/*the year*/
/*16D4*/int16 DreamStage = 0;/*the kind of the last dream trigger*/
/*16D6*/int16 GameStateEnd = 0;/*end marker*/
/*16D8*/int16 D_16D8 = 1;/*not used*/
