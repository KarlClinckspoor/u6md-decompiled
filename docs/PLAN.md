# Martian Dreams: plan for the decompilation

Goal, as for Ultima VI and The Savage Empire: C and assembly that rebuild Martian Dreams'
executables byte for byte with the original tools, header and relocation tables included.
This plan starts from what this repository already has (the dialogue export, the
MD-to-SE function match in `docs/md_se_match.txt`, a copy of the SE tools in `tools/se/`)
and from the Savage Empire lessons (`SE-WRITEUP.md`, `U6-LESSONS_DECOMPILATION.md`, and the
SE checkout next door, `..\u6se-decompiled`).

## 1. What we know already

| Fact | How it was found |
|---|---|
| Three programs: `MARTIAN.EXE` (launcher, title, intro, character creation, endgame), `GAME.EXE` (the game), `INSTALL.EXE` (set aside, as for SE) | file list |
| All three say "Turbo C++ - Copyright 1990 Borland Intl." | strings |
| **The run-time library is Turbo C++ 1.00's `CM.LIB`, unmodified.** Every library module found in both programs matches 1.00, including `FARHEAP`, `NEARHEAP` and `FINDFIRS`, which in SE were 1.01's appended with `tlib -+`. | `libscan.scan` against 1.00 and 1.01 `CM.LIB` (run 2026-11-13, see 5.1) |
| **`GAME.EXE` uses Borland VROOMM overlays** (`FBOV` after the MZ image, TLINK `/o`). The overlay manager is 1.00's `OVERLAY.LIB` (`OVRMAN` at 2B21E, `OVRUSER` at 2BAE3, `OVRHALT` at 00663). `OVERLAY.LIB` is in TC++ 1.00's `XLIB.ZIP`. | header, `libscan` |
| `GAME.EXE` segment table (at file 325C0): 102 entries, **63 resident code segments, 17 overlaid segments** (66,603 bytes of overlay code), 11 + 11 others (data / stubs, to be decoded). Each entry gives segment and size, so it is very nearly a module map for free. | `FBOV` header parse |
| `GAME.EXE`: DGROUP 2FABh, 3,202 MZ relocations (resident only; overlay fixups live in the overlays), stack segment 3EE3h | header, `mov dx,DGROUP` at entry |
| `GAME.EXE` is pascal like SE's: 322 `retf n` against 246 `retf` in resident game code, 120 against 65 in the overlays. `push cs / call near` is used in both (728 and 123 times), so same-module calls behave as in SE. | byte counts |
| `MARTIAN.EXE`: no overlays, DGROUP 172Ah, 1,200 relocations. Game code sits in two places: 0242-0EAE (right after C0M, before the library) and 4FE4-172A0 (after it). | header, `mdmatch_proto.py` |
| SE's shared library (EMS heap, LZW/LZC, file I/O, MIDI music, timer, CPU calibration: `SRC_S/LIB`) is in both MD programs, mostly at 0.93-1.00 similarity. The OSI assembly (mouse, keyboard, delay, random) is there too. | `docs/md_se_match.txt` |

How close Martian Dreams is to Savage Empire (`docs/md_se_match.txt`, token similarity of
each MD function to its closest SE function):

| Part | Functions | Code | 0.95+ | 0.85-0.95 | 0.5-0.85 | below 0.5 |
|---|---|---|---|---|---|---|
| `GAME.EXE` resident | 559 | 168 KB | 22% | 13% | 33% | 31% |
| `GAME.EXE` overlays | 158 | 67 KB | 9% | 5% | 9% | **77%** |
| `MARTIAN.EXE` | 236 | 78 KB | 21% | 9% | 10% | **60%** |

For comparison SE's GAME.EXE was 195 KB, about 40% of it starting from U6 code. MD is
about 313 KB of code, and roughly a third starts from SE code nearly as is. The resident
segments that are mostly SE: `console.c` (R2238, 75% at 0.85+), SE's music driver (R2710,
80%), `main.c` (R02CD, 67%), the shared library and OSI assembly (R2669-R2B00, 80-100%),
`status.c`, `npctrack.c` and `object.c` (43-49%). The overlays are mostly new code: object
commands and combat that SE kept resident in `objcmd.c` and `combat.c`, apart from one
segment that is SE's `talkdr.c` almost unchanged (O2F88, 99%).

