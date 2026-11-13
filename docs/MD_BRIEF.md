# Brief for workers: decompiling Martian Dreams' MARTIAN.EXE

You are decompiling part of Martian Dreams' `MARTIAN.EXE` (Origin, 1991, Turbo C++ 1.00)
into C or assembly that compiles back to the same bytes. Repository (Windows):
`C:\Users\karl9\Downloads\Claude\u6md-decompiled`. Run tools from the repository root with
`.venv/Scripts/python` (the Bash tool is Git Bash; PowerShell also works).

Read first, in this order:
1. `SRC_M/DECOMP.md`: the layout of MARTIAN.EXE, the tools, the rules of the sources.
2. `docs/GAME_NOTES.md`: Turbo C++ 1.00's habits, learned on Savage Empire (S1-S25) and here
   (M1-). They save hours: register choice, -y, how conditions compile, data layout.
3. The Savage Empire decompilation next door, `..\u6se-decompiled\` (finished, byte-identical):
   `SRC_S/` (SAVAGE.EXE: main.c, title.c, events.asm, music.c, OSILIB/, LIB/) is the
   closest relative of MARTIAN.EXE; `SRC/` (SE's GAME.EXE) has more examples.

Martian Dreams runs on the Savage Empire engine. Most library modules are SE's with small
changes; `build/port/<file>` is the closest SE source with its addresses already moved to
MD's (tools/md/mdport.py), and `build/portall.txt` says how far it matches as it is.
`docs/match_martian.txt` pairs every MD function with the closest SE function (address and
similarity); use it to find the SE code to start from.

## Tools (all with --target martian, the default)

- `tools/md/mddis.py FRAME [END_FRAME]`: annotated disassembly (END is a paragraph).
- `tools/md/mdcmp.py FILE --seg FRAME --at AT --include SRC_M [--include SRC_M/OSILIB] [--data-range START END]`:
  compiles the file in DOSBox, compares the whole module with the original at FRAME:AT,
  prints the first real difference with both disassemblies and every fixup problem
  (names that resolve to the wrong address). Done for a module = 0 bytes differ, 0 problems,
  `--data-range` matching.
- `tools/md/mdfunc.py FILE --include SRC_M [NAME ...]`: compares function by function at the
  addresses the source gives (for partial files, functions out of order). Treats a far call
  and `push cs / call near` as equal, ignores names: finish with mdcmp.
- `tools/md/mdport.py SE_FILE --seg FRAME --at AT -o OUT`: an SE source with its address
  names moved to MD's (where the comparison can tell).
- `tools/md/mdcheck.py [FRAME]`: runs mdcmp on modules listed in SRC_M/modules.txt.

A compile takes about 3 seconds. A file names its flags on its first line:
`/* flags: -mm -c -O -y */` (default `-mm -c -O -Z -d`).

## Method that worked

Get the module compiling with every function in place first (even wrong), then fix
functions in order from the first difference mdcmp reports, until the code is identical.
Then make the data identical with `--data-range` (initialised variables in DS order, then
string literals in order of use; uninitialised data are only declared `extern`, with type
and size in a comment, never defined). Keep SE's names where the function does the same
job, `/*C_SSSS_OOOO*/` before every function and `/*XXXX*/` before every variable you
define; refer to other modules' things by address name (`C_SSSS_OOOO`, `D_XXXX`) or by the
name their source gives. Short factual comments: what the module holds, what each function
does (a file from SE says so in its header and keeps SE's comments only where still true).
When a module's real boundary differs from `SRC_M/modules.txt` (its first byte or length),
work with the real one and report it.

## Constraints

Several workers share this checkout. Create or edit only your assigned files under
`SRC_M/` and scratch files under `build/<your worker name>/`. Do not edit `tools/`,
`SRC_M/modules.txt`, `docs/`, other workers' files, or the SE checkout; run no git command
that changes anything. If a tool bug blocks you, work around it in your scratch folder and
describe the bug and fix in your report.

Done = mdcmp reports the code identical, no fixup problems, and the data range matching,
for every assigned module. If after a long honest effort a few functions still differ,
stop and report them precisely (address, what differs, what you tried).

## Report (short)

For each file: frame/AT/length (say if it differs from modules.txt), status, the `_DATA`
range (DS START-END), flags used, functions you named (address = name), what you learned
about the compiler or the tools that other workers should know (one or two lines each,
for docs/GAME_NOTES.md), and any tool bug.

## Round 2 (resumed 2026-11-13)

Some files under `SRC_M/` were started in round 1 and are part-way: start from them
(and from `build/<old worker>/` scratch where your assignment says so), not from zero.
A module's first byte in `modules.txt` can be one or two bytes off when it starts with
data or a code-segment variable (docs/GAME_NOTES.md M4): when the original's first bytes
are not code, try `--at` one or two lower and report the boundary.

The main module (04FE, 7B8D bytes, 25 functions) is split among four workers. Each writes
a partial file `build/<name>/main_<name>.c` holding only its functions (at their real
addresses, in address order) plus a scaffolding section at its top that `main.c` will
replace (types, `extern` declarations). Check it with `mdfunc.py` (function by function).
Write down, as a comment before each function, the DS addresses of the string literals and
variables it uses (`/* strings: 0123 "...", ... */`), so the parts can be merged into one
`SRC_M/main.c` with its `_DATA` in order. The whole module needs one set of flags; round 1's
draft (build/main-b/main_b.c) used `-mm -c -O -G -y`: check it fits your functions and
report the flags yours need.
