# What the decompilation found

This is what decompiling Martian Dreams showed about its engine and how it differs from
The Savage Empire's. Both games run on the Ultima VI engine. Savage Empire's own findings,
measured against Ultima VI, are in `..\u6se-decompiled\docs\FINDINGS.md`. The workers wrote
these notes down as they matched each module (`docs/GAME_NOTES.md`, M1-M35; the module
headers in `SRC/` and `SRC_M/`).

Conventions:

- Addresses are Martian Dreams'. Functions and variables are called by the names the
  sources give them (`python tools/md/mdsym.py --target game list` maps names to
  addresses). The few still without a name are written by their address: `C_SSSS_OOOO` or
  `D_XXXX`; an overlaid function's address is its stub segment's (`C_2F8D_01C1`, UseRead).
  An address of Savage Empire's says so.
- `TalkFlags[n] & m` is bit m of NPC n's conversation flags. The scripts and the engine
  use these bits as the story's state.

The layout of both programs is in `docs/MAP.md`.

## Executables and build

- **Two programs, as in Savage Empire.** `MARTIAN.EXE` plays the part of `SAVAGE.EXE`:
  configuration, title, introduction, character creation, endgame and credits. It runs
  `GAME.EXE` with `execl("game.exe", 0)`. `GAME.EXE` runs `martian.exe` again when there is
  no character, and runs `martian.exe !` at the end of the game. The `!` argument makes
  MARTIAN.EXE play only the endgame and the credits.
- **Compiler and library.** Origin used Turbo C++ 1.00 (C0M, game code) and TLINK 3.0. The
  run-time library is 1.00's `CM.LIB`, **unmodified**. Savage Empire's had 1.01's
  `FARHEAP`, `NEARHEAP` and `FINDFIRS` appended with `tlib`. `MARTIAN.EXE` also links the
  8087 emulator and `MATHM.LIB`, because main.c's `Calibrate` takes a square root.
- **Overlays.** `GAME.EXE` uses Borland's VROOMM overlays (1.00's `OVERLAY.LIB`, TLINK
  `/o`). Savage Empire has none.
  - 17 modules are overlaid, 66,603 bytes of code: talk and its interpreter, the dream
    (three modules), the cut scenes, rest, the machinery (belts, drawbridges, levers,
    power, jets), revive, disk, and the three modules of "use" actions.
  - What runs every turn stays resident: console, object, npctrack, combat, objcmd, gfx,
    map and the others.
  - The overlaid modules are linked after use.c, where their `_DATA` is (DS 1CD6-29F4),
    not where their stubs are.
- **What overlays do to the code** (M2, M14, M35):
  - `-Y` changes no code. It only adds the COMENT record that TLINK `/o` checks.
  - TLINK turns a far call from an overlay to a later function of the same overlay into
    `nop / push cs / call near`.
  - TLINK gives a stub entry to every public of an overlay, and to every static that a far
    fixup reaches. The entries come in the reverse order of the module's PUBDEF records.
    Turbo C writes those in the hash order of the names, so each overlaid source carries a
    `/* stub order: */` line.
- **The link.** `GAME.EXE` was linked as `ultima.exe` on 23 May 1991. TLINK `/o` writes the
  output name and the date into `_EXEINFO_`. As in Savage Empire's GAME.EXE, C0M's
  `_BSSEND` comes before `_BSS` (`SRC/SEGORDER.ASM`), so the start-up code clears nothing.
- **Flags.**
  - GAME.EXE's code is pascal (`-p`), as in SE. All of its C game modules but main.c and
    disk.c were compiled with line numbers (`-y`). In Savage Empire it was 13 of 19.
  - MARTIAN.EXE's code is cdecl. Its main.c and title.c have `-y`. Its main.c has no `-d`:
    the repeated literals ("fair.lzc" ...) are separate strings.
- **Far data.**
  - The far messages (`MSGS.ASM`, segment 2BB4) and the monster tables (`MONSDATA.ASM`,
    2DE5) are assembly: one paragraph-aligned `FAR_DATA` segment each, with a public name
    for every string or table. Turbo C would give every far variable a segment of its own.
  - use.c's two far tables are C, one segment each.
  - Every data-only module shows up in the segment table as an empty code segment:
    1649:A twice (MSGS, MONSDATA), 1FA9:2 (gamestat.c), 263D:E (drvnames.c).
