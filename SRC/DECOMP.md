# GAME.EXE decompiled

`GAME.EXE` is the game itself, started by `MARTIAN.EXE`. This folder holds its
decompilation: C and assembly that Turbo C++ 1.00/1.01, MASM 3.0 and TLINK 3.0 build back
into the same `GAME.EXE`. The method and the tools are MARTIAN.EXE's (`SRC_M/DECOMP.md`,
`tools/md/`, all with `--target game`), which come from Savage Empire's
(`..\u6se-decompiled`, whose `SRC/` is SE's GAME.EXE: the closest relative of this one).

## Layout of the executable

| Image | What |
|---|---|
| 0000-0242 | C0M.OBJ (Turbo C++ 1.00, identical) |
| 0242-0663 | OSILIB/OSI_FILE.ASM (linked after combat.c: its code joins C0M's `_TEXT`) |
| 0663-2CDD | OVERLAY.LIB and CM.LIB modules (1.00's) |
| 2CDD-2B21E | Origin's resident modules (`modules.txt`) |
| 2B21E-2BB37 | OVERLAY.LIB's overlay manager (`_OVRTEXT_`) |
| 2BB40-2F0FC | far data (class `FAR_DATA`, each segment paragraph aligned): MSGS.ASM (2BB4), MONSDATA.ASM (2DE5), use.c's UseObjects (2E08) and UseActions (2E67) |
| 2F100-2F520 | the overlay manager's data and the segment table (`_EXEINFO_`), the start-up tables |
| 2F520-2FAB0 | the 17 overlay stubs (2F52-2FA1) |
| 2FAB0- | DGROUP (2FABh), then the stack (3EE3:0080) |
| after the MZ image | the `FBOV` block: 17 overlays (VROOMM), each with its fixups |

Overlaid modules are listed in `modules.txt` by their stub segment; their code is at
100000h + its file offset in `target.load_image()`, and their functions are named
`C_<stub>_<offset in the overlay>`. The program is compiled `-mm -p` (pascal) by default;
the library's C files name their own flags (cdecl) and often Turbo C++ 1.01.

The link is `TLINK /m /i SEGORDER C0M <modules> BSS, ULTIMA, ULTIMA, OVERLAY CM`, the
overlaid modules between `/o` and `/o-`: they sit in the link after use.c, where their
data are (DS 1CD6-29F4). TLINK ignores case.

DGROUP: C0M's data 0000-00A8; Origin's initialised data 00A8-2EC0; the library's to
3232; C0M's `_BSSEND` (edata@) at 3232, before `_BSS` (SEGORDER.ASM, as in Savage
Empire's GAME.EXE: the start-up code clears nothing); Origin's uninitialised data
3232-F24E (`BSS.ASM`, `tools/md/mkbss.py --target game --start 3232 --end F24E`); the
library's F24E-F378.

## Status

**Done (2026-11-15): `python tools/md/mdbuild.py --target game` builds GAME.EXE
identical to the original** (341712 bytes: header, 3202 relocations, image and FBOV
block; sha1 6b6b91fd6291d6d4d7b91615e5b6333c3e2b68a8).

`python tools/md/mdcheck.py --target game` compares every module that has code (73, all
identical; the data-only rows, gamestat.c, OSILIB/drvnames.c, MSGS.ASM and MONSDATA.ASM,
are checked by the link). Before a build, after renaming or adding names: `tools/md/mkbss.py
--target game --start 3232 --end F24E` (BSS.ASM) and `tools/md/mdsym.py --target game
apply`.

- Carried over unchanged but for addresses (`tools/md/mdport.py --target game`):
  from SRC_M (MARTIAN.EXE): music.c and the whole library from MEMMOVE to EMSALLOC
  (2706-2B00), CALIB, LINPTR, PTRLIN, PTRNORM, PIT; from SE's GAME.EXE: OSILIB/KBD,
  MOUSE, DELAY, RAND, OSI_FILE.
- Done by workers: OSILIB (DRVLOAD, VIDRESET, LOW, CLIP, OSIWRITE, OSICREAT,
  drvnames.c), main.c, console.c, talkdr.c (overlay 2F88, the first overlay done),
  talk.c (overlay 2F83), status.c (2077), objtype.c (100B), typetest.c (222C),
  revive.c (overlay 2F55), disk.c (overlay 2F5B), map.c (1534), egg.c (1649), jet.c
  (overlay 2F80), visible.c (1157), use.c (2597), power.c (overlay 2F72), barge.c (03CE),
  belt.c (overlay 2F52), bridge.c (overlay 2F58), sound.c (1FA9, SE's game-side music.c),
  lever.c (overlay 2F6E), rest.c (overlay 2F7C),
  gfx.c (0E1A), canmove.c (104E), dream.c (overlay 2F5F), dreamobj.c (overlay 2F63),
  dreamstg.c (overlay 2F68),
  cutscene.c (overlay 2F75), gameloop.c (120E), npctrack.c (170D), object.c (1B09),
  gamestat.c (data only, DS 1672-16DA), objcmd.c (047A), combat.c (07D1, the largest
  module; SE's combat.c reworked, DS 06BA-07FA; docs/GAME_NOTES.md M30, M31),
  useact1.c (overlay 2F8D, the first of the "use" actions, new code; M32), useact2.c
  (overlay 2F98, the second of them, new code; M33), useact3.c (overlay 2FA1, the
  third and last of them, new code; M34).
- Every code module of GAME.EXE has an identical source.
  SRC/se.h is SE's.

## Rules of the sources

As MARTIAN.EXE's (`SRC_M/DECOMP.md`) and Savage Empire's GAME.EXE
(`..\u6se-decompiled\SRC\DECOMP.md`): pascal by default, so a C file's own functions get
upper-case link names; library routines are declared cdecl where they are. See
docs/GAME_NOTES.md (S1-S25 from Savage Empire, M1- from Martian Dreams).

## How it was finished (2026-11-15)

The first full link, step by step (docs/GAME_NOTES.md M35):

- Data-only modules: the `_DATA` ranges in `modules.txt` leave no gap, but the segment
  table (`target.segment_table()`) has three more empty code segments, WORD aligned (the
  OSI macros' empty segment: one byte of padding before each): 0E17:4, OSI_FILE.ASM's,
  which is linked after combat.c (its relocations follow combat.c's, its data at 07FA
  too); and two at 1649:A, after map.c: MSGS.ASM and MONSDATA.ASM, the far messages and
  the monster tables (assembly: one paragraph-aligned segment of class `FAR_DATA` each,
  with a public name at every string or table). use.c defines UseObjects and UseActions
  as far variables (Turbo C gives each a paragraph-aligned segment); UseActions' far
  pointers to the overlaid actions are the relocations after use.c's.
- BSS: `bdata@ = edata@ = 3232` and the library's uninitialised data at F24E: SEGORDER.ASM
  (Savage Empire's) and BSS.ASM 3232-F24E.
- Names (`mdsym apply`): three functions other modules call were `static`
  (OBJ_ShowMultiTile, combat.c's __DoHit and __TryHit); gameloop.c's emsInUse is cdecl.
- Overlays: linked after use.c (their data). TLINK makes a stub entry for every public of
  an overlaid module (and for a function a far fixup reaches), in the reverse order of
  its PUBDEFs, which Turbo C writes in the hash order of the names: each overlaid module
  has a `/* stub order: */` line (tools/md/stuborder.py rewrites the object's PUBDEFs in
  mdbuild). talkdr.c's functions and rest.c's REST_FoesNear/REST_OthersNear are public
  (stub entries in the original), dreamobj.c's C_2F63_0726 and useact1.c's
  C_2F8D_03FC/043F static (none).
- OSIWRITE.ASM is linked after OSIREAD.ASM (its relocation), its code after VIDRESET's:
  VIDRESET.ASM names OSIWRITE's segment first.
- The overlay data hold the name and date of the link: GAME.EXE was linked as
  `ultima.exe` on 23 May 1991 (target.py `link_name`, `link_date`; mdbuild sets DOS's date).
