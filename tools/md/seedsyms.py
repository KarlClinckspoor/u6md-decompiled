#!/usr/bin/env python3
"""Write the target's symbol file with the run-time library's names: C0M.OBJ's publics and
those of every CMSE.LIB module found in the image (tools/md/libscan.py).

    python3 tools/md/seedsyms.py [--target martian|game]      (rewrites tools/md/<target>_syms.txt)
"""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import target, omf, libscan  # noqa: E402

t = target.T
img, rel, _ = target.load_image(t['exe_path'])
lines = ['# %s symbols: NAME SEG:OFF (code, far) or NAME OFF (DS offset).' % t['exe'],
         '# Library entries: Turbo C++ 1.00 C0M.OBJ and CM.LIB (EMU, MATHM, OVERLAY) modules matched in the image',
         '# (tools/md/seedsyms.py); the rest learned by tools/md/mdcmp.py --learn.']
c0 = omf.parse_obj(os.path.join(target.ROOT, 'tools', 'tcpp100', 'LIB', 'C0M.OBJ'))
for n, (si, o) in sorted(c0.pubs.items(), key=lambda x: x[1][1]):
    if si and c0.segs[si].name == '_TEXT':
        lines.append('%s 0000:%04X' % (n, o))
mods = {}
for lib in ('EMU.LIB', 'MATHM.LIB', 'OVERLAY.LIB', t['lib']):
    for m in omf.parse_lib(os.path.join(target.ROOT, 'tools', 'tcpp100', 'LIB', lib)):
        mods.setdefault(m.name, m)
for st, n, name, seg in libscan.scan(img, 0, t['dgroup'] * 16):
    m = mods[name]
    # _TEXT joins C0M's frame 0000; other code segments (EMU_PROG, _OVRTEXT_) have their own
    fr = 0 if seg == '_TEXT' else st // 16
    for pn, (si, o) in sorted(m.pubs.items(), key=lambda x: x[1][1]):
        if si and m.segs[si].name == seg:
            lines.append('%s %04X:%04X' % (pn, fr, st - fr * 16 + o))
open(t['syms'], 'w').write('\n'.join(lines) + '\n')
print('%d names -> %s' % (len(lines) - 3, t['syms']))