## 2. What is different from Savage Empire

1. **Overlays.** Nothing in the SE tools knows about them. Needed:
   - `load_image` and the comparer must address overlay code (file offset past the MZ
     image, the stub segment that owns it, its own fixup list) as well as resident code.
     `mdmatch_proto.py` already reads the segment table and stub headers; that code moves
     into the shared target module.
   - The compile flags of overlaid programs: in TC++ 1.00 every module of an overlaid
     program is compiled with `-Y`, and the overlaid ones are named at link time with
     `/o+ ... /o-` (or compiled `-Yo`). Whether `-Y` changes the code (prologues, far
     returns, same-segment calls) must be tested on day one, before any C is written.
   - The link: TLINK `/o` must reproduce the `FBOV` block, the segment table, the stubs and
     the order of the overlays. A toy two-overlay program linked both ways will show how
     the stub table is laid out and what decides overlay order.
2. **The library is plain 1.00.** `setup.py`'s `CMSE.LIB` step (1.01 modules appended) is
   not wanted; use 1.00's `CM.LIB` and `OVERLAY.LIB` as shipped, and check the module
   order against the image before the first link.
3. **Module boundaries for GAME.EXE come from the segment table**, not only from the
   disassembly. Each resident or overlaid code segment is one module's `_TEXT` (medium
   model), so the 80 entries are the module list. Check a few against `push bp` starts:
   SE's switch tables pushed starts to :3, :4, :9, and the table gives the true answer.
4. **MARTIAN.EXE has game code before the library.** Some Origin objects or an Origin
   library were linked between C0M and `CM.LIB`. The map has to say which, before the
   link order can be written.

## 3. Phases

### Phase 0: the toolchain on this machine

Nothing is set up here yet: no `tools/tcpp100`, no `capstone` / `rapidfuzz` / `py7zr`, no
`mcopy` on Windows, and the SE checkout has no unpacked tools either.

- `pip install capstone rapidfuzz py7zr`.
- Unpack TC++ 1.00 into `tools/tcpp100/` (`TCC.ZIP`, `BIN1.ZIP`, `INCLUDE.ZIP`, `MLIB.ZIP`,
  `STARTUP.ZIP`, **`XLIB.ZIP`** for `OVERLAY.LIB`). 7-Zip reads the FAT disk images and the
  imploded ZIPs directly, so a Windows `setup.py` can use `7z.exe` instead of `mcopy` and
  Info-ZIP. MASM 3.0 comes out of `tools/masm300/*.img` the same way.
- DOSBox: `gamedata/DOSBOX/DOSBox.exe` (0.74-2) through `SE_DOSBOX`, or install DOSBox-X.
  Confirm a headless `tcc.py` compile works (`SDL_VIDEODRIVER=dummy`).
- Add a `.gitignore` like SE's: `gamedata/*`, `tools/*.7z`, `tools/tcpp10*/`,
  `tools/masm300/`, object files, `build*/`.

Done when `tcc.py` compiles a one-line C file to an `.OBJ` here.

### Phase 1: tools for Martian Dreams

Fork `tools/se/` into `tools/md/` (keep `tools/se/` untouched as the reference) and give it
`target.py` entries for `martian` and `game`: exe, source dir, DGROUP (172Ah, 2FABh), BSS
start, flags (`-mm -c -O -Z -d`, plus `-p` and `-Y` for GAME.EXE, to be confirmed in
phase 2), symbol files. Then, in this order:

1. `target.load_image` returns overlays too; one address space for both
   (`mdmatch_proto.py` uses 0x100000 + overlay offset; keep that).
2. `libscan` with 1.00's `CM.LIB` and `OVERLAY.LIB`: commit its output as
   `docs/MAP.md`'s library section.
3. `modmap.py`: GAME.EXE modules from the segment table, MARTIAN.EXE from the
   disassembly as in SE. Write `SRC/modules.txt` and `SRC_M/modules.txt`.
