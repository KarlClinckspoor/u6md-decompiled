# From Ultima VI to Martian Dreams: how one engine was reused

Origin shipped three games on one engine within about a year: Ultima VI (1990), The Savage
Empire (October 1990) and Martian Dreams (May 1991). All three now have decompilations that
rebuild them byte for byte: `..\u6-decompiled`, `..\u6se-decompiled` and this repository. So
the code can be compared directly. This essay asks three questions:

1. How did the code change from one game to the next, and why?
2. Origin reused the engine to save time and money. By the developers' later accounts it did
   not save much. What in the code explains that?
3. What could a small team in 1990-91 have done instead, with the tools of the day?

Most of what follows comes from the write-ups and findings of the three decompilations
(`U6-LESSONS_DECOMPILATION.md` and `..\u6-decompiled\FINDINGS.md`; `SE-WRITEUP.md` and
`..\u6se-decompiled\docs\FINDINGS.md`; `MD-WRITEUP.md`, `docs/FINDINGS.md`, `docs/MAP.md`,
`docs/match_game.txt`). The essay goes back to the C sources only to check particular points.
Two cautions apply throughout:

- **The names are ours.** Every function, variable and constant name in the three
  decompilations was given by the decompilers. U6's `OBJ_` and `TIL_` constants come from
  its readability pass. Compiled code holds only numbers, so when this essay says the engine
  "tests type 0x1a2", nobody can tell whether Origin's source spelled the number or a macro.
  What the code does show is *where* a rule lives: in C code, or in a data file.
- **Similarity figures measure compiled code.** `u6match.py` and `mdmatch.py` compare
  instruction sequences. Savage Empire was built with a different compiler from Ultima VI,
  so its similarity to U6 is lower than its source similarity would be. Martian Dreams used
  SE's compiler, so the MD-to-SE figures are closer to the truth.

Each section keeps what the code shows apart from hypothesis. Hypotheses are labelled.

---

## Part 1. How the code evolved

### 1.1 The toolchain

| | Ultima VI | The Savage Empire | Martian Dreams |
|---|---|---|---|
| Released | 1990 | October 1990 | May 1991 (GAME.EXE linked 23 May 1991) |
| Compiler | Turbo C 2.0 | Turbo C++ 1.00 (May 1990) | Turbo C++ 1.00 for the game; 1.01 for Origin's library C files |
| Run-time library | Turbo C 2.0 `CM.LIB` with OSI versions of `clreol` and `gotoxy` | 1.00 `CM.LIB` with 1.01's `FARHEAP`, `NEARHEAP` and `FINDFIRS` appended with `tlib -+` | 1.00 `CM.LIB` unmodified, plus `OVERLAY.LIB` (VROOMM) |
| Assembler | TASM (same-segment far calls kept as far calls) | TASM, newer: same-segment far calls become `push cs / call near` | TASM, with `JUMPS` in two files and its own fixup order |
| Memory model | medium; game code pascal (`-p`) | the same | the same, with 17 overlaid modules |
| Line numbers (`-y`) | not determined | 13 of 19 game modules | every C game module except main.c and disk.c |

**What the code shows.** The compiler changed between Ultima VI and Savage Empire, only a few
months after Turbo C++ 1.00 shipped. SE's start-up code and library are 1.00's, but three
heap and directory modules were replaced with 1.01's by hand (`SE-WRITEUP.md`, "Finding the
compiler"). In Martian Dreams the run-time library is plain 1.00 again. Origin's own library
files (HEAP.C, MEMORY.C, LZC.C, music.c, the driver loaders) were compiled with 1.01, the game
modules with 1.00. The OSI library C files are mixed: SHPHIGH, PALUSED and FADESTEP need
1.01, while FADEIN needs 1.00 (`MD-WRITEUP.md`, "Turbo C++ 1.00 and 1.01 in one program").
More modules carry line-number records in each game: 13 of 19 in SE, all but two in MD.
That is what the IDE's debug setting produces.

**Hypotheses.**

- *Why the compiler changed.* Turbo C++ 1.00 offered nothing a C program like this needed
  except a newer optimiser and, later, VROOMM overlays. SE has no overlays. A likely reason
  is that the new team took up Borland's current release when it started. Confidence: low.
  The code shows only that the change happened.
- *Why 1.01 heap modules in SE and not in MD.* Patching three heap modules into a run-time
  library looks like a fix for a specific heap bug, made on the machine that linked SE.
  MD's link used an unpatched 1.00 library, so it was probably a different machine or a
  clean install. Origin's own library code, meanwhile, came from 1.01. That fits a
  library kept by one person on a newer compiler while the game programmers stayed on
  1.00. Confidence: low to medium. It is consistent with the evidence, but not proved.
- *Why more `-y`.* Debug information in nearly every module suggests the MD programmers
  worked in the IDE and Turbo Debugger much of the time. That fits a lot of new code being
  debugged on a short schedule. Confidence: low.

