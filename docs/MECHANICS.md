# Martian Dreams: game mechanics

How *Martian Dreams* (Origin, 1991) plays, as its code implements it. Everything here was
read from the byte-identical decompilation: `GAME.EXE` in `SRC/` and `MARTIAN.EXE`
(character creation, the endgame) in `SRC_M/`. Each rule cites the function that implements
it and its file: "`GetStr` (combat.c)". The sources are fully named;
`python tools/md/mdsym.py --target game list` maps the names to addresses. Where the code
has a bug, the rule is given as the code runs, with the bug noted. Where something could
not be established, the section says so at its end.

The game runs on the Ultima VI engine, by way of *The Savage Empire*. How its engine differs
from Savage Empire's is in [`docs/FINDINGS.md`](FINDINGS.md); this document only says "as in
SE" or "unlike SE" where that helps a reader who knows Savage Empire
(`..\u6se-decompiled\docs\MECHANICS.md`). Where this document first disagreed with
FINDINGS.md, both were checked against the code and FINDINGS.md was corrected (see the end
of *What could not be determined*).

Conventions:

- Numbers are hexadecimal (0x..) for object types, tiles, NPC numbers, talk-flag masks and
  map coordinates, decimal otherwise.
- Object names are the game's own LOOK names (`gamedata/MARTIAN/LOOK.LZC`, through
  `build/names/look.py`): "oxium 0x83". NPC names come from the conversation scripts
  (`dialogue.json`): "NPC 0x10 (Tesla)".
- `TalkFlags[n] & m` is bit m of NPC n's conversation flags; "flag k of NPC n" is the same
  bit numbered 0-7 (flag 4 = mask 0x10).
- Random numbers: `OSI_rand(a, b)` returns a number from a to b, both included. "1dN" is
  `OSI_rand(1, N)`, and "1 in 4" means a test such as `OSI_rand(1, 4) == 1`.

## At a glance

Some of what the code does is not what a player would expect:

- **Only the last answer of Freud's interview counts.** Strength, dexterity and intelligence
  all start from one roll of 18-22; the last of the four questions raises one of them to
  24-26 and another to 22-24. The new character starts at level 4 with 600 experience
  (`CreateCharacter`, SRC_M/main.c).
- **Attacks can miss** (unlike SE, where the to-hit roll was ignored): a hit needs
  1d30 ≥ (defender's DEX + 30 − attacker's DEX) / 2 (`__AttackTest`, combat.c). Damage is
  1d(weapon damage), less 1d(armour).
- **Walking passes no time** (unlike SE). The clock moves one minute each time every actor
  in the area has spent its movement points (`RunNPCTurns`, npctrack.c), and by rests,
  waits and a few events. A month is 28 days, a year 12 months.
- **Hit points never come back with time.** They return by resting more than 4 hours
  (once every 6 hours: (3-8 + 3-8) × hours/2), +30 on a level-up, by healing items and
  scripts, and when the party wakes after defeat. There is no hunger.
- **Resting is done by using a folded tent** (`UseBed`, useact1.c); the R key does nothing.
  A member with the second or third talent cannot sleep, so nobody rests.
- **Levels rise only in the obelisk dream while resting**: one level per member per rest,
  at experience 100, 200, 400, 800, 1600, 3200 and 6400, at most level 8. The sword, heart
  or book raises STR, DEX or INT by 1 (`REST_LevelUp`, rest.c). Experience comes from kills
  and scripts, capped at 9999.
- **Talents come from berries and fade.** Each berry 0x49-0x4b raises one talent by 1 (at
  most 10); each minute a talent drops by 1 with a chance of 1 in 4 × INT; waking from the
  dream clears them all.
- **Mars hurts.** Once the capsule hatch is open, a member without oxium 0x83 gasps (−3 to
  STR, DEX and INT; no damage). On the surface from 22:00 to 3:59 every member freezes,
  losing about 0.75 hit points a minute even when warmly dressed (2.25 when not); in the
  evening and early morning only the poorly dressed freeze. Radium carried outside a lead box poisons; poison costs 1 hit point
  1 minute in 8 and wears off 1 minute in 26.
- **Defeat is not the end** once Tesla has let the party out of the capsule: the party wakes
  a week later at 7 o'clock, at the space capsule or (later) the Olympus mine, with a quarter
  of its experience lost. Before that, it is game over.
- **The story and the machines are kept in the talk flags**: the underground and surface
  power, the canals' water and the pumps are bits of NPCs 0x71, 0x73 and 0x76.
- **Use is table-driven**: 125 rows give each object type its actions, 75 actions say
  which second and third objects they accept (use.c).
- **The dream is played by a copy of the avatar, NPC 0**, sent in by the dream machine. The
  party wakes at the next 8:00 after the dream began, however long the dream took
  inside (the clock is set, not run).

Sections: *Character creation*; *Conversations and quest state*; *Time, rest, stats and
levels*; *Combat*; *Objects and commands*; *The use command*; *The map and machinery*;
*The dream*; *Cut scenes and the endgame*; *Debug keys*; *What could not be determined*;
*Sources*.


## Character creation

The character is made by `MARTIAN.EXE`, not by the game. This section covers how
`MARTIAN.EXE` starts and hands over to `GAME.EXE`, Freud's interview on the space bullet
and exactly how its answers set the attributes, the values the new character gets, what is
written into the saved game, and the starting party. Everything here is in `SRC_M/main.c`
unless another file is named. The endgame and credits that `MARTIAN.EXE` also plays are in
*Cut scenes and the endgame*.

### From MARTIAN.EXE to GAME.EXE

`main` (SRC_M/main.c) seeds the random numbers from the clock, then reads the first 6
bytes of `CONFIG.MD` and, after it, every command-line argument, each through
`ParseConfig`. A letter later in the string or on the command line overrides an earlier
one; any other character is ignored, and parsing stops at a carriage return.

| Letter | Effect |
|---|---|
| `V` / `E` / `T` | graphics: VGA (mode 0x13), EGA (mode 0x0D), Tandy |
| `M` / `K` | mouse / keyboard |
| `N` or `P` | no music |
| `A` / `R` | music on AdLib / on Roland |
| `X` | skip the introduction (`Intro`) after the title |
| `!` | play only the endgame and the credits, then exit to DOS |

`!` is what `GAME.EXE` passes at the end of the game (see *The end of the game*). Without
it, `MARTIAN.EXE` plays the title (`PlayTitle`, SRC_M/title.c: the letters of `TITLE.LZC`
change and fly apart, the sun sets behind the mountains, sparkles), then `Intro` unless `X`
was given, then shows the main menu (`mdmenu.lzc`). The menu has four buttons, handled in
the order of the code:

| Button | Action |
|---|---|
| 1st | `PlayIntro`: the story introduction (the lecture, the cannon, the space bullet and the moongate, Mars; `scenetxt.lzc` items 0-2, `scene1`-`scene4b.lzc`) |
| 2nd | `CreateCharacter` (below) |
| 3rd | runs `GAME.EXE` with `execl("game.exe", 0)`: no arguments, not even a program name |
| 4th | `ScrollCredits` (`scenetxt.lzc` item 4 scrolling over `mars.lzc`) |

The arrow keys move the selection (right/down to the next button, left/up to the previous,
wrapping round), Enter chooses. Backspace, or 120 seconds without a key, replays `Intro`
(the idle timeout sets the key to 8, the code of Backspace). Ctrl+E, Ctrl+Q and Alt+X leave
to DOS. `Intro` itself is the attract sequence: Origin's logos, the 1893 World's Fair,
Lowell, the space bullet's flight and the cabin, Mars, with credit pages in between. In
both introductions Esc (or Ctrl+E, Ctrl+Q, Alt+X) stops the sequence; another key skips to
the next scene (`Pause`).

The menu runs `GAME.EXE` whether a character exists or not. `GAME.EXE`'s `main`
(SRC/main.c) loads the saved game and, if `AvatarPortrait` is 0, prints "You must first
create a character.", waits for a key, shuts down and runs `martian.exe` with no argument
(in an endless `execl` loop). It also builds a configuration string (`BuildConfigString`)
but does not pass it: `MARTIAN.EXE` reads `CONFIG.MD` again, and so plays the title (and
`Intro`, unless `CONFIG.MD` holds an `X`) before the menu comes back.

### The interview

`CreateCharacter` first reads `savegame\objlist`: its first 0x1CF1 bytes (the object and
NPC arrays) and the next 0x62 bytes (the start of the game-state block, `SRC_M/OBJHDR.ASM`).
If the file cannot be opened or read it returns at once, silently. If `AvatarPortrait` is
already 1 it prints "Run INSTALL to remove your existing character before creating
another." and waits for a key: a character can be made only once per saved game.

Otherwise it rolls three values, once each (`Random` is inclusive, as `OSI_rand`):

| Value | Roll |
|---|---|
| hi | 24-26 |
| mid | 22-24 |
| lo | 18-22 |

Strength, dexterity and intelligence all start at **lo** (the same roll). The interview
then plays as a dialogue (`PlayDialogue`, lines from `scenetxt.lzc` item 5 in the order of
the table `creationLines`; the pictures are `create.lzc` 0-2, the song is `mdd_mus.lzc` 7).
Freud sits down next to the player and asks:

1. **The name.** Letters A-Z only, 1 to 12 of them; Backspace deletes, Enter accepts once
   there is at least one letter. Every `$P` in later lines is replaced by it.
2. **The sex**: which parent do you identify with, (A) father or (B) mother. A makes a
   man, B a woman. Freud comments, by sex.
3. **Four questions**, answered with A or B (the first with A, B or C). Any other key is
   ignored. The first three answers only choose which question comes next; **only the
   last answer sets the attributes**.

The questions, by the label used below:

| Label | Question (in short) | A | B | C |
|---|---|---|---|---|
| Q1 | Freud says you do not belong on the expedition | anger | fear | neither |
| P | a journey to see a friend of the opposite sex | warm conversation | warm bath | |
| O | your mother calls you in from play | obey | defy her | |
| X | exploring the Martian surface | send a scout | stay together | |
| D | a business breakfast | doughnuts | eclairs | |
| F | the animal you would become | fish | bird | |
| M | a monument to your beliefs | grand tower | garden | |
| E | your place of escape | alone | with a companion | |
| I | your employer breaks the law | tell the authorities anonymously | confront him | |
| T | your most recent journey | to see a friend | on business | |

The order of the questions (each arrow is an answer):

| Q1 | 2nd | 3rd | last question |
|---|---|---|---|
| A (anger) | P | A → O | A → D, B → F |
| | | B → X | A → M, B → E |
| B (fear) | X | A → O | A → D, B → F |
| | | B → P | A → I, B → M |
| C (neither) | O | A → X | A → M, B → F |
| | | B → P | A → D, B → T |

The last answer sets one attribute to **hi** and another to **mid**; the third keeps
**lo**:

| Last question | Answer A | Answer B |
|---|---|---|
| D | Str hi, Int mid | Int hi, Str mid |
| F | Int hi, Dex mid | Dex hi, Int mid |
| M | Str hi, Dex mid | Dex hi, Str mid |
| E | Int hi, Dex mid | Dex hi, Int mid |
| I | Int hi, Str mid | Str hi, Int mid |
| T | Dex hi, Str mid | Str hi, Dex mid |

All six orderings of the three attributes can be reached. Because the three rolls overlap
(24 can be both hi and mid, 22 both mid and lo), two attributes may come out equal. After
answers A and C of question Q1 Freud remarks on them; after every last answer but D-B he
comments
(by sex after D-A and after M), and the interview ends with his thanks. The comments
change nothing.

### What the new character gets

After the interview `CreateCharacter` sets the Avatar (object 1):

| Value | Set to |
|---|---|
| Strength, dexterity, intelligence | as above: one hi (24-26), one mid (22-24), one lo (18-22) |
| Level | 4 |
| Experience | 600 |
| Hit points | 2 × strength + 4 × 24 = 2 × strength + 96 (132-148), the same as `MaxHP` (combat.c) for a player-controlled NPC |
| Movement points | 20, written into the second of the saved game's two movement-point arrays (`GAME_Load`, console.c, reads both into `MovePts`, so the second wins) |
| Shape | type 0x157 for a man, 0x158 for a woman (LOOK names both "yourself"), frame kept; also the original shape of objects 0 and 1 |
| `PlayerIsFemale` | 1 for a woman, else 0 |
| `AvatarPortrait` | 1: a character now exists |
| Name | party slot 0's name |

There is **no portrait choice**: `GAME.EXE` draws face 1 of `mdfaces.lzc` for a man and
face 0 for a woman (`DrawPortrait`, gfx.c, by `PlayerIsFemale`). In MD `AvatarPortrait`
means only "a character exists". The sex also picks the dreamer's shape (0x157/0x158,
`DreamSleep`, dream.c), the "Sir"/"Madam" of the conversations (`$G`, talk.c) and the
second picture of each cut scene (see *Cut scenes and the endgame*).

`CreateCharacter` sets **no talent**, no inventory, no position and no date: those come
from the saved game as installed (for talents see *Time, rest, stats and levels*).

### What is written to the saved game

`CreateCharacter` opens `savegame\objlist` for writing without truncating it, writes back
the 0x1CF1 + 0x62 bytes it read, with these changes, and leaves the rest of the file as it
was. No write is checked.

| File offset | Size | GAME.EXE variable | Value |
|---|---|---|---|
| 0x402 | word | `ObjShapeType[1]` | type 0x157 or 0x158, frame bits kept |
| 0x901 | byte | `STREN[1]` | strength |
| 0xA01 | byte | `DEXTE[1]` | dexterity |
| 0xB01 | byte | `INTEL[1]` | intelligence |
| 0xC02 | word | `ExpPoints[1]` | 600 |
| 0xE01 | byte | `HitPoints[1]` | 2 × strength + 96 |
| 0xF00 | 14 bytes | `Names[0]` (party slot 0) | the name, zero-ended |
| 0xFF2 | byte | `Level[1]` | 4 |
| 0x15F2 | byte | second `MovePts` array, [1] | 20 |
| 0x16F1 | word | `OrigShapeType[0]` | the new shape |
| 0x16F3 | word | `OrigShapeType[1]` | the new shape |
| 0x1D27 | byte | `PlayerIsFemale` (game state + 0x36) | 0 or 1 |
| 0x1D28 | byte | `AvatarPortrait` (game state + 0x37) | 1 |

The offsets follow from the order in which `GAME_Load` (console.c) reads the file: 0x100
bytes of `ObjStatus`, 0x300 of positions, 0x200 of shapes, and so on up to `PathData`,
which ends at 0x1CF1; the game-state block (gamestat.c, from `IsOnQuest`) follows.

### The starting party

`CreateCharacter` does not set the party; it comes from the installed
`savegame\objlist`. In the GOG release's copy (no character yet) the party is objects 1,
2 and 3 (party size 3), all on the surface, with the leader at (0x1A1, 0x274, 0), at 8:00
on 6 January 1895. All talents are 0.

| NPC | Slot name | Str | Dex | Int | Level | Experience | Hit points |
|---|---|---|---|---|---|---|---|
| 1 (the Avatar), before creation | "Avatar" | 15 | 15 | 15 | 1 | 50 | 30 |
| 0x02 (Spector) | "Spector" | 16 | 18 | 22 | 2 | 120 | 80 |
| 0x03 (Nellie Bly) | "Nellie_Bly" | 14 | 21 | 19 | 1 | 43 | 52 |

The Avatar's row is replaced by creation; the others' hit points equal their `MaxHP`.
These are data, not code: another release's saved game may differ.

### What could not be determined

- The labels of the menu's four buttons are in the picture `mdmenu.lzc`; the table above
  gives them by what the code does.
- What `INSTALL.EXE` does to remove a character is not in these sources.
- The scripts and pictures of the two introductions were not followed scene by scene; only
  their files and the keys that stop them are given.

## Conversations and quest state

Martian Dreams keeps the conversation engine of Ultima VI and Savage Empire: each NPC has a
compiled script in `talk.lzc`, run by a small interpreter (`talkdr.c`, overlay 2F88), with
the entry points, the party routines and the scripted events in `talk.c` (overlay 2F83). The
story is tracked with per-NPC **talk flags**, which scripts set and test, and which many
engine routines (the machinery, the use actions, combat, the dream) also read and write.
This section describes both, as the code implements them, and how the state is saved.

Notation: NPC numbers are object numbers (hex). "Flag *n* of NPC *x*" is talk flag *n*
(0-15) of that NPC, as scripts address it with `SET`/`CLR`/`TST`; for flags 0-7 the engine
writes `TalkFlags[x] & mask`, so flag 2 of NPC 0x73 is `TalkFlags[0x73] & 4`. NPC names
come from the scripts (`dialogue.json`); where two scripts carry the same
name, the description tells them apart.

### Starting a conversation

A conversation starts in one of these ways. All of them go through `TALK_talkTo` (talk.c);
the third argument `showName` is 1 only for the Talk command and the debug key.

| How | Code | Notes |
|---|---|---|
| The Talk command (`T`, or the talk icon) and a target | `GameLoop` → `TALK_talkTo(Active, SelectionObj, 1)` (gameloop.c) | The cursor for Talk has `SelectRange = 7` (squares from the active member). An empty square gives object −1. |
| An NPC in A.I. mode 0x96 ("converse") comes to the party | `NPC_TakeTurn`, `ApproachNearestMember`, `TalkToParty` (npctrack.c) | When the NPC's schedule spot is within 4 squares of the view's centre on both axes, the NPC walks toward a living party member it can see (the code aims at the last member seen, not the nearest); once next to one, its mode becomes 0x90 and it talks. Otherwise it loiters near its spot. NPCs 0x16 and 0x24 start in this mode; NPC 4 (Dibbs) has a 0x96 schedule entry. |
| An NPC in mode 0x95 waits for NPC 1 | `TalkWhenNear` (npctrack.c) | On the party's level and inside the loaded area: farther than 2 squares (`COMBAT_range`) from NPC 1 it only spends 5 movement points; within 2 it talks. The mode is not changed, so it talks again every turn until its script ends the matter. NPC 0x3c (Jean) starts in this mode and has it in his schedule; his script kills him with event 2. |
| The party wakes after its death | `ResurrectParty` (map.c) | NPC 0x12 (Blood) talks, with its flag 5 set. See *The party's death and waking*. |
| Use and look actions, combat, the dream | many | Each sets a flag of the NPC first, so its script knows why it is talking; the list is in *Flags the engine itself reads or writes*. |
| Debug key 243 (0xF3): NPC number | `GameLoop` | `TALK_talkTo(Active, n, 1)`. See *Debug keys*. |

Unlike SE there is no automatic first conversation at the start of the game.

#### `TALK_talkTo` (talk.c)

1. If `showName` is 0 and the party is in solo mode, solo mode is lifted for the
   conversation (`SoloMember = -1`) and restored at the end, so engine-started conversations
   work in solo mode.
2. **Talking to an object** (number ≥ 0x100, or −1) when the active member has the
   **second talent** (`Talents[Party[Active]].psychic`, the high four bits of the talents'
   first byte): if the object's `ObjStatus & 1` is clear, it prints "You receive no psychic
   impressions." and stops. If it is set, the object's type decides:

   | Object type | Result |
   |---|---|
   | conveyor belts 0xbc, 0xbf; bare rollers 0xc0 | conversation with NPC 0x72 (Conveyor) |
   | tower 0xc8, tower top 0xc9, power cables 0xd6, 0xd7 | conversation with NPC 0x73 (Tower) |
   | dream machine 0x64, 0x120, 0x121; power unit 0x122 | conversation with NPC 0x74 (Machine) |
   | strange saw 0xf7 | "This saw was used by a Martian doctor, or "Arborist", for surgery." |
   | injector 0xf9 | "This device was used by a Martian doctor, or "Arborist", on patients." |
   | masonic symbol 0x5c | "This was worn last by a dying man." |
   | ruby slippers 0x0c | "You receive a momentary impression of a house falling on you." |
   | gong hammer 0xe7 | "This was used by a Martian whose entire life focussed on measuring time." |
   | martian jewelry 0xea | "This jewelry was worn by a distinguished Martian, a mayor or "Agrarian"." |
   | footbags 0xf6 | "These footbags were worn by ... "Gatherer", ... undertaker, dustman, and treasurer." |
   | strange sculpture 0x113 | "...a city, or "Grove", of Martians. The image is of "very proud soil"." |
   | any other type | nothing |

   The texts are in the code (`TALK_talkTo`), not in a data file. After a text, or for an
   unlisted type, the object number is still ≥ 0x100, so the function goes on to print
   "nothing!" as well, the same as for an object without the talent. A member without the
   talent always gets "nothing!" for an object.
3. **Talking to an actor** (0-0xFF). An actor with `ObjStatus & 1` (NPCs 0x72-0x74 in the
   shipped data) answers only a member with the second talent; otherwise "nothing!".
   - Object 0 or 1 ("yourself"; with `showName` it prints "yourself"): in the dream
     (`GameFlags & 0x10`) "You try to wake yourself up." and, unless on level 3,
     `DreamWakeUp` (see *The dream*); in solo mode "Not while in solo mode!"; otherwise "You
     aren't yet that insane."
   - An actor of type 0x187 ("your mother", in the dream): its description and "No backtalk
     from you, young man!" ("woman" for a female player). No conversation.
   - Anyone else: with `showName` it prints the name if the actor's flag 0 ("met") is set
     (`TALK_GetCharName`), otherwise its LOOK description; then `TALK_initTalk`.
4. After the conversation, if a script event left a number in `TalkEvent`, one animation
   tick runs and the event's second part runs (see *Scripted events*); `TalkEvent` goes back
   to −1.

`TalkDriver` (talkdr.c) then refuses the conversation, in this order:

| Check | Message |
|---|---|
| Solo mode with a member other than party slot 0 (`SoloMember` not −1 or 0) | "Not while in solo mode!" |
| Target dead (`NPCStatus & 0x10`) or asleep (`& 0x04`), or its A.I. mode is 0xa0 (an NPC that was attacked and assaults), 0x1b or 7 (retreating modes) | "There is no response." |
| (the active member turns to face the pointer) | |
| Target is object 0, or has no script | "There is no response." |

Unlike SE there is no "Talking to yourself?" test (`TALK_talkTo` handles it), no test that a
party member being talked to is near, and no `IsArmageddon` test.

#### Which script an actor uses

`ConversationNum` (gfx.c) gives the `talk.lzc` entry; `LoadConversation` and
`HasConversation` (gfx.c) load or test it.

| Actor | `talk.lzc` entry |
|---|---|
| 0 | none (`TalkDriver` refuses object 0 anyway) |
| 1 - 9, 0x10 - 0xBE | the actor's own number (unlike SE, no offset) |
| 0x0A - 0x0F | 0x51 (Melies' script; the six dream figures of NPC 0x51's dream share it) |
| 0xBF | none |
| 0xC0 - 0xFF (temporary creatures) of type 0x179 (a wisp) | 0x1d (Wisp) |
| other temporary creatures | none |

An entry whose size is 0 or less means "no script". An entry of 0x36b0 bytes or more is
refused: it prints "Aargh!" and counts as no script (0x36b0 is the room before
`ScratchBufHigh`). Entry 0x1e is a second copy of Cooter's script (its NPC byte is 0x18).

An actor's name is the text between the script's NPC byte and the first byte ≥ 0x80
(`TALK_GetNpcName`, talk.c). For an actor with no script it prints "Aargh!" and returns
"NPC 0x" and the number in hex (unlike SE, no "a " prefix; there are no shared scripts with
a `#N` index). Party members are named from the party name table (`TALK_GetCharName`). The
portrait panel (`ShowPortrait`, objcmd.c) names an NPC only once its flag 0 is set, and
shows NPC 0x6d's face for NPC 0x67 (Shartek) until flag 2 of NPC 0x67 is set; an actor with
`ObjStatus & 1` shows the player's own face, labelled "yourself". Temporary creatures get no
portrait (`DrawPortrait` draws nothing from 0xbf up), so the wisp talks without one.

### The conversation, step by step

`TALK_initTalk` (talk.c) sets the variables (below), hides the mouse and calls
`TalkDriver(speaker, npc)` with speaker 1, or 0 in the dream. `TalkDriver` (talkdr.c):

1. Refuses as above. Sets `IsInConversation` (deaths it causes print no "killed"
   message), clears the map window, loads the script, copies the name into `$N`, shows the
   portrait and opens the large text window (`TextWindows[2]`), status view 0x9E.
2. Prints "You sense " for an actor with `ObjStatus & 1`, else "You see ", then the
   description (the text after `OP_DESC`). It remembers where the description starts.
3. Skips to `OP_MAIN` and runs the statements there, unless the first opcode is `ASKTOP`.
4. Loops: reads an input opcode, gets the input, sets `$Z` to it, then runs the statements
   that follow (normally the keyword blocks). Typing **look** (any case) reprints "You see"
   and the description instead and asks again. The map animation stops while it waits
   for a key (`MapAnimEnabled = 0`).
5. Stops when a statement executes `LEAVE`, at `OP_SCRIPT_MARK` (end of script), or on a
   script error ("Command for input key expected.", "Logical error occured.", a malformed
   `GETSTR`/`GETINT`/`GETDIGIT`).
6. Prints "Press any key...", waits, redraws the command mark (the combat form in combat),
   the map window, and **always** restores the status area it found (SE did so only when
   the member shown had left the party).

The conversation passes no game time.

### Script format

A script is a byte stream. Bytes below 0x80 are text; bytes 0x80 and up are opcodes. Layout
(the same as SE's):

```
FF <npc number> <name> F1 <description...> F2 <main statements...>
F7 (or another input opcode)
EF "kw1,kw2,..." F6 <answer statements> EE
EF "..." F6 ... EE
...                         (usually a jump back to the input opcode)
```

#### Text

Text bytes go through `CON_putch` (console.c), so the in-text escapes of all game messages
work in scripts:

| Escape | Effect |
|---|---|
| `#c` | prints integer variable `#c` (c = 0-9, A-Z, either case; any other c means `#0`) |
| `$c` | prints string variable `$c` (same rule) |
| `/` | the letters that follow print only if the last printed number was 1 |
| `\` | the letters that follow print only if it was not 1 (both end at the next non-letter) |
| `^c` | prints control character `c & 0x1F` (special glyphs) |
| `@` | the next word is a keyword: red while help is on (Ctrl+H toggles `HelpOn`; on at start) |
| `*` | waits for a key |
| `` ` `` | capitalises the next character |
| `%` | cancels a pending escape |

The texts write `&` where a closing quotation mark appears on screen (uncertain: it is not an
escape in `CON_putch`, so it must be a glyph of the font). A text byte below 0x20 other than
newline prints "Try to print out a control char" and ends the statement run
(`parse_statement`).

`parse_statement` executes statements until it meets byte 0, `ENDRES`, `GET`, `GETSTR`,
`ASKTOP` or any opcode ≥ 0xF0, or until `LEAVE` has run. A `KEY` block met there is matched
against the input (only the first matching block answers; see *Keywords and input*).

#### Statements

Arguments marked *expr* are each one expression ending with `OP_END_OF_FACTOR` (0xA7). An
"npc" argument goes through `mk_npcnum`: `OP_NPC` (0xEB) stands for the NPC being talked to,
and **in the dream NPC 1 stands for NPC 0** (the dreamer), so a script's references to the
player follow the dreamer. Statements are run by `execute_op` (talkdr.c).

| Opcode | Name | Arguments | Effect |
|---|---|---|---|
| 0xA1 | IF | expr | False (0): skips to the next `ELSE` or `ENDIF` byte. No nesting. While skipping it steps over the operands of `GOTO` (4 bytes), `BYTE` (1) and `WORD` (2), not those of `ADDRESS`. |
| 0xA3 | ELSE | | Skips to the next `ENDIF` (same rules). |
| 0xA2 | ENDIF | | Nothing. |
| 0xA4 | SET | npc, bit | Sets talk flag *bit* (0-15) of npc. Bits > 15: nothing. |
| 0xA5 | CLR | npc, bit | Clears it. |
| 0xA6 | LET | destination, `OP_LET_VALUE`, value | `<i> B2` (`#i`) or an array element (`ADDRESS` ...) takes an expression; `<i> B3` (`$i`) takes a constant string or another `<j> B3`, copied into `$i`'s buffer. |
| 0xB0 | GOTO | 4-byte offset | Jumps. |
| 0xB5 | PRINTSTR | `ADDRESS` string, or `<i> B3 <byte>`, or 0xD5 | Prints a constant string, or `$i`, or nothing. |
| 0xB6 | LEAVE | | Ends the conversation. |
| 0xB9 | GIVEOBJ | npc, type, quality, quantity | `GiveObj`; `#T` = **1 if it returned −1**, else 0 (SE: the reverse). |
| 0xBA | TAKEOBJ | npc, type, quality, quantity | `TakeObj`. |
| 0xBF | SHOW_CONVERSE | npc | Shows npc's portrait. |
| 0xC8 | MOVEOBJ | object, owner | Puts the object into owner's inventory. (`mk_npcnum` is applied to the first argument only.) |
| 0xC9 | TRANSFEROBJ | type, quality, from, to | Moves the first such object of *from*'s inventory to *to*'s; a missing object is not checked for. |
| 0xCB | WAIT | | Waits for a key. |
| 0xCD | SETMODE | npc, mode | Sets the A.I. mode. Mode 0x91 also puts it to sleep. |
| 0xD0 | DELAY | n | Runs n × 20 animation ticks. |
| 0xD1 | EVENT | n | `TALK_Event(n)` (below). |
| 0xD6 | RESURRECT | npc | Revives the first body (type 0x155) in npc's inventory (`SPELL_ReviveNPC`). `#R` = the revived NPC and `$Y` = its name, or `#R` = −1. |
| 0xD8 | SETNAME_Y | npc | `$Y` = npc's name. |
| 0xD9 | HEAL | npc | Hit points to `MaxHP`. |
| 0xDB | CURE | npc | Clears poison. |
| 0xDF | SETNAME_Y_YOU | npc | `$Y` = npc's name, or "you" for NPC 1. |
| 0x9C, 0x9E, 0xBE, 0xCF, 0xA7 | | | Do nothing (0x9C was U6's "get horse", 0x9E "rest"). |
| others ≥ 0x80 | | | Print "Unknown command.". |

`GiveObj` (object.c) returns −1 for an invalid owner, or when the object does not fit in
the inventory and `GiveObjRefuses` is set; with `GiveObjRefuses` at 0 what does not fit is
dropped at the owner's feet and counts as given. `GiveObjRefuses` (D_EBC0) is no variable
of its own: it is word 29 of `VarInt` (D_EB86 + 2 × 29), that is **`#T`** itself
(`SRC/BSS.ASM`). So `#T` works both ways:

- Before the gift, `#T` = 1 asks for a refusal, `#T` = 0 for a drop. `TALK_initTalk` sets
  it to 0, and every `GIVEOBJ` leaves it 0 unless the gift was refused.
- After the gift, `#T` = 1 means "refused, nothing given".

The scripts use it so: Spector, Tesla, Garrett, Hearst, Nathaniel, Edison, Carnegie,
Bernhardt and others write `#T := 1` before `GIVEOBJ` and then test `#T` to say "put
something down first" (`dialogue.json`). Jean (NPC 0x3c) and Clemens give once with
`#T := 1`, and on a refusal set `#T := 0` and give again, which drops the object at the
player's feet ("He drops his Masonic symbol at your feet."). Hearst's (NPC 0x34)
spoon sale, as `dialogue.json` decodes it, gives without setting `#T` just before and then
takes the oxium only `if #T`; with `#T` at 0 the spoons would be given (or dropped) free
with "You couldn't carry that many spoons." This looks like a script bug (not checked in
play).

The value an `IF` gets is checked for −1 (0xFFFF), which makes `execute_op` return an error
that nobody reads: such an `IF` does not skip, so it behaves as true.

#### Expressions

Expressions are postfix (`parse_factor`, talkdr.c). The stack is a local array of ten
32-bit values with no overflow check. `BYTE` (0xD3) pushes the next byte, `WORD` (0xD4) the
next 16-bit word, `ADDRESS` (0xD2) the next 32-bit value (a script offset). Any other byte
that is not an operator is pushed as its own value, so 0x00-0x7F are small constants and
`OP_NPC` is pushed as 0xEB. The expression ends at `OP_END_OF_FACTOR` or `OP_LET_VALUE`; the
result is the bottom entry, cut to 16 bits. More than one entry left prints "Error
occured.".

Operators pop their operands (listed in push order) and push one result:

| Opcode | Operands | Result |
|---|---|---|
| 0x81 > , 0x82 >= , 0x83 < , 0x84 <= | a, b | 1/0, compared as unsigned 32-bit |
| 0x85 != | a, b | 1/0 |
| 0x86 == | a, b | 1/0; once the expression has used a string variable, compares the two as strings, case-insensitive |
| 0x90 + , 0x91 − , 0x92 × | a, b | a+b, a−b, a×b |
| 0x93 ÷ | a, b | a ÷ b unsigned; b = 0 prints "Divide error from talkdr." and leaves a |
| 0x94 OR , 0x95 AND | a, b | logical, 1/0 |
| 0xA0 RND | min, max | `OSI_rand(min, max)` of the low bytes |
| 0xAB TST | npc, bit | talk flag *bit* (0-15) of npc, 1/0; for bit > 15 the npc value stays as the result |
| 0x9A CANCARRY | npc | `GetStr` × 200 − (equipment + inventory weight) × 10, at least 0 (SE used the raw strength) |
| 0x9B WEIGHT | type, quantity | quantity × type weight, × 10 unless the type is weighed per ten |
| 0x9D HORSED | npc | always 0 |
| 0x9F OWNS | npc, type, quality | 1 if npc carries such an object (containers included) |
| 0xB2 VARINT | i | `#i` |
| 0xB3 VARSTR | i | pointer to `$i` |
| 0xB4 DATA | array, index | the 16-bit word at array + 2×index |
| 0xB7 STRSEARCH | string array, i | index of the first entry whose first min(4, length) characters match a word of `$i`; the number of entries if none. The list ends with 0xB8. |
| 0xC3 VALSEARCH | word array, i | index of the entry equal to `#i`, or 0x8001. The list ends with 0xB8. |
| 0xBB TEST_OBJ | npc, type | `TestObj(npc, type)` |
| 0xBC, 0xBD | a, b | always 100 |
| 0xC1 OWNER | object | the object's owner field; also rewrites the next script byte in place through `mk_npcnum` |
| 0xC2 OBJTYPE | object | its type |
| 0xC6 ISINPARTY | npc | 1 if player-controlled, on screen (`IsNpcOnScreen`) and not dead, asleep or paralysed |
| 0xC7 WHOSGOT | type, quality | the first on-screen party member carrying it (in the dream only NPC 0 is tried, giving 0), or 0x8001 |
| 0xCA JOIN | npc | `JoinParty` result (below) |
| 0xCC LEAVEPARTY | npc | `LeaveParty` result (below) |
| 0xD7 ISONSCREEN | npc | 1 if inside the visible 9×9 and on a visible square (`IsNpcOnScreen`, talk.c; no vehicle case in MD) |
| 0xDA WOUNDED | npc | 1 if hit points < `MaxHP` |
| 0xDC POISONED | npc | the poison bit |
| 0xDD PARTYMEMBER | index, any | the NPC in party slot *index*; if *any* is 0, the *index*-th on-screen member; 0 if none |
| 0xE0 ADDEXP | npc, n | adds n to experience, capped at 9999; result is the new value |
| 0xE1 ADDLVL | npc, n | adds n to the level (no cap) |
| 0xE2 ADDSTR, 0xE3 ADDINT, 0xE4 ADDDEX | npc, n | adds n, capped at 30; the stats are bytes, so a negative n that goes below 0 wraps round and is then capped to 30 |

The shipped scripts use only `TST`, `ISINPARTY`, `ISONSCREEN`, `WHOSGOT`, `TEST_OBJ`, `JOIN`,
`LEAVEPARTY`, `RND`, `PARTYMEMBER`, `WOUNDED` and `POISONED` of the special operators
(`dialogue.json`).

#### Variables

There are 36 integer variables `#0`-`#9`, `#A`-`#Z` (`VarInt`) and 36 string variables
`$0`-`$9`, `$A`-`$Z` (`VarStr`); letter *X* is index `X − 0x37` (10-35). `TALK_initTalk`
(talk.c) sets these at the start of every conversation:

| Variable | Value |
|---|---|
| `#0`-`#9` | 0 |
| `$0`-`$9` | point at ten 100-byte buffers (`TalkStrBuf`); the contents are not cleared |
| `#A`, `#I`, `#S` | NPC 1's effective dexterity, intelligence, strength (`GetDex`, `GetInt`, `GetStr`) |
| `#E`, `#P` | NPC 1's experience and hit points |
| `#D`, `#M`, `#H` | day, month, hour |
| `#G`, `$G` | 1 and "Madam" for a female player, 0 and "Sir" for a male one |
| `#N` | the party size (SE: the index of a shared script) |
| `#O` | the number of actor slots 0-0xFF that `OBJ_IsOnScreen` accepts: within 5 squares across and 4 down of the view's centre (or the far half of a double-size tile just outside). No level, life or empty-slot test, so it counts party members, the dead and actors on other levels too. |
| `#T` | 0 (then `GIVEOBJ` sets it) |
| `#W` | the NPC's A.I. mode (set twice; the same value) |
| `#X`, `#Y`, `#Z` | the view's position (`MapX`, `MapY`, `MapZ`) |
| `$N` | a 34-byte buffer (the literal "Winona Ryder is a really hot babe"), which `TalkDriver` overwrites with the NPC's name |
| `$P` | the name in party slot 0 (the player) |
| `$T` | "morning" before 12:00, "afternoon" before 18:00, else "evening" |

Others: `#R` and `$Y` (`RESURRECT`, `SETNAME_Y`, `SETNAME_Y_YOU`), `$Z` (the last input,
`TalkDriver`), and `#Q`, which `GAME_Load` loads from the saved `IsOnQuest` byte and
`GAME_Save` stores back: it is the only variable kept in the saved game. The other letter
variables keep their values from one conversation to the next while the game runs and are
lost on quitting. In the dream the `#A`...`#S` values are still NPC 1's, not the dreamer's.
The shipped scripts use `#0`-`#9`, `#T`, `#G`, `#N`, `#H`, `#R`, `$P`, `$G`, `$Y` and `$T`;
none uses `#Q`.

#### Keywords and input

Input opcodes, read by `TalkDriver`'s loop; each but `WAIT` clears "already answered":

| Opcode | Input |
|---|---|
| 0xF7 ASKTOP | prints "You say:", reads a line of up to 50 characters |
| 0xF9 GETSTR `<i> B3` | reads a line into `$i` |
| 0xFB GETINT `<i> B2` | reads a line as a number into `#i`; invalid or escaped input gives 0 |
| 0xFC GETDIGIT `<i> B2` | waits for a digit key (Enter or Esc give 0) and stores it in `#i` |
| 0xFA GETCHR | reads one key |
| 0xF8 GET `<chars>` | waits for a key equal to one of the listed characters (upper-cased); the list ends at the next `KEY` |
| 0xCB WAIT | waits for a key |
| 0xEE ENDRES | no input; continues |

A keyword block is `KEY` (0xEF), a comma-separated list, `RES` (0xF6), the answer, `ENDRES`
(0xEE). `TALK_MatchKeywords` (talkdr.c) compares each keyword with the input:

- An empty input counts as "bye".
- The input is split into words at spaces. A keyword matches if its **whole length**
  matches the start of any word, ignoring case; `?` in a keyword matches any character
  (`str_i_compare`). So "job" matches "jobs" and "my job", but "jobs" does not match "job".
  The scripts use four-letter keywords ("drea", "towe", ...).
- A keyword `*` matches anything.
- Only the first matching block after an input runs; later blocks are skipped to their
  `ENDRES`. If nothing matches, nothing is printed.

The keyword buffer (`TalkKeyword`) has 12 bytes before the next variable (`TalkDescPC`, the
saved description position), so a keyword or `GET` list longer than 11 characters
overwrites it, as in SE.

### Joining and leaving the party

`JoinParty` (talk.c), from `OP_JOIN` (the scripts of NPCs 2 Spector, 4 Dibbs, 5 Sherman and
6 Chsheket use it):

- Returns 0 at once if the NPC is already in the party.
- Free room is 5 minus the party size, the player included: **the party holds at most five**.
  Unlike SE no NPC is exempt. No room: returns **2**.
- In a vehicle it returns **1** (the full check comes first).
- Otherwise the NPC is appended; its party name is its script name, cut to 13 characters
  (with `strncpy`, so a name of 14 or more characters gets no terminator from the copy);
  it becomes player-controlled; its wait flag (flag 15) is cleared; a neutral NPC becomes
  good; its A.I. mode becomes 1 (follow) and its combat mode 8. Returns **0**.

`LeaveParty` (talk.c), only from `OP_LEAVEPARTY` (Spector, Dibbs, Sherman):

- In a vehicle it returns **1**; if the NPC is not in the party, **2**.
- Otherwise it is removed (later members and names move up), stops being player-controlled,
  and a good NPC becomes neutral. Returns **0**.
- With its wait flag set, its mode becomes 0x92 and it keeps its inventory.
- Without it, its mode becomes 0x8f and it drops its **whole inventory** at its feet (on
  the current level). Unlike SE the dropped objects are not marked as free to take.

### Scripted events

`TALK_Event` (talk.c) runs event *n* of `OP_EVENT`. Some events leave a number in
`TalkEvent`; `TALK_talkTo` runs that part after the conversation window closes. An unknown
number prints "CallGuards(n)!". "Aargh!" is printed whenever an object the event needs is
not found or cannot be made. Positions are on the current level unless given.

| Event | Script (NPC) | Effect |
|---|---|---|
| 1 | 0x39 Edison | Every panel 0x1ca carried by a party member: sound 4, quality bit 1 ("broken") cleared, quantity set to 4 (the kind that fits a cabinet of quality 4, the dream machine's; see *The use command*). Flag 3 of NPC 0x39 is set if more than one panel was found, else cleared. |
| 2 | 0x3c Jean | `COMBAT_Kill(0x3c)`: Jean dies. |
| 3 | 0x2b Melies | nothing |
| 4 | none | Clears the wait flags of NPCs 0x1e and 0x18 (the two Cooters). |
| 5 | 0x18 Cooter, 0x21 Prektesh (young) | NPC 0x18: wait flag set, mode 0, moved to (0x18, 0x18, level 6), off the map. |
| 6 | 0x36 Nathaniel | The gate 0xb5 at (0x2c3, 0x1f3) takes frame 3 and moves to (0x2c2, 0x1f3, 0); the gate 0xb5 at (0x2c4, 0x1f3) takes frame 7 (a wall). |
| 7 | 0x3b Legrande | The door 0x98 at (0x2c7, 0x1dc) takes frame 7. |
| 8 | 0x3a Carnegie | If a rail car 0x19a with frame > 3 and quality 3 is in (0x9a, 0xbb)-(0xaa, 0xcb), flag 2 of NPC 0x3a is set. |
| 9 | 0x3a Carnegie | `TalkEvent` 9: after the conversation, `GameEnd` (see *Cut scenes and the endgame*). |
| 10 | 0x20 Tekapesh | `TalkEvent` 10: after the conversation, cut scene 10 (`Cutscene10`). |
| 11 (0x0b) | 0x20 Tekapesh | The door 0x98 at (0x13f, 0x194): quality 0, frame 7. |
| 0x33 | 0x5a Auctioneer | The hitching post 0x4f at (9, 0x1f) takes frame 1. |
| 0x34 | 0x5c Marshall | `DreamReleaseNpc(0x5c)`; `TalkEvent` 0x34: after the conversation, `DreamWakeUp`. |
| 0x35 | 0x52 Wells | Event jingle; `DreamFoesLeft` = 0; six tries: a creature of type 0x91 (LOOK: monster footprints) is made at the view's position and moved to a random free square up to 3 away (`COMBAT_TryTeleport`: each axis ±(1d4 + 1d4 − 5), 8 tries); placed ones get A.I. and combat mode 8 and count in `DreamFoesLeft`, the others are deleted. |
| 0x36 | 0x52 Wells | `DreamReleaseNpc(0x52)`; `TalkEvent` 0x36: after, `DreamWakeUp`. |
| 0x37 | 0x56 Carver | NPC 0x56 leaves the map (level 6); far local objects are purged; NPC 0 receives 4 wormsbane seeds 0x9e, a martian ritual pod knife 0xf2 and a bucket of water (0xa0, frame 1); a hole 0x101 and a covered martian seed 0xfb (quality 0xf) are made at (0x19, 0x6f), with a live Martian seed 0xfc in the hole; then three groups of 4 cave worms (0x16e, frame 1) start at (0xa, 0x6f), (0x19, 0x60) and (0x28, 0x6f), A.I. and combat mode 0x18, each moved up to 3 squares away but more than 5 from the view's centre on at least one axis, or deleted. Event jingle. |
| 0x38 | 0x55 Lenin | Counts rubles 0x84 (below); `TalkEvent` 0x38: after the conversation, if flag 3 of NPC 0x55 is set and flag 2 clear, `DreamReleaseNpc(0x55)` and `DreamWakeUp`. |
| 0x39 | 0x5a Auctioneer | The body 0x155 of NPC 0x5b (Clanton; quality 0x5b) in (8, 0x18)-(0x16, 0x39) comes back to life: not dead, full hit points, not poisoned, original shape, neutral; flag 4 of NPC 0x5b is cleared. |
| 0x65 | 0x6b Faulinei | Event jingle; `TalkEvent` 0x65: after, an obelisk 0x124 (frame 3, quality 0xe) at (0x34, 0x20) and a dream teleporter 0x1cd (frame 1, quality 0x34, quantity 0x0b, status 0xc1) at (0x34, 0x21); if NPC 0 stands on (0x34, 0x20) it moves to (0x34, 0x22) and the view follows. |
| 0x66 | 0x6a Astaroth | Event jingle; `TalkEvent` 0x66: after, an obelisk (quality 0xf) at (0x12, 0x22) and a teleporter (quality 0x1b, quantity 0x1a, status 0xc5) at (0x12, 0x23), NPC 0 moved off (0x12, 0x22) to (0x13, 0x23); NPC 0x68's flags 1-7 cleared; the bodies 0x155 at (0x14, 0xb), (0x16, 0xb), (0x18, 0xb) become figures of type 0x189, frames 0, 1, 2 (LOOK: Dibbs, Nellie Bly, Sherman). |
| 0x67 | 0x68 Tarpch | `TalkEvent` 0x67: after, Tarpch strikes one figure (below). |
| 0x68 | 0x6c Nosfentor | Event jingle; `TalkEvent` 0x68: after, an obelisk (quality 0x10) at (0x6a, 0x2a) and a teleporter (quality 0x53, quantity 0x28, status 0xe0) at (0x6a, 0x2b), NPC 0 moved off (0x6a, 0x2a) to (0x6b, 0x2b); flag 0 of NPC 0x69 is set. |
| 0x69 | 0x67 Shartek | Sound 0x32; if NPC 0x67 is at x = 0x2c, the door 0xdb at (0x3b, 0xc) takes frame 3, else the door 0xdb at (0x2b, 0xf) takes frame 7; NPC 0x67 becomes a martian 0x171 (same frame). `TalkEvent` 0x69: after, `ShowObjects`. |
| 0x6a | 0x6d Spector (dream figure) | The party is teleported to (`WakeX`, `WakeY`). |
| 0x6b | 0x68 Tarpch | Sound 4; far locals purged; party to (0x6a, 0x2c); NPC 0's inventory deleted; `WakeX`, `WakeY` = (0x6a, 0x2c); `DreamStage` = 0xe4. |
| 0x6c | 0x66 Raxachk | Combat begins if not on; `DreamFoesLeft` = 6; far locals purged; party to (0x6e, 0x17) with forced hatching; a bridge 0xe0 (frame 3) is made at (0x75, 0x17) unless one is there. |
| 0x6d | 0x66 Raxachk | The same with (0x7a, 0xd) and a bridge (frame 7) at (0x7a, 0x12). |
| 0x6e | 0x66 Raxachk | Combat begins if not on; far locals purged; party to (0x82, 0x18) with forced hatching (`DreamFoesLeft` unchanged). |
| 0x6f | 0x6c Nosfentor | NPC 0's hit points = NPC 1's `MaxHP`. |
| 0xfe | none | `DrunkCounter` + 1 (see *Time, rest, stats and levels*). |
| 0xff | none | End of the game: temporary map files deleted, video reset, memory freed, exit to DOS. Nothing is saved. |

For the events about the dream (0x34-0x38, 0x65-0x6f) see *The dream* for what the places
and objects are; `DreamReleaseNpc` and `DreamWakeUp` live in dream.c.

**Event 0x38** (the rubles): flags 3 and 4 of NPC 0x55 are set and flag 2 cleared. Every
ruble object 0x84 in (0x49, 0x30)-(0x57, 0x51) counts and clears flag 4; flag 3 is cleared
if two of them have different quantities. A heart stone 0x1aa or azurite stone 0x1a9 there
sets flag 2. In NPC 0's inventory, rubles clear flags 3 and 4 and set flag 5, and the stones
set flag 2. Finally flag 3 is cleared unless there were exactly 25 ruble objects (or none)
and their common quantity is 10 (or there were none). So flag 3 means "25 piles of 10
rubles laid out and none carried", flag 4 "no rubles there or carried", flag 5 "carrying
rubles", flag 2 "a stone there or carried". The script reads them.

**Event 0x67** (Tarpch's blows), by NPC 0x68's flags: if flag 2 is clear it is set and the
target is (0x14, 0xb), victim NPC 4; else if flag 3 is clear, it is set and the target is
(0x16, 0xb), NPC 3; else if flag 4 is clear, it is set and the target is (0x18, 0xb), NPC 5;
else the target is NPC 0's square and the victim NPC 1. It prints Tarpch's description with
"heavily wounded." (while flag 3 is clear) or "critical!", shoots a missile (tile 0x16a)
from NPC 0x68, and prints "<victim> killed!". For NPCs 3-5 the figure 0x189 on the target
square becomes a body 0x155 of the victim's kind and the support beam 0xae there is moved
in place. For NPC 1 the dreamer and Tarpch show as unconscious for about half a second, get
their shapes back, and the party goes to (`WakeX`, `WakeY`).

Bug: in the second part of 0x65, 0x66 and 0x68, a failed object creation prints "Aargh!"
and returns before `TalkEvent` is reset and solo mode restored, so the event's second part
runs again after the next conversation.

### How quest state is kept

#### Talk flags

Every actor (0-255) has 16 flags. MD keeps SE's second byte:

| Flags | Storage | Notes |
|---|---|---|
| 0-7 | `TalkFlags[actor]`, bit *n* | Flag 0 = "met": the name is shown instead of the description. |
| 8-15 | `TalkFlagsHi[actor]`, bit *n* − 8 (a far 256-byte array allocated in `main`) | Only bit 7 (**flag 15**) is used by the engine: the **wait flag**. |

No shipped script tests or sets flags 8-15 (`dialogue.json`). The shipped saved game has flag
8 set on 21 NPCs (value 0x81), and `PanelDreamMachine` (useact2.c) ORs 0xcb (flags 8, 9, 11,
14 and 15) into NPCs 0x61 and 0x63; nothing reads any of them but flag 15.

The wait flag: an NPC that has it and is not player-controlled keeps its current A.I. mode
and counts as being at its schedule spot when a schedule entry starts
(`StartScheduleAction`, npctrack.c), and the hourly schedule update skips it
(`UpdateSchedules`). Unlike SE its mode is not changed to 0x92 there. It also decides what
`LeaveParty` does. Writers: `JoinParty` clears it; events 4 and 5; cut scenes 8, 9 and 10
set it (and cut scene 10 clears it for NPCs 0x2a-0x31; see *Cut scenes and the endgame*);
the dream stages and `DreamReleaseNpc` set it for the dream figures they place (see *The
dream*); `EGG_generate` (egg.c) clears the whole `TalkFlagsHi` byte for a new creature. No
code clears `TalkFlags` for a new creature, so a temporary creature slot (0xC0-0xFF) keeps
flags 0-7 of the creature that used it before.

Scripts set the flags with `SET`/`CLR` and read them with `TST`. Unlike SE, NPC 1's flags are
not the story flags: no script sets any of them, and the engine reads only flag 6 (debug).
The story lives in the flags of the NPCs concerned, often the NPC a script belongs to
(`set(npc, ...)`).

#### The saved game

Everything is saved in `savegame\objlist` (`GAME_Save` and `GAME_Load`, console.c), in the
same layout as SE's:

| Offset | Size | Contents |
|---|---|---|
| 0x0000 | 0x100 | object status, actors |
| 0x0100 | 0x300 | positions, actors |
| 0x0400 | 0x200 | shape/type, actors |
| 0x0600 | 0x200 | quality/quantity, actors |
| 0x0800 | 0x100 | NPC status |
| 0x0900 / 0x0A00 / 0x0B00 | 0x100 each | strength / dexterity / intelligence |
| 0x0C00 | 0x200 | experience (16-bit) |
| 0x0E00 | 0x100 | hit points |
| 0x0F00 | 0xE0 | party names (16 × 14) |
| 0x0FE0 | 0x10 | party list |
| 0x0FF0 | 1 | party size |
| 0x0FF1 | 0x100 | levels |
| 0x10F1 | 0x100 | `SchedIndex` (current schedule entry) |
| 0x11F1 | 0x100 | A.I. modes |
| **0x12F1** | 0x100 | **`TalkFlagsHi`: talk flags 8-15** |
| 0x13F1 | 0x100 | combat modes |
| 0x14F1, 0x15F1 | 0x100 each | movement points (written twice) |
| 0x16F1 | 0x200 | original shapes |
| **0x18F1** | 0x100 | **`TalkFlags`: talk flags 0-7** |
| 0x19F1 | 0x100 | `Leader` |
| 0x1AF1 | 0x100 | `NPCFlag` |
| 0x1BF1 | 0x100 | `PathData` |
| 0x1CF1 | 0x64 | the game-state block |

The objects outside the actors are in the `objblk` files, written by `OBJ_CommitBlocks`.

The **game-state block** (gamestat.c) is everything from `IsOnQuest` to `GameStateEnd`,
0x64 bytes, read and written as one piece. (The shipped `OBJLIST` has 0x82 bytes after
0x1CF1, as MARTIAN.EXE leaves it; GAME.EXE reads only the first 0x64.)

| Offset in block | Variable | Use |
|---|---|---|
| +0x00 | `IsOnQuest` | the saved copy of `#Q`; nothing else reads it |
| +0x01 | `NextSleep` | (see *Time, rest, stats and levels*) |
| +0x02 - +0x07 | `Time_M`, `Time_H`, `Date_D`, `Date_M`, `Date_Y` (16-bit) | the clock; scripts see `#H`, `#D`, `#M` |
| +0x08 | `Active` | the active party member |
| +0x09 | `AmbientLight` | |
| +0x0A - +0x0F | `MapX`, `MapY`, `MapZ` | `GAME_Load` recomputes them from the active member |
| +0x12, +0x13 | `WakeX`, `WakeY` | the dreamer's return square (*The dream*) |
| +0x14 | `DrunkCounter` | event 0xfe |
| +0x15, +0x17 | `SoundFlag`, `AvatarNotSolid` (16-bit) | |
| +0x19 - +0x30 | `PathObject`, `PathCounter`, `PathTries` | 8 bytes each, path finding |
| +0x31 | `DreamFoesLeft` | |
| +0x32 - +0x35 | `InCombat`, `SoloMember`, `SavedNPCMode`, `DefaultCommand` | `GAME_Load` clears `InCombat` |
| +0x36 | `PlayerIsFemale` | `#G`, `$G`, the portraits |
| +0x37 | `AvatarPortrait` | 0 = no character yet (*Character creation*) |
| +0x38 | `GameFlags` (32-bit) | 0x10 = in the dream |
| +0x3C | `ViewRotation` | |
| +0x3E | `Talents[16]` | the talents of NPCs 0-15, 2 bytes each (four 4-bit values) |
| +0x5E - +0x61 | `WakeDay`, `WakeMonth`, `WakeYear` | |
| +0x62 | `DreamStage` | |

Saving (`GameLoop`'s Ctrl+S, gameloop.c) is refused while `IsArmageddon` is set (below) and
in combat ("Not while in combat mode!"). After "Save game? Y" it runs `GAME_Save` and then
**`GAME_Restore`**: every save reloads the game it wrote, which breaks off combat, clears
`IsArmageddon`, rebuilds the map and the animations, restarts the NPC tracker and runs
`PassTime(0)`. `GAME_Save` clears flag 5 of NPC 0x76 and flag 4 of NPC 0x73 in memory after
writing them (it has just undone the canal animation that flag 5 shifts); the reload brings
them back from the file. `GAME_Load` copies `IsOnQuest` into `#Q`, gives NPC 6 the original
shape 0x162 (lady) if flag 1 of NPC 6 is set, else 0x165 (metal woman), and sets the
machinery's animations and objects by the power and canal flags (see *The map and
machinery*).

Bug: the "Disk full" prompt (`DISK_full`, disk.c) stores 'S' or 'R', but the end-of-turn
code tests for 'R' and 'G', so the answer "Save the game and continue" saves nothing.

#### Flags the engine itself reads or writes

Every other piece of story state is up to the scripts. These are the flags the engine
touches (all of SRC/); "scripts" names the NPC whose script sets or reads the flag where
`dialogue.json` shows it. Masks are `TalkFlags[npc]` bits.

| NPC, flag (mask) | Written by | Read by |
|---|---|---|
| any, 0 (1): met | scripts (`set(npc, 0)`) | `TALK_talkTo`, `ShowPortrait` (name or description) |
| 1, 6 (0x40) | nothing (debug key 242 or a saved-game edit) | `GameLoop`: enables the debug keys (*Debug keys*) |
| 2 Spector, 2 (4) | `UseReplaceCable` (useact1.c) after a shock, then Spector talks | Spector's script (which clears it) |
| 2 Spector, 4 (0x10) | `UseReplaceCable`: set while a member other than NPC 1 tries the cable and that member talks, then cleared | the scripts of NPCs 3, 4 and 5 |
| 2 Spector, 6 (0x40) | `CmdLook` (objcmd.c) on a broken strap 0x1cc, then Spector talks | Spector's script (clears it) |
| 5 Sherman, 2 (4) | `PanelDreamMachine` (robot): set while Sherman talks, if the party has 5 members | Sherman's script |
| 5 Sherman, 3 (8) | Sherman's script | `UseRead`: the affidavit shows his signature |
| 5 Sherman, 4 (0x10) | `PanelRubyLenses` (useact2.c): set while Sherman talks after cut scene 6 | Sherman's script |
| 6 Chsheket, 1 (2) | `PanelRubberSprayer` (with flag 5, cleared after NPC 6 talks); `LeverMachinery` (lever.c, lever 6) | `GAME_Load` (original shape), `DrawPortrait` (face 0x16) |
| 6 Chsheket, 2 (4) | `PanelRubberSprayer` when the sprayer holds rubber but not flesh colour; NPC 6 talks | NPC 6's script (clears it) |
| 0x10 Tesla, 2 (4) | `UseOpenHatch` (useact1.c) while the capsule door is shut; Tesla talks | Tesla's script (clears it) |
| 0x10 Tesla, 4 (0x10): capsule door open | Tesla's script | `UseOpenHatch` (the hatch opens only then); `PartyUnconscious` (no waking before it) |
| 0x10 Tesla, 5 (0x20): hatch opened | `UseOpenHatch` | `PassTime`: from now on the party needs oxium to breathe (*Time, rest, stats and levels*) |
| 0x12 Blood, 3 and 6 (0x48) | `UseOpenHatch` after cut scene 1; Blood talks | Blood's script (clears 3) |
| 0x12 Blood, 4 (0x10) | `UseOxiumBin` (useact2.c) | Blood's script |
| 0x12 Blood, 5 (0x20) | `ResurrectParty` (map.c); Blood talks | Blood's script (clears it) |
| 0x19 Duprey, 3 (8); 0x1a Yellin, 3 (8) | their scripts | `UseRead`: the affidavit's signatures |
| 0x20 Tekapesh, 4 (0x10) | `DreamReleaseNpc` (dream.c) once all eight dream figures 0x50-0x57 have flag 7 | scripts |
| 0x21 Prektesh (young), 2 (4) | `PanelDreamMachine` (after cut scene 5); its script | `PanelDreamMachine` (once); `DrawPortrait` (face 0x61 until set) |
| 0x2e Tiffany, 0x39 Edison, 7 (0x80): lens installed | `UseInstallLenses` (useact3.c); `PanelRubyLenses`, `LeverMachinery` | `UseInstallLenses`, `PanelRubyLenses` |
| 0x30 Carver, 7 (0x80) | `UseSprayGun` killing a vine 0xcd; `PanelRubyLenses`, `LeverMachinery` | `PanelRubyLenses` (all three needed) |
| 0x39 Edison, 3 (8) | event 1 | scripts |
| 0x3a Carnegie, 2 (4) | event 8 | scripts (NPCs 3 Nellie Bly, 0x33 Segal) |
| 0x3b Legrande, 1 (2) | `COMBAT_Kill` (combat.c) when NPC 0x3b or 0x36 dies | scripts |
| 0x3e Bernhardt, 4 (0x10) | nothing found (no script, no engine code) | `PanelRubberSprayer`: "rubber but not flesh" refusal |
| 0x3e Bernhardt, 5 (0x20) | Bernhardt's script | `CmdLook` on the sprayer 0x114 (the same message) |
| 0x40 Rasputin, 3 (8) | `COMBAT_RasputinThrow` (combat.c); Rasputin talks | Rasputin's script |
| 0x41 Maurice, 2 (4) | `UseCannon` (useact2.c) when the cannon blows the door open | scripts |
| 0x46 Rasputin (second script), 0 (1) | its script (met) | `PanelDreamMachine`: setting 3 enters the dream (`DreamEnter`) only with it |
| 0x46, 3 (8): Raxachk shot | `COMBAT_FireM60` when its shot hits NPC 0x66 (after cut scene 9) | `PanelDreamMachine` ("ceased"); `EGG_hatches` (eggs of quality ≥ 100 hatch); `PassTime` (1 in 5 per call: the screen shakes); `ResurrectParty`, `PartyUnconscious` (Olympus mine); many scripts (`tst(70, 3)`) |
| 0x51, 0x57, 0x50, 6 (0x40) | `UseOil` (on tile 8), `UseManuscript`, `UseMirror` (useact3.c); the NPC talks, then release and wake | their scripts |
| 0x50-0x57, 7 (0x80): released | `DreamReleaseNpc` | `DreamReleaseNpc` (flags of 0x20 and 0x60), `DreamEndStage44` (0x54) |
| 0x50, 0x51, 0x52, 0x54, 0x55, 0x57; 0x59-0x5d | the dream stages (dreamstg.c) clear some or all; `DreamTrigger` 0xa4 clears flags 1-7 of 0x56 | scripts |
| 0x52 Wells, 1 (2) | `COMBAT_Kill`: last foe of stage 0xa1 | scripts |
| 0x54 Tiffany, 1 (2) | `COMBAT_AI_ChaseCape` (combat.c), `LeverObjects` (lever.c): the creature reaches the cape; Tiffany talks | its script (clears it) |
| 0x54 Tiffany, 3, 4 (8, 0x10) | `DreamEndStage44` | scripts |
| 0x54 Tiffany, 6 (0x40) | `LoseHP`: the last glass pitcher 0xd9 of stage 0x44 breaks | `COMBAT_AI_ChaseCape`, `LeverObjects` |
| 0x55 Lenin, 2-5 | event 0x38 | event 0x38's second part, Lenin's script |
| 0x56 Carver (dream), 3 and 7 (0x88) | `UseOpenPod` in the dream; Carver talks | its script |
| 0x5b Clanton, 4 (0x10) | `COMBAT_Kill` when NPC 0x5b dies; event 0x39 clears it | scripts |
| 0x60 Marcus, 2 (4) | `UseFastenPanel`, `UseRadium`, `UseInstallHeadgear`, but only if NPC 0x74's flag 0 is set | NPC 0x74's script ("nothing stands in your way") |
| 0x60 Marcus, 3 (8) | `PanelDreamMachine` with a setting below 2 | scripts |
| 0x60 Marcus, 4 (0x10) | `DreamReleaseNpc` once 0x52, 0x54, 0x55, 0x56 have flag 7 | scripts |
| 0x61 Prektesh, 3 (8) | `UseOpenPod` outside the dream (a Martian falls out) | scripts |
| 0x61 Prektesh, 4 (0x10) | Prektesh's script | `PanelDreamMachine` (the machine 0x123 works only then) |
| 0x66 Raxachk, 1 and 5 (0x22) | `COMBAT_Kill` (the level-3 stage); Raxachk talks | its script |
| 0x66, 1 and 3 (0xa) or 1 and 4 (0x12) | `DreamLastFoeKilled` (dreamobj.c); Raxachk talks | `DreamTrigger` (flag 3: no new foes), `GoThroughEntrance` (flag 4: no forced hatching), `PartyUnconscious` (flags 3, 4 set and 2, 5 clear) |
| 0x66, 7 (0x80) | `PartyUnconscious` in stage 0xc0; Raxachk talks | its script (clears it) |
| 0x67 Shartek, 2 (4) | its script | `ShowPortrait` |
| 0x68 Tarpch, 2-4 | event 0x67 | event 0x67; `LoseHP` (flag 2: "critical!") |
| 0x68 Tarpch, 5 (0x20) | `LoseHP` when Tarpch is hit; he talks | his script (clears it) |
| 0x68 Tarpch, 6 (0x40) | `UseHealingElixir` on him; he talks | his script |
| 0x68 Tarpch, all | `DreamTrigger` 0xc5 clears; event 0x66 clears 1-7 | |
| 0x69, 0 (1) | event 0x68 | scripts |
| 0x6a, 0x6b, 0x6c, 2 (4) | `DreamTrigger` 0xc5, 0xc1, 0xe0 | their scripts |
| 0x6b Faulinei, 3 (8) | `LoseHP` when NPC 0x6d or 0x67 is attacked | its script |
| 0x71 Stoker, 3 (8): power not yet permanent | `BeltStart` (belt.c), `LeverMachinery`; cleared by `BeltUpdate` (1 in 7 per update), `LeverMachinery`, `NPC_TakeTurn` | the same, `UseReplaceCable` |
| 0x72 Conveyor, 2 (4): rollers fixed | `UseFixRollers` (useact2.c) | `NPC_TakeTurn` (the coal worker's lever: permanent power and cut scene 2, else `BeltStart`); NPC 0x72's script |
| 0x73 Tower, 2 (4): underground power | `BeltStart`, `LeverMachinery`, `NPC_TakeTurn`; cleared by `BeltUpdate`, `LeverMachinery` | belts, jetters, levers, lamps, machine sounds, doors (belt.c, jet.c, lever.c, object.c, `UseAutoDoor`, `UseReplaceCable`, `GAME_Load`); scripts |
| 0x73 Tower, 4 (0x10): surface power | `UseReplaceCable` (then cut scene 3), `LeverMachinery`; cleared by `GAME_Save` | `Drawbridge`, `GoThroughEntrance` (chamber 0xc5), `PassTime` (lamp posts 0xe4), object.c, `LeverObjects`, `UseAutoDoor`; scripts of 0x60 and 0x73 |
| 0x74 Machine, 0 (1) | nothing found | the three writers of NPC 0x60's flag 2 |
| 0x74 Machine, 2, 3, 4 (4, 8, 0x10) | `UseRadium` (power unit 0x122), `UseFastenPanel` (a panel of kind 4), `UseInstallHeadgear` | NPC 0x74's script ("The Dream Machine is ready!") |
| 0x75 Cutter, 3 (8) | `UseRadiumOnMechMan` (useact1.c) | scripts |
| 0x76 Diver, 5 (0x20): water pressure | `PanelRubyLenses`, `LeverMachinery`; cleared by `GAME_Save` and `GAME_Load` | barges, canals, splashes, buckets, valves (barge.c, canmove.c, objcmd.c, object.c, useact2.c, useact3.c, console.c) |
| 0x76 Diver, 6 (0x40): canals running | cabinet 7 (`UseCabinetPanel`), `UseValve`, `LeverMachinery` | the same |
| 0x76 Diver, 7 (0x80) | `BargePumps` (barge.c) once both are set | NPC 0x60's script |

For the machinery flags see *The map and machinery*; for the dream ones *The dream*.

Two of these look like mistakes in the original, as far as the scripts in `dialogue.json`
show: flag 0 of NPC 0x74 is never set, so NPC 0x60's flag 2 never is either and NPC 0x74's
"door" answer always says "The way is blocked."; and flag 4 of NPC 0x3e is never set, so
`PanelRubberSprayer` never refuses with the message that `CmdLook` gives on flag 5
(uncertain whether 0x20 was meant).

Bug (kept from the original, listed in FINDINGS): after a shock `UseReplaceCable` ends with
`TalkFlags[2] &= 4` where `&= ~4` was meant, so all of Spector's flags 0-7 but flag 2 are
cleared (his "met" flag among them) and flag 2 stays set.

### The party's death and waking

`CheckActiveMember` (npctrack.c) calls `PartyUnconscious` (sound.c) when NPC 1 is dead, or
NPC 0 in the dream. `PartyUnconscious` returns at once if `Active` is 0x10 (why is not
clear, as in SE). In the dream it brings the dreamer back to (`WakeX`, `WakeY`) or wakes it
(see *The dream*; in stage 0xc0 Raxachk talks with flag 7). Otherwise:

1. The music fades; it prints "Overcome by your wounds, you fall unconscious....." and,
   with music, plays song 11 and queues the area song.
2. If flag 4 of NPC 0x10 is clear (Tesla's script sets it; it is also what lets the hatch be
   pried open, and the message calls it the capsule door being open), **the game ends**: sound, memory and
   graphics are shut down, "Your character cannot be resurrected before the capsule door is
   open. Please try again!" is written on the text screen, and the program exits (code 4).
   Nothing is saved.
3. Otherwise it prints "....You awaken back at the Olympus mine, feeling restored." if flag 3
   of NPC 0x46 is set, else "...at the space capsule...", and calls `ResurrectParty`.

`ResurrectParty` (map.c):

- Every player-controlled actor 0-0xBF that is dead is revived from its body (type 0x155
  with its number as quality): NPC 1 directly (`OBJ_ReviveFromBody`, full hit points,
  original shape, status bits 0-4 cleared), the others with `SPELL_ReviveNPC`.
- Every actor 0-0xBF in mode 0xa0 goes back to its schedule (mode 0x81).
- Every living party member wakes (getting its original shape back if asleep), becomes
  good, loses invisibility, charm and the 0x40 status (cursed, or gasping for air), loses
  paralysis, sleep, poison and the dead bit, gets full hit points, **loses a quarter of its
  experience** (experience × 3 / 4, integer) and gets movement points equal to `GetDex`.
- Combat ends; party mode or the solo member is restored (dead members get mode 0).
- The party is teleported to **(0xa4, 0xc3, level 4)**, the Olympus mine, if flag 3 of NPC
  0x46 is set, else to **(0x19d, 0x278, level 0)**, the space capsule.
- **A week passes**: the date moves on 7 days (months of 28 days, 12 months), the clock is
  set to 7 o'clock and a random minute 0-59, and `PassTime(0)` runs once (no hourly
  updates for that week).
- Flag 5 of NPC 0x12 is set and **NPC 0x12 (Blood) talks**.

### IsArmageddon

`IsArmageddon` (gameloop.c) is not saved. `COMBAT_Kill` (combat.c) sets it to 1 when NPC
0x3b, 0x36, 0x12, 0x1a or 0x20 dies, and to 2 when one of a list of historical figures dies
(see *Combat* for the lists and the other effects). Its only effect is on saving: Ctrl+S
prints "You have killed someone whose help you need to win the game!" (1) or "One of the
important historical figures you were here to rescue is dead!" (2) and does not save.
`GAME_Restore` clears it, so the way on is to restore. Unlike SE, `TalkDriver` does not test
it: conversations go on.

### What could not be determined

- Which objects carry `ObjStatus & 1`, the bit that psychic impressions need: in the shipped
  surface object blocks (`SAVEGAME\OBJBLK*`) only 15 conveyor belts 0xbf, 13 bare rollers
  0xc0, one dream machine 0x120 and 80 dream teleporters 0x1cd have it; none of the saws,
  injectors, sculptures and other types with impression texts does. The dungeon blocks
  (`LZDNGBLK`) were not decoded, and no code sets the bit except events 0x65 and 0x66 (on
  teleporters). So it is not known whether any of the impression texts can be seen in play.
- With the second talent, talking to an empty square tests `ObjStatus[-1]`, the byte before
  the array (part of the `TalkFlagsHi` pointer); the result depends on memory layout.
- The meaning of A.I. modes 0x90 and 0x92 and of many flags is only what the scripts do with
  them; this section lists the flags the engine touches, not the scripts' own.
- The `Active == 0x10` early return in `PartyUnconscious`.

## Time, rest, stats and levels

This section covers the game clock and calendar, what happens as time passes, the light
and the sky, NPC schedules, the Martian air and oxium, the cold, status effects and
drunkenness, resting and healing, experience and the obelisk level-up dream, the
character attributes and talents, carrying capacity, and movement points. Every rule
cites the function that implements it. `OSI_rand(lo, hi)` returns a number from lo to
hi, both included, so "1 in 26" below means `OSI_rand(0, 25) == 0`.

### The clock and the calendar

The time is kept in five saved variables of the game-state block (`gamestat.c`): `Time_M`
(minute, 0-59), `Time_H` (hour, 0-23), `Date_D` (day, 1-28), `Date_M` (month, 1-12) and
`Date_Y` (the year itself, a 16-bit number). The values built into GAME.EXE are **8:00 on
day 6, month 1, 1895**. A saved game overrides them.

The calendar has 60 minutes per hour, 24 hours per day, **28 days per month** and **12
months per year**, so a year has 336 days (`PassTime`, gameloop.c). For NPC schedules the
day of the week is `(Date_D - 1) % 7 + 1` (1-7), so every month is exactly four weeks
(`UpdateSchedules`, npctrack.c). Nothing in GAME.EXE prints the date; scripts can read the
day, month and hour as the variables D, M and H (`TALK_talkTo`, talk.c).

The time of day can be read by *Look* at a pocketwatch (0x5b or 0x62): "The time is
h:mm A.M./P.M.", at any hour (`CmdLook`, objcmd.c). The animated pocketwatch tile (the
tile of 0x62, 0x268) also shows the hour: `DrawSkyBar` (gfx.c) sets its first animation
frame to tile `0x1a0 + Time_H % 12`.

#### What makes time pass

Time passes only when something calls `PassTime(minutes)` (gameloop.c). `PassTime(0)`
advances nothing: it refreshes the light, the sky and the per-call effects listed below.

| Event | Minutes | Where |
|---|---|---|
| A scheduler round ends: no actor in the area has movement points left; every actor gets new points (see *Movement points and turns*) | 1 | `RunNPCTurns` (npctrack.c) |
| Combat is broken off | 1 | `COMBAT_breakOff` (combat.c) |
| Opening the hatch 0x1ab with a chisel or pry bar (cut scene 1) | 1 | `UseOpenHatch` (useact1.c) |
| Resting, for each hour asked | 3 × 20 | `REST_Command` (rest.c) |
| A party member other than the player sleeps in the dream machine: "dreams until dawn while the party waits" | to the next full hour, then 20 at a time until 8:00 of the wake day | `DreamSleep` (dream.c) |
| Aligning the ruby lenses at night (18:00-5:59): "You wait until dawn..." | `(28 - Time_H) × 60 + (60 - Time_M)` | `PanelRubyLenses` (useact2.c) |
| A debug key (see *Debug keys*) | 60 | `GameLoop` (gameloop.c) |

**Unlike SE, a step of the party leader passes no time.** `CheckGatesAndEntrances`
(npctrack.c), run after every leader step, calls `PassTime(0)`. Walking therefore costs time
only through the scheduler rounds: about one minute each time every actor has spent its
movement points.

The ruby-lens wait ends at 5:00 when it starts between 18:00 and 23:59. Started between
0:00 and 5:59 it ends at 5:00 a day later (from 2:00 it waits 27 hours).

Some events set the clock directly, **without** `PassTime` (no status effects, schedules or
hourly updates run for the skipped time):

- the party is overcome and wakes up: `Date_D += 7` (rolling over into the next month and
  year), `Time_H = 7`, `Time_M = OSI_rand(0, 59)`, then `PassTime(0)` (`ResurrectParty`,
  map.c; see *Conversations and quest state*);
- waking from the dream: 8:00 on the saved wake date (`WakeDay`, `WakeMonth`, `WakeYear`;
  see *The dream*).

The other `PassTime(0)` callers (loading a game, teleports and entrances, lighting, putting
out or refuelling a light, burning something, picking up or dropping a light) only refresh
the state.

#### What happens in each `PassTime` call

The steps below run in this order (`PassTime`, gameloop.c).

1. **Screen shaking.** Once `TalkFlags[0x46] & 8` is set (Raxachk, NPC 0x66, was killed
   in the dream, `COMBAT_FireM60` combat.c), each call has a 1 in 5 chance
   (`OSI_rand(0, 4) == 0`) to shake the screen for `OSI_rand(4, 20)` units at amplitude 1
   (`ShakeScreen`, gfx.c). This happens even with `minutes` = 0.
2. **Objects of the area** (only if `minutes > 0`). Every object 0x101-0xbff inside the
   loaded 40×40 area is checked once per call, whatever the number of minutes:
   - barge controllers 0x1a2 are collected and the barges moved (`BargeAddController`,
     `BargeMoveAll`; see *The map and machinery*);
   - 0x1d0 (a jetter; LOOK has no name) runs `UpdateJet` (see *The map and machinery*);
   - a covered martian seed 0xfb grows (`GrowPlant`; see *The use command*);
   - a wormsbane plant 0x9d shrinks (`ShrinkWormsbane`): its quality goes down by 1; at 0
     its frame goes down by 1 and its quality restarts at `OSI_rand(10, 15)`; at frame 0
     and quality 0 it is deleted.

   Because these run once per call, a 20-minute rest step counts as one tick, as does a
   one-minute round.
3. **For each actor 0..0xff that exists:**
   - the talents of NPCs 0-15 are cut to at most 10 (see *Talents*);
   - for a party member that is alive, out of the dream (`GameFlags & 0x10` clear), and
     not NPC 0: the air check, the light sources and the warmth, and the cold (see *Air and
     oxium*, *Light sources* and *Cold and warmth*). In the dream this whole block is
     skipped;
   - the status effects, rolled **once per minute** (see *Status effects*), and for NPCs
     0-7 the talent decay, melting ice and the radium check;
   - harmful terrain, once per call (`ApplyTerrainEffects`; see *The map and machinery*).
4. **Calendar.** For each hour boundary crossed, in order: the hour goes up (rolling over
   day, month and year); `NextSleep` goes down by 1 if non-zero; the party breathes one
   hour of oxium (see *Air and oxium*); the lamp posts are set for day or night;
   `DrunkCounter` goes down by 1 if non-zero; `UpdateSchedules` runs. A call that crosses
   several hours does all of this once per hour.
5. **Sky.** `Time_M` is stored. If `minutes` is non-zero, or on the first call after
   start-up, the sky bar is redrawn (`DrawSkyBar`).
6. **Hourly refresh.** When the hour differs from `LastHourUpdate`: the equipment panel is
   redrawn if it is shown, and the area music is refreshed (`MUS_PlayAreaSong`).
7. **Ambient light**, out of the dream only (see below). If the level changed, the view
   is recomputed (`VIS_UpdateView`).

There is no hunger and no thirst, and **no hit points come back with time**. `PassTime` has
no food or regeneration logic.

#### Ambient light (`PassTime`)

`t` is the time of day in minutes (`Time_H × 60 + Time_M`).

| Where / when | `AmbientLight` |
|---|---|
| Below the surface (`MapZ > 0`) | 0 |
| Surface, `t < 400` (before 6:40) or `t > 1080` (after 18:00) | 0 |
| Surface, 400 ≤ t < 440 (dawn) | `(t - 400) / 10 + 2`: 2, 3, 4, 5 |
| Surface, 1040 < t ≤ 1080 (dusk) | `(1080 - t) / 10 + 2`: 5 down to 2 |
| Surface, 440 ≤ t ≤ 1040 | 7 |
| Then: if the value is below 4 and a party member has a lit light source readied | **set to** that light's level |

The light level of a light source is `TileFlag & 3` of its tile (the brightest one readied
counts). In TILEFLAG it is **1** for the lit torch 0x6e, the lit candle 0x70 and the lit
candelabra 0x72, and **2** for the lit oil lamp 0x74 and the lit lantern 0x76. The last
rule *replaces* the value rather than raising it: at the start of dawn or the end of dusk
(ambient 2 or 3) a lit torch lowers the light to 1. In the dream the light is not
recomputed: entering it sets `AmbientLight` to 6 (`DreamSleep`, `DreamEnter`, dream.c).

The party lights the squares it can see: a square at distance *d* gets
`4 - d + AmbientLight`, clamped to 0..4, and a square at 0 is dark (`VIS_FloodFill`,
visible.c). So at night the party sees 3 squares around it, 4 with a torch or candle, 5 with
an oil lamp or lantern. While `AmbientLight < 7`, every light-giving tile in view adds its
own light; unlike SE a square keeps the brightest light that reaches it, not the sum
(`VIS_UpdateView`). The third talent shows the whole view lit (see *Talents*).

#### The sky bar (`DrawSkyBar`, gfx.c)

The strip above the map shows:

- on levels 2 and 3: a gray strip; on the other underground levels: a black strip;
- on the surface: a strip of a sky picture (MDSCREEN.LZC entry 2, frame 2 from 0:00 to
  11:59, frame 3 from 12:00 to 23:59) drawn at x = `8 - (16 × (Time_H % 12) + 4 ×
  (Time_M / 15))`. It scrolls left 4 pixels every quarter hour, through the whole
  picture in each half day.

Then the pocketwatch animation is set to the hour (above). The bar is redrawn at every
leader step (`CheckGatesAndEntrances`) and from `PassTime` as described.

#### DisplayTick (gameloop.c)

`DisplayTick` is the per-frame display update, not a time step: it recomputes the view if
needed, animates the tiles (`AnimateTiles`) and redraws the map window. Unlike SE it has no
gameplay effect; the screen shaking moved into `PassTime`.

### Light sources

A light burns only while it is a lit light type (`IsLitLightType`, objtype.c: 0x6e, 0x70,
0x72, 0x74, 0x76) **readied** (`ObjStatus & 0x18 == 0x18`) by a party member. On each
`PassTime` call each such light has a 1 in 2 chance (`OSI_rand(0, 1)`) to lose `minutes`
from its quality (its fuel). If its quality is `≤ minutes` when the roll succeeds:

- a lit oil lamp 0x74 or lit lantern 0x76 becomes the unlit 0x73 or 0x75 with quality 0:
  "The ... is out of fuel.";
- any other light is deleted: "... went out."

On average a light loses half a point per minute however the time passes, so it lasts
about twice its fuel in minutes. Lighting sets the fuel when it is 0 (`UseLight`,
useact3.c): oil lamp 255, lantern 180 (each takes a can of lamp oil 0x7c), candle and
candelabra 100, torch 180; refuelling sets 255 or 180 (`UseRefuelLantern`). So an oil
lamp lasts about 8½ hours, a torch or lantern about 6, a candle about 3⅓. The lights of a
member burn even while it sleeps or rests. Lighting and refuelling are covered in *The
use command*.

**Lamp posts.** Every hour, each lamp post 0xe4 in memory gets frame 7 at night
(`Time_H < 6` or `> 17`) once `TalkFlags[0x73] & 0x10` is set, else frame 3 (`PassTime`).
Frame 7's tile gives light 3, frame 3's none. The same rule runs when objects are loaded
(`__ObjectsDeserialize`, object.c); the power that sets the flag is covered in *The map
and machinery*.

### NPC schedules

Each NPC has a list of schedule entries (`Schedule[SchedPointer[n] ..
SchedPointer[n+1]-1]`, from the SCHEDULE file). Each entry has a time byte, an action (an
AI mode) and a location. Bits 0-4 of the time byte are the hour; bits 5-7 are the day of
the week, 1-7, where 0 means every day. `SchedIndex[n]` is the current entry.

**Every hour** (`UpdateSchedules`, npctrack.c, called from `PassTime`), each NPC 2..0xbe in
turn:

- mode 0xa0 (an assault mode of the combat A.I.) calms down to AI_FINDPATH (0x81);
- an NPC with the WAIT flag (`TalkFlagsHi[n] & 0x80`, i.e. talk flag 15 of the NPC) that
  is not a party member is skipped and keeps its mode (SE gave it mode 0x92);
- otherwise an NPC that exists, is not a party member (`NPCStatus & 0x80`) and is in a
  schedule mode (≥ 0x80) or mode 7 searches its entries **backwards**. The first entry
  whose hour equals `Time_H` and whose day is 0 or today becomes its current entry and the
  NPC is set to AI_FINDPATH. If several entries match, the last one in the list wins. If
  none matches, the NPC keeps what it is doing.

**Getting there** (`PTH_SearchPaths`, called at the start of each `RunNPCTurns`). Each pass
looks at NPCs 2..0xbf in AI_FINDPATH. The first 3 are **teleported** straight to their
scheduled spot (`TeleportToSchedule`), wherever the player is: unlike SE, NPCs near the
party are not spared, so scheduled NPCs jump into place even on screen. A teleport costs 10
movement points and does not apply harmful terrain. Only NPCs left over once 3 have
teleported get a path search. When no search was made, NPCs whose path failed
(AI_PATH_FAILED, 0x86) are set back to AI_FINDPATH and teleported in the same way. NPCs on
a path that leave the 40×40 area are teleported too (`RunNPCTurns`). A path blocked for 3
turns fails; arriving within 1 square of the spot starts the action (`__DoOnPath`).

**Starting the action** (`StartScheduleAction`). The NPC's mode becomes the entry's
action, except that a waiting NPC (WAIT flag, not in the party) keeps its mode and counts
as arrived. Then:

- **sleep (0x91):** the NPC is marked asleep (`NPCStatus |= 4`) and takes its unconscious
  shape (`SetTypeUnconscious`); outside the loaded area its mode is set to 0x80 instead, and
  `RunNPCTurns` starts the action again once it is loaded. Unlike SE no bed object changes
  frame;
- any other action wakes it (asleep cleared, original shape back);
- if it is not on its scheduled square, its mode becomes AI_FINDPATH.

The schedule actions that `NPC_TakeTurn` runs:

| Mode | Behaviour |
|---|---|
| 0x8f (and 0x0c) wander | a random straight step 1 in 8, else wait (`WanderRandomly`) |
| 0x90, 0x94 loiter | 1 in 8 a step biased back toward the spot, else wait (`LoiterNear`) |
| 0x95 | wait (5 points) until NPC 1 is within 2 squares, then talk to him (`TalkWhenNear`) |
| 0x96 converse | if the spot is within 5 squares of the party: walk to the nearest member and talk (then mode 0x90); else loiter |
| 0x98 brawl | 1 in 4: go and harmlessly "hit" an opponent within 5 of the spot, then come back; else loiter |
| 0x99-0x9e | the coal workers (see *The map and machinery*) |
| 0x82 | look for a piece of furniture (`FurnitureTypes`) |
| 0x8b-0x8e | nothing |
| others (0x91 sleep ...) | stand: 5 points |

A turn that spent no points costs 10.

### Air and oxium

From the moment the hatch 0x1ab is opened (cut scene 1; `TalkFlags[0x10] & 0x20`, set
by `UseOpenHatch`, useact1.c), every party member needs oxium 0x83. The check is made
in `PassTime` for each party member that is alive, out of the dream and not NPC 0.

**Each call** with `minutes > 0`, for a member that is not asleep and is inside the loaded
area: if it is NPC 6 (Chsheket, "a mechanical person"), or the flag is not set yet, or it
carries a blob of oxium anywhere in its inventory except in a readied slot
(`FindInvOfType`), it is fine; if it was gasping (`ObjStatus & 0x40`) it "breathes
easier!". Otherwise it starts gasping: "... gasps for air!".

**Each hour** (in the calendar step), unless the party is resting or in the dream and once
the flag is set, each party member other than NPC 6 and not asleep uses one blob: the
first stack found loses 1; a single blob is deleted and the next stack, if any, is used.
With none left, the member starts gasping. The hourly use is skipped while resting, and
the per-call check skips resting members, who are off the map (see *Rest*): **no oxium is
used while resting**.

**Effect of gasping.** `ObjStatus & 0x40` is SE's "cursed" bit, tested by `IsCursed`
(combat.c). It gives **−3 to STR, DEX and INT** (each at least 1; see *Character
attributes*), which also lowers carrying capacity and speeds up the talent decay. There is
**no damage** from lack of air. Hit points show in blue, as do STR and DEX on the
portrait panel (`STAT_PrintHP`, `STAT_ShowPortrait`, status.c). The oxium bin gives 20
blobs at a time (see *The use command*).

### Cold and warmth

The cold is computed in `PassTime` on each call, for every party member that is alive,
out of the dream and not NPC 0, **only while the party is on the surface** (`MapZ == 0`)
**and not resting**.

**Warmth** is the sum of `GetWarmth` (objtype.c) over the member's readied objects
(`WarmTypes`/`WarmthValues`):

| Warmth | Objects |
|---|---|
| 1 | cap 0x1, kerchief 0x6, silk scarf 0x7, man's shoes 0x9, woman's shoes 0xa, lady's silk gloves 0x21, driving gloves 0x22, cotton work gloves 0x23, rubber gloves 0x26 |
| 2 | cowboy hat 0x2, derby 0x5, muffler 0x8, riding boots 0xb, cloth jacket 0x11, cotton pants 0x19, cotton dress 0x1a, denim jeans 0x1b, work gloves 0x24 |
| 3 | pith helmet 0x3, military helmet 0x4, thigh boots 0xd, wool sweater 0x12, cape 0x13, sheepskin jacket 0x16, wool pants 0x1c, wool dress 0x1d, chaps and jeans 0x1e, wool mittens 0x25, welding gloves 0x27 |
| 4 | hip boots 0xe, duster 0x14, wool overcoat 0x15 |
| 5 | arctic parka 0x17 |
| 12 | electric belt 0x5a |

**Cold level** ("enough" means warmth ≥ 10):

| Time | Warmth < 10 | Warmth ≥ 10 |
|---|---|---|
| 22:00-3:59 | 2 | 1 |
| 4:00-6:59 and 18:00-21:59 | 1 | 0 |
| 7:00-17:59 | 0 | 0 |

So in the deep of the night **everyone freezes, however well dressed**; warm clothes only
halve the harm. NPC 6 always has cold 0.

**Effects.** A member with cold > 0 gets the "freezing" status (`NPCStatus & 1`; SE's
"protected" bit) with "... is freezing!". Then, **for each minute**: with a 1 in 2 chance
(`OSI_rand(0, 1)`) it loses `OSI_rand(1, 2)` hit points, and at cold 2 another
`OSI_rand(1, 2)`; when the roll fails, cold 2 still costs `OSI_rand(1, 2)`. Each loss prints
"... is freezing!" and goes through `LoseHP` (damage kind 1), so it can kill. On average
that is 0.75 HP per minute at cold 1 and 2.25 at cold 2. When cold returns to 0, or the
party goes underground or rests, the status is cleared: "... feels warmer!". Freezing
members' hit points show in white (`STAT_PrintHP`). The check ignores whether the member is
asleep or in the loaded area, and uses the party's level (`MapZ`), not the member's.

### Status effects and how they wear off

The status bits are spread over two arrays. `NPCStatus` holds 0x01 freezing (SE's
"protected"), 0x02 paralysed, 0x04 asleep, 0x08 poisoned, 0x10 dead, 0x60 alignment and
0x80 party member. `ObjStatus` (for actors) holds 0x04 charmed, 0x40 gasping (SE's
"cursed") and 0x80 battle frenzy (SE's "wild"). The rolls below are made **once per
minute** for every existing actor 0..0xff, anywhere in the world (`PassTime`).

| Effect | How it is gained | How it wears off (per minute) |
|---|---|---|
| Freezing (NPCStatus 0x01) | the cold | as soon as the cold level is 0 (see *Cold and warmth*) |
| Gasping (ObjStatus 0x40) | no oxium | as soon as oxium is carried (see *Air and oxium*) |
| Poisoned (NPCStatus 0x08) | radium (below) | **1 in 26** (`OSI_rand(0, 25) == 0`); unlike SE it does wear off |
| Charmed (ObjStatus 0x04) | Cupid's bow and arrows 0x28 on an NPC that is not good (`__TryHit`, combat.c; see *Combat*) | 1 in 26, then `COMBAT_stopCharm` restores the alignment kept in `NPCFlag` |
| Paralysed (NPCStatus 0x02) | the freeze ray gun 0xf1 and monster attacks (see *Combat*) | NPC 6: at once. Others: 1 in 4 **and** `GetStr ≥ OSI_rand(1, 30)`, both each minute |
| Asleep (NPCStatus 0x04) | schedule sleep (0x91), script `OP_SETMODE 0x91` (talkdr.c), a member dreaming in the dream machine | 1 in 21 (`OSI_rand(0, 20) == 0`), only if the NPC's mode is not 0x91, its square is loaded, and it is not a party member during a rest. Schedule or script sleep never wears off this way |
| Battle frenzy (ObjStatus 0x80) | use action 27 (`UseBattleFrenzy`, useact3.c), which no object type has in use.c's table, so never in play | at the first minute out of combat |

While poisoned, each minute an actor other than NPC 6 has a 1 in 8 chance
(`OSI_rand(0, 7) == 0`) to lose 1 hit point (`LoseHP`, kind 0). `LoseHP` does nothing to
an actor on another level than the party's (`GetZ != MapZ`), so poison only hurts there,
and not during a rest (the members are off the map; see *Rest*). Poison can kill. It is
cured by the 1 in 26 roll, the rest's own roll (see *Rest*), eating the berries 0x4c
(see *The use command*), the script op `OP_CURE` (talkdr.c), and the party's waking from
defeat (see *Conversations and quest state*).

When an actor wakes from non-scheduled sleep it gets its original shape back and is marked
as handled for terrain this call (`NPCFlag |= 8`). The variable `CharmedNPC`, which SE used
for a summoned creature, is tested each minute but never set in Martian Dreams. Paralysed,
asleep or dead actors get no turns (`RunNPCTurns`), and asleep actors have effective DEX 1.

#### Radium

Carried radium poisons (`PassTime`, per minute, NPCs 0-7 only, whether in the party or
not). For each block of radium 0x1c0, and each chip of radium 0x1c1, carried anywhere in
the actor's inventory but not readied, the game climbs the chain of its holders until it
reaches a lead box 0x8b or the actor itself. If it reaches the actor (no lead box around
it), the actor becomes **poisoned**: "... is poisoned!". The chips are skipped if the
actor is already poisoned or is NPC 6; the blocks are not, so a block repeats the message
every minute and poisons even NPC 6 (which poison does not hurt).

Taking, moving or dropping radium by hand also poisons ("OUCH. It is very hot.") a member
without the first talent who is not NPC 6 and holds no tongs 0x88 (`CmdGet`, `CmdDrop`,
`CmdMove`, objcmd.c; see *Objects and commands*).

#### Ice

Per minute, for NPCs 0-7 (`PassTime`): each bucket 0xa0 of frame 2 or more (a bucket of
ice) in the inventory has a 10 in 101 chance (`OSI_rand(0, 100) < 10`) to become frame 1,
a bucket of water: "Some ice has melted."; each chunk of ice 0x100 has the same chance to
be deleted, with the same message.

#### Drunkenness

`DrunkCounter` is a single counter for the whole party. It goes up by
`OSI_rand(1, 2)` when someone eats the berries 0x4c (`UseEatBerries`, useact3.c), and by 1
for the conversation event 0xfe (talk.c). It goes down by 1 every hour (`PassTime`) and is
cleared on entering the dream (dream.c). While it is **non-zero** (SE: above 3), each step
of the leader has a 1 in 2 chance to go in a random direction, with "you are dizzy!"
(`PartyAdvance`, npctrack.c).

### Rest

#### How to rest

Resting is started by *Use* on a folded tent 0x6a (`UseBed`, useact1.c, the action 0 of
*The use command*). The key R gives the command code 0x89, but `GameLoop` has no case for
it, so it does nothing. `UseBed` refuses in solo mode ("Not while in solo mode!") and in
the dream ("You can't sleep in a dream."). The camp goes on the tent's square, or on the
party's square if the tent is carried. The squares x-1..x+1, y-2..y must hold no object
other than a party member, the folded tent or an object whose tile has `TileFlag2 & 0x10`
("The ... is in the way."), and the tiles x-1..x, y-2..y no tile with `TerrainType & 2`
("The ground is not flat enough."); the tile test misses the column x+1. The folded tent is
not used up.

#### Refusals (`REST_Command`, rest.c)

The checks are made in this order; the first failure refuses the rest:

1. Not in combat: "Not while in combat mode!".
2. "Not while foes are near!": any actor 0x10-0xff that exists, has the hostile bit
   (`NPCStatus & 0x20`) and is on the party's level within 39 squares in both x and y
   (wrapping distances, `REST_FoesNear`). Dead or asleep status is not checked.
3. "... is too near to set up camp.": the first actor 1-0xff that is not dead, not asleep
   and not a party member, within 5 squares in x and y on the party's level, that the
   player (NPC 1) can see (`COMBAT_canSee`; `REST_OthersNear`).
4. Party mode, not solo.
5. Not in the dream (`Active == PartySize`): "You can't sleep while dreaming!".
6. **Poisoned members** are named ("... is poisoned!") and the game asks "Do you really
   want to sleep anyway?". Any answer other than Y cancels.
7. **Talents**: a member with the second (psychic) talent "complains of inanimate things
   talking", one with the third "complains of too much light", one with both says both.
   Then "Nobody can sleep." (or "You can't sleep." alone) and the rest is refused.

Unlike SE there is no test for a vehicle or a turtle, and no food.

#### The tent and how long

Five tent objects 0x86 are put around the camp square (x, y): frame 3 at (x, y-1), 5 at
(x+1, y-1), 6 at (x-1, y), 9 at (x+1, y) and 8 at (x, y); the middle one turns to frame 7
once the party has gathered. The party walks to the camp square and is taken off the map
(to level 6) by `PartyEnter`, which also wakes members and frees them from paralysis
(map.c). SE's places around the leader (`CampOfsX`, `CampOfsY`, `CampFacing`) are still in
the data but unused.

Before 7:00 or from 17:00 the game offers "Rest until sunrise?": `7 - Time_H` hours before
7:00, `24 - Time_H + 7` from 17:00. From 7:00 to 16:59 it offers "Rest until sunset?":
`18 - Time_H` hours. Minutes are ignored, so the party wakes at 7:MM or 18:MM. Any key but
N accepts. N asks "How many hours?" and takes one digit 1-9; anything else rests 0 hours
(straight to waking).

#### Healing

If `NextSleep` is 0 and the rest is **more than 4 hours**, each member heals, before the
time passes:

`HP += (OSI_rand(3, 8) + OSI_rand(3, 8)) × (hours / 2)` (integer division), up to MaxHP.

A separate roll is made for each member. Then `NextSleep` is set to **6**; it goes down by
1 each game hour, so the next rest heals only 6 hours later. (The source's header comment
says 2-8 + 2-8; the calls are `OSI_rand(3, 8)`.) An 8-hour rest heals 24-64 points. No
other hit points come back while resting.

#### The time passes

Then `IsResting` is set and the time passes as `hours × 3` calls of `PassTime(20)`. The
status rolls, light fuel, talents, schedules and calendar all run. Because the party is off
the map (level 6): the cold and the oxium are skipped (`IsResting`), the per-minute poison
damage does nothing, sleep does not wear off for party members, and harmful terrain does not
apply. After the i-th call, if `hours × 3 > PartySize` and i < `PartySize`, party member i
may go up a level (see *The obelisk dream*).

#### Poison during the rest

After the time has passed, each poisoned member rolls `OSI_rand(1, 8) + OSI_rand(1, 8)`:

- 15 or more (a 3 in 64 chance): the poison is cured, "... feels better.";
- otherwise it loses `2 × hours` hit points, but a member that would drop below 5 is
  **set** to 5 (which raises a member that had fewer than 5).

The cure message prints `Names[Party[i]]`, the party name of the slot numbered after the
member's NPC number, instead of `Names[i]`: it names the wrong member, or a blank, when
the NPC number differs from the party slot (a bug).

#### Waking

`IsResting` is cleared, the middle tent goes back to frame 8, and if sound is on and the
sound card is not type 3 the morning song plays (`MUS_MorningSong(0)`, sound.c: only with
music, on the surface between 5:00 and 11:59). The party is put back on the map around the
camp square (`PartyExit`, map.c), each member's movement points set to its DEX. Finally
the first five objects 0x86 found within 6 squares of the camp are deleted, which could
include another tent nearby.

### Experience and levels

**Experience** comes from killing hostile monsters (`AddXP(attacker, LoseHP(...))` in
`__DoHit`, combat.c; the amount is in *Combat*) and from the script op `OP_ADDEXP`
(talkdr.c). Both cap it at **9999** (`AddXP`, combat.c). The party's defeat costs a quarter
of it (`ExpPoints × 3 / 4`, `ResurrectParty`, map.c). Scripts read the player's
experience as the variable E (talk.c).

**Levels** (`Level[]`) go up only in the obelisk dream during a rest, or through the
script op `OP_ADDLVL`, which has no cap. They are **not** computed from experience:
`ComputeLevel` (combat.c), 1 + the number of halvings needed to bring XP to 99 or less, is
never called. A level matters for maximum HP (24 per level) and for the reach of the first
talent (see *Talents*).

#### The obelisk dream (`REST_LevelUp`, rest.c)

During a rest of `hours` hours, if `hours × 3 > PartySize`, then after the i-th 20-minute
step (i < `PartySize`) party member i is checked. A one-hour rest with three or more
members therefore checks nobody. The member must be NPC 1-15 and must have
`ExpPoints ≥ expTable[Level]`:

| Current level | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|---|
| XP needed for the next level | 0 | 100 | 200 | 400 | 800 | 1600 | 3200 | 6400 | 32767 (never) |

So resting takes a character to **level 8** at most, **one level per member per rest**.
A level above 8 (only through `OP_ADDLVL`) would read past the end of the 9-entry table.

The member's level goes up by 1, then it gets **30 hit points back** (up to the new MaxHP).
The dream's portrait and text show in the conversation window: the member sees three stone
obelisks, inscribed with a book, a heart and a sword, feels drawn to one, and is asked
"Which (B H S):". The pronoun is "She" unless `IsMale` (npctrack.c) says the member's
shape is a man's; for a shape outside its humanoid list `IsMale` tosses a coin. Only the
keys B, H and S (either case) are accepted; any other key is ignored and the question waits.

| Key | Raises (by 1, not above 30) |
|---|---|
| S (sword) | STR |
| H (heart) | DEX |
| B (book) | INT |

The attribute is raised in the base array (`STREN`, `DEXTE`, `INTEL`). The new level adds 24
to MaxHP, and STR adds 2 more (see below). Unlike SE's bear, jaguar and monkey, there is
no default choice.

### Character attributes

Each actor has base STR (`STREN`), DEX (`DEXTE`), INT (`INTEL`), hit points (`HitPoints`,
0-255), `Level` and `ExpPoints`. Unlike SE, the portrait panel shows the **effective**
STR, DEX and INT (`STAT_ShowPortrait`, status.c), coloured blue while gasping, STR magenta
with the first talent, dark blue with the battle frenzy. Hit points show red when
poisoned, else white when freezing, else light red below 10, else blue when gasping, else
dark blue with the frenzy (not for NPC 1) (`STAT_PrintHP`). The magenta set for a
paralysed actor is always overwritten by that chain, whose last case is the normal colour,
so paralysis never shows (a bug).

| Function (combat.c) | Rule |
|---|---|
| `GetStr` | STR, −3 if gasping, −3 for NPCs 0-15 with the first talent, +3 with the frenzy; then clamped to 1..30 |
| `GetDex` | DEX, −3 if gasping (min 1), +3 with the frenzy (max 30); 1 if asleep |
| `GetInt` | INT, −3 (min 1, applied once) if gasping, or with the frenzy unless NPC 1 |
| `MaxHP` | NPC 6: **240**. A party member (`NPCStatus & 0x80`): `2 × STREN + 24 × Level` (base STR). Any other actor: the HP of its monster class (`MonsterHP[class + 1]`). 1 if the result is 0 |

`MaxHP` uses the base STR, so gasping and the first talent do not lower it (in SE the curse
did). It has no upper bound: at level 9 or more (only through `OP_ADDLVL`) with high STR it
could exceed what the byte `HitPoints` holds.

Bounds and sources of change:

- The obelisk dream and the script ops `OP_ADDSTR`, `OP_ADDDEX`, `OP_ADDINT` cap the base
  attributes at **30** (talkdr.c).
- After loading, any actor with DEX 0 is given DEX 1 (`InitNPCTracker`, npctrack.c).
- New creatures from eggs roll their attributes from their class and start at level 0;
  classless ones get level 1 (`EGG_generate`, egg.c; see *Combat*). One combat A.I. turns a
  creature 0x17a into 0x17c with STR +5, DEX −5, INT −7 and 20 more HP
  (`COMBAT_AI_Unfriendly`).
- The starting values come from character creation (see *Character creation*).

Where the effective attributes are used:

- **STR** (`GetStr`): carrying capacity (below); to-hit for monster classes with flag 0x20
  (`__AttackTest`); shaking off paralysis; resisting being pushed by *Move*
  (`ResistStrVsStr`, objcmd.c: the target resists if
  `(GetStr(target) / 2 + 30 - GetStr(mover)) / 2 > OSI_rand(1, 30)`); the berserk A.I.'s
  choice of the strongest opponent (STR + DEX + INT, `COMBAT_AI_Berserk`); the script
  variable S and `OP_CANCARRY` (talk.c, talkdr.c).
- **DEX** (`GetDex`): movement points and turn order; to-hit and defence
  (`__AttackTest`); dodging sprays (the freeze ray gun 0xf1's spray paralyses only if
  `OSI_rand(1, 45) > DEX`, other sprays hit only if `OSI_rand(1, 60) > DEX`,
  `COMBAT_Spray`); how long a billiards shot goes on and "Good shot, old bean!"
  (`UseBilliards`, useact2.c); the script variable A.
- **INT** (`GetInt`): the talent decay (below); fleeing when badly hurt (INT ≥ 5,
  `COMBAT_displaySeverity`); a non-hostile attacker's careful weapon choice
  (`OSI_rand(1, 30) ≤ INT`, `__PickWeapon`); the berserk choice; the script variable I.

The combat rules are in *Combat*.

Some actors cannot be hurt (`IsInvulnerable`, combat.c): those whose original type is
0x17d, 0x187, 0x186, 0x166 or 0x17e, and NPCs 0x5c, 0x5d, 0x59, 0x5a, 0x54, 0x52, 0x6d,
0x68, 0x67, 0x69, 0x77, 0x78, 0x40 and 0x3c (see *Combat*).

**How hit points come back** (never with time): the rest's healing, +30 on a level-up,
the script op `OP_HEAL` (to MaxHP), the healing and revival use actions (see *The use
command*), and the party's waking from defeat (to MaxHP; see *Conversations and quest
state*).

### Talents

Martian berries give three mental talents. They are kept in `Talents[16]` (gamestat.c,
saved with the game), one entry per NPC 0-15 with three 4-bit fields: `.a` (the first),
`.psychic` (the second; `.b` in some modules) and `.c` (the third); a fourth field `.d` is
unused. Only the value 0 or non-zero matters for the effects, except that the first
talent's reach is the member's **level**, not the talent's value. The party list shows
icons for them (tiles 0x120, 0x121, 0x122; `STAT_ShowParty`, status.c).

| Talent | Berry and its message | Effects (detailed in the sections named) |
|---|---|---|
| First (`.a`) | 0x49, "relationship with things around you" | *Get*, *Move*, *Use* and *Drop* reach as far as the member's level (+1 for Move and Use; `REACH`, `TALENT_RANGE`; see *Objects and commands*); handling radium does not burn; a devil pod 0x133 thrown within that reach does not break; **STR counts 3 less** (`GetStr`) |
| Second (`.psychic`) | 0x4a, "sensitivity to the feelings of objects" | talking to objects (psychic impressions; see *Conversations and quest state*); objects with `ObjStatus & 1` are drawn outlined (`ShowObjects`, object.c); the dreamstuff hoard 0x1c7 can be used (see *The use command*); the member **cannot rest** |
| Third (`.c`) | 0x4b, "sense of your spatial location" | invisible objects are drawn outlined, and the whole view is lit and unhidden as with the spyglass 0x5e's `XRayView` (`VIS_UpdateView`, visible.c); the member **cannot rest** |

Only the talents of the **active** member (`Party[Active]`) change what is seen and what
can be reached.

**Gaining.** Eating a berry adds 1 to its talent (`UseEatBerries`, useact3.c; see *The use
command* for the action, the berries 0x4c and NPC 6's refusal). Nothing caps the value at
that moment, but at every `PassTime` call any talent of NPCs 0-15 above 10 is cut to
**10**. (A 4-bit field would wrap from 15 to 0 if a member ate six berries of one kind
after reaching 10 within one scheduler round; this is possible in principle, not
observed.)

**Losing.** Each minute (`PassTime`), for NPCs 0-7, each non-zero talent drops by 1 with a
chance of **1 in `4 × GetInt`** (`OSI_rand(1, GetInt(i) << 2) == 1`), each talent rolled
separately. With INT 20 that is 1 in 80 per minute, so a talent of 10 lasts about 13 hours
on average; a gasping member (INT −3) loses them faster. When a talent reaches 0 for a
party member, "A psychic power fades..." prints with sound 0x32; for NPC 0 only in the
dream, for the others only out of it. When the second or third talent of the active member
ends, the view is recomputed.

**Clearing.** Waking from the dream clears all three talents of every party member and of
NPC 0 (dream.c); entering the dream clears NPC 0's. One dream stage sets NPC 0's first
talent to 2 (dreamstg.c). See *The dream*.

### Carrying capacity and weight

Weights are kept in **tenths of a stone** (the panel shows "s"). `TypeWeight[type]` (from
TILEFLAG) is the weight of one object. For the 18 types of `WeightPer10Types` (objtype.c)
it is the weight of 10 units: pistol, shotgun, rifle and elephant gun rounds 0x39-0x3c,
sling stones 0x3f, arrows 0x40, the pocketwatch 0x5b, the masonic symbol 0x5c, spectacles
0x5d, matches 0x6b, oxium 0x83, the berries 0x49, 0x4a, 0x4b, 0x4d, dreamstuff 0x14b,
dollars 0x18 and rubles 0x84.

| Function (status.c) | Rule |
|---|---|
| `GetTypeWeight` | `TypeWeight × qty`, divided by 10 (integer) for the per-10 types |
| `GetWeight` | one object without its contents: qty is the 16-bit amount for quantity kind 4, the quantity byte for kind 2 (stackable types), else 1 |
| `Encumbrance` | the total `GetWeight` of everything inside the object, at any depth; 0 for dreamstuff 0x14b |
| `STAT_ComputeWeights` | `WeightEquip` = sum of `TypeWeight` of the readied objects (one unit each, contents not counted); `WeightInven` = sum of `GetWeight + Encumbrance` of the other carried objects |
| `WeightToStones` | display only: tenths / 10 rounded to the nearest stone; a weight under one stone that is not 0 shows as 1 |

**The limits** use the **effective** STR (`GetStr`), so the first talent and gasping lower
them (SE used the base STR):

- all that is carried (readied + pack + the new object) at most **20 × STR tenths**, i.e.
  2 × STR stones: checked by *Get* ("The total is too heavy.", `CmdGet` objcmd.c), by
  *Move* into another member's inventory (`CmdMove`), and whenever an object is created in
  or moved into an actor (`AddInvObj`, `InsertObj`, object.c; `InsertObj` checks it only for
  an object coming from the map);
- what is readied (readied + the new object) at most **10 × STR tenths**, i.e. STR stones:
  checked when an object is created or moved into a readied slot (`AddInvObj`,
  `InsertObj`), which is how *Ready* puts it there (`STAT_Ready`; see *Objects and
  commands*). Unlike SE, the readied limit is enforced.

A body 0x155 being picked up counts its own weight only. An object of weight 0 (unless it
is a per-10 type) or 255, or an actor, cannot be taken ("That is not possible."). A pack
holds at most 16 objects (`OBJ_InvSlotsUsed`). Being over the limit (when STR drops) has
no effect other than refusing new objects. The inventory panel shows the readied weight /
STR and the total / 2 × STR (`STAT_DrawInventory`).

### Movement points and turns

**The scheduler** (`RunNPCTurns`, npctrack.c) runs after every player command, as in SE.
Each actor has `MovePts` (a signed byte). Repeatedly, among the actors on the party's
level inside the loaded 40×40 area that can act, it picks the one with the highest
`MovePts / GetDex` ratio. Actors that cannot act are those that are motionless (mode 0),
paralysed, asleep (unless in schedule mode 0x80), dead, inside a container or inventory,
or have `MovePts ≤ 0`. An actor with `MovePts ≥ GetDex` is taken at once. Ties go to the
higher DEX. Followers (mode 1) are skipped, because they move with the leader; while any
follower has `MovePts ≤ 0`, `Party[0]` gets 0 points, so the leader waits for the slowest
follower. An actor in mode 0x80 found in the area starts its schedule action.

When nobody has points left, every actor 0..0xff gets its **effective DEX** in points: a
negative balance is carried over (`MovePts + GetDex`), otherwise the balance is set to
`GetDex`, so unspent points are lost. Then **1 minute passes** (`PassTime(1)`). The loop
ends when the player's commanded character (mode 2) is picked. Actors that leave the area
are frozen with 0 points; scheduled walkers among them are teleported.

**Costs** (`SubMov`). Every cost is reduced by 2 for actors 0..0x0f **out of combat**, but
is at least 1. The numbers below are before that reduction. `SubTerrainMov` charges
`5 + (TerrainType >> 4)` of the base tile, plus the same upper nibble for every object
tile on the square, skipping impassable tiles. It is charged on the square the actor is
**leaving**, because it is applied before the move.

| Action | Cost | Where |
|---|---|---|
| Leader step, straight | `SubTerrainMov` (5 + terrain) | `PartyAdvance` (npctrack.c) |
| Leader step, diagonal | the same + 5, charged as a separate `SubMov` and so reduced separately | `PartyAdvance` |
| Follower step (any terrain) | 5 | `MoveFollowers` |
| NPC step (`TryStraightMove`) | `SubTerrainMov`, paid even when blocked | npctrack.c |
| Attack | 10 | `COMBAT_attack` (combat.c) |
| Get | 3; a stackable object 3 more (it goes through `AddInvObj`) | `CmdGet`, `AddInvObj` |
| Drop or throw | 3 | `CmdDrop` |
| Any object created in an inventory | 3 (to the owner) | `AddInvObj` |
| Move (push) | 5 | `CmdMove` |
| Use | 5 | `CmdUse` (use.c) |
| Pass (space or Esc) | all remaining points (set to 0); followers regroup | `GameLoop` |
| NPC loitering, waiting, standing | 5 | `LoiterNear`, `NPC_TakeTurn` |
| An NPC turn that spent nothing | 10 | `NPC_TakeTurn` |
| Teleport to schedule | 10 | `TeleportToSchedule` |

Unlike SE, `InsertObj` (moving an existing object into an inventory) charges nothing. A
blocked leader step costs nothing but applies the terrain of the square it bumped into
(`ApplyTerrainEffects`; see *The map and machinery*).

So a character's speed is its effective DEX divided by what it does. Out of combat a
straight party step on cost-0 terrain is 3 points; a DEX 20 party walks about 6-7 steps per
minute of game time, slowed by its lowest-DEX follower. After resting, waking from
defeat, a party teleport or entrance (`PartyExit`, `PartyEnter`, map.c) or being created,
`MovePts` is set to DEX.

### What could not be determined

- What the gray sky bar of levels 2 and 3 stands for, and what the frames of the sky
  picture show, were not checked against the graphics.
- The meaning of `TileFlag2 & 0x10`, which keeps an object from blocking a camp, is not
  known.
- Whether the R key's command 0x89 is handled anywhere else was searched for (gameloop.c,
  console.c) without finding a handler; it appears to do nothing.
- How often each paralysing or charming attack happens in normal play is left to
  *Combat*.

## Combat

This section covers combat mode, the Attack command, how an attack is resolved (to-hit,
damage, armour, wounds, bleeding, paralysis, death, experience), the weapon and armour
tables, ranged and spraying weapons, the deaths that make the game unwinnable, the NPCs
that cannot be hurt, the monster tables, the special creatures, the combat A.I. and the
eggs that spawn creatures. Almost all of it is in `SRC/combat.c` (segment 07D1); the
weapon and armour tables are in `SRC/objtype.c`, the monster tables in
`SRC/MONSDATA.ASM`, creature generation in `SRC/egg.c`, the A.I. dispatcher and the turn
scheduler in `SRC/npctrack.c`, the missile animation in `SRC/gfx.c`. "1dN" below means
`OSI_rand(1, N)`. Object names are the LOOK names (`build/names/look.py`); numbers such as
"missile tile 0x103" are tile numbers, not object types.

### Entering and leaving combat mode

`InCombat` is the flag. Entering draws the combat mark on the command bar (entry 0 of
mdscreen.lzc's part 2), leaving draws the peace mark (entry 1).

Combat begins:

- with the combat command ("Begin combat!"), refused in solo mode ("Not while in solo
  mode!") (`GameLoop` command 0x88, gameloop.c). Unlike SE there is no vehicle test;
- automatically, when a blow from an attacker whose alignment has bit 0x20 (evil or
  chaotic) does damage after armour to a player-controlled actor, out of combat, in party
  mode (`__DoHit` calls `COMBAT_beginAttacked`);
- when a creature hidden in the sand bursts out (mode 0x12, `COMBAT_AI_HiddenInSand`),
  whatever the mode of the party and even in combat;
- from conversation events (talk.c, see *Conversations and quest state*) and dream stages
  (dreamstg.c, see *The dream*).

On entering (`COMBAT_begin`, `COMBAT_beginAttacked`, combat.c): every party member that is
not charmed (ObjStatus 4) takes its combat mode (`NPCMode = NPCComMode`) and becomes its
own leader. The controlled member (in party mode the first one that is not paralysed,
asleep, dead or dragged under; in solo mode the solo member) gets mode 2 (command).
`COMBAT_begin` resets `CombatOutcome` and plays `MUS_CombatSong`; `COMBAT_beginAttacked`
keeps `CombatOutcome` and plays `MUS_AttackedSong` (sound.c). On the surface the first
plays jingle 6, the second jingle 7, both followed by song 8; underground both fade to
song 14 (level 1) or song 13 (levels 2 and 3).

Combat ends:

- with the combat command again ("Break off combat!");
- when the party takes an entrance (`GoThroughEntrance`, map.c), when a game is restored
  (`GAME_Restore`, console.c), when the party wakes after being knocked out
  (`ResurrectParty`, map.c), and in several places of the dream (dream.c, dreamobj.c,
  `PartyUnconscious`).

There is no automatic end when the enemies are gone. On leaving (`COMBAT_breakOff`): the
"enemy heard" warning is re-armed (`EnemyWarningGiven = 0`, see *Eggs and spawning*);
dead members get mode 0 (motionless); the others that are not charmed follow again
(party mode, mode 1) or stand still (solo mode, mode 0); the controlled member gets mode
2. The music (`MUS_CombatEndSong`): underground, the area song; on the surface jingle 11
if `CombatOutcome` is negative (a party member died), jingle 12 if positive (an enemy
died), else a fade to the area song. Then one minute passes (`PassTime(1)`).

While in combat:

| Rule | Where |
|---|---|
| No switching to party mode or solo mode, no saving, no resting ("Not while in combat mode!") | `GameLoop`, rest.c |
| A player-controlled actor cannot step to a square more than 8 squares (either axis) from the screen centre (`MapX`,`MapY`) | `TryStraightMove` (npctrack.c) |
| Party members (NPCs 0-15) out of combat pay 2 movement points less for everything (minimum 1); in combat they pay full price | `SubMov` (npctrack.c) |
| The "wild" status (ObjStatus 0x80) lasts only in combat: it is cleared every minute outside | `PassTime` (gameloop.c) |
| The battle frenzy object works only in combat; see *The use command* | `UseBattleFrenzy` (useact3.c) |
| Each player turn, while `CombatOutcome` is 1 or -1, it is doubled and (on the surface) song 9 (a member died) or 10 (an enemy died) plays, then song 8 | `GameLoop`, `MUS_CombatOutcomeSong` (sound.c) |

`CombatOutcome` is set by `COMBAT_Kill`: -1 when a player-controlled actor dies; 1 when a
hostile (bit 0x20) actor other than a tentacle 0x16a dies and nothing worse happened yet.
`COMBAT_displaySeverity` also sets it to 1 when the giant maw 0x175 becomes critical.

### The party's combat modes

Each actor has a combat mode `NPCComMode`, used as its A.I. mode (`NPCMode`) during
combat. The equipment view shows it (`ComModeNames`, `STAT_refreshComMode`, status.c):

| NPCComMode | Shown | A.I. in combat |
|---|---|---|
| 2 | CMND | the player controls the member (the turn scheduler stops on it) |
| 4 | RANGE | `COMBAT_AI_Range` |
| 6 | WILD | `COMBAT_AI_Berserk` |
| 7 | FLEE | `COMBAT_AI_Retreat` |
| 8 | ATTK | `COMBAT_AI_Assault` |

The combat-mode button of the equipment view (x 248-263, y 102 and below) adds 1 to the
mode, skipping 3 and 5 and going from 9 back to 2, so it cycles CMND -> RANGE -> FLEE ->
ATTK -> CMND; 6 is never reached (`MouseInEquipView`, console.c; its comment says "2, 4,
5, 7, 8" but 5 is skipped too). The button does nothing for the first member
(`StatusMember` 0), for a wild member (ObjStatus 0x80) and in the dream (GameFlags 0x10).
In combat the new mode applies at once unless the member is the active one or charmed. A
wild member uses `COMBAT_AI_Berserk` whatever its mode and shows "WILD" (except NPC 1)
(`NPC_TakeTurn`, `STAT_refreshComMode`); `SetPartyMode` gives it mode 6 in combat. An NPC
that joins the party gets ATTK (talk.c, the join function).

### Turn cost

The scheduler gives every actor its DEX in movement points each round (see *Time, rest,
stats and levels*). An attack costs 10 points: the player's whole Attack command
(`COMBAT_attack`, once, however many weapons strike) and every A.I. attack
(`SubMov(npc, 10)`). An actor whose A.I. spent nothing loses 10 (`NPC_TakeTurn`). Waiting
costs 5 in most modes; a few MD modes cost 2 or 3 (below).

### The Attack command

`COMBAT_attack` and `COMBAT_AttackWithNextWeapon` (combat.c):

- "Attack " is printed and the active member's readied weapons are listed: every equipped
  object (ObjStatus position 0x18) for which `GetDamagePoints` is not -1, that is every
  type of the weapon table below, at most 8 (`COMBAT_InitWeaponList`). With none, "with
  bare hands-" and the member attacks with itself (its natural attack). Objects that can
  be readied in a hand but are not in the weapon table (the martian ritual pod knife 0xf2,
  the tongs 0x88...) cannot be used to attack.
- The player picks a target for each weapon in turn ("Attack with <weapon>-"); the cursor
  is limited to the weapon's range. The target must be on the map, not in an inventory;
  any other key or choice prints "what?" and ends the command. All the attacks of one
  command cost 10 points together, so two readied weapons strike twice for one price.
- Before each attack the ammunition is checked; without it the attack is skipped with a
  sound (5) and "Out of arrows!" (bow) or "Out of ammunition!" (all the others): see the
  weapon table. The ammunition must be among the attacker's direct contents
  (`FindInvType`), except the bow's arrows, which may be anywhere in the inventory but not
  readied (`FindInvOfType`).
- Targeting oneself prints "pass." and does nothing. Otherwise the target's name is
  printed (or, on an empty square, the terrain's name, "nothing!" in the dark) and the
  attacker turns to face it. On an empty square the target becomes the first object
  there (`COMBAT_GetTargetAt`).
- Farther than the weapon's range: "Out of range!". The code also handles a result 3
  ("That is not possible."), which neither `__TryHit` nor `COMBAT_FireM60` returns.
- The M60 machine gun 0x139 strikes through `COMBAT_FireM60` (see *Special weapons*), the
  others through `__TryHit`.

### Resolving an attack

`__TryHit(attacker, x, y, weapon)` (combat.c) does every attack, the player's and the
A.I.'s; the weapon is the attacker itself for a natural attack:

1. Range: if `COMBAT_RangeToObj(x, y, attacker)` is more than the weapon's range, return
   "out of range".
2. Ammunition (A.I. attackers too): the derringer and revolver need a pistol round 0x39,
   the shotgun a shotgun shell 0x3a, the rifle a rifle round 0x3b, the elephant gun an
   elephant gun round 0x3c, the sling a sling stone 0x3f, all as direct contents; the weed
   sprayer and the spray gun need a quantity above 0. **The bow's arrows are not checked
   here**, only by the player's command, so an A.I.-run bowman (a party member in an A.I.
   mode included) shoots without arrows: `TakeObj` then takes nothing. The sprayers'
   checks are under *Spraying weapons*. Without ammunition the attack does nothing (an
   A.I. attacker still pays 10).
3. The attack is ranged when the target is farther than 1, or the weapon is a gun or bow
   (types 0x29-0x30), the weed sprayer 0x81, the spray gun 0x105, a ray gun 0xf0/0xf1 or
   Cupid's bow 0x28. A ranged attack flies a missile (`COMBAT_FireRangedWeapon`). If the
   missile stops short of the target (a blocking tile or object, or out of range), the
   blow goes to what `COMBAT_GetTargetAt` finds where it stopped, possibly nothing
   (`Missiles`, gfx.c).
4. The to-hit test (`__AttackTest`, below) is made before the missile flies, against the
   selected target if it is an actor; if the selected target is not an actor it is made
   after, against the actor the missile reached. A part 0x172/0x173 of a rockworm counts
   as its head (`COMBAT_getHead`). The weed sprayer and spray gun always hit. **Unlike in
   SE, the result is used**: a miss plays sound 3 and nothing else happens (the
   ammunition is spent all the same).
5. A melee attack plays sound 0 when the attacker is within 5 squares of the screen centre.
6. On a hit (and if the target is not the attacker): the special weapons' effects (below),
   else `__DoHit(attacker, target, damage, kind)`, kind 3 for the heat ray gun, 1 for the
   freeze ray gun, else 0. The damage is the class's `MonsterDamage[class + 1]` for a
   natural attack (1 for an actor without a class), else `GetDamagePoints(weapon)` (1 if
   that is -1).
7. Then, for a sprayer, the spray (below).

#### The to-hit test

`__AttackTest` (combat.c):

- Against an object that is not an actor: always a hit.
- The attacker's stat is its STR (`GetStr`) for a natural attack of a class with flag
  0x20, else its DEX (`GetDex`); the defender's stat is its DEX (1 if asleep).
- Hit when `1d30 >= (defenderDEX + 30 - attackerStat) / 2` (integer division).

So with T = (defenderDEX + 30 - attackerStat) / 2 the chance is (31 - T)/30: always
when T is 1 or less, never when T is above 30. Equal stats give T = 15, 16 in 30. There
are no special cases (SE's had some for its spear and staff).

#### Damage

`__DoHit(attacker, defender, hitPts, kind)` (combat.c):

1. hitPts -1 counts as 1. If `1 < hitPts < 255` the damage is `1d(hitPts)`; 0, 1 and 255
   stay as they are (none of the tables has 255).
2. Armour: P is the defender's protection (`GetProtection`) if it is an actor, 2 for a
   glow worm 0x184 of frame 0, else 0; P is 0 for kind 2. If P > 0 and the damage is under
   255 it is reduced by `1dP`.
3. Damage 0 or less: the hit flash (tile 0x101; for kind 1 the effect flash, tile 0x102)
   and "<name> grazed.". Otherwise, if the attacker is hostile (bit 0x20) and the defender a
   player-controlled actor, out of combat in party mode, combat begins
   (`COMBAT_beginAttacked`); then `LoseHP(defender, damage, kind)` and the experience it
   returns goes to the attacker (`AddXP`).
4. If the defender is the selected object (`SelectionObj`), `COMBAT_ProvokeNeutral` runs.
5. If the defender is now at 0 hit points: if the attacker is a canal worm 0x17b and a
   body was made, `COMBAT_CanalWormGulp` (see *Special creatures*); stop.
6. Paralysis: if the attacker's class has flag 4, one time in 4 (`OSI_rand(0, 3) == 0`)
   the defender is paralysed ("<name> paralyzed!", NPCStatus bit 2), unless it is
   invulnerable. This test also runs after a graze.
7. If the defender is alive and the damage was above 0, the wound message
   (`COMBAT_displaySeverity`, below).

STR adds nothing to damage. There are no critical hits. No blow poisons in MD: no combat
code sets the poisoned bit (SE's class flag 0x40 poison is gone).

#### Weapons

The tables are in objtype.c: `WeaponTypes`/`WeaponDamages` (32 entries, read by
`GetDamagePoints`), `RangedWeaponTypes`/`WeaponRanges` (18 entries, `GetWeaponRange`; any
other type has range 1), `MissileWeaponTypes` with `WeaponMissileTiles`, `MissileSfx` and
`MissileAmmo` (17 entries, `GetMissileInfo`), and `TwoHandedTypes`/`OneHandedTypes`
(`STAT_GetEquipSlot`; see *Objects and commands*). Unlike SE no weapon has a special
case in `GetDamagePoints` or `GetWeaponRange`.

| Type | LOOK name | Max damage | Range | Hands | Ammunition | Missile tile, sound |
|---|---|---|---|---|---|---|
| 0x29 | derringer | 15 | 4 | one | pistol round 0x39 | 0x103, 7 |
| 0x2a | revolver | 18 | 5 | one | pistol round 0x39 | 0x103, 7 |
| 0x2b | shotgun | 20 (spray) | 4 | two | shotgun shell 0x3a | spray only |
| 0x2c | rifle | 30 | 9 | two | rifle round 0x3b | 0x103, 8 |
| 0x2d | Belgian combine | 30 / 20 (spray) | 9 | two | rifle round 0x3b; shell 0x3a for the spray | 0x103, 8 |
| 0x2e | elephant gun | 45 | 9 | two | elephant gun round 0x3c | 0x103, 8 |
| 0x2f | sling | 8 | 4 | two | sling stone 0x3f | 0x23e, 6 |
| 0x30 | bow | 12 | 5 | two | arrow 0x40 | 0x23f, 6 |
| 0x31 | hatchet | 15 | 1 | one | | |
| 0x32 | axe | 20 | 1 | one | | |
| 0x33 | ball-peen hammer | 10 | 1 | one | | |
| 0x34 | sledge hammer | 25 | 1 | two | | |
| 0x36 | knife | 10 | 1 | one | | |
| 0x37 | machete | 20 | 1 | one | | |
| 0x38 | saber | 25 | 1 | one | | |
| 0x10 | bloody saber | 30 | 1 | one | | |
| 0x41 | pick | 15 | 1 | two | | |
| 0x42 | shovel | 8 | 1 | two | | |
| 0x43 | hoe | 10 | 1 | two | | |
| 0x44 | rake | 10 | 1 | two | | |
| 0x45 | pitchfork | 15 | 1 | two | | |
| 0x46 | cultivator | 12 | 1 | two | | |
| 0x47 | scythe | 20 | 1 | two | | |
| 0x48 | saw | 10 | 1 | two | | |
| 0x191 | pool cue | 12 | 1 | two | | |
| 0x66 | pry bar | 12 | 1 | one | | |
| 0xf0 | heat ray gun | 20 / 25 (spray) | 6 | two | its own quantity (charges) | 0x16b, 10 |
| 0xf1 | freeze ray gun | 15 / 10 (spray) | 6 | two | its own quantity (charges) | 0x16a, 10 |
| 0x81 | weed sprayer | 60 | 2 | one | its own quantity | 0x10a, 6 |
| 0x105 | spray gun | 60 | 2 | one | its own quantity | 0x10a, 6 |
| 0x28 | Cupid's bow and arrows | 1 | 6 | two | none | 0x23f, 6 |
| 0x139 | M60 machine gun | 254 | 9 | two | none | 0x10b, 0x32 |

Four creature types have a natural attack with a range and a missile (no ammunition):
cave worm 0x16e (range 3, missile tile 0x16f, sound 0x22), sand trapper 0x182 (4, 0x16c,
0x22), proto martian 0x16c (3, 0x16d, 6) and pod devil 0x180 (2, 0x16e, 6). Every other
natural attack has range 1.

#### Ranged weapons and ammunition

`COMBAT_FireRangedWeapon` (combat.c), for every ranged attack: the weapon's sound plays,
the missile flies (`COMBAT_Missile` -> `Missiles`, gfx.c) up to the weapon's range, then
the ammunition is used according to `GetMissileInfo`:

- an ammunition type (guns, sling, bow): one object of that type is taken from anywhere in
  the attacker's inventory (`TakeObj`);
- "own quantity" (ray guns, weed sprayer, spray gun): the weapon's quantity drops by 1;
- "none" (Cupid's bow, M60, the creatures' missiles): nothing.

The function also has SE's code for a thrown weapon (one not in `MissileWeaponTypes`,
when the target is farther than 1): it lands where the missile stopped (or is deleted on
open water) and a spare of its type is readied in the hand slot it came from. **No MD
weapon reaches it**: every type with a range above 1 is in `MissileWeaponTypes` except
the shotgun, which never makes a single shot.

Combat does not read the radium. The gun settings and loading the sprayers are use
actions (see *The use command*); combat only reads the resulting quality and quantity.

#### Range and line of fire

The combat distance between two squares is `RangeTable[|dx|][|dy|]`, roughly `max +
min/2`; if either axis is 8 or more the distance is 9 (`COMBAT_range`; the map wraps):

| dy \ dx | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| 0 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
| 1 | 1 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
| 2 | 2 | 2 | 2 | 3 | 4 | 5 | 6 | 7 |
| 3 | 3 | 3 | 3 | 4 | 5 | 6 | 7 | 7 |
| 4 | 4 | 4 | 4 | 5 | 6 | 7 | 7 | 8 |
| 5 | 5 | 5 | 5 | 6 | 7 | 7 | 8 | 8 |
| 6 | 6 | 6 | 6 | 7 | 7 | 8 | 8 | 8 |
| 7 | 7 | 7 | 7 | 7 | 8 | 8 | 8 | 8 |

`COMBAT_RangeToObj(x, y, obj)` measures from an object, by the `TileFlag` bits 0x40/0x80
of its tile:

- neither bit: `COMBAT_range` from its square;
- one of them (a shape of two squares): by `frame / 4`, 0 measures from the square above
  it (y - 1), 1 from the square to its left (x - 1), 2 and 3 from its own square. For
  frames 16 and up the code returns `x / (x - x)`, a division by zero (a crash), which no
  known object reaches;
- both (a shape of four squares, 2 x 2, the object at its bottom right): 1 when (x, y) is
  within -2..+1 of the object's square on both axes (touching the block), else
  `COMBAT_range` from its square.

Line of fire: an A.I. attacker with a target farther than 1 first flies an invisible test
missile (`COMBAT_Missile` with tile 0) with the weapon's range, and attacks only if it
reaches the target's square (`Missiles` returns 1 when every missile reached its
destination; `COMBAT_Missile` has no `return` statement but leaves that value in AX, as
the machine code shows). A missile stops when it leaves the range (a `RangeTable`
distance above the range, or 8 or more on an axis), at a tile that blocks missiles
(windows are crossed when next to the shooter) or at an object whose tile blocks
missiles. With no line of fire the attacker steps toward the target's row or column: one
time in 2 toward (its own x, the target's y), else toward (the target's x, its own y).

#### Spraying weapons

The shotgun 0x2b, the Belgian combine 0x2d and the ray guns 0xf0/0xf1 spray
(`COMBAT_Spray`, `MissileSpray` in gfx.c). A spray is 12 missiles of range 6 toward fixed
offsets in the direction of the target, whatever its distance: for north (0,-1), (0,-2),
(-1,-2), (1,-2), (0,-3), (-1,-3), (1,-3), (0,-4), (-1,-4), (1,-4), (-2,-4), (2,-4), a cone
4 squares deep (`SprayDX`/`SprayDY`, by `SprayDir`). Everything the missiles cross or
stop on is hit once: the breakable objects and buckets 0xa0 first, then every actor (the
shooter excepted, the party included).

| Weapon | Missile tile | Kind | Damage | Sound |
|---|---|---|---|---|
| shotgun 0x2b, Belgian combine 0x2d | 0x106 | 0 | 20 | 8 |
| heat ray gun 0xf0 | 0x14e | 3 (burns) | 25 | 10 |
| freeze ray gun 0xf1 | 0x14f | special | 10 | 10 |

What a weapon does depends on its setting, its quality (`__TryHit`):

| Weapon | Quality 0 | Quality 1 | Quality 2 or more |
|---|---|---|---|
| shotgun | spray | spray | spray |
| Belgian combine | single shot (rifle round) and spray (shell) | single shot | spray |
| ray guns | single shot (1 charge) and spray (4 charges) | single shot | spray (4 charges) |

The combine drops whichever part lacks its ammunition (and does nothing with neither); a
ray gun without charges does nothing, and drops the spray when its charges are fewer
than the parts need (1 + 4 for both, 4 for the spray alone). The single shot is an
ordinary ranged attack (to-hit test, `__DoHit`); the spray follows it toward the same
square (where the shot's missile stopped), unless the shot ended in Rasputin's throw
(`CancelSpray`, see *Invulnerable NPCs*). A spray uses one shotgun shell (`TakeObj`) for
the shotgun and the combine, 4 charges for a ray gun. The player's command refuses with
"Out of ammunition!" a combine with neither usable part, and a ray gun with no charges or
set to spray with fewer than 4.

Each actor hit by a spray may dodge ("<name> dodges!", sound 3): it is hit when
`1d60 > DEX` (`GetDex`), so the chance to be hit is (60 - DEX)/60 (none at DEX 60 or
more). Objects never dodge. Hits go through `__DoHit` with the table's damage and kind (so
armour applies and the sprayer gains the experience), without a to-hit test.

The freeze ray gun's spray and single shot work alike: a bucket of water (0xa0 frame 1)
becomes ice (frame 2, "The water freezes."). An actor (for the spray: only when
`1d45 > DEX`, else it dodges) of a class with flag 0x4000 or 0x80 is paralysed (NPCStatus
bit 2, "<name> paralyzed!"); if the class has 0x80 without 0x4000 (plant life) it is
also hurt (`__DoHit` with kind 1: up to 10 for the spray, up to 15 for the shot), else
only the effect flash shows. Any other actor: "It has no effect.". This paralysis ignores
invulnerability.

A clever ally does not spray through the party (`__PickWeapon`): an A.I. attacker that
is not hostile to the party (alignment bit 0x20 clear) is clever when `1d30 <= INT`
(`GetInt`). If it is clever and a party member stands in the spray cone toward its target
(`COMBAT_NoPartyInLine`, an invisible spray), the shotgun is not chosen at all and a
combine or ray gun is set to quality 1 (single shot). Otherwise every combine and ray gun
the A.I. looks at is set to quality 0. **So when a party member in an A.I. mode picks its
weapon, the setting the player chose for its combine or ray gun is overwritten.**

#### Special weapons

All in `__TryHit` (combat.c) except the M60, applied when the attack hits:

| Weapon | Target | Effect |
|---|---|---|
| freeze ray gun 0xf1 | bucket or actor | see *Spraying weapons* |
| weed sprayer 0x81, spray gun 0x105, quality not 0 (loaded with paint) | the actor 0x91 (monster footprints) | it becomes a proto martian 0x16c, "<name> becomes visible!" |
| the same | an invisible actor or object | made visible, "<name> becomes visible!" |
| the same | anything else | "It has no effect." |
| weed sprayer, spray gun, quality 0 | an actor of a class with flag 0x80 and not 0x4000 (plant life) | `__DoHit` with kind 2 (no armour), up to 60 |
| the same | anything else | "It has no effect." |
| Cupid's bow 0x28 | an actor that is not good | its alignment is kept in NPCFlag, it becomes good and charmed (ObjStatus 4): "<name> charmed!", effect flash, no damage |

The sprayers' class test reads `MonsterFlags[-1]` for a target without a class (it only
excludes class 0), which holds 0x1000 (the word made of the last two bytes of
`MonsterAlignment`), so such targets, plant objects included, get "It has no effect.".
The A.I. chooses a loaded sprayer only against an invisible opponent, and an unloaded one
only against a plant-life class or a type of `IsPlantType` (`__PickWeapon`). The charm
wears off with time (see *Time, rest, stats and levels*).

**The M60 machine gun** (0x139), in the player's command only (`COMBAT_FireM60`): the
attacker's figure, NPC 0 (the dreamer: this is a dream weapon, see *The dream*), is drawn
as 0x18c/0x18d (yourself) facing the target while it fires.

1. Farther than 9: "Out of range!".
2. An invisible test missile flies to the target (the head of a rockworm part).
3. If it stopped short on a square with the tile 0x90, 0x91, 0x95 or 0x96 and no crystal
   wall 0xb6: sounds, a visible missile (tile 0x10b) to that square, a crystal wall 0xb6
   (frame 0-3 by the tile) placed there and a broken crystal 0xda placed at (stop x + the
   x step toward the target, **target y** + the y step): the code uses the target's y
   where the stop square's was meant.
4. **If the test missile reached the target, the command ends there and nothing is hit.**
   Only when it was stopped does the real shot fire: the tiles 0x90-0x97 lose their
   missile-blocking flag (`TileFlag` 0x20) for the flight, missile tile 0x10b flies to
   the target, and whatever it reaches gets `__DoHit` with 254 (so 1d254 minus armour; no
   to-hit test). Hitting NPC 0x66 (Raxachk) instead plays the hit flash, cut scene 9
   (`Cutscene9`), wakes the dreamer (`DreamWakeUp`) and sets TalkFlags[0x46] & 8.

An A.I. actor holding the M60 (none is known to) would fire it through `__TryHit` as an
ordinary ranged weapon of damage 254.

### Armour

Protection is the sum of `GetArmorPoints` over the actor's equipped objects
(`GetProtection`, combat.c; `ArmorTypes`/`ArmorValues`, objtype.c). Unlike SE there is no
curse or protection term. Each blow is reduced by 1d(protection) (see *Damage*).

| Type | LOOK name | Points | Type | LOOK name | Points |
|---|---|---|---|---|---|
| 0x03 | pith helmet | 1 | 0x16 | sheepskin jacket | 2 |
| 0x04 | military helmet | 2 | 0x17 | arctic parka | 2 |
| 0x0b | riding boots | 1 | 0x1b | denim jeans | 1 |
| 0x0d | thigh boots | 2 | 0x1e | chaps and jeans | 3 |
| 0x0e | hip boots | 3 | 0x22 | driving gloves | 1 |
| 0x15 | wool overcoat | 1 | 0x24 | work gloves | 2 |
| | | | 0x26 | rubber gloves | 1 |
| | | | 0x27 | welding gloves | 3 |

The best possible total is 2 + 3 + 2 + 3 + 3 = 13 (head, feet, coat, legs, hands). **The
monster classes' armour (`MonsterArmor`) protects nothing**: only the experience formula
reads it. A creature is protected only by armour objects it wears; MD's eggs give none
(`EGG_GivePossessions` is empty).

### Derived attributes

Combat uses these (combat.c; how the stats themselves change is in *Time, rest, stats and
levels*):

| Value | Rule | Where |
|---|---|---|
| Effective STR | STR, -3 if cursed (ObjStatus 0x40), -3 for NPCs 0-15 while their first talent is above 0, +3 if wild (ObjStatus 0x80); then kept within 1-30 | `GetStr` |
| Effective DEX | DEX, -3 if cursed (minimum 1), +3 if wild (maximum 30); 1 if asleep | `GetDex` |
| Effective INT | INT, -3 (minimum 1) if cursed, or if wild and not NPC 1 | `GetInt` |
| Max HP | 240 for NPC 6; a player-controlled actor `2 * STR + 24 * level` (raw STR); any other actor its class's `MonsterHP[class + 1]` (0, so 1, without a class); 1 if 0 | `MaxHP` |
| XP gain | added, capped at 9999 | `AddXP` |

So a creature's Max HP is its class's table value, not the value rolled for it by its egg
(which can be up to half as much again, see *Eggs and spawning*): a strong one reads as
unhurt in the wound messages until it falls below its class value, and healing while
fleeing stops at the class value. A named NPC that is not in the party has its class's
Max HP too (90 for most human figures).

### Losing hit points

`LoseHP(obj, damage, kind)` (combat.c) is used by every source of damage (blows,
explosions, terrain, hazards, poison). Nothing happens if the object is on another level
or damage is negative. In dream stage 0xe0 the dreamer (NPC 0) cannot drop below 1 hit
point. The kinds:

| Kind | Used by | Effect in `LoseHP` |
|---|---|---|
| 0 | ordinary blows | actors may bleed (below) |
| 1 | freezing (freeze ray gun) | the effect flash (tile 0x102) instead of the hit flash; freezes a bucket of water (0xa0 frame 1 -> 2, "The water freezes."); the paralysis itself is set by the freeze-ray code |
| 2 | the weed sprayer | `__DoHit` skips armour; otherwise as 0 without bleeding |
| 3 | burning (heat ray gun, fires) | a class with flag 8 is immune ("It has no effect."); a class with flag 0x10 should take double damage, but the code computes `damage << 1;` and drops the result (marked `/*sic*/`); a burnable breakable object (`IsBurnableType`) loses all its points; thaws ice (0xa0 frame 2 -> 1, "The ice thaws.") |

No class has flag 8 or 0x10, so neither burning rule ever applies.

For an actor: the hit flash (tile 0x101, sound 1 for a player-controlled actor, else 2,
when on screen) wakes it if it was asleep. Then:

- an invulnerable actor keeps 255 hit points and reacts instead (see *Invulnerable
  NPCs*);
- kind 0 only: bleeding, when `OSI_rand(6, 100) <= damage`, so for damage d from 6 to 100
  a chance of (d - 5)/95 (not for the dreamer in stage 0xe0);
- if its hit points are at most the damage, `COMBAT_Kill`;
- else it loses them. An actor in mode 9 or 0x0b (unfriendly) switches to mode 8
  (assault); one in mode 0x0a (like) or 0x0c (wander) switches to 0x1b (retreat). A class
  with flag 0x40 would divide (a copy, `CloneObject`, on a free square within 1, 8 tries:
  "<name> divides!"), but no class has the flag;
- a party member's hit points are redrawn.

A breakable object (`IsBreakableType`: closed door 0xb3, glow worm 0x184, devil pod 0x133,
devil plant 0x12c, glass pitcher 0xd9) with a quantity above 0 uses its quantity as hit
points. It is hurt by kinds 0 and 3, and by every kind if it is a plant
(`IsPlantType`: the devil pod and devil plant). If it survives: the devil pod splits open
("The pod splits open!": a pod devil 0x180 appears, `EGG_generate`, and the pod is
deleted); a glow worm gets frame 0 and quality 11-18 (sound 0x22); the quantity drops.
If not: a glass pitcher becomes a broken crystal 0xda (sound 0x1a; in dream stage 0x44,
breaking the last one in (0x37-0x47, 0x37-0x47) sets TalkFlags[0x54] & 0x40); anything
else is deleted (sound 0).

#### Wound messages

After a damaging blow that leaves the defender alive, `COMBAT_displaySeverity` (combat.c)
computes `severity = HP * 4 / MaxHP` (integer):

| Severity | Message |
|---|---|
| 0 | "<Name> critical!" |
| 1 | "<Name> heavily wounded." |
| 2 | "<Name> lightly wounded." |
| 3 | "<Name> barely wounded." |
| 4 or more | nothing |

"damaged." replaces "wounded." for a class without flag 0x4000 or 0x80 (machines, and
actors without a class, through `MonsterFlags[-1]` = 0x1000). Nothing is printed for a
rockworm base 0x172 or at 0 HP. A critical actor:

- the giant maw 0x175 sets `CombatOutcome` to 1 (in combat);
- an actor in mode 0x0d (a rockworm) switches to 0x11 (retreat into the ground), with the
  actor in its quality (its other part);
- otherwise it flees (mode 7) if its mode is none of 0, 1, 2, 6, 0x0e, 0x11, 0x19, its
  class is 0 or has flag 0x80 or 0x4000, its INT (`GetInt`) is 5 or more and it is not
  the monster footprints 0x91. Only `NPCMode` changes, so Retreat later restores
  `NPCComMode`.

#### Bleeding

`COMBAT_Bleed` (combat.c), for a class with flag 0x2000, at a blow (kind 0, chance above)
and at death: the blood type depends on the class flags, 0x4000 alone blood 0x14e, 0x80
alone sap 0x150, both ichor 0x14f; with neither, none. A blood object already on the
square (0x14e-0x150) is reused (moved to the top, or re-placed at death), else a new one
is made; it gets the type and a random frame 0-2. None on rail track tiles
(`IsTrackTile`) or on a square holding a rail object 0x19c-0x19f or 0x1a3. The giant maw
bleeds on its four squares when it dies.

### Death and bodies

When an actor's hit points reach 0 (`COMBAT_Kill`, combat.c):

- NPC 0 (the dreamer): its inventory is deleted (except on level 3), it is marked dead and
  `CheckActiveMember` runs; nothing else (see *The dream*).
- The giant maw 0x175 takes along every living tentacle 0x16a/0xfe (actor slots 0xc0-0xff)
  whose quality is the maw: each is removed (moved to level 6, slot freed) and adds the
  tentacle class's experience (whatever its alignment). On level 3 the maw's death instead
  removes the egg 0x1d2 at (0x87, 0x17), places a bridge 0xe0 (frame 3) at (0x7f, 0x18)
  if none is there ("Aargh!" if either fails), sets TalkFlags[0x66] & 0x22, starts NPC
  0x66's conversation, leaves the maw's body and returns no experience (see *The dream*).
- A rockworm base 0x172 with a living head 0x174 in its quality kills the head instead;
  a head kills its base with it (the base's body is made, the base moved to level 6).
- Hit points 0, marked dead. The unwinnable-game rules and plot flags (below) apply.
- If the actor is hostile (bit 0x20): experience (below); in the dream, the count of foes
  left (`DreamFoesLeft`, not for tentacles 0x16a or pod devils 0x180) and the stage
  endings 0x20, 0xa1, 0xc0 (see *The dream*).
- Made visible. Unless it is a rockworm base or a conversation is running: "<Name>
  falls!" for NPCs 0-0x0f; for the others "<Name> killed!" if the class has flag 0x4000
  or 0x80, else "<Name> destroyed!".
- Its body shape (`SetTypeUnconscious`) and AI mode 0. A dead player-controlled actor
  other than `Party[0]` leaves the party list. With a body shape, `OBJ_MakeDeadBody`
  makes the body: a map object of the actor's (body) shape holding its inventory, quality
  the actor's number, and the actor goes to level 6. Without one, the actor is moved to
  level 6 and, if it is a temporary slot (0xc0-0xff), freed.
- A class with flag 0x800 explodes (`COMBAT_Explode`, see *Special creatures*).
- If it was the active member, `CheckActiveMember` (the party's death and waking are in
  *Conversations and quest state*).

`KillX`, `KillY` and `KillBody` keep where the last actor died and its body. `KillBody` is
not set on the dreamer's death path, so a later gulp could see a stale value (uncertain
whether reachable).

**Body shapes** (`SetTypeUnconscious`, also used for sleep):

| Live type | Body (asleep or dead) |
|---|---|
| human figures 0x156-0x165 | body 0x155, frame = type - 0x156; asleep on a cot 0x8d: sleeping person 0x8e |
| 0x169-0x171 (plantellope, tentacle, ammonoid, proto martian, hedgehog, cave worm, janitor, planther, martian) | dead plantellope 0x151, frame = type - 0x169 |
| sextelleger 0x167, bushalo 0x168 | dead sextelleger 0x152 (frame from type and facing) |
| agrobot 0x178 | 0x153 (no LOOK name), frame 3 |
| rockworm head 0x174 | rockworm 0x9f, frame / 2 |
| rockworm base 0x172 | rockworm casing 0x154 (even without a class test) |
| tree 0x198 | tree stump 0xfd |
| other types with a class | keep their own shape |
| a class with flag 0x800 or 0x1000, or a type without a class | no body: the actor vanishes |

#### Games that can no longer be won

`COMBAT_Kill` sets `IsArmageddon`; while it is set, saving is refused with a message
(1: "You have killed someone whose help you need to win the game!", 2: "One of the
important historical figures you were here to rescue is dead!") (`GameLoop`); restoring
a game clears it (`GAME_Restore`). The tests run in this order:

| Killed | Effect |
|---|---|
| NPC 0x3b (Legrande) or 0x36 (Nathaniel) | IsArmageddon 1; TalkFlags[0x3b] & 2; NPCs 0x33-0x3e get alignment bit 0x20 (good ones become chaotic, neutral ones evil) and mode 8 (assault) |
| NPC 0x12 (Blood) or 0x1a (Yellin) | IsArmageddon 1 and the message at once |
| NPC 0x20 (Tekapesh) | IsArmageddon 1 |
| NPCs 0x0a-0x0f, 0x10-0x1c, 0x22-0x34, 0x37-0x3a, 0x3d, 0x3e, 0x42 (Goldman), 0x46 (Rasputin, second script) | IsArmageddon 2 |

(The 0x10-0x1c range includes Tesla 0x10, Freud 0x11, Garrett 0x13, Cooter 0x18, Duprey
0x19, Calamity Jane 0x1b and Buffalo Bill 0x1c; 0x22-0x34 the Martians 0x22-0x28, the
second scripts 0x2a-0x31 of Lowell, Melies, Wells, Earp, Tiffany, Lenin, Carver and
Clemens, Segal 0x33 and Hearst 0x34; 0x37-0x3a Curie, Roosevelt, Edison, Carnegie;
0x3d-0x3e Peary and Bernhardt.) Killing NPC 0x5b (Clanton) sets TalkFlags[0x5b] & 0x10.

#### Invulnerable NPCs

`IsInvulnerable` (combat.c): actors whose original type (`OrigShapeType`) is the dust
devil 0x17d, your mother 0x187, the shadowlord 0x186, or a mechanical man 0x166/0x17e, and
the NPCs 0x5c (Marshall), 0x5d (Smith), 0x59 (Pistol), 0x5a (Auctioneer), 0x54
(Tiffany), 0x52 (Wells), 0x6d (Spector, second script), 0x68 (Tarpch), 0x67 (Shartek),
0x69 (Duprey, second script), 0x77 (Shikach), 0x78 (Basichk), 0x40 (Rasputin) and 0x3c
(Jean). They cannot be paralysed by a creature's blow and take no terrain or rampage
damage. A blow that gets through armour (`LoseHP`) shows the hit flash, sets their hit
points to 255 and does this instead:

| Who | Reaction |
|---|---|
| NPC 0x69 | "<Duprey (NPC 0x19)'s name> critical!" |
| NPC 0x3c | "<name> critical!" |
| NPC 0x52 | "<name> heavily wounded." |
| NPC 0x68 | "<name> critical!" if TalkFlags[0x68] & 4, else "heavily wounded."; then TalkFlags[0x68] & 0x20 and his conversation starts |
| the others | "It has no effect." |
| original type 0x187 (in addition) | "How dare you, young man!" ("woman" for a female player), then three blows on NPC 0 (`__DoHit` with 45 each) |
| NPC 0x6d or 0x67 (in addition) | TalkFlags[0x6b] & 8 and the party is moved to the dreamer's waking place (`WakeX`, `WakeY`) |
| NPC 0x40 (in addition) | Rasputin's throw (`COMBAT_RasputinThrow`) |

**Rasputin's throw**: TalkFlags[0x40] & 8 and his conversation with NPC 1; then the
player is hit by a missile (tile 0x101) from Rasputin, drawn as 0x1a8, and flies (tile
0x18e or 0x18a) along the line from Rasputin through the player (always southward: a
northward or level step is made 1) until at least 10 squares south of Rasputin; he lands
one square short, unconscious, and is killed (`COMBAT_Kill(1)`), so the party is knocked
out. `CancelSpray` stops the attacking weapon's spray.

### Experience

Only a kill made through `__DoHit` gives experience, to the attacker (`AddXP` of
`LoseHP`'s result): weapon blows, natural attacks, spray hits, the M60, the 0x187's and
the mode 0x15 blows. Explosions, rampaging, terrain, poison and the other callers drop the
value.

A killed actor gives experience only if its alignment has bit 0x20 (evil or chaotic) when
it dies and its type has a class. With the class's table values (not the individual's
rolled stats):

    xp = ( A * Damage + (HP + Armor) * DEX ) / 16      (integer, shift right 4)
    A  = STR if the class has flag 0x20, else DEX

For the giant maw the tentacles it takes along add their class's `A * Damage + (HP +
Armor) * DEX` before the shift (`COMBAT_Kill`). The XP column of the class table gives the
value. A neutral creature turns evil when a good actor hits it (below), so provoked
creatures give experience.

### Paralysis

A creature whose class has flag 4 (only the sand trapper 0x182) paralyses one time in 4
on a blow that leaves the defender alive, invulnerable defenders excepted (`__DoHit`);
the freeze ray gun paralyses living classes (see *Spraying weapons*). Paralysis is
NPCStatus bit 2; how it wears off is in *Time, rest, stats and levels*.

### Provoking neutrals

`COMBAT_ProvokeNeutral` (combat.c) runs when the defender of a blow is the selected object
and is a neutral actor (alignment 0), not player-controlled, in a mode up to 0x10 or of
0x80 and above:

- If the attacker is in command mode (the player), the defender's class has flag 0x4000
  without 0x80 and the blow did damage: "<Name> attacks!".
- A named NPC (below 0xc0) on its schedule (mode 0x80 or above): if the player attacked,
  it switches to mode 0xa0 (assault); its alignment stays neutral.
- Otherwise, if the attacker is good, the defender becomes evil (0x20; SE made it
  chaotic). A temporary creature (0xc0 and up) switches to mode 8 (assault).

### Monster classes

`MONSDATA.ASM` holds `MonsterTypes` (51 words: an unused 0, 49 creature types, a 0
terminator), seven byte tables (`MonsterStr`, `MonsterDex`, `MonsterInt`, `MonsterArmor`,
`MonsterDamage`, `MonsterHP`: 50 bytes each; `MonsterAlignment`: 52) and the word table
`MonsterFlags` (49 words). `GetMonsterClass(type)` (egg.c) returns the type's position in
`MonsterTypes` minus 1 (-1 if absent; the last answer is cached). Every use reads the byte
tables at `class + 1` and `MonsterFlags` at `class` (combat.c, egg.c, canmove.c,
gameloop.c, dreamobj.c). That is consistent: the byte tables start with an unused entry
like `MonsterTypes`, `MonsterFlags` does not, so each creature gets its own row
everywhere (the human figures' flags 0x6000 end exactly at class 14 and the metal woman,
class 15, has 0). The two extra bytes of `MonsterAlignment` (0, 0x10) are never read as
alignments, but they form the word `MonsterFlags[-1]` = 0x1000, which the code reads for
actors without a class in the places that do not test for -1 (the "killed/destroyed" and
"wounded/damaged" choice, the flee test, explosions, paralysis, the sprayers).

Alignment (NPCStatus bits 5-6): 0x00 neutral, 0x20 evil (hostile to the party), 0x40 good,
0x60 chaotic. Range is the natural attack's (`GetWeaponRange`). XP is the formula of
*Experience*; in parentheses for classes that give it only if they have turned hostile by
the time they die.

| Class | Type | LOOK name | STR | DEX | INT | Armour | Damage | Range | HP | Alignment | Flags | XP |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | 0x157 | yourself | 25 | 25 | 25 | 0 | 6 | 1 | 90 | good | 0x6000 bleeds, animal | (150) |
| 1 | 0x158 | yourself | 25 | 25 | 25 | 0 | 6 | 1 | 90 | good | 0x6000 bleeds, animal | (150) |
| 2 | 0x156 | poor monk | 22 | 22 | 22 | 0 | 6 | 1 | 60 | evil | 0x6000 bleeds, animal | 90 |
| 3 | 0x159 | scientist | 15 | 15 | 25 | 0 | 6 | 1 | 30 | good | 0x6000 bleeds, animal | (33) |
| 4 | 0x15a | young woman | 18 | 18 | 22 | 0 | 6 | 1 | 60 | good | 0x6000 bleeds, animal | (74) |
| 5 | 0x15b | cowboy | 20 | 25 | 18 | 0 | 6 | 1 | 60 | good | 0x6000 bleeds, animal | (103) |
| 6 | 0x15c | gentleman | 25 | 25 | 25 | 0 | 6 | 1 | 90 | good | 0x6000 bleeds, animal | (150) |
| 7 | 0x15d | uniformed man | 25 | 25 | 25 | 0 | 6 | 1 | 90 | good | 0x6000 bleeds, animal | (150) |
| 8 | 0x15e | adventurer | 25 | 25 | 25 | 0 | 6 | 1 | 90 | good | 0x6000 bleeds, animal | (150) |
| 9 | 0x15f | common fellow | 25 | 25 | 25 | 0 | 6 | 1 | 90 | good | 0x6000 bleeds, animal | (150) |
| 10 | 0x160 | man | 25 | 25 | 25 | 0 | 6 | 1 | 90 | good | 0x6000 bleeds, animal | (150) |
| 11 | 0x161 | man in white | 25 | 25 | 25 | 0 | 6 | 1 | 90 | good | 0x6000 bleeds, animal | (150) |
| 12 | 0x162 | lady | 25 | 25 | 25 | 0 | 6 | 1 | 90 | good | 0x6000 bleeds, animal | (150) |
| 13 | 0x163 | woman | 25 | 25 | 25 | 0 | 6 | 1 | 90 | good | 0x6000 bleeds, animal | (150) |
| 14 | 0x164 | adventuress | 25 | 25 | 25 | 0 | 6 | 1 | 90 | good | 0x6000 bleeds, animal | (150) |
| 15 | 0x165 | metal woman | 25 | 25 | 25 | 0 | 10 | 1 | 90 | good | 0x0000 - | (156) |
| 16 | 0x166 | mechanical man | 25 | 15 | 10 | 4 | 12 | 1 | 40 | good | 0x0000 - | (52) |
| 17 | 0x171 | martian | 15 | 15 | 20 | 0 | 6 | 1 | 30 | neutral | 0x2080 plant, bleeds | (33) |
| 18 | 0x167 | sextelleger | 20 | 20 | 10 | 4 | 35 | 1 | 45 | chaotic | 0x2080 plant, bleeds | 105 |
| 19 | 0x168 | bushalo | 25 | 12 | 11 | 8 | 20 | 1 | 35 | neutral | 0x20a0 STR, plant, bleeds | (63) |
| 20 | 0x169 | plantellope | 15 | 22 | 9 | 1 | 12 | 1 | 20 | neutral | 0x2080 plant, bleeds | (45) |
| 21 | 0x174 | rockworm | 20 | 15 | 10 | 8 | 25 | 1 | 40 | chaotic | 0x6080 plant, bleeds, animal | 68 |
| 22 | 0x179 | wisp | 20 | 20 | 20 | 3 | 30 | 1 | 80 | neutral | 0x5400 flies, no body, animal | (141) |
| 23 | 0x17a | airsquid | 10 | 20 | 15 | 0 | 6 | 1 | 10 | evil | 0x0980 plant, anywhere, explodes | 20 |
| 24 | 0x17c | airsquid | 15 | 15 | 8 | 4 | 20 | 1 | 30 | evil | 0x3080 plant, no body, bleeds | 50 |
| 25 | 0x175 | giant maw | 30 | 10 | 9 | 3 | 30 | 1 | 100 | chaotic | 0xe080 plant, bleeds, animal, dig tiles | 83 |
| 26 | 0x16a | tentacle | 20 | 20 | 9 | 2 | 20 | 1 | 10 | chaotic | 0xe0a0 STR, plant, bleeds, animal, dig tiles | 40 |
| 27 | 0x17f | creeping cactus | 20 | 12 | 3 | 4 | 25 | 1 | 30 | chaotic | 0x3080 plant, no body, bleeds | 44 |
| 28 | 0x16b | ammonoid | 15 | 15 | 3 | 10 | 12 | 1 | 20 | chaotic | 0x2080 plant, bleeds | 39 |
| 29 | 0x16c | proto martian | 20 | 19 | 10 | 2 | 10 | 3 | 20 | evil | 0x20a0 STR, plant, bleeds | 38 |
| 30 | 0x91 | monster footprints | 20 | 19 | 10 | 2 | 12 | 1 | 20 | evil | 0xa0a0 STR, plant, bleeds, dig tiles | 41 |
| 31 | 0x180 | pod devil | 15 | 18 | 3 | 4 | 12 | 2 | 25 | chaotic | 0x3080 plant, no body, bleeds | 46 |
| 32 | 0x181 | jumping bean | 10 | 22 | 10 | 1 | 4 | 1 | 10 | chaotic | 0x3080 plant, no body, bleeds | 20 |
| 33 | 0x16d | hedgehog | 17 | 12 | 8 | 3 | 20 | 1 | 20 | chaotic | 0x2080 plant, bleeds | 32 |
| 34 | 0x176 | creeper | 15 | 15 | 3 | 1 | 16 | 1 | 20 | chaotic | 0x3080 plant, no body, bleeds | 34 |
| 35 | 0x178 | agrobot | 29 | 18 | 10 | 10 | 20 | 1 | 30 | chaotic | 0x0020 STR | 81 |
| 36 | 0x17b | canal worm | 20 | 20 | 15 | 5 | 25 | 1 | 30 | chaotic | 0x1280 plant, swims, no body | 75 |
| 37 | 0x17d | dust devil | 30 | 30 | 30 | 99 | 60 | 1 | 255 | neutral | 0x9000 no body, dig tiles | (776) |
| 38 | 0x182 | sand trapper | 25 | 25 | 10 | 4 | 18 | 4 | 40 | chaotic | 0xb084 paralyses, plant, no body, bleeds, dig tiles | 96 |
| 39 | 0x177 | oxy leech | 10 | 25 | 12 | 1 | 10 | 1 | 20 | chaotic | 0x7080 plant, no body, bleeds, animal | 48 |
| 40 | 0x16e | cave worm | 15 | 15 | 15 | 2 | 15 | 3 | 30 | evil | 0x6080 plant, bleeds, animal | 44 |
| 41 | 0x16f | janitor | 20 | 15 | 10 | 10 | 20 | 1 | 30 | chaotic | 0x0020 STR | 62 |
| 42 | 0x183 | bushrat | 10 | 25 | 10 | 2 | 12 | 1 | 20 | evil | 0x3080 plant, no body, bleeds | 53 |
| 43 | 0x170 | planther | 15 | 22 | 12 | 4 | 20 | 1 | 20 | chaotic | 0x2080 plant, bleeds | 60 |
| 44 | 0x18e | minotaur | 20 | 18 | 8 | 2 | 15 | 1 | 90 | neutral | 0x7020 STR, no body, bleeds, animal | (122) |
| 45 | 0x187 | your mother | 12 | 20 | 15 | 9 | 1 | 1 | 255 | chaotic | 0x5020 STR, no body, animal | 330 |
| 46 | 0x198 | tree | 25 | 18 | 5 | 3 | 12 | 1 | 90 | neutral | 0x8080 plant, dig tiles | (118) |
| 47 | 0x1a8 | (no LOOK name) | 25 | 18 | 5 | 3 | 12 | 1 | 90 | neutral | 0x8080 plant, dig tiles | (118) |
| 48 | 0x193 | huge red spot | 20 | 20 | 10 | 99 | 12 | 1 | 255 | evil | 0x8000 dig tiles | 457 |

The dust devil and your mother are invulnerable, so their XP is never earned. The three
classes with 255 HP get from their egg a value of 128-382 in a one-byte field, so it can
wrap (see *Eggs and spawning*).

`GetDamagePoints` (objtype.c) reads `MonsterDamage[class]`, one entry early, for an
actor, but no caller passes it an actor: the Attack command, `__PickWeapon` and
`COMBAT_PickRangedWeapon` pass inventory objects, and a natural attack's damage is read
by `__TryHit` itself at `class + 1`. So that slip has no effect.

#### Monster flags

| Bit | Meaning | Where | Classes |
|---|---|---|---|
| 0x0004 | the creature's blows paralyse 1 time in 4 | `__DoHit` | sand trapper |
| 0x0008 | immune to burning (kind 3) | `LoseHP` | none |
| 0x0010 | meant to double burning damage (the result is dropped) | `LoseHP` | none |
| 0x0020 | the natural attack's to-hit uses STR; STR in the XP formula | `__AttackTest`, `COMBAT_Kill` | see the table |
| 0x0040 | divides when hurt | `LoseHP` | none |
| 0x0080 | plant life: "killed", "wounded"; sap (alone) or ichor (with 0x4000); hurt by the freeze ray gun and the weed sprayer when without 0x4000; can flee when critical; burnt by the fires of dreamobj.c | combat.c, dreamobj.c | most creatures |
| 0x0100 | moves anywhere but walls | canmove.c | airsquid 0x17a |
| 0x0200 | swims | canmove.c | canal worm |
| 0x0400 | flies (0x100 with 0x400 would also skip terrain effects; no class has both) | canmove.c, `ApplyTerrainEffects` | wisp |
| 0x0800 | explodes when it dies; no body | `COMBAT_Kill`, `SetTypeUnconscious` | airsquid 0x17a |
| 0x1000 | no body; the map is redrawn (`RedrawMapWithObjects(1)`) when it dies within 5 squares of the screen centre | `SetTypeUnconscious` | see the table |
| 0x2000 | bleeds | `COMBAT_Bleed` | see the table |
| 0x4000 | animal life: "killed", "wounded"; blood (alone) or ichor; paralysed by the freeze ray gun; can flee when critical; "<Name> attacks!" when provoked (without 0x80) | combat.c | humans, rockworm, wisp, giant maw, tentacle, oxy leech, cave worm, minotaur, your mother |
| 0x8000 | walks only on tiles with a dig class (`GetDigClass`) | canmove.c | see the table |

Bits 0x0001, 0x0002, 0x0008, 0x0010 and 0x0040 are set in no class. The names "plant"
and "animal" are an interpretation from the blood colours and the weed sprayer; the code
only tests the bits. The movement rules are in *The map and machinery*.

The modes the saved game's eggs give each creature (embryo qualities in
`gamedata/MARTIAN/SAVEGAME/OBJBLK*`; data, not code): sextelleger 8; bushalo 8, 9, 0x0b,
0x0c; plantellope 9, 0x0c; tentacle 0x0e; ammonoid 6, 8; proto martian 8, 0x0b; hedgehog
6, 8, 0x0b, 0x0c; cave worm 8; janitor 6, 8; planther 6, 8; rockworm base 0x172 0x0d;
giant maw 0x0e, 0x12; creeper 8; oxy leech 0x14 (the oxium thief); agrobot 8, 9, 0x0b;
wisp 0x0c; airsquid 0x17a 0x0b; canal worm 8; dust devil 0x13; creeping cactus 8; pod
devil 8; jumping bean 6, 8; sand trapper 0x12; bushrat 6, 8, 0x1b; tree 0x198 0, 2, 0x17
(and 0x1a, made as an object).

### Special creatures

**Rockworms** (modes 0x0d and 0x11; `COMBAT_AI_Worm`, `COMBAT_AI_WormFlee`,
`COMBAT_WormBackUp`). A rockworm is a base 0x172 (rockworm base, under the ground) holding
its head's number in its quality, a head 0x174 (rockworm) holding the base's, and tail
segments 0x173 (rockworm) holding the head's. Attacking the base or a segment attacks the
head (`COMBAT_GetTargetAt`, `COMBAT_getHead`).

- An egg makes only one base per egg square and runs its A.I. at once.
- Either part dies quietly (body made, moved to level 6) when its partner is dead or
  gone.
- The base never attacks. With no head and its enemy farther than 1, it raises a head
  (`EGG_generate(0x174)`, mode 0x0d) on the neighbouring square toward the enemy
  (diagonals rounded to the straight direction), or on the first free one turning 90
  degrees at a time; with no room, head and base are deleted (`DreamFoesLeft` counted
  down). The base then spends all its points.
- The head: farther than 8 squares from its base (or the base out of the loaded area), it
  backs up. Next to its enemy (distance below 2) it strikes with its natural attack (10
  points). Otherwise it steps toward the enemy in a straight direction (for a diagonal,
  first the straight direction just anticlockwise of it, then the next one clockwise:
  north-east tries north, then east), leaving a segment 0x173 on its old square
  (straight or bent, by the turn). It does not step onto its own segment (it backs up
  instead), nor in the direction it just backed out of (its quantity remembers it); when
  blocked, one time in 2 it backs up; 5 points.
- Backing up (`COMBAT_WormBackUp`): the head moves onto its last segment, deletes it and
  faces the way the segment pointed. Without a segment behind it, in mode 0x11 the worm
  dies: if the base stands there the head vanishes and the base is killed (`COMBAT_Kill`,
  no experience, a rockworm casing 0x154 left); if the actor found there is not the base,
  "Worm error!".
- A critical rockworm switches both parts to mode 0x11 (`COMBAT_displaySeverity`): the
  base waits, the head retreats into the ground and the worm dies there.

**The giant maw** 0x175 and its **tentacles** 0x16a (modes 0x12, 0x0e, 0x19). Hidden in
the sand (mode 0x12, invisible), the maw and the sand trapper 0x182 wait (2 points) until
an enemy is next to them (`COMBAT_AI_HiddenInSand`): "Suddenly, something bursts up
through the sand!", sound 0x21, combat begins (`COMBAT_beginAttacked`), the creature
becomes visible in mode 0x0e (immobile) and acts at once. The maw also brings up
`OSI_rand(0, 10) + OSI_rand(0, 10) + 10` tentacles (10-30): each is made with
`EGG_generate`, gets frame 3, mode 0x19 and the maw's number in its quality, and is
placed on a random free square within 6 of the maw (8 tries, else deleted). The placement
also accepts a square whose first object number equals 0x16a, which looks like a slip
for a type test. An immobile maw or sand trapper whose enemy is 8 or more away hides
again (invisible, mode 0x12). The maw is a 2 x 2 shape (see *Range and line of fire*).
Tentacles (`COMBAT_AI_Tentacle`):

- an enemy in range and in line of fire: a hidden tentacle shows itself (frame 3 or 7),
  then strikes (10 points);
- out of range and visible (5 points): when weak (hit points below a quarter of its Max
  HP) or one time in 2 it sinks (shape 0xfe, tentacle) and becomes invisible, or is
  deleted if weak;
- out of range and hidden: deleted if more than 8 from its maw; else it moves toward the
  enemy and shows itself when `OSI_rand(0, 2 * distance) <= 1`.

With no enemy the code reads the position of object -1 (`GetX(-1)`), so what a tentacle
or a rockworm head does then is not defined. A missile does not count a hidden
(invisible) 0x175, 0x16a, 0xfe or 0x182 among the actors it crossed (`Missiles`), so
sprays, and explosions away from their centre square, pass them by. The maw's death
takes its tentacles along (see *Death and bodies*).

**The canal worm** 0x17b: when its blow kills an actor that leaves a body,
`COMBAT_CanalWormGulp` prints "Gulp!" (sound 0x24), puts the body's contents where the
actor died (deleting those whose tile has `TileFlag2` bit 8), deletes the body and, on
screen, shows the worm gulping (frames 0x23-0x2f).

**Exploding creatures** (class flag 0x800, the airsquid 0x17a): on death, `COMBAT_Explode`
plays sound 0xb and makes an explosion (missile tile 0x107, the 3 x 3 square, `Explosion`
in gfx.c); one roll of 1d15 hurts everything it reaches (`LoseHP`, kind 0, no
experience), other airsquids last (at most 5) and only if still alive, so they can blow up
in turn.

**The angry airsquid**: an airsquid 0x17a in mode 0x0b that turns hostile (see
*Unfriendly*) becomes a 0x17c (airsquid) of frame 0 with STR +5, DEX -5, movement points
-5, INT -7 and hit points +20 (bytes, so a DEX under 5 wraps).

**Dividing creatures** (class flag 0x40): the code exists in `LoseHP` but no class has
the flag.

**Creeping cacti** 0x17f: whenever one is made, moved or deleted, every living cactus
takes a frame showing which of its four neighbours are cacti, so they join up
(`OBJ_JoinCreepingCacti`, object.c). In combat they are ordinary creatures.

**The oxium thief** (mode 0x14, the oxy leech 0x177 in the eggs; `COMBAT_AI_Thief`): next
to its enemy (in either direction of `COMBAT_RangeToObj`), if the enemy carries oxium
0x83 (anywhere but readied) the thief takes the stack ("Oxium stolen!", sound 0x24, 10
points): the stack moves into the thief (`InsertObj`) and its quantity is then set to
`OSI_rand(0, q)`, q its quantity, so the rest is lost; then, when `OSI_rand(0, 50) <= q`,
it flees for good (mode and combat mode 0x1b). If the enemy has none, another party member
next to the thief who has some is robbed instead; failing that the thief strikes the enemy
(10 points). Not next to it: it moves toward it (5 points).

**Mode 0x15** (`COMBAT_AI_MoveSouth`, given to your mother 0x187 in dream stage 0x25):
it steps south; when it cannot, it hits an enemy within 1 through `__DoHit` with 6
points, kind 0 (no to-hit test); 3 points.

**Mode 0x17** (`COMBAT_AI_Together`): a tree 0x198 and an object 0x1a8 move about together
on one square: a 0x198 that finds a 0x1a8 on its square is deleted; otherwise it becomes
a 0x1a8 itself and a tree object 0x198 of frame 3 is made on its square ("Aargh!" if it
cannot). Each turn the 0x1a8 steps in a random direction and the tree is moved with it
(5 points).

**The weapon 0x139 and NPC 0x66**: see *Special weapons*.

### Combat A.I.

`NPC_TakeTurn` (npctrack.c) runs an actor's A.I. by its `NPCMode`; a wild actor (ObjStatus
0x80) always uses Berserk. Party members in CMND mode are not run: the scheduler hands
them to the player (`RunNPCTurns`).

| Mode | Routine (combat.c unless noted) |
|---|---|
| 0x04 | `COMBAT_AI_Range` |
| 0x06 | `COMBAT_AI_Berserk` |
| 0x08, 0xa0 | `COMBAT_AI_Assault` |
| 0x07, 0x09, 0x1b, 0x9f | `COMBAT_AI_Retreat` |
| 0x0a | `COMBAT_AI_Like` |
| 0x0b | `COMBAT_AI_Unfriendly` |
| 0x0c, 0x8f | `WanderRandomly` (npctrack.c) |
| 0x0d, 0x11 | `COMBAT_AI_Worm`, `COMBAT_AI_WormFlee` (rockworms) |
| 0x0e | `COMBAT_AI_Immobile` |
| 0x12, 0x19 | `COMBAT_AI_HiddenInSand`, `COMBAT_AI_Tentacle` |
| 0x13 | `COMBAT_AI_Rampage` |
| 0x14 | `COMBAT_AI_Thief` |
| 0x15 | `COMBAT_AI_MoveSouth` |
| 0x16 | `COMBAT_AI_ChaseCape` (the dream) |
| 0x17 | `COMBAT_AI_Together` |
| 0x18 | `AI_SeekSeed` (dreamobj.c, the dream) |
| 0x1a | `NPC_BecomeObject` (npctrack.c): the actor becomes a map object of its shape, its quality and its hit points as quantity |
| 0x98 | brawl: one time in 4, go for an opponent within 4 squares of its schedule spot and "hit" it (a hit flash only, no damage), then go back |
| 0x0f, 0x10, 0x8b-0x8e | nothing (so the 10-point charge applies) |

The other modes (0x80 and up: schedules, paths, conversation, the coal workers 0x99-0x9e)
are not combat; see *Time, rest, stats and levels* and *The map and machinery*. A mode
with no handler at all costs 5.

#### Choosing an opponent

`COMBAT_pickOpponent` (combat.c) takes the closest actor (squared straight-line distance)
that is on the current level (the chooser too), not dead, not itself, within 11 squares
on both axes, visible to it (`COMBAT_canSee`: not invisible unless both are
player-controlled, not dragged under, not on an "ignore" tile, same level), not a dust
devil 0x17d, not a fleeing actor (mode 7) more than 5 squares from the screen centre, and
an enemy by alignment:

| Chooser | Attacks |
|---|---|
| good (0x40) | evil or chaotic (bit 0x20) |
| evil (0x20) | good or chaotic (bit 0x40) |
| chaotic (0x60) | anyone except its own type; a rockworm head spares rockworm segments and bases; the giant maw, tentacles 0x16a and sunk tentacles 0xfe spare each other |
| neutral (0x00) | anyone not neutral |

On a tie in distance the one whose class has the higher `MonsterDamage` wins.

#### Choosing a weapon

`__PickWeapon` (combat.c): the readied weapon with the highest `GetDamagePoints` whose
range reaches the opponent, else the natural attack (any readied weapon with damage
above 0 beats it, whatever the natural attack's damage). It skips sprayers the way
*Special weapons* and *Spraying weapons* describe and sets the combine's and ray guns'
quality. It does not check ammunition, so an attacker out of ammunition can keep choosing
its gun and waste its turns. `COMBAT_PickRangedWeapon` (for Range mode) looks only at
readied weapons of range 3 or more: the longest range until one reaches the opponent,
then the highest damage among those that do; -1 if none.

#### The modes

When an attack is made (`__TryHit` and 10 points) the routine ends there; SE's routines
moved afterwards too.

- **Assault** (ATTK, 8, and 0xa0; `COMBAT_AI_Assault`): pick the opponent and the weapon.
  If the opponent is within the weapon's range and adjacent, or the test missile reaches
  it, attack. If the line of fire is blocked, move toward its row or column; out of
  range, move toward it. No opponent: wander.
- **Range** (RANGE, 4; `COMBAT_AI_Range`): if the opponent is within 2 squares on both
  axes, step away; the code passes `(x - opponentX, y - opponentY)`, a difference, as
  the destination, so the step goes toward a point near the map's origin rather than
  reliably away (as in SE). With no readied weapon of range 3 or more: "<Name> has no
  ranged weapons!" (each turn) and nothing else. If more than 5 squares (x) or 4 (y)
  from the screen centre, walk back toward it. Otherwise: in range and in line of fire,
  shoot; blocked, move toward the opponent's row or column twice (two `TryMoveTo`
  calls); out of range, approach. No opponent: wander.
- **Berserk** (WILD, 6, and wild actors; `COMBAT_AI_Berserk`): the target is the visible
  living actor within 8 squares with the highest effective STR + DEX + INT. Good attacks
  only exactly evil (0x20), evil only exactly good (0x40), chaotic anyone but its own
  kind (the exceptions of *Choosing an opponent*, so neutral and other chaotic actors
  too), neutral anyone. In range and in line of fire: attack. Otherwise move toward it
  (or toward its row or column); if that move fails, act as Assault. No target: Assault.
  It costs 5 more points, except after an attack of its own.
- **Retreat** (FLEE, 7, and 9, 0x1b, 0x9f; `COMBAT_AI_Retreat`): more than 7 squares
  from the screen centre it stops (points set to 0). With no opponent it wanders and
  nothing else. With one within 7 squares on both axes it moves straight away; farther,
  good actors go toward the screen centre (the party) and the others wander. Then an
  actor not under player control, one time in 4, regains 1 hit point (up to Max HP) and,
  if `HP * 4 / MaxHP` is at least 1 (no longer critical), returns to its combat mode. So
  a creature whose combat mode is itself a retreat mode (9, 0x1b) keeps fleeing; one in
  mode 9 turns to Assault when hurt (`LoseHP`).
- **Like** (0x0a; `COMBAT_AI_Like`): within 2 squares of a party member, one time in 2
  it moves toward the screen centre. Otherwise one time in 2 it walks toward a point 10
  squares off in a random straight direction, else it waits (5). Because of a precedence
  slip, `x = (x + OSI_rand(0, 1)?10:-10) & 0x3ff`, the point is column (or row) 10 of the
  map (0x3f6 if the sum is 0), not 10 squares away. Hurt, it switches to Retreat (0x1b).
- **Unfriendly** (0x0b; `COMBAT_AI_Unfriendly`): within 2 squares of a party member, one
  time in 6 (`OSI_rand(0, 5) == 0`) it turns hostile (alignment bit 0x20 added, mode 8),
  an airsquid 0x17a becomes the angry 0x17c, and it attacks at once (Assault). Otherwise
  as Like without approaching (with the same slip). Hurt, it switches to Assault.
- **Immobile** (0x0e; `COMBAT_AI_Immobile`): never moves. The giant maw, tentacles and
  sand trapper take a random frame (3 or 7) each turn. A neutral one never attacks.
  Otherwise it picks its opponent and weapon and attacks if in range and adjacent or in
  line of fire (10 points); the maw and sand trapper hide again when the opponent is 8
  or more away (above). Else it waits (5). (SE's version read an uninitialised position;
  MD's picks its opponent properly.)
- **Rampage** (0x13; `COMBAT_AI_Rampage`; the dust devils, and the minotaur 0x18e of
  dream stage 0x44): 5 points; the minotaur in stage 0x44 acts only one time in 2 and
  shakes the screen; it steps in a random direction, then everything else on its square
  that is not invulnerable takes 2d10 (`LoseHP`, kind 0).
- **Chase the cape** (0x16; `COMBAT_AI_ChaseCape`): a dream creature that goes for the red
  cape 0xa2, hurting everything on its way (2d10); see *The dream*.

### Eggs and spawning

An egg is an invisible object of type 0x1d2 (egg.c). Its quantity is the percentage chance
to hatch; its quality's tens digit is when (0 any time, 1 by day, 6-17 h, 2 by night) and
its units digit 1-4 overrides the creatures' alignment (neutral, evil, good, chaotic). The
objects inside it, the embryos, say what to make: the embryo's type, its quantity the
number, its quality the creatures' mode and combat mode (0x1a: a type of 0x156 or more
made as an object), its frame theirs, its ObjStatus bits 2 (invisible) and 0x80 (wild)
theirs.

When eggs hatch (`EGG_hatchArea`): whenever the 40 x 40 loaded area is rebuilt
(`MAP_BuildArea`, map.c), the eggs of the current level in it that are more than 8 squares
(either axis) from the screen centre try to hatch; after a party teleport (`PartyTeleport`)
and some dream and conversation events (`ForceHatching`) all of them, however close. Not
while a game is being loaded (`HatchingDisabled`). An egg tries once: whatever the roll it
is then marked hatched (ObjStatus 0x40) and invisible. The mark is cleared when the egg
lies 20 or more squares from the rebuilt area's centre, or on another level, as the far
temporary objects and creatures are purged (`OBJ_PurgeFarLocals`, object.c), so a
revisited place can hatch again.

`EGG_hatches`:

1. An egg of quality 100 or more is meant to wait for TalkFlags[0x46] & 8 (set by the
   M60's shot at Raxachk). **But its tens digit is then 10 or more, which matches none of
   the time rules, so it never hatches**; nor does any egg of quality 30-99. The saved
   game's eggs have qualities 0, 2, 100 and 104, so 23 of them can never hatch.
2. The hatch roll: `1d100 <= egg quantity` (a quantity of 100 or more always hatches).
3. For each embryo, the number is the embryo's quantity, or `1d(quantity)` if the egg's
   quantity is not exactly 100 (so a quantity of 101 always hatches a random number).
4. Oxygenated air bottles 0x144: if one already lies on the egg's square the whole egg
   gives up without being marked (it will try again); else that many bottles appear there
   (and the enemy warning is suppressed).
5. Other stackable objects: one pile of that many, on a random square within 3 (3 tries:
   not open water, no terrain bit 2 or 4, no object of the type there).
6. Other objects: that many, each on such a square (also without dreamstuff 0x14b); a
   breakable type takes the embryo's quality as its quantity (hit points); a devil plant
   0x12c gets, one time in 2, a devil pod 0x133 of quantity 4-10 beside it.
7. Creatures (`EGG_generate`), each: a rockworm base is not made where one stands on the
   egg's square. The frame, the alignment override, the mode, invisibility (also for mode
   0x12) and wildness are applied. The first one placed stays on the egg if it can stand
   there; the others (and a first one that cannot) are moved to a random square
   `x + 2d4 - 5`, `y + 2d4 - 5` (-3..+3, centre-weighted, 8 tries) that must be more than 5
   squares from the screen centre on some axis unless hatching was forced
   (`COMBAT_TryTeleport`); failing that the creature is deleted (while `DreamFoesLeft` is
   not 0, printing "monster <n> (0x<type>) unplaced." and counting it down; a creature
   that could not be made at all also counts it down). A rockworm base then runs its A.I.
   at once.
8. If a hostile (bit 0x20) creature was made, no warning was given since combat last ended
   (`EnemyWarningGiven`) and the party is not dreaming: "You hear something to the
   <direction>!" with a jingle (`MUS_EventJingle`: song 5 on the surface, 15 underground,
   when not in combat).

`EGG_generate(type, x, y, z)` (egg.c), also used by the rockworms, the maw, the devil pod
and the dream:

- With a class: STR, DEX, INT and HP are `mkRandom(class value)`: with `h = value / 2`,
  `value - h + OSI_rand(0, h) + OSI_rand(0, h)`, so from `value - h` to `value + h`,
  centre-weighted; 1 stays 1. The fields are one byte, so the HP 255 classes can wrap
  (128-382 modulo 256). Level 0, alignment from the class.
- Without a class: STR, DEX and INT 15, HP 30, level 1, chaotic.
- In both cases: experience 0, movement points = DEX, original type set, talk flags 8-15
  cleared, mode and combat mode 8 (the egg then sets them from the embryo). No
  possessions (`EGG_GivePossessions` is empty).

### What could not be determined

- Whether the M60's inverted line-of-fire test (it hits only when its test shot is
  stopped) is a bug or meant for a particular dream scene.
- What object number the tentacle placement's `loc == 0x16a` test was meant to catch.
- Whether a stale `KillBody` after the dreamer's death can reach the canal worm's gulp.
- Whether Origin meant the quality-100 eggs to hatch after the M60 scene (the code
  prevents it).
- The meaning of flags 0x80 and 0x4000 ("plant" and "animal" life) is inferred from what
  the code does with them.
- What a tentacle or a rockworm head does with no opponent (the code reads object -1).

## Objects and commands

This section covers the command bar and the keys a player uses, how a command picks its
target and how far it reaches, the Look, Get, Drop and Move commands, Ready and Unready
with the equipment slots, containers and the inventory grid, the weight and slot checks the
commands apply, stacking, and the object changes made by the object routines (object.c).
The Use command is in *The use command*, Talk in *Conversations and quest state*, Attack in
*Combat*, and what machines do with objects in *The map and machinery*. The general weight
rule is in *Time, rest, stats and levels*; here are only the places where commands refuse.

### The command bar and the keys

The seven icons under the map are commands 0x81 to 0x87, 18 pixels apart from x = 16:

| Code | Icon | Key | Selection range (keyboard cursor) |
|---|---|---|---|
| 0x81 | Attack | A | 7 (the last target is aimed at again if still on screen) |
| 0x82 | Talk | T | 7 |
| 0x83 | Look | L | 7 |
| 0x84 | Get | G | level + 1 with the first talent, else adjacent only |
| 0x85 | Drop | D | an inventory slot (the slot cursor opens on the active member's equipment view) |
| 0x86 | Move | M | level + 1 with the first talent, else adjacent only |
| 0x87 | Use | U | level + 1 with the first talent, else adjacent only |

`GameLoop` (gameloop.c) turns the letter into the code, marks the icon, prints the command
name and a dash, and waits for a target (code 0x8e). Anything else prints "what?". With the
mouse, a left click on an icon gives the command (`mkMouseCommand`, console.c).

**The default command.** A right click on an icon, or ALT with the command's letter
(`CON_ReadCommand`, console.c), makes that command the default (`setDefaultCommand`); it is
marked on its icon. A right click on the map or on the status panel then runs the default
command on what was clicked, the target already chosen (`CON_getch` returns it and then
0x8e). TAB with no command pending gives a free cursor, and Enter on it runs the default
command. There is none at the start, and restoring a game clears it (`GAME_Restore`).

**While aiming** (`CON_ReadCommand`): the direction keys move the cursor, which stays in the
map window and within the selection range of the active member (the larger of the x and y
distances, `COMBAT_TileDistance`). With range "adjacent only" a direction key selects that
square at once. Enter, or the command's letter again, selects. Space or Esc cancels. TAB
switches between the map cursor and the slot cursor of the equipment view. Keys 1 to 9 pick
that party member as the target. A mouse click on the map selects any square of the window:
the range limit applies to the keyboard cursor only.

The other keys of `GameLoop` and `CON_ReadCommand`:

| Key | Effect |
|---|---|
| arrows, keypad | walk (code 0x80); the keys turn with a rotated view |
| Space, Esc | pass: "Pass!", the active member's movement points become 0 |
| B | 0x88: "Begin combat!" or "Break off combat!"; beginning is refused in solo mode (see *Combat*) |
| R | 0x89, which `GameLoop` does not handle: it prints "what?". Rest is only by using a bed (see *The use command*) |
| Ctrl-S | 0xaa: "Save game?" (refused in combat and when the game can no longer be won; see *Conversations and quest state*) |
| Ctrl-R | "Restore game?" |
| 0 | party mode, if nobody is more than 8 squares from the view's centre ("Not everyone is here.") |
| 1 to 9 | solo mode for that member, unless paralysed, asleep or dead (NPCStatus 0x16), with NPCFlag 0x10, or charmed (ObjStatus 0x04): "*name* fails to respond." |
| Ctrl-E, Ctrl-Q, ALT-X | "Exit to DOS?" |
| Ctrl-Z, Ctrl-A | sound effects on/off, music on/off |
| Ctrl-H | highlighting of conversation keywords on/off ("Help on/off!") |
| Ctrl-V | the version string |
| `*`, `/` or F10, `+`, `-`, F1 to F8 | status panel: portrait/equipment view, party view, next/previous member, member n (not in the dream) |
| `` ` `` or `~` | in the equipment view, the combat mode button |

Party mode and solo mode are refused while dreaming ("Not while dreaming!") and in combat
("Not while in combat mode!"); the equipment view's keys do nothing in the dream. The debug keys
are in *Debug keys*.

### Reach

`REACH` (objcmd.c) is the active member's level if it has the first talent, else 0. It
decides how far an object can be handled:

| Command | Rule | Refusal |
|---|---|---|
| Get | an adjacent square always; farther only with the first talent, and then the object must fly to the actor along a clear path (`COMBAT_Missile`, at most 8 squares). No distance is compared with the level: the keyboard cursor stops at level + 1, a mouse click reaches the whole map window | "Out of range!" without the talent, "Blocked!" |
| Move (on the map) | combat distance (`COMBAT_range`, see *Combat*) at most `REACH + 1`: 1 without the talent | "Out of range!" |
| Drop | anywhere the thrown object reaches along its path; the reach only decides whether a devil pod breaks | "Blocked!" |

Most refusals play sound effect 5 (`MUS_PlaySfx(5)`). Get, Look and Move turn the active
member towards the pointer (`UpdateTurnFrame`).

### Look (`CmdLook`, objcmd.c)

Look prints "You see " and then:

- "darkness." for a square that is dark or not seen (`SelectionIsDark`);
- the map tile's name when no object is selected, or the object is invisible (ObjStatus
  0x02) or has an "ignore" tile (TileFlag2 0x10). Look does not test the talents: an
  invisible object that a talent outlines on the map (`ShowObjects`, object.c) is still
  not described. Outside the map window it says "nothing!";
- "Yourself" for NPC 0 or 1;
- for an NPC: its article (none for a party member) and LOOK name, then its portrait
  (`ShowPortrait`, below), which waits for a key;
- for the obelisk 0x124: its name, then picture *quality* of `mdream.lzc` (entry quality -
  1) in the status area until a key is pressed (`ShowDreamPicture`, gfx.c);
- for any other object: the count (stackable types) or the article, the name, ".", then the
  report of the table below.

A stack of quantity 0 is counted as 1 (Look, Get and the searches set the quantity to 1).

| Object | Report |
|---|---|
| plant 0xfb, frames 0-3 (covered martian seed, young pod plant, pod plant, martian pod) | quality 0x0f "It is growing.", 0x10 "It is ripe.", else "It is not growing." |
| lights: torch 0x6d, candlestick 0x6f, candelabra 0x71, oil lamp 0x73, lantern 0x75 and their lit types (+1) | quality = minutes left: over 30 "Plenty of", 7-30 "It has *n* more minutes of", 1-6 "It's almost out of" (followed by a line break), 0 "No" for the oil lamps and lanterns (0x73-0x76) and "Plenty of" for the others; then "fuel." (lamps, lanterns) or "wick." (the others). Nothing for 0x6d/0x6e with a quantity over 1 |
| switch bar 0x19b | quality 1 "It seems to be loose.", 2 "It is not installed." |
| metal woman 0x11f | quality not 0: "It has a heartstone." |
| rail car 0x19a, martian wheel barrow 0x10c | quantity 0 "It is empty.", else "It has *n* load(s) of " and, by quality, 1 "dirt.", 2 "rock.", 3 "iron ore.", 4 "coal." |
| pocketwatch 0x5b or 0x62 | "The time is *h*:*mm* A.M./P.M." (12-hour clock; hour 0 is 12 A.M.) |
| Belgian combine 0x2d | "It is set to 'Combination'" (quality 0), "'Rifle'" (1), "'Shotgun'" (2 and up) |
| heat ray gun 0xf0, freeze ray gun 0xf1 | the same three settings, then ", and has *n* charge(s) left." (quantity) |
| tracking motor 0x13a | quality bit 0, on the map: "It appears to be loose."; bit 1: "It appears to be broken." |
| panel 0x1ca | by quantity: 4 "It appears to control a dream machine.", 5 "... a spray system.", 6 "... the ruby lens system.", 7 "... the cistern valves."; 0, by frame: 0 "These appear to be mechanical controls.", 1 "... valve controls.", 2 "... electrical controls."; any other quantity "You cannot decipher its purpose." Then quality bit 0, on the map: "The panel is loose."; bit 1: "The panel is broken." |
| cabinet 0x1c9 | by quality: 1-3 "It appears to control a mechanical / valve / electrical system.", 4 "... this dream machine.", 5 "... this spray system.", 6 "... the ruby lens system.", 7 "... the cistern valves."; else "You cannot decipher what it controls." |
| weed sprayer 0x81, spray gun 0x105 | "It's loaded with" the tile name of type 0x77 (bottle of green paint) if the quality is not 0, else of type 0x80 (weed killer), then ", and has *n* charge(s) left." |
| sprayer system 0x114 | quality bit 0 = rubber loaded, bit 1 = flesh-coloured. No rubber: "There is no rubber in the sprayer system." and, when TalkFlags[0x3e] & 0x20 and bit 1 is clear, "It is also not flesh-colored."; rubber, TalkFlags[0x3e] & 0x20 and bit 1 clear: "The rubber in the sprayer system is not flesh-colored."; else "It is ready to use." |
| closed door 0xb3, door 0xe3 | quality bit 7 set: "It is apparently rusted shut." if quality - 0x80 is 0x40 or more, else "It is apparently locked." |
| marker flag 0xac, on the surface only | quality 1-12: its name (below); over 12 "Aargh!"; 0 no name. Then the coordinates, whatever the quality (below) |
| brass plaque 0xe8 | quality 1 "Test Plaque", over 1 "Aargh!" |
| cannon 0x14d | quality not 0: "The cannon will fire steel cannon balls." |
| broken strap 0x1cc | no report: sets TalkFlags[0x02] & 0x40 and starts a conversation with NPC 0x02 (Spector), `TALK_talkTo(1, 2, 0)` (see *Conversations and quest state*) |
| a container on the map, next to the active member | it is searched (below) |

**The marker flags.** The names by quality 1-12 (`PlaqueNames`): Coprates Chasma, Arsia Mons,
Pavonis Mons, Ascraeus Mons, Alber Tholus, Elysium Mons, Hecates Tholus, Terra Sirenum,
Noctis Labyrinthus, Deuteronicus Mensae, Syrtis Major Planum, Olympus Mons. Then, with C
integer division (towards 0) of the flag's position:

- lat = (y - 0x200) × 240 / 1024, printed " *lat*S " if positive, " *-lat*N " if negative,
  " 0 " if 0;
- lon = (x - 0x200) × 360 / 1024, printed " *180 - lon*W" if positive, " *lon + 180*E" if
  negative, "0" if 0. At x = 0 (lon = -180) it prints "-180", a line break and " 0E".

So a flag at (0x300, 0x300) reads " 60S  90W". The map side of the coordinates is in *The
map and machinery*.

**Portraits** (`ShowPortrait`, objcmd.c, and `DrawPortrait`, gfx.c). The portrait goes in the
status panel with a name under it: `TALK_GetCharName` once TalkFlags[n] & 1 is set, else the
LOOK name. Some NPCs are shown as someone else:

| NPC | Shown as |
|---|---|
| 0x67 (Shartek) without TalkFlags[0x67] & 4 | NPC 0x6d, portrait and name |
| 10 to 15 | NPC 0x51, portrait and name |
| ObjStatus bit 0x01 set | the player's face (0x72 male, 0x73 female) named "yourself" (purpose not determined) |
| 6 with TalkFlags[6] & 2 | face 0x16 (`DrawPortrait`) |
| 0x21 without TalkFlags[0x21] & 4 | face 0x61 (`DrawPortrait`) |
| 0xc0 and up (temporary creatures) | nothing: the case for the wisp 0x179 falls through to the return, so the wisp's portrait and the name "wisp" are never shown (bug) |

**Searching** (`SearchObj`, objcmd.c). Looking at a container on the map from an adjacent
square empties it: every object inside is moved onto the container's square, and Look lists
them ("Searching here, you find ... and ...", or "Searching here, you find nothing."). A
container type below 0x151 is searched only at frame 0 (open). The body 0x155 is searched
at any frame. On an "ignore" tile the first other object on the square is taken instead.
Look costs no movement points.
### Get (`CmdGet`, objcmd.c)

Get takes an object from the map into the active member's pack. Pointing at an NPC or at an
"ignore" tile takes the first other object on that square (`FindNonIgnoredObjAt`). Objects
inside an inventory are moved with Move, not Get. In order:

1. Nothing there, or the object is not on the map: "nothing!".
2. The reach (above): "Out of range!" or "Blocked!".
3. The weight: the object's weight plus everything inside it (a body 0x155: its own weight
   only). "That is not possible." if that weight is 0 and the type is not weighed per 10
   units, if the object's own weight (`GetWeight`) is 255, or if it is an NPC. "The total is
   too heavy." if 20 × `GetStr` of the active member is less than the weight plus what it
   already carries and wears (weights in tenths, `STAT_ComputeWeights`, status.c).
4. An object fetched from afar plays sound 0x32 and flies to the actor. The actor pays 3
   movement points (`SubMov`; see *Time, rest, stats and levels*).
5. The radium burns (below).
6. Then by type:

| Object | What happens |
|---|---|
| stackable types | added to the pack as a stack (`AddInvObj`, 3 more movement points); the quality is not carried over (the new stack's quality is 0). "You are carrying too much already." if refused |
| body 0x155 | its contents are spilled onto its square and listed ("Searching the body, you find ..."), then the empty body goes into the pack. The listing reads an uninitialised `found`, so a "," or " and " may appear before the first object (bug) |
| switch bar 0x19b, panel 0x1ca, tracking motor 0x13a, tiffany lens 0xce | taken only when loose (quality odd); else "Won't budge !" with sound 0 |
| lit torch, candle, candelabra, oil lamp, lantern (0x6e, 0x70, 0x72, 0x74, 0x76) | readied in a free hand, still lit; "Your hands are full." if both hands hold something, "That is not possible." if refused. A lit torch gets quality 0xb4 (180 minutes). The light is recomputed (`PassTime(0)`) |
| chunk of ice 0x100 | into the pack, then "The ice is melting." (the melting is in *Time, rest, stats and levels*) |
| anything else | into the pack (`InsertObj`); "You are carrying too much already." if refused |

`InsertObj` and `AddInvObj` refuse when the pack is full or the weight too much (see
*Weight and slot checks*). The free-hand test and the tongs test read `Equipment[]`, which
holds the slots of the member shown in the status panel; when that is not the active
member, they look at the wrong member's hands (as the code runs).

### Drop (`CmdDrop`, objcmd.c)

Drop throws an object of the inventory shown in the status panel (the displayed member,
`StatusMember`) to a square. The pointer must be on an inventory or equipment slot, not on
the owner's picture: else "nothing!". In order:

1. The radium burns (below).
2. An equipped object must come off (`STAT_CheckUnready`, under Unready): a lit torch burns
   out, other lit lights are put out, the winged shoes 0xf won't come off. Lit lights (0x6e,
   0x70, 0x72, 0x74, 0x76) skip this test: they are dropped still lit.
3. A stack asks "How many?" (`AskHowMany`): Enter alone takes all; 0, more than there are,
   or a bad answer gives 0 and ends the command.
4. "Location: ": a square of the map; else "nowhere.".
5. "That is not possible." if any NPC stands there (`OBJ_FindNPCAt`).
6. The object's flight from the displayed member (`COMBAT_Missile`, 8 squares): "Blocked!"
   if it cannot get there.
7. **Open water** (`IsOpenWater`: a tile with terrain bits 0x01 and 0x02 and no object to
   stand on, TileFlag2 0x02 or 0x04), except for the plank 0x18b: the object (or the number
   chosen) is lost with a splash. This costs no movement points. The canal's sound is in
   *The map and machinery*.
8. A square where nothing can be put (`CanPlaceObjAt`: map tile with terrain bit 0x04, or a
   tile or object that is impassable, bit 0x02, without one of the side bits 0xf0) and with
   nothing to stand an object on (`HasSupportAt`: an object of TileFlag2 0x02 before any
   impassable one) takes the object only into a container there; else "That is not
   possible.".
9. The object leaves its equipment slot (a two-handed one frees both hands).
10. The tracking motor 0x13a and the tiffany lens 0xce need something to stand on: "The *x*
   cannot be dropped on the ground." Then the actor pays 3 movement points and the object
   flies (its tile is drawn).
11. **Breaking**: farther than 1 square and farther than `REACH`, a fragile type breaks.
   `FragileTypes` holds only the devil pod 0x133: it lands and takes 1 point of damage
   (`LoseHP(obj, 1, 0)`; see *The use command* and *Combat*). The other branches ("It
   broke!" for stacks, breakable types and containers) cannot be reached.
12. Ice 0x100 dropped on an empty bucket 0xa0 (the first object of the target square, any
   frame) makes it a bucket of ice (frame 2) and the ice is gone.
13. Onto a container (the first object of the square; any container type but the hole
   0x101) that accepts it (`CanPutInContainer`): put inside. A stack keeps its quality.
14. Else on the ground: the object, or a new stack of the number chosen.

The test meant to recompute the light after a lit object is dropped calls
`IsUnlitLightType` with the object number instead of its type, so it never succeeds.

### Move (`CmdMove`, objcmd.c)

**An object on the map** is pushed one square. In order: the reach ("Out of range!", here
without sound 5); the radium burns; the switch bar 0x19b and the panel 0x1ca must be loose,
else "It is fastened securely."; "Where? " wants a direction (a direction key or a click
around the object), else "nowhere."; the direction's name is printed. Then:

- "That is not possible." if the type weighs 0 (`TypeWeight`) and is not a per-10 type.
  Nothing else about weight is tested: heavy objects can be pushed.
- An NPC (if its type weighs something) resists by strength: "Failed." when
  ((STR of the NPC / 2) + 30 - STR of the actor) / 2 > `OSI_rand(1, 30)` (`ResistStrVsStr`,
  strengths from `GetStr`), when it is the actor itself, or when it lies down (frame & 3 = 3,
  except a body 0x155).
- The actor pays 5 movement points, even if the push then fails.
- Into open water (not the plank 0x18b): an object is lost with a splash; an NPC: "Failed.".
  The splash sound is chosen from the actor's square, not the water's.
- The tracking motor 0x13a and the tiffany lens 0xce need something to stand on there: "The
  *x* cannot be moved onto the ground." The name printed comes from an uninitialised
  variable (`obj`), so it can be wrong (bug).
- Ice 0x100 onto an empty bucket 0xa0 (frame 0): a bucket of ice, the ice is gone.
- Onto a container that accepts it (the first object of the square), unless the pushed
  object with its contents weighs 255: it goes inside. Onto the
  hole 0x101, or when `InsertObj` refuses, it is simply moved onto that square.
- Otherwise `PushObj` decides: "Won't go in that direction!" or "Blocked!".

`PushObj` (objcmd.c):

| Object | Rule |
|---|---|
| rail car 0x19a | never diagonally. Even frames move only east-west, odd frames only north-south (the source comment says the opposite). The square must be track: a track tile (`IsTrackTile`), or a track object (0x19c rail switch, 0x19d turntable, 0x19e and 0x1a3 track, 0xaf ground, 0xa3 mine exit) whose frame has the car's parity, or frame 2, or is a rail switch, ground or mine exit. Blood, ichor and sap (0x14e-0x150) and the barge 0x1a1 do not block; any other object does |
| plank 0x18b | goes onto a chasm 0xb0 |
| others | the square must be one the object could walk onto (`CanMoveTo`, canmove.c) or have something to stand it on (`HasSupportAt`). Diagonally, the wheel barrow 0x10c and type 0x1b9 (LOOK has no name) "won't go"; other objects need one of the two side squares to be free in the same way |
| 0x1b9, when blocked | turns to a frame by the direction (5 if the square south of it holds ground 0xaf and it was pushed north or north-west) |

**Walking into** a rail car, a wheel barrow or type 0x1b9 pushes it ahead of the active
member (`NPC_TryStep`, canmove.c). The wheel barrow first turns to face the way it goes
(frame 0 north, 1 north-east to south-east, 2 south, 3 the rest), type 0x1b9 likewise (or
frame 5 on ground 0xaf). If `PushObj` refuses, the step is blocked ("Won't go in that
direction!" when it says so). A rail car may also be pushed onto an empty water square: what
it does in the canals is in *The map and machinery*. A pushed rail car plays sound 0xc, and
0xd onto track 0x1a3.

**An object of the inventory** shown in the status panel goes to a party member or into or
out of a container. In order: the radium burns; `STAT_CheckUnready` (as for Drop, but lit
lights are not spared: a lit torch burns out, other lit lights are put out); the displayed
member must be on screen ("That person isn't here!"); "How many?" for a stack; "Where? " and
a target, else "Nobody.". A target on the map that is not an NPC answers "Nobody.".

- **To an NPC**: "That person isn't here!" if not on screen, "Only within the party!" if
  not a party member (NPCStatus 0x80). Refused with "*name* is carrying too much already."
  when 20 × `GetStr` of the receiver is less than the weight given plus what it carries and
  wears, or its pack has 16 slots used. The object leaves its equipment slot and goes into
  the receiver's pack. Giving to oneself prints "Yourself". The actor turns and pays 5
  movement points. A stack whose quality is not 0 is wholly deleted from the giver even when
  only part was given, because the code compares the quantity with quality << 8 | number
  (bug).
- **Into or out of a container** of the same holder ("That container belongs to someone
  else!"): not an equipped object ("You have to unready it first."), not into itself ("An
  item can't be placed inside itself!"). Moving an object onto the container it is already
  in (for example the container's picture at the top right of the panel) takes it out into
  the container's holder ("out of ..."; "You are carrying too much already." if that holder
  has 16 slots used; if the holder still refuses it, it is put on the ground at the holder's
  square). Otherwise it goes in ("into ..."): "It's already full." at 16 slots, "You can't
  do that!" if `CanPutInContainer` refuses, "That container is full." if the insertion
  fails. Ice into an empty bucket makes a bucket of ice; into any other bucket "You can't do
  that!" (the bucket's own "That container is full." cannot be reached). A stack put into a
  container loses its quality (the new stack gets the number only); a stack taken out has
  the same quality bug as giving. Each costs 5 movement points.

### The radium burns

The block of radium 0x1c0 and chips of radium 0x1c1 burn the hands. When Get, Drop or Move
handles one, and the active member has no first talent (`REACH` is 0), is not NPC 0x06
(Chsheket; for Drop the test is on the displayed member), and neither hand slot holds tongs
0x88, the active member is shown hit, becomes poisoned (NPCStatus 0x08) and "OUCH. It is
very hot." is printed. The command goes on. Readying the tongs prints "The tongs will now
protect your hands." Carrying radium outside a lead box is in *Time, rest, stats and levels*.

### Ready, Unready and the equipment slots

In the equipment view (`MouseInEquipView`, console.c), with no command pending and not
while Move chooses a target, a click (or Enter on the slot cursor) on an object of the
grid readies it, on a container opens it (below), and on an equipped object unreadies it.
The Use command can also ready (see *The use command*).

The slot comes from the type (`STAT_GetEquipSlot`, objtype.c). The ranges are tested first,
then the lists, each from its end:

| Slot | On the panel | Types |
|---|---|---|
| 0 head | top | 0x01-0x05 (cap, cowboy hat, pith helmet, military helmet, derby) |
| 1 neck | left, top | 0x06-0x08 (kerchief, silk scarf, muffler), martian jewelry 0xea |
| 4 body | right, top | 0x11-0x17 (cloth jacket, wool sweater, cape, duster, wool overcoat, sheepskin jacket, arctic parka), electric belt 0x5a |
| 2 right hand | left, middle | one-handed: derringer 0x29, revolver 0x2a, hatchet 0x31, axe 0x32, ball-peen hammer 0x33, knife 0x36, machete 0x37, saber 0x38, bloody saber 0x10, martian ritual pod knife 0xf2, weed sprayer 0x81, spray gun 0x105, tongs 0x88, pry bar 0x66 |
| 5 left hand | right, middle | the lights 0x6d-0x76 (torch, candlestick, candelabra, oil lamp, lantern; unlit and lit) |
| 3 (Ultima VI's right finger) | left, bottom | 0x21-0x27, the gloves (lady's silk gloves ... welding gloves) |
| 6 (Ultima VI's left finger) | right, bottom | 0x19-0x1e, trousers and dresses (cotton pants ... chaps and jeans) |
| 7 feet | bottom | 0x09-0x0f, shoes and boots (man's shoes ... winged shoes) |
| 8 both hands | right hand, left marked taken | shotgun 0x2b, rifle 0x2c, Belgian combine 0x2d, elephant gun 0x2e, sling 0x2f, bow 0x30, sledge hammer 0x34, pick 0x41, shovel 0x42, hoe 0x43, rake 0x44, pitchfork 0x45, cultivator 0x46, scythe 0x47, saw 0x48, pool cue 0x191, heat ray gun 0xf0, freeze ray gun 0xf1, M60 machine gun 0x139, red cape 0xa2, red throw rug 0xa1, Cupid's bow and arrows 0x28 |

**Ready** (`STAT_Ready`, status.c) prints "Ready-*name*". A type with no slot: "Can't be
readied!". A one-handed object goes to the left hand when the right is taken and the left
free; a light goes to the right hand when the left is taken and the right free. A two-handed
object needs both hands free. A taken slot: "No place to put!". From a stack one object is
readied, with the stack's quality. The object moves to the NPC that carries it (even from
inside a container). `InsertObj`/`AddInvObj` apply the worn-weight test (10 × `GetStr`, see
below); a refusal prints nothing, and for a stack the object already taken from it is lost
(bug). There is no "modern weapons" rule (unlike SE). Side effects:

- The red throw rug 0xa1 becomes the red cape 0xa2, for good.
- Tongs 0x88: "The tongs will now protect your hands."
- Winged shoes 0xf on level 2: an object 0x92 (LOOK: "something you shouldn't") is made at
  (0xc9, 0x9b, 2), made permanent (ObjStatus 0x20 cleared); "Aargh!" if it cannot be made.
  Its purpose was not determined.

**Unready** (`STAT_Unready`, status.c) prints "Unready-*name*" and asks `STAT_CheckUnready`:

- a lit torch (shape 0x6e, frame 0) burns out: it is deleted, "A torch burns out.";
- another lit light is put out (`UsePutOutLight`; see *The use command*) and comes off;
- the winged shoes 0xf: "They won't come off!" They cannot be unreadied, dropped or moved;
- anything else comes off.

The object goes back to the pack; "There is no room left in your pack." if `InsertObj`
refuses (16 slots). No weight is tested.

### The inventory and containers

The equipment view shows the paper doll with the 8 slots and a 4 × 4 grid of the objects not
equipped, owned by the member or the container shown (`STAT_DrawInventory`, status.c). The
picture at the top right is the owner; clicking it goes back to the container's holder, or
from a member to the party view. Clicking a container in the grid opens it, except frame 2
(locked) of any container but a body: "Locked!" with sound 5. The view can scroll only back
(the forward scroll is gone), so at most 16 objects are shown. Under the doll, "E:" is the
worn weight over STR and "I:" the total weight over 2 × STR, in whole units (`GetStr` of
the NPC carrying the shown inventory; `WeightToStones` rounds to the nearest unit and
shows any weight under one unit as 1).

The 15 container types (`ContainerTypes`, objtype.c): backpack 0x50, large sack 0x51,
small pouch 0x52, brass chest 0x53, obsidian box 0x55, wooden crate 0x56, steamer trunk
0x57, carpet bag 0x59, open brass trunk 0x130, lead box 0x8b, body 0x155, hole 0x101, open
barrel 0x68, open crate 0x11c, bag 0x11d. Frame 0 is open, frame 2 locked or sealed.

`CanPutInContainer` (objcmd.c) accepts an object into a container when:

- it is an object (not an NPC) and the container has fewer than 16 slots used
  (`OBJ_InvSlotsUsed`, counting a stack it would join as no new slot);
- ice 0x100 into an empty bucket 0xa0 (frame 0) is always accepted;
- otherwise the container must be of a container type, at frame 0, not a body 0x155, and the
  object must not be of a container type: no container goes into another.

The test meant to keep lit lights out ("an unlit type at a frame above 0", SE's way of
marking a lit light) passes the object number to `IsUnlitLightType` instead of the type, so
it never refuses, and MD's lit lights are other types anyway. Lit lights can be put into
containers.

### Weight and slot checks

An NPC or a container holds at most **16 slots** (`OBJ_InvSlotsUsed`, object.c): every
object directly inside, but for NPCs 0x00-0x0F the equipped objects are not counted, and a
stackable object that would join an existing stack of its type (quality 0, quantity under
0x90) uses no new slot. A body 0x155 has no limit.

| Where | Test (weights in tenths) | Result when refused |
|---|---|---|
| `CmdGet` | 20 × `GetStr` < object with contents + carried + worn | "The total is too heavy." |
| `AddInvObj` (stacks taken, given, created) | owner an NPC: 20 × `GetStr` < weight of the number added + carried + worn; when readying, 10 × `GetStr` < that weight + worn; 16 slots for the pack or a container | -1: the caller's message |
| `InsertObj` (objects moved into an NPC or container) | NPC: object from the map and 20 × `GetStr` < its own weight (contents not counted) + carried + worn; readying an object not yet worn and 10 × `GetStr` < its weight + worn; 16 slots for the pack or a container. Not while `AllowSubmissionMove` (bodies being filled) | -1 |
| `CmdMove` to a member | 20 × `GetStr` of the receiver < weight given + carried + worn, or 16 slots | "*name* is carrying too much already." |

Containers have no weight limit of their own. "Worn" (`WeightEquip`) counts `TypeWeight`
once per equipped object. The first slot test in `InsertObj` reads the object's type before
it is set (`QuanType(objTyp)`, marked "sic" in the source); a later test catches stackable
objects, so whether a 17th non-stackable object can be added depends on a leftover value
(uncertain).

### Stacking

The 21 stackable types (`StackableTypes`, objtype.c; `QuanType` returns 2) keep a quantity in
the low byte of `Amount` and a quality in the high byte: pistol rounds 0x39, shotgun shells
0x3a, rifle rounds 0x3b, elephant gun rounds 0x3c, sling stones 0x3f, arrows 0x40, cans of
lamp oil 0x7c, matches 0x6b, torches 0x6d, blobs of oxium 0x83, berries 0x49, 0x4a, 0x4b,
0x4c and 0x4d, chips of radium 0x1c1, dollars 0x18, rubles 0x84, wormsbane seeds 0x9e, pages
0x7a, oxygenated air bottles 0x144. MD has no type with a 16-bit quantity: the code for
`QuanType` 4 is never used.

An object joins a stack of its type in the same owner when the sum stays under 0x90 (so a
stack holds at most 143), the stack is not equipped and not flagged 0x40 (`AddInvObj`,
`InsertObj`). The stack's quality is not compared, but an amount passed with a quality in its
high byte is always 256 or more, so it never joins and makes a new stack. Equipped stacks
are always new objects.

The 18 types whose `TypeWeight` is for 10 units (`WeightPer10Types`; `GetTypeWeight` =
`TypeWeight` × quantity / 10): the ammunition 0x39, 0x3a, 0x3b, 0x3c, 0x3f, 0x40, the
pocketwatch 0x5b, masonic symbol 0x5c, spectacles 0x5d, matches 0x6b, oxium 0x83, berries
0x49, 0x4a, 0x4b, 0x4d, dreamstuff 0x14b, dollars 0x18, rubles 0x84. Dreamstuff 0x14b also
counts nothing of what is inside it (`Encumbrance`, status.c).

### Objects given and taken by scripts

`GiveObj` (object.c), called by the conversation opcode OP_GIVEOBJ, makes a stack in one
`AddInvObj` call, or a non-stackable object one at a time with the quality and quantity 0.
What does not fit is dropped on the receiver's square (made permanent), and the receiver
is then moved onto that square again, above it. It is refused instead (the function
returns -1 at once, earlier objects staying given) when the word at D_EBC0, which the
sources name `GiveObjRefuses`, is not 0. Nothing names that word otherwise: `VarInt[]`
starts at D_EB86 with 2 bytes per variable, so D_EBC0 is `VarInt[29]`, the conversation
variable #T. OP_GIVEOBJ sets #T to 1 when `GiveObj` returns -1, else 0 (see
*Conversations and quest state*). So a gift that does not fit is dropped while #T is 0, and
refused while #T is not 0.

`TakeObj` (object.c), for OP_TAKEOBJ, takes *quantity* objects of a type (of a given quality,
or any when the quality is 0) from anywhere inside the owner: from stacks by their
quantity, other objects one by one. After deleting a non-stackable object it skips the next
object in the list (bug): of two such objects next to each other, the second is not taken
on that pass.

### Object changes made by the object routines

- **The pocketwatch**: put into anything (`InsertObj`) type 0x5b becomes 0x62; put on the
  map (`MoveObj`) 0x62 becomes 0x5b. Only 0x5b is weighed per 10 units. Why it changes was
  not determined.
- **Creeping cacti**: whenever a creeping cactus 0x17f (a creature in slots 0xc0-0xff) is
  moved or created, `OBJ_JoinCreepingCacti` gives every live one a frame from its live
  0x17f neighbours: bit 0 north, 1 east, 2 south, 3 west (frames 0-15).
- **A rail car 0x19a moved onto water** floats, crashes or sinks (`MoveObj`; see *The map
  and machinery*).
- **An NPC moved** by `MoveObj` gets the effects of what it lands on when the mover asks for
  it (`MoveAppliesTerrain`; `ApplyTerrainEffects`, gameloop.c). `MoveObj` passes
  area-relative coordinates there (marked "sic" in the source). The hazards are in *The map
  and machinery*.
- **Dead bodies** (`OBJ_MakeDeadBody`, object.c): a map object with the NPC's shape and
  status, quality = the NPC number, quantity 0, permanent unless the NPC was a temporary
  creature. A phlogistonite cannister 0xef the NPC carries is dropped on the square;
  everything else goes into the body, equipped objects flagged 0x40 (they are readied
  again if the NPC comes back, `OBJ_ReviveFromBody`). "Oh no!" is printed when `InsertObj`
  refuses, which cannot happen for lack of room or strength since those tests are off while
  the body is filled. Blood, ichor or sap (0x14e-0x150) that was first on the square is
  moved over the body. The NPC goes to (n, n) on level 6 and is marked dead (NPCStatus
  0x10); a temporary creature's slot is freed. Death itself is in *Combat*.

### What could not be determined

- What the first slot test of `InsertObj` sees in its uninitialised type, so whether a full
  pack can take a 17th non-stackable object.
- What `AddInvObj`'s `SubMov(owner, 3)` changes when the owner is a container (object
  numbers 0x100 and up index past the actors' movement points).
- The purpose of object 0x92 made when the winged shoes are readied on level 2, of
  ObjStatus bit 0x01 on NPCs (portrait "yourself"), and of the pocketwatch's two types.
- The `TypeWeight` values (game data), so which objects weigh 0 or 255 and cannot be taken.
- The value of `found` in `CmdGet`'s body search, so whether the stray comma appears.

## The use command

Use is where most of Martian Dreams' puzzles live: tools on objects, seeds and dirt,
light sources, machine parts, radium, the cabinet panels. Savage Empire's `CmdUse` was a
long chain of tests by type; Martian Dreams looks the used object's type up in two far
tables in `use.c` and calls one of 75 actions, which live in three overlays (`useact1.c`,
`useact2.c`, `useact3.c`) plus `lever.c`'s `UseLever`. This section explains how an
action is chosen, gives both tables decoded, then every action by subject, with its
conditions, messages and effects. Levers, power, belts, barges, drawbridges and the
canals' machinery are in *The map and machinery*; the dream machine's dream in *The
dream*; the cut scenes' contents in *Cut scenes and the endgame*.

Notation: an object's *quality* is the high byte of `Amount[]`, its *quantity* the low
byte. "Local" means `ObjStatus & 0x20`: such objects are deleted when the party is 20 or
more squares away (`object.c`). Objects that the actions create with `AddMapObj` are local
unless the action clears the bit; the text says "permanent" when it does.

### How Use works

#### Choosing the object (`CmdUse`, use.c)

1. The selection is replaced by the head of a multi-square object (`COMBAT_getHead`).
2. An actor chosen on the map, or an object whose tile is "ignored" (`TileFlag2 & 0x10`),
   is replaced by the first object at that square that is neither ignored nor an actor
   (`FindNonIgnoredObjAt`, objcmd.c). If there is none: "nothing!". **You cannot use an
   actor directly**; you use what lies under it. (Actors can still be the second object.)
3. The object's name is printed.
4. An object on the map must be within reach: `COMBAT_range` from the active member to the
   object at most **REACH + 1**, where REACH is the member's level when its first talent
   is non-zero, else 0. So without the talent only the 8 adjacent squares count (the
   combat distance of a diagonal step is 1). Farther: "Out of range!". A use from more
   than 1 square away plays sound 0x32. Objects in an inventory or container have no
   range check.
5. For a map object the active member turns toward the pointer.
6. `UseObject` runs (below).
7. Whatever happened, the active member then spends **5 movement points**
   (`SubMov(Party[Active], 5)`); only the two refusals above ("nothing!", "Out of range!"
   for the used object itself) end the command without that cost.

`COMBAT_range` is the combat distance of `RangeTable` (combat.c): roughly the larger of
\|dx\|, \|dy\| plus half the smaller, 9 when either is 8 or more.

#### Asking for the other objects (`UseObject`)

1. `GetUseActions` collects the actions of every `UseObjects` row of the object's type (at
   most 10; duplicates removed). None: "*name* can't be used." In the table as it stands
   each type has exactly one row (125 rows, 125 types).
2. If one of the actions wants a second object (its `on[0]` is not −1), "On - " is
   printed and the player aims (`PromptUseTarget` → `GetUseTarget`). The cursor starts at
   the centre of the view; with REACH it may move REACH + 1 squares, without it a direction
   key picks the adjacent square at once (`SelectRange` −1, console.c). A key other than
   a selection prints "nothing!" and ends the command. A map tile with no object can be
   chosen: its tile name is printed and the second object is −1. Otherwise the object's
   name is printed.
3. The distance to the second object is `COMBAT_range` from the active member (1 for an
   object in an inventory) and is kept in `TargetDist` (some actions read it). Beyond REACH
   + 1 and not adjacent: "Out of range!".
4. `FilterActions` drops the actions whose `on` list does not accept the type. If none is
   left: "*the object* has no effect on *the target*." Otherwise, if the target was more
   than 1 square away, sound 0x32 plays.
5. If one of the remaining actions wants a third object, the same happens with the prompt
   "To -"; a refusal prints "*object* on *second* to *third* has no effect."
6. The best of the remaining actions is called with (object, second, third).

A type list accepts a type when its first entry is −2 ("anything, or a map tile"), or
when the type is among its entries (up to −1). The case "−1 alone accepts no object" can
never match a real target: the type of a chosen object is never −1, so **once a target is
asked for, the actions that want no target are dropped**. No type mixes the two kinds of
action, so this never matters in play.

When a map tile is chosen the code still computes `GetType(-1)`, which reads the word
before `ObjShapeType[]` (`NumAnimData`, BSS.ASM). Actions with −2 accept the tile anyway;
an action with a type list refuses it unless that number happened to match.

#### Choosing the action (`BestAction`)

Each remaining action scores `Specificity(on[0]) + Specificity(to[0])`, where −2
("anything") scores 0, −1 ("none") 1 and a list of types 2. The highest score wins; on a
tie the last in the list wins (and `ListRemove` reorders the list as it removes).
Checked over the whole table: no two actions that can survive together ever tie, so the
rule always picks the one with the most specific lists. In practice: a tool on a named
object type beats the same tool's "on anything" action, and an action that names the
third object beats one that takes anything there.

### The tables

`UseObjects` (far segment 2E08): 125 rows of an object type and up to 10 action numbers
(0xff ends a row; a type below 0 ends the table). `UseActions` (far segment 2E67): 75 rows
of `on[8]`, `to[8]` and a far function; the functions are the overlays' stub entries.

#### Object types and their actions

Types with the same action list are grouped. Names are LOOK.LZC's.

| Actions | Object types |
|---|---|
| (none: "can't be used") | rake 0x44, pitchfork 0x45, cultivator 0x46, martian pitchfork 0x109, martian rake 0x10a |
| 0 | folded tent 0x6a |
| 1 | book 0x94, note 0x97, scroll 0xf3 |
| 2 | rubber gloves 0x26, tongs 0x88, pool cue 0x191 |
| 3, 21, 22 | gong hammer 0xe7 |
| 4, 5, 6 | wrench 0x87 |
| 7, 21, 22 | sledge hammer 0x34 |
| 8 | pliers 0x35 |
| 9, 10, 11 | pick 0x41, hoe 0x43, martian hoe 0x107, martian pick 0x147 |
| 9-16, 62 | shovel 0x42, martian shovel 0x10b |
| 17, 18 | pry bar 0x66, chisel 0x110 |
| 17, 21, 22 | hammer 0x111 |
| 20 | axe 0x32, knife 0x36, machete 0x37, scythe 0x47, saw 0x48, martian ritual pod knife 0xf2, martian scythe 0x108 |
| 21, 22 | ball-peen hammer 0x33 |
| 23 | barge controller 0x1a2 |
| 24 | lever 0xc4, switch 0x137, switch 0x138 |
| 25 | cannon 0x14d |
| 26 | berries 0x49, 0x4a, 0x4b, 0x4c |
| 28 | torch 0x6d, candlestick 0x6f, candelabra 0x71, oil lamp 0x73, lantern 0x75 |
| 19, 29 | match 0x6b, tinder box 0x6c |
| 19, 29, 63 | lit torch 0x6e, lit candle 0x70, lit candelabra 0x72, lit oil lamp 0x74, lit lantern 0x76 |
| 30, 60 | can of lamp oil 0x7c |
| 31 | switch bar 0x19b |
| 32 | spyglass 0x5e |
| 33 | sextant 0x60 |
| 34 | brass chest 0x53, obsidian box 0x55, wooden crate 0x56, steamer trunk 0x57, open barrel 0x68, open crate 0x11c, open brass trunk 0x130 |
| 35 | pool table 0x18f |
| 36 | pipe organ 0xa5, sheet music 0x199 |
| 37, 60 | chemicals 0xeb (by frame: Potash, Phosphorous, Acid, Nitrogen, oil) |
| 38 | tiffany lens 0xce, tracking motor 0x13a |
| 39 | assembled drill 0x1b9 (LOOK names its frames 1, 3-5) |
| 40 | Belgian combine 0x2d, heat ray gun 0xf0, freeze ray gun 0xf1 |
| 41 | steel cannonballs 0x8a |
| 42, 71 | empty bucket 0xa0 (frame 1 bucket of water, 2 bucket of ice) |
| 43 | map 0x96 |
| 44 | door 0x98, door 0xde, closed hatch 0x1a5 |
| 45 | panel 0x1ca |
| 46 | repaired belt 0x90 |
| 47, 73 | chip of radium 0x1c1 |
| 48 | bottle of green paint 0x77, weed killer 0x80 |
| 49 | weed sprayer 0x81, spray gun 0x105 |
| 50 | oxium bin 0xd4 |
| 51 | pigment jar 0x7b, rubber 0x14c |
| 52 | headgear 0x128 |
| 53 | dreamstuff 0x14b |
| 54 | dreamstuff hoard 0x1c7 |
| 55 | magical healing elixir 0x79 |
| 56 | closed door 0xb3, door 0xe3 |
| 57 | cymbals 0x118, tamborine 0x119, drum 0x11a, accordion 0x11b |
| 58 | wormsbane plant 0x9d |
| 59 | wormsbane seed 0x9e |
| 61 | block of radium 0x1c0 |
| 64 | berries 0x4d, clump of rouge berries 0x4e, blob of oxium 0x83, camera 0xb8, cable spool 0xc7, dream machine 0x120, martian clock 0x125, oxygenated air machine 0x143, hatch 0x1ab, drill 0x1b8, pile of rocks 0x1ba, pile of iron ore 0x1bb, pile of coal 0x1bc |
| 65 | heart stone 0x1aa |
| 66 | spittoon 0x11e |
| 67 | ruby slippers 0xc |
| 68 | manuscript 0x58 |
| 69 | hand mirror 0x93 |
| 70 | gate 0xb5 |
| 72 | control valve 0xdc |
| 74 | oxygenated air bottle 0x144 |

Action 27 (`UseBattleFrenzy`) belongs to no type: it cannot be reached.

#### The actions

"On" and "To" are the accepted second and third objects ("any" is −2, "-" is none). The
file is where the function lives (24: lever.c).

| # | Function (file) | On | To | What it does |
|---|---|---|---|---|
| 0 | `UseBed` (1) | - | - | pitch the tent and rest |
| 1 | `UseRead` (1) | - | - | read (MDTEXT.LZC or the affidavit) |
| 2 | `UseTakeAndReady` (1) | - | - | take and ready |
| 3 | `UseInstrument` (3) | gong 0x12a | - | "GONG!!!!" |
| 4 | `UseFastenSwitch` (1) | switch bar 0x19b | - | fasten or loosen the switch |
| 5 | `UseInstallDrill` (1) | drill 0x1b8 | - | mount the drill on its cart |
| 6 | `UseFastenPanel` (1) | panel 0x1ca | - | fasten or loosen a panel |
| 7 | `UseFixRail` (1) | replacement track 0x194 | broken track 0x195 | fix the rail |
| 8 | `UseReplaceCable` (1) | cable spool 0xc7 | tower top 0xc9, power cable 0xd7 | replace the power cable |
| 9 | `UseWrongTool` (1) | hole 0x101, hole in the ice 0xff | - | "You can't dig any deeper." |
| 10 | `UseUprootPlant` (1) | covered seed / pod plant 0xfb | - | uproot it |
| 11 | `UseDigHole` (1) | any | - | dig a hole |
| 12 | `UseMoveDirt` (1) | piles 0x1ba, 0x1bb, 0x1bc | any | shovel a pile elsewhere |
| 13 | `UseLoadDirt` (1) | dirt pile 0x102, piles 0x1ba-0x1bc | wheelbarrow 0x10c, rail car 0x19a | load a cart |
| 14 | `UseUnloadCart` (1) | wheelbarrow 0x10c, rail car 0x19a | any | unload one load |
| 15 | `UseBury` (1) | dirt pile 0x102 | any | bury / move the dirt |
| 16 | `UseDirtOnBelt` (1) | pile of coal 0x1bc | furnace 0xe9 | stoke the furnace |
| 17 | `UseOnCrate` (1) | wooden crate 0x56 | - | pry open / nail shut |
| 18 | `UseOpenHatch` (1) | hatch 0x1ab | - | force the hatch |
| 19 | `UseBurn` (3) | any | - | burn something |
| 20 | `UseOpenPod` (1) | pod plant 0xfb | - | cut a ripe pod open |
| 21 | `UsePickBerries` (1) | oxium geode 0x82 | - | break the geode |
| 22 | `UseOnCrate` (1) | brass chest 0x53, obsidian box 0x55, steamer trunk 0x57, open brass trunk 0x130 | - | force it open |
| 23 | `UseBargeController` (2) | - | - | set a barge controller |
| 24 | `UseLever` (lever.c) | - | - | pull a lever |
| 25 | `UseCannon` (2) | - | - | fire the cannon |
| 26 | `UseEatBerries` (3) | - | - | eat a berry |
| 27 | `UseBattleFrenzy` (3) | - | - | battle frenzy (unused) |
| 28 | `UseLightWithMatch` (3) | - | - | light it with a match from the pack |
| 29 | `UseLight` (3) | candlestick 0x6f, candelabra 0x71, lantern 0x75, oil lamp 0x73, torch 0x6d | - | light a light source |
| 30 | `UseRefuelLantern` (3) | lanterns 0x75, 0x76, oil lamps 0x73, 0x74 | - | refuel |
| 31 | `UseSwitch` (3) | - | - | throw a rail switch |
| 32 | `UseTelescope` (3) | - | - | look far away |
| 33 | `UseSextant` (2) | - | - | latitude and longitude |
| 34 | `UseChest` (3) | - | - | open / close a container |
| 35 | `UseBilliards` (2) | - | - | play billiards |
| 36 | `UseOrgan` (2) | - | - | play the organ |
| 37 | `UseChemicals` (3) | pod plant 0xfb | - | treat a planted seed |
| 38 | `UseInstallLenses` (3) | lens holder 0xca | - | mount a lens |
| 39 | `UseDrill` (3) | - | - | drill |
| 40 | `UseGunSetting` (1) | - | - | Rifle / Shotgun / Combination |
| 41 | `UseLoadCannon` (2) | cannon 0x14d | - | load steel balls |
| 42 | `UseBucket` (3) | any | - | fill, pour, dump ice |
| 43 | `UseMap` (3) | - | - | show a map |
| 44 | `UseGate` (3) | - | - | open / close a door |
| 45 | `UseCabinetPanel` (2) | - | - | work a control panel |
| 46 | `UseFixRollers` (2) | bare rollers 0xc0 | - | put the belt back |
| 47 | `UseRadium` (1) | heat ray gun 0xf0, freeze ray gun 0xf1 | - | charge a ray gun |
| 48 | `UseChargeSprayGun` (2) | weed sprayer 0x81, spray gun 0x105 | - | load paint / weed killer |
| 49 | `UseSprayGun` (2) | any | - | spray |
| 50 | `UseOxiumBin` (2) | - | - | take oxium |
| 51 | `UseLoadSprayer` (3) | sprayer system 0x114 | - | add rubber / dye |
| 52 | `UseInstallHeadgear` (2) | dream machines 0x120, 0x121 | - | install the headgear |
| 53 | `UseDreamstuff` (2) | - | - | imagine |
| 54 | `UseDreamstuffHoard` (2) | - | - | pull dreamstuff |
| 55 | `UseHealingElixir` (2) | any | - | heal the dreamer / Tarpch |
| 56 | `UseAutoDoor` (3) | - | - | why the door won't open |
| 57 | `UseInstrument` (3) | - | - | play an instrument |
| 58 | `UseHarvestSeed` (2) | - | - | take a wormsbane seed |
| 59 | `UsePlantSeed` (2) | - | - | plant a wormsbane seed |
| 60 | `UseOil` (3) | any | - | oil a rusted door |
| 61 | `UseRadium` (1) | power unit 0x122, block of radium 0x1c0 | - | install radium |
| 62 | `UseFillHole` (1) | dirt pile 0x102 | hole 0x101 | fill a hole |
| 63 | `UsePutOutLight` (3) | lit sources 0x6e, 0x70, 0x72, 0x74, 0x76 | - | put it out |
| 64 | `UseWrongTool` (1) | - | - | a message per type |
| 65 | `UseInstallHeartstone` (1) | metal woman 0x11f | - | install the heartstone |
| 66 | `UseSpittoon` (2) | - | - | spit |
| 67 | `UseRubySlippers` (2) | - | - | click the heels |
| 68 | `UseManuscript` (3) | mailbox 0x54 | - | post it (dream) |
| 69 | `UseMirror` (3) | - | - | look (dream) |
| 70 | `UseGateB5` (3) | - | - | open / close the gate |
| 71 | `UsePourBucket` (3) | empty bucket 0xa0 | - | pour bucket into bucket |
| 72 | `UseValve` (2) | - | - | start the canals |
| 73 | `UseRadiumOnMechMan` (1) | mechanical man 0x166 | - | give him radium |
| 74 | `UseOxygenatedAir` (2) | any | - | revive / restore |

(1), (2), (3): useact1.c, useact2.c, useact3.c.

How the shovel's actions resolve, as an example of `BestAction`: shovel on a hole → 9;
on a covered seed 0xfb → 10; on bare ground → 11; on a dirt pile 0x102 then "To -" a
cart → 13, a hole → 62, anything else (or a map tile) → 15; on a rock, ore or coal pile
then a cart → 13, the furnace (coal only) → 16, anything else → 12; on a cart then
anything → 14.

### Plants and digging

The digging tools are the pick 0x41, shovel 0x42, hoe 0x43, martian hoe 0x107, martian
shovel 0x10b and martian pick 0x147; only the two shovels move dirt. A square is "free"
for new dirt when it has no map object and its tile has no `TerrainType & 2`. Several
actions look for one around the digger: the 8 neighbours in `DirIncrX/Y` order, starting
north and going clockwise.

#### Holes

**Digging** (11, `UseDigHole`). The second object, if any, must be an ignored tile, else
"The ground is not clear for digging.". `GetDigClass` (typetest.c) of the map tile at the
chosen square decides:

| Tile | Hole made there | Pile put next to the digger |
|---|---|---|
| ground and dunes 0x20-0x24, 0x0c, 0x0d, 0x28-0x3f | hole 0x101 | dirt pile 0x102 |
| polar ice 0x7c, 0x7d, 0x7f | hole in the ice 0xff | chunk of ice 0x100 |
| anything else | - | - ("It has no effect.") |

Both are permanent. With no free square around the digger: "You can't dig here." and the
hole is removed again. A hole is a container: what is buried goes inside it.

**Wrong target** (9): a digging tool on a hole: "You can't dig any deeper."

**Filling** (62, `UseFillHole`): shovel, a dirt pile 0x102, then a hole 0x101. If the hole
holds anything, its contents are brought out (`SearchObj`) and the dirt is not used.
Otherwise the dirt pile and the hole are deleted: "You filled in the hole.". (The
function's parameters are named the other way round from the table; the behaviour is as
described.)

**Moving dirt** (12, `UseMoveDirt`; and 15 with a map tile): the pile goes to the chosen
square unless its tile is impassable ("That is not possible.") or a map object is there
(only the error sound). The pile takes frame 0. If the square it came from has a hole,
the hole is emptied (`SearchHole`): moving the dirt off a filled hole uncovers what was
buried.

**Burying** (15, `UseBury`): shovel, a dirt pile 0x102, then the thing to bury.

1. A map tile: the dirt is just moved (above).
2. An actor: "`Don't bury people on Mars!" (the backquote is in the string). The shovel
   itself: "You can't bury something using itself!".
3. A dirt pile or a covered seed 0xfb already on that square: "You can't pile any more
   dirt there.". No hole 0x101 there: "You must bury the *thing* in a hole.".
4. The objects listed on the square before the hole are counted; an actor among them
   refuses ("Don't bury people..."), and so do two live seeds 0xfc ("You can't bury
   anything here.").
5. Every object there that is not an ignored tile goes into the hole; if one does not
   fit: "Aargh!" and the hole is emptied.
6. The dirt pile moves onto the square (frame 0, permanent). If a seed 0xfc was buried
   the pile becomes a covered seed 0xfb: "You planted the Martian seed."; else "It is
   covered with dirt." (one object) or "They have been covered with dirt.".
7. The hole the dirt came from, if any, is emptied (as when moving dirt).

The mound starts with quality 0 (the dirt pile's).

**Uprooting** (10, `UseUprootPlant`): a digging tool on a covered seed or pod plant 0xfb.
First a dirt pile 0x102 is put on a free square around the digger ("You can't dig here."
if none). Then, if there is no hole 0x101 under the plant: "It has no effect." (the new
dirt pile stays). If the hole's first object is a seed 0xfc, a live one (frame 0) dies
(frame 1) and the seed is put on the square; the objects after the first in the hole
are deleted. The
plant is deleted; "You uprooted the pod plant." when it had grown (frame > 0). The hole
stays open.

#### Wheelbarrows, rail cars and the furnace

A cart's quantity is its number of loads, its quality the kind of load (`DirtLoadKind`,
`LoadKindDirt`): 1 dirt pile 0x102, 2 pile of rocks 0x1ba, 3 pile of iron ore 0x1bb, 4
pile of coal 0x1bc.

- **Loading** (13, `UseLoadDirt`): shovel, a pile, then the cart. The wheelbarrow 0x10c
  takes one load ("There is no more room." when its quantity is 1). The rail car 0x19a
  takes up to 7 loads of one kind ("There is no more room.", "You can't mix different
  things in the same load!"). Its frame becomes (frame & 1) + 2 while partly loaded,
  (frame & 1) + 4 when full (frames 0/1 are empty). "You put the *pile* into the
  wheelbarrow." / "... into the rail car."; the pile is deleted.
- **Unloading** (14, `UseUnloadCart`): shovel, the cart, then a target. Empty: "There is
  nothing to unload.". A permanent pile of the load's kind is made on the cart's square,
  then handled by the target: a map tile → moved there; a cart → loaded; the furnace →
  as 16; a hole → filled; anything else → buried. If that fails, the pile is removed and
  the load stays; else the quantity drops by one (a rail car back to frame (frame & 1)
  when empty, else (frame & 1) + 2).
- **The furnace** (16, `UseDirtOnBelt`): shovel, a pile of coal 0x1bc, then the furnace
  0xe9. The coal is used up; if the furnace's frame is below 4, `BeltStart` runs (see
  *The map and machinery*). A load of another kind unloaded into the furnace is lost:
  "It has no effect.".
- Using a pile by itself (64): "You must use a shovel to move that.".

The drill (below) is the other source of rock, ore and coal piles.

#### Martian pod plants

The pod plant is one object 0xfb whose frame is its stage (LOOK: 0 covered martian seed,
1 young pod plant, 2 pod plant, 3 martian pod, 4 open Martian pod), over a hole 0x101 that
holds the live Martian seed 0xfc (frame 1: dead seed). The steps:

1. Dig a hole, put a live seed on it, and bury it with a dirt pile (above): the dirt
   becomes the covered seed 0xfb, quality 0.
2. **Fertilize** (37, `UseChemicals`): a chemical 0xeb on the mound. Without a seed (on
   the square or in the hole there) the chemical is wasted: "It has no effect.". By the
   chemical's frame:

   | Frame | Chemical | Effect |
   |---|---|---|
   | 3 | Nitrogen | quality \|= 1: "You pour the nitrogen compound onto the mound." |
   | 1 | Phosphorous | quality \|= 2: "...the phosphorous solution..." |
   | 0 | Potash | quality \|= 4: "...the potash mixture..." |
   | 2 | Acid | "The seed is destroyed by acid.": the seed is deleted, the mound becomes a dirt pile 0x102, quality 0 |
   | 4 | oil | "The seed is poisoned by oil.": the seed dies (frame 1, its date cleared), quality 0 |

   A chemical already given prints "It has no effect." and is still used up. Every use
   deletes the chemical (the whole object).
3. **Water** (42, `UseBucket`): a bucket of water on the mound sets quality \|= 8: "You
   pour the water onto the seed." (no seed: "You pour the water onto the ground.").
4. When the quality becomes 0xf (all three chemicals and water, in any order) and the
   seed is alive, the seed records the date: its quality = `Date_D`, its quantity =
   `Date_M` (both `UseChemicals` and `UseBucket`).
5. **Growth** (`GrowPlant`, gameloop.c; below): one stage at a time up to frame 3, then
   the quality becomes 0x10 ("ripe"; Look reports growing / ripe by the quality).
6. **Harvest** (20, `UseOpenPod`): a cutting tool (axe, knife, machete, saw, scythe, the
   ritual pod knife, the martian scythe) on the pod. Frame below 3: "It is not fully
   grown."; frame 4: "The pod is already open.". At frame 3 the pod opens (frame 4):
   - outside the dream an unconscious Martian 0x123 (permanent) appears on the square:
     "An unconscious Martian falls out of the pod."; the open pod becomes local; flag
     bit 3 of NPC 0x61 (Prektesh) is set;
   - in the dream NPC 0x56 (Carver) is moved there, flag bits 3 and 7 of NPC 0x56 are set,
     he talks, and the dreamer wakes (`DreamReleaseNpc`, `DreamWakeUp`; see *The dream*).

#### Growth (`GrowPlant`)

`PassTime` (gameloop.c), at each call with minutes > 0, whatever the number of minutes,
calls `GrowPlant` for every object 0xfb (objects 0x101-0xbff) inside the 40 x 40 area
around the party on the current level (`IsOutOfArea`). A plant grows only with quality 0xf
and frame below 3; it needs the hole 0x101 under it and the seed 0xfc in that hole, else
"Aargh!" is printed. Then:

- **Outside the dream**, if the seed's recorded day and month are both non-zero: the plant
  grows one frame when `Date_M * 28 + Date_D > month * 28 + day + 3` (four days or more
  after the recorded date), or when `Date_M < month` (the year turned). It then records
  today's date in the seed. So each stage takes at least four days, about 12 days from
  the covered seed to the pod. Away from the area the plant does not grow; on return it
  grows at most one stage, then waits again.
- **In the dream** (`GameFlags & 0x10`) the date is ignored: each call adds 1-2
  (`OSI_rand(1, 2)`) to the seed's quantity (starting from the recorded month); above
  12 the plant grows a frame and the quantity returns to 0. After the first stage a plant
  there grows every 7 to 13 calls.

At frame 3 the quality becomes 0x10, so the plant stops.

#### Wormsbane

The wormsbane plant 0x9d is kept alive by watering; its frame 1 holds a seed.

- **Planting** (59, `UsePlantSeed`): use a wormsbane seed 0x9e; "Location: " and a square
  is chosen (cancel or off the map: "nowhere."). Farther than 1 square, an invisible
  missile (range 9) must reach it, else "Blocked!". The tile must be diggable
  (`GetDigClass`), else "Won't grow there!". One seed goes there (split from a stack):
  "Now it just needs watering.".
- **Watering the seed** (42): a bucket of water on a seed 0x9e lying on the map on a
  diggable tile turns it into a wormsbane plant 0x9d (frame 0) with quality 10-15
  (`OSI_rand(10, 15)`).
- **Watering the plant** (42): at frame 0 it goes to frame 1, quality 10-15; at frame 1:
  "It has no effect." (the water is used anyway).
- **Wilting** (`ShrinkWormsbane`, gameloop.c, every `PassTime` call with minutes > 0, in
  the area): the quality drops by 1; when it reaches 0 a frame-1 plant goes back to frame 0
  with quality 10-15, and a frame-0 plant is deleted. So a plant left alone dies after
  two countdowns of 10-15 calls at most.
- **Harvesting** (58, `UseHarvestSeed`): use the plant. Farther than 1 square the missile
  test applies ("Blocked!"). Frame 0: "No seed to harvest yet.". Otherwise one seed 0x9e
  goes into the active member's inventory ("You are carrying too much already." if it
  cannot), flying there when afar; the plant goes to frame 0 with quality 10-15.

### Light sources and fire

The pairs (objtype.c's `UnlitLightTypes` / `LitLightTypes`): torch 0x6d / lit torch 0x6e,
candlestick 0x6f / lit candle 0x70, candelabra 0x71 / lit candelabra 0x72, oil lamp 0x73 /
lit oil lamp 0x74, lantern 0x75 / lit lantern 0x76. A source's quality is its fuel in
minutes; `PassTime` burns it (see *Time, rest, stats and levels*).

- **Using an unlit source** (28, `UseLightWithMatch`): a tinder box 0x6c, else a match
  0x6b, is looked for in the active member's inventory (`FindInvOfType`); none: "Nothing
  to light the *source* with.". Then as 29 with it.
- **Lighting** (29, `UseLight`): a match, tinder box or lit source on an unlit source.
  - A source in an inventory but not readied must go to a hand: from a stack of torches
    one torch is split off as a readied torch of the member shown in the status area
    (`Party[StatusMember]`); another source is readied (`STAT_Ready`) and must end up in
    `Equipment[2]` or `[5]`. Failing either: "Your hands are full.".
  - "(using *igniter*)" ("a match" for 0x6b) is printed.
  - The source becomes its lit type. If its fuel (quality) is 0 it is filled: the oil lamp
    0x73 and the lantern 0x75 need a can of lamp oil 0x7c from the holder's inventory
    (one is used; none: "There is no fuel."), then get 255 (oil lamp) or 180 (lantern);
    the candlestick and candelabra get 100; a torch 180.
  - One match is used up; a tinder box is never used up.
  - The view is redrawn once with `AmbientLight` 7, then `PassTime(0)` refreshes the light.
- **Putting out** (63, `UsePutOutLight`): a lit source used on itself. A lit torch is
  deleted, "A torch burns out.", unless it is readied: "No.". The others return to their
  unlit type: "The *source* is extinguished.". A lit source on another lit source: "It's
  already lit.". `status.c` also calls this action when a lit source is unreadied.
- **Refuelling** (30, `UseRefuelLantern`): a can of lamp oil 0x7c on a lantern or oil lamp,
  lit or not. A lantern with quality 126 or more, or an oil lamp with 178 or more: "It's
  already full.". Otherwise one can is used and the fuel set to 180 (lantern) or 255 (oil
  lamp).
- **Burning** (19, `UseBurn`): a match, tinder box or lit source on anything else. Rubles
  0x84 go to `BurnMoney` (dreamobj.c: on the map a fire 0x85 starts and spreads; in an
  inventory the dreamer NPC 0 loses 1-10 hit points and the rubles are deleted; see *The
  dream*). A type of `BurnableTypes` (dollars 0x18, rubles 0x84, photograph 0xb9, thread
  0x3e, box of cigars 0x65, matches 0x6b, book 0x94, map 0x96, note 0x97, wormsbane seed
  0x9e, scroll 0xf3, live Martian seed 0xfc, pile of coal 0x1bc, huge lump of coal 0x1bf,
  chunk of ice 0x100) is deleted: "You incinerated the *thing*.". Anything else: "It has
  no effect.". The igniter is not used up.

How the choice falls: a lit source on an unlit one lights it (29), on a lit one puts it
out (63), on anything else burns it (19).

### Reading and looking

- **Reading** (1, `UseRead`): a book 0x94, note 0x97 or scroll 0xf3, in the large text
  window. Quality *n* > 0 shows entry *n* − 1 of `MDTEXT.LZC`, cut into lines at each
  newline or after 80 characters ("You can't read it." if the entry does not load).
  Quality 0 is the **affidavit**: "--Affidavit--", a text naming `$P` (the name in party
  slot 0), and three signature lines, each printed only when its signer's flag is set:
  Richard Sherman (TalkFlags[5] & 8, NPC 5), Capt. Gregory Duprey (TalkFlags[0x19] & 8,
  NPC 0x19), Doctor David Yellin (TalkFlags[0x1a] & 8, NPC 0x1a). Then "Press any key...".
- **Map** (43, `UseMap`): map 0x96 shows picture *quality* of `MDMAP.LZC` until a key
  (`ShowMdMap`, cutscene.c).
- **Sextant** (33, `UseSextant`): not off the surface ("Not while underground."). From the
  active member's square: lon = (x − 512) × 360 / 1024 and lat = (y − 512) × 240 / 1024
  (long arithmetic, truncated toward zero). It prints "You are somewhere near", then
  Lat: \|lat\| with S (lat > 0), N (lat < 0) or a space, and Long: 180 − \|lon\| with W
  (lon > 0), E (lon < 0) or a space at 0 and ±180. Plaques give the same figures (see *The
  map and machinery*).
- **Telescope** (32, `UseTelescope`): the spyglass 0x5e works on levels 0, 2 and 3 only
  ("Not while underground."), and on level 0 not before 05:00 or after 17:59 ("Not at
  night!"). Local trees 0x198 (the moving trees) in the view refuse it: "There are too many
  moving trees nearby.". The view centre moves to the spyglass (or to the outermost holder
  of the container it is in); "Direction-" asks for a direction key, then with `XRayView`
  (Ultima VI's see-through view) set, the view steps one square at a time in that
  direction, 41 steps (the counter runs from 40 down to 0), stopping early on a square
  whose map tile is darkness 0xff. The view then returns. **Bug:** a key other than a
  direction prints "nowhere." and returns without restoring `MapX`/`MapY`: the view
  centre stays on the spyglass's square.

### Tools, containers and doors

- **Take and ready** (2, `UseTakeAndReady`): the rubber gloves 0x26, tongs 0x88 and pool
  cue 0x191. One on the map goes into the active member's inventory ("You are carrying too
  much already." when `InsertObj` refuses), then is readied if it is not.
- **Chests** (34, `UseChest`): brass chest 0x53, obsidian box 0x55, wooden crate 0x56,
  steamer trunk 0x57, open barrel 0x68, open crate 0x11c, open brass trunk 0x130. Frame 2:
  "It is nailed shut." (0x56, 0x11c) or "It is locked." (the others). Frame 1 (closed):
  frame 0 and the contents are brought out onto the square (`SearchObj`). Frame 0: closed
  again (frame 1).
- **Forcing** (17 and 22, `UseOnCrate`): 17 is a pry bar 0x66, chisel 0x110 or hammer 0x111
  on the wooden crate 0x56; 22 a ball-peen hammer 0x33, gong hammer 0xe7, hammer 0x111 or
  sledge hammer 0x34 on the brass chest, obsidian box, steamer trunk or open brass trunk.
  Frame 2 → frame 1: "You pried the nails out of the crate." (for every container). Frame
  1 with the hammer 0x111 → frame 2: "You nail the crate shut.". Else "It has no effect.".
- **Oxium geodes** (21, `UsePickBerries`): one of the hammers 0x33, 0xe7, 0x111, 0x34 on an
  oxium geode 0x82 of frame 0 breaks it (frame 1) and makes a blob of oxium 0x83, quantity
  `OSI_rand(1, 50) + OSI_rand(1, 50)`, on its square or in the same container. A broken
  geode: "It has no effect.". (The function's name and comment speak of berries; the
  types are the geode and oxium.)
- **Doors 0x98, 0xde and the closed hatch 0x1a5** (44, `UseGate`): quality & 0x80: "It is
  locked."; an actor on the square: "Blocked!"; else frame bit 1 flips (closed ↔ open),
  sound 0x31.
- **Automatic doors 0xb3 and 0xe3** (56, `UseAutoDoor`): "`It is automatic and can't be
  opened by hand.", then the reason: quality 0xc0 or more "It is apparently rusted shut.",
  else quality & 0x80 "It is apparently locked.", else no power: 0xe3 without
  TalkFlags[0x73] & 4 (underground power), 0xb3 without TalkFlags[0x73] & 0x10 (surface
  power): "It has no power.". They are worked by levers and plates (see *The map and
  machinery*).
- **The gate 0xb5** (70, `UseGateB5`): quality & 0x80: "It is apparently locked.". Frames 0
  and 1 become frame 3 (frame 1 also moves one square west), frames 2 and 3 become frame 1
  one square east; frames 4-5 become 7 and 6-7 become 5 in place.
- **Oil on doors** (60, `UseOil`): a can of lamp oil 0x7c, or the chemical 0xeb of frame 4
  (oil; other frames: "It has no effect."), on one of the doors 0xb3, 0xe3, 0x1a5, 0x98,
  0xdb, 0xde. With quality 0xc0 or more (rusted) the quality loses 0x80: "The door is
  unstuck."; else "This door is not rusted.". On a map tile, the rusted hatch tile 8 ends
  a dream (flag bit 6 of NPC 0x51 (Melies), who talks; `DreamReleaseNpc`, `DreamWakeUp`);
  other tiles: "It has no effect.". **Bug (not marked in the source):** the oil is used
  up with `if(QuanType(obj))`, which passes the object number where a type is expected;
  for almost any object number the test fails and the whole stack of cans is deleted.
- **There are no keys.** No use action opens the lock bit 0x80 of a door; locked doors are
  opened by levers (`UseLever`'s qualities 1 and 2, see *The map and machinery*).
- **The hatch 0x1ab** (18, `UseOpenHatch`): the pry bar 0x66 or chisel 0x110. Only NPC 1
  (the player) can: others get "*name* is not strong enough.". Until TalkFlags[0x10] &
  0x10 (NPC 0x10, Tesla) is set, it sets TalkFlags[0x10] \|= 4 and Tesla talks. After:
  cut scene 1 (`Cutscene1`, see *Cut scenes and the endgame*), TalkFlags[0x12] \|= 0x48
  and NPC 0x12 (Blood) talks, the hatch becomes a broken hatch 0x1ac (frame 0) moved one
  square south-east, TalkFlags[0x10] \|= 0x20 and one minute passes. TalkFlags[0x10] &
  0x20 is the flag that makes party members without oxium gasp for air in `PassTime` (see
  *Time, rest, stats and levels*). Using the hatch itself (64): "It is stuck.".

### The tent

Martian Dreams' only way to rest is the folded tent 0x6a (0, `UseBed`): `REST_Command`
(rest.c) is called from nowhere else, and `GameLoop` handles no Rest command code (the
R key sets the code 0x89, which no case of the loop takes). `UseBed` refuses in solo mode
("Not while in solo mode!") and in the dream ("You can't sleep in a dream."). The camp is
centred on the tent's square, or on the party's (`MapX`, `MapY`) when the tent is carried.
The squares x − 1 to x + 1, y − 2 to y must hold no object other than party members,
ignored tiles and tents ("The *object* is in the way."), and the tiles x − 1 to x, y − 2
to y no `TerrainType & 2` ("The ground is not flat enough."). The tent itself is
not used up. What resting does is in *Time, rest, stats and levels*.

### Machine parts

- **Rail switches** (31, `UseSwitch`): the switch bar 0x19b works the turntable 0x19d at
  (x − 1, y − 1); none: "It has no effect.". The switch's frame toggles between 0 and 1. If
  the switch's quality is not 1, the turntable takes the same frame and a rail car 0x19a
  on it flips frame bit 0 (turns); with quality 1 (loose) only the switch moves: "It turns
  loosely.".
- **Fastening a switch** (4, `UseFastenSwitch`): the wrench 0x87 on a switch bar. Quality 1
  (loose): with a turntable at (x − 1, y − 1) the switch takes its frame and quality 0:
  "The switch is fastened."; without: "This switch cannot be fixed.". Any other quality
  (0, or 2 "not installed" by Look) becomes 1: "The switch is loose.".
- **Rails** (7, `UseFixRail`): the sledge hammer 0x34, a replacement track 0x194, then a
  broken track 0x195. It prints "Which section of rail needs fixing -" (a prompt, printed
  after both choices are made), deletes the replacement track and turns the broken one
  into a track 0x19e of the same frame.
- **The drill** (5, `UseInstallDrill`): the wrench on a drill 0x1b8 lying on a drill cart
  0x1b7 ("The drill must be installed onto a drill cart." otherwise). Both are replaced by
  a permanent assembled drill 0x1b9 of frame 1: "The drill's power is connected.". The
  drill 0x1b8 used alone (64): "It has no effect.".
- **Drilling** (39, `UseDrill`): use the assembled drill (sound 0x10). It works on the
  square in front of it, by frame: 1 west, 3 north, 4 east, any other south.
  - Iron ore 0x1bd there gives a pile of iron ore 0x1bb, a vein of coal 0x1be a pile of
    coal 0x1bc; the vein's quality is the loads left (1: the vein is deleted, else
    quality − 1).
  - A cave-in 0xd5 there is removed, with no pile.
  - Otherwise an earthen wall tile 0xf0-0xfb gives a pile of rocks 0x1ba, without limit.
  - Otherwise a target there (`COMBAT_GetTargetAt`) is hit for 40 points
    (`__DoHit(member, target, 40, 0)`); with none: "There is nothing to drill into.".
  - The pile goes, permanent, on the first free square around the drill, starting north
    for frame 1, east for 3, south for 4, west otherwise, clockwise; blood, ichor and sap
    0x14e-0x150 do not count as objects. None free: "There is no room left for the ore."
    (the ore is then not used up).
- **The power cable** (8, `UseReplaceCable`): pliers 0x35, the cable spool 0xc7, then a
  tower top 0xc9 or power cable 0xd7.
  1. The last target farther than 1 square: "The work is too precise to perform
     telekinetically.".
  2. Without the underground power (TalkFlags[0x73] & 4) or with TalkFlags[0x71] & 8 (the
     power not yet permanent, lever.c): "The cable does not need replacement.".
  3. A member other than NPC 1: TalkFlags[2] \|= 0x10, that member talks (solo mode
     lifted meanwhile), the flag is cleared; nothing else.
  4. NPC 1 without readied rubber gloves 0x26: a shock of `OSI_rand(0, MaxHP / 2)` hit
     points (`LoseHP`, kind 0). If NPC 2 (Spector) is on screen, TalkFlags[2] \|= 4 and
     Spector talks; then `TalkFlags[2] &= 4` (**bug**, listed in FINDINGS: `&= ~4` was
     meant, so every other bit of TalkFlags[2] is cleared and bit 2 stays set).
  5. With the gloves: the spool is used up, TalkFlags[0x73] \|= 0x10 (surface power),
     `LampsAtNight` and cut scene 3 (`Cutscene3`).
  Using the cable spool alone (64): "You need to use pliers to attach the cable.".
- **Conveyor rollers** (46, `UseFixRollers`): the repaired belt 0x90 on bare rollers 0xc0.
  From the clicked roller it goes west to the first roller of frame 0 ("Ooops. These
  rollers can never be fixed!" if the chain breaks), then eastward turns every 0xc0 into a
  conveyor belt 0xbc and adds a permanent 0xbd on the 5th, 9th, 13th... The belt object is
  used up; TalkFlags[0x72] \|= 4. See *The map and machinery* for the belts.
- **The heartstone** (65, `UseInstallHeartstone`): the heart stone 0x1aa on the metal
  woman 0x11f. Quality & 1: "The robot already has a heartstone."; else quality 1, the
  stone used up: "The heartstone is installed." (the dream machine panel needs it, below).
- **The headgear** (52, `UseInstallHeadgear`): the headgear 0x128 on a dream machine 0x120,
  or on 0x121 with a 0x120 on its square ("It has no effect." otherwise). A permanent
  installed headgear 0x64 (frame 1) is made on the 0x120's square ("Not now." if it cannot
  be), the headgear is used up: "The headgear is installed."; TalkFlags[0x74] \|= 0x10, and
  if TalkFlags[0x74] & 1, TalkFlags[0x60] \|= 4. The dream machine 0x120 used alone (64):
  "The control panel operates the dream machine.".
- **Barge controllers** (23, `UseBargeController`) and **levers** (24, `UseLever`): see
  *The map and machinery*. The controller needs TalkFlags[0x76] & 0x20 and & 0x40, or the
  dream ("the lever seems stuck."); "Push lever to " a direction sets its frame to the
  direction / 2, any other key "center." (frame 4).
- **The valve** (72, `UseValve`): the control valve 0xdc works like panel 7 (below).

### Radium and the guns

**Hot radium.** The block 0x1c0 and the chip 0x1c1 hurt the hands. In `UseRadium` (both
actions 47 and 61) a member with no first talent (REACH 0), no readied tongs 0x88, and
who is not NPC 6 (Chsheket) is shown hit and **poisoned** (`NPCStatus |= 8`): "OUCH. It is
very hot."; the action goes on. (Get does the same, see *Objects and commands*.)

- **The power unit** (61): the block of radium 0x1c0 on the power unit 0x122. Used on
  itself, a block lying on the map acts on the object linked after it (`Link[]`) if that
  one lies on the same square; in an inventory: "It has no effect.". If the target is the
  power unit: the block is used up, "The radium has been installed.", the unit takes frame
  1 (the dream machine panel looks for it), TalkFlags[0x74] \|= 4, and if TalkFlags[0x74]
  & 1, TalkFlags[0x60] \|= 4. Another target: "It has no effect.". **Bug (FINDINGS):** the
  test for "Radium has already been installed." reads a local variable, `installed`,
  that is never set; whatever the stack holds decides it, so in practice a second block
  can usually be installed again.
- **Ray guns** (47): the chip 0x1c1 on a heat ray gun 0xf0 or freeze ray gun 0xf1. The
  gun's quantity is its charges. More than 210: "The *gun* is fully charged.". Otherwise
  "How many?" (`AskHowMany`; 0 ends). Each chip gives 30 charges, up to 240: if more were
  offered, n = (240 − charges) / 30 and "The *gun* only needed *n* radium blocks." and
  "fully charged" are printed. The chips used are removed from the stack.
- **The mechanical man** (73, `UseRadiumOnMechMan`): a chip on a mechanical man 0x166.
  Only NPC 0x75 (Cutter) takes it; any other: "This mechanical man already has radium.".
  One chip is used, NPC 0x75 takes frame 9 and TalkFlags[0x75] \|= 8. No heat test here.
- **The guns' setting** (40, `UseGunSetting`): the Belgian combine 0x2d and the two ray
  guns. "It is set to "Combination"" (quality 0), "Rifle" (1) or "Shotgun" (2 or more);
  the ray guns add "It has *n* charges left.". Then "New setting (R/S/C)?" (the string is
  `R%/S%/C`, passed to `CON_printf` as a format) waits for R, S, C or Enter: R sets 1, S 2,
  C 0, Enter keeps it. What the setting does in combat is in *Combat*.

### The cabinet panels

A panel 0x1ca sits on a cabinet 0x1c9. The panel's quality has bit 0 "loose" and bit 1
"broken"; its quantity is its kind (Look: 4 dream machine, 5 spray system, 6 ruby lens, 7
cistern valves; 0 a plain panel whose frame tells mechanical, valve or electrical). The
cabinet's quality says what it controls.

**Fastening** (6, `UseFastenPanel`): the wrench on a panel lying on the map (in an
inventory: "It has no effect."). Quality 0 (fastened): it becomes loose (quality \| 1):
"The panel is loose.". Otherwise: broken (bit 1): "It must be repaired first."; no cabinet
under it: "Panels are only installed onto cabinets."; then the panel must match the cabinet
("This cabinet requires a different type of panel."): a cabinet of quality 0 takes a panel
of kind 0; a cabinet of quality 1-3 takes a kind-0 panel of frame quality − 1; any other
cabinet the panel of kind equal to its quality. Then the quality drops by 1 (1 → 0):
"The panel is fastened.". If the panel's quality was 4 before, TalkFlags[0x74] \|= 8 (and
TalkFlags[0x60] \|= 4 when TalkFlags[0x74] & 1). The test reads the panel's quality, not
its kind or the cabinet's: with bit 0 "loose" and bit 1 "broken" it is unclear when a
panel can have quality 4 (uncertain; the source comment calls it "the fourth").

**Using a panel** (45, `UseCabinetPanel`): broken: "The panel is broken."; loose: "The panel
is not installed."; not on a cabinet: "Panels are only installed onto cabinets.". By the
cabinet's quality:

| Cabinet | Effect |
|---|---|
| 0 | "You are completely unsure what you just did." |
| 1 | "You actuate the mechanism to no apparent effect." |
| 2 | "Status lights change but you see no other effect." |
| 3 | "Lights flash and change color but nothing else happens." |
| 4 | the dream machines (`PanelDreamMachine`) |
| 5 | the rubber sprayer (`PanelRubberSprayer`) |
| 6 | the ruby lenses (`PanelRubyLenses`) |
| 7 | the canals |
| other | "It has no effect." |

**4, the dream machines.** If TalkFlags[0x46] & 8: "The dream machines seem to have ceased
functioning.". Within 5 squares of the cabinet it needs the power unit 0x122 at frame 1
(radium installed), a dream machine 0x120 (its quality is the setting) and an installed
headgear 0x64, else "The machine does not work."; the seat is the square of the dream
machine 0x121. On that square:

- an unconscious Martian 0x123: "It has no effect." unless TalkFlags[0x61] & 0x10 is set
  and TalkFlags[0x21] & 4 clear; then cut scene 5, NPC 0x21 (Prektesh) talks and gets
  flag bit 2, NPC 0x61 (also named Prektesh) is moved off the map ((0x61, 0x61) on level
  6, modes 0, `TalkFlagsHi[0x61] |= 0xcb`), NPC 0x21 is moved one square south and killed
  (`COMBAT_Kill`), and the 0x123 is deleted;
- a metal woman 0x11f with her heartstone (quality non-zero; else "It has no effect."):
  cut scene 12; in a party of exactly five, NPC 5 (Sherman) talks with TalkFlags[5] bit 2
  set (solo mode or a status view of him is closed first); NPC 6 (Chsheket) talks and is
  moved one square south; NPC 0x63 is moved off the map like 0x61; the 0x11f is deleted;
- otherwise the actor sitting there (`OBJ_FindNPCAt(1, x, y)`) dreams. Nobody: "There is
  nobody sitting in the machine.". Below the surface: `DreamWakeUp`. A setting below 2
  sets TalkFlags[0x60] \|= 8; a setting other than 3 calls `DreamSleep(actor, setting)`;
  setting 3 calls `DreamEnter` if TalkFlags[0x46] & 1, else "The machine does not work.".

The seat's x, y are left unset when no 0x121 is near (not marked in the source).
Everything that follows is in *The dream* and *Cut scenes and the endgame*.

**5, the rubber sprayer.** The sprayer system 0x114 must be within 5 squares ("Funny, no
sprayer near sprayer controls!"). Its quality bit 0 (rubber) must be set ("There is no
rubber in the sprayer system."). Without bit 1 (dye): if TalkFlags[0x3e] & 0x10, "The
rubber in the sprayer system is not flesh-colored."; else TalkFlags[6] \|= 4 (left set)
and NPC 6 (Chsheket) talks. With both, the actor on the sprayer's square: none "Nobody is
standing under the sprayer."; not NPC 6 "You don't think it would be safe to coat an
animal or plant with rubber."; NPC 6 no longer of type 0x165 "Chsheket has already been
treated.". Else NPC 6 becomes type 0x162 (also in `OrigShapeType`), TalkFlags[6] \|= 0x22,
cut scene 8, she talks, and bit 5 is cleared again. The supplies (51, `UseLoadSprayer`):
rubber 0x14c on the sprayer sets quality bit 0, "The sprayer system now has rubber."; the
pigment jar 0x7b bit 1, "The sprayer system now has dye.". Both are used up.

**6, the ruby lenses.** Already done (TalkFlags[0x76] & 0x20): "The ruby lenses are already
aligned.". It needs bit 7 of TalkFlags[0x2e], [0x39] and [0x30], else "Some of the lenses
are not responding.". Those come from the tiffany lens and the tracking motor on their
holders (below) and from killing the vine with weed killer (spray guns, below). Then
"Alignment coordinate:" and a typed number *n*. With t = `Time_H` × 100 + `Time_M` (12:58
is 1258, so the scale jumps from 1259 to 1300), *n* must satisfy t − 5 < *n* < t + 5;
else "It has no effect.". At night (`Time_H` 18 or more, or 5 or less) "You wait until
dawn..." and time passes, resting, (28 − `Time_H`) × 60 + (60 − `Time_M`) minutes: that
always ends at 05:00 the next day, so before 05:00 it waits more than a day. Then the
three flags are set again, TalkFlags[0x76] \|= 0x20, `BargePumps(1)`, cut scene 6, and NPC
5 (Sherman) talks with TalkFlags[5] bit 4 set for the talk.

**Lenses** (38, `UseInstallLenses`): the tiffany lens 0xce (needs a lens holder 0xca of
frame 0, sets TalkFlags[0x2e] \|= 0x80) or the tracking motor 0x13a (holder of frame 1,
TalkFlags[0x39] \|= 0x80). Broken (quality & 2): "The *lens* is broken."; flag already set:
"It has no effect.". The lens moves onto the holder, quality 0. **Bug (FINDINGS):** if the
chosen holder has the wrong frame, the code sets the holder to −1 and then searches the
3 x 3 squares around `GetX(-1)`, `GetY(-1)` (garbage) for one of the right frame; in
practice "You cannot install it there.".

**7, the canals**, and the control valve 0xdc (72): already working (TalkFlags[0x76] &
0x40): "The canals are already working."; the lenses aligned (& 0x20): TalkFlags[0x76] \|=
0x40, `BargePumps(0)`, cut scene 7; else "There is no water pressure in the valve
system.". See *The map and machinery*.

### Buckets

The bucket 0xa0 has frame 0 empty, 1 water, 2 ice (42, `UseBucket`; the freeze ray turns
water to ice, see *Combat*).

1. Ice: "`Dump out the ice? ", and Y empties it. Nothing else.
2. A map tile chosen (no object) once the lenses are aligned (TalkFlags[0x76] & 0x20): a
   cistern tile 1-4, or with the canals running (& 0x40) a water tile (canal 0x10-0x17 or
   0xc0-0xc3, `IsWaterTile`): "`You fill the bucket with water." (frame 1).
3. Empty: "There is nothing in the bucket.".
4. Water on a map tile: "The *tile* is all wet.", splash. **Bug (FINDINGS):** the code then
   goes on with the type of object −1 (the word before `ObjShapeType[]`); unless that
   matches a tested type it prints the "all wet" message a second time, with the name of
   object −1.
5. Water on a covered seed 0xfb, a wormsbane seed 0x9e or plant 0x9d: see *Plants and
   digging*. On the fountain 0x112: "`You fill the bucket with water.", but this test
   comes after step 3, so **an empty bucket cannot be filled at the fountain** (not marked
   in the source); a full one stays full. On an actor: "*name* is all wet."; on another
   object: "The *object* is all wet.".
6. Except after filling or dumping, the bucket ends empty, **but not in the dream**: there
   a bucket of water never empties.

**Pouring** (71, `UsePourBucket`): a bucket on another bucket. On itself: "An item can't be
placed inside itself!". The full one is poured into the empty one (frame copied, the
other emptied), in either direction; both full: "There is already something in the
bucket."; both empty: "There is nothing in the bucket.".

### Spray guns

The weed sprayer 0x81 and the spray gun 0x105 hold charges (quantity) of paint (quality 1)
or weed killer (quality 0).

- **Loading** (48, `UseChargeSprayGun`): green paint 0x77 into a gun of quality 0: 20
  charges of paint, "The spray gun now contains 20 charges of green paint.". Weed killer
  0x80 into a gun of quality non-zero: 10 charges, quality 0, "...10 charges of weed
  killer.". The same kind again: +10 charges, at most 30, "The spray gun has 10 more
  charges.". The bottle is used up. (A new gun of quality 0 counts as weed killer: weed
  killer into it adds 10.)
- **Spraying** (49, `UseSprayGun`), at anything. No charges: "There is nothing in the
  gun.".
  - Paint (one charge): an actor of type 0x91 (LOOK: monster footprints) becomes
    type 0x16c (proto martian): "`*name* becomes visible!"; an invisible
    object or actor (`ObjStatus & 2`) loses the bit, same message; anything else: "It has
    no effect.".
  - Weed killer on an actor: an attack with the gun (`__TryHit`, see *Combat*); no charge is
    taken here. On a plant (`IsPlantType`: vine 0xcd, tree 0x198, Porcupod 0xa8, flower
    0xaa, hard-shelled plant 0xab, devil plant 0x12c, devil pod 0x133, hedge 0x12b): one
    charge, "You killed the plant.": the vine is deleted and sets TalkFlags[0x30] \|= 0x80
    (NPC 0x30, Carver; one of the ruby lens conditions), the tree becomes a dead tree
    0xa6, the others are deleted. Anything else: one charge, "It has no effect.".

### Food, drink and medicine

- **Berries** (26, `UseEatBerries`): NPC 6 (Chsheket): "A mechanical person can't eat
  berries!". Otherwise one berry is eaten: "You eat a Martian berry. You feel an increase
  in the ..." when the talent is already non-zero, "... a ..." when not, then:

  | Type | Message ends | Effect |
  |---|---|---|
  | 0x49 | "relationship with things around you." | first talent + 1 |
  | 0x4a | "sensitivity to the feelings of objects around you." | psychic talent + 1 |
  | 0x4b | "sense of your spatial location." | third talent + 1 |
  | 0x4c | "sudden flush, dizziness, and nausea." | `OSI_rand(5, 20) + OSI_rand(5, 20)` hit points lost (`LoseHP` kind 2: no armour, no bleeding), drunkenness + 1-2 (`DrunkCounter`), poison cured |

  For 0x49 "already non-zero" is tested on REACH (level when the talent is set). A new
  psychic or third talent marks the view for redrawing. The talents are 4-bit fields:
  `PassTime` caps them at 10 for NPCs 0-0xf at every call (gameloop.c); eating several
  berries before it runs could pass 10, and a 16th point would wrap to 0. What the
  talents do and how they fade: *Time, rest, stats and levels*. The berries 0x4d and the
  clump of rouge berries 0x4e (64): "The berries are not edible.".
- **Oxium** 0x83 used (64): "You will chew it instinctively." (carrying it is what counts,
  see *Time, rest, stats and levels*). **The oxium bin** 0xd4 (50, `UseOxiumBin`): farther
  than 1 square an invisible missile (range 9) must reach it ("Blocked!"); 20 oxium go into
  the active member's inventory ("You are carrying too much already." if not), "You get
  two handfuls of oxium from the bin.", and TalkFlags[0x12] \|= 0x10 (NPC 0x12, Blood).
  The bin never empties.
- **Oxygenated air** (74, `UseOxygenatedAir`): the bottle 0x144 on a body 0x155 or on a
  party member (`NPCStatus & 0x80`); anything else: "The *object* doesn't need oxygenated
  air.". A party member is restored to full hit points: "*name* is revived!". A body:
  `SPELL_ReviveNPC` (revive.c) brings its NPC back with full hit points, cured, on a free
  square within 4: "*name* is revived!"; a body of quality 0x10 or more (not an NPC's) is
  deleted: "*body* is beyond help.". One bottle (the stack's quantity) is used in every
  case.
  **Bug (FINDINGS):** before calling `SPELL_ReviveNPC` the code tests `n < 0x10` with the
  register variable `n` not yet set (the quality test was meant); when the leftover value
  is 0x10 or more, the body is "beyond help" (and stays) whatever it is. The oxygenated
  air machine 0x143 (64): "You don't know how it works.".
- **The healing elixir** (55, `UseHealingElixir`): the magical healing elixir 0x79 on NPC
  0x68 (Tarpch): TalkFlags[0x68] \|= 0x40 and he talks. On NPC 0 (the dreamer) whose hit
  points are below NPC 1's maximum: "*NPC 1's name* is healed." and `HitPoints[0]` =
  `MaxHP(1)` (**bug** listed in FINDINGS: NPC 1's maximum and name, NPC 0's hit points).
  Else "It has no effect.". The elixir is never used up.
- **The spittoon** 0x11e (66): "Your mouth is too dry." when the active member is gasping
  for air (`ObjStatus & 0x40`), else "You spit into the spittoon.".

### Dream objects

These belong to the dream (see *The dream*); the use side:

- **Dreamstuff** 0x14b (53, `UseDreamstuff`): sound 0x32, "You imagine " and the first
  object inside it (none: "nothing!"). An object below type 0x156 takes the dreamstuff's
  place (on the map or in its container). A creature type is made with `EGG_generate` at
  the party's square, given the inner object's frame, made permanent unless the inner
  object is local, given its `ObjStatus & 0x80`, its quality as A.I. mode (`NPCMode`,
  `NPCComMode`), the alignment bits 0x60 from the dreamstuff's quality (1 → 0, 2 → 0x20,
  3 → 0x40, 4 → 0x60; other values leave them), and placed on the first square around the
  party it can stand on, else teleported (`COMBAT_TryTeleport`), else "nothing!" and it is
  removed (the dreamstuff then stays). The dreamstuff is used up.
- **The dreamstuff hoard** 0x1c7 (54, `UseDreamstuffHoard`): needs the psychic talent ("You
  lack the mental power to affect the dreamstuff hoard."); farther than 1 square the
  missile test applies ("Blocked!"). A dreamstuff 0x14b goes into the active member's
  inventory: "You pull some of the dreamstuff from the hoard.". Inside it: an M60 machine
  gun 0x139 if the hoard's quality is non-zero (the quality then becomes 0), else a random
  type 0x31-0x38 (hatchet, axe, ball-peen hammer, sledge hammer, pliers, knife, machete,
  saber). The hoard never empties.
- **Release triggers** (each sets flag bit 6 of the NPC, makes him talk, then
  `DreamReleaseNpc` and `DreamWakeUp`): the manuscript 0x58 on the mailbox 0x54 (68,
  `UseManuscript`, NPC 0x57 Clemens, no other condition); the hand mirror 0x93 (69,
  `UseMirror`) when the view centre is at x 0x76-0x78, y 0xca-0xcc on level 2 (NPC 0x50,
  Lowell; elsewhere "You see yourself."); oil on the rusted hatch tile 8 (60, NPC 0x51,
  Melies); a ripe pod opened in the dream (20, NPC 0x56, Carver, flag bits 3 and 7).

### Amusements

- **Billiards** (35, `UseBilliards`): the pool table 0x18f needs a readied pool cue 0x191
  ("You need a cue stick in your hands."). Balls 0x190 at (x − 1, y) and (x, y) are found
  or made (random frames 0-6). The balls change frame with click sounds at least once and
  again while `OSI_rand(0, 40) < dexterity`; then "Good shot, old bean!" if one more
  `OSI_rand(0, 40) < dexterity`. With dexterity *d* each roll succeeds with chance *d* / 41.
- **The pipe organ** (36, `UseOrgan`): the pipe organ 0xa5, or the sheet music 0x199 lying
  on the map with the organ on the square east of it ("Organ music must be played on a
  pipe organ." otherwise, also for sheet music in an inventory). When the sheet music lies
  on the square west of the organ: "Tocatta and Fugue in D minor (J.S. Bach)." and
  `MUS_PlayMelody`. Otherwise the music stops, "Press keys 1-0 to play -", and each digit
  key '0'-'9' plays sound effect key − 10 (0x26-0x2f); any other key ends it and the area
  song returns. Nobody needs to sit anywhere: the source comment's "seat" is the object
  0x199.
- **Instruments** (57, and 3 for the gong, `UseInstrument`): cymbals 0x118 "Ching!", tamborine
  0x119 "Shing!", drum 0x11a "Thump! Thump!" (two beats 0.2 s apart), accordion 0x11b
  "Wheeeeze", the gong hammer 0xe7 on the gong 0x12a "GONG!!!!". Any other: "You play the
  *object*.".
- **The cannon** (25, `UseCannon`): "Fire!", sound 0x30. It fires in the direction of its
  frame (0 north, 1 east, 2 south, 3 west), at the first actor in the 6 squares ahead (or
  the 7th square if none), with a missile of range 6. If the missile does not reach that
  square, the square where it stopped is searched for a door 0xad (LOOK: frames 0-1
  unnamed, 2-5 "bulkhead") of frame below 4: with plain shot (cannon quality 0) "Splat!
  There is no effect."; with steel balls the bulkhead's frame goes up by 2, and from frame
  2 or 3 it is blown open: "Kaboom! You blew the door down!", TalkFlags[0x41] \|= 4 (NPC
  0x41, Maurice). So steel balls need two hits on a door of frame 0 or 1. When the missile
  stopped early, the explosion is one square back toward the cannon, unless the door was
  blown. The explosion (`Explosion`, tile 0x107) costs every actor and every breakable
  object with a quantity in it `OSI_rand(1, 15) + OSI_rand(1, 15)` hit points (`LoseHP`
  kind 0). `COMBAT_Missile` has no return statement; the cannon uses what `Missiles` left
  in AX (1 when the missile reached its target).
- **Steel cannonballs** (41, `UseLoadCannon`): 0x8a on the cannon: "The cannon will fire
  steel cannon balls." (quality 1, the balls used up, for good); already loaded: "It
  already has steel cannon balls.".

### The ruby slippers

The ruby slippers 0xc (67, `UseRubySlippers`): not readied, they are taken and readied (as
2). Readied, the first two uses add 1 to their quality: "Click.". The third: "You may use
the ruby slippers to go home. If you choose to, you may now view the Grand Finale.",
"Afterwards, you will exit to DOS...", and the question waits for Y or N. N plays cut
scene 13 (`Cutscene13`); Y calls `GameEnd`. The quality stays 2, so every later use asks
again. See *Cut scenes and the endgame*.

### Other messages (64, `UseWrongTool`)

Using these objects alone only prints a line: the camera 0xb8 "The plate in the camera has
already been exposed."; the martian clock 0x125 "You can't determine how to read the
time."; and the ones given above (hatch, dream machine, cable spool, oxium, piles,
berries, oxygenated air machine). The drill 0x1b8: "It has no effect.".

### Bugs in the use actions

Listed in FINDINGS (`/*sic*/` is not used in these files; some comments say "bug"):
`TalkFlags[2] &= 4` (cable), the uninitialised `installed` (radium), the unset `n`
(oxygenated air), the elixir's NPC 0/NPC 1 mix, the bucket's `GetType(-1)`, the lens
search around `GetX(-1)`, the telescope's unrestored view. Not listed there:

- `UseOil` uses up the whole stack of lamp oil cans (`QuanType(obj)` with an object
  number).
- `UseBucket` tests the fountain only after refusing an empty bucket.
- `PanelDreamMachine` uses the seat's x, y unset when no dream machine 0x121 is near.
- `UseUprootPlant` leaves its new dirt pile when there is no hole under the plant.
- `UseFastenPanel` tests the panel's quality for 4 where its kind (or the cabinet's
  quality) seems meant (uncertain).
- `UseLight` puts a torch split from a stack into the hand of the member shown in the
  status area, who need not be the one carrying the stack.

### What could not be determined

- When a panel 0x1ca can have quality 4 (the only way `UseFastenPanel` sets
  TalkFlags[0x74] & 8), and what TalkFlags[0x74] & 1 and TalkFlags[0x60] & 4 mean (set by
  conversation scripts, not traced here).
- The value of the uninitialised `installed` and `n` in practice (whatever the stack or
  register held).
- What `CON_printf` prints for the `%/` in the gun setting prompt.
- Whether `InsertObj` checks weight when `UseTakeAndReady` takes an object.
- The game data was not searched for which books carry which `MDTEXT.LZC` entry, which
  maps which `MDMAP.LZC` picture, or which cabinet has which quality.

## The map and machinery

This section covers the world itself: its coordinates and levels, the entrances between
levels, the squares that turn the controls, the plaques, who can stand where, the squares
and objects that hurt, and the machines of Mars (power, levers, doors, conveyor belts,
coal workers, barges, drawbridges, jetters, canals). The machines are new code with no
Savage Empire counterpart; their state is kept in a few talk flags. The cut scenes the
levers start are described in *Cut scenes and the endgame*, the use actions that are not
machinery in *The use command*.

### Coordinates and wrapping

- **The surface is 1024 x 1024 tiles and both axes wrap** at 0x400. Every sum of
  coordinates is masked with `& 0x3ff`. Differences go through `WrapDelta` (map.c), which
  brings them into -0x200..0x200, or `MapDistance` (map.c), which gives the shorter way
  round (`d > 0x200` becomes `0x400 - d`). Unlike SE, there is no world edge.
- **The underground levels are 256 x 256 tiles** (32 x 32 chunks of 8 x 8). There the
  coordinates wrap at 0x100: `MapCoordMask` is 0xff when `MapZ` is not 0
  (`MAP_BuildArea`, map.c) and `MoveObj` (object.c) masks with 0xff for any z other
  than 0.
- The 40 x 40 area loaded around the party starts at `AreaX = (MapX - 0x10) & 0x3f8` (same
  for y) and may cross the map's edge (`AreaFirstCol`/`AreaLastCol`, `MAP_BuildArea`).

### Levels (MapZ)

The "map" file holds the surface (64 superchunks of 16 x 16 chunks) and then 5 levels of
0x600 bytes from offset 0x6000 (`MAP_LoadChunkIndexes`, map.c; the file is 0x7e00 bytes),
so MapZ runs 0-5, plus the pseudo-level 6.

| MapZ | What the code does with it |
|---|---|
| 0 | The surface: sky bar with the sun, day/night light, the plaques and the sextant work only here (`DrawSkyBar` gfx.c, `PassTime`, `UseSextant` useact2.c). |
| 1-5 | Underground: black sky bar, ambient light 0, area song 2 (`MUS_AreaSong` sound.c). The Peer map floods from the party only on levels 1-4 (`PeerMap` gfx.c; why not 5 is not known). The party wakes from defeat at (0xa4, 0xc3, 4) once TalkFlags[0x46] & 8 is set ("Olympus mine", `ResurrectParty` map.c). |
| 2, 3 | Also the dream's levels: gray sky bar and area song 3 (`DrawSkyBar`, `MUS_AreaSong`); the telescope works here (`UseTelescope` useact3.c). See *The dream*. |
| 6 | "Nowhere": the party is put there while it changes place (`PartyEnter` map.c), NPCs leave the world with `MoveObj(n, x, y, 6)`, NPC 0 waits there between dreams. |

### Entrances

**How they are used.** Only by stepping: after each step of the party leader,
`CheckGatesAndEntrances` (npctrack.c) looks at the objects on the leader's square. The
first one of these goes through `GoThroughEntrance` (map.c):

- 0xaf ("ground"), 0xa3 ("mine exit") or 0xd2 ("ice"), any frame;
- 0xb2 ("stairs up") with frame 0-1, 0xb4 ("stairs down") with frame 2-3;
- 0xc5 ("chamber") with frame 3.

The party must be in party mode: in solo mode the game prints "You must be in party mode
to leave." (0xa3, 0xb2) or "... to enter." (the others). The Use command has no action for
entrances.

**Where they lead.** By the entrance's quality, from `Entrances[24]` (map.c). A quality
outside 1-24 prints "Aargh!" and nothing happens.

| Quality | x, y, z | Quality | x, y, z | Quality | x, y, z |
|---|---|---|---|---|---|
| 1 | 0x043, 0x051, 1 | 9 | 0x0b3, 0x0e1, 4 | 17 | 0x07c, 0x028, 4 |
| 2 | 0x080, 0x08d, 0 | 10 | 0x27b, 0x1f1, 0 | 18 | 0x13e, 0x118, 0 |
| 3 | 0x076, 0x0f2, 3 | 11 | 0x0c3, 0x070, 1 | 19 | 0x00a, 0x060, 5 |
| 4 | 0x07a, 0x00c, 3 | 12 | 0x0cb, 0x1d1, 0 | 20 | 0x3b7, 0x1c4, 0 |
| 5 | 0x09a, 0x040, 4 | 13 | 0x024, 0x0f1, 1 | 21 | 0x05c, 0x0b0, 5 |
| 6 | 0x2b2, 0x1cc, 0 | 14 | 0x31a, 0x232, 0 | 22 | 0x3dc, 0x1dd, 0 |
| 7 | 0x073, 0x040, 4 | 15 | 0x05c, 0x0f1, 1 | 23 | 0x05e, 0x029, 4 |
| 8 | 0x29d, 0x1ce, 0 | 16 | 0x34c, 0x25a, 0 | 24 | 0x227, 0x1ab, 0 |

**The chambers (0xc5)** use their own table, `ChamberDests[4]`, by quality 1-4, always on
the surface: (0x0ca, 0x295), (0x34d, 0x295), (0x13a, 0x17a), (0x2cd, 0x1fa). They need the
surface power (TalkFlags[0x73] & 0x10), else "The alien device doesn't seem to be
functioning."; quality 0 prints "Nothing appears to happen.", above 4 "Aargh!". What gives
a chamber its frame 3 is not in GAME.EXE's code (it may be its animation; not determined).

**What goes along.** Before the move the party is gathered and taken off the map
(`PartyEnter`): instantly for 0xa3, with object 0x13c shown and sound 0x22 for 0xc5,
else by walking to the leader and fading. Then:

- 0xaf and 0xd2: every object on the square north of the entrance moves to the square north
  of the destination; an object 0x1b9 (a pushable cart; LOOK has no name) among them
  gets frame 3.
- 0xa3: every object on the square south of the entrance moves two squares south of the
  destination.
- 0xb2, 0xb4, 0xc5: nothing goes along. Any other type prints "Aargh!" and the party comes
  back out where it was.

Combat is broken off first (`COMBAT_breakOff`). On MapZ 3, quality 3 also clears the
temporary objects and sets the dream's foe count to 6 unless TalkFlags[0x66] & 0x10 is set
(see *The dream*). The party then steps out around the leader (`PartyExit`).

### The 0x1d1 squares: turned controls

Object 0x1d1 (invisible; LOOK has no name) turns the controls when the leader steps on it
(`CheckGatesAndEntrances`). If its quality differs from `ViewRotation`, then
`ViewRotation = quality & 7`, sound 0x32 and "You feel dizzy...".

- The test is written `GetQual(o) != ViewRotation & 7`; by precedence it compares the whole
  quality, so a quality of 8 or more repeats the message on every step onto it (bug).
- `ViewRotation` (eighths of a turn) does not rotate the picture: no drawing code reads it.
  It turns the direction keys (`AdvanceDir = (AdvanceDir + ViewRotation) & 7`) and the
  mouse pointer around the leader (`RotCos`/`RotSin`, console.c). The player's "north"
  becomes another direction.
- The dream's lever (quality 21, quantity 1) toggles it between 0 and 4 (see *Levers*);
  waking from the dream resets it to 0 (dream.c). It is saved with the game (gamestat.c).

### Plaques, latitude and longitude

Looking at a marker flag 0xac on the surface prints a place name by its quality 1-12
(above 12 "Aargh!", 0 no name), then its latitude and longitude (`CmdLook`, objcmd.c):

| Quality | Name | Quality | Name | Quality | Name |
|---|---|---|---|---|---|
| 1 | Coprates Chasma | 5 | Alber Tholus | 9 | Noctis Labyrinthus |
| 2 | Arsia Mons | 6 | Elysium Mons | 10 | Deuteronicus Mensae |
| 3 | Pavonis Mons | 7 | Hecates Tholus | 11 | Syrtis Major Planum |
| 4 | Ascraeus Mons | 8 | Terra Sirenum | 12 | Olympus Mons |

The 13th name, "Test Plaque", is shown by a brass plaque 0xe8 of quality 1 (above 1
"Aargh!").

- **Latitude** = (y - 512) * 240 / 1024, in long integers, truncated toward 0. Positive is
  printed "S", negative "N" (as its absolute value), 0 as "0".
- **Longitude**: lon = (x - 512) * 360 / 1024. Positive is printed as (180 - lon) "W",
  negative as (lon + 180) "E": x = 512 is the 180th meridian, x = 0 the 0th. Slips: at
  lon 0 (x = 510-514) the plaque prints "0" for 180; at x = 0 it prints "-180", then
  " 0E" on a new line.
- The sextant 0x60 (`UseSextant`, useact2.c) uses the same formulas ("You are somewhere
  near / Lat:%d %c / Long:%d %c"), showing 180 at lon 0 and 0 at lon -180, with no
  letter. It refuses to work off the surface.

### Who can stand where

`CanMoveTo` (canmove.c) decides whether a creature may be on a square; `NPC_TryStep`
(canmove.c) whether it may step there from its square (SE's `CanMoveTo`).

**Movement classes**, from the creature's class flags (`MonsterFlags`; NPCs 0x100 and up
and the types without a class walk):

| Flag | Class | Ground it accepts | Objects that stop it |
|---|---|---|---|
| 0x400 | flies (the wisp 0x179) | everything | NPCs |
| 0x100 | goes anywhere but walls (the airsquid 0x17a) | not `TerrainType & 4` | wall objects, other 0x100 creatures |
| 0x200 | swims (the canal worm 0x17b) | water (`TerrainType` bits 1 and 2, not 4 or 8), only while the canal pumps run (TalkFlags[0x76] & 0x20 and & 0x40) | anything that is not such water, floors (`TileFlag2 & 4`), NPCs not of class 0x100 |
| none | walks | not `TerrainType` 2, 4 or 8 | `TerrainType` 2 or 8 objects, NPCs not of class 0x100 |
| 0x8000 | (with walking) only on diggable ground (`GetDigClass`) | as walking, and a dig class | as walking |

- Swimmers need the pumps on every water square, not only in the canals. The code exempts
  ground tile 8, but tile 8 has `TerrainType & 4` set, so the exemption never applies
  (game data).
- Objects whose tile has `TileFlag2 & 4` (floors: barge pieces, drawbridge planks) let a
  walker stand over water; one with `TerrainType & 2` stops it. Decorations
  (`TileFlag2 & 0x10` without `TerrainType & 2` or `TileFlag2 & 4`) are ignored.
- The classes with 0x8000 are the giant maw 0x175, tentacles 0x16a, the monster footprints
  0x91, the dust devil 0x17d, the sand trapper 0x182, the tree 0x198, 0x1a8 (LOOK has no
  name) and the huge red spot 0x193.
- **Fences and walls of `TerrainType` 2** are passable from the sides whose bits are set:
  0x10 west, 0x20 south, 0x40 east, 0x80 north. A walker enters such a square only through
  an open side and leaves its own square only through one (`NPC_TryStep`).
- **Creatures of two squares** (tile flag 0x40: the other square above; 0x80: the other
  square to the left) need both squares free; the sextelleger 0x167, bushalo 0x168 and
  plantellope 0x169 use the square above when facing north or south and the one to the
  left when facing east or west. They cannot step diagonally. The wheelbarrow 0x10c and
  type 0x1b9 always count as one square, and pass over 0xaf objects.
- **Four-square creatures** (tile flags 0xc0) are tested on their own square only
  (`CanMoveTo` takes 0xc0 as "not two squares").
- `NPC_TryStep` sets `HazardFound` when the destination holds a damaging object; it blocks
  only while `AvoidHazards` is set. It also lets the leader push pushable objects (see
  *Objects and commands*; rail cars into water, see *Canals and rail cars*) and records
  whether NPC 1 (`LeaderOnBelt`) or another party member (`MemberOnBelt`) steps onto a
  conveyor belt 0xbc.
- NPC 0 may always be put on tile 0x6f (the dream; see *The dream*).

### Hazards

No map tile is damaging: no ground tile (0-0x1ff) has `TerrainType & 8` in the game data,
and `TileDamage` (gameloop.c) is empty anyway. The damaging tiles all belong to five
object types: the porcupod 0xa8, the steam leak 0xd1, the live power cable 0xd7, the dust
devil 0x17d and the invisible hazard 0x1cf.

**Damage** (`TerrainDamage`, gameloop.c). Invulnerable and dead actors are unaffected.
Damage goes through `LoseHP` (combat.c) with no armour and no experience; kind 3 burns
(no effect on class flag 0x08), kind 1 is cold, kind 0 may make the victim bleed (see
*Combat*).

| Object | Damage |
|---|---|
| 0xd1 steam leak | `OSI_rand(HP / 2, MaxHP / 2)`, kind 3. NPC 6 (Chsheket) takes none. |
| 0x17d dust devil | 2d10 (two `OSI_rand(1, 10)`), to an actor on the same square |
| 0xd7 power cable (live) | sound 0x15, 2d20 |
| 0xa8 porcupod | 1d20 |
| 0x1cf, by quality | see below |

**Object 0x1cf**: quality 0 picks 1-8 at random each time. Quality 0x14 prints "It's hot
here!" and does 1d20. For any other quality, nothing happens if NPC 0's hit points are below
2 (the code tests `HitPoints[0]`, probably meant for the victim; uncertain); otherwise it
does `OSI_rand(1, HP - 2)` to the victim (it cannot kill an actor with 3 HP or more) after
this show, centred on the party leader's square (the explosions and rays are pictures
only; their hit lists are ignored):

| Quality | Message | Sound | Kind | Picture |
|---|---|---|---|---|
| 1 | Fire! | 9 | 3 | |
| 2 | Frost! | 10 | 1 | |
| 3 | Fire bomb! | 0xb | 3 | explosion of tile 0x107 next to the leader |
| 4 | Fragmentation bomb! | 0xb | 0 | explosion of tile 0x106 |
| 5 | Steam jet! | 0x19 | 3 | a jet (tile 0x104) from up to 5 squares north |
| 6 | Heat Ray! | 10 | 3 | ray of tile 0x16b |
| 7 | Freeze Ray! | 10 | 1 | ray of tile 0x16a |
| 8 | Acid! | 0x1a | 0 | |
| other | none | | 0 | |

**When it applies** (`ApplyTerrainEffects`, gameloop.c). The first object on the square
whose tile has `TileFlag2 & 4` makes it safe; the first whose tile has `TerrainType & 8`
applies. Skipped: actors on another level, outside the loaded 40 x 40 area, and classes
with both flags 0x100 and 0x400 (no class has both, so none in practice).

- **Every `PassTime` call**, with or without minutes, for each actor 0-0xff. Since
  `CheckGatesAndEntrances` calls `PassTime(0)` after each step of the leader, standing on a
  hazard hurts on every step and every turn.
- **A blocked step**: when the leader fails to step, the square he tried to enter is
  checked (`PartyAdvance`, npctrack.c), so bumping into a dust devil hurts. (Damaging
  objects themselves do not block a step unless `AvoidHazards` is set.)
- **Entering by `MoveObj`** (object.c, while `MoveAppliesTerrain`): the call passes the
  area-relative coordinates (0-39) instead of the map ones (bug, marked in the source).
  The area test then fails except near the map's top-left corner, where the wrong square
  is checked. In practice entering a hazard does nothing until the next `PassTime`.
- `TerrainDamage` sets `NPCFlag & 8`, which spares that actor at the next `PassTime`.
- Followers avoid hazards: a damaging square scores 128 instead of 256 and is refused if
  they are in contact with the chain (`MoveFollowers`, npctrack.c), as in SE.
- The jetters are another hazard (see *Jetters*).

### The machinery's state

| Flag | Meaning | Set by | Cleared by |
|---|---|---|---|
| TalkFlags[0x73] & 4 | underground power: belts, jetters, furnaces, lamps 0x9a, doors 0xe3 | power lever 1 (quality 20, quantity 1), `BeltStart`, the stoker | power lever 5, a belt's breakdown |
| TalkFlags[0x71] & 8 | the underground power is not yet permanent | `BeltStart` | made permanent, power lever 5, breakdown |
| TalkFlags[0x73] & 0x10 | surface power: drawbridges, doors 0xb3, lamp posts 0xe4, tower tops 0xc9, chambers 0xc5 | power lever 2, replacing the cable (use action 8, `UseReplaceCable` useact1.c) | nothing |
| TalkFlags[0x76] & 0x20 | water in the canals (pumps, part 1) | power lever 3, cabinet panel 6 (`PanelRubyLenses` useact2.c) | nothing |
| TalkFlags[0x76] & 0x40 | the canals working (pumps, part 2; needs & 0x20) | power lever 4, cabinet panel 7, the control valve 0xdc (`UseValve` useact2.c) | nothing |
| TalkFlags[0x76] & 0x80 | both pump flags were set (`BargePumps`); read only by conversations | `BargePumps` | nothing |

The surface power needs the underground power to be permanent, and both pump flags are
needed for the barges, the swimmers and the canal effects (written "the pumps run" below).
No conversation sets these flags as far as dialogue.json shows; several test them.
`GAME_Save` clears TalkFlags[0x73] & 0x10 and TalkFlags[0x76] & 0x20 after writing the
file, but the game is reloaded right after a save, which reads them back (console.c,
gameloop.c).

### Power

**`MachineryOn`** (power.c) starts the animated tiles of the conveyor belts 0xbc and 0xbf,
the pump 0x1b0 and the drive wheel 0x1b3, and changes the map objects 0x101-0xbff:

| Object | On | Off (`MachineryOff`) |
|---|---|---|
| lamp 0x9a | frame 1 | frame 0 |
| tower top 0xc9 | frame \| 4 | frame & ~4 |
| power cable 0xd6 | becomes 0xd7 (live, damaging) | 0xd7 becomes 0xd6 |
| furnace 0xe9 | frames 1/3 become 5/7 | 5/7 become 1/3 |
| crack 0xd0 | a steam leak 0xd1 (frame 1, kept) is added south of it unless one is there | every 0xd1 is deleted |

**The lamp posts 0xe4** take frame 7 at night (hour < 6 or > 17) with the surface power,
else frame 3, at each hour passed (`PassTime`) and when an area is loaded;
`LampsAtNight` (power.c) lights them at once when the surface power comes on. The tower
tops 0xc9 also light when drawn with the surface power (`ShowObjects`, object.c).

**Keeping in step.** Only the loaded objects are changed by `MachineryOn`, so the others
are fixed when they come in or go out:

- Loading an area (`__ObjectsDeserialize`, object.c) sets 0xe4 as above, 0x9a to frame 1
  or 0 and 0xc9's bit 2, all by TalkFlags[0x73] & 4 (0xc9 too, though `ShowObjects` lights
  it by & 0x10).
- Saving (`OBJ_CommitBlocks`, object.c) reloads the area and adds the missing 0xd1 objects
  south of every 0xd0 with the underground power, or deletes every 0xd1 without it.
- Loading a game (`GAME_Load`, console.c) stops the machine animations without power and
  redoes the 0x9a, 0xd0/0xd1 and 0xe9 changes.

**Machine sounds** (`ShowObjects`). The nearest of these objects within the view plays a
looping sound (`MachineTypes`/`MachineSfx`): the dust devil 0x17d (sound 0x25), steam leak
0xd1 (0x18) and live cable 0xd7 (0x15) always; the furnace 0xe9, belt 0xbc, pump 0x1b0 and
drive wheel 0x1b3 (0x16, 0x17, 0x17, 0x17) with the underground power; the tower top 0xc9
and cable 0xd6 (0x14) with the surface power. In the dream's stage 0x40 sound 0x25 plays
instead.

### Levers

Levers are the lever 0xc4 and the switches 0x137 and 0x138 (use action 24). `UseLever`
(lever.c) goes by the lever's quality; other qualities print "What an odd lever!".

**Quality 1 and 2** (`LeverObjects`): the door 0xb3 or 0xe3 whose quality & 0x7f equals the
lever's quantity (the last one in object order), and up to four levers 0xc4/0x137/0x138 of
quality <= 2 with the same quantity, which all turn together (frame 0 or 1). No door:
"Strangely, nothing happens."; no lever: "Strangely, it doesn't move.". Sound 0x11.

- Quality 1 toggles the door's bit 7 (the lock that stops it opening by itself and makes
  Look say "It is apparently locked.").
- Quality 2 opens or closes the door (frames 1/3 to 9/11 or back, through 5/7, with the
  machinery sound). It should refuse with "the lever seems stuck." without the door's
  power, but the test reads the type of object 0xbff, the last one of the search loop,
  instead of the door's (bug noted in the source); unless that slot is a door it never
  refuses. In the dream's stage 0x44, closing a door can end the minotaur trap (see *The
  dream*).

**Quality 10**: a drawbridge (see *Drawbridges*).

**Quality 20** (`LeverMachinery`), by quantity. The lever turns (frame ^ 1) with sound 0x11
unless noted.

| Quantity | Effect |
|---|---|
| 1 | Underground power: if off, on for good (`MachineryOn`) and cut scene 2. If temporary, made permanent and cut scene 2. If permanent: "Sorry, boss, the power is on for good." |
| 2 | Surface power: needs permanent underground power ("You can't turn the surface power on until the underground power is permanently on."); if already on, "Sorry, boss. The surface power can't be turned off." (lever does not turn); else on, lamps lit, cut scene 3. |
| 3 | Pumps part 1: "The ruby lenses are already aligned." if done; else sets bit 7 of TalkFlags[0x2e], [0x39], [0x30], TalkFlags[0x76] & 0x20, `BargePumps(1)`, cut scene 6 (as the ruby lens panel, without its time puzzle). |
| 4 | Pumps part 2: "The canals are already working." or, without part 1, "There is no water pressure in the valve system."; else TalkFlags[0x76] & 0x40, `BargePumps(0)`, cut scene 7. |
| 5 | Temporary power on (`BeltStart`); if it is temporary: "Power prematurely cut off." and off; if permanent: "Power is already permanently on." (lever does not turn). |
| 6 | If object 6 is still type 0x165 (metal woman): it becomes 0x162, TalkFlags[6] & 2, cut scene 8; else "Chsheket has already been treated.". |
| other | "The lever doesn't do anything!" (does not turn) |

Which quality-20 levers the world holds is not in the code.

**Quality 21**: the dream's levers (`LeverDream`): quantity 1 toggles `ViewRotation`
between 0 ("You feel a strange disorientation.") and 4 ("The world returns to normal."), 2
is `DreamSleep`, 3 `DreamEnter`, others "Strangely, the lever does nothing at all!". See
*The dream*.

### Automatic doors and mirrors

**Automatic doors** (`ShowObjects`, on every redraw of the view): a door 0xe3 with the
underground power, or 0xb3 with the surface power, whose quality bit 7 is clear, reacts to
NPCs. Bit 1 of its frame gives its axis (0: it watches the squares north and south, 2: east
and west):

- An NPC on the door: frame 9/11 (open), with the machinery sound if it changed.
- Closed (frame < 4) and an NPC on a watched square: frame 5/7 (half-open).
- Half-open (4-7) and nobody on the watched squares: frame 1/3 (closed).
- Open (8 and up) and nobody there: frame 5/7.

The last three happen only while the door is on screen. Using such a door prints "It is
automatic and can't be opened by hand." and why it is not working: quality bits 6-7 both
set "It is apparently rusted shut.", bit 7 "It is apparently locked.", no power "It has no
power." (`UseAutoDoor`, useact3.c).

**Mirrors.** The reflective surface 0x12d (frames 0-2) shows the NPC on the square south of
it (`ShowObjects`): frame 0 for nobody, NPCs 0x70 and up, 0x5d (Smith) and 0x6a-0x6c
(Astaroth, Faulinei, Nosfentor); frame 1 for NPCs below 0x20 except 6, 0x2a-0x5c, 0x60,
0x69 and 0x6d; frame 2 for the others (NPC 6, 0x20-0x29, 0x5e, 0x5f, 0x61-0x68, 0x6e,
0x6f). Frame 1 looks like a human reflection and frame 2 a Martian one (uncertain).

### Conveyor belts

`BeltUpdate` (belt.c) runs after every step of the party leader (`PartyAdvance`), after the
Pass command, and once in `BeltStart`; not in `PassTime`. It does nothing without the
underground power. Every conveyor belt piece 0xbc in the loaded area moves what lies on it
one square: west when its quality bit 0 is set, else east. Eastward pieces are taken from
the east end first so that nothing moves twice.

- **First piece** (frame 2 for a westward belt, 0 eastward): when its quantity is 0 it puts
  a lump of coal 0xbd on the belt (kept; LOOK calls 0xbd "conveyor belt") and sets the
  quantity to 4; the quantity then drops by 1. So a lump every fourth update.
- **Last piece** (the other frame): deletes the 0xbd on it.
- **What moves**: the 0xbd lump, NPCs, huge lumps of coal 0x1bf and any object whose type
  weighs something (`TypeWeight > 0`). When the party leader rides, `MapX`/`MapY` follow
  and the area is reloaded. Nothing moves off the loaded area's west edge or east edge.
- **Followers on belts** (`MoveFollowers`): while the leader is on a belt, a follower's
  candidate square on a belt scores +112 (0x70), off one -112; while the leader is not,
  a belt square scores -112.
- **Breakdowns**: while the power is temporary (TalkFlags[0x71] & 8), each update except
  `BeltStart`'s stops the belt 1 time in 7 (`!OSI_rand(0, 6)`): "The conveyor belt
  stops.", both flags cleared, `MachineryOff`.
- `BeltStart`: "The coal begins moving down the belt.", sets TalkFlags[0x73] & 4 and
  TalkFlags[0x71] & 8, `MachineryOn`, one update.
- Fixing the bare rollers 0xc0 (use action 46, see *The use command*) turns them into belt
  pieces and sets TalkFlags[0x72] & 4.

### Coal workers

Two sets of NPC modes run the coal (`NPC_TakeTurn`, npctrack.c). The miner:

| Mode | What he does |
|---|---|
| 0x99 | Walks north. When blocked, with a vein of coal 0x1be two squares north: sound 0x10 if on screen, mode 0x9a. |
| 0x9a | With a belt 0xbc south of him: puts a huge lump of coal 0x1bf there, mode 0x9b. With bare rollers 0xc0 there: mode 0x9b with no coal. Else walks south. |
| 0x9b | Waits until no 0x1bf and no 0xc0 is south of him, then mode 0x99. |

The stoker:

| Mode | What he does |
|---|---|
| 0x9c | Mode 0x9d when a belt end 0xbf is two squares south; else walks south ("Stoker's path is blocked!" and sound 0 if he cannot, on screen). |
| 0x9d | Deletes every 0x1bf south of him, then mode 0x9e (waits until there is one). |
| 0x9e | Walks north; when blocked, uses the furnace 0xe9 north or north-east of him, then mode 0x9c (no furnace: the blocked message). |

**Stoking the furnace**: with TalkFlags[0x72] & 4 (rollers fixed), if the underground power
is off or temporary it becomes permanent (`MachineryOn`, cut scene 2); without the flag,
`BeltStart` (temporary power) and `SetFrame(lever, GetFrame(lever | 4))`, which reads the
frame of object number `lever | 4` (bug noted in the source; `MachineryOn` has already set
the furnace's frame).

### Barges

A barge (barge.c) is a set of martian barge pieces 0x1a1 and one barge controller 0x1a2,
all with the barge's number as quality.

| Field | Piece 0x1a1 | Controller 0x1a2 |
|---|---|---|
| quality | barge number | barge number |
| quantity | the party place it holds, 1-9 (0: none) | the number of objects of the barge, controller included |
| frame | | 0-3: moving north, east, south, west; 4 and up: stopped |

**Moving.** Each `PassTime` call that passes time collects the controllers in the loaded
area with frame below 4 (at most 16) and moves each barge one square, if the pumps run or
in the dream (`BargeAddController`, `BargeMoveAll`):

- The pieces of the barge within 6 squares of the controller are collected (at most 32
  objects). If there are fewer than the controller's quantity:
  "Found %d barge pieces when expecting %d!"; if more, the barge silently stays.
- Every piece's next square must be in the loaded area, a water tile (0x10-0x17,
  0xc0-0xc3, `IsWaterTile` typetest.c) and hold either a piece of the same barge (whatever
  else stands there) or only objects of types 0x1cd and up; a piece of another barge
  always blocks. If not, the barge stops: the controller is set to frame 4.
- In party mode outside the dream, a barge carrying some but not all of the party members
  does not move (no frame change).
- **Load**: the objects on its squares are counted, except floors (`TileFlag2 & 4`, which
  the pieces themselves are), tiles with `TileFlag & 0x10` and types 0x1cd and up. From
  412 (0x19c) objects: "The barge is riding low in the water."; at the list's limit of 512:
  "The barge is scraping the bottom of the canal." and it does not move.
- The pieces move, then everything on them. If the leader rode, the view follows.
- A controller that is no longer 0x1a2 prints "The barge creaks." (sound 0xc).

**The party aboard.** After each step of the leader, the Pass command, an attack and a
load, `BargeFindLeader` looks for a piece or the controller under the leader:
`LeaderOnBarge` takes its quality, and `CurrentBarge[i]` the piece of quantity i + 1 (the
leader's own square where none). `MoveFollowers` (npctrack.c) then walks each following
member to his place instead of the formation; a member whose place is the leader's square
takes the place `CurrentBarge[PartySize]`.

**Steering** (use action 23 on the controller, `UseBargeController` useact2.c): it needs the
pumps or the dream, else "the lever seems stuck.". It prints "Push lever to " and reads one
key or click. A direction sets frame `AdvanceDir >> 1` and prints the direction's name, so
a diagonal is rounded counterclockwise (north-east goes north) though its name is printed;
anything else prints "center." and stops the barge (frame 4). `ViewRotation` turns the
direction too.

**Pumps** (`BargePumps`): with TalkFlags[0x76] & 0x20 and & 0x40 it sets & 0x80 and makes
the water tiles' animation flow (`AnimFirstFrame` + 1); its argument 1 does the same to
tiles 1-4 (the canal beds, apparently). Loading a game redoes this.

**Debug dump**: debug key f6 (`BargeDump`) writes "=======BARGE INFO=========" and the
barge variables to a file "brg" in the current directory, then prints "LB: " and
`LeaderOnBarge`.

### Drawbridges

A drawbridge (bridge.c) is worked by its levers 0xc4 (quality 10, quantity the bridge's
number; at most 2 are turned) and starts from its posts, objects 0xc3 of frame 0 with the
bridge's number as quality (at most 5). Using a lever (`Drawbridge`):

1. Without the surface power: "There doesn't seem to be any power for the machinery.",
   sound 5.
2. No lever or no post: "Argh--drawbridge data inconsistent!".
3. The first post found gives the length (quantity bits 0-4) and the direction (bit 0x80:
   north-south). The bridge covers the posts' bounding box moved out: north-south, the
   rows from (top post's y - length) to (bottom post's y - 1); east-west, the columns
   from (left post's x - length) to (right post's x - 1). So it extends north or west of
   its posts.
4. Any object in that rectangle other than 0xc3: "The bridge doesn't move.", sound 5. A
   0xc3 there means the bridge is out.
5. The levers take frame 0 (bridge going in) or 1 (going out), sound 0x12.
6. **Extending** adds drawbridge objects 0xc3 (kept, quality = the bridge's number) one row
   at a time from the posts outward, about 63 ms apart: frame 3 on the eastmost column
   (north-south) or frame 2 on the southmost row (east-west), frame 1 elsewhere. "The
   drawbridge slides out to cross the canal."
7. **Retracting** deletes the 0xc3 objects of frame other than 0 row by row from the far end
   toward the posts. "The drawbridge retracts into its slot."

The planks are floors (`TileFlag2 & 4`), so the party walks over the canal on them.

### Jetters

A jetter is object 0x1d0 (invisible; LOOK has no name), run by `UpdateJet` and
`TriggerJet` (jet.c) for each jetter in the loaded area at each `PassTime` call that
passes time, only with the underground power.

- **Fields**: quality bits 0-3 the delay, bits 4-7 the jet's length; quantity bits 0-3 the
  countdown, bits 5-7 the direction (0 north, clockwise in eighths).
- **Firing** (`TriggerJet`): sound 0x19, a jet of tile 0x104 along `length` squares
  (`Missiles`). Every NPC hit and every breakable object (`IsBreakableType`: the closed door
  0xb3, the glow worm 0x184, the devil pod 0x133, the devil plant 0x12c, the glass pitcher
  0xd9) takes `OSI_rand(10, 50)`, kind 0, no armour. With a delay, firing reloads the
  countdown with the delay. `TriggerJet` refuses a jetter with both a delay and a
  countdown ("Nothing comes out."), which `UpdateJet` never asks for.
- **Counting down** (`UpdateJet`) is meant to decrement the countdown and fire at 0, or,
  with no delay, fire 1 time in `countdown`. As compiled it computes
  `counter = GetQuan(o) != 0` (1 or 0, the whole quantity byte). In effect:
  - with a delay: an update with a non-zero quantity clears the countdown bits; a jetter
    pointing north (quantity then 0) fires on the next update and then every second
    update; one pointing elsewhere never fires (its direction bits keep the quantity
    non-zero);
  - with no delay: it fires on every update while its quantity is non-zero, never when it
    is 0.

### Canals and rail cars

The canals are the water tiles; the pumps make them "run" (TalkFlags[0x76] & 0x20 and
& 0x40). The object 0x19a is a rail car (LOOK), which runs on track (`PushObj`, objcmd.c;
see *Objects and commands*).

**A rail car into the water.** The party leader can walk a rail car off its track into
water when nothing is on the water square (`NPC_TryStep` lets the push through although
`PushObj` refuses it). Every rail car push plays sound 0xc, and after a pause sound 0xd
when it lands on a track object 0x1a3. Then, when `MoveObj` (object.c) puts a 0x19a alone
on a water tile, it looks for the direction it came from. The search stops at the first
direction whose step matches the move on either axis (the test uses `&&` where `||` seems
meant), so the direction is rough.

- If none matches (it moved two or more squares on both axes): "BOOM!!!" and it stays.
- Otherwise, after a short pause, it is deleted:
  - **pumps not running**: a sweeping sound; it drifts one square with the current (tiles
    0x10 and 0x12 south-east, 0x11 and 0x13 south, 0x14 and 0x15 east), then a smashed rail
    car 0x1a0 is put on the first free shore square (tile 0x16 or 0x17 with no object) among
    the 8 around, starting from the push direction (none if there is no such square);
    "Crash!!".
  - **pumps running**: the splash sound and "Splash!!".

**Objects dropped, thrown or pushed into open water** (water with no floor object, any
object but a plank 0x18b) are lost; NPCs cannot be (see *Objects and commands*). The
sound is the splash when the square is a water tile and the pumps run, or a tile 1-4 with
TalkFlags[0x76] & 0x20; else the sweeping sound. For a push (`CmdMove`) the code tests the
tile under the pusher instead of the target square (slip).

**Swimmers** need the pumps to move on any water (see *Who can stand where*); the canal
worm 0x17b is the only swimming class.

### What could not be determined

- What each underground level (1, 3, 5) is, and why the Peer map treats level 5 as open
  ground; which entrance objects hold which quality.
- What gives a chamber 0xc5 the frame 3 that makes it an entrance.
- Which quality-20 levers exist in the world (the object data was not searched).
- Whether `HitPoints[0]` in `TerrainDamage` (object 0x1cf) is a slip for the victim's hit
  points.
- The reading of the mirror 0x12d's frames (human, Martian, none) is a guess from the NPC
  numbers.
- Whether `Missiles` can list the same NPC twice for one jet (it would be hurt twice).

## The dream

Savage Empire has nothing like it. When the avatar sleeps in a dream machine, a copy of
him, NPC 0, goes alone into the dream world: on MapZ 2 he frees eight Earth people caught
in their nightmares, on MapZ 3 he faces the Shadowlords and Raxachk. This section covers
going in, the dreamer, what changes while dreaming, the dream teleporters and the stages
they set up, the dream's creatures, and every way of waking. The code is in three overlays:
`dream.c` (going in, triggers, waking), `dreamobj.c` (passing out, fire chains, A.I. mode
0x18) and `dreamstg.c` (the stages). The cut scenes are in *Cut scenes and the endgame*;
the obelisk dream that raises a level while resting is in *Time, rest, stats and levels*.

### Going in

#### The dream machine

A dream machine is a group of objects around a cabinet 0x1c9 of quality 4. Using its panel
0x1ca on the cabinet (use action 45, see *The use command*) runs `PanelDreamMachine`
(useact2.c), which looks at the 11x11 squares around the cabinet for: the chair 0x122
(LOOK: power unit) at frame 1; the seat 0x121 (a dream machine), whose square the sleeper
stands on; the headgear 0x64 (a dream machine), made by using a headgear 0x128 on the
machine (`UseInstallHeadgear`); and 0x120 (a dream machine), whose quality is the setting.
The checks, in order:

1. TalkFlags[0x46] & 8 (Raxachk shot, see *Waking*): "The dream machines seem to have
   ceased functioning."
2. No chair at frame 1, no 0x120 or no 0x64: "The machine does not work."
3. An unconscious Martian 0x123 or a metal woman 0x11f on the seat: cut scene 5 (NPC 0x21)
   or 12 (NPC 6), see *Cut scenes and the endgame*.
4. The sleeper is the first living NPC numbered 1 or more on the seat's square
   (`OBJ_FindNPCAt(1, x, y)`); none: "There is nobody sitting in the machine." (With no
   0x121 the seat's x, y are never set.)
5. MapZ not 0 (a machine inside the dream): `DreamWakeUp`. NPC 0 is never found as the
   sleeper, so this needs another NPC on the seat.
6. Setting 0 or 1: TalkFlags[0x60] |= 8 (NPC 0x60, Marcus), then as below.
7. Setting other than 3: `DreamSleep(sleeper, setting)`. The dreamer starts at (0x64, 0x3b,
   2) for settings 0-1, at (0x93, 0x34, 2) for the others.
8. Setting 3: `DreamEnter` if TalkFlags[0x46] & 1 (NPC 0x46, Rasputin), else "The machine
   does not work." `DreamEnter` ignores the sleeper: anyone on the seat sends the
   avatar's dreamer in, at (0x35, 0x26, 3).

In the initial map (`SAVEGAME/OBJBLKAB`) the surface machines around (0x3f-0x5a,
0xa5-0xae) have settings 0, 1 and 3. A lever of quality 21 (`LeverDream`, lever.c) does,
by its quantity: 1 toggles ViewRotation between 0 and 4 ("You feel a strange
disorientation." / "The world returns to normal."); 2 `DreamSleep(Party[Active], 0)`; 3
`DreamEnter()` with no flag test; other quantities "Strangely, the lever does nothing at
all!"

#### `DreamSleep` and `DreamEnter`

`DreamSleep(npc, how)` (dream.c) first sets the wake-up date WakeDay/WakeMonth/WakeYear:
today, or tomorrow when the hour is 8 or later (day 28 rolls over to the next month, month
12 to the next year). The dream ends at 8:00 that day.

If `npc` is not 1, that member does not enter the dream: "*Name* dreams until dawn while
the party waits." It is marked asleep, IsResting is set, `PassTime(60 - Time_M)` runs to
the next full hour, then `PassTime(20)` repeats while `Date_Y < WakeYear`, `Date_M <
WakeMonth`, `Date_D < WakeDay` or `Time_H < 8`. The member wakes; the morning song plays.

If `npc` is 1, `DreamSleep` plays cut scene 4; `DreamEnter` plays cut scene 11 and sets no
wake-up date (the last one stays). Then both:

1. End solo mode. NPC 0 becomes a copy of NPC 1 (`CloneObject`: strength, dexterity,
   intelligence, experience, level, modes, status), with `MaxHP(1)` hit points, ObjStatus
   & 0x18, NPCStatus good (0x40) and player-controlled (0x80) with bits 0-4 cleared,
   movement points = dexterity.
2. Move NPC 0 to its start and give it shape 0x157, or 0x158 when `PlayerIsFemale` (LOOK
   "yourself"), at frame 1; mode 2 (command). Clear its four talents and delete its
   inventory: the dreamer starts empty-handed.
3. Make the party the dreamer: `Party[Active = PartySize] = 0`, `Names[PartySize]` a copy
   of `Names[0]`. This is U6's "in a vehicle" state: `Active == PartySize` holds for the
   whole dream. The real party stays where it was.
4. Show the dreamer's equipment (StatusDisplay 0x92, StatusMember PartySize); turn the
   panel gray (ColorPaper 7, the rectangle (184, 128)-(311, 195) and the four text windows);
   GameFlags |= 0x10; AmbientLight = 6.
5. `DreamSleep`: DreamStage 0, WakeX = WakeY = 0, DrunkCounter 0, area song. `DreamEnter`:
   DreamStage 0xe1, WakeX/WakeY = (0x35, 0x26).

GameFlags, DreamStage, DreamFoesLeft, WakeX/WakeY, the wake-up date and ViewRotation are
saved with the game (gamestat.c); `GAME_Load` (console.c) restores the gray panel.

### The dreamer

NPC 0 has NPC 1's attributes, level and experience as they were when it went in, the
party's first name, and only what the dream gives it, mostly through dreamstuff 0x14b (an
object holding another, which turns into it when used, see *The use command*). Between
dreams it waits on MapZ 6.

Each time `CheckActiveMember` (npctrack.c) runs in the dream, NPC 0's curse, charm,
"dragged under" and ailments (NPCStatus 0x10, 8, 4, 2) are cleared and its alignment set
to good, after its death has been tested (see *Waking*). So poison never lasts. Every
trigger clears its three talents; they wear off as everyone's do (*Time, rest, stats and
levels*), and "A psychic power fades..." is printed in the dream for NPC 0 only.

### What changes in dream mode

GameFlags is an int32 of which only bit 0x10 ("in a dream", `IN_DREAM` in some modules) is
used. Its tests, and the tests of `Active == PartySize` (`IN_VEHICLE`), which is true in
the dream:

| Function (file) | In the dream |
|---|---|
| `STAT_ShowInventory` (status.c) | always the dreamer's; no buttons or combat mode; instead its talent icons and "HP:" |
| `MouseInPartyView`, `MouseInPortraitView`, `MouseInEquipView`, `DrawSlotCursor`, `CON_ReadCommand`, `CON_getch` (console.c) | only the equipment view: clicks and keys (`*`, `/`, F10, `+`, `-`, `` ` ``, `~`, F1-F*n*) for the other views, other members and the combat-mode button do nothing; a member chosen as target gives the dreamer's square |
| `ShowPortrait` (objcmd.c), `PortraitBackground` (gfx.c) | portraits on a plain background |
| `TALK_initTalk` (talk.c) | NPC 0 is the speaker (`TalkDriver(0, npc)`) |
| `mk_npcnum` (talkdr.c) | a script's NPC 1 means NPC 0: tests and changes aimed at the avatar act on the dreamer |
| `FindPartyMemberWithObj` (talk.c) | looks only in NPC 0's objects |
| `TALK_talkTo` (talk.c) | talking to yourself wakes you |
| `JoinParty`, `LeaveParty` (talk.c) | refused |
| `REST_Command` (rest.c), `UseBed` (useact1.c) | "You can't sleep while dreaming!" / "You can't sleep in a dream." |
| `GameLoop` (gameloop.c) | keys 0-9 (party and solo mode): "Not while dreaming!"; the debug heal heals NPC 0 |
| `PassTime` (gameloop.c) | the members' air, light fuel and cold are not checked, air bottles 0x83 are not used up, AmbientLight is not recomputed (stays 6) |
| `GrowPlant` (gameloop.c) | plants grow at random (see stage 0xa4) |
| `EGG_hatches` (egg.c) | no "You hear something to the *direction*" |
| `COMBAT_Kill`, `LoseHP` (combat.c) | DreamFoesLeft counted down; the stage-0x44 glassware; a hurt member redraws the status area |
| `CheckActiveMember` (npctrack.c), `PartyUnconscious` (sound.c) | NPC 0's death knocks the dreamer out (see *Waking*) |
| `MoveFollowers`, `RunNPCTurns` (npctrack.c), `PartyEnter`, `PartyExit` (map.c) | only NPC 0 moves and acts |
| `ShowObjects` (object.c), `UseLever` (lever.c) | stages 0x40 and 0x44 |
| barge.c, `UseBargeController` (useact2.c) | barges and their controllers work without the canal pumps; the "whole party aboard" test is skipped |
| `UseOpenPod` (useact1.c) | stage 0xa4 |
| `UseBucket` (useact3.c) | the bucket stays full after pouring |
| `UseEatBerries` (useact3.c) | redraws the dreamer's inventory |

MapZ 2 and 3 themselves, not the flag, give the gray sky bar (`DrawSkyBar`) and the area
song 3 (`MUS_AreaSong`).

### Triggers

#### Dream teleporters

The dream's paths are dream teleporters 0x1cd that the leader steps on
(`CheckGatesAndEntrances`, npctrack.c). If an obelisk 0x124 stands north of one, its
picture is shown over the map for a moment (looking at an obelisk shows picture *quality*
of `mdream.lzc`, `ShowDreamPicture`). The kind is `ObjStatus & 0xe5`. Kind 0xa5 does
nothing while NPC 0 has 4 hit points or fewer. Kind 0 is a plain teleporter: local objects
are purged and the party goes to (quality, quantity) on the same level. Any other kind
calls `DreamTrigger`.

The frame is the teleport mode (`PartyTeleport`, map.c): 0 moves at once, other values
fade the screen, 1 also plays sound 0x33. `OBJ_PurgeFarLocals(-1, -1, -1)` deletes every
local creature and object (ObjStatus 0x20) but the teleporters and lets the eggs 0x1d2
hatch again. The stages make their objects with `DreamAddMapObj` (`AddMapObj` clearing
ObjStatus 0x20), so purges spare them and each stage remakes them by type.

An object 0x1ce (also "dream teleporter") gives the dreamer shape 0x13e/0x13f ("yourself";
quality not 0) or NPC 1's original shape (quality 0); four stand at (0x5f, 0x17-0x1a, 2).

#### `DreamTrigger`

`DreamTrigger(obj)` (dream.c) takes `kind = ObjStatus & 0xe5`, `x` = quality, `y` =
quantity, `mode` = frame, then:

1. If DreamStage is not 0 and the kind is neither 0xa5 nor 0xe5, closes stage DreamStage:
   5 `DreamEndStage05` (deletes the creatures 0x17b and 0x193 in (0x76, 0x98)-(0xce,
   0xcc)); 0x20 deletes the dreamer's inventory; 0x40 gives the dreamer NPC 1's type at
   frame 9; 0x44 `DreamEndStage44`; 0xa0 `DreamEndStageA0` (gray panel again, inventory
   deleted but for dollars 0x18, area song).
2. Acts before the teleport, by kind (table). Kinds 1 and 0x24 stop here.
3. Unless the kind is 0xa5: purges local objects, DreamFoesLeft = 0.
4. Clears NPC 0's three talents; `PartyTeleport(x, y, MapZ, mode)`.
5. Acts after the teleport, by kind. Kinds 0xa5 and 0xe5 stop here.
6. `ShowObjects`; DreamStage = kind.

| Kind | Before the teleport | After the teleport |
|---|---|---|
| 1 | DreamStage = 0; no purge, no teleport | |
| 4 | `DreamStage04(x, y)` | |
| 5 | | `DreamStage05` |
| 0x20 | | `DreamStage20(x, y)` |
| 0x24 | `DreamWakeUp` | |
| 0x25 | | `DreamStage25(x, y)` |
| 0x40 | `DreamStage40` | |
| 0x44 | | `DreamStage44` |
| 0x60 | | `DreamStage60` |
| 0x85 | `DreamStage85(DreamStage != 0x85, x, y)` | |
| 0xa1 | `DreamStageA1` | |
| 0xa4 | NPC 0x56 set up (stage 0xa4) | |
| 0xa5 | (no closing, no purge) | DreamStage unchanged |
| 0xc0 | NPC 0 to `MaxHP(1)`, inventory deleted | the same; WakeX/WakeY = (x, y) |
| 0xc1 | TalkFlags[0x6b] \|= 4; the obelisk north of the trigger and the trigger deleted; 1 in 2 (`OSI_rand(1, 100) <= 50`) NPCs 0x6d and 0x67 swap squares and frames | |
| 0xc4 | inventory deleted | WakeX/WakeY = (x, y) |
| 0xc5 | TalkFlags[0x6a] \|= 4; obelisk and trigger deleted | TalkFlags[0x68] = 0 |
| 0xe0 | TalkFlags[0x6c] \|= 4; obelisk and trigger deleted | |
| 0xe5 | (no closing) | DreamFoesLeft = 6 unless TalkFlags[0x66] & 8; DreamStage unchanged |

Other kinds only close, purge, teleport and become DreamStage. DreamStage also takes 0xe1
(`DreamEnter`), 0xa0 (`DreamerPassesOut`) and 0xe4 (event 0x6b). `DreamWakeUp` calls
`DreamTrigger(0)`: NPC 0's own ObjStatus normally gives kind 0, so this closes the stage,
purges, and teleports NPC 0 to the place its own Amount gives, just before the wake-up
moves it away.

#### The dream's script events

The scripts drive much of the dream through `TALK_Event` (talk.c; see *Conversations and
quest state*). Events 0x34, 0x36 and 0x38 only leave their number in TalkEvent;
`TALK_talkTo` acts on it after the conversation.

| Event | Script | Effect |
|---|---|---|
| 0x33 | 0x5a Auctioneer | hitching post 0x4f at (9, 0x1f) to frame 1 |
| 0x34 | 0x5c Marshall | `DreamReleaseNpc(0x5c)`, then the dreamer wakes |
| 0x35 | 0x52 Wells | six monster footprints 0x91 in mode 8 (assault) placed around the party; DreamFoesLeft = the number placed |
| 0x36 | 0x52 Wells | `DreamReleaseNpc(0x52)`, then wakes |
| 0x37 | 0x56 Carver | Carver's pod (stage 0xa4) |
| 0x38 | 0x55 Lenin | counts the rubles (stage 0x60); then if TalkFlags[0x55] & 0xc is 8, `DreamReleaseNpc(0x55)` and wakes |
| 0x39 | 0x5a Auctioneer | NPC 0x5b (Clanton) revived from his body in (8, 0x18)-(0x16, 0x39), his flag bit 4 cleared |
| 0x65 | 0x6b Faulinei | obelisk 0x124 (frame 3, quality 0x0e) at (0x34, 0x20), teleporter of kind 0xc1 to (0x34, 0x0b) below it; a dreamer on the obelisk's square moves to (0x34, 0x22) |
| 0x66 | 0x6a Astaroth | obelisk (quality 0x0f) at (0x12, 0x22), kind 0xc5 to (0x1b, 0x1a) below it; TalkFlags[0x68] &= 1; bodies 0x155 at (0x14/0x16/0x18, 0xb) become 0x189 at frames 0-2 (LOOK: Dibbs, Nellie Bly, Sherman) |
| 0x67 | 0x68 Tarpch | his bolt kills the next figure (Dibbs, Nellie Bly, Sherman: each becomes a body of NPC 4, 3, 5's type); the fourth time the dreamer: both fall, then the party goes to WakeX/WakeY |
| 0x68 | 0x6c Nosfentor | obelisk (quality 0x10) at (0x6a, 0x2a), kind 0xe0 to (0x53, 0x28) below it; TalkFlags[0x69] \|= 1 |
| 0x69 | 0x67 Shartek | a door 0xdb opens ((0x3b, 0xc) when NPC 0x67 stands at x 0x2c, else (0x2b, 0xf)); NPC 0x67 takes shape 0x171 (a martian) |
| 0x6a | 0x6d Spector_ | the party to WakeX/WakeY |
| 0x6b | 0x68 Tarpch | the party to (0x6a, 0x2c), inventory deleted, wake-up place (0x6a, 0x2c), DreamStage 0xe4 |
| 0x6c, 0x6d | 0x66 Raxachk | combat begins, DreamFoesLeft = 6, the party to (0x6e, 0x17) / (0x7a, 0x0d) with the eggs forced to hatch; a bridge 0xe0 at (0x75, 0x17) / (0x7a, 0x12) if none |
| 0x6e | 0x66 Raxachk | combat begins, the party to (0x82, 0x18), eggs forced to hatch |
| 0x6f | 0x6c Nosfentor | NPC 0 to `MaxHP(1)` |

### The stages

#### The Hall of Dreams and its prisoners

The code knows stages only by number. Which prisoner each belongs to follows from the NPCs
it resets and from the initial object list of MapZ 2 (`SAVEGAME/OBJBLKBI`): nine obelisks
(quality = picture 1-9) in two halls, each with a teleporter (frame 1) south of it. The
settings 0-1 start the dreamer in the hall of pictures 2-5 and 9, the others in the hall of
1 and 6-8.

| Picture | Obelisk | Teleporter kind, to | Stages | Prisoner | Freed by |
|---|---|---|---|---|---|
| 1 | (0x91, 0x30) | 0x20, (0x24, 0x1b) | 0x20, 0xa0 | 0x53 Earp | Marshall's event 0x34 |
| 2 | (0x66, 0x3d) | 0x25, (0x39, 0x01) | 0x25, 0x40, 0x44 | 0x54 Tiffany | luring the minotaur out (stage 0x44) |
| 3 | (0x67, 0x38) | 0xa1, (0x72, 0x20) | 0xa1 | 0x52 Wells | his event 0x36 |
| 4 | (0x61, 0x38) | 0xa4, (0x0e, 0x4a) | 0xa4 | 0x56 Carver | opening the ripe pod (`UseOpenPod`) |
| 5 | (0x61, 0x3d) | 0x60, (0x55, 0x33) | 0x60 | 0x55 Lenin | his event 0x38 |
| 6 | (0x94, 0x30) | 4, (0x8b, 0x10) | 4 | 0x57 Clemens | the manuscript 0x58 used on the mailbox 0x54 (`UseManuscript`) |
| 7 | (0x94, 0x36) | 5, (0xcb, 0x9b) | 5 | 0x50 Lowell | the hand mirror 0x93 used in (0x76-0x78, 0xca-0xcc, 2) (`UseMirror`) |
| 8 | (0x91, 0x36) | 0x85, (0x33, 0x56) | 0x85 | 0x51 Melies | oil used on map tile 8, a rusted hatch (`UseOil`) |
| 9 | (0x6a, 0x3c) | 0, (0x35, 0xac) | | not known | |

Earp's script names the eight. The engine-side successes set one of the prisoner's flags
(0x40 for 0x50, 0x51, 0x57; 2 for 0x54; 0x88 for 0x56), make him talk, then call
`DreamReleaseNpc` and `DreamWakeUp`; the others follow the conversation (events 0x34, 0x36,
0x38). `UseMirror`, `UseOil` and `UseManuscript` do not test GameFlags.

`DreamReleaseNpc(npc)` (dream.c) moves the NPC to the place of the first entry of its
schedule on the current level, deletes the teleporter 0x1cd there and the obelisk north of
it ("Aargh!" if missing) and sets TalkFlags[npc] |= 0x80. For 0x5c (Marshall) it first
sends 0x5c to (0x5c, 0x5c, 6) and frees 0x53 instead. With 0x56, 0x54, 0x52 and 0x55 freed,
TalkFlags[0x60] |= 0x10; with all eight 0x50-0x57, TalkFlags[0x20] |= 0x10 (Tekapesh).

A stage that resets an NPC puts it at its place with frame 9 (1 for 0x55 and 0x56), mode 0,
TalkFlagsHi bit 7 set and alignment cleared.

#### Stages 0x20 and 0xa0 (Earp)

`DreamStage20(x, y)` (dreamstg.c), after the teleport: NPC 0x5b (Clanton), unless already a
cowboy 0x15b on MapZ 2, is made alive and whole at (0xd, 0x1d); talk flags of 0x59-0x5d
are cleared (0x5d & 0xa7, 0x5c & 0xe6, the others to 0) and their alignments too; 0x5b
wanders (mode 0x8f). The hitching post at (9, 0x1f) goes to frame 0. In (8, 0x10)-(0x1f,
0x37) bloody sabers 0x10, sabers 0x38, dreamstuff, blood 0x14e and bodies 0x155 are
deleted. The dreamer gets a dreamstuff holding a saber. DreamFoesLeft = PartySize - 1 (4
when alone), and that many proto martians 0x16c are made north, east, south, west of (x,
y) in that order; combat begins.

The last one's death (`COMBAT_Kill`, no experience) calls `DreamerPassesOut` (dreamobj.c):

1. "Overcome by fatigue, you pass out." Combat ends, the dreamer falls, a pause.
2. In (0x21, 0x19)-(0x27, 0x1f) (the dream's copy of the machine room), each 0x151 (a
   proto martian's body: LOOK "dead proto-martian") becomes a body 0x155 with the shape of
   the next companion (party members 1, 2, ..., or NPCs 2-5 when alone), moved by (-0x13,
   +0x18), holding dollars 0x18: `0x34 / (PartySize - 1)` + 0-3, or 13 + 0-3 when alone.
   The creatures were the companions.
3. In that rectangle sap 0x150 becomes blood, sabers become bloody sabers, and these,
   blood and every local object ("Moving *name* to new room.") are moved by the same step.
4. The dreamer's inventory keeps only its sabers (made bloody). The loop is buggy: it
   deletes the object after the one tested (`DeleteObj(obj)`, `obj` being the next) and
   walks on from the deleted one, so what survives depends on the order; it ends with
   `DeleteObj(-1)`.
5. The dreamer gets a copy of each object of NPC 1, readied ones readied; if it kept a
   readied saber, the copies for slots 2 (right hand) and 8 (two-handed) are skipped.
6. It wakes at (0x10, 0x33, 2) (a second copy of the machine room), NPC 1's type at frame
   11, the panel in its normal color: "You wake up!", the morning song, DreamStage 0xa0,
   "All your friends have been killed!"

GameFlags keeps 0x10: this is a false waking. The kind-1 teleporter at (0x14, 0x34) closes
stage 0xa0 (gray panel, all but dollars deleted). The rest is the horse sale of the scripts
(Auctioneer 0x5a, Clanton 0x5b, horses 0x59, 0x5c, 0x5d). Killing Clanton sets his flag bit
4 (`COMBAT_Kill`); event 0x39 revives him.

#### Stage 4 (Clemens)

`DreamStage04(x, y)`, before the teleport to (x, y). In the 0x60 x 0x40 squares from (x -
3, y - 8): each sandbar 0x78 is made invisible; dreamstuff (with its content) and objects
of types 1, 3, 7, 0x2c, 0x4b, 0x58, 0x65, 0x7a, 0xb8, 0x100, 0x116, 0x12d, 0x181, 0x1a1,
0x1a2 are deleted. A martian barge is built on (x, y - 2)-(x + 2, y): pieces 0x1a1 (frames
0-3, 5-8, quality 1) around the barge controller 0x1a2 (frame 4, quality 1, quantity 9).
NPC 0x57 (Clemens, flags cleared) stands on it at (x, y - 2). Each sandbar's quality puts,
by `DreamStage04Types`:

| Quality | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|
| Object | page 0x7a | berries 0x4b (4) | pith helmet 3 | silk scarf 7 | camera 0xb8 | rifle 0x2c | box of cigars 0x65 | Belgian combine 0x2d | sheepskin jacket 0x16 | weed sprayer 0x81 |

Qualities 0 and 11 put nothing; 12 or more would read past the table. The page lies loose;
the others come inside a dreamstuff.

#### Stage 5 (Lowell)

`DreamStage05`, after the teleport: NPC 0x50 (flags & 0xde) at (0xa5, 0xa4). The object
0x92 at (0xc9, 0x9b) is deleted, and in (0x76, 0x98)-(0xce, 0xcc) the types 0xf, 0x28,
0x93, 0x96, 0x97, 0x14e-0x150. Made anew on MapZ 2: hand mirror 0x93 (0xc1, 0x9c), Cupid's
bow and arrows 0x28 (0xc1, 0x9b), winged shoes 0xf (0xc7, 0x9a), a map 0x96 (quality 1)
and a note 0x97 (quality 8) at (0xc1, 0xa1). The dreamer's first talent becomes 2. A huge
red spot 0x193 at (0xb0, 0xa2) and a canal worm 0x17b (frame 15) at (0xa0, 0xa3) attack
(mode 8). Readying the winged shoes on MapZ 2 adds an object 0x92 at (0xc9, 0x9b)
(`STAT_Ready`, status.c).

#### Stages 0x25, 0x40, 0x44 (Tiffany)

- **0x25**, after the teleport to (0x39, 0x01): map tile 0x6f stops animating (its
  AnimTile entry, looked up once into DreamAnimEntry, gets mask 0), and "your mother"
  0x187 appears north of the dreamer in mode 0x15: each turn it steps south if it can,
  else strikes an enemy next to it (`__DoHit` with 6 hit points; `COMBAT_AI_MoveSouth`,
  combat.c). It is invulnerable; talking to it: "No backtalk from you, young man!" (or
  woman).
- **0x40**, at the trigger (0x39, 0x0a), before the teleport to (0x3c, 0x15): "You fall
  off the cliff!", the dreamer takes shape 0x126 (0x127 for a woman), the 0x187 within
  (MapX ± 1, MapY - 8 to - 4) is removed, and tile 0x6f animates again. While DreamStage is
  0x40, `ShowObjects` plays sound 0x25.
- **Tile 0x6f.** Every step of the leader onto map tile 0x6f calls `DreamAnimFaster`
  (dreamobj.c): DreamAnimSteps + 1 (at most 4; stage 0x40 resets it and calls it once);
  frame shift 4 - steps, mask 3 << shift, so the tile's four frames advance every 8, 4, 2,
  then 1 ticks; "...faster!" at steps 2-4.
- **0x44**, after the teleport to (0x27, 0x34): NPC 0x54 (flags & 0xbe) at (0x37, 0x3d).
  In (0x21, 0x33)-(0x3d, 0x43) types 0xa1, 0xa2, 0xd9, 0xda, 0x18e are deleted. Made anew:
  a red throw rug 0xa1 at (0x24, 0x36); 27 pieces of glassware 0xd9 (quantity 3, frame
  0-2 equally likely: glass pitcher, fine china plate, crystal glass) in the shop at x
  0x37-0x3b, y 0x39-0x41; a minotaur 0x18e at (0x3b, 0x41) in mode 0x16.

Readying the rug turns it into a red cape 0xa2 (`STAT_Ready`). The minotaur goes for the
cape, breaking what lies in its way. A glassware broken becomes broken crystal 0xda; when
none is left in (0x37, 0x37)-(0x47, 0x47), TalkFlags[0x54] |= 0x40 (`LoseHP`). While that
flag is clear, Tiffany is freed when the minotaur, at x >= 0x37 and y <= 0x37, is next to a
cape lying on the map (`COMBAT_AI_ChaseCape`), or when a door is closed with a lever of
quality 2 while it stands there (`UseLever`). Leaving the stage unfreed (`DreamEndStage44`)
sets his flag bit 3, or bit 4 if bit 3 was set; the minotaur is deleted.

#### Stage 0x60 (Lenin) and burning money

`DreamStage60`, after the teleport: NPC 0x55 (flags 0) at (0x4e, 0x4d). In (0x49, 0x4a)-
(0x57, 0x51) types 0x6f, 0x70, 0x84, 0x1a9, 0x1aa are deleted. Made anew: lit candles 0x70
at the four corners (0x49/0x51, 0x4a/0x51); an azurite stone 0x1a9 at (0x4d, 0x50) under 9
rubles 0x84; 24 more piles of rubles: `j + k + 2` at (0x49 + j, 0x4a + k) for j = 0, 2, 6,
8 and k = 2, 4, 6; 10 at j = 1, 3, 5, 7 and k = 5, 7; 11 at (0x4a, 0x4b), (0x4a, 0x4d),
(0x50, 0x4b), (0x50, 0x4d). That is 25 piles, 253 rubles.

Lenin's event 0x38 counts: TalkFlags[0x55] |= 0x18, &= ~4. In (0x49, 0x30)-(0x57, 0x51)
each pile clears bit 4, and bit 3 if its quantity differs from the first pile's; an
azurite or heart stone there or on the dreamer sets bit 2; rubles on the dreamer clear bits
3 and 4 and set bit 5. Bit 3 is cleared unless there are 0 or 25 piles, and unless they
hold 10 each. Bit 3 set and bit 2 clear frees Lenin: 25 piles of 10, the 3 extra rubles
neither in the area nor carried, no stone. As the code reads, no rubles at all also passes.

`BurnMoney` (dreamobj.c), from a lit light source used on rubles (`UseBurn`): "The paper
money flares into a great pyre!" Rubles on the map become a fire 0x85 and start a chain;
rubles in an inventory are deleted and NPC 0 loses 1-10 hit points (burning), whoever
carried them, even outside the dream. In the chain a fire going off (`FireBurnAt`) hurts
by 1-10 (burning) each creature on its square whose class has flag 0x80 or 0x4000,
deletes the burnable objects there, and for each pile of rubles deleted lights a fire of
the next generation on every neighbouring square holding rubles. `FireSpread(q)` sets off
the fire, then every fire of quality q in the world (objects 0x101-0xbff), then starts
the next round from the first fire of quality 1 - q, until none is left: touching piles
all burn. `FireBurnAt` skips class 0, which is the male dreamer 0x157; a female dreamer
(0x158, class 1, flags 0x6000) is hurt. Any other fire 0x85 on the map of the right
quality would go off too.

#### Stage 0x85 (Melies)

`DreamStage85(first, x, y)`, before the teleport; `first` when DreamStage was not 0x85.
First: the dreamer's inventory is deleted; NPC 0x51 (flags 0) at (0x39, 0x65) and NPCs
0xa-0xf (no scripts) at (0x3d, 0x66), (0x34, 0x66), (0x3c, 0x5e), (0x34, 0x5e), (0x3c,
0x56), (0x34, 0x56); every dreamstuff and potash 0xeb in (0x30, 0x50)-(0x3f, 0x67) is
deleted and a dreamstuff holding oil (0xeb, frame 4) put at (0x34, 0x50). Later triggers:
the dreamstuff or oil in the destination's 8x8 block is deleted and the first one in the
dreamer's 8x8 block moves to the destination block, at (+5, 0) when x > 0x3a and y >=
0x60, (+1, 0) for the block (0x38, 0x60), else (+4, 0): the oil follows the dreamer. The
initial map holds 71 more kind-0x85 teleporters there (x 0x31-0x35 and 0x3a-0x3e, y
0x51-0x66) and seven of kind 0x24 (wake) at x 0x39/0x3d, y 0x61-0x65.

#### Stage 0xa1 (Wells)

`DreamStageA1`, before the teleport: NPC 0x52 (flags 0) at (0x5f, 0x10). In (0x58,
8)-(0x7f, 0x2e) a carpet bag 0x59 keeps only an elephant gun 0x2e, an open barrel 0x68
(set to frame 1) only elephant gun rounds 0x3c, each given one (16 rounds) if missing;
loose guns and rounds, dreamstuffs holding them, and the paint and berries away from their
places are deleted. Made if missing: bag and barrel at (0x58, 0x20), a bottle of green
paint 0x77 at (0x7e, 0x21), berries 0x49 at (0x75, 0x21), dreamstuffs holding a weed
sprayer 0x81 (0x73, 0x21), a revolver 0x2a (0x75, 0x29) and 24 pistol rounds 0x39 (0x76,
0x2a). Wells' "invisible" foes come from his event 0x35; killing the last sets
TalkFlags[0x52] |= 2, and his event 0x36 frees him. (A weed sprayer loaded with paint
shows invisible creatures, *The use command*.)

#### Stage 0xa4 (Carver)

Kind 0xa4: TalkFlags[0x56] &= 1, NPC 0x56 at (0x19, 0x6f, 2). His event 0x37: Carver
leaves, locals are purged, the dreamer gets 4 wormsbane seeds 0x9e, a Martian ritual pod
knife 0xf2 and a bucket of water (0xa0, frame 1); a hole 0x101 holding a live Martian seed
0xfc, under a covered Martian seed 0xfb (quality 0xf), is made at (0x19, 0x6f); twelve
cave worms 0x16e (frame 1, mode 0x18), four from each of (0xa, 0x6f), (0x19, 0x60), (0x28,
0x6f). In the dream `GrowPlant` adds 1-2 to the seed's quantity at each `PassTime` call
that passes time; above 12 the plant takes its next frame and the count restarts, so each
of its three growth stages takes 7 to 13 calls; at frame 3 it is ripe (quality 0x10). The
knife on the ripe pod (`UseOpenPod`) sets it to frame 4 and brings Carver out.

#### MapZ 3: the Shadowlords and Raxachk

`DreamEnter` starts at (0x35, 0x26, 3), stage 0xe1. Kinds 0xc4 (initial map: (0x28, 0xf),
(0x3b, 0x11), to (0x12, 0x24)) and 0xc0 ((0x52-0x54, 0xd-0xe), to (0x7c, 0x16)) are
checkpoints: inventory deleted, the destination becomes the wake-up place, and 0xc0 also
restores NPC 0's hit points. Kind 0xa5 ((0x52-0x54, 0x18)) is refused at 4 hit points or
fewer.

- **Faulinei (0x6b)**: event 0x65 opens the way (kind 0xc1). The two Spectors, NPCs 0x6d
  and 0x67, are invulnerable: striking either sets TalkFlags[0x6b] |= 8 and sends the party
  to the wake-up place (`LoseHP`). Shartek (0x67) unmasked opens the door (event 0x69).
- **Astaroth (0x6a)**: event 0x66 (kind 0xc5). Tarpch (0x68) is invulnerable; striking him
  sets his flag bit 5 and he talks. His event 0x67 kills; his event 0x6b (healed) moves on.
- **Nosfentor (0x6c)**: event 0x68 (kind 0xe0). In stage 0xe0 `LoseHP` never takes the
  dreamer below 1 hit point and it does not bleed. Duprey_ (0x69) fights there,
  invulnerable.

The three triggers set the Shadowlord's flag bit 2, which his script answers with its
"disciple" lines; no script clears it. Raxachk (0x66) sets three combats (events 0x6c,
0x6d, 0x6e); each is won in stage 0xc0:

| Combat | Foes | Won when | Then |
|---|---|---|---|
| 1 | jumping beans 0x181, egg at (0x6d, 0x16) | DreamFoesLeft reaches 0, last foe 0x181 | egg deleted; a plain teleporter to (0x73, 0x17) at (0x70, 0x17); TalkFlags[0x66] \|= 0xa; Raxachk talks |
| 2 | ammonoids 0x16b, egg at (0x73, 0xec), reached by the mine entrance of quality 3 | the same, last foe 0x16b | egg deleted; TalkFlags[0x66] \|= 0x12; Raxachk talks |
| 3 | the giant maw 0x175, egg at (0x87, 0x17) | the maw killed on MapZ 3 | egg deleted; a bridge 0xe0 at (0x7f, 0x18); TalkFlags[0x66] \|= 0x22; Raxachk talks |

An entrance of quality 3 on MapZ 3 sets DreamFoesLeft = 6 and forces the eggs while
TalkFlags[0x66] & 0x10 is clear (`GoThroughEntrance`, map.c); kind 0xe5 ((0x72, 0x17))
sets it while TalkFlags[0x66] & 8 is clear. None of the three deciding kills gives
experience. The maw is in *Combat*.

#### DreamFoesLeft

Set by `DreamStage20`, events 0x35, 0x6c, 0x6d, kind 0xe5 and the entrance above (6: the
forced eggs are expected to make six); reset to 0 by every trigger but 0xa5, by
`PartyUnconscious` and `DreamWakeUp`. `COMBAT_Kill` lowers it when, in the dream with
DreamStage not 0, a hostile actor (NPCStatus & 0x20) dies that is not a tentacle 0x16a or
a pod devil 0x180. Reaching 0 acts only in stages 0x20 (`DreamerPassesOut`), 0xa1
(TalkFlags[0x52] |= 2) and 0xc0 (`DreamLastFoeKilled`, for 0x181 and 0x16b). It is also
lowered for each creature an egg fails to make or place ("monster *n* (0x*type*)
unplaced.", `EGG_hatches`) and for a rock worm dying for lack of room (`COMBAT_AI_Worm`).

### Dream creatures

Besides those of the stages and events above (proto martians, the red spot and canal worm,
your mother, the minotaur, the footprints, the cave worms, the arenas' eggs), the dream has
three A.I. modes:

- **0x13** (`COMBAT_AI_Rampage`, combat.c): loses 5 movement points; the stage-0x44
  minotaur then stops 1 time in 2 (`OSI_rand(0, 1)`), else shakes the screen. It steps in
  a random direction, and every other object on its new square, invulnerable ones
  excepted, takes `OSI_rand(1, 10) + OSI_rand(1, 10)` (kind 0).
- **0x16** (`COMBAT_AI_ChaseCape`, combat.c): deleted in stage 0x60. Its target is NPC 0 if
  it carries a red cape 0xa2, else the first cape on the map in the 40x40 loaded area;
  with neither, it acts as 0x13. Next to NPC 0 it strikes him (`__TryHit`, itself as the
  weapon) and loses 10 points; next to a cape on the map it may free Tiffany. Otherwise it
  shakes the screen, steps towards the target, deals `OSI_rand(1, 10) + OSI_rand(1, 10)`
  to every other object on its new square, and loses 5 points.
- **0x18** (`AI_SeekSeed`, dreamobj.c): its target is the first covered seed 0xfb within
  10 squares, else the middle of the view. Within 1 square of the seed: "The cave worm eats
  the pod plant!", the seed is deleted, 10 points lost. Otherwise it scores its square and
  its 8 neighbours (`DreamStepScore`; a neighbour wins when higher, or equal and
  `OSI_rand(0, 1)` is 1), steps to the best, or in a random direction when the best score
  is below 0, and loses 5 points.

`DreamStepScore` for a square at distance *d*: `200 - 15 * d`, +50 when *d* <= 1; if it
cannot step there avoiding hazards, -50 when it can with hazards, else -1000 for terrain &
6, -100 for terrain & 8, -50 otherwise; then, for each object in the 3x3 around, -5 per
wormsbane plant 0x9d at frame 0 (one at frame 1 or more: -1000 at once) and -1 per
wormsbane seeds 0x9e. Grown wormsbane keeps the worms off.

### Waking

#### `DreamWakeUp`

`DreamWakeUp` (dream.c):

1. `DreamTrigger(0)` (closes the stage). Active = 0. NPC 0 leaves the player's control,
   loses its inventory, gets mode 0. ViewRotation = 0, DreamFoesLeft = 0.
2. The clock is set to 8:00 on the wake-up date: set, not run (one `PassTime(0)`); if the
   dream lasted past that time, the clock goes back.
3. Each party member's three talents are cleared. A poisoned member with more than 30 hit
   points loses 24-30 (`OSI_rand(24, 30)`); with 11-30 it is left with 1-10; with 10 or
   fewer, with 1. The poison stays.
4. The map is loaded around NPC 1; NPC 0 goes to MapZ 6. Normal panel colors, GameFlags &=
   ~0x10, StatusMember 0, a fade.
5. Control returns to the avatar (NPCMode[1] = 2, `SetPartyMode`); combat is broken off.
   "You wake up." and the morning song.

#### The ways out

| How | Function (file) | Result |
|---|---|---|
| a teleporter of kind 0x24 | `DreamTrigger` | wakes |
| freeing a prisoner | see *The stages* | wakes |
| talking to yourself (NPC 0 or 1) | `TALK_talkTo` (talk.c) | "You try to wake yourself up."; wakes unless on MapZ 3 |
| a conversation leaving TalkEvent 0x34, 0x36, or 0x38 with Lenin's count passed | `TALK_talkTo` | wakes |
| a dream machine's panel inside the dream | `PanelDreamMachine` | wakes |
| knocked out with no wake-up place | `PartyUnconscious` (sound.c) | wakes |
| the M60 0x139 hitting NPC 0x66 | `COMBAT_FireM60` (combat.c) | wakes; the machines stop |
| knocked out with a wake-up place | `PartyUnconscious` | back to the wake-up place |
| striking NPC 0x6d or 0x67 | `LoseHP` | back to the wake-up place |
| the last proto martian of stage 0x20 | `DreamerPassesOut` | a false waking |

**Knocked out.** When NPC 0 dies (`COMBAT_Kill` deletes its inventory unless on MapZ 3),
`CheckActiveMember` calls `PartyUnconscious`, whose dream branch undoes the death and sets
DreamFoesLeft = 0. With WakeX not 0 (set by `DreamEnter`, kinds 0xc0/0xc4, event 0x6b):
NPC 0 gets `MaxHP(1)`, its poison is cured, it is moved to (WakeX, WakeY), combat is broken
off: "You feel your dream consciousness returning." In stage 0xc0, if TalkFlags[0x66] &
0x3c is 0x18 (combats 1 and 2 won, not 3), each giant maw in mode 0x12 at (0x87, 0x17) is
removed and the bridge at (0x7f, 0x18) made; then TalkFlags[0x66] |= 0x80 and Raxachk
talks (conversation 0x66: "What an inept worm!"). With WakeX 0 (every dream begun by
`DreamSleep`, so all of MapZ 2): combat ends, "You shake yourself awake from the
nightmare.", `DreamWakeUp`.

**Shooting Raxachk.** The M60 comes from the dreamstuff hoard 0x1c7 at (0x79, 0x19, 3):
with the second (psychic) talent, using it gives a dreamstuff holding the M60 while the
hoard's quality is not 0 (then 0), else a random object 0x31-0x38 (`UseDreamstuffHoard`).
When the M60's shot hits NPC 0x66: cut scene 9, `DreamWakeUp`, TalkFlags[0x46] |= 8. The
machines then refuse ("The dream machines seem to have ceased functioning."); the dream's
levers do not test the flag.

### What could not be determined

- Why `DreamWakeUp` calls `DreamTrigger(0)`, which teleports NPC 0 by its own quality and
  quantity just before it is moved away.
- What the object 0x92 ("something you shouldn't") at (0xc9, 0x9b) does in stage 5.
- Where the ninth obelisk's plain teleporter (to (0x35, 0xac, 2)) belongs in the story.
- The Shadowlords' flag bit 2: set on entering a trial, cleared by nothing.
- Whether the forced eggs always make six creatures, as DreamFoesLeft = 6 assumes.
- `DreamAnimFaster` runs on every step onto map tile 0x6f, in or out of the dream, and
  does not look up its AnimTile entry: before stage 0x25 or 0x40 has (DreamAnimEntry -1,
  e.g. after loading a game), it would write `AnimFrameShift[-1]`. Whether tile 0x6f can
  be walked on there was not checked.
- The stages' story meaning comes from the NPCs they reset, the scripts and the initial
  object lists, not from the code.

## Cut scenes and the endgame

The story's big moments are cut scenes: a few pages of pictures and text, after which the
code changes the world to match. This section lists the thirteen cut scenes of
`cutscene.c` (an overlay), what starts each one and what each changes, then the end of the
game: `GameEnd`, the endgame and credits that `MARTIAN.EXE` plays, and the other ways the
program ends. What the conversations do around these events is in *Conversations and
quest state*; the machinery behind cut scenes 2, 3, 6 and 7 is in *The map and machinery*;
cut scenes 4, 11 and 9 in their dream context are in *The dream*.

### How a cut scene plays

`PlayCutscene(n, pages, portraits)` (cutscene.c) shows cut scene *n* from
`MIDGAMEX.LZC`, where X is the n-th character of "123456789abcdef" (`MIDGAME1.LZC` to
`MIDGAMED.LZC`). Page *p* (from 0) is five entries of the file:

| Entry | Use |
|---|---|
| 5p + 4 | the picture, drawn over the map window. If it cannot be loaded, the cut scene stops there |
| 5p + 5 | a second picture drawn over the first, with `PlayerIsFemale` (0 or 1) as the driver's last argument; presumably the man's or the woman's figure (uncertain). Several pages have none |
| 5p + 1 to 5p + 3 | up to three texts, one at a time in the text window, each waiting for a key; a text starting with "*E" (the files hold "\*END\*") is skipped |

Entry 0 of every file is the word "bogus!" and is never read. During the cut scene
`IsInConversation` is set (so a death it causes prints no "killed" message, `COMBAT_Kill`
in combat.c), the status area shows mode 0x9E and the mouse pointer is hidden. Every
caller passes a portrait list of zeros, so no portrait is ever shown. `PlayCutscene` plays
no music itself; the scenes below say what they do with it. Afterwards most scenes redraw
the map and the status area and play the area song (`EndCutscene`, `MUS_PlayAreaSong`).

### The cut scenes

| # | Function | Pages (code / file) | Shows (in short) | Music |
|---|---|---|---|---|
| 1 | `Cutscene1` | 2 / 2 | prying the capsule's hatch open; the air rushing out | unchanged |
| 2 | `Cutscene2` | 3 / 3 | Stoker feeds the furnace; the underground complex comes to life | unchanged |
| 3 | `Cutscene3` | 2 / 2 | the power tower's cable replaced | unchanged |
| 4 | `Cutscene4` | 2 / 2 | settling into the dream machine; a hall of obelisks | song 3 from the 2nd page |
| 5 | `Cutscene5` | 3 / 3 | the Martian body in the dream machine; Prektesh wakes in it | unchanged |
| 6 | `Cutscene6` | 3 / 3 | the ruby lenses focus the sun on the ice cap; the cisterns fill | unchanged |
| 7 | `Cutscene7` | 3 / 3 | Chsheket opens the valves; water flows into the canals | unchanged |
| 8 | `Cutscene8` | 3 / 3 | Chsheket sprayed in the transformation chamber; she looks human | unchanged |
| 9 | `Cutscene9` | 2 / 2 | Raxachk defeated; the phlogistonite loaded; the dreamer wakes | stopped |
| 10 | `Cutscene10` | 2 / 2 | Tekapesh leads the Elysium Martians back to the dream world | unchanged |
| 11 | `Cutscene11` | 1 / 1 | entering the dream again, somewhere else | song 15 |
| 12 | `Cutscene12` | 3 / 1 | the metal body in the dream machine; Chsheket speaks | unchanged |
| 13 | `Cutscene13` | 1 / 1 | a brief vision of Kansas | stopped |

`Cutscene12` asks for three pages but `MIDGAMEC.LZC` has one, so it stops after the first.

What starts each one (the condition, then what the caller itself changes around it):

| # | Trigger |
|---|---|
| 1 | Using a chisel (0x110) or pry bar (0x66) on the hatch (0x1AB), by the Avatar only, when TalkFlags[0x10] & 0x10 is set (else NPC 0x10 (Tesla) talks, with TalkFlags[0x10] \|= 4). After the scene: TalkFlags[0x12] \|= 0x48 and NPC 0x12 (Blood) talks; the hatch becomes a broken hatch (0x1AC, frame 0) moved one square east and south; TalkFlags[0x10] \|= 0x20; one minute passes (`UseOpenHatch`, useact1.c). |
| 2 | The power lever (`LeverMachinery` quantity 1, lever.c), or Stoker (NPC 0x71) pulling the lever by the furnace (0xE9) in his A.I. mode 0x9E when TalkFlags[0x72] & 4 is set (npctrack.c); neither when the power is already on for good. Either way TalkFlags[0x73] \|= 4 and `MachineryOn` run first (see *The map and machinery*). |
| 3 | The surface-power lever (`LeverMachinery` 2), or replacing the cable with pliers (0x35) while wearing rubber gloves (0x26) (`UseReplaceCable`, useact1.c). Both need the underground power on for good and set TalkFlags[0x73] \|= 0x10 and `LampsAtNight` first. |
| 4 | The Avatar (NPC 1) going to sleep in a dream machine (`DreamSleep`, dream.c: from cabinet panel 4 or the dream lever 2). |
| 5 | Cabinet panel 4 with an unconscious Martian (0x123) under the headgear, TalkFlags[0x61] & 0x10 set and TalkFlags[0x21] & 4 clear (`PanelDreamMachine`, useact2.c). After the scene: NPC 0x21 (Prektesh) talks; TalkFlags[0x21] \|= 4; NPC 0x61 (Prektesh) leaves the world (moved to level 6, A.I. modes 0, TalkFlagsHi[0x61] \|= 0xCB); NPC 0x21 is moved one square south of the headgear and **killed** (`COMBAT_Kill`); the 0x123 object is deleted. |
| 6 | The lens lever (`LeverMachinery` 3) unless TalkFlags[0x76] & 0x20, or cabinet panel 6 (the ruby lenses, `PanelRubyLenses`, useact2.c). Both first set TalkFlags[0x2E], [0x39] and [0x30] \|= 0x80 and TalkFlags[0x76] \|= 0x20, and call `BargePumps(1)`. After the panel's scene NPC 0x05 (Sherman) talks with TalkFlags[5] & 0x10 set for the conversation. |
| 7 | The canal lever (`LeverMachinery` 4), cabinet panel 7, or the control valve (0xDC, `UseValve`), when TalkFlags[0x76] & 0x20 is set and & 0x40 clear. All set TalkFlags[0x76] \|= 0x40 and call `BargePumps(0)` first. |
| 8 | `LeverMachinery` 6, or cabinet panel 5 (the sprayer system 0x114, `PanelRubberSprayer`, useact2.c), while NPC 0x06 (Chsheket) is still type 0x165 (the metal woman). Both make NPC 6 type 0x162 (a lady) and set TalkFlags[6] \|= 2; the panel also sets its original shape, and NPC 6 talks after the scene. |
| 9 | A shot of the M60 machine gun (0x139) whose missile hits NPC 0x66 (Raxachk) (`COMBAT_FireM60`, combat.c). After the scene: `DreamWakeUp` and TalkFlags[0x46] \|= 8 (see below). |
| 10 | Event 10, run by NPC 0x20's (Tekapesh's) script, after the conversation ends (`TALK_talkTo`, talk.c). |
| 11 | The dreamer entering the dream at its second place (`DreamEnter`, dream.c: cabinet panel 4 at setting 3 with TalkFlags[0x46] & 1, or the dream lever 3). |
| 12 | Cabinet panel 4 with the metal woman (0x11F) under the headgear and its quality not 0 (`PanelDreamMachine`). After the scene: if the party has five members, NPC 5 talks (with TalkFlags[5] & 4 set for the conversation); then NPC 6 talks, is moved one square south of the headgear, NPC 0x63 (Chsheket) leaves the world (level 6, modes 0, TalkFlagsHi[0x63] \|= 0xCB) and the 0x11F object is deleted. |
| 13 | Answering N to the ruby slippers' question (below). |

The debug key 245 also plays cut scenes 1 to 9, with their world changes but without the
callers' (see *Debug keys*).

### What each cut scene function changes

Several functions move the map's centre to a place, load the area there, change objects,
and move back (`MAP_LoadAround`). Rectangles below are on level 0 and include both corners.
"Leaves the world" means `MoveObj(n, n, n, 6)`: moved to level 6, which is nowhere.
"Waits" means the wait flag (TalkFlagsHi bit 7, flag 15) set and A.I. mode 0, so the
schedules no longer move the NPC (`UpdateSchedules`, `StartScheduleAction`, npctrack.c).

**Cut scenes 1, 4, 5, 12, 13** change nothing themselves.

**Cut scene 2** (the underground power): in (0x3D0, 0x1D0)-(0x3F0, 0x1E7) every power
cable 0xD6 becomes type 0xD7 (frame kept; LOOK calls frames 2-3 and 6-7 of 0xD7 "sparking
cable"), and every tower top (0xC9) gets frame bit 2 (frame | 4).

**Cut scene 3** (the surface power): in the same rectangle every 0xD7 becomes 0xD6 again
(tower tops keep their frame). Every object 0x13B ("chamber" in LOOK) is deleted in the
four rectangles (0xC8, 0x290)-(0xD7, 0x2A0), (0x348, 0x290)-(0x357, 0x2A0),
(0x138, 0x178)-(0x147, 0x187) and (0x2C8, 0x1F8)-(0x2D7, 0x207).

**Cut scene 6** (the lenses): every ice object (0xD2, frame 0 or 1) in (0x13B, 0x116)-
(0x140, 0x11A) is deleted (frames 2-3 are "ice cave" and stay).

**Cut scene 7** (the canals): in the whole loaded area around (0x241, 0x27B), every object
of type 0x1CB at frame 1 ("phlogistonite cannister" in LOOK) gets frame 2 ("broken
straps"), and a new broken strap (0x1CC) is made at (0x243, 0x27A, 0) ("Aargh!" if it
cannot be).

**Cut scene 8**: around (0x384, 0x280): NPCs 0x41 (Maurice), 0x43 (Gordo), 0x44 (Miles)
and 0x45 (Fenton) are killed (`COMBAT_Kill`; no message, since `IsInConversation` is still
set); NPC 0x42 (Goldman) waits at (0x394, 0x284, 0); NPC 0x46 (Rasputin, his second script)
waits at (0x393, 0x282, 0); NPC 0x40 (Rasputin) waits and leaves the world. Why the
sprayer scene does this is not shown by the code.

**Cut scene 9** (the end of Raxachk): the music stops. Three phlogistonite cannisters
(0xEF) are made two squares west of the Avatar's body (NPC 1), at x − 2 and y − 2, y − 1,
y, on level 0 ("Aargh!" for each that cannot be). Then on level 4, in the Olympus mine
(see *Conversations and quest state* for waking there), NPCs are put back in two rows,
each waiting with the frame given:

| Row | NPCs | Frame |
|---|---|---|
| y 0xC4, x 0xA3-0xB2 | 0x05 (Sherman, only if not in the party), 0x10-0x16, 0x19-0x1C, 0x1E, 0x2A-0x2C | 9 |
| y 0xC2, x 0xA3-0xB2 | 0x2D-0x31, 0x46 (at 0xA8), 0x34-0x39, 0x3B, 0x3D, 0x3E, 0x42 | 1 |
| (0xA1, 0xBD) | 0x33 (Segal) | 13 |

The map and status area are not redrawn here; `DreamWakeUp`, which the caller runs next,
does that. The caller's TalkFlags[0x46] \|= 8 then marks the dream as over: the dream
machines stop working ("The dream machines seem to have ceased functioning.",
`PanelDreamMachine`), a knocked-out party wakes in the Olympus mine (`PartyUnconscious`,
sound.c), and Carnegie's script accepts the phlogistonite (below).

**Cut scene 10** (the Martians leave): NPCs 0x20 and 0x22-0x29 (Tekapesh, Bikchiz, Fazek,
Xichak, Chaktsaf, Pashesh, Cheshef, Sisik; 0x21 is skipped) wait and leave the world. NPCs
0x2A-0x31 (Lowell, Melies, Wells, Earp, Tiffany, Lenin, Carver, Clemens) lose their wait
flag and are moved to the place of their current schedule entry. Then, with the area
loaded around (0x64, 0x3B) on level 2 (the dream), NPCs 0x50-0x57 (the same eight names'
other scripts) wait and leave the world.

**Cut scene 11** loads the area around (0, 0, 0) and then the party's area again, and
changes nothing else; the purpose of the extra load is not known.

**`ShowMdMap(n)`**, in the same overlay, is not a cut scene: using a map (0x96) shows
picture n (the map's quality) of `MDMAP.LZC` until a key or click (`UseMap`, useact3.c).
The file has pictures 0 and 1 only; for any other quality nothing is shown.

### The end of the game

**The ruby slippers** (0x0C, `UseRubySlippers`, useact2.c): using them when not readied
readies them. Readied, the first two uses only print "Click." and raise their quality (0 to
1 to 2). From the third use on, the game says the slippers can take you home, offers the
Grand Finale, warns that afterwards the game exits to DOS and that play resumes from the
last saved game, and waits for Y or N (other keys are ignored). N plays cut scene 13 and
play goes on; the quality stays 2, so every later use asks again. Y calls `GameEnd`.

**Event 9** also calls `GameEnd`, after the conversation ends (`TALK_talkTo`). Its only
user is NPC 0x3A (Carnegie): once TalkFlags[0x46] & 8 is set, he takes the phlogistonite
(0xEF) from the party one barrel at a time, setting his flags 4 and 5 for the first two;
the third runs event 9. Without any he says all three barrels are needed. (From
`dialogue.json`; see *Conversations and quest state*.)

**`GameEnd`** (cutscene.c) starts song 12 and at once shuts everything down: it deletes the
temporary area files (`OBJ_DeleteTmpBlocks`, the `objblkXY.tmp` files), stops the sound,
frees memory, unhooks the interrupts, shuts the graphics driver, and runs
`martian.exe !` in an endless `execl` loop. **The game in progress is not saved.** (In SE
the last cut scene ran `SAVAGE.EXE` the same way, with the date and an ending code as
arguments; MD passes only `!`.)

**MARTIAN.EXE with `!`** (SRC_M/main.c) reads `CONFIG.MD` as usual, then plays `Endgame`,
fades the music, plays `ScrollCredits` and exits to DOS (exit code 0). `Endgame` first
reads the first 0x1CF1 bytes of `savegame\objlist`, which is the **last saved game**
(`GameEnd` saved nothing), only for the Avatar's shape: type 0x157 shows the man's figure,
anything else the woman's. If the read fails it skips straight to the credits (it does not
check that the file opened). It plays `mdd_mus.lzc` song 11 and six parts, with the text
lines of `scenetxt.lzc` item 3:

| Part | Pictures (`endgame.lzc`) | Text lines | Timing |
|---|---|---|---|
| 1 | 0, with the Avatar's figure | 0, 1 (two pages) | 6 s each |
| 2 | 1: the space bullet rising from the cannon, growing for 13 steps | 2 | 40 steps of 100 ms, then 2 s |
| 3 | 2, animated | 3, then 4 | 10 × 4 frames of 300 ms |
| 4 | 3, with the Avatar's figure | 5 | 6 s |
| 5 | 4 and `moongate.lzc` 1, with the Avatar's figure; a flash at step 40 | 6, then 7 | 80 steps of 100 ms |
| 6 | 5-8: five shapes crossing the sky with random jitter | 8, then 9 at the end | 63 steps of 50 ms, then 6 s |

The texts tell, in order: Segal's congratulations as the phlogistonite is loaded; the
launch from the cannon in Mount Olympus during a quake; the splashdown in the Atlantic and
the rescue; the heroes' welcome; the group photo with Tesla, Freud and the others, which
will reach the player's home a century later; and the dust storms that will erase
Martian civilization. **There is only one ending**: the man's or woman's figure is the
only thing that varies.

Esc (or Ctrl+E, Ctrl+Q, Alt+X) stops the endgame; the credits still follow. Another key
ends the current part early in parts 2, 3, 5 and 6, and only cuts the wait short in parts
1 and 4 (`Pause`). The credits (`ScrollCredits`) scroll the lines of `scenetxt.lzc` item 4
over `mars.lzc` to song 8; a key jumps 4 lines ahead, Esc stops them.

### Other ends

- **Event 0xFF** (`TALK_Event`, talk.c) deletes the temporary area files, resets the
  video mode and exits to DOS (exit code 0), without saving. No conversation script uses
  it (no `EVENT 255` in `dialogue.json`).
- **Defeat before the capsule is open**: when the party is knocked out outside the dream
  and TalkFlags[0x10] & 0x10 is clear, `PartyUnconscious` (sound.c) prints "Overcome by
  your wounds, you fall unconscious.....", plays song 11, then shuts down sound, memory and
  graphics, prints on the text screen through the BIOS that the character cannot be
  resurrected before the capsule door is open and to try again, and exits with code 4. The
  game is not saved. (This is the same flag bit that `UseOpenHatch` requires before cut
  scene 1; opening the hatch sets bit 0x20.) Otherwise the party wakes; see
  *Conversations and quest state*.

### What could not be determined

- What the second picture's driver argument does exactly (a frame choice, a flip or a
  colour mode); only that the sex is passed is certain.
- Why cut scene 8 kills and moves Rasputin's people, and why cut scene 11 reloads the
  area at (0, 0, 0).
- What the objects 0x13B removed by cut scene 3 are for (LOOK: "chamber"), and what the
  frames 9, 1 and 13 given in cut scene 9 look like.
- `GameEnd` starts song 12 and shuts the sound down in the next calls, so the song is
  probably cut off at once (not tested).

## Debug keys

`GameLoop` (gameloop.c) handles 23 key codes as debug commands. They are typed as Alt plus
the decimal code on the numeric keypad (213, 214, ...), as in SE. They work only while
**NPC 1's flag 6** (`TalkFlags[1] & 0x40`) is set. No engine code and no conversation
script sets it (only the debug key 242, which needs it), so it takes a saved-game edit:
byte 0x18F2 of `savegame\objlist`, bit 0x40.

Without the flag:

- 213 prints "That's an old, worn-out magic word!" (in SE, 213 showed the time without the
  flag);
- 242 prints the time and position, as 213 does with the flag (text only);
- every other code prints "Are you a god?" with sound effect 5.

With the flag (numbers typed at the prompts are read by `ReadHexNumber`/`ReadDecNumber`,
gameloop.c; hexadecimal digits are not checked, a wrong character gives a wrong value):

| Code | Effect |
|---|---|
| 213 (0xD5) | prints `TIME hh:mm` and the map centre `Xxxx Yyyy Zz` (hexadecimal), then shows the overview map (`PeerMap`, gfx.c) |
| 214 (0xD6) | goto: x (3 hex digits), y (3), level (1). A level other than 0 masks x and y to 0xFF, level 0 to 0x3FF; the level is capped at 6 (nowhere). The party teleports with the sound 0x33 (`PartyTeleport`, map.c: eggs hatch, `PassTime(0)`), and the area song plays. No check for the dream or a vehicle |
| 215 (0xD7) | `PassTime(60)`, then one round of NPC path searches. It sets `AllowNPCTeleport` around the search, but no code reads that variable |
| 221 (0xDD) | "Divine Flare--": aim with the cursor (range 8), any key fires missile 0x101 from the active member. Prints the start and end squares and "successful!" (sound 4) or "failed!" (sound 0). No damage is done. (`COMBAT_Missile` has no return statement; the value tested is presumably `Missiles`' result, 1 when the missile reached its target) |
| 222 (0xDE) | toggles `AvatarNotSolid`: "Avatar desolidifies!" / "Avatar solidifies!" (sound 0x32). While set, `CanMoveTo` (canmove.c) lets the **active** party member move anywhere. The variable is in the saved game-state block (file offset 0x1D08), so it survives saving and loading |
| 223 (0xDF) | go to NPC n (2 hex digits): if n ≤ 0xBF and not dead, prints its name and teleports the party to (x + 1, y + 1) on its level, without the animation; else "invalid" (sound 5) |
| 224-229 (0xE0-0xE5) | accepted as debug codes but do nothing: "what?" (sound 5). (SE's 224 was "go to NPC") |
| 232 (0xE8) | NPC n (2 hex digits): prints its name and shows its portrait. No range check |
| 233 (0xE9) | if sound is on: sound effect nn (2 decimal digits); 14, 19 and 35 play the sweep, machinery and splash sequences (`MUS_SweepSfx`, `MUS_MachinerySfx`, `MUS_SplashSfx`), others `MUS_PlaySfx` (0-55) |
| 234 (0xEA) | if sound is on: keeps sound effect nn looping (`MUS_PlayLoopSfx`; values of 56 and up do nothing) |
| 242 (0xF2) | talk flags: NPC (2 hex digits); prints its name and `TalkFlags` byte; bit (1 hex digit): 0-7 is that bit of `TalkFlags`, 8-F all mean the wait flag (TalkFlagsHi bit 7); prints "on." or "off."; new value (1 hex digit, 0 = off, anything else = on) |
| 243 (0xF3) | talk to NPC n (2 hex digits) as the Talk command does (`TALK_talkTo`), events included |
| 245 (0xF5) | "Do midgame:" n (2 decimal digits): runs `Cutscene1`-`Cutscene9`, world changes included (so 8 kills again and 9 makes three more phlogistonite cannisters), but none of the trigger's own changes; other numbers "None!". Cut scenes 10-13 cannot be played |
| 246 (0xF6) | writes the barge tables to a file named `brg` (`BargeDump`, barge.c) and prints `LB:` and the barge the leader stands on (`LeaderOnBarge`) |
| 249 (0xF9) | "Analyze what?": aim at an object (range 9) and select it. Prints its number, position, type, frame, amount, quality and quantity (hex and decimal), then labels for `ObjStatus` bits: Au 1, In 2 (invisible), Ch 4, Cu 0x40, Fn 0x80; for an NPC also `NPCStatus` bits: Fe 1, Pa 2 (paralysed), As 4 (asleep), Po 8 (poisoned), De 0x10 (dead), Pt 0x80 (in the party), and the alignment G, E, N or C |
| 250 (0xFA) | "Create Item": type (3 hex digits), frame (1 hex digit, asked only if the type has more than one tile), quality (2), quantity (2, asked only for stackable types and the lever 0xC4 and switches 0x137, 0x138). The object goes into the active member's inventory; if that fails, onto the map centre's square ("(dropped)"); else "Failed!" |
| 251 (0xFB) | "Healing party": every member's hit points to `MaxHP`. In the dream (where the dreamer is the extra party slot) only NPC 0, to the Avatar's `MaxHP`. Nothing else is cured |
| 252 (0xFC) | prints the memory left for tiles (`GrMemoryKB`) |

## What could not be determined

Each section ends with its own list. The questions that matter most to a player or modder:

- **What many objects, flags and A.I. modes are for in the story.** The engine shows only
  what it does with them; the conversation scripts (`dialogue.json`) hold the rest, and they
  were followed only where the engine reads a flag. The meaning of A.I. modes 0x90 and 0x92,
  of `TalkFlags[0x74] & 1`, `TalkFlags[0x60] & 4` and of `ObjStatus` bit 0x01 on NPCs is
  not known.
- **Whether psychic impressions can be seen in play.** They need `ObjStatus & 1` on the
  object; in the shipped surface data no object with an impression text has it, and the
  dungeon blocks were not decoded (*Conversations and quest state*).
- **Uninitialised values.** Several branches read variables that are never set (`found` in
  `CmdGet`, `objTyp` in `InsertObj`, `installed` and `n` in the use actions, the seat's
  x, y in `PanelDreamMachine`); what they hold in practice was not measured.
- **Places and levels.** What underground levels 1, 3 and 5 are, which quality-20 levers
  exist in the world, which book shows which `MDTEXT.LZC` entry and which cabinet has which
  quality were not looked up in the game data.
- **A few readings rest on inference**: the mirror 0x12d's frames (none, human, Martian),
  the "plant" and "animal" reading of monster flags 0x80 and 0x4000, whether the M60's
  inverted line-of-fire test is a bug, and the `Active == 0x10` early return in
  `PartyUnconscious`. Each is marked where it is described.
- **`TypeWeight`**, the weight of each object type, is game data (TILEFLAG) and is not
  tabulated here.

### FINDINGS.md and this document

When this document was written, it disagreed with `docs/FINDINGS.md` in 35 places,
among them rest and the R key, the rest heal, light, what PassTime does on each call, the
to-hit test, damage kinds, class flags, eggs, reach, containers, `GiveObj`, the hot
radium, the rail car, the mirror, the furnace, big creatures, view rotation, swimmers,
belts, cut scenes 6, 7, 8 and 12, the debug keys, the starting place, the dreamer passing
out, the organ, the telescope and `#O`. Each was checked again in the code. In every case
the code does what this document says, and FINDINGS.md was corrected, its list of the
original's bugs included. None of them is still open.

## Sources

- `SRC/*.c`, `SRC/MSGS.ASM`, `SRC/MONSDATA.ASM`, `SRC/BSS.ASM`, `SRC/se.h`: GAME.EXE.
  `SRC_M/*.c`: MARTIAN.EXE. Each module's header comment says what it holds.
- `docs/FINDINGS.md` (the engine against Savage Empire's), `docs/MAP.md` (the layout of
  both programs and the data files), `docs/GAME_NOTES.md` (the compiler notes).
- `gamedata/MARTIAN/`: `LOOK.LZC` and `BASETILE` for object names
  (`build/names/look.py`), `TALK.LZC` decoded in `dialogue.json`, and the saved game
  (`SAVEGAME/OBJLIST`, `OBJBLK*`) where a section says it used them.
- The models: `..\u6se-decompiled\docs\MECHANICS.md` (Savage Empire) and
  `..\u6-decompiled\docs\GAME_MECHANICS.md` (Ultima VI).