4. Turn `mdmatch_proto.py` into `mdmatch.py` wired to `target.py`, matching against the
   SE sources by module so that each MD module gets a list of SE functions to start from.
5. `mdcmp.py` (from `secmp.py`): compile one file, compare at its address, overlay or
   resident, fixups checked by name or address. This is the check every worker runs.

### Phase 2: day-one experiments (small, before any module)

Each one answers a question that would otherwise turn up as the last bytes:

1. The first 0242h bytes of each program against 1.00's `C0M.OBJ`.
2. `-Y` on and off on a small function, compared with an MD function compiled both ways.
3. A toy program linked with `/o` into two overlays: the `FBOV` header, the segment
   table, stub layout, overlay order, where overlay fixups go.
4. The SE compiler rules hold: compile one near-identical SE module unchanged at its MD
   address. Candidates: `talkdr.c` as overlay O2F88 (99%), and the shared library's
   `HEAP.C`, `MEMORY.C`, `LIBOPEN.C` in both programs.
5. Whether line numbers (`-y`) are on, module by module, by length as in SE.

Write the answers into `docs/GAME_NOTES.md`, starting it as a copy of SE's (same
compiler, so its register, condition and layout rules apply from the start).

### Phase 3: the shared library and MARTIAN.EXE (calibration run)

As SAVAGE.EXE was for SE: a smaller program without overlays, that exercises the whole
loop including the link.

- (Done differently: `mdport.py` carries each SE file over with MD addresses; see 5.0.)
  Copy SE's `SRC_S/LIB/` to `SRC_M/LIB/` and match each file at its MD addresses
  (`shared.py` pairs the BSS addresses). Most of it should be unchanged; it then serves
  GAME.EXE too.
- Decompile MARTIAN.EXE's modules starting from SE's `SRC_S` where they match (`main.c`,
  `title.c`, music, events), and from nothing where they do not (60% of the code:
  probably the intro, the character creation questions and the endgame, which MD tells
  differently).
- `BSS.ASM`, data-only modules, the full link, `mdbuild.py --target martian` identical.

### Phase 4: GAME.EXE

Parallel workers, one or two modules each, from a written brief (`docs/GAME_BRIEF.md`,
`SRC/DECOMP.md`), each one done when `mdcmp.py` reports identical code and data. Order:

1. Mostly SE, to settle the rules for this program: shared library and OSI assembly
   (from SRC_M/LIB and SE's OSILIB), SE's music driver (R2710), `console.c` (R2238),
   `main.c` (R02CD), `talkdr.c` overlay (O2F88).
2. Recognisable SE: `object.c` (R1B09), `npctrack.c` (R170D), `status.c` (R2077),
   `map.c` (R1534), `gfx.c` (R0E1A), `combat.c` (R07D1), `gameloop.c` (R120E).
3. Mostly new: `objcmd`-like resident code (R03CE, R047A) and the 16 other overlays.
   These are the bulk of the new work (about 100 KB) and need the most reading; the
   dialogue export and `docs/md_se_match.txt` help name things.

Addresses stay on everything (`C_SSSS_OOOO`, `D_XXXX`) until all modules match; overlay
functions need their own form, for example `O_SSSS_OOOO` with the stub segment.

### Phase 5: the link

Everything SE needed (`TLINK /i`, `BSS.ASM`, segment order before C0M, far data as
`-mh -zTFAR_DATA`, data-only modules, module starts that are not :0), plus `/o` and the
overlay order. Done when `mdbuild.py --target game` is identical, `FBOV` block included.

### Phase 6: names and findings

`sesym.py apply` equivalent for names; `docs/FINDINGS.md` on how MD's engine differs
from SE's (as SE's did against U6), `docs/MAP.md`, a writeup.

## 4. Open questions

- ~~Does `-Y` change code generation?~~ No (phase 2): it adds one COMENT record (class
  EAh) that TLINK `/o` checks; `-Yo` gives the same object. `mdbuild.py` adds `-Y` to
  GAME.EXE's modules.
- What are the 11 type-0 and 11 type-4 segment table entries? Partly answered by a toy
  link: type 0 are data segments (far data, `_OVRDATA_`, `_STUB_`, `_EXEINFO_`, DGROUP,
  stack), type 4 the empty `OVRINFO`/`INITDATA` ones. GAME.EXE's 2BB4-2F10 are far data.
