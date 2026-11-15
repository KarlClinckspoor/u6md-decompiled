#!/usr/bin/env python3
"""Annotated disassembly of a part of the executable, for decompiling it.

    python3 tools/md/mddis.py [--target martian|game] SEG [END_SEG] > listing.txt     (hex paragraphs)

Offsets are relative to SEG (the module's code segment, as its far calls see it). SEG can
be an overlay's stub segment (GAME.EXE): the whole overlay is listed, offsets in it.
Annotations:
  - far calls and far pointers: the target's name from the target's symbol file, else
    C_SSSS_OOOO (the convention of the GAME.EXE decompilation);
  - "push cs / call near X": a far call into the same segment, named C_SEG_X;
  - switch tables (jmp [cs:bx+T]) are listed as data, with their cases;
  - immediates and memory operands that are DS offsets of strings: the string;
  - relocated words (segment values): marked SEG.
"""
import os, re, struct, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from mdcmp import find_exe, load_image, load_syms, DGROUP, STUBS, resolve_far  # noqa: E402


def strings_at(img):
    ds = DGROUP * 16
    data = img[ds:]
    out = {}
    for mm in re.finditer(rb'[\x20-\x7e\r\n\t%\x80-\xff]{2,}\x00', data):
        s = mm.group(0)[:-1]
        if sum(32 <= c < 127 for c in s) >= len(s) * 0.8:
            for k in range(len(s)):
                if k == 0:
                    out[mm.start()] = s.decode('latin-1')
    return out


def main():
    seg = int(sys.argv[1], 16)
    end = int(sys.argv[2], 16) if len(sys.argv) > 2 else None
    img, relocs = load_image(find_exe())
    syms, dsyms = load_syms()
    try:
        import mdsym
        for k, (name, fn, lang, static) in mdsym.table().items():
            if k[0] == 'C':
                syms[name] = (k[1], k[2])
            else:
                dsyms[name] = k[1]
    except Exception:  # noqa: BLE001
        pass
    names = {v: k for k, v in syms.items()}
    dnames = {v: k for k, v in dsyms.items()}
    if end is None:
        # the next segment value any relocation holds
        segvals = sorted({struct.unpack_from('<H', img, r)[0] for r in relocs})
        end = min([s for s in segvals if s > seg] or [DGROUP])
    base, length = seg * 16, (end - seg) * 16
    if seg in STUBS:  # an overlay: its code is at OVL_BASE + its file offset
        base, length = STUBS[seg]['addr'], STUBS[seg]['size']
    code = img[base:base + length]
    strs = strings_at(img)

    # switch tables: 2E FF A7 tt tt   jmp [cs:bx+tt]; count from a preceding cmp
    skips = []
    tables = {}
    for mm in re.finditer(rb'\x2e\xff\xa7(..)', code, re.S):
        t = struct.unpack('<H', mm.group(1))[0]
        before = code[max(0, mm.start() - 24):mm.start()]
        n = None
        # cmp ax/any reg, imm16 or imm8 (sign-extended); ja/jbe
        c = list(re.finditer(rb'(?:\x3d|\x81[\xf8-\xff])(..)[\x76\x77]|\x83[\xf8-\xff](.)[\x76\x77]', before, re.S))
        if c:
            g = c[-1]
            n = struct.unpack('<H', g.group(1))[0] + 1 if g.group(1) else g.group(2)[0] + 1
        if n is None or n > 256 or t + 2 * n > length:
            continue
        tables[t] = [struct.unpack_from('<H', code, t + 2 * k)[0] for k in range(n)]
        skips.append((t, 2 * n))
    from ndis import ndisasm
    lines = ndisasm(code, 0, skips)
    prev_push_cs = False
    print('; %s segment' % os.path.basename(find_exe()) + ' %04X (image %05X..%05X)' % (seg, base, base + length))
    for l in lines:
        parts = l.split(None, 2)
        if len(parts) < 3:
            print(l)
            continue
        addr = int(parts[0], 16)
        hexb, ins = parts[1], parts[2]
        if ins.startswith('skipping'):
            if addr in tables:
                print('%04X  ; switch table: %s' % (addr, ' '.join('%X' % x for x in tables[addr])))
            continue
        note = []
        if hexb.startswith('9A') and len(hexb) == 10:
            o, s = struct.unpack('<HH', bytes.fromhex(hexb[2:]))
            s, o = resolve_far(s, o)
            nm = names.get((s, o)) or ('C_%04X_%04X' % (s, o))
            if s == seg:
                nm = names.get((s, o)) or 'C_%04X_%04X' % (s, o)
            ins = 'call far %s' % nm
        elif hexb.startswith('E8') and prev_push_cs:
            tgt = int(ins.split()[-1], 16)
            ins = 'call far %s (push cs/call near)' % (names.get((seg, tgt)) or 'C_%04X_%04X' % (seg, tgt))
        prev_push_cs = hexb == '0E'
        if re.search(r'jmp (word )?\[cs:bx\+', ins):
            t = int(ins.split('+')[-1].rstrip(']'), 16)
            if t in tables:
                note.append('switch: ' + ', '.join('%d:%X' % (k, v) for k, v in enumerate(tables[t])))
        # relocated words inside the instruction
        for k in range(0, len(hexb) // 2 - 1):
            if base + addr + k in relocs:
                v = struct.unpack_from('<H', img, base + addr + k)[0]
                if not hexb.startswith('9A'):
                    note.append('SEG %04X%s' % (v, ' (DGROUP)' if v == DGROUP else ''))
        for mm in re.finditer(r'0x([0-9a-f]{2,4})', '' if re.match(r'(j|call|loop)', ins) else ins):
            v = int(mm.group(1), 16)
            if v in strs and v >= 0x40:
                note.append('"%s"' % strs[v][:60].replace('\n', '\\n').replace('\r', '\\r'))
            elif v in dnames:
                note.append(dnames[v])
        print('%04X  %-22s %-38s%s' % (addr, hexb[:22], ins, ('; ' + '; '.join(note)) if note else ''))


if __name__ == '__main__':
    main()
