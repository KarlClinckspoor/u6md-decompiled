#!/usr/bin/env python3
"""Put the publics of an overlaid module's object in the order that gives the original's
overlay stub entries (GAME.EXE).

    python3 tools/md/stuborder.py FILE.c FILE.OBJ      (rewrites FILE.OBJ)

TLINK /o gives each overlay a stub with one entry (int 3Fh, offset) for each far function
outside code can reach: every public of the overlaid code segment, and every function of
it that a far fixup points at (a static whose address is stored, a call to a function
defined later). The entries of the publics come in the reverse order of the module's
PUBDEF records; those that only fixups ask for come before them. Turbo C writes the
PUBDEFs in the order of its symbol table, a hash of the names, so Origin's names gave
an order that ours do not. A module whose order differs says so in a comment with the
offsets (in the overlay's code) of its functions in the order of the original's entries:

    /* stub order: 0000 0386 0237 */

The module's PUBDEF records are then rewritten, one public each, in the place of the
first one: the publics of other segments first, then those of the code segment, last
listed first. Every public of the code segment must be listed; a listed offset with no
public (a static reached by a fixup) is skipped: TLINK puts its entry first anyway.
"""
import re, struct, sys, os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import omf  # noqa: E402


def wanted_order(src):
    m = re.search(r'/\*\s*stub order:([0-9A-Fa-f\s]+)\*/', open(src, encoding='latin-1').read())
    return [int(x, 16) for x in m.group(1).split()] if m else []


def _index(b, i):
    return ((b[i] & 0x7F) << 8 | b[i + 1], i + 2) if b[i] & 0x80 else (b[i], i + 1)


def _enc_index(v):
    return bytes([v]) if v < 0x80 else bytes([0x80 | v >> 8, v & 0xFF])


def _record(t, body):
    L = len(body) + 1
    return bytes([t]) + struct.pack('<H', L) + body + bytes([(-(t + (L & 0xFF) + (L >> 8) + sum(body))) & 0xFF])


def fix(src, obj):
    order = wanted_order(src)
    if not order:
        return
    code = {i for i, s in enumerate(omf.parse_obj(obj).segs) if s is not None and s.cls == 'CODE'}
    d = open(obj, 'rb').read()
    recs, pubs, p = [], [], 0
    while p < len(d):
        t = d[p]
        ln = struct.unpack_from('<H', d, p + 1)[0]
        if t == 0x90:  # PUBDEF: group, segment (frame if both 0), then name, offset, type
            rec = d[p + 3:p + 2 + ln]
            grp, i = _index(rec, 0)
            seg, i = _index(rec, i)
            if grp == 0 and seg == 0:
                raise SystemExit('%s: a PUBDEF with a frame number' % obj)
            while i < len(rec):
                n = rec[i]
                name = rec[i + 1:i + 1 + n]
                off = struct.unpack_from('<H', rec, i + 1 + n)[0]
                typ, i = _index(rec, i + 3 + n)
                pubs.append((grp, seg, name, off, typ))
            if not recs or recs[-1] != 'PUB':
                recs.append('PUB')
        else:
            recs.append(d[p:p + 3 + ln])
        p += 3 + ln
    if recs.count('PUB') != 1:
        raise SystemExit('%s: PUBDEF records not together' % obj)
    rank = {o: k for k, o in enumerate(order)}
    mine = [x for x in pubs if x[1] in code]
    left = [x for x in mine if x[3] not in rank]
    if left:
        raise SystemExit('%s: stub order lacks %s' % (src, ' '.join('%04X' % x[3] for x in left)))
    new = [x for x in pubs if x[1] not in code] + sorted(mine, key=lambda x: -rank[x[3]])
    body = b''.join(_record(0x90, _enc_index(g) + _enc_index(s) + bytes([len(n)]) + n + struct.pack('<H', o) +
                            _enc_index(ty)) for g, s, n, o, ty in new)
    open(obj, 'wb').write(b''.join(body if r == 'PUB' else r for r in recs))


if __name__ == '__main__':
    fix(sys.argv[1], sys.argv[2])
