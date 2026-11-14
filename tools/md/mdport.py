#!/usr/bin/env python3
"""Carry a Savage Empire source over to Martian Dreams: compile it as it is, compare it
with the Martian Dreams module it matches (tools/md/mdcmp.py), and rewrite its address
names (C_SSSS_OOOO, D_XXXX and the /*C_SSSS_OOOO*/, /*XXXX*/ definition comments) to the
Martian Dreams addresses the comparison found.

    python tools/md/mdport.py [--target martian|game] SE_FILE --seg SEG --at AT -o OUT
                              [--se-target savage|game] [--flags ...]

SE_FILE is a file of the SE decompilation (default checkout ../u6se-decompiled, or
SE_ROOT), or of this repository's SRC_M (MARTIAN.EXE, to carry it over to GAME.EXE with
--target game). Its own position (frame, first byte, _DATA range) is read from its
modules.txt. Only what the comparison can vouch for is renamed: external names at fixups
before the first real difference (and in _DATA), the module's own functions that start
before it, its own initialised data when _DATA matched up to them. Everything else keeps
its SE name and is listed, for the worker to settle by hand. Comments are not touched.
"""
import argparse, json, os, re, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import target  # noqa: E402

TOK = r'(?<![A-Za-z0-9])'  # an address name, alone or as an assembly/C link name (_D_XXXX)
SE_ROOT = os.environ.get('SE_ROOT') or os.path.join(os.path.dirname(target.ROOT), 'u6se-decompiled')


