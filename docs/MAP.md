# Martian Dreams executables: map

The GOG release's `MARTIAN.EXE` and `GAME.EXE`. All addresses are in the load image (the
MZ header removed): code as `FRAME:OFFSET` (the paragraph a far call uses), data as DS
offsets. Lengths are hexadecimal bytes up to the next module. The module tables are
`SRC_M/modules.txt` and `SRC/modules.txt` with a line on each module, taken from the
header comment of its source. `tools/md/` reproduces the tables (`segmap.py`, `modmap.py`,
`libscan.py`, `dsrefs.py`; `target.py` reads the overlays). How Martian Dreams differs from
Savage Empire is in `docs/FINDINGS.md`.

## Toolchain

Both programs were built with **Turbo C++ 1.00**, **TLINK 3.0** and an assembler that MASM
3.0 (`/Mx`) matches byte for byte:

- `C0M.OBJ` of 1.00 is the first 0242h bytes of both.
- The run-time library is 1.00's `CM.LIB`, unmodified. Savage Empire's had 1.01's
  `FARHEAP`, `NEARHEAP` and `FINDFIRS` appended. `GAME.EXE` also links 1.00's
  `OVERLAY.LIB` (VROOMM overlays, TLINK `/o`). `MARTIAN.EXE` links `EMU.LIB` and
  `MATHM.LIB` (8087 emulator, `sqrt`).
- Medium model (far code, near data). `MARTIAN.EXE`'s own code is cdecl, `GAME.EXE`'s
  pascal (`-p`), as in Savage Empire and Ultima VI.
- Origin's library C files (heap, memory, .LZC, `music.c`, the driver loaders) were
  compiled with **Turbo C++ 1.01**. The game code was compiled with 1.00. Each source file
  names its compiler and flags on its first lines.

Both rebuild byte for byte: `python tools/md/mdbuild.py --target martian` (`SRC_M/`) and
`--target game` (`SRC/`).

## MARTIAN.EXE

118,480 bytes; 1,200 relocations; header 1400h bytes; entry 0000:0000; DGROUP 172Ah;
stack 1BA5:0080. No overlays. It is the program you start: configuration, title,
introduction, character creation, the endgame and credits. It runs `GAME.EXE` with
`execl("game.exe", 0)`. Started as `martian.exe !`, it plays only the endgame.

The link: `TLINK /m /i C0M <modules> BSS, MARTIAN, MARTIAN, EMU MATHM CM`.

