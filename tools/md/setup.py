#!/usr/bin/env python3
"""Unpack Turbo C++ 1.00 and 1.01 from the disk-image archives in tools/
(tools/*1.00*.7z, tools/*1.01*.7z), and MASM 3.0's MASM.EXE from the floppy image in
tools/masm300/ (unless MASM.EXE is already there), with 7-Zip (it reads the FAT images
and Borland's imploded ZIPs, so neither mtools nor Info-ZIP is needed; this works on
Windows).

    python tools/md/setup.py

Result:
  tools/tcpp100/   TCC.EXE TLINK.EXE TLIB.EXE ... INCLUDE\\ LIB\\ (C0M.OBJ, CM.LIB, OVERLAY.LIB)
  tools/tcpp101/   the same from 1.01 (Origin's library C files that name it:
                   /* compiler: Turbo C++ 1.01 */)
  tools/masm300/MASM.EXE

Martian Dreams links Turbo C++ 1.00's CM.LIB as shipped (unlike Savage Empire, which had
three 1.01 modules appended), and GAME.EXE also 1.00's OVERLAY.LIB (VROOMM).
Needs 7-Zip: 7z on the PATH, in C:\\Program Files\\7-Zip, or SEVENZIP=path.
"""
import glob, os, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
TOOLS = os.environ.get('MD_DOSTOOLS') or os.path.join(ROOT, 'tools')

WANT = {'TCC.ZIP': '', 'BIN1.ZIP': '', 'BIN2.ZIP': '', 'INCLUDE.ZIP': 'INCLUDE', 'MLIB.ZIP': 'LIB',
        'XLIB.ZIP': 'LIB', 'STARTUP.ZIP': 'STARTUP'}


def sevenzip():
    for p in (os.environ.get('SEVENZIP'), shutil.which('7z'), shutil.which('7za'),
              r'C:\Program Files\7-Zip\7z.exe', r'C:\Program Files (x86)\7-Zip\7z.exe'):
        if p and os.path.exists(p):
            return p
    sys.exit('7-Zip not found: install it or set SEVENZIP')


def x(z, *args):
    r = subprocess.run([sevenzip(), *args], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if r.returncode not in (0, 1):
        raise SystemExit('7z %s failed:\n%s' % (' '.join(args), r.stdout[-2000:]))


def unpack(archive, dst):
    with tempfile.TemporaryDirectory() as tmp:
        x(None, 'x', '-y', '-o' + tmp, archive)
        zips = os.path.join(tmp, 'zips')
        for img in sorted(glob.glob(os.path.join(tmp, '**', '*.img'), recursive=True)):
            x(None, 'e', '-y', '-o' + zips, img, '*.ZIP')
        for z, sub in WANT.items():
            p = os.path.join(zips, z)
            if not os.path.exists(p):
                continue
            out = os.path.join(dst, sub)
            os.makedirs(out, exist_ok=True)
            x(None, 'x', '-y', '-o' + out, p)
    for dp, _, fs in os.walk(dst):  # DOS names in upper case
        for f in fs:
            if f != f.upper():
                os.rename(os.path.join(dp, f), os.path.join(dp, f.upper()))


def main():
    a100 = glob.glob(os.path.join(TOOLS, '*1.00*.7z'))
    a101 = glob.glob(os.path.join(TOOLS, '*1.01*.7z'))
    if not a100 or not a101:
        sys.exit('put the Turbo C++ 1.00 and 1.01 .7z archives in tools/ (names with "1.00" and "1.01")')
    for arcs, d in ((a100, 'tcpp100'), (a101, 'tcpp101')):
        dst = os.path.join(TOOLS, d)
        shutil.rmtree(dst, ignore_errors=True)
        unpack(arcs[0], dst)
        print('unpacked', os.path.basename(arcs[0]), '->', dst)
    mdir = os.path.join(TOOLS, 'masm300')
    if os.path.isdir(mdir) and not any(n.upper() == 'MASM.EXE' for n in os.listdir(mdir)):
        img = [n for n in os.listdir(mdir) if n.lower().endswith('.img')]
        if img:
            x(None, 'e', '-y', '-o' + mdir, os.path.join(mdir, img[0]), 'MASM.EXE')
    print('%-16s %s' % ('MASM.EXE', 'ok' if os.path.exists(os.path.join(mdir, 'MASM.EXE')) else
                        'MISSING: put MASM 3.0 (MASM.EXE or its floppy image, .img) in tools/masm300/'))
    for f in ('TCC.EXE', 'TLINK.EXE', 'TLIB.EXE', os.path.join('LIB', 'C0M.OBJ'), os.path.join('LIB', 'CM.LIB'),
              os.path.join('LIB', 'OVERLAY.LIB')):
        print('%-16s %s' % (f, 'ok' if os.path.exists(os.path.join(TOOLS, 'tcpp100', f)) else 'MISSING'))
    print('%-16s %s' % ('1.01 TCC.EXE', 'ok' if os.path.exists(os.path.join(TOOLS, 'tcpp101', 'TCC.EXE')) else 'MISSING'))


if __name__ == '__main__':
    main()
