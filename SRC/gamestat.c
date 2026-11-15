/*
	Game state block saved in the savegame: everything from IsOnQuest to
	GameStateEnd (excluded) is read and written as one block, so the order and
	sizes of these variables must not change. A data-only module (a zero-length
	code segment at 1FA9:2), linked between object.c and sound.c. Savage
	Empire's gamestat.c (Ultima VI's) reworked: no KARMA, WindDir, SpellFx,
	moonstones, powder keg, Gargish, moons; new: the dream's variables
	(WakeX/WakeY, GameFlags, ViewRotation, the wake date, DreamStage,
	DreamFoesLeft), the player's sex and the talents.
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
/*1684*/unsigned char WakeX = 0;/*where the dreamer wakes up: x*/
/*1685*/unsigned char WakeY = 0;/*y*/
/*1686*/unsigned char DrunkCounter = 0;
/*1687*/int16 SoundFlag = 1;
/*1689*/int16 AvatarNotSolid = 0;/*the avatar walks through anything (debug key)*/
/*168B*/unsigned char PathObject[8] = {0};
/*1693*/unsigned char PathCounter[8] = {0};
/*169B*/unsigned char PathTries[8] = {0};
/*the creatures left to kill in a dream stage: counted down as they die
  (COMBAT_Kill, COMBAT_AI_Worm) or when egg.c cannot place one*/
/*16A3*/unsigned char DreamFoesLeft = 0;
/*16A4*/char InCombat = 1;
/*16A5*/char SoloMember = -1;/*solo mode: party index of the solo member, -1 = party mode*/
/*16A6*/char SavedNPCMode = 0;/*NPCMode of the commanded member before it got AI_COMMAND*/
/*16A7*/unsigned char DefaultCommand = 0xFF;
/*16A8*/unsigned char PlayerIsFemale = 0;/*the player is a woman*/
/*16A9*/unsigned char AvatarPortrait = 0;/*0: no character created yet*/
/*16AA*/int32 GameFlags = 0;/*0x10: in a dream (the only bit used)*/
/*16AE*/int16 ViewRotation = 0;/*rotation of the map view*/
/*the talents of NPCs 0-15*/
struct tTalents { unsigned a:4, psychic:4, c:4, d:4; };
/*16B0*/struct tTalents Talents[16] = {0};
/*16D0*/unsigned char WakeDay = 0;/*the day to wake up from the dream*/
/*16D1*/unsigned char WakeMonth = 0;/*the month*/
/*16D2*/int16 WakeYear = 0;/*the year*/
/*16D4*/int16 DreamStage = 0;/*the kind of the last dream trigger*/
/*16D6*/int16 GameStateEnd = 0;/*end marker*/
/*16D8*/int16 D_16D8 = 1;/*not used*/