### 1.2 Program structure

**Executables.** Ultima VI has four programs: a small Microsoft C launcher (`ULTIMA6.EXE`),
`U.EXE` (menu, introduction, character creation, transfer from Ultima IV/V), `GAME.EXE` and
`END.EXE`. Savage Empire folds the launcher, introduction, character creation and endgame into
one program, `SAVAGE.EXE`, which runs `GAME.EXE` and is run again for the end. Martian Dreams
copies that layout exactly. `MARTIAN.EXE` runs `game.exe`, and `GAME.EXE` runs
`martian.exe !` for the endgame (`docs/FINDINGS.md`, "Executables and build").
The two-program split survived because it is a cheap way to free memory: the game never
holds the introduction's code and pictures. That is the simplest explanation, and probably
the right one.

**Modules, U6 to SE: nothing moved.** SE's GAME.EXE has U6's 19 game modules in U6's link
order: main, gameloop, console, map, visible, object, status, talk, talkdr, spell,
npctrack, combat, objcmd, egg, music, gfx, rest, disk, then the OSI assembly
(`..\u6se-decompiled\docs\MAP.md`). Magic was redesigned completely, from a spellbook to
"Invoke" with offerings, yet it stayed in `spell.c`. Every new "use" handler (tar, clay,
loom, gunpowder, camera, the turtle, the statue heads) went into `objcmd.c`, which grew to
35 KB, SE's largest module. The C sources are about the same size: 24,231 lines for U6's game
modules and 24,248 for SE's. SE was U6 edited in place, file by file.

**Modules, SE to MD: reorganised.** MD's GAME.EXE has 37 C game modules instead of 19:

- SE's `objcmd.c` is spread over `objcmd.c` (Look, Get, Drop, Move), `objtype.c` (type
  tables SE kept in combat.c, objcmd.c, object.c and status.c), `use.c` and three overlays
  of use actions (`useact1-3.c`), and `cutscene.c`.
- SE's npctrack.c gave its passability test to the new `canmove.c`. SE's talkdr.c gave
  its helpers and `TALK_Event` to `talk.c`.
- SE's 636-line `spell.c` shrank to `revive.c`: one function, `SPELL_ReviveNPC`, alone in
  an overlay.
- New modules hold new systems: `barge.c`, `belt.c`, `bridge.c`, `lever.c`, `power.c`,
  `jet.c`, `dream.c`, `dreamobj.c`, `dreamstg.c`, `typetest.c`.

**Overlays.** MD is the only one of the three with overlays: 17 modules and 66,603 bytes of
code, loaded on demand by Borland's VROOMM manager (`docs/MAP.md`). What runs every turn
stays resident: console, object, npctrack, combat, objcmd, gfx, map. What runs now and then
is overlaid: talk and its interpreter, the dream, the cut scenes, rest, the machinery,
revive, disk and the use actions.

**Hypothesis: the overlays caused the reorganisation.** VROOMM overlays whole code segments,
and in the medium model one C file is one code segment. MD's notes point out that "one C
file cannot be split over several code segments" (the LZC library had to become three
files for that reason). To overlay the use actions, they had to leave objcmd.c. To overlay
the cut scenes, they had to leave it too. To overlay the conversation code, talkdr.c had to
be a module of its own, and it is: 99% identical to SE's, now overlaid. Many of the module
moves in Martian Dreams are the cuts you would make to separate per-turn code from
occasional code. Confidence: medium to high. The pattern fits the overlay split closely,
though some moves (objtype.c, canmove.c) are resident and served tidiness or the new map
rules instead.

**Hypothesis: memory forced the overlays.** All three games keep their near data at the top
of the 64 KB DGROUP. Uninitialised game data ends at about DS ECCA in U6, EEB2 in SE and F24E
in MD. Then come the library's variables and a 4 KB stack at the top of DGROUP
(`_stklen` 0x1000). Each game grew its near data until little room was left. Far data
already held the long things: the messages and monster tables, and in MD the two use
tables, each in its own far segment. Code had a similar problem. `main.c` gives all the
memory that is left, minus 16 bytes, to the graphics driver as its tile cache
(`GrMemorySize = MEM_largestFree(0) - 0x10`, MD main.c). So every kilobyte of resident code
was a kilobyte less for tiles. Counting the run-time library, SE's GAME.EXE has about
195 KB of code, all resident. MD's has about 179 KB resident (image 0000-2BB37, the
overlay manager included) and 67 KB in overlays, about 245 KB in all. So the overlays cut
resident code by about 16 KB while the program's code grew by about 50 KB. Confidence: medium. The arithmetic is from the maps. That this
was the reason, rather than, say, keeping the minimum-memory requirement on the box, is
inference.

### 1.3 How much code carried over