def se_module(path):
    """(SE target, frame, at, len, (d0, d1) or None) of an SE source file"""
    ap = os.path.normcase(os.path.abspath(path))
    for name, d in (('savage', os.path.join(SE_ROOT, 'SRC_S')), ('game', os.path.join(SE_ROOT, 'SRC')),
                    ('martian', os.path.join(target.ROOT, 'SRC_M'))):
        p = os.path.join(d, 'modules.txt')
        if not os.path.exists(p):
            continue
        for line in open(p):
            f = line.split('#')[0].split()
            if len(f) >= 4 and os.path.normcase(os.path.abspath(os.path.join(d, f[3]))) == ap and f[0] != '-':
                data = tuple(int(x, 16) for x in f[4].split('-')) if len(f) > 4 else None
                return name, int(f[0], 16), int(f[1], 16), int(f[2], 16), data
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('file')
    ap.add_argument('--seg', required=True)
    ap.add_argument('--at', default='0')
    ap.add_argument('-o', '--out', required=True)
    ap.add_argument('--flags')
    a = ap.parse_args()
    mod = se_module(a.file)
    if not mod:
        sys.exit('%s is not in an SE modules.txt' % a.file)
    se_t, se_fr, se_at, se_len, se_data = mod
    se_start = se_fr * 16 + se_at
    with tempfile.TemporaryDirectory() as tmp:
        mp = os.path.join(tmp, 'map.json')
        args = [sys.executable, os.path.join(HERE, 'mdcmp.py'), '--target', target.T['name'], a.file,
                '--seg', a.seg, '--at', a.at, '--map-out', mp,
                '--include', os.path.dirname(os.path.abspath(a.file)),
                '--include', {'savage': os.path.join(SE_ROOT, 'SRC_S'), 'game': os.path.join(SE_ROOT, 'SRC'),
                              'martian': os.path.join(target.ROOT, 'SRC_M')}[se_t]]
        if a.flags:
            args += ['--flags', a.flags]
        r = subprocess.run(args, capture_output=True, text=True)
        print('\n'.join(r.stdout.splitlines()[:3]))
        if not os.path.exists(mp):
            print(r.stdout, r.stderr)
            sys.exit('comparison failed')
        m = json.load(open(mp))
    seg, at, cutoff = m['seg'], m['at'], m['cutoff']
    ren, clash = {}, set()

    def put(old, new):
        if old in clash:
            return
        if old in ren and ren[old] != new:
            clash.add(old)
            del ren[old]
            return
        ren[old] = new

    for sn, off, name, kind, v in m['obs']:
        if sn == m['cseg'] and off >= cutoff:
            continue
        n = name.lstrip('_')
        if kind == 'C' and re.fullmatch(r'C_[0-9A-F]{4}_[0-9A-F]{4}', n, re.I):
            put(n.upper().replace('C_', 'C_', 1), 'C_%04X_%04X' % tuple(v))
        elif kind == 'D' and re.fullmatch(r'D_[0-9A-F]{4}', n, re.I):
            put(n.upper(), 'D_%04X' % v)
    # a function's address taken as OFFSET and SEG (a far pointer built in two moves)
    segs, offs = {}, {}
    for sn, off, name, kind, v in m['obs']:
        if (sn == m['cseg'] and off >= cutoff) or kind not in ('CO', 'CS'):
            continue
        (offs if kind == 'CO' else segs).setdefault(name.lstrip('_').upper(), set()).add(v)
    for n in set(segs) & set(offs):
        if len(segs[n]) == 1 and len(offs[n]) == 1 and re.fullmatch(r'C_[0-9A-F]{4}_[0-9A-F]{4}', n):
            put(n, 'C_%04X_%04X' % (segs[n].pop(), offs[n].pop()))
    s = open(a.file, encoding='latin-1', newline='').read()
    # own functions: SE address -> offset in the module -> MD address
    own_c, own_d = {}, {}
    # (C sources name them C_SSSS_OOOO, assembly writes ;SSSS:OOOO after a PROC or PUBLIC)
    for mm in re.finditer(TOK + r'C_([0-9A-F]{4})_([0-9A-F]{4})\b|;\s*([0-9A-F]{4}):([0-9A-F]{4})\b', s):
        sg, of = (mm.group(1), mm.group(2)) if mm.group(1) else (mm.group(3), mm.group(4))
        rel = int(sg, 16) * 16 + int(of, 16) - se_start
        if 0 <= rel < se_len:
            own_c['C_%s_%s' % (sg, of)] = rel
    for k, rel in own_c.items():
        if rel < cutoff:
            put(k, 'C_%04X_%04X' % (seg, at + rel))
    # own initialised data: SE DS offset -> offset in _DATA -> MD DS offset
    if se_data and m['data'] is not None:
        lim = m['dfirst'] if m['dfirst'] is not None else se_data[1] - se_data[0]
        for mm in re.finditer(r'/\*([0-9A-F]{4})\*/|' + TOK + r'D_([0-9A-F]{4})\b', s):
            v = int(mm.group(1) or mm.group(2), 16)
            if se_data[0] <= v < se_data[1]:
                own_d[v] = v - se_data[0]
        for v, rel in own_d.items():
            if rel < lim:
                put('D_%04X' % v, 'D_%04X' % (m['data'] + rel))
    out = re.sub(TOK + r'(C_[0-9A-F]{4}_[0-9A-F]{4}|D_[0-9A-F]{4})\b', lambda g: ren.get(g.group(1), g.group(1)), s)
    out = re.sub(r'/\*([0-9A-F]{4})\*/', lambda g: '/*%s*/' % ren.get('D_' + g.group(1), 'D_' + g.group(1))[2:], out)

    def asm_def(g):
        k = 'C_%s_%s' % (g.group(2), g.group(3))
        return ';%s%s' % (g.group(1), ren.get(k, k)[2:].replace('_', ':'))
    out = re.sub(r';(\s*)([0-9A-F]{4}):([0-9A-F]{4})\b', asm_def, out)
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    open(a.out, 'w', encoding='latin-1', newline='').write(out)
    left = sorted(set(re.findall(TOK + r'(C_[0-9A-F]{4}_[0-9A-F]{4}|D_[0-9A-F]{4})\b', s)) - set(ren))
    print('%s -> %s: %d names moved, %d kept (code identical up to +%X of %X%s)' % (
        os.path.basename(a.file), a.out, len(ren), len(left), cutoff, m['length'],
        ', _DATA at DS:%04X' % m['data'] if m['data'] is not None else ''))
    if clash:
        print('  names seen at two addresses (kept): ' + ' '.join(sorted(clash)))
    if left:
        print('  kept: ' + ' '.join(left))


if __name__ == '__main__':
    main()
