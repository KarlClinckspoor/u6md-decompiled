# Decompiling Martian Dreams: what we learned

Martian Dreams (Origin, 1991) is the second game on Savage Empire's version of the Ultima VI
engine. Its two executables, `MARTIAN.EXE` and `GAME.EXE`, are now C and assembly that
rebuild them byte for byte. This write-up covers what the work taught: what carried over
from Savage Empire, what was new (above all the overlays), what Turbo C++ did with the
source, how the work was organised, and how the result compares with Savage Empire.

Shorter references: `docs/FINDINGS.md` lists the engine's differences from Savage Empire
point by point, `docs/GAME_NOTES.md` has the compiler notes as the workers wrote them
(S1-S25 from Savage Empire, M1-M35 from here), and `docs/MAP.md` maps both executables.
`SRC_M/DECOMP.md` and `SRC/DECOMP.md` give each program's rules and status.
`SE-WRITEUP.md` and `U6-LESSONS_DECOMPILATION.md` are the write-ups of the two earlier
games.

## The result

| Program | What it is | Origin's code | Modules | Rebuilt by |
|---|---|---|---|---|
| `MARTIAN.EXE` | launcher: configuration, title, introduction, character creation, endgame and credits | 62,828 bytes | 72 | `mdbuild.py --target martian` |
| `GAME.EXE` | the game, with 17 VROOMM overlays | 165,185 bytes resident + 66,603 overlaid | 77 (17 overlaid) | `mdbuild.py --target game` |

Both rebuild equal to the GOG release, header and relocation table included:

| Program | Size | Relocations | sha1 |
|---|---|---|---|
| `MARTIAN.EXE` | 118,480 | 1,200 | `5e0e77776cdfe1222254ae4663c9f252a2271780` |
| `GAME.EXE` | 341,712 | 3,202 (and each overlay's own fixups) | `6b6b91fd6291d6d4d7b91615e5b6333c3e2b68a8` |

For GAME.EXE that includes the `FBOV` block after the MZ image: the 17 overlays, each with
its fixups. "Modules" counts the rows of `modules.txt`: four in each program are data only
(68 and 73 modules have code). `INSTALL.EXE` is set aside, as it was for Savage Empire.

The decompilation ran from 13 to 15 November 2026. MARTIAN.EXE came first, on the 13th,
as the calibration run. GAME.EXE was identical early in the afternoon of the 15th. The
names pass took the rest of that afternoon.

## What carried over from Savage Empire

Almost the whole method. Savage Empire was built with the same compiler, so its notes
applied from day one.

- **The compiler was already known.** The first 0242h bytes of both programs are Turbo
  C++ 1.00's `C0M.OBJ`. `libscan.py` found the run-time library to be 1.00's `CM.LIB`,
  unmodified this time. Savage Empire's had 1.01's `FARHEAP`, `NEARHEAP` and `FINDFIRS`
  appended with `tlib`. So SE's `CMSE.LIB` step was dropped and the libraries are linked
  as shipped: `EMU MATHM CM` for MARTIAN.EXE (main.c's `Calibrate` takes a square root),
  `OVERLAY CM` for GAME.EXE.
- **The tools.** `tools/md/` is a fork of SE's `tools/se/`, with `target.py` entries for
  both programs: `secmp.py` became `mdcmp.py`, `secheck.py` became `mdcheck.py`, and so on.
  `tools/se/` stayed untouched as the reference.
- **The compiler notes.** `docs/GAME_NOTES.md` began as a copy of SE's 25 notes. Those
  rules held: register choice, `-y`, how conditions compile, data layout, `BSS.ASM`.
- **Starting points from SE's sources.** `mdmatch.py` pairs every Martian Dreams function
  with the closest SE function by its instruction sequence (`docs/match_game.txt`,
  `docs/match_martian.txt`). `mdport.py` is new. It compiles an SE source as it is,
  compares it with the MD module it matches, and rewrites the address names (`C_SSSS_OOOO`,
  `D_XXXX`) to MD's where the comparison can vouch for them. `build/port/` holds every MD
  module's closest SE source run through it, and `build/portall.txt` says how far each
  one matches as it is. 29 of MARTIAN.EXE's 72 modules came over this way with only their
  addresses moved. In GAME.EXE, music.c and the shared library from MEMMOVE to EMSALLOC
  came from MARTIAN.EXE's copies the same way, and KBD, MOUSE, DELAY, RAND and OSI_FILE
  from SE's GAME.EXE.
- **The link's lessons.** `TLINK /i`, `BSS.ASM`, data-only modules, module starts that are
  not :0. GAME.EXE also has SE's segment order: C0M's `_BSSEND` comes before `_BSS`, so
  the start-up code clears nothing. `SEGORDER.ASM` came over unchanged.

## What was new

### Overlays

GAME.EXE uses Borland's VROOMM overlays: 1.00's `OVERLAY.LIB` (from `XLIB.ZIP`) and TLINK
`/o`. Nothing in the SE tools knew about them. 17 modules are overlaid, 66,603 bytes of
code: talk and its interpreter, the dream (three modules), the cut scenes, rest, the
machinery, revive, disk, and the three modules of "use" actions.

- **One address space for the tools.** `target.load_image()` puts each overlay's code at
  100000h plus its file offset. Its functions are named `C_<stub>_<offset>`, and mdcmp,
  mdcheck and mddis take an overlaid module by its stub segment. An overlay's relocated
  words hold an offset into the segment table (entry * 8), not a segment. `load_image`
  turns them into the segment as the overlay manager does (the stub, for overlaid code),
  so far calls out of an overlay resolve like any other.
- **`-Y` changes no code.** This was the first day-one experiment, before any C was
  written. It only adds a COMENT record (class EAh) that TLINK `/o` checks. `mdbuild`
  adds `-Y` to GAME.EXE's modules.
- **TLINK rewrites calls.** A far call from an overlay to a later function of the same
  overlay becomes `nop / push cs / call near`. mdcmp accepts that form.
- **The segment table is a module map for free.** GAME.EXE's `_EXEINFO_` lists 102
  segments: 63 resident code segments, 17 overlay stubs, and the data and empty ones. Each
  entry gives a code segment's first and last offset, which settled every module boundary.
  In Savage Empire the automatic map had five starts wrong. A zero-length entry is a
  data-only module's empty code segment.
- **Stub entries.** A stub is `int 3Fh`, the overlay's place and size, then one 5-byte
  entry (`int 3Fh; dw offset`) for every function outside code can reach. A toy
  two-overlay link showed that the publics get entries; the full link showed their order.
  Every public of the overlay gets an entry, in the *reverse* order of the module's PUBDEF
  records. Statics that a far fixup reaches (a stored address, a call to a later function)
  get entries before them. Turbo C writes PUBDEFs in the order
  of its symbol table, which is a hash of the names. So Origin's names fixed an order that
  ours could not reproduce. 14 of the 17 overlaid sources give the original's order on
  their second line, as offsets:

      /* stub order: 0000 0AC1 0477 0688 02CE */

  `tools/md/stuborder.py` rewrites the object's PUBDEFs to match, in `mdbuild`. The line
  is keyed on offsets, not names, so renaming does not disturb it. The stubs also tell
  which functions were static: a function with an entry in the original is public, one
  without is static. All of talkdr.c's functions turned out to be public.
- **Overlaid modules link where their data are.** Their `_DATA` sits after use.c's (DS
  1CD6-29F4), so they go into the link right after use.c, between `/o` and `/o-`. Their
  stubs come after all the code anyway.
- **The link wrote its name and date into the file.** TLINK `/o` writes the output name
  and the DOS date into `_EXEINFO_`, after the segment table. GAME.EXE was linked as
  `ultima.exe` on 23 May 1991. `target.py` records both (`link_name`, `link_date`),
  `mdbuild` sets DOS's date before the link, and the link is
  `TLINK /m /i SEGORDER C0M <modules> BSS, ULTIMA, ULTIMA, OVERLAY CM`.
- **Function addresses stored as data.** An overlay function's address stored in a
  variable holds the stub entry (`DriveErrorCallback = DISK_NullCallback` gives 2F5B:0020).
  It needs a prototype before the definition to get a relocated segment (the same holds in
  resident code: without it Turbo C writes `mov [x+2],cs`).

### Turbo C++ 1.00 and 1.01 in one program

The run-time library is 1.00's, but Origin's own library C files were compiled with Turbo
C++ 1.01: HEAP.C, MEMORY.C, LZC.C, LIBOPEN.C, music.c, tmdrv.c, surface.c and others. The
game modules are 1.00. The OSILIB C files are mixed: SHPHIGH, PALUSED and FADESTEP need
1.01, FADEIN needs 1.00, and VMODE compiles the same under either. A file says
`/* compiler: Turbo C++ 1.01 */` on its first lines (22 files across `SRC/` and `SRC_M/`),
and `tcc.py`, mdcmp, mdcheck and mdbuild then use `tools/tcpp101`. What told the two
apart:

- After `X = expr; f(X);`, 1.00 reuses the register and 1.01 pushes X again from memory.
- 1.01 shortens a long expression assigned to an int, even through casts.
- 1.01 tests a register variable after a call (`mov di,ax / cmp di,-1`) where 1.00 tests AX.
- Long sums come out in a different order.

The rule that came out of it: when a library file stalls, try the other compiler.

### TLINK ignores case

TLINK without `/c` ignores case, and Origin's link did too. A public whose name differs
from a library function's only in case silently takes that function's place, and the
library module is never linked. MARTIAN.EXE's main.c has its own `Delay` and `Exit`, so
they are called `Pause` and `ExitGame` here. The names pass met the same trap with no
message from TLINK at all: music.c's `timerHook` against PIT.ASM's `_timerHook` left 2 bytes different.

### Data-only modules and far data

- **Data-only modules.** MARTIAN.EXE has four: `OBJHDR.ASM`, `LIB/drvdata.c`,
  `OSILIB/OSIDATA.ASM` and `OSILIB/MOUSDATA.ASM`. They showed up as gaps between the
  modules' `_DATA` ranges. Their place in the link came from where their relocations
  (far null pointers) sit in the .EXE's relocation table. GAME.EXE's four (`gamestat.c`,
  `OSILIB/drvnames.c`, `MSGS.ASM`, `MONSDATA.ASM`) show up in the segment table as empty
  code segments. An empty WORD-aligned one adds a byte of padding before the next module.
- **Far data.** The far messages (segment 2BB4) and the monster tables (2DE5) are each one
  paragraph-aligned segment of class `FAR_DATA`, with a public name at every string or
  table. In Savage Empire they matched as huge-model C (`-mh -zTFAR_DATA`). Here they are
  written as assembly, `MSGS.ASM` and `MONSDATA.ASM`. Declared `far` in C, every variable
  would get a paragraph-aligned segment of its own. use.c's `UseObjects` and `UseActions`
  are exactly that: two C far variables, one segment each. mdcmp learned to compare a
  module's own far variables, located by their `/*C_SSSS_0000*/` comments. One small trap:
  MASM 3.0 does not read `''` inside a quoted string as a quote, so the source writes 39.
- **OSI_FILE joins C0M.** `OSILIB/OSI_FILE.ASM` sits at 0242, right after C0M, but it is
  linked after combat.c: its relocations follow combat.c's, and its code segment is
  `_TEXT`, so TLINK puts it in C0M's segment.

### The order of fixups

TLINK lays out the relocation table in the order it meets the fixups, so the order of
fixups within an object shows in the .EXE.

- **Assembly.** MASM 3.0 writes a record's fixups in code order. Origin's TASM did not
  always. events.asm and CALIBDRV.ASM name TASM's order in a `;TASM fixup order:` line,
  and `tasmorder.py`, from the Ultima VI tools, rewrites the object.
- **C.** Turbo C writes a C module's fixups last to first, but in code order when the file
  went through the assembler (inline asm). `LIBOPEN.C` needs `#pragma inline` for that.
  `emsmerge.c` has one statement in inline asm (no C form gives its word read), so MASM
  assembles it. A `/* fixup order: */` comment puts the fixups back in Turbo C's order.
- **TASM's other habits.** MEMMOVE.ASM and OSIREAD.ASM were assembled with TASM's `JUMPS`.
  Forward jumps are padded with NOPs, and the sources write those bytes with a `JPAD`
  macro. MASM 3.0 encodes register-register `XCHG` and `TEST`, and `OR reg,imm8`,
  differently from TASM, so those are `db`.

## What Turbo C++ did with the source

The compiler is deterministic, but much of what it does is not stated in the source. The
35 Martian Dreams notes in `docs/GAME_NOTES.md` record what the workers found, with
addresses. The most instructive points follow.

### Line numbers, again

`-y` stops `-O` from merging identical code that comes from different source lines. All of
GAME.EXE's C game modules but main.c and disk.c were compiled with it. In Savage Empire it
was 13 of 19. MARTIAN.EXE's main.c and title.c have it too, and its main.c also lacks
`-d`, so its repeated literals ("fair.lzc") are separate strings. As in SE, a module that
compiles too short is the first sign to add `-y`.

### Register variables are found by trial

SE's rules (most-used variable gets SI, on a tie the later-declared) were a starting point.
They did not always hold:

- In visible.c, use.c and power.c, `register int16 i, score;` gave `i` DI and `score` SI,
  four times. In objcmd.c, `register int16 a, b;` gave `a` SI and `b` DI in all five
  functions that have it.
- In useact2.c's `PanelRubyLenses`, `register int16 n, t;` and `register int16 t, n;` gave
  the same wrong pair. Only `register int16 t; int16 n;` worked, with `n` left to the
  compiler.
- In useact3.c, `register int16 o = -1, f;` gave `o` DI and `register int16 o = -1, y;`
  gave `o` SI. The note says "by trial".
- A `register` parameter with a `register` local usually gave the parameter SI, the
  opposite of SE's note S4. A plain parameter with a `register` local gave the parameter
  DI.
- A variable declared inside a block whose life holds no call can be kept in DX, with no
  stack slot (`COMBAT_AI_Berserk`).

When SI and DI come out swapped, swapping the declarations is still the first thing to try.

### Cross-jumping shows what was written twice

`-O` still merges many identical tails under `-y`. How it merges them shows the original
source:

- `{ A; return 0; }` written out three times in one function becomes `jne L / jmp last`:
  every copy jumps forward to the *last* one. One shared label reached by `goto` gives a
  plain conditional jump instead (`BargeCanMove`). So a conditional jump over a `jmp`
  means a repeated block.
- Identical bodies in separate `else if` branches are merged the same way.
  `COMBAT_AttackWithNextWeapon`'s eight out-of-ammunition branches are separate else-ifs;
  one `||` condition gives direct conditional jumps instead.
- Any label between the copy and its target stops the merge: the end of an `if`, or the
  end of a block that declares locals. `default: if(f & 0x20) { A; S; } break;` merges
  nothing, while `default: if((f & 0x20) == 0) break; A; S;` merges every `case: S; break;`
  into it.
- Writing one copy as `return B();` (the same bytes) keeps two identical blocks apart.
- An if/else and the equivalent `?:` can give the same code, but after the if/else the
  compiler forgets its registers and after the `?:` it does not. `COMBAT_Kill` has one of
  each.
- Where the compiler kept a copy the original did not, a `goto` reproduces the jump
  (`UseOil`: `else goto fail; } else { fail: CON_printf(...); }`).

### Byte tests

On a byte flag, the way a test is written decides the instruction:

```c
if(!(ObjStatus[obj] & 0x18))       /* mov al,[..] / mov ah,0 / test ax,18h */
if((ObjStatus[obj] & 0x18) == 0)   /* test byte ptr [..],18h */
```

This holds in `&&` and `||` chains too, but not everywhere: as a `for` condition, the bare
`ObjStatus[o] & 0x18` gives the `mov al` form, while in an `if` it gives `test byte`. A
positive test in the `mov al` form needs `!!x`. The notes come back to these forms again
and again (S17, M18, M20, M23, M24, M30, M33, M34).

### Switches

- `switch(v)` on a plain local copies `v` into a free register (`cmp [cs:bx],di` in the
  search). `switch(v = f())` compares AX. `switch(GetType(i))` keeps the value in a
  compiler temporary on the stack, and `switch(t = GetType(i))` adds a variable.
- The case table is sorted by value. The source order of the cases shows only in where
  their code is.
- A case that returns the default's value stays in the table, and one that only breaks is
  dropped from it:

  ```c
  switch(t) { case 0x102: return 1; ... default: return 1; }   /* 0x102 in the table */
  switch(t) { case 0x102: break; ... } return 1;                /* 0x102 dropped */
  ```

- A switch on five sparse values gives a value-search loop
  (`mov cx,5 ... cmp ax,cs:[bx] ... loop`), with the cases' code in source order.
- Locals declared in sibling `case X: { int16 a, b; ... }` blocks share frame slots after
  the function's locals and all its switch temporaries (`GameLoop`).

### Small things that cost hours

- **`abs()` is always inlined.** It is inlined (`cwd / xor / sub`) even when declared as a
  function. `UseSextant`'s `lat = (abs)(lat);` gives the original's `call _abs`.
- **Implicit declarations compile silently.** A function used with no prototype or macro
  becomes an implicit far call. mdcmp then reports "learned N names", and it checks those
  names not at all. The briefs made "keep N at 0" a rule. A wrong `D_` offset hides there
  too.
- **`-d` shares string tails.** `"%s%>"` is the tail of `"\n%s%>"`, and `"!\n"` and `"\n"`
  are the end of `"It's hot here!\n"`. An initialised `char *a[]` puts its strings in the
  pool where the array is defined, not where they are used.
- **`min` and `max` are macros** and evaluate their arguments twice.
  `max(4, min(10, 20-i))` shows it.
- **`0L` as an argument** is `xor ax,ax / xor dx,dx / push ax / push dx`. A long constant
  is pushed high word first: `TimerDelay(476600L)` is `mov ax,7; mov dx,45b8h`.

## How the work was organised

`docs/PLAN.md` set out seven phases: 0, the toolchain on this machine; 1, the tools; 2,
day-one experiments (C0M, `-Y`, a toy `/o` link, one SE module compiled at its MD address,
`-y` by length); 3, the shared library and MARTIAN.EXE; 4, GAME.EXE; 5, the link; 6,
names and findings. Phase 2 answered the questions that would otherwise have shown up as
the last bytes.

- **MARTIAN.EXE: parallel workers.** Several workers shared the checkout, each with its own
  files, under `docs/MD_BRIEF.md`. Round 2 had nine of them. The main module was split
  among four (Intro, character creation, the intro scenes and credits, the rest) and
  merged by `build/merge/merge.py`, with a common `head.c` for the types, the initialised
  data and the declarations.
- **GAME.EXE: one worker at a time.** To save tokens, the workers ran sequentially from a
  queue (`build/game_queue.txt`) and a shared brief (`build/game_brief.md`). Each took one
  module or a bundle of small ones (`222C 100B 2F55 2F5B`, then `1534 1649 2F80` ...).
  Each was checked with mdcmp and mdcheck before the next started, and committed when
  verified. That made about 30 worker rounds at roughly 150,000 to 470,000 tokens each.
  GAME.EXE's history has 23 commits from the first modules to the identical link.
- **Notes as the hand-over.** Every worker added one note to `docs/GAME_NOTES.md`: the
  compiler habits found, Martian Dreams' changes from SE, and every address and name it
  learned. The next worker started from them. The later notes (M26-M34) run from about
  800 to 2,000 words each. They are also where `docs/FINDINGS.md` and the names came from.
- **Big modules in parts, compiled whole.** A module cannot be compiled in pieces: a
  function called before its definition gets a far call, one defined earlier
  `push cs / call near`. The large modules (object, objcmd, combat, the three action
  overlays) were worked as one file per function (`new/OOOO.c`), put in the module's order
  by `order.txt` and `assemble.py`, and compiled whole every time.
  `fd.sh` gave a function-by-function diff and `cmp.sh` ran mdcmp on the whole. combat.c
  (07D1, 25,686 bytes, the largest module) was split between two workers. The first
  finished 33 of 63 functions and left a hand-over in M30.
- **mdcmp has the last word.** The quicker per-function comparers had blind spots, and
  workers found them. `mdfunc.py` and `fd.py` skipped functions whose names begin with
  `__`. `fd.py` did not compare jump targets or short against near jumps. `mddis.py` ran
  on through switch tables into the next function. Each of those was fixed or worked
  around, and every module ended with mdcmp on the whole file: "0 bytes differ", fixups
  checked, no learned names.
- **The names pass in five batches.** Modules referred to each other by address until
  everything matched. Phase 6 then gave names in five batches, one at a time: combat,
  objects and object commands; NPC movement, map, eggs, barges and visibility; game loop,
  console, status, graphics, sound and the library; the 17 overlays; MARTIAN.EXE. Each
  batch ended with both builds identical. The address stays in the comment before each
  definition, which is how `mdsym.py` knows what is what.

Traps met along the way:

- **Silent name clashes at link time.** A new name could replace a library function or
  collide with another module's public without a word from TLINK (above). `mdbuild` could
  print "identical" while TLINK had reported an error, because TLINK still writes the EXE.
  So the names brief had workers check `--log` for "Error" and "Duplicate". Renaming can
  also reorder an object's PUBDEF and EXTDEF records, because their order is the name
  hash.
  `mdsym apply` could hand a module another module's static name.
- **Workers stopped by session ends or limits.** A worker could stop mid-module. Its partial
  work stayed in `build/<scratch>`, and a new worker began by verifying it. The briefs
  asked for partial results that say exactly where they stop ("identical up to +X").
- **Shell heredocs mangle backslashes.** In this shell, a heredoc halved `\\` in C strings,
  even quoted and even through Python. `"\n"` in a source written that way came out wrong.
  Three notes (M27-M29) warn about it. Sources were written with the editor tools
  instead, and combat.c's declarations were edited through `hedit.py` patch blocks.

## Martian Dreams against Savage Empire

GAME.EXE is 341,712 bytes against Savage Empire's 270,016, and it has 77 modules against
SE's 53. Its own code is 165,185 bytes resident plus 66,603 overlaid. SE's write-up gives
its GAME.EXE's code as 195 KB.

How much of Martian Dreams' code starts from SE's (`docs/match_game.txt`: every Origin
function of GAME.EXE, OSI assembly included, against the closest SE function):

| Closeness to SE | Functions | Code | Share of code |
|---|---|---|---|
| 0.85 or more (SE's function with small changes) | 298 | 70,250 bytes | 30% |
| 0.5 to 0.85 (reworked but recognisable) | 179 | 61,812 bytes | 27% |
| below 0.5, or no counterpart | 192 | 100,771 bytes | 43% |

The two halves differ sharply:

- **Resident code** (511 functions, 166,230 bytes): 36% at 0.85 or more, 30% below 0.5.
- **Overlays** (158 functions, 66,603 bytes): 14% at 0.85 or more and 77% below 0.5.

For MARTIAN.EXE, 144 of 192 functions are at 0.85 or more, but they are only 40% of the
code. 17 functions below 0.5 make up 48% of it: the introduction and character creation
that Martian Dreams tells differently.

So, as `docs/PLAN.md` estimated, about a third of the code started from SE's readable
source nearly as is. Martian Dreams is much closer to Savage Empire than Savage Empire was
to Ultima VI. By the measure SE's write-up used (0.5 or more), 57% of GAME.EXE's code
started from SE's source, against about 40% of SE's from Ultima VI. At 0.85 or more it is
30% here, against 5.8 KB, under 4%, there.

The modules that came over keep SE's names and comments where those are still true. The
new code is the dream, the machinery, the cut scenes, the table-driven "use" command and
its three overlays of actions, and the wrapping map. `docs/FINDINGS.md` describes it, and
what Martian Dreams removed: magic, floppies, vehicles and SE's big creatures. Some modules
were reorganised too. SE's 35 KB objcmd.c is spread over objcmd.c, objtype.c, use.c, the
action overlays and cutscene.c.

## The original's bugs, kept

A byte-identical rebuild keeps every mistake in the original source. The sources mark them
`/*sic*/` or explain them in a comment, and `docs/FINDINGS.md` lists them all. A few:

- `damage << 1;` in `LoseHP`, a statement whose result is dropped, still compiles.
- `x = (x + OSI_rand(0, 1)?10:-10) & 0x3ff;` in `COMBAT_AI_Like` and
  `COMBAT_AI_Unfriendly`: because of precedence, x becomes 10 or 0x3f6.
- `area_y = +PTH_DirY[dir] & 0x3ff;` in `PTH_BuildPath` is an `=+`, so nothing is added.
- `COMBAT_RangeToObj`'s default case returns `x / (x - x)`.
- `TalkFlags[2] &= 4;` where `&= ~4` was meant. The radium action tests an uninitialised
  local ("Radium has already been installed.").
- `GetDamagePoints` reads `MonsterDamage[class]` where combat.c reads `[class + 1]`. No
  caller passes it a creature (combat.c reads the table itself for a creature's own
  attack), so the slip has no effect.

## What remains unknown

- **How Origin's link placed OSIWRITE.** OSIWRITE.ASM is linked after OSIREAD.ASM (its
  relocation follows OSIREAD's), but its code sits after VIDRESET's. Our VIDRESET.ASM names
  OSIWRITE's segment first, which gives the same layout. How Origin's link got there is not
  known.
- **How Origin got `_BSSEND` before `_BSS`.** `SEGORDER.ASM` reproduces it, as in Savage
  Empire. The cause is still not known.
- **TASM's fixup order.** The rule could not be found without TASM itself. The two modules
  that need it list their order by hand.
- **Unused data and code.** MARTIAN.EXE has 16 variables that nothing reads (unused, or set
  and never read). They keep their address names. In GAME.EXE, gamestat.c's two unused words D_1682 and
  D_16D8 are still by address; OSILIB/CLIP.ASM's four routines are named although nothing
  calls them. U6 and SE leftovers such as WindDirNames, the ship and raft
  costs and CampOfsX/Y are named but unused.
- **Origin's names.** Every name in the sources is ours, or SE's where the function is
  SE's. The stub order shows only that Origin's names hashed differently from ours.