| Step | ≥ 0.85 (small changes) | 0.5-0.85 (reworked) | < 0.5 or new |
|---|---|---|---|
| U6 → SE GAME.EXE (`SE-WRITEUP.md`) | 5.8 KB (4%) | 62 KB (38%) | 97 KB (59%) |
| SE → MD GAME.EXE (`MD-WRITEUP.md`) | 70 KB (30%) | 62 KB (27%) | 101 KB (43%) |
| SE → MD GAME.EXE, overlays only | 14% | 9% | 77% |
| SAVAGE → MARTIAN.EXE | 144 of 192 functions, but 40% of code | | 17 functions, 48% of code |

The U6-to-SE figures understate how much was kept, because the compiler changed (Turbo C++
1.00 always pushes a frame and reuses AX differently). Even so, SE rewrote most functions to
some degree. MD stayed closer to SE. It used the same compiler, and the reused core was the
same core.

Per module (from `docs/match_game.txt`, by bytes of MD code):

- **Almost unchanged:** the talk interpreter talkdr.c (99% at ≥ 0.85), the OSI assembly (file,
  mouse, low-level: 99-100%), music.c (79%), console.c (75%), the shared library (heap,
  memory, LZC: 80-100%).
- **Reworked but recognisable:** main.c (67% at ≥ 0.85), status.c (48%), npctrack.c (44%),
  object.c (43%), map.c (40%), visible.c (90% in the middle band).