- **Sizes.** GAME.EXE is 341,712 bytes (SE's: 270,016). It has 511 resident and 158
  overlaid functions (`docs/match_game.txt`).
  - 30% of the code starts from SE code nearly as is (similarity 0.85 or more).
  - Most of the overlays are new: 77% of their code (about two thirds of their
    functions) is below 0.5 similarity to any SE function.
- **Modules reorganised.**
  - SE's objcmd.c was one 35 KB module. In MD it is spread over several:
    - objcmd.c: Look, Get, Drop, Move.
    - objtype.c: the type tables, which SE kept in combat.c, objcmd.c, object.c and
      status.c.
    - use.c and the three action overlays.
    - cutscene.c: SE's cut scenes.
  - SE's npctrack.c gave its passability test to canmove.c.
  - SE's talkdr.c gave its helpers and TALK_Event to talk.c.
  - SE's spell.c is reduced to `SPELL_ReviveNPC`, alone in overlay 2F55.

## The map

- **Coordinates wrap at 0x400.** The surface is 1024x1024 tiles, stored as in U6 and SE
  (8x8-tile chunks, a map of 12-bit chunk indexes, superchunks), and both axes wrap.
  - Sums are masked with `& 0x3ff` everywhere: map, object, npctrack, combat, gfx,
    console, the cut scenes.
  - Differences go through `WrapDelta` (a value in -0x200..0x200) or
    `MapDistance`, where SE subtracted or used `abs()`.
- **The 40x40 area can cross the map's edge.**
  - `MAP_BuildArea` records where it crosses: when AreaX > 0x3d8, AreaFirstCol = 0x400 - AreaX
    and AreaLastCol = AreaFirstCol - 1. Y is handled the same way in AreaFirstRow/AreaLastRow.
  - `OBJ_FindListPos` scans `MapObjPtr` in map order across the edge.
  - `SearchArea(x1, y1, x2, y2)` takes x1 > x2 (or y1 > y2) as a rectangle that crosses
    it (AreaWrapX/AreaWrapY).
  - IsOutOfArea/IsSquareOutOfArea tell whether an object or a square is outside the area.
  - The peer map lost SE's world-edge cases.
- **Double tiles.**
  - `FindLoc` keeps four cursors (LocCursor[4], LocTarget[4]): the square,
    and the squares right, below and below-right of it. An object of two or four squares
    is found from any of them. SE had one cursor.
  - Creatures of two squares: TileFlag 0x40 means the other square is above, 0x80 to the
    left; types 0x167-0x169 depend on the direction. Creatures of four squares: 0xc0.
  - `CanMoveTo` (canmove.c) tests every square of such a creature, and so do the combat
    distances (`COMBAT_RangeToObj`).
- **The view can rotate.** ViewRotation holds the rotation (console.c's RotCos/RotSin).
  - Stepping on an object 0x1d1 sets ViewRotation = quality & 7 ("You feel dizzy...").
  - One of the dream's levers toggles it between 0 and 4.
  - Waking from the dream resets it.
- **Entrances.** SE had destination tables by object type. MD has one table,
  Entrances[24] (DS 1344), of (x, y, z) by the entrance's quality 1-24.
  - Object 0xc5 has its own four places (ChamberDests) and needs TalkFlags[0x73] & 0x10.
  - Objects of types 0xaf/0xd2 and 0xa3 are moved along with the party.
  - SE's type-0x44 travel objects are gone.
- **Levels.**
  - MapZ 0 is the surface.
  - MapZ 2 and 3 are the dream. There the area song is 3 and the sky bar is gray.
  - MapZ 6 is "nowhere": NPCs leave the world with `MoveObj(n, n, n, 6)`, and NPC 0 waits
    there between dreams.
- **Where the game starts and where the party wakes.** The game starts at 8:00 on day 6 of
  month 1, 1895, at (0x198, 0x270, 0) (gamestat.c).
  - After the party's death (ResurrectParty) it wakes at 7:00-7:59 at (0xa4, 0xc3, 4), the
    "Olympus mine", once TalkFlags[0x46] & 8 is set. Otherwise it wakes at
    (0x19d, 0x278, 0), the "space capsule". Then conversation 0x12 runs.
  - In SE the party woke in Intanya's hut, seven days later.
- **Latitude and longitude.** Plaques (0xac on MapZ 0) show one of 12 Martian place names,
  from Coprates Chasma to Olympus Mons; PlaqueNames' 13th entry is "Test Plaque". They also
  give the coordinates: latitude (y - 512) * 240 / 1024, longitude (x - 512) * 360 / 1024.
  The position action (object 0x60) prints the same.

## The dream

Savage Empire has nothing like it. Three overlays hold it: dream.c (2F5F), dreamobj.c
(2F63) and dreamstg.c (2F68).

- **The dreamer is NPC 0.** Its shape is 0x157, or 0x158 for a woman (PlayerIsFemale is the
  player's sex).
  - It goes into the dream through the dream machine: useact2.c's cabinet panel 4 and its
    chair 0x122 with the headgear. Settings other than 3 call `DreamSleep(npc, setting)`,
    setting 3 calls `DreamEnter`. One of the dream's levers also calls them.
  - `DreamSleep` puts it at (0x64, 0x3b, 2), or (0x93, 0x34, 2).
  - `DreamEnter` puts it at (0x35, 0x26, 3), DreamStage 0xe1.
  - Another member who sleeps "dreams until dawn while the party waits".
- **The party becomes the dreamer.** Active = PartySize, Party[Active] = 0, with NPC 0's
  name. This is U6's "in a vehicle" state (`IN_VEHICLE` is still `Active == PartySize`).
  MD has no vehicles.
  - The panel turns gray (ColorPaper 7), AmbientLight is 6, and the int32 GameFlags gets
    bit 0x10. Every module tests that bit for the dream:
    - the status panel shows only the dreamer's inventory, talents and hit points;
    - `mk_npcnum` turns NPC 1 into 0;
    - FindPartyMemberWithObj looks only at NPC 0;
    - rest is refused;
    - buckets stay full;
    - plants grow at random;
    - air and cold are not checked.
- **Triggers.** A dream trigger is a map object stepped on (`DreamTrigger`).
  - Its status bits & 0xe5 give the kind of stage, its quality and quantity a place, its
    frame a teleport mode.
  - The trigger cleans up the stage the dreamer leaves and sets up the next. dreamstg.c's
    twelve functions put the stage's NPCs back, clear its rectangle of the map and make
    its objects and creatures anew.
  - The stages are known by number: 4, 5, 0x20, 0x25, 0x40, 0x44, 0x60, 0x85, 0xa0, 0xa1,
    0xc0, 0xe0 ...
  - DreamStage is saved with the game.
- **Waking.** `DreamWakeUp` wakes the party at 8:00 on the date the dream
  began, or the next day after 8:00 (WakeDay/WakeMonth/WakeYear).
  - It clears the party's talents and costs poisoned members hit points.
  - It puts NPC 0 back on MapZ 6 and prints "You wake up."
  - Other ways out:
    - Talking to yourself wakes you.
    - A dreamer knocked out goes back to its wake-up place (WakeX/WakeY, then
      conversation 0x66). With no place to go back to, it shakes itself awake from the
      nightmare.
    - If the dreamer passes out ("Overcome by fatigue, you pass out.", DreamerPassesOut), the
      party's bodies are moved and the dreamer wakes in NPC 1's place with NPC 1's
      inventory: "All your friends have been killed!"
- **Dream creatures.**
  - The A.I. modes 0x13 and 0x16 (combat.c) and 0x18 (AI_SeekSeed, which walks towards an
    object 0xfb) are the dream's.
  - DreamFoesLeft counts the creatures of a stage. Killing the last one ends stages 0x20, 0xa1
    and 0xc0.
  - Tile 0x6f animates faster and faster (DreamAnimFaster, "...faster!").
  - Objects 0x85 go off in chains (BurnMoney, FireBurnAt, FireSpread).

## Machinery

The world's machines are new code. Talk flags hold their state:

| Flag | Meaning |
|---|---|
| TalkFlags[0x73] & 4 | underground power: the conveyor belts and the jetters run, the automatic doors 0xe3 work |
| TalkFlags[0x73] & 0x10 | surface power: the drawbridges and the doors 0xb3 work, the lamps 0xe4 are lit at night, the entrance 0xc5 opens |
| TalkFlags[0x71] & 8 | the power is not yet permanent: a belt may stop at random, and the machinery with it |
| TalkFlags[0x76] & 0x20 | water in the canals (set by the ruby lenses, cabinet panel 6) |
| TalkFlags[0x76] & 0x40 | the canals working (cabinet panel 7, or the valve 0xdc) |

- **Power** (power.c, overlay 2F72). `MachineryOn` starts the animated tiles of types 0xbc,
  0xbf, 0x1b0 and 0x1b3, and changes the objects of the map: 0x9a to frame 1, 0xc9
  frame | 4, 0xd6 to 0xd7, 0xe9 frames 1/3 to 5/7, and a 0xd1 placed south of every 0xd0.
  `MachineryOff` undoes it.
  - Loading an area (`__ObjectsDeserialize`) and saving one (`OBJ_CommitBlocks`) keep
    these objects in step with the flags.
  - `ShowObjects` plays the looping sound of the nearest machine (MachineTypes).
- **Levers** (lever.c, use action 24). `UseLever` goes by the lever's quality:
  - 1-2: the doors 0xb3/0xe3 whose quality & 0x7f is the lever's quantity.
  - 10: a drawbridge.
  - 20: the power station and the canal pumps, by quantity: 1 underground power (cut
    scene 2), 2 surface power (cut scene 3), 3 and 4 pumps on and off (cut scenes 6 and
    7), 5 start the belt, 6 cut scene 8.
  - 21: the dream's levers.
  - Objects 0xe3/0xb3 and plates 0x12d react to the NPCs standing on them, with a sound
    (`ShowObjects`).
- **Conveyor belts** (belt.c, overlay 2F52). Each belt object 0xbc moves what lies on it one
  tile per turn, west when its quality bit 0 is set, else east.
  - The first piece puts coal 0xbd on the belt every fourth turn; the last piece takes it
    off.
  - NPCs ride along, and the view follows the leader (LeaderOnBelt/MemberOnBelt,
    canmove.c).
  - The coal workers are NPC modes 0x99-0x9e (npctrack.c). They carry coal 0x1be/0x1bf to
    the belt and pull the lever 0xe9.
- **Barges** (barge.c, resident 03CE). A barge is a set of pieces 0x1a1 with one
  controller 0x1a2. They share the barge's number as quality; the controller's frame is
  the direction and its quantity the number of pieces.
  - While the pumps run, or in the dream, each barge moves one tile per turn over water
    and carries what stands on it.
  - `MoveFollowers` keeps the party on its places aboard (CurrentBarge).
  - The controller is steered by Use (action 23): the pointer gives the direction, or
    "center." to stop.
  - barge.c begins with a debugging dump to a file "brg" (debug key f6).
- **Drawbridges** (bridge.c, overlay 2F58). A drawbridge is worked by levers 0xc4
  (quality 10, quantity the bridge's number) and spans posts 0xc3. The first post's
  quantity gives the length (bits 0-4) and 0x80 for north-south.
  - Extending adds 0xc3 objects row by row; retracting deletes them.
  - Inconsistent data prints "Argh--drawbridge data inconsistent!".
- **Jetters** (jet.c, overlay 2F80). Object 0x1d0: quality bits 0-3 the delay, bits 4-7 the
  jet's length; quantity the countdown and the direction. The jet is drawn with
  `Missiles` and hurts what it hits. Jetters run only with TalkFlags[0x73] & 4.
- **Canals.**
  - Barrels 0x19a float only along the canal (`PushObj`). `MoveObj` gives them their
    side effects: "BOOM!!!", "Crash!!", "Splash!!" while the pumps run.
  - Objects dropped or pushed into a running canal splash.
  - Swimming creatures (class flag 0x200) can use canal water only while the pumps run
    (canmove.c).
- **Hazards** (gameloop.c `TerrainDamage`, new). Object 0x1cf's quality picks one of: "It's
  hot here!", "Fire!", "Frost!", "Fire bomb!", "Fragmentation bomb!", "Steam jet!",
  "Heat Ray!", "Freeze Ray!", "Acid!". Objects 0xd1, 0x17d, 0xd7 and 0xa8 hurt by
  themselves.

## The "use" command

- **SE vs MD.** SE's `CmdUse` (objcmd.c, C_1DBD_8276 in SE's GAME.EXE) is a long chain of tests by type. MD's
  (use.c) looks the type up in two far tables.
- **UseObjects** (far 2E08). 125 rows of 12 bytes: an object type and up to 10 action
  numbers (0xff ends). A type may have several rows. The table ends with a type < 0.
- **UseActions** (far 2E67). 75 rows of 0x24 bytes:
  - `on[8]`: the types allowed as the second object ("On - ");
  - `to[8]`: the types allowed as the third ("To -");
  - then the far pascal function, called as `f(obj, on, to)`.
  - In both lists -1 ends the list; -1 alone means no such object; -2 means anything,
    or a map tile.
- **UseObject** (use.c). It gathers the actions of the used object's type. It asks
  "On - " while one of them wants a second object, and "To -" for a third. It drops the
  actions that refuse what was chosen and calls the most specific one left.
  - When none accepts: "%s has no effect on ...".
  - The target may be as far away as the actor's level, plus one, when the actor has the
    first talent; otherwise only a direction.
- **Where the actions are.** Three overlays: useact1.c (2F8D, 26 stub entries), useact2.c
  (2F98, 22) and useact3.c (2FA1, 24). Plus lever.c's `UseLever`. The table points at the
  stub entries (`2F8D:0020 + 5k`).
- **What they do.**
  - Sleeping in a bed (`REST_Command`); reading (`MDTEXT.LZC`).
  - Dirt: digging, filling and burying, and the dirt loaded into wheelbarrows and rail
    cars. A cart's quantity is its loads, its quality the kind.
  - The drill and its cart, the power cable.
  - Radium for the guns, the power station and the mechanical man.
  - The guns' setting (Rifle/Shotgun/Combination).
  - Billiards ("Good shot, old bean!"); the pipe organ (keys 0-9 play notes; someone on
    its seat plays a melody); the cannon ("Kaboom! You blew the door down!").
  - The cabinet panels 0x1ca:
    - 4 the dream machines;
    - 5 the rubber sprayer;
    - 6 the ruby lenses: the number typed must be within 5 of the time as
      `Time_H * 100 + Time_M`;
    - 7 the canals.
  - Spray guns loaded with paint (reveals invisible creatures) or weed killer; seeds and
    plants; the oxium bin.
  - The ruby slippers ask the Grand Finale question.
  - Dreamstuff 0x14b turns into the first object inside it: an object, or a creature made
    with EGG_generate.
  - Oxygenated air revives a body.
  - Light sources: lit, put out, refuelled; they burn things.
  - The telescope looks 40 squares away (with U6's `XRayView`).
  - Buckets of water and ice, the chemicals on a mound, oil on rusted doors, the berries,
    the instruments ("GONG!!!!").

## Objects and the other commands

- **Command codes.**
  - SE's were 0x81 move, 0x82 get, 0x83 drop, 0x84 use, 0x85 talk, 0x86 look, 0x87 attack.
  - MD's icons are 0x81 attack, 0x82 talk, 0x83 look, 0x84 get, 0x85 drop, 0x86 move,
    0x87 use (keys A T L G D M U), 18 pixels apart from x = 16.
  - The keys B, R and Ctrl-S give 0x88 combat, 0x89 rest and 0xaa save. ALT+letter makes
    a command the default.
- **Reach.**
  - Get, Move and Use reach as far as the actor's level when it has the first talent
    (`TALENT_RANGE`), plus one for Move and Use; otherwise only the adjacent square.
  - A thrown or fetched object flies along `COMBAT_Missile`'s path ("Blocked!").
  - A refusal plays sound effect 5.
- **Inventories.**
  - An inventory or a container holds 16 slots (OBJ_InvSlotsUsed counts them).
  - What is carried is weighed against 20 x strength, what is worn against 10 x strength
    (SE weighed against STREN).
  - A container may not go into a container, and a lit light may not go into one.
- **Ready and Unready.** Readying no longer has SE's "can't use modern weapons!" rule.
  - Lit torches burn out in the hand.
  - Some objects can't be taken off ("They won't come off!").
  - The hot objects 0x1c0/0x1c1 hurt a member who lacks the first talent and gloves 0x88
    in a hand ("OUCH. It is very hot.").
- **Look.** Look reports the state of many objects:
  - a plant's growth and a lamp's fuel;
  - a cart's loads of dirt, rock, iron ore or coal;
  - the time on a clock; a gun's setting;
  - whether a panel is loose or broken; doors locked or rusted shut;
  - the plaques' coordinates;
  - object 0x124 shows a picture of `mdream.lzc` in the status area.
- **Object changes.**
  - Containers are searched on the map.
  - The clock types 0x5b/0x62 are swapped as they are picked up and put down.
  - Creatures 0x17f join their neighbours' frames (OBJ_JoinCreepingCacti).
  - `OBJ_MakeDeadBody` prints "Oh no!" when the body cannot take the inventory.
  - `GiveObj` refuses instead of dropping.
  - `TakeObj` also takes non-stacking objects one by one.

## Talk

- **The scripts are unchanged in form.** talkdr.c defines the same 89 opcodes as SE's,
  OP_EVENT (0xd1) included. A few behave differently:
  - `mk_npcnum` turns NPC 1 into 0 in the dream.
  - OP_CANCARRY takes the strength from GetStr.
  - OP_GIVEOBJ sets #T to 1 when the gift fails (SE: 0).
  - OP_RESURRECT looks for bodies of type 0x155.
  - `TalkDriver` has no "Talking to yourself?" or IsArmageddon test. It says "You sense "
    for an object of ObjStatus & 1. It stops the map animation while it waits for input
    and redraws the command mark at the end.
- **Two overlays.** talkdr.c (2F88) is SE's interpreter. talk.c (2F83) is SE's talk.c plus
  SE talkdr.c's helpers (TALK_GetNpcName ... FindPartyMemberWithObj) and TALK_Event.
- **Script variables.** `TALK_initTalk` sets them:
  - the avatar's (NPC 1's) #A dexterity, #E experience, #I intelligence, #P hit points,
    #S strength;
  - #G and $G ("Madam"/"Sir"), #N party size;
  - #O how many NPCs are near the view's middle;
  - $T morning/afternoon/evening.
  - It calls TalkDriver with 0 in the dream.
- **Talking to things.** Talking to an object gives a psychic impression when the active
  character has the second talent. An example:

  > This saw was used by a Martian doctor, or "Arborist", for surgery.

  - Some objects stand for NPCs 0x72-0x74.
  - Talking to yourself wakes you from the dream.
  - An NPC without a script is called "NPC 0x<hex>".
- **Events.** TALK_Event has 31 events: 1-11, 0x33-0x39, 0x65-0x6f, 0xfe and 0xff.
  - They move NPCs, make creatures and objects, and (0x6a-0x6e) teleport the party.
  - An event may leave a number in TalkEvent for `TALK_talkTo` to run after the
    conversation:
    - 9 ends the game (GameEnd);
    - 10 plays cut scene 10;
    - 0x34, 0x36 and 0x38 wake the dreamer.
  - Event 0xfe makes the party drunker. Event 0xff resets the video and exits to DOS.
  - An unknown event prints "CallGuards(%d)!".
- **The party.**
  - At most five members in all. SE allowed five besides its three core characters.
  - LeaveParty has no SetOkToGet.
- **Scripts.**
  - Scripts come from `talk.lzc` by a face/conversation number (ConversationNum). NPCs 10-15
    share 0x51's. Temporary NPCs of type 0x179 use 0x1d's.
  - Scripts of 0x36b0 bytes or more are refused.

## Combat and A.I.

- **Derived values** (combat.c):
  - MaxHP is 240 for NPC 6, 2 x STR + 24 x level for an actor under the player's control,
    else the class's MonsterHP.
  - GetStr subtracts 3 for a member with the first talent and stays within 1-30.
  - Armour (`GetProtection`) is the sum of what the equipped objects give (GetArmorPoints),
    with no curse or protection terms.
- **The monster tables** (`MONSDATA.ASM`). MonsterTypes lists 49 creature types after an
  unused first entry. `GetMonsterClass` returns the index minus 1. So the byte tables
  (strength, dexterity, intelligence, armour, damage, HP, alignment) are read at
  class + 1, and the word table MonsterFlags at class. The class flags seen:

  | Flag | Effect |
  |---|---|
  | 4 | the attacker paralyzes 1 time in 4 |
  | 8 | resists burning |
  | 0x20 | attacks with strength |
  | 0x40 | divides ("%s divides!") |
  | 0x80, 0x4000 | living: "killed" (else "destroyed") |
  | 0x100, 0x200, 0x400 | movement: goes anywhere but walls, swims, flies |
  | 0x800 | explodes, leaves no body |
  | 0x1000 | leaves no body |
  | 0x2000 | bleeds: blood objects 0x14e-0x150 |
  | 0x8000 | walks only on certain tiles |

- **Ranged weapons.** objtype.c's GetMissileInfo gives a weapon's missile tile, its sound and
  its ammunition: an ammunition type taken from the inventory, the weapon's own quantity,
  none, or the weapon itself thrown.
  - A thrown weapon lands where the missile stops, and the next one of its type is
    readied in the same hand.
  - COMBAT_AttackWithNextWeapon checks the ammunition (0x30 needs 0x40, 0x2b needs 0x3a
    ...).
- **Sprayers.** Weapons 0x2b, 0x2d, 0xf0 and 0xf1 spray 12 missiles (MissileSpray, damage
  20/20/25/10). Actors dodge by dexterity.
  - 0xf1 freezes water (bucket 0xa0 frame 1 to 2) and paralyzes.
  - A clever ally (`OSI_rand(1, 30) <= intelligence`) does not spray through the party.
- **Damage kinds** (`LoseHP(obj, damage, kind)`):

  | Kind | Effect |
  |---|---|
  | 0 | normal: the target may bleed |
  | 1 | paralyzes; freezes water |
  | 2 | ignores armour |
  | 3 | burns; thaws ice |

  Plants (IsPlantType's types) are hurt only by kinds 0 and 3.
- **Bodies.** Humans fall as 0x155; other creatures as 0x151-0x154, 0x9f or 0xfd.
- **Unwinnable game.** Killing certain NPCs sets IsArmageddon to 1 or 2. For some
  it prints "You have killed someone whose help you need to win the game!". Saving is
  refused while it is set. Killing 0x3b or 0x36 turns NPCs 0x33-0x3e hostile.
- **Invulnerable NPCs** (IsInvulnerable) answer a blow instead:
  - some talk;
  - 0x6d/0x67 send the party to the dreamer's wake-up place;
  - 0x40 hurls the player south (COMBAT_RasputinThrow).
- **New A.I. modes:**

  | Mode | What |
  |---|---|
  | 0x0d, 0x11 | the worms: body 0x172, head 0x174, segments 0x173 behind; "Worm error!" when they disagree |
  | 0x12, 0x19 | the creature 0x175 buried in the sand ("Suddenly, something bursts up through the sand!") and its 10-30 parts 0x16a |
  | 0x13 | steps at random and hurts everything on its square (the 0x18e of dream stage 0x44) |
  | 0x14 | the oxium thief ("Oxium stolen!") |
  | 0x15 | moves south while it can, else hits |
  | 0x16 | the dream: goes for an object 0xa2 |
  | 0x17 | the 0x198 with its 0x1a8 |
  | 0x18 | the dream creature that walks to an object 0xfb |
  | 0x1a | an NPC that turns into an object |
  | 0x95 | talks to the party when it comes near |
  | 0x98 | brawl |
  | 0x99-0x9e | the coal workers |

- **SE's modes, reworked.**
  - No special weapons.
  - With no line of fire the attacker steps to the target's row or column.
  - Assault, Range and Berserk stop after a hit.
  - The choice of opponent skips other levels; chaotic actors spare their own kind and a
    creature's own parts.
  - Unfriendly turns hostile one time in six: type 0x17a becomes 0x17c, stronger and
    slower.
  - Immobile creatures 0x175/0x182 hide when the enemy is far.
- **Special blows.**
  - Type 0x17b swallows a body ("Gulp!").
  - The weapon 0x139 strikes through COMBAT_FireM60. Hitting NPC 0x66 with it plays cut
    scene 9, wakes the dreamer and sets TalkFlags[0x46] & 8.
- **Music.** Combat has its own music: MUS_CombatSong at the start, MUS_CombatEndSong at the end,
  by the outcome.

## Attributes, talents and levels

- **Attributes and levels.** Strength, dexterity, intelligence, `ComputeLevel` (the
  experience halved until 99 or less) and `AddXP` are as in SE.
- **Levels in a dream.** A member goes up a level while resting, in a dream of three
  obelisks (`REST_LevelUp`):
  - the sword raises strength, the heart dexterity, the book intelligence, up to 30;
  - 30 hit points come back.
  - In SE a bear, a jaguar and a monkey asked the questions.
- **Talents.** Each NPC has three talents: Talents[16], three 4-bit fields .a, .psychic
  and .c, saved with the game. The party view shows them as icons (tiles 0x120-0x122).
  - **First (.a).** The actor reaches as far as its level (Get, Move, Use, Drop) and its
    strength counts 3 less. It may handle the hot objects.
  - **Second (.psychic).**
    - Impressions of objects when talking to them.
    - Objects flagged for it show.
    - The dreamstuff hoard can be used.
    - The member cannot rest ("inanimate things talking").
  - **Third (.c).** Invisible objects show outlined, and the whole view is lit as with
    `XRayView`. The member cannot rest ("too much light").
  - **Gaining and losing them.**
    - The berries 0x49, 0x4a and 0x4b raise them by one. 0x4c instead hurts (5-20 +
      5-20, ignoring armour), makes the eater drunker and cures poison.
    - Talents are at most 10.
    - Each minute a talent of NPCs 0-7 drops by one when `OSI_rand(1, GetInt * 4) == 1`.
    - Waking from the dream clears the party's talents.

## Rest and time

- **Rest** (rest.c, overlay 2F7C).
  - Refused in combat, with foes within 39 tiles, with someone watching within 5, in solo
    mode and in the dream.
  - Poisoned members are named and the player may give up.
  - The party camps in a tent of five objects 0x86, until sunrise or sunset or for 1-9
    hours.
  - More than 4 hours heals (2-8 + 2-8) x hours/2 when NextSleep allows (then NextSleep is
    6).
  - Poison wears off on 2-16 >= 15, else costs hit points.
  - SE's food (IsEdible, TryEat ...) and gathering around the leader are gone. CampOfsX/Y
    and CampFacing remain, unused.
- **PassTime** (gameloop.c) keeps SE's frame with much new code. Every call:
  - updates barges, jetters, plants 0xfb and shrinking objects 0x9d. A plant grows a
    frame three days after planting; the seed 0xfc in the hole beneath holds the date.
  - shakes the screen one time in 5 once TalkFlags[0x46] & 8 is set.
- **For each member out of the dream:**
  - **Air.** Under TalkFlags[0x10] & 0x20, a member without oxium 0x83 "gasps for air!"
    (ObjStatus 0x40), then "breathes easier!".
  - **Lights.** They burn out ("went out"). Lanterns 0x74/0x76 run out of fuel and become
    0x73/0x75.
  - **Cold.** On the surface at night (22-3 h, or 18-6 h with too little warmth from what
    is worn) a member "is freezing!" and loses 1-2 points a minute, until it "feels
    warmer!". Never NPC 6.
- **Per minute:**
  - charm and other effects wear off;
  - the talents drop;
  - ice in a bucket melts back to water, and ice 0x100 melts away;
  - carrying 0x1c0/0x1c1 outside a container 0x8b poisons.
- **Hourly:**
  - the oxium 0x83 is used up one by one;
  - lamps 0xe4 change for the night;
  - DrunkCounter goes down;
  - the schedules are updated.
- **Light.** AmbientLight ramps at 6:40-7:20 and 17:20-18:00. It is 0 underground and at
  night, but never below what a carried light gives.

## Cut scenes and the end of the game

- **Cut scenes** (cutscene.c, overlay 2F75). PlayCutscene(n) plays cut scene n of
  `MIDGAMEX.LZC`, where X is "123456789abcdef"[n - 1] (MIDGAME1-D).
  - Each page is five entries: three texts and two pictures, the second drawn over the
    first by the player's sex.
  - The others play one scene each and change the world to what it tells:

  | Scene | Pages | Function | Called from | Changes |
  |---|---|---|---|---|
  | 1 | 2 | Cutscene1 | a use action (useact1.c's UseOpenHatch) | |
  | 2 | 3 | Cutscene2 | LeverMachinery 1; the coal workers' lever | underground power: lights 0xc9 and machines 0xd6 in (0x3d0, 0x1d0)-(0x3f0, 0x1e7) |
  | 3 | 2 | Cutscene3 | LeverMachinery 2; the power cable | surface power: removes the objects 0x13b at four places |
  | 4 | 2 | Cutscene4 | dream.c, before entering | |
  | 5 | 3 | Cutscene5 | the dream machine, NPC 0x21 | |
  | 6 | 3 | Cutscene6 | LeverMachinery 3; cabinet panel 6 | canal pumps on |
  | 7 | 3 | Cutscene7 | LeverMachinery 4; panel 7, the valve | canal pumps off |
  | 8 | 3 | Cutscene8 | LeverMachinery 6; the rubber sprayer | moves NPCs 0x40-0x46 |
  | 9 | 2 | Cutscene9 | combat.c, the weapon 0x139 hitting NPC 0x66 | puts NPCs back on MapZ 4 |
  | 10 | 2 | Cutscene10 | talk.c, event 10 | removes or moves NPCs 0x20-0x31 and 0x50-0x57 |
  | 11 | 1 | Cutscene11 | dream.c, before entering | |
  | 12 | 3 | Cutscene12 | the dream machine, the robot 0x11f | NPC 6 |
  | 13 | 1 | Cutscene13 | the ruby slippers, answer "N" | |

  - ShowMdMap shows a map of `MDMAP.LZC` (the map object 0x96).
  - Debug key f5 plays scenes 1-9.
  - SE had four cut scenes and the MIDGAMEn.LZC pictures in gfx.c's `ShowMidgame`, which
    is gone.
- **The end.** GameEnd runs when the answer to the Grand Finale question is "Y", or
  from event 9.
  - It plays song 0xc and deletes the `.tmp` area files. The game is not saved.
  - It shuts sound and memory down and runs `execl("martian.exe", "martian.exe", "!",
    0L)` in an endless loop.
  - MARTIAN.EXE then plays `Endgame` (`scenetxt.lzc` entry 3, `endgame.lzc`,
    `moongate.lzc`). Endgame reads `savegame\objlist` for the avatar's shape, then
    scrolls the credits.
  - In SE the last cut scene ran SAVAGE.EXE in the same way.
- **Other ends.**
  - Event 0xff exits to DOS.
  - A party knocked out before the capsule door is open (TalkFlags[0x10] & 0x10 clear)
    ends the game on the text screen: "Your character cannot be resurrected before the
    capsule door is open.", exit code 4 (sound.c PartyUnconscious).

## Status display, graphics and sound

- **Status panel** (status.c).
  - The party view shows five members with their talent icons.
  - `STAT_PrintHP` colours hit points by status, below 10, and by ObjStatus.
  - The portrait view shows the computed STR/DEX/INT, coloured.
  - The empty slot and frame tiles follow the panel colour (ColorPaper 7, gray, in the
    dream).
  - The inventory is still SE's 4x4 grid.
  - STAT_GetEquipSlot and IsWeightPer10Type moved to objtype.c. SE's inventory scroll
    stubs and STAT_FindEquipSlotByType are gone.
- **The sky bar** (gfx.c `DrawSkyBar`). A strip of `mdscreen.lzc` entry 2, scrolled by the
  hour; gray in the dream, black underground. It also sets the frames of the clocks
  (animated tiles of BaseTile[0x62] get Time_H % 12 + 0x1a0).
- **Portraits.** A face of `mdfaces.lzc` over a background of `mdback.lzc`, chosen from a
  table by NPC (PortraitBgs). SE chose the background by place and time of day.
- **The paper doll.** A picture of `mdinv.lzc` by the character's type, over four frame
  tiles.
- **Graphics effects.**
  - The missile tables have 23 tiles. A missile's hit list puts objects first, then NPCs.
  - MissileSpray sprays 12 missiles in one of eight directions. It replaces SE's AnimWebs.
  - Darkness is tile 0x10c (SE: 0x1bc).
- **What is visible** (visible.c).
  - A square keeps the brightest light reaching it instead of the sum. A light of level n
    spreads n + 1 squares.
  - Objects 0x184 near the party change frame.
  - Wall variants exist only for the wall tiles 0xa0-0xa5, 0xb0-0xb2 and 0xe0-0xe5.
- **Console.**
  - The "more" prompt waits for any key or click.
  - Containers can be locked.
  - Load and save have no disk swapping.
- **Sound** (sound.c, SE's game-side music.c reworked).
  - 16 songs from `music.lzc`, timbres from `md.tim`.
  - The area song is 3 in the dream, 2 underground, 8 in combat, 0 by day, else 1.
  - Jingles queue the next song with SetNextSong instead of waiting for a marker.
  - 56 sound effects; machines keep one looping (MUS_PlayLoopSfx).
  - Someone at the organ plays a melody (MUS_PlayMelody, Melody).
- **Debug keys.** They work when TalkFlags[1] & 0x40 is set. Without the flag, most
  answer "Are you a god?" and one says "That's an old, worn-out magic word!". They give:
  - time and goto, +60 minutes, the peer map;
  - "Divine Flare--";
  - not solid (AvatarNotSolid);
  - going to an NPC, sound effects, talk flags;
  - cut scenes 1-9, the barge dump;
  - analyse, create, heal.

## What was removed from Savage Empire

- **Magic.** SE's spell.c (Invoke, the spells and their offerings) is gone. Only
  `SPELL_ReviveNPC` remains, for oxygenated air and the scripts.
- **Floppies.** DISK_IsInDrive, DISK_ScanDrive and DISK_confirm's body are gone, and so
  are the calls to DISK_confirm in map.c, gfx.c, object.c and sound.c. disk.c keeps
  DISK_init, DISK_full, an empty function where DISK_confirm was, and DiskNames (unused).
- **Vehicles.** Board/Unboard and riding the turtle are gone, and so are SE's travel
  objects 0x44 and teleport tables. GetPartyObjOrVehicle and GetObjX/YOrVehicle are plain
  now.
- **Big creatures.** SE's multi-object creatures (OBJ_MoveCreatureParts, OBJ_getHead,
  their cases in UpdateMoveFrame and IsBlockedAt) are gone. MD has its own linked
  creatures, whose parts hold the head's number in their quality:
  - the worms (head 0x174, body 0x172, segments 0x173);
  - the 0x175 with its parts 0x16a/0xfe.
- **Other gone code.**
  - SE's type tests C_130F_0002-01C6 (SE's addresses) and FindAdjacentPlate.
  - The door case of __ComputeResistance and the 0x149 seats of NPC_TakeTurn.
  - TeleportToSchedule's test that spared NPCs near the player.
  - EGG_CombatRange, and EGG_GivePossessions' items (it is empty).
  - The special weapons in the A.I.
  - Food and eating.
  - The "modern weapons" rule.
  - MagicalHalo, MagicWind, AnimHailStorm, PortraitBg, ShowMidgame.
  - SetTileAnimation, DrawClockHands and the first-run conversation.
  - VIS_IsInViewSquare.
- **The game state block** (gamestat.c) has no KARMA, WindDir, SpellFx, moonstones, powder
  keg, Gargish or moons. SE still carried most of U6's. It gained:
  - the dream's variables;
  - the player's sex;
  - the talents.

## Leftovers

- `CommandNames` still lists "CAST" after "Combat" and "REST", though there is no magic.
- The dream uses U6's vehicle state (`IN_VEHICLE`, `Active == PartySize`).
- Unused U6 and SE data:
  - WindDirNames (gameloop.c, "not used here");
  - ShipWindCost, SkiffCurrentCost and RaftDriftDir (npctrack.c);
  - CampOfsX/CampOfsY/CampFacing (rest.c);
  - DiskNames (disk.c);
  - D_1682 and D_16D8 in the game state.
- U6's `XRayView` is back in use, by the telescope. SE had no X-ray view.
- `TileDamage`, the hook for map tiles that hurt, is an empty function.

## Library and compiler

- **The shared library is SE's, a little later.** Both programs carry it, and the two
  copies are the same code: GAME.EXE's music.c and its library from MEMMOVE to EMSALLOC
  are MARTIAN.EXE's with the addresses moved.
  - Unchanged from SE: KEYB, PIT, CALIB, EMSINIT, EMSMAP, EMSFREE, HEAPFREE, LIBREAD, and
    the OSI shape, text and mouse assembly.
  - Changed:
    - EMS_alloc returns the block's new place when it moved to the next page; EMS_none
      is new.
    - EMSNEXT.ASM and emsmerge.c are new (EMS heap compaction). MEM_free compacts the
      EMS heap, MEM_alloc sets up the conventional heap if needed, and MEM_size is new.
    - OSIREAD saves BX/CX, returns the bytes read as a long and reads nothing for 0.
      OSIWRITE saves BX/CX/DX. OSICREAT is new.
    - LZW keeps its state in its code segment.
    - LZC is three modules (LZC.C, LZCTYPE.C, LZCLOAD.C) with LIBSIZE between: one C file
      cannot be split over several code segments. LZCTYPE loads by MD's entry types, with
      two loaders the program sets.
    - LIBOPEN takes its buffers as DS far pointers.
    - MEMMOVE does nothing for 0 bytes.
    - In MARTIAN.EXE, SE's tmdrv.c is split in two (tmdrv.c, supertm.c); palfile.c and
      TXTSEL are new.
    - events.asm hooks the timer with 10h for 0Ah.
- **Turbo C++ 1.00 against 1.01.** Origin's library C files are 1.01 output: HEAP.C,
  MEMORY.C, LZC.C, LIBOPEN.C, music.c, tmdrv.c, surface.c ... The game modules are 1.00.
  - The OSILIB C files are mixed: SHPHIGH, PALUSED, FADESTEP need 1.01; FADEIN needs
    1.00; VMODE takes either.
  - The differences that told them apart (M5, M8, M9):
    - After `X = expr; f(X);` 1.00 reuses the register, 1.01 pushes X again from memory.
    - 1.01 shortens a long expression assigned to an int.
    - 1.01 tests a register variable after a call (`mov di,ax / cmp di,-1`) where 1.00
      tests AX.
    - Long sums come out in another order.
- **OSI assembly in GAME.EXE.**
  - KBD, MOUSE, DELAY, RAND and OSI_FILE are SE's GAME.EXE's, unchanged.
  - SE's LOW.ASM lost its first two code segments (OSI_selectDrive, OSI_getDrive).
    DRVLOAD.ASM (the graphics driver loader) and VIDRESET.ASM (back to the start-up video
    mode) took their places.
  - CLIP.ASM (264A, the driver's mode call and line clipping) and OSIWRITE sit between
    them.
  - 264A reaches its helpers with near calls that return with RETF: labels inside one FAR
    PROC.
- **Origin's assembler.**
  - MEMMOVE.ASM and OSIREAD.ASM were assembled with TASM's JUMPS: forward jumps are
    padded with NOPs, which the sources write out.
  - TASM's fixup order differs from MASM's: events.asm and CALIBDRV.ASM name theirs.
  - MASM 3.0 encodes `XCHG`/`TEST` between registers, and `OR reg,imm8`, differently, so
    those are `db`.
- **TLINK ignores case.** In MARTIAN.EXE a module function named like a library function
  in another case would silently replace it. main.c's `Delay` and `Exit` are called
  `Pause` and `ExitGame` for that reason.

## Bugs of the original, kept in the sources

The sources reproduce these, marked `/*sic*/` or described in a comment:

- **combat.c**
  - LoseHP computes `damage << 1;` and drops the result: kind 3 against class flag 0x10.
  - AI_Like and AI_Unfriendly: `x = (x + OSI_rand(0, 1)?10:-10) & 0x3ff`. Because of
    precedence, x becomes 10 or 0x3f6.
  - COMBAT_RangeToObj's default case returns `x / (x - x)`, a division by zero for frames 16
    and up.
  - COMBAT_FireM60 places its 0xda with the target's y.
- **npctrack.c**
  - `GetQual(o) != ViewRotation & 7` (precedence).
  - `GetY(o) + DirIncrY[info] == y & 0x3ff` in __DoOnPath.
  - `area_y = +PTH_DirY[dir] & 0x3ff`: an `=+`, so nothing is added.
  - `y = WrapDelta(x - area_y)` in PTH_FindPath.
  - `SetFrame(lever, GetFrame(lever | 4))`.
  - ApproachNearestMember walks to the last member looked at, not the nearest.
- **object.c**
  - ApplyTerrainEffects is called with the area-relative coordinates.
  - InsertObj tests `QuanType(objTyp)` before objTyp is set.
  - TakeObj skips the object after each one it deletes.
- **objcmd.c**
  - ShowPortrait sets objNum to 0x1d for type 0x179 and returns, and tests
    `objNum > 0xbf` after `objNum >= 0xc0`.
  - CmdMove's map branch prints `GetObjectString(obj)` with obj never set, and its "into"
    branch sets the frame of a target it never set.
  - The light-type test IsUnlitLightType is called with object numbers (CanPutInContainer,
    CmdDrop, CmdMove).
  - CmdGet's `found` is uninitialised; CmdMove's "out of" compares GetQuan with the
    quantity code. Both are SE's.
- **dreamobj.c**
  - When the dreamer passes out, the loop over its inventory deletes the next object
    instead of the one it tested.
- **useact1.c**
  - `TalkFlags[2] &= 4` where `&= ~4` was meant.
  - The radium action tests an uninitialised `installed` ("Radium has already been
    installed.").
- **useact2.c**
  - Oxygenated air tests a register variable before setting it.
  - Action 55 heals with NPC 1's MaxHP and name but NPC 0's hit points.
- **useact3.c**
  - The bucket poured on the ground (on == -1) goes on to GetType(-1).
  - The lens action sets on = -1 and then searches around GetX(on).
  - The telescope returns after "nowhere." without restoring MapX/MapY.
- **rest.c**
  - REST_LevelUp's `default: done = 0;` is written twice: the original clears DI twice.
- **objtype.c** (not marked in the source)
  - GetDamagePoints reads `MonsterDamage[class]`, where combat.c reads the same table at
    `[class + 1]`. A creature's own attack takes the damage of the class before it.
