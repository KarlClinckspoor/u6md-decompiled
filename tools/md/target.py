"""The two executables of the Martian Dreams decompilation and what differs between them.

Pick one with --target or the MD_TARGET environment variable (default: martian).

GAME.EXE is an overlaid program (Borland VROOMM, TLINK /o): its overlaid code is not in
the MZ image but in the FBOV block after it. load_image() returns one address space for
both: the MZ image as it is, then each overlay's code at OVL_BASE + its offset in the
FBOV block (as tools/md/mdmatch_proto.py numbered them). overlays() lists them.
"""
import os, struct

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
OVL_BASE = 0x100000

TARGETS = {
    # MARTIAN.EXE: launcher, title, intro, character creation, endgame (cdecl)
    'martian': dict(exe='MARTIAN.EXE', src='SRC_M', dgroup=0x172A, syms='martian_syms.txt', bss=0x2366, bss_end=0x465C,
                    lib='CM.LIB', libs=['EMU', 'MATHM', 'CM'], flags='-mm -c -O -Z -d'),
    # GAME.EXE: the game (pascal, overlaid)
    'game': dict(exe='GAME.EXE', src='SRC', dgroup=0x2FAB, syms='game_syms.txt', bss=None,
                 lib='CM.LIB', libs=['OVERLAY', 'CM'], overlay=True, flags='-mm -p -c -O -Z -d'),
}


def get(name=None):
    name = name or os.environ.get('MD_TARGET') or 'martian'
    t = dict(TARGETS[name])
    t['name'] = name
    t['src'] = os.path.join(ROOT, t['src'])
    t['syms'] = os.path.join(HERE, t['syms'])
    t['exe_path'] = find_exe(t['exe'])
    return t


def find_exe(name):
    for p in (os.environ.get('MD_GAMEDIR') and os.path.join(os.environ['MD_GAMEDIR'], name),
              os.path.join(ROOT, 'gamedata', 'MARTIAN', name), os.path.join(ROOT, 'gamedata', name)):
        if p and os.path.exists(p):
            return p
    return os.path.join(ROOT, 'gamedata', 'MARTIAN', name)


def mz(d):
    """(header bytes, MZ image end in the file)"""
    last, pages = struct.unpack_from('<HH', d, 2)
    size = (pages - 1) * 512 + (last or 512)
    return struct.unpack_from('<H', d, 8)[0] * 16, size


def overlays(path=None):
    """The VROOMM overlays of an executable, [] if it has none. Each one is a dict:
    stub (paragraph of its stub segment in the image), size (code bytes), fileofs (offset of
    its code in the FBOV block, past the 16-byte FBOV header), addr (its address in
    load_image()'s address space), nrel (its relocation count), relofs (file offset of
    its relocation list, after the code: one word per fixed-up segment word, offsets in
    the overlay's code), entries (the stub's entry offsets, in order).

    The segment table that lists them (FBOV header +8: its file offset; +12: entries)
    has 8-byte entries: segment, max offset, flags (1 code, 3 overlay stub, 0/4 data),
    min offset."""
    d = open(path or get()['exe_path'], 'rb').read()
    hdr, size = mz(d)
    if d[size:size + 4] != b'FBOV':
        return []
    ovsize, exeinfo, segnum = struct.unpack_from('<III', d, size + 4)
    img = d[hdr:size]
    out = []
    for i in range(segnum):
        seg, mx, fl, mn = struct.unpack_from('<HHHH', d, exeinfo + 8 * i)
        if fl != 3:
            continue
        st = seg * 16
        # stub: CD 3F (int 3Fh), then: dword file offset, word code size, word reloc bytes,
        # word entry count, ... then 5-byte entries (CD 3F, word offset, 1 byte)
        fo, cs, rs, ne = struct.unpack_from('<IHHH', img, st + 4)
        ents = [struct.unpack_from('<H', img, st + 0x20 + 5 * k + 2)[0] for k in range(ne)]
        out.append(dict(stub=seg, size=cs, fileofs=fo, addr=OVL_BASE + fo, nrel=rs // 2,
                        relofs=size + 16 + fo + cs, entries=ents, table=i))
    return out


def segment_table(path=None):
    """[(segment, max offset, flags, min offset)] of an overlaid executable, [] otherwise"""
    d = open(path or get()['exe_path'], 'rb').read()
    hdr, size = mz(d)
    if d[size:size + 4] != b'FBOV':
        return []
    ovsize, exeinfo, segnum = struct.unpack_from('<III', d, size + 4)
    return [struct.unpack_from('<HHHH', d, exeinfo + 8 * i) for i in range(segnum)]


def load_image(path):
    """(image bytes, set of image addresses holding relocated segment words, header bytes)

    For an overlaid program the image continues with its overlays' code at OVL_BASE +
    file offset (zeros in between); their relocations are in the set too."""
    d = open(path, 'rb').read()
    hdrp, size = mz(d)
    nrel, relofs = struct.unpack_from('<H', d, 6)[0], struct.unpack_from('<H', d, 0x18)[0]
    relocs = set()
    for i in range(nrel):
        off, seg = struct.unpack_from('<HH', d, relofs + 4 * i)
        relocs.add(seg * 16 + off)
    img = d[hdrp:size]
    ovl = overlays(path)
    if ovl:
        end = max(o['addr'] + o['size'] for o in ovl)
        big = bytearray(end)
        big[:len(img)] = img
        for o in ovl:
            code = d[size + 16 + o['fileofs']:size + 16 + o['fileofs'] + o['size']]
            big[o['addr']:o['addr'] + o['size']] = code
            for k in range(o['nrel']):
                relocs.add(o['addr'] + struct.unpack_from('<H', d, o['relofs'] + 2 * k)[0])
        img = bytes(big)
    return img, relocs, d[:hdrp]


def overlay_at(addr, path=None):
    """the overlay holding image address addr, or None"""
    for o in overlays(path):
        if o['addr'] <= addr < o['addr'] + o['size']:
            return o
    return None


def _from_argv():
    """--target NAME anywhere on a tool's command line selects the target (and is removed)"""
    import sys
    a = sys.argv
    for i, x in enumerate(a):
        if x == '--target' and i + 1 < len(a):
            os.environ['MD_TARGET'] = a[i + 1]
            del a[i:i + 2]
            return
        if x.startswith('--target='):
            os.environ['MD_TARGET'] = x.split('=', 1)[1]
            del a[i]
            return


_from_argv()
T = get()
