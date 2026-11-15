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
| 0242-0663 | OSILIB/OSI_FILE.ASM (linked after main.c: its code joins C0M's `_TEXT`) |
| 0663-2CDD | OVERLAY.LIB and CM.LIB modules (1.00's) |
| 2CDD-2B210 | Origin's resident modules (`modules.txt`), then the overlay stubs |
| 2FAB0- | DGROUP (2FABh) |
| after the MZ image | the `FBOV` block: 17 overlays (VROOMM), each with its fixups |

Overlaid modules are listed in `modules.txt` by their stub segment; their code is at
100000h + its file offset in `target.load_image()`, and their functions are named
`C_<stub>_<offset in the overlay>`. The program is compiled `-mm -p` (pascal) by default;
the library's C files name their own flags (cdecl) and often Turbo C++ 1.01.

## Status

`python tools/md/mdcheck.py --target game` compares every module that has a source.

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
  useact1.c (overlay 2F8D, the first of the "use" actions, new code; M32).
- Next: the SE-like modules, then the new code and the other overlays (docs/PLAN.md
  Phase 4). SRC/se.h is SE's.

## Rules of the sources

As MARTIAN.EXE's (`SRC_M/DECOMP.md`) and Savage Empire's GAME.EXE
(`..\u6se-decompiled\SRC\DECOMP.md`): pascal by default, so a C file's own functions get
upper-case link names; library routines are declared cdecl where they are. See
docs/GAME_NOTES.md (S1-S25 from Savage Empire, M1- from Martian Dreams).
