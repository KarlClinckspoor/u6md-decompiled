#!/usr/bin/env python3
"""Pair every Martian Dreams function with the closest Savage Empire function, so that each
MD module knows which SE source to start from.

    python tools/md/mdmatch.py [--target martian|game] [--module FRAME] [--min 0.0]

Functions are cut at `push bp / mov bp,sp` (and at far entries) inside each module of
<SRC>/modules.txt; SE functions likewise inside each module of SE's SRC/ and SRC_S/
modules.txt (checkout ../u6se-decompiled or SE_ROOT, executables in its gamedata/SAVAGE).
Similarity: Indel ratio of the instruction sequences with branch targets and constants of
3+ hex digits masked. Output: MD frame, address, size, similarity, SE file, SE address
(the SE function's own C_SSSS_OOOO address).
"""
import argparse, os, re, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import target, shared  # noqa: E402
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from rapidfuzz.distance import Indel

SE_ROOT = os.environ.get('SE_ROOT') or os.path.join(os.path.dirname(target.ROOT), 'u6se-decompiled')
md = Cs(CS_ARCH_X86, CS_MODE_16)


def toks(code, base):
    out = []
    for i in md.disasm(code, base):
        s = i.mnemonic + ' ' + i.op_str
        if i.mnemonic.startswith(('j', 'call', 'lcall', 'loop')):
            s = i.mnemonic + ' T'
        out.append(re.sub(r'\b0x[0-9a-f]{3,}\b', 'N', s))
    return out


def split(img, st, en, frame, label, extra=()):
    fs = {st} | {a for a in extra if st < a < en}
    for m in re.finditer(b'\x55\x8b\xec', img[st:en]):
        fs.add(st + m.start())
    fs = sorted(fs)
    out = []
    for k, a in enumerate(fs):
        b = fs[k + 1] if k + 1 < len(fs) else en
        if b > a:
            out.append(dict(addr=a, size=b - a, frame=frame, mod=label, toks=toks(img[a:b], a)))
    return out


def md_functions(t, only=None):
    img, _, _ = target.load_image(t['exe_path'])
    F = []
    for m in shared.read_modules(t['src']):
        if m['frame'] is None or (only is not None and m['frame'] != only):
            continue
        o = [x for x in target.overlays(t['exe_path']) if x['stub'] == m['frame']]
        st = o[0]['addr'] + m['at'] if o else m['frame'] * 16 + m['at']
        extra = [o[0]['addr'] + e for e in o[0]['entries']] if o else []
        F += split(img, st, st + m['len'], m['frame'], m['src'], extra)
    return F


def se_functions():
    F = []
    for exe, src in (('SAVAGE.EXE', 'SRC_S'), ('GAME.EXE', 'SRC')):
        p = os.path.join(SE_ROOT, 'gamedata', 'SAVAGE', exe)
        if not os.path.exists(p):
            sys.exit('%s not found (SE_ROOT=%s)' % (p, SE_ROOT))
        img, _, _ = target.load_image(p)
        for m in shared.read_modules(os.path.join(SE_ROOT, src)):
            if m['frame'] is None:
                continue
            st = m['frame'] * 16 + m['at']
            F += split(img, st, st + m['len'], m['frame'], '%s/%s' % (src, m['src']))
    return F


def best(A, B):
    vocab = {}
    enc = lambda t: [vocab.setdefault(x, len(vocab)) for x in t]  # noqa: E731
    for f in A + B:
        f['e'] = enc(f['toks'])
    exact = {}
    for j, g in enumerate(B):
        exact.setdefault(tuple(g['e']), j)
    res = []
    for f in A:
        k = tuple(f['e'])
        if k in exact:
            res.append((1.0, exact[k]))
            continue
        bs, bj, n = 0.0, None, len(f['e'])
        for j, g in enumerate(B):
            m = len(g['e'])
            if 2 * min(n, m) / max(n + m, 1) <= bs:
                continue
            r = Indel.normalized_similarity(f['e'], g['e'], score_cutoff=bs)
            if r > bs:
                bs, bj = r, j
        res.append((bs, bj))
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--module', help='only this frame (hex)')
    ap.add_argument('--min', type=float, default=0.0)
    a = ap.parse_args()
    t = target.T
    A = md_functions(t, int(a.module, 16) if a.module else None)
    B = se_functions()
    print('# %s function -> closest Savage Empire function (tools/md/mdmatch.py)' % t['exe'])
    print('# FRAME   ADDRESS  SIZE  SIM  MD_SOURCE  SE_FILE  SE_ADDRESS')
    for f, (r, j) in zip(A, best(A, B)):
        if r < a.min:
            continue
        g = B[j] if j is not None else None
        ovl = f['addr'] >= target.OVL_BASE
        print('%s%04X %06X %5d %.2f %-10s %-28s %s' % (
            'O' if ovl else ' ', f['frame'], f['addr'], f['size'], r, f['mod'],
            g['mod'] if g else '-', 'C_%04X_%04X' % (g['frame'], g['addr'] - g['frame'] * 16) if g else '-'))


if __name__ == '__main__':
    main()
