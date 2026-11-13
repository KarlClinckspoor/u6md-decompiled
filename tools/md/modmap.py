#!/usr/bin/env python3
"""Split Origin's code (after the run-time library) into modules and list each one.

    python tools/md/modmap.py [--target martian|game] [--write]

MARTIAN.EXE: one module per frame (a TLINK segment's paragraph), starting at the first
instruction after the previous module's last return within the frame's first 16 bytes
(the Savage Empire method).
GAME.EXE (overlaid): its segment table lists every code segment with its frame and its
first and last offsets, so the resident modules are read from it; the overlaid modules
follow, one per overlay stub. --write writes <SRC>/modules.txt with no sources yet.
"""
import argparse, os, re, struct, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import target, segmap  # noqa: E402
from capstone import Cs, CS_ARCH_X86, CS_MODE_16

md = Cs(CS_ARCH_X86, CS_MODE_16)
RET = {0xC3, 0xCB, 0xCF, 0xC2, 0xCA, 0xE9, 0xEB, 0xEA}
PROLOGUE = bytes([0x55, 0x8B, 0xEC])

# where Origin's code starts (the end of the run-time library) and ends (MARTIAN.EXE: the
# 8087 emulator's segments, EMU_PROG/E87_PROG, follow it; GAME.EXE: the overlay manager)
LIB_END = {'martian': 0x4FE4, 'game': 0x2CDD}
CODE_END = {'martian': 0x14550, 'game': 0x2B210}


def modules(img, relocs, dgroup, lib_end):
    ent = segmap.entries(img, relocs, dgroup)
    frames = sorted(f for f in ent if f * 16 >= lib_end - 15)
    starts = []
    prev_start = lib_end
    for fr in frames:
        lo = fr * 16
        if lo < prev_start:
            lo = prev_start
        # linear sweep of the previous module up to here: instruction boundaries
        st = None
        last_end_ret = None
        for ins in md.disasm(img[prev_start:fr * 16 + 16], prev_start):
            a = ins.address
            if a >= lo and a < fr * 16 + 16 and last_end_ret == a:
                st = a
                break
            if ins.bytes[0] in RET:
                last_end_ret = a + ins.size
        if st is None:
            st = max(lo, fr * 16)
        first = min(ent[fr]) if ent[fr] else None
        if first is not None and first < 16 and fr * 16 + first != st:
            st = fr * 16 + first   # a far entry in the first paragraph is the module's start
        starts.append((fr, st))
        prev_start = st
    return starts, ent


def module_list(t=None):
    """[(frame, first byte, end, far entries)] of Origin's modules in link order. An
    overlaid program's resident modules come from its segment table (each code segment's
    frame, min and max offset), its overlaid ones follow as (stub, OVL_BASE + file offset,
    end, entries): their code is in the overlay, addressed as target.load_image() does."""
    t = t or target.T
    img, relocs, _ = target.load_image(t['exe_path'])
    lib_end, code_end = LIB_END[t['name']], CODE_END[t['name']]
    tab = target.segment_table(t['exe_path'])
    if tab:
        ent = segmap.entries(img, relocs, t['dgroup'])
        out = []
        for seg, mx, fl, mn in tab:
            if fl == 1 and mx > mn and seg * 16 + mx > lib_end and seg * 16 + mn < code_end:
                st = max(seg * 16 + mn, lib_end)
                out.append((seg, st, seg * 16 + mx, sorted(ent.get(seg, ()))))
        for o in target.overlays(t['exe_path']):
            out.append((o['stub'], o['addr'], o['addr'] + o['size'], sorted(set(o['entries']))))
        return out
    starts, ent = modules(img, relocs, t['dgroup'], lib_end)
    starts = [(fr, st) for fr, st in starts if st < code_end]
    ends = [s for _, s in starts[1:]] + [code_end]
    return [(fr, st, en, sorted(ent[fr])) for (fr, st), en in zip(starts, ends)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--write', action='store_true', help='write <SRC>/modules.txt (sources "-")')
    a = ap.parse_args()
    t = target.T
    img, relocs, _ = target.load_image(t['exe_path'])
    rows = []
    for fr, st, en, offs in module_list(t):
        code = img[st:en]
        ovl = st >= target.OVL_BASE
        at = 0 if ovl else st - fr * 16
        prologues = code.count(PROLOGUE)
        print('%s%04X  start +%X  len %5d  far entries %2d  bp-frames %3d   %s' % (
            'O' if ovl else ' ', fr, at, en - st, len(offs), prologues, ' '.join('%04X' % o for o in offs[:10])))
        rows.append('%04X %X %X -%s' % (fr, at, en - st, '  # overlay' if ovl else ''))
    if a.write:
        os.makedirs(t['src'], exist_ok=True)
        p = os.path.join(t['src'], 'modules.txt')
        if os.path.exists(p):
            sys.exit('%s exists: not overwritten' % p)
        head = ['# %s modules after the run-time library, in link order (tools/md/modmap.py).' % t['exe'],
                '# FRAME AT LEN SOURCE [DS-RANGE]   FRAME: the module code segment (paragraph; an overlaid',
                '# module: its stub segment); AT: its first byte in the frame (overlay: in the overlay);',
                '# LEN: bytes up to the next module (hex); SOURCE: the file ("-" while not decompiled).']
        open(p, 'w').write('\n'.join(head + rows) + '\n')
        print('wrote', p)


if __name__ == '__main__':
    main()