- **Mostly new:** combat.c (61% below 0.5, though it is the largest module and still starts
  from SE's), canmove.c, use.c, barge.c, egg.c, and every overlay except talkdr.c and revive.c.
  The use actions are 88-94% below 0.5. Where they resemble anything, it is SE's objcmd.c,
  the module of the same job.

The reusable part of the engine turned out to be the part that knows nothing about the game:
script interpreter, text console, file and memory library, input, music driver. The part
that knows the game's objects (combat, object commands, the clock, movement) was rewritten
each time.

### 1.4 The data model

The world's data model barely changed in three games. That is the part of the reuse that
plainly worked.

- **Objects.** Every object is a slot with a position word (x in bits 0-9, y in 10-19, z in
  20-23, or the owner's slot when the object is carried), a type-and-frame word, a status byte
  and quality and quantity bytes (`..\u6-decompiled\FINDINGS.md`, "How the game works").
  There is one object list per map area, in `savegame\objblkXY` files. MD keeps all of it.
- **NPCs.** Each NPC is a low object slot with parallel tables: mode (`NPCMode`), schedule
  pointers, the leader/target word. Temporary monsters use the top slots: 0xE0-0xFF in U6,
  0xC0-0xFF in SE. Monster classes come from a far table of types with per-class strength,
  dexterity, armour, damage, hit points, alignment and flags. U6 has 64 classes with starting
  inventories, SE 47 without inventories, MD 49 (`MONSDATA.ASM`).
- **Map.** 8x8-tile chunks, a map of 12-bit chunk indexes and superchunks for the surface.
  MD's `chunks` and `map` files are read "as in U6 and SE" (`docs/FINDINGS.md`, "The map").
  MD's one change of meaning, coordinates that wrap at 0x400, is in the code, not the files.
- **Schedules.** `SchedPointer[n]` and `SchedIndex[n]`, with entries keyed by hour and
  weekday, from a `schedule` file. MD updates them hourly in `PassTime`, as before.
- **Talk scripts.** A bytecode interpreter with 89 opcodes, 88 of them U6's. SE added opcode
  0xd1 (`OP_EVENT`), which calls a numbered event written in C (`TALK_Event`). MD keeps the
  89 opcodes and grows the events to 31 (`docs/FINDINGS.md`, "Talk").
- **Talk flags.** `TalkFlags[npc]`, eight bits per NPC, set and tested by the scripts. In
  all three games they are also the story's state.
- **Game state block** (`gamestat.c`). SE still carried most of U6's: karma, wind, spell
  effects, moonstones, moons, the powder keg, Gargish. MD removed those and added the dream's
  variables, the player's sex and the talents.

The formats stayed stable, and so did the editors' output formats with them, apparently: the
map and object-block layouts did not change. What changed was the meaning of the numbers in
them. U6's `obj.h`, `tile.h` and `spells.h` "mean nothing in Savage Empire"
(`SE-WRITEUP.md`). Object type 0x44 is a travel object in SE; MD has no travel objects at all.
Every new game renumbered the world, so every type test in the C code had to be found and
rewritten.

### 1.5 How features were added

**Kept.** The core loop (`GameLoop`, `PassTime`, the 11x11 view and its flood fill, the
console's command codes from 0x8e up, the save format) and the conversation interpreter.
talkdr.c is the clearest case: 99% the same in MD as in SE, and SE's in turn is U6's plus one
opcode.

**Rewritten.**
- *The use command.* SE's `CmdUse` (objcmd.c, SE address C_1DBD_8276) is a chain of tests:
  map tiles first (`IsTreeTile`, `IsLavaTile`, `tile == 0x34` for the yucca plant), then
  object types, then a long run of handlers. It still tests `IN_VEHICLE` and prints a
  `NotOnShipMsg` inherited from U6's ships. MD's `use.c` replaces the chain with two tables.
  `UseObjects` has 125 rows of (type, up to ten action numbers). `UseActions` has 75 rows of
  (types allowed as the second object, types allowed as the third, function). `UseObject`
  collects the actions for the type, asks "On - " and "To -" while some action wants another
  object, drops the actions that refuse the choice, and calls the most specific one left.
  The table is compiled into the program as two far arrays of C initialisers, and its
  function pointers point at overlay stub entries. So it is *table-driven*, but not *data-
  driven*: changing a combination still means recompiling.
- *Levelling up.* U6 raised levels through the shrines' conversation scripts (`OP_ADDLVL`).
  SE's `REST_LevelUp` asks a bear, a jaguar and a monkey questions in a dream. MD's asks
  questions at three obelisks. Each time the rule is C code in rest.c.
- *Magic.* A spellbook with reagents in U6, "Invoke" in SE, nothing in MD except
  `SPELL_ReviveNPC`.
- *Portraits, the sky bar, cut scenes.* SE chose a portrait background by place and time of
  day. MD chooses it from a table by NPC. SE had four cut scenes and `ShowMidgame`. MD has 13
  cut scenes in an overlay of their own, each a C function that also changes the world.

**Bolted on.**
- *The dream* is a state, not a mode of the engine. The party "becomes" NPC 0 by reusing
  U6's vehicle state (`IN_VEHICLE` is still `Active == PartySize`; MD has no vehicles). Bit
  0x10 of `GameFlags` marks the dream. `GameFlags` appears in 19 C files, and the findings
  list what tests it: the status panel, `mk_npcnum` in the talk code, rest, buckets, plants,
  the air and cold checks.
- *Machinery.* Belts, drawbridges, barges, levers, jetters and lamps keep their state in
  talk flags: `TalkFlags[0x73] & 4` is underground power, `& 0x10` surface power,
  `TalkFlags[0x76] & 0x40` the canals. `__ObjectsDeserialize` and `OBJ_CommitBlocks` in
  object.c, the core of the object system, keep the machines' objects in step with those
  flags whenever an area loads or saves.
- *Survival rules.* Air (oxium), cold at night, lamp fuel, melting ice and hot radium all
  live in `PassTime` in gameloop.c, next to U6's clock.
- *The wrapping map.* MD's surface wraps at 1024 tiles. Sums are masked with `& 0x3ff` and
  differences go through `WrapDelta`. Those two spellings occur 90 times in SE's C and 506
  times in MD's, spread over 32 of the 37 files, from npctrack.c (78) and object.c (55) to the
  use actions and the cut scenes.

**Special-cased by number.** One rough count: lines comparing a value with a hexadecimal
constant (`case 0x..`, `== 0x..`, `!= 0x..`) number about 780 in SE's game C and 930 in
MD's. U6's readable source has about 70 such lines plus about 510 comparisons with `OBJ_` or
`TIL_` names, so roughly 580. The count includes flag and tile tests too, but most are object
types and NPC numbers: NPC 6 has 240 hit points, NPC 0x40 hurls the player south
(`COMBAT_RasputinThrow`), killing certain NPCs sets `IsArmageddon`, type 0x17a turns into
0x17c, the drawbridge is 0xc3/0xc4. Talk flags tell the same story more sharply. The engine's
C reads or writes `TalkFlags[...]` 11 times in U6 (in 4 files), 58 times in SE (6 files) and
327 times in MD (24 files). In U6 the story's state belonged to the scripts. By MD it was
everywhere in the engine.

**Left behind.**
- *U6 in SE:* the moonstone and moon variables of the game state block (with U6's values),
  ship wind and raft drift costs, `CyclopsFlag`, the spread-direction table, a stub
  `OBJ_getHead`, `NotOnShipMsg`, and "Winona Ryder" as the scratch buffer for names in talk.c
  and rest.c.
- *U6 and SE in MD:* `CommandNames` still lists "CAST". `WindDirNames`, `ShipWindCost`,
  `SkiffCurrentCost` and `RaftDriftDir` remain in npctrack.c and gameloop.c. `CampOfsX/Y` and
  `CampFacing` remain in rest.c, `DiskNames` in disk.c, two unused words in the game state.
  `DISK_confirm` survives as an empty function where SE's was, and `TileDamage`, the hook for
  harmful map tiles, is empty too. U6's `XRayView`, absent from SE, came back for the
  telescope.
- *Copies that drifted:* MD's objtype.c reads `MonsterDamage[class]` where combat.c reads
  `[class + 1]`, so a creature's own attack takes the previous class's damage. That is the
  kind of mistake you get when a table moves to a new module and one of its readers is
  copied from an older layout. (The off-by-one is a fact. The cause is a guess.)

### 1.6 The changes and their likely reasons

| Change | Evidence | Likely reason |
|---|---|---|
| Turbo C 2.0 → Turbo C++ 1.00 | C0M, CM.LIB, frame habits (`SE-WRITEUP.md`) | new team, current tools (guess) |
| 1.01 heap patched into SE's library | `tlib -+` modules at the library's end | a heap fix (guess) |
| Launcher, intro and endgame in one program | SAVAGE.EXE, MARTIAN.EXE | memory: keep the game program lean |
| New shared library (EMS heap, LZC, MIDI) in SE | `SRC_S/LIB`, linked in both programs | new hardware support (EMS, MIDI cards) and one library for both programs |
| SE keeps U6's modules as they were | identical module list and order | schedule: edit in place |
| MD splits objcmd.c, talkdr.c, npctrack.c | `docs/FINDINGS.md`, "Modules reorganised" | overlays need module boundaries; tidiness |
| Overlays in MD | 17 overlaid modules, 66,603 bytes | conventional memory: tile cache, DGROUP full |
| Table-driven use in MD | `use.c` | many object combinations; the chain had become unmanageable (guess) |
| Wrapping map, double tiles, view rotation | 506 wrap sites; `FindLoc`'s four cursors; `ViewRotation` | the design: a spherical Mars, big creatures, the dream |
| Removed magic, floppies, vehicles | `docs/FINDINGS.md`, "What was removed" | design; hard-disk installs by 1991 (guess) |
| Dead data left in place | the leftover lists above | removing costs testing time and gains nothing on screen |

---

## Part 2. Why the reuse did not save what it was meant to

### What was said

Three sources bear on the claim, and they are not equally strong.

- **The plan.** Jimmy Maher's history of the series (*The Digital Antiquarian*, "The Worlds
  of Ultima", 23 February 2018) says each spin-off was meant to take close to six months on
  the previous Ultima's engine, against about two years for a numbered Ultima, with a
  separate team. It adds that Ultima VI had needed four programmers and five artists, and
  that each mainline game sold a steady 200,000-250,000 copies while costs rose. It names
  Jeff Johannigman (producer), Aaron Allston (designer) and Stephen Beeman for Savage Empire,
  and Warren Spector leading Martian Dreams.
- **The intent, in a developer's words.** In the RPG Codex's retrospective interview with
  Warren Spector (22 October 2013), Spector says the series was his idea. He describes the
  aim as more games, quickly and cheaply, from the old code, and calls the spin-offs
  "content plays, not tech demos". He also says they had smaller budgets and tighter
  timelines, and that neither game sold well. He does not say the reuse failed to save time
  or money.
- **The outcome.** The claim that reuse did not save money is attributed to Richard Garriott,
  second hand. A fan's blog, Pix's Origin Adventures ("Through the Portalarium", 21 October
  2012), recounts a Q&A with Garriott at Portalarium. By that account Garriott said Savage
  Empire was Origin's first attempt to reuse an engine. Stripping out the original assets was
  so much work that it cost about as much as Ultima VI. Martian Dreams was meant to learn from
  that and cost as much again, and both sold less than the mainline games. This is a
  paraphrase in a blog post, not a transcript: Pix's own summary, written about a week after
  an unrecorded Q&A that followed a tour of Portalarium's Austin offices. The same account
  was reposted on the Ultima Codex, which does not make it independent. A second search
  (Garriott, Spector and the spin-off teams' interviews, the Digital Antiquarian, forums)
  found no primary source and no independent corroboration; every repetition of the claim
  traces back to this post.

So the claim is as reported: it rests on one second-hand account of Garriott (Spector
confirms the aim, the tight budgets and the poor sales, not a cost overrun), and on the
fact that the series stopped after two games. The rest of this part argues from the code
about why reuse might have cost more than planned. It is hypothesis throughout. Each
hypothesis gets a confidence rating.

One point in Garriott's reported explanation is worth noting first. The cost he blamed was
*assets*: tiles, maps, portraits, conversations. The decompilations cannot see Origin's
content budget. But they show that the code, which was supposed to be the free part, was
not free either.

### H1. Only the game-neutral layer was reusable, and it was the smaller part

**Evidence.** The modules that crossed from SE to MD nearly unchanged are the interpreter,
console, music, file and memory library and OSI assembly (section 1.3). Together that is
roughly 40 KB of MD's 232 KB of Origin code. Everything that touches the game's objects was
reworked or rewritten: 43% of MD's code is below 0.5 similarity to SE, and SE's own figure
against U6 is 59%. MARTIAN.EXE reused 144 of its 192 functions, but the 17 it rewrote are
half its code.

**Argument.** A 1990 RPG engine was mostly game rules, not infrastructure. Reusing an engine
whose bulk was the previous game's rules meant inheriting code that had to be read, understood
and then mostly replaced. With a separate team, which is how Maher describes the spin-offs, the
reading was itself a cost.

**Confidence: high** that most game code was rewritten each time. **Medium** that this is
the main reason the savings fell short. Assets, as Garriott reportedly said, may have
mattered more.

### H2. Game rules lived in C, keyed by type numbers

**Evidence.** Each new game renumbered objects, tiles and NPCs, and U6's constants were
useless in SE. About 930 lines of MD's C compare against a hexadecimal constant. Engine
modules read talk flags 327 times in MD, against 11 in U6. The use command's actions,
24 KB in three overlays, are 88-94% new code. Things that a designer would think of as
properties are code: what hurts (`TerrainDamage`'s object 0x1cf, plus 0xd1, 0x17d, 0xd7 and
0xa8), what glows, which creatures leave a body, which NPCs are invulnerable and what they
say instead.

**Argument.** When rules are code, every new object costs a programmer, a compile and a test
of the whole program. Designers cannot try ideas without a programmer. And the next game
starts with code full of the last game's numbers, each of which must be found and either
changed or proven dead. The leftovers in section 1.5 are what was not found, or not worth
removing.

**Confidence: high** on the facts. **Medium to high** that this was a large cost. It is the
pattern the code shows most consistently.

### H3. Each game forked the whole source

**Evidence.** There was a shared library, but only below the game: SE's EMS heap, LZC files,
music and timer, compiled once and linked into both of SE's programs, then carried into MD
and partly recompiled with 1.01. Above that line, each game is a copy. SE is U6's 19 files
edited in place. MD is SE's files edited, split and extended. Fixes did not flow back: SE's
`CmdGet` uninitialised `found` and its `CmdMove` quantity comparison are still in MD
(`docs/FINDINGS.md`, "Bugs of the original").

**Argument.** Forking is the fastest way to start a game and the slowest way to finish the
next one. Nothing is shared, so nothing is protected. Any change to the core (the wrapping
map, double tiles, the view rotation) is free to touch every file, and does. The next team
inherits an engine that now carries two games' assumptions.

**Confidence: high** on the facts. **Medium** on the cost: for two games a fork may well have
been the cheapest option (see Part 3).

### H4. New features cut across the whole program

**Evidence.** The wrapping map touches 32 of 37 files (90 → 506 sites). The dream bit is
tested in 19 files. Removing the floppies meant deleting `DISK_confirm` calls from map.c,
gfx.c, object.c and sound.c, and leaving an empty function behind. Machinery state is
synchronised in object.c's load and save. The talents change reach in Get, Move, Use and
Drop, strength in `GetStr`, visibility in visible.c, and rest in rest.c.

**Argument.** A feature that touches 20 files has to be designed into each one, and each
file has to be tested again. Nothing suggests automated testing (what the programs do carry
is debug keys), so testing meant playing. One design decision for Martian Dreams, a map that
wraps around, meant work in almost every module the team had inherited.

**Confidence: high** on the facts. **Medium** on the cost.

### H5. Memory pressure forced restructuring

**Evidence.** DGROUP was close to full in all three games (section 1.2). MD moved the use
tables to far data and adopted overlays. Overlays meant new link rules: overlaid modules
linked where their data are, stub entries, `-Y` on every module, TLINK rewriting calls. They
also meant splitting modules along per-turn and occasional lines, which is what MD did to
objcmd.c and talkdr.c.

**Argument.** The engine was built for one game in 640 KB. A second game with more content
and more rules does not fit the same memory without restructuring. Restructuring a C program
of this size is work that adds nothing visible: no new object, no new scene.

**Confidence: medium.** The pressure is measurable. That it cost a lot of the schedule is
inference.

### H6. The engine had no notion of a "game definition"

**Evidence.** Where data files exist, they are good: `tileflag`, `basetile`, `animdata`,
`schedule`, the map and chunk files, `talk.lzc`. Everything else is compiled in: monster
tables (`MONSDATA.ASM`), messages (`MSGS.ASM`), use tables, entrance destinations
(`Entrances[24]` in map.c), the dream's stage setups (dreamstg.c's twelve functions). MD's
library even gained a new loader, `LZCTYPE.C`, for MD's own entry types.

**Argument.** If the set of things that make Savage Empire Savage Empire had been data,
stripping it out would have meant replacing files. Instead it meant going through C.
Garriott's reported remark about how hard it was to strip out the original assets fits this.
It also fits the content tools: tools built for U6's data had to follow every engine change.
The code cannot show those tools, but the formats they wrote mostly did not change.

**Confidence: medium.**

### H7. The plan was squeezed by the market and the schedule

**Evidence from the histories.** About six months per game on a smaller budget, less
marketing, sales of a "B" title. **Evidence in the code.** SE was built with a compiler a
few months old, linked with a hand-patched library. MD's GAME.EXE was linked on 23 May 1991,
the month it shipped. Debug code was left in both: MD's barge dump to a file "brg", debug keys
behind a talk flag. Known bugs went out: `TalkFlags[2] &= 4` for `&= ~4`, and an
uninitialised local in the radium action.

**Argument.** Six months is a short time to learn someone else's engine, rewrite half of
it, build a new world and ship. When the schedule is fixed, savings in code are spent on
features, not banked. MD has more systems than SE (dream, machinery, talents, wrapping map,
13 cut scenes) and about a quarter more code. Spector said the spin-offs were not meant to improve
the technology much. Martian Dreams improved it a good deal anyway.

**Confidence: medium.** The engine work in MD is a fact. Whether it pushed costs up
against plan depends on figures that are not public.

---

## Part 3. What could have been done differently

The constraints were real: real-mode DOS, 640 KB with a 64 KB near data segment, floppies,
Turbo C/C++ and TLINK, teams of a few programmers, months not years. Each option below is
judged against them.

### 3.1 A shared engine library with game modules on top

**What.** Split the code into an engine (object lists and iterators, the map and chunk
cache, the 11x11 view, schedules and path finding, the console, the talk interpreter, the
save format) and game modules (rules, use actions, events, survival, cut scenes). The engine
is compiled once into a `.LIB` with `tlib`, as Origin already did for its low-level library
and for U6's `OSISND.LIB`. Games call it and supply hooks: tables of far function pointers
for "an object was used", "a minute passed", "an NPC took a turn".

**Evidence it was within reach.** Origin used function pointers in these programs: the
use table's `func`, the driver entry points, the `tHandler` union, `DriveErrorCallback`.
Origin's own library was already shared between two programs and kept on a different
compiler version.

**Cost then.** High up front. The engine's modules share hundreds of near globals; MD's
sources declare their externs by address, module by module. Drawing a line through that
means deciding which globals belong to the engine, and paying for indirection on a 286. More
seriously, both later games needed core changes: wrapping, double tiles, rotation. A library
would have had to absorb those as options, which is exactly the work that makes engines
expensive. **Verdict:** worth it from the second game on, if a third and fourth were
planned. For two games on a six-month clock, the fork was a defensible choice. The
cancelled third game (Arthurian England, per Maher) is the case where it would have paid.

### 3.2 Data-driven object behaviour

**What.** Put object properties in a data file that the engine reads, as `tileflag` already
does for tiles: light level, fuel, burns, hurts with damage kind n, container capacity,
weight class, "leaves body", "living". Load the use-combination tables from a file instead of
compiling them.

**The step MD took.** use.c is that design half done. The combinations are already a table
of types and action numbers. Only the actions themselves need code. Moving `UseObjects` and
the `on`/`to` lists of `UseActions` into a file, with the function looked up by action
number, would have let a designer change combinations without a compile. objtype.c, which
collected type tables from four SE modules, is the same instinct.

**Cost then.** Low. A loader is a few dozen lines, and the LZC library already loads typed
entries. Memory cost is a few kilobytes, possibly in EMS. The real cost is discipline:
every time a programmer is tempted to write `if(type == 0x1c0)`, a property must be added
instead. **Verdict:** the cheapest change with the biggest return. It would have taken most
of the 930 numeric comparisons out of the C.

### 3.3 Scripting for game logic

**What.** Extend the talk interpreter so that scripts can run on events other than
conversation: an object used, a trigger stepped on, an hour passed, an NPC killed. The
interpreter already has variables, flags, object and inventory opcodes, party management and
`OP_EVENT` to call C.

**Evidence it was within reach.** talkdr.c is the best-reused code in the three games: 88
opcodes from U6, one added by SE, unchanged in MD at 99% similarity. Scripts are loaded on
demand from `talk.lzc`, so they cost memory only while they run. MD already runs the
interpreter from an overlay. What happened instead goes the other way: SE's `OP_EVENT`
lets scripts call C, and MD grew `TALK_Event` to 31 C-coded events. Story logic moved from
scripts into the engine (talk flags 11 → 327).

**Cost then.** Moderate. The bytecode would need new opcodes for world changes (create,
move and delete objects, set frames, start a cut scene). Somebody would need to extend the
script compiler. Speed matters little for events that fire rarely. Origin did make this
move a year later: Ultima VII attached scripts ("usecode") to object types. **Verdict:**
the best long-term choice and a natural growth of what existed. It was probably too big a
change for SE's schedule, but within reach for MD's.

### 3.4 Overlays planned from the start

**What.** Draw the per-turn/occasional line when the engine is designed. Keep each game's
content code (use actions, cut scenes, events, special A.I.) in overlay-sized modules from
day one.

**Evidence.** VROOMM came with Turbo C++ 1.00, the compiler SE used. MD's split is the right
split, but it was made after the fact, and it brought new link rules worked out the hard way.

**Cost then.** Low for a new design, moderate for a retrofit. The price is a few
milliseconds when an overlay loads from a hard disk, more from floppy. A game that wanted
to run from floppies (SE still did) had a reason to wait. **Verdict:** cheap and right for
MD's generation, a hard-disk game. Doing it in SE would have saved MD's restructuring.

### 3.5 Keeping numbers, formats and tools stable

**What.** Keep a fixed core of object numbers that the engine knows (party, bodies, doors,
light sources, containers), and give each game its own range above it. Keep the editors'
formats fixed, and version them when they change.

**Evidence.** The map, chunk and object-block formats did stay stable, and the code that
reads them came through almost unchanged. The renumbering of types did the damage. Bodies
are 0x155 in MD's `OP_RESURRECT`, torches 0xd0/0xd1 in SE; each game had to find every such
number again.

**Cost then.** Low, if decided before the second game's art was drawn. Tile sheets and type
numbers are tied together, so this is mostly an art-pipeline rule. **Verdict:** cheap, and it
would have made forking cheaper too.

### 3.6 Cleaning up as a rule

**What.** Remove a feature completely, or put it behind a compile-time switch, instead of
leaving its data and empty functions.

**Cost then.** Small per feature, but it needs test time, and every hour of testing was
taken from the new game. **Verdict:** useful, but the least important. The leftovers cost
memory (a few hundred bytes of DGROUP) and confusion, not much else.

---

## Summary

What the code shows:

- The *data model* and *file formats* of Ultima VI survived both later games almost intact,
  and so did the *conversation interpreter* and Origin's *low-level library*. Those were the
  real savings.
- The *game rules* were C code, keyed by object, tile and NPC numbers that each game
  renumbered. Savage Empire edited U6's 19 modules in place. Martian Dreams split them into
  37, largely along the line its overlays needed, and rewrote 43% of the code.
- Story state moved from scripts into the engine: talk flags are read in C 11 times in U6,
  58 in SE and 327 in MD.
- Memory was the constant pressure: DGROUP full in all three games, the graphics cache
  taking whatever was left, overlays in MD.
- MD's table-driven use command is the one clear move towards data. Its tables are still
  compiled in.

What is hypothesis:

- That the reuse saved less than planned rests on one second-hand account of Garriott's
  words and on the series' cancellation. From the code, the most likely reasons are rules in
  C keyed by numbers (high confidence), whole-source forks (medium), features that cut across
  every module (medium) and memory restructuring (medium), all under a six-month schedule
  that turned savings into features (medium).
- The cheapest improvement then would have been data-driven object properties and loaded use
  tables. The most valuable would have been scripting game logic in the interpreter Origin
  already had, the direction Ultima VII later took.

## Sources

Repository documents: `MD-WRITEUP.md`, `SE-WRITEUP.md`, `U6-LESSONS_DECOMPILATION.md`,
`docs/FINDINGS.md`, `docs/MAP.md`, `docs/PLAN.md`, `docs/match_game.txt`;
`..\u6se-decompiled\README.md`, `docs\FINDINGS.md`, `docs\MAP.md`, `SRC\DECOMP.md`;
`..\u6-decompiled\README.md`, `FINDINGS.md`, `SRC\u6.lnk`, `SRC_U\DECOMP.md`. Sources
checked: MD `SRC/use.c`, `main.c`, `combat.c`, `console.c`, `disk.c`, `gameloop.c`,
`BSS.ASM`; SE `SRC/objcmd.c` (`CmdUse`), `talkdr.c`, `gamestat.c`, `talk.c`, `rest.c`,
`BSS.ASM`; U6 `SRC/*.c`, `BSS.ASM`. Counts (talk flags, hexadecimal comparisons, wrap sites,
line counts) are `grep` over the C game modules of each `SRC/` folder. Per-module similarity
is from `docs/match_game.txt`.

Published sources:

- Jimmy Maher, "The Worlds of Ultima", *The Digital Antiquarian*, 23 February 2018:
  https://www.filfre.net/2018/02/the-worlds-of-ultima/
- "RPG Codex Retrospective Interview: Warren Spector on Ultima, Origin, and CRPG Design",
  RPG Codex, 22 October 2013: https://rpgcodex.net/content.php?id=9190
- David Taylor, interview with Warren Spector, *Game Bytes*, 1993 (on the series'
  disappointing sales; nothing on engine costs): https://wcnews.com/articles/art50.shtml
- Pix, "Through the Portalarium", Pix's Origin Adventures, 21 October 2012 (a blog account of
  Richard Garriott's remarks in a Q&A; paraphrase, not a transcript):
  https://www.pixsoriginadventures.co.uk/through-the-portalarium/
