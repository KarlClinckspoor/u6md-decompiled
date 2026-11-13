#!/usr/bin/env python3
"""Check every decompiled module listed in <SRC>/modules.txt against the original
(tools/md/mdcmp.py), in parallel.

    python3 tools/md/mdcheck.py [--target martian|game] [FILE_OR_FRAME ...]

A module passes when its code is identical and its length is the table's LEN (or LEN-1:
the last byte may be TLINK's word-alignment padding). A module's _DATA is checked against
the DS range given in modules.txt as a fifth column START-END, when there is one.
A line whose frame is "-" is a module with data only: it is linked in its place but has no
code to compare (the full build, tools/md/mdbuild.py, checks it).
"""
import os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import target  # noqa: E402


def modules():
    out = []
    for line in open(os.path.join(target.T['src'], 'modules.txt')):
        line = line.split('#')[0].split()
        if len(line) < 4:
            continue
        fr, at, ln, src = line[:4]
        data = line[4] if len(line) > 4 else None
        out.append((fr, at, int(ln, 16), src, data))
    return out


def check(m):
    fr, at, ln, src, data = m
    path = os.path.join(target.T['src'], src)
    args = [sys.executable, os.path.join(HERE, 'mdcmp.py'), '--target', target.T['name'], path, '--seg', fr,
            '--at', at, '--include', target.T['src']]
    if os.path.dirname(path) != target.T['src']:
        args += ['--include', os.path.dirname(path)]
    if data:
        d0, d1 = data.split('-')
        args += ['--data-range', d0, d1]
    r = subprocess.run(args, capture_output=True, text=True)
    lines = [l for l in r.stdout.splitlines() if not l.startswith('Warning ')]
    ok = r.returncode == 0
    mm = re.search(r'code ([0-9a-f]+) bytes', r.stdout)
    end = int(fr, 16) * 16 + int(at, 16) + ln
    code = int(mm.group(1), 16) if mm else ln
    # a pad byte after an odd end, or zeros up to a paragraph-aligned segment that follows
    padded = code in (ln, ln - 1) or (end % 16 == 0 and 0 < ln - code < 16)
    if mm and not padded:
        ok = False
        lines.append('  LENGTH: %x bytes, the module is %x' % (int(mm.group(1), 16), ln))
    return src, fr, ok, lines + [l for l in r.stderr.splitlines() if l.strip()][-3:]


def main():
    want = [w.upper() for w in sys.argv[1:]]
    mods = [m for m in modules() if m[3] != '-' and m[0] != '-' and (not want or m[0] in want or m[3].upper() in want or
                                                      os.path.splitext(m[3])[0].upper() in want)]
    bad = 0
    with ThreadPoolExecutor(max_workers=6) as ex:
        for src, fr, ok, lines in ex.map(check, mods):
            print('%-4s %s %s' % ('ok' if ok else 'FAIL', fr, src))
            if not ok:
                bad += 1
                for l in lines[:25]:
                    print('    ' + l)
    done = sum(1 for m in modules() if m[3] != '-')
    print('%d of %d modules decompiled, %d failing' % (done, len(modules()), bad))
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
