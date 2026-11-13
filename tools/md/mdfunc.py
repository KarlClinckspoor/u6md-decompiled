#!/usr/bin/env python3
"""Compare a C (or assembly) file with the original function by function, wherever each
function sits: for files that hold only part of a module, or functions out of order.

    python tools/md/mdfunc.py [--target martian|game] FILE [--include DIR ...] [--flags ...] [-v] [NAME ...]

Every public function of the compiled object whose address the source gives
(/*C_SSSS_OOOO*/ before its definition; asm ;SSSS:OOOO) is compared with the original at
that address, instruction by instruction (capstone), taking as equal:
  - bytes under fixups (names are not checked here: mdcmp.py does that on whole modules);
  - branch and near-call displacements of the same instruction;
  - a far call (9A) and `push cs / call near` (0E E8), which is what the same call becomes
    when the callee is defined earlier in the same file;
A static function has no public: make it non-static while you work, or compare its
caller. Prints, per function, "same" or the first difference with both disassemblies.
Exit status 0 when every compared function is the same.
"""
import argparse, os, re, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import target, omf, mdsym  # noqa: E402
from capstone import Cs, CS_ARCH_X86, CS_MODE_16

md = Cs(CS_ARCH_X86, CS_MODE_16)


def norm(code, base, mask):
    """[(addr, text, key)]: key compares equal for instructions that are the same up to
    masked bytes and displacements; push cs + call near folds into one 'callfar' item"""
    out = []
    ins = list(md.disasm(bytes(code), base))
    k = 0
    while k < len(ins):
        i = ins[k]
        b = bytes(i.bytes)
        off = i.address - base
        if b[0] == 0x0E and k + 1 < len(ins) and ins[k + 1].bytes[0] == 0xE8:
            out.append((i.address, 'push cs; call near %s' % ins[k + 1].op_str, 'callfar'))
            k += 2
            continue
        if b[0] == 0x9A:
            out.append((i.address, 'call far', 'callfar'))
            k += 1
            continue
        if i.mnemonic.startswith(('j', 'loop')) or (i.mnemonic == 'call' and b[0] == 0xE8):
            key = i.mnemonic
        else:  # bytes, and which of them lie under a fixup (compared as equal)
            key = (b, bytes(1 if off + j < len(mask) and mask[off + j] else 0 for j in range(len(b))))
        out.append((i.address, '%s %s' % (i.mnemonic, i.op_str), key))
        k += 1
    return out


def same(mine, orig):
    if isinstance(mine, str) or isinstance(orig, str):
        return mine == orig
    (b, m), (c, _) = mine, orig
    return len(b) == len(c) and all(m[j] or b[j] == c[j] for j in range(len(b)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('file')
    ap.add_argument('names', nargs='*')
    ap.add_argument('--include', action='append', default=[])
    ap.add_argument('--flags')
    ap.add_argument('-v', action='store_true', help='show the whole listing of differing functions')
    a = ap.parse_args()
    import tcc
    masm = a.file.lower().endswith('.asm')
    flags = a.flags or ('/Mx' if masm else target.T['flags'])
    res = tcc.compile_files([a.file], flags, out=tempfile.mkdtemp(), includes=a.include, masm=masm,
                            per_file=a.flags is None)
    obj, msgs = res[a.file]
    for m in msgs:
        print(m)
    if not obj:
        sys.exit('compile failed')
    t = {}
    mdsym.parse_file(a.file, os.path.basename(a.file), t)
    addr = {}
    for k, (name, fn, lang, static) in t.items():
        if k[0] == 'C':
            addr[name.upper()] = (k[1], k[2])
    img, _, _ = target.load_image(target.T['exe_path'])
    stubs = {o['stub']: o for o in target.overlays(target.T['exe_path'])}
    m = omf.parse_obj(obj)
    bad = 0
    for cs in [s for s in m.segs[1:] if s.cls == 'CODE' and s.length]:
        mask = bytearray(cs.length)
        for f in cs.fixups:
            for j in range(f.size):
                if f.offset + j < cs.length:
                    mask[f.offset + j] = 1
        pubs = sorted((o, n) for n, (si, o) in m.pubs.items() if m.segs[si] is cs)
        for idx, (o, n) in enumerate(pubs):
            key = n.lstrip('_').upper()
            if key not in addr or (a.names and n.lstrip('_') not in a.names and key not in [x.upper() for x in a.names]):
                continue
            end = pubs[idx + 1][0] if idx + 1 < len(pubs) else cs.length
            sg, of = addr[key]
            base = stubs[sg]['addr'] + of if sg in stubs else sg * 16 + of
            mine = norm(cs.data[o:end], 0, mask[o:end])
            orig = norm(img[base:base + (end - o) + 64], 0, b'')
            diff = None
            for j, x in enumerate(mine):
                if j >= len(orig) or not same(x[2], orig[j][2]):
                    diff = j
                    break
            if diff is None:
                print('same  %-28s C_%04X_%04X (%d bytes)' % (n, sg, of, end - o))
                continue
            bad += 1
            print('DIFF  %-28s C_%04X_%04X at +%X of %X:' % (n, sg, of, mine[diff][0], end - o))
            lo, hi = (0, len(mine)) if a.v else (max(0, diff - 6), diff + 8)
            print('   %-44s | %s' % ('original', 'yours'))
            for j in range(lo, hi):
                ol = '%04X %s' % orig[j][:2] if j < len(orig) else ''
                nl = '%04X %s' % mine[j][:2] if j < len(mine) else ''
                print(' %s %-44s | %s' % ('>' if j == diff else ' ', ol[:44], nl[:60]))
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