| Image | What |
|---|---|
| 0000-0242 | `C0M.OBJ` (Turbo C++ 1.00) |
| 0242-0EAE | `MATHM.LIB` and `EMU.LIB` modules in `_TEXT` (FPINIT ... _MATHERR) |
| 0EAE-4FE4 | `CM.LIB` modules (ACCESS ... SETENVP), 1.00's |
| 4FE4-14550 | Origin's modules (table below), 62,828 bytes |
| 14550-172A0 | EMU086, EMU087, E87TRAP (the 8087 emulator's own segments) |
| 172A0- | DGROUP (172Ah), then the start-up stack segment (1BA5) |

Data segment (DGROUP 172Ah):

| DS | What |
|---|---|
| 0000-00A8 | C0M's data |
| 00A8-1CB8 | Origin's initialised data, the modules' `_DATA` in link order (table below) |
| 1CB8-1CC0 | padding before EMU.LIB's paragraph-aligned FPINIT data |
| 1CC0-2366 | the library's data (`_CVTSEG`/`_SCNSEG` end at 2366) |
| 2366-465C | Origin's uninitialised data (`SRC_M/BSS.ASM`) |
| 465C-47AA | the library's uninitialised data (C0M clears 2366-47AA) |

### Modules (link order)

`-` in the first column: a module with data only (its place in the link comes from where
its relocations sit in the relocation table). An empty DS column: no `_DATA` of its own.
"1.01": compiled with Turbo C++ 1.01.

| Frame:at | Length | Source (`SRC_M/`) | DS | What |
|---|---|---|---|---|
| 04FE:4 | 7B8D | `main.c` | 00A8-0BC0 | the program: configuration, memory, drivers, the menu, the introduction, character creation, the endgame, the credits |
| - | 0 | `OBJHDR.ASM` | 0BC0-0C24 | data only: the game state block at the end of savegame\objlist (GAME.EXE's gamestat.c; PlayerIsFemale, AvatarPortrait: "a character exists") |
| 0CB7:1 | 223 | `OSILIB/FADEIN.C` |  | palette fades: fade a picture in, colours out (1.00) |
| 0CD9:4 | E5C | `title.c` | 0C24-0DF4 | the title (TITLE.LZC): letters, sunset, sparkles (SE's) |
| 0DBF:0 | 156 | `sprites.c` | 0DF4-0DFC | the sparkles of the title's last scene (SE's) |
| 0DD4:6 | 158 | `LIB/CALIBDRV.ASM` | 0DFC-0E39 | how many TM.DRV calls fit in a timer tick |
| 0DE9:E | 388 | `LIB/KEYB.ASM` |  | the keyboard interrupt (int 9) |
| 0E22:6 | 1C0 | `LIB/PIT.ASM` | 0E3A-0E48 | the timer chip: rate, interrupt vectors, a delay |
| 0E3E:6 | 24 | `LIB/LINPTR.ASM` |  | linear address to far pointer |
| 0E40:A | 22 | `LIB/PTRLIN.ASM` |  | far pointer to linear address |
| 0E42:C | 38 | `LIB/PTRNORM.ASM` |  | normalise a far pointer |
| 0E46:4 | 11E | `LIB/CALIB.ASM` | 0E48-0E69 | a delay loop calibrated on the timer |
| 0E58:2 | 51 | `LIB/RANDOM.ASM` | 0E6A-0E6E | random numbers |
| 0E5D:3 | 55 | `OSILIB/SHPHIGH.C` |  | shape files: the height of the shapes (1.01) |
| 0E62:8 | 5E | `OSILIB/PALSAVE.C` |  | save/restore the whole palette |
| 0E68:6 | 5A | `OSILIB/SHPMAXSZ.ASM` |  | shape files: the largest buffer a shape needs |
| 0E6E:0 | 44 | `OSILIB/SHPSIZE.ASM` |  | shape files: the buffer that saves what is under a shape |
| 0E72:4 | 9C | `OSILIB/SHPRECT.ASM` |  | shape files: the rectangle a shape covers |
| 0E7C:0 | 2A | `OSILIB/SHPCOUNT.ASM` |  | shape files: the number of shapes |
| - | 0 | `LIB/drvdata.c` | 0E6E-14F0 | data only: video modes, graphics driver names, the drivers' entry points |
| - | 0 | `OSILIB/OSIDATA.ASM` | 14F0-1510 | data only: the OSI library's shared variables |
| 0E7E:A | 28A | `OSILIB/TXTJUST.ASM` | 1510-1525 | text window: a string word-wrapped and justified |
| 0EA7:4 | E | `OSILIB/TXTSEL.ASM` |  | text window: choose the current one (not in SAVAGE.EXE) |
| 0EA8:2 | 18 | `OSILIB/TXTGOTO.ASM` |  | text window: move the cursor |
| 0EA9:A | 9E | `OSILIB/TXTCHAR.ASM` |  | text window: print a character |
| 0EB3:8 | 58 | `OSILIB/TXTSCRL.ASM` |  | text window: scroll up |
| 0EB9:0 | 21 | `OSILIB/TXTCOPY.ASM` |  | text window: copy a line of pixels |
| 0EBB:1 | 12D | `OSILIB/VMODE.C` | 1526-172D | video mode; allocation of the palette's colours (1.00 or 1.01) |
| 0ECD:E | 3C1 | `LIB/surface.c` |  | allocate a drawing surface for the video mode (1.01) |
| 0F09:F | 1FB | `LIB/screen.c` |  | the screen and back-buffer surfaces for each video mode |
| 0F29:A | 14 | `OSILIB/VIDPORT.ASM` |  | the CRT status port |
| 0F2A:E | 10 | `OSILIB/VSYNC.ASM` |  | wait for the start of a vertical retrace |
| 0F2B:E | A | `OSILIB/VRETRACE.ASM` |  | wait until the display is in vertical retrace |
| 0F2C:8 | 90C | `events.asm` | 18CA-1916 | timer tick (int 8) handler, keyboard (Escape, Alt-X, Ctrl-Alt-Del), mouse, input event queue (SE's) |
| 0FBD:4 | 56 | `OSILIB/MOUSEVT.ASM` |  | events: take the next one from the queue |
| 0FC2:A | 88 | `OSILIB/MOUSEINI.ASM` |  | mouse: initialisation |
| 0FCB:2 | 96 | `OSILIB/MOUSESRV.ASM` |  | mouse: follow its moves |
| 0FD4:8 | 3F | `OSILIB/MOUSESHO.ASM` |  | mouse: show the pointer |
| 0FD8:6 | 44 | `OSILIB/MOUSEHID.ASM` |  | mouse: hide the pointer |
| 0FDC:A | 26 | `OSILIB/MOUSERST.ASM` |  | mouse: restore the background under the pointer |
| 0FDF:0 | 7C | `OSILIB/MOUSESAV.ASM` |  | mouse: save the background under the pointer |
| 0FE6:C | 3C | `OSILIB/MOUSEDRW.ASM` |  | mouse: draw the pointer |
| - | 0 | `OSILIB/MOUSDATA.ASM` | 1916-1940 | data only: the mouse's variables |
| 0FEA:8 | 63 | `OSILIB/PALUSED.C` |  | the colours a surface uses (1.01) |
| 0FF0:B | 1AF | `OSILIB/FADESTEP.C` | 1940-1942 | palette fades: one step (1.01) |
| 100B:A | 36 | `OSILIB/PALSET.ASM` |  | set the whole VGA palette |
| 100F:0 | 15B | `LIB/tmdrv.c` |  | load TM.DRV and take its entry points (1.01) |
| 1024:B | A3 | `LIB/supertm.c` |  | load SUPERTM.DRV and take its entry points (1.01) |
| 102E:E | 7C | `LIB/palfile.c` | 1942-1946 | load a VGA palette from a file (1.01; not in SAVAGE.EXE) |
| 1036:A | 4C | `OSILIB/TXTCLRLN.ASM` |  | text window: clear a line |
| 103B:6 | AF | `LIB/MEMMOVE.ASM` |  | copy between overlapping far buffers |
| 1046:5 | 1C1F | `music.c` | 1946-1C38 | MIDI music and sound effects through STRAX.DRV (1.01) |
| 1208:4 | 120 | `LIB/emsmerge.c` |  | EMS: merge free blocks of the EMS heap (not in SE) |
| 121A:4 | 127 | `LIB/EMSINIT.ASM` | 1C38-1C56 | EMS: find the manager, take all free pages |
| 122C:C | 10C | `LIB/EMSMAP.ASM` | 1C56-1C5A | EMS: map a virtual pointer; save/restore the page map |
| 123D:8 | 164 | `LIB/EMSFREE.ASM` |  | EMS: free space in the EMS heap |
| 1253:C | 5D | `LIB/EMSNEXT.ASM` |  | EMS: the next block of the heap (not in SE) |
| 1259:9 | CC6 | `LIB/HEAP.C` | 1C5A-1C6E | the conventional heap (1.01) |
| 1325:F | 13C | `LIB/HEAPFREE.C` |  | free space in the conventional heap |
| 1339:B | 297 | `LIB/MEMORY.C` |  | MEM_alloc, MEM_free, MEM_size over both heaps (1.01) |
| 1363:2 | 290 | `LIB/LZW.ASM` | 1C6E-1C9A | LZW decompression |
| 138C:2 | 4C | `LIB/OSIOPEN.ASM` | 1C9A-1CA0 | open/close a file, the file error handler |
| 1390:E | F0 | `LIB/OSIREAD.ASM` | 1CA0-1CA4 | read a file, into EMS too |
| 139F:E | 38 | `LIB/OSISEEK.ASM` |  | move a file's pointer |
| 13A3:6 | 31D | `LIB/LZC.C` | 1CA4-1CB0 | .LZC libraries: decompress an LZW entry (1.01) |
| 13D5:3 | 84 | `LIB/LIBSIZE.C` |  | the size of a library entry (MD's entry types; 1.01) |
| 13DD:7 | 160 | `LIB/LZCTYPE.C` | 1CB0-1CB8 | load a library entry by its type (MD's variant of LZC_load; 1.01) |
| 13F3:7 | EE | `LIB/LZCLOAD.C` |  | load a library entry (SE's LZC_load; 1.01) |
| 1402:5 | 182 | `LIB/LIBOPEN.C` |  | open a library entry (1.01) |
| 141A:7 | A7 | `LIB/LIBREAD.C` |  | read from a library entry |
| 1424:E | E8 | `LIB/LIBSEEK.C` |  | seek in a library entry (1.01) |
| 1433:6 | 21A | `LIB/EMSALLOC.ASM` |  | EMS: allocate and free a block of the EMS heap |

Compared with `SAVAGE.EXE`: `LIB/palfile.c`, `LIB/emsmerge.c`, `LIB/EMSNEXT.ASM` and
`OSILIB/TXTSEL.ASM` are new; `LIB/tmdrv.c` and `LIB/supertm.c` are SE's `tmdrv.c` split in
two; `title.c`, `sprites.c` and `events.asm` are SE's with the addresses moved. From
`LIB/MEMMOVE.ASM` on, the library is the same code as `GAME.EXE` 2706-2B00. `SRC_M/BSS.ASM`
(the uninitialised data) has no row. All 68 modules with code have identical sources
(`tools/md/mdcheck.py --target martian` prints "72 of 72": the 72 rows, all with a source;
it compares the 68 with code, and the four data-only rows are checked by the link).

## GAME.EXE

341,712 bytes: header 3400h bytes, the MZ image (load image 257,712 bytes, ending at file
offset 422B0h), then the `FBOV` block of the overlays (70,688 bytes). 3,202 relocations
(resident code only: each overlay carries its own fixups). Entry 0000:0000; DGROUP 2FABh;
stack 3EE3:0080. The segment table (`_EXEINFO_`, file offset 325C0h) has 102 entries:
63 resident code segments, 17 overlay stubs, and data/empty segments. It gives every
code segment's first and last offset, which settled the module boundaries. An empty code
segment marks a data-only module.

The link: `TLINK /m /i SEGORDER C0M <modules> BSS, ULTIMA, ULTIMA, OVERLAY CM`, with the
overlaid modules between `/o` and `/o-`. The overlay data hold the output name and the
date: Origin linked `ultima.exe` on 23 May 1991.

| Image | What |
|---|---|
| 0000-0242 | `C0M.OBJ` (Turbo C++ 1.00) |
| 0242-0663 | `OSILIB/OSI_FILE.ASM` (linked after combat.c; its code joins C0M's `_TEXT`) |
| 0663-2CDD | `OVERLAY.LIB` (OVRHALT at 0663) and `CM.LIB` modules, 1.00's |
| 2CDD-2B21E | Origin's resident modules (table below), 165,185 bytes |
| 2B21E-2BB37 | `OVERLAY.LIB`'s overlay manager (`_OVRTEXT_`: OVRMAN at 2B21E, OVRUSER at 2BAE3) |
| 2BB40-2F0FC | far data (class `FAR_DATA`, each segment paragraph aligned): `MSGS.ASM` (2BB4), `MONSDATA.ASM` (2DE5), use.c's `UseObjects` (2E08) and `UseActions` (2E67) |
| 2F100-2F520 | the overlay manager's data, the segment table (`_EXEINFO_`), C0M's start-up and exit tables |
| 2F520-2FAB0 | the 17 overlay stubs (2F52-2FA1) |
| 2FAB0-3EEB0 | DGROUP (2FABh), then the 80h-byte stack segment (3EE3) |
| after the MZ image | the `FBOV` block: 17 overlays, 66,603 bytes of code, each followed by its fixups |

An overlay stub is a small segment: `int 3Fh`, the overlay's place in the `FBOV` block, its
size, then one 5-byte entry (`int 3Fh; dw offset`) for every public function of the
overlay and for every static one that a far fixup reaches. Calls into an overlay go to
the stub entries (`2F8D:0020 + 5k`). In the tools, an overlay's code is at 100000h plus
its offset in the block, and its functions are named `C_<stub>_<offset>`.

Data segment (DGROUP 2FABh):

| DS | What |
|---|---|
| 0000-00A8 | C0M's data |
| 00A8-1CD6 | the resident game modules' `_DATA`, in link order |
| 1CD6-29F4 | the overlaid modules' `_DATA` (they are linked after use.c, where their data are) |
| 29F4-2EC0 | the OSI and Origin library data (`EMMXXXX0`, `strax.drv`, `MThd` ...) |
| 2EC0-3232 | `CM.LIB`'s data |
| 3232 | C0M's `_BSSEND` (edata@) before `_BSS` (`SRC/SEGORDER.ASM`, as in SE's GAME.EXE): bdata@ = edata@ = 3232, so the start-up code clears nothing |
| 3232-F24E | Origin's uninitialised data (`SRC/BSS.ASM`; zeros in the file) |
| F24E-F378 | the library's uninitialised data |

### Modules (link order)

As above, plus: **bold** frame = an overlaid module. Its frame is its stub segment, the
offset is in the overlay, and the last column gives the overlay's place in the `FBOV`
block (after the block's 16-byte header) and the number of stub entries. Overlaid modules
are linked after use.c, where their data are. Their stubs come after all the code anyway.

| Frame:at | Length | Source (`SRC/`) | DS | What |
|---|---|---|---|---|
| 02CD:D | 1009 | `main.c` | 00A8-028C | start-up: CONFIG.MD and the command line, memory, mouse, graphics driver, palette, font, tile tables, the saved game, then GameLoop; back to MARTIAN.EXE when there is no character |
| 03CE:6 | AC8 | `barge.c` | 028C-042A | the barges on the canals (new); a debugging dump to the file "brg" |
| 047A:E | 356F | `objcmd.c` | 042A-06BA | Look, Get, Drop, Move and their helpers (SE objcmd.c's first part, reworked) |
| 07D1:D | 6456 | `combat.c` | 06BA-07FA | combat: derived attributes, missiles, sprayers, damage, death, bleeding, the combat A.I. modes (the largest module) |
| 0000:242 | 421 | `OSILIB/OSI_FILE.ASM` | 07FA-0804 | OSI file routines (OSI_open ... OSI_memory; SE's); linked here, its code joins C0M's `_TEXT` at image 0242 |
| 0E17:4 | 38 | `LIB/PTRNORM.ASM` |  | normalise a far pointer |
| 0E1A:C | 1EAF | `gfx.c` | 0804-0BB2 | graphics driver set-up, missiles, explosions, sprays, screen shake, sky bar, portraits, conversation loading, paper doll, peer map |
| 1005:C | 62 | `OSILIB/KBD.ASM` |  | keyboard: KBD_GetKey (SE's) |
| 100B:E | 429 | `objtype.c` | 0BB2-0E96 | object type tables and their lookups (new as a module: SE had them in four modules) |
| 104E:7 | 1093 | `canmove.c` | 0E96-0EA0 | can an NPC stand on or step to a square: movement classes, pushing, belts (new) |
| 1157:A | B6F | `visible.c` | 0EA0-0F10 | what is visible and lit: the flood fill of the 11x11 view |
| 120E:9 | 3257 | `gameloop.c` | 0F10-132E | GameLoop (keys to commands, debug keys), PassTime (clock, air, cold, lights, plants), terrain damage, map window refresh |
| 1534:0 | 1159 | `map.c` | 132E-13F0 | the "chunks" and "map" files, the 40x40 area, party enter/exit/teleport, entrances, waking after the party's death |
| - | 0 | `MSGS.ASM` |  | data only: the far messages (far segment 2BB4); empty code segment at 1649:A |
| - | 0 | `MONSDATA.ASM` |  | data only: the monster tables (far segment 2DE5); empty code segment at 1649:A |
| 1649:A | 8DF | `egg.c` | 13F0-1412 | creature eggs (type 0x1d2) and EGG_generate |
| 16D7:A | 358 | `OSILIB/MOUSE.ASM` | 1412-1420 | mouse pointer: show, hide, draw, save the background (SE's) |
| 170D:2 | 3FC0 | `npctrack.c` | 1420-14F0 | NPC movement, schedules, path finding, the party following, NPC_TakeTurn |
| 1B09:2 | 4A00 | `object.c` | 14F0-1672 | object lists and iterators, creating/moving/deleting objects, the objblk files, the 11x11 display stacks |
| - | 0 | `gamestat.c` | 1672-16DA | data only: the game state block saved with the game (time, date, position, talents, dream state); empty code segment at 1FA9:2 |
| 1FA9:2 | CE1 | `sound.c` | 16DA-1A8A | the game's side of music and sound: songs of music.lzc, area songs, 56 effects, machine loops; the party knocked out |
| 2077:3 | 1B55 | `status.c` | 1A8A-1B84 | status panel: party list, portrait, equipment and inventory, Ready/Unready |
| 222C:8 | B8 | `typetest.c` |  | four range tests of a tile or type number (new) |
| 2238:0 | 35F2 | `console.c` | 1B84-1CBA | text windows, keyboard and mouse input, command codes, game load/save |
| 2597:2 | A6C | `use.c` | 1CBA-1CD6 | the Use command; its far tables UseObjects (2E08) and UseActions (2E67) (new) |
| **2F52** (ovl) | 4AD | `belt.c` |  | conveyor belts carrying coal (new) *Overlay at FBOV+00000, 2 stub entries.* |
| **2F55** (ovl) | 232 | `revive.c` |  | SPELL_ReviveNPC, all that is left of SE's spell.c *Overlay at FBOV+00500, 1 stub entries.* |
| **2F58** (ovl) | 552 | `bridge.c` | 1CD6-1DA4 | drawbridges over the canals (new) *Overlay at FBOV+00750, 3 stub entries.* |
| **2F5B** (ovl) | 8F | `disk.c` | 1DA4-1E48 | what is left of the floppy module: DISK_init, DISK_full *Overlay at FBOV+00CF0, 4 stub entries.* |
| **2F5F** (ovl) | C13 | `dream.c` | 1E48-1E88 | the dream: going in, the triggers, waking up (new) *Overlay at FBOV+00D90, 5 stub entries.* |
| **2F63** (ovl) | D00 | `dreamobj.c` | 1E88-1F02 | the dream: the dreamer passing out, chains of objects 0x85, a creature's path (new) *Overlay at FBOV+01A60, 8 stub entries.* |
| **2F68** (ovl) | 1A87 | `dreamstg.c` | 1F02-1F28 | the dream's stages, set up and cleaned up (new) *Overlay at FBOV+02840, 12 stub entries.* |
| **2F6E** (ovl) | 60F | `lever.c` | 1F28-20F4 | UseLever (use action 24): doors, drawbridges, power station, canal pumps, the dream's levers (new) *Overlay at FBOV+04420, 4 stub entries.* |
| **2F72** (ovl) | 402 | `power.c` |  | machinery power on and off; lamps lit at night (new) *Overlay at FBOV+04A90, 3 stub entries.* |
| **2F75** (ovl) | 16F2 | `cutscene.c` | 20F4-213E | the cut scenes (MIDGAME1-D.LZC), the maps of MDMAP.LZC, the end of the game (new) *Overlay at FBOV+04EA0, 16 stub entries.* |
| **2F7C** (ovl) | B3D | `rest.c` | 213E-2408 | the Rest command and the obelisk dream that raises a level *Overlay at FBOV+06700, 4 stub entries.* |
| **2F80** (ovl) | 268 | `jet.c` | 2408-247A | jetters (type 0x1d0) (new) *Overlay at FBOV+072F0, 2 stub entries.* |
| **2F83** (ovl) | 1700 | `talk.c` | 247A-2802 | TALK_talkTo, TALK_initTalk, join/leave party, TALK_Event (the story's events) *Overlay at FBOV+07570, 9 stub entries.* |
| **2F88** (ovl) | 1F6B | `talkdr.c` | 2802-28F0 | the conversation script interpreter (TalkDriver) *Overlay at FBOV+08DE0, 8 stub entries.* |
| **2F8D** (ovl) | 200B | `useact1.c` | 28F0-2990 | use actions: bed, reading, switches, panels, rails, cable, crates, pods, dirt and digging, guns, radium (new) *Overlay at FBOV+0AE00, 26 stub entries.* |
| **2F98** (ovl) | 201A | `useact2.c` | 2990-29A2 | use actions: billiards, organ, rollers, cannon, cabinet panels, spray guns, seeds, ruby slippers, dreamstuff, oxygenated air (new) *Overlay at FBOV+0D080, 22 stub entries.* |
| **2FA1** (ovl) | 1E99 | `useact3.c` | 29A2-29F4 | use actions: lights, switch, drill, telescope, buckets, chemicals, oil, lenses, berries, doors, chests, instruments (new) *Overlay at FBOV+0F340, 24 stub entries.* |
| - | 0 | `OSILIB/drvnames.c` | 29F4-2A28 | data only: the graphics drivers' file names (CGA/EGA/T1K/HER/MCG.DRV); empty code segment at 263D:E |
| 263D:E | C1 | `OSILIB/DRVLOAD.ASM` | 2A28-2A6C | loading the graphics driver; its entry point and parameter block (new in MD) |
| 264A:0 | 1DF | `OSILIB/CLIP.ASM` | 2A6C-2A7A | the driver's mode call, line clipping (new in MD; nothing calls it directly) |
| 2668:0 | 1A | `OSILIB/VIDRESET.ASM` | 2A7A-2A7C | back to the start-up video mode (new in MD) |
| 2674:2 | 530 | `OSILIB/LOW.ASM` | 2A7C-2B12 | critical error and Ctrl-Break handlers, computer speed, memory, loading a file whole (SE LOW.ASM's third segment) |
| 26C7:2 | 78 | `OSILIB/DELAY.ASM` | 2B12-2B1A | OSI_delay_init / OSI_delay_clean (SE's) |
| 26CE:A | 52 | `OSILIB/RAND.ASM` | 2B1A-2B1E | OSI_rand, OSI_randomize (SE's) |
| 26D3:C | 1BF | `LIB/PIT.ASM` | 2B1E-2B2C | the timer chip: rate, interrupt vectors, a delay |
| 26EF:C | 24 | `LIB/LINPTR.ASM` |  | linear address to far pointer |
| 26F2:0 | 22 | `LIB/PTRLIN.ASM` |  | far pointer to linear address |
| 26F4:2 | 11E | `LIB/CALIB.ASM` | 2B2C-2B4D | a delay loop calibrated on the timer |
| 2706:0 | AF | `LIB/MEMMOVE.ASM` |  | copy between overlapping far buffers |
| 2710:F | 1C1F | `music.c` | 2B4E-2E40 | MIDI music and sound effects through STRAX.DRV (MARTIAN.EXE's module, Turbo C++ 1.01) |
| 28D2:E | 11F | `LIB/emsmerge.c` |  | EMS: merge free blocks of the EMS heap (not in SE) |
| 28E4:E | 127 | `LIB/EMSINIT.ASM` | 2E40-2E5E | EMS: find the manager, take all free pages |
| 28F7:6 | 10B | `LIB/EMSMAP.ASM` | 2E5E-2E62 | EMS: map a virtual pointer; save/restore the page map |
| 2908:2 | 164 | `LIB/EMSFREE.ASM` |  | EMS: free space in the EMS heap |
| 291E:6 | 5D | `LIB/EMSNEXT.ASM` |  | EMS: the next block of the heap (not in SE) |
| 2924:3 | CC6 | `LIB/HEAP.C` | 2E62-2E76 | the conventional heap (1.01) |
| 29F0:9 | 13C | `LIB/HEAPFREE.C` |  | free space in the conventional heap |
| 2A04:5 | 297 | `LIB/MEMORY.C` |  | MEM_alloc, MEM_free, MEM_size over both heaps (1.01) |
| 2A2D:C | 290 | `LIB/LZW.ASM` | 2E76-2EA2 | LZW decompression |
| 2A56:C | 4C | `LIB/OSIOPEN.ASM` | 2EA2-2EA8 | open/close a file, the file error handler |
| 2A5B:8 | 2B | `LIB/OSICREAT.ASM` |  | create a file (new in MD) |
| 2A5E:4 | F0 | `LIB/OSIREAD.ASM` | 2EA8-2EAC | read a file, into EMS too |
| 2669:A | A8 | `LIB/OSIWRITE.ASM` |  | write a file, from EMS too (linked after OSIREAD; its code sits after VIDRESET's) |
| 2A6D:4 | 38 | `LIB/OSISEEK.ASM` |  | move a file's pointer |
| 2A70:C | 31D | `LIB/LZC.C` | 2EAC-2EB8 | .LZC libraries: decompress an LZW entry (1.01) |
| 2AA2:9 | 84 | `LIB/LIBSIZE.C` |  | the size of a library entry (MD's entry types; 1.01) |
| 2AAA:D | 160 | `LIB/LZCTYPE.C` | 2EB8-2EC0 | load a library entry by its type (MD's variant of LZC_load; 1.01) |
| 2AC0:D | EE | `LIB/LZCLOAD.C` |  | load a library entry (SE's LZC_load; 1.01) |
| 2ACF:B | 182 | `LIB/LIBOPEN.C` |  | open a library entry (1.01) |
| 2AE7:D | A7 | `LIB/LIBREAD.C` |  | read from a library entry |
| 2AF2:4 | E8 | `LIB/LIBSEEK.C` |  | seek in a library entry (1.01) |
| 2B00:C | 212 | `LIB/EMSALLOC.ASM` |  | EMS: allocate and free a block of the EMS heap |

`SRC/BSS.ASM` (Origin's uninitialised data) and `SRC/SEGORDER.ASM` (the order of C0M's
segments, linked first) have no code and are not rows. Every module with code (73) has
an identical source, checked by `tools/md/mdcheck.py --target game`. The four data-only
rows are checked by the link.

## Data files

The files the sources open, by name as the code spells it. "Entry n" is an entry of an
Origin library file (.LZC: a table of offsets, entries LZW-compressed or stored).

### GAME.EXE

| File | Read by | What |
|---|---|---|
| `config.md` | main.c | the configuration (graphics/music adapter letters), with the command line |
| `system.lzc` | main.c, gfx.c | the graphics drivers (an entry by adapter) and the font (entry 3) |
| `CGA.DRV`, `EGA.DRV`, `T1K.DRV`, `HER.DRV`, `MCG.DRV` | OSILIB/DRVLOAD.ASM | graphics driver names (`OSILIB/drvnames.c`) |
| `mdpal` | main.c | the VGA palette |
| `mdcursor.ptr` | main.c | the mouse pointers |
| `mdscreen.lzc` | main.c, gfx.c | the main screen (entry 0), the status area background (1), the sky bar strip and the command marks (2) |
| `animdata`, `tileflag` | main.c | tile animation data; tile and type property tables |
| `basetile`, `schedule` | console.c | each type's first tile; the NPC schedules |
| `savegame\objlist` | console.c | the saved game (NPCs, party, game state) |
| `savegame\objblkXY` (`.tmp`, `.bak`) | object.c | the objects of each map area; `.tmp` copies for areas left behind, renamed over the real files on saving |
| `chunks`, `map` | map.c | 8x8-tile chunks; the map as 12-bit chunk indexes (surface in superchunks, the other levels whole) |
| `look.lzc` | object.c | the names of the tiles and objects |
| `talk.lzc` | gfx.c | the conversation scripts, one entry per face/conversation number |
| `mdfaces.lzc`, `mdback.lzc` | gfx.c | the portraits and their backgrounds |
| `mdream.lzc` | gfx.c | the dream pictures shown in the status area |
| `mdinv.lzc` | gfx.c | the paper doll pictures, by the character's type |
| `mdtext.lzc` | useact1.c | the books and notes read with Use (entry = quality - 1) |
| `MIDGAME1.LZC` ... `MIDGAMED.LZC` | cutscene.c | the 13 cut scenes: five entries per page (three texts, two pictures) |
| `MDMAP.LZC` | cutscene.c | maps shown by the map object 0x96 (entry = quality) |
| `music.lzc` | sound.c | the 16 songs |
| `md.tim`, `strax.drv` | sound.c, music.c | timbres; the sound card drivers (an entry per card) |
| `brg` | barge.c | written: the barge debugging dump |
| `martian.exe` | main.c, cutscene.c | run when there is no character, and as `martian.exe !` at the end of the game |

`MAPTILES.VGA`, `OBJTILES.VGA`, `MASKTYPE.VGA`, `TILEINDX.VGA`, `LZMAP`, `LZOBJBLK` and
`LZDNGBLK` are in the game's directory but no source of either program names them.

### MARTIAN.EXE

| File | What |
|---|---|
| `config.md` | the configuration (read at start, with the command line) |
| `fonts.lzc` | the fonts |
| `TM.DRV`, `SUPERTM.DRV` | MARTIAN.EXE's graphics drivers (LIB/tmdrv.c, LIB/supertm.c) |
| `md.tim`, `strax.drv` | sound: timbres, sound card drivers |
| `strx_mus.lzc`, `mdd_mus.lzc` | music (12 pieces in `mdd_mus.lzc`) |
| `md_title.pal`, `strax.pal` | palettes |
| `TITLE.LZC` | the title (title.c) |
| `mdmenu.lzc` | the menu: introduction, create a character, play (`game.exe`), credits |
| `logos.lzc`, `fair.lzc`, `credits.lzc`, `lowell.lzc`, `fuse.lzc`, `flash.lzc`, `flight.lzc`, `cabin.lzc`, `mars.lzc` | the opening (Intro: logos, the World's Fair, Lowell, the flight, Mars, with the credits) |
| `scene1.lzc`, `scene2.lzc`, `scene3.lzc`, `scene4a.lzc`, `scene4b.lzc`, `moongate.lzc` | the introduction (PlayIntro) |
| `scenetxt.lzc` | the texts of the introduction (entries 0-2), the endgame (3) and character creation (5) |
| `create.lzc` | character creation's pictures |
| `endgame.lzc`, `moongate.lzc` | the endgame |
| `savegame\objlist` | read by the endgame (the avatar's shape); read and written by character creation: 1CF1h bytes into objList (main.c), then the 62h-byte game state block (`OBJHDR.ASM`) |
| `game.exe` | run to play |
