# MARTIAN.EXE decompiled

`MARTIAN.EXE` is the program you start: configuration, title, introduction, character
creation, endgame, and it runs `GAME.EXE`. This folder holds its decompilation: C and
assembly that Turbo C++ 1.00, MASM 3.0 and TLINK 3.0 build back into the same
`MARTIAN.EXE`, byte for byte. The method and tools are Savage Empire's (`SAVAGE.EXE`,
checkout `..\u6se-decompiled`, `SRC_S/DECOMP.md`), forked into `tools/md/`.

## Layout of the executable

| Image | What |
|---|---|
| 0000-0242 | C0M.OBJ (Turbo C++ 1.00, identical) |
| 0242-0EAE | MATHM.LIB and EMU.LIB modules in `_TEXT` (FPINIT ... _MATHERR) |
| 0EAE-4FE4 | CM.LIB modules (ACCESS ... SETENVP), 1.00's, unmodified |
| 4FE4-14550 | Origin's modules: `modules.txt` |
| 14550-172A0 | EMU086, EMU087, E87TRAP (the 8087 emulator's own segments) |
| 172A0- | DGROUP (172Ah): C0M's data 0000-00A8, then the modules' `_DATA` in link order |

The link is `TLINK /m /i C0M <modules> BSS, MARTIAN, MARTIAN, EMU MATHM CM`. TLINK
ignores case, as Origin's link did: a public of ours whose name differs from a library
function's only in case takes that function's place (main.c's own `Delay` and `Exit` are
called `Pause` and `ExitGame` for that reason).

DGROUP: Origin's initialised data DS 00A8-1CB8 (then padding to EMU.LIB's paragraph-
aligned FPINIT data at 1CC0, the library's data, `_CVTSEG`/`_SCNSEG` to 2366); Origin's
uninitialised data 2366-465C (`BSS.ASM`, `tools/md/mkbss.py --start 2366 --end 465C`);
the library's 465C-47AA (C0M clears 2366-47AA).

## Status

**Done (2026-11-13): `python tools/md/mdbuild.py --target martian` builds MARTIAN.EXE
identical to the original** (118480 bytes, sha1 5e0e77776cdfe1222254ae4663c9f252a2271780).

`modules.txt` lists Origin's modules in link order: frame, first byte, length, source,
`_DATA` range; `- - 0` rows are modules with data only. `python tools/md/mdcheck.py
--target martian` compares every module with code (72 of 72 identical).

Before a build, after renaming or adding names: `tools/md/mkbss.py --start 2366 --end
465C` (BSS.ASM) and `tools/md/mdsym.py apply` (references by address become the
defining module's names).

## Tools

Windows: use the repository's venv, `.venv/Scripts/python`. All tools take
`--target martian` (the default) or `--target game`.

    python tools/md/setup.py                  unpack Turbo C++ 1.00/1.01 and MASM (once; uses 7-Zip)
    python tools/md/mddis.py SEG [END_SEG]    annotated disassembly from paragraph SEG to END_SEG
    python tools/md/mdcmp.py FILE --seg SEG --at AT [--data-range START END] --include SRC_M
                                              compile one module, compare it with the original
    python tools/md/mdcheck.py [FILE|FRAME]   compare every module in modules.txt
    python tools/md/mdport.py SE_FILE --seg SEG --at AT -o OUT
                                              an SE source, its addresses moved to MD's
    python tools/md/mdmatch.py [--module FRAME]   closest SE function for each MD function
                                              (docs/match_martian.txt has the whole table)
    python tools/md/mdsym.py list|apply       the names given in the sources
    python tools/md/libscan.py, modmap.py, segmap.py, dsrefs.py, dsparts.py   how the map was made

`build/port/` holds every MD module's closest SE source already run through `mdport.py`
(addresses moved where the comparison could tell): the starting point for a module.
`build/portall.txt` says how far each one matches ("identical up to +X of LEN").

## Rules of the sources

Savage Empire's rules (`..\u6se-decompiled\SRC_S\DECOMP.md`, "Rules of the sources")
apply unchanged. In short:

- **Compiler:** Turbo C++ 1.00. A C file names its flags on its first line,
  `/* flags: -mm -c -O */`, otherwise `-mm -c -O -Z -d`. `-y` (line numbers) when
  merged statements show up; library C files often `-mm -c -O -G`; Origin's library
  routines are often `pascal`.
- **Assembly:** `.asm` that MASM 3.0 `/Mx` assembles to the same bytes: `OFFSET DGROUP:x`,
  `FAR PTR`, `SHORT`; `db` for an instruction MASM encodes differently (TASM's `83 /4`).
  Include files sit next to the source (`OSILIB/OSI.INC`).
- **Names:** `/*C_SSSS_OOOO*/` before every function definition (asm: `;SSSS:OOOO` after
  the PROC or PUBLIC), `/*XXXX*/` before every variable definition (asm: `;D_XXXX`).
  Other modules' things by address: `C_SSSS_OOOO`, `D_XXXX`. Give a real name only when
  sure.
- **Initialised data:** a module's variables in DS order, then its string literals in
  order of use. `--data-range START END` checks a module's `_DATA` exactly.
- **Uninitialised data:** only declared `extern`, never defined; `BSS.ASM`
  (`tools/md/mkbss.py`) lays them all out at the end.
- **Comments:** what each module holds, what each function does: short and factual. A
  file carried over from SE says so in its header comment and keeps SE's comments only
  where still true.

## How it was finished (2026-11-13)

- Round 2 (nine workers, docs/MD_BRIEF.md "Round 2"): the main module split in four
  (main-a 0004-177E, main-c 177E Intro, main-d 2E9A/3577 character creation, main-b
  4286/792B intro scenes and credits), merged by `build/merge/merge.py` (head.c holds the
  types, the initialised data 00A8-04DB and the declarations); music.c; title.c,
  events.asm, surface.c, tmdrv.c/supertm.c, palfile.c, emsmerge.c; the OSI leftovers; the
  EMS/heap/memory and LZW/LZC/file library.
- Compiler: Origin's library C files are mostly Turbo C++ 1.01 output (`/* compiler:
  Turbo C++ 1.01 */` on a file's first lines; tcc.py, mdcmp, mdcheck and mdbuild then use
  tools/tcpp101). The OSILIB C files are mixed (FADEIN needs 1.00).
- Data-only modules, placed by where their relocations sit in the .EXE's table:
  `OBJHDR.ASM` (0BC0-0C24, after main.c), `LIB/drvdata.c` (0E6E-14F0) and
  `OSILIB/OSIDATA.ASM` (14F0-1510) after the shape routines, `OSILIB/MOUSDATA.ASM`
  (1916-1940) after MOUSEDRW. sprites.c ends with a zero word (0DFA).
- Relocation order: assembly modules keep their `;TASM fixup order:` lines; LIBOPEN.C has
  `#pragma inline` (Origin's went through the assembler: its fixups are in code order);
  emsmerge.c has one statement in inline asm (no C form gives its word read) and a
  `/* fixup order: ... */` comment that puts MASM's fixups back in Turbo C's order.

## What the work found (2026-11-13)

- The library modules are SE's library, a little later: many are identical (KEYB, PIT,
  CALIB, EMSINIT, EMSFREE, HEAPFREE, LIBREAD, the OSI shape/text/mouse assembly), others
  changed in a function or two. Some sit in a different order from SAVAGE.EXE's, and LZC
  is split over three code segments (13A3, 13DD, 13F3).
- Module boundaries from `modmap.py` are guesses where a module ends in data or a switch
  table: `sprites.c` is 156h bytes (so `LIB/CALIBDRV.ASM` starts at 0DD4:6); `LZW.ASM`
  begins with six bytes of data. Report a boundary you find to differ.
- `title.c` and `sprites.c` are SE's: title.c is identical up to `PlayTitle` (+0C6D in the
  module).