- ~~What is MARTIAN.EXE's code before `CM.LIB`?~~ Library: MATHM.LIB and EMU.LIB modules
  (FPINIT ... _MATHERR); the link is `EMU MATHM CM`. In GAME.EXE it is SE's
  `OSI_FILE.ASM` (0242-0663, the same 421h bytes) and OVERLAY.LIB's OVRHALT.
- Which `-y` / `-O` combinations, module by module (SE: 13 of 19 game modules had `-y`).
- Is the shared library compiled from the same source as SE's, or a later version?

## 5. Notes

### 5.0 Progress (2026-11-13)

- **Phase 0 done.** `.venv` (capstone, rapidfuzz, py7zr), `tools/md/setup.py` (7-Zip, no
  mtools), DOSBox from `gamedata/DOSBOX`. A compile takes about 3 seconds.
- **Phase 1 done.** `tools/md/` is the fork: `target.py` (both programs, overlays in one
  address space at 100000h + file offset, `overlays()`, `segment_table()`), `mdcmp.py`
  (overlay stubs resolved to the functions they stand for, capstone instead of ndisasm,
  `--map-out`), `mdport.py` (an SE source with its addresses moved to MD's),
  `mdfunc.py` (function-by-function comparison), `mdmatch.py` (`docs/match_*.txt`),
  `modmap.py` (GAME.EXE from its segment table), `libscan.py`/`seedsyms.py` (EMU, MATHM,
  OVERLAY, CM), `mdbuild.py` (libraries per program, `/o ... /o-` and `-Y` for overlays).
  `SRC_M/modules.txt` (68 modules) and `SRC/modules.txt` (76, 17 of them overlays).
- **Phase 2 done.** C0M identical in both programs; `-Y` changes no code; a toy `/o`
  link reproduces GAME.EXE's layout (stubs of class STUBSEG before DGROUP, overlays in
  link order, stub entries for the module's publics); SE's `title.c` compiles to MD's
  bytes up to `PlayTitle`, SE's `talkdr.c` to overlay 2F88's first function, through
  the stub.
- **Phase 3 done (2026-11-13).** `mdbuild.py --target martian` rebuilds MARTIAN.EXE
  byte for byte (sha1 5e0e7777...). 72 modules: 29 came over from SE with only the
  addresses moved, the rest were decompiled by workers in two rounds; the main module was
  split among four workers and merged (build/merge/merge.py). Four data-only modules
  (OBJHDR.ASM, LIB/drvdata.c, OSILIB/OSIDATA.ASM, OSILIB/MOUSDATA.ASM); BSS.ASM covers
  DS 2366-465C. What it took is in SRC_M/DECOMP.md and docs/GAME_NOTES.md M4-M10: Origin's
  library C files were built with Turbo C++ 1.01 (`/* compiler: Turbo C++ 1.01 */`), the
  link is case-insensitive (main.c's own Delay/Exit renamed), fixup order matters for the
  relocation table.

Run from the scratchpad with `tools/se/libscan.py`'s `scan()` against both TC++ versions'
`CM.LIB` and `OVERLAY.LIB`. Every hit matched 1.00; none matched only 1.01.

```
GAME.EXE     FBRK FCORELFT FNSPLIT SETUPIO WRITE FINDFIRS INT86 INTDOS IOERROR SEARCHP
             VPRINTER EXEC FARHEAP NEARHEAP (08A6-2B4B), OVRHALT 00663, OVRMAN 2B21E,
             OVRUSER 2BAE3
MARTIAN.EXE  ACCESS CALLOC CPRINTF FBRK FCORELFT FNSPLIT FOPEN FREAD PUTC PUTS SCREEN
             SCROLL SETUPIO TIMECVT WRITE FINDFIRS GETC IOERROR SEARCHP TZSET VPRINTER
             EXEC FARHEAP NEARHEAP (0EAE-4E3B)
```

Modules with no long run of fixup-free bytes are not found by the scanner; the full list
comes from the map in phase 1.
