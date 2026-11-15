"""Run DOS batch files in a headless DOSBox (or DOSBox-X) on a scratch C: drive."""
import os, shutil, subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
TOOLS = os.environ.get('MD_DOSTOOLS') or os.path.join(ROOT, 'tools')


def dosbox():
    """MD_DOSBOX, else DOSBox-X or DOSBox on the PATH, else the GOG release's DOSBox 0.74
    (gamedata/DOSBOX/DOSBox.exe); exits with a message when there is none"""
    gog = os.path.join(ROOT, 'gamedata', 'DOSBOX', 'DOSBox.exe')
    db = (os.environ.get('MD_DOSBOX') or shutil.which('dosbox-x') or shutil.which('dosbox') or
          (gog if os.path.exists(gog) else None))
    if not db:
        raise SystemExit('DOSBox not found: put dosbox or dosbox-x on the PATH, set MD_DOSBOX, '
                         'or copy the DOSBOX folder of the GOG release to gamedata/DOSBOX')
    return db


def masm_exe(tools=None):
    """tools/masm300/MASM.EXE (tools/md/setup.py takes it out of a floppy image there)"""
    mdir = os.path.join(tools or TOOLS, 'masm300')
    exe = [n for n in os.listdir(mdir) if n.upper() == 'MASM.EXE'] if os.path.isdir(mdir) else []
    if not exe:
        raise SystemExit('tools/masm300/MASM.EXE missing: put MASM 3.0 (MASM.EXE or its floppy image) '
                         'in tools/masm300/ and run python3 tools/md/setup.py')
    return os.path.join(mdir, exe[0])


def crlf(src, dst):
    b = open(src, 'rb').read().replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')
    open(dst, 'wb').write(b)


def run(cdir, lines, timeout=1800):
    """Write lines to cdir/RUN.BAT and run it with cdir mounted as C:."""
    open(os.path.join(cdir, 'RUN.BAT'), 'w', newline='').write('\r\n'.join(['@echo off'] + lines) + '\r\n')
    conf = os.path.join(cdir, '..', 'dosbox.conf')
    open(conf, 'w').write('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncycles=max\n'
                          '[autoexec]\nmount c "%s"\nc:\ncall \\RUN.BAT\nexit\n' % os.path.abspath(cdir))
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    db = dosbox()
    args = [db, '-conf', conf, '-noconsole'] if 'dosbox-x' not in os.path.basename(db).lower() \
        else [db, '-conf', conf, '-nogui', '-exit']
    subprocess.run(args, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=timeout)


def install_tools(cdir, masm=True):
    """Copy Turbo C++ 1.00 (tools/tcpp100) to C:\\TC and MASM 3.0 to C:\\TC\\MASM.EXE."""
    tc = os.path.join(TOOLS, 'tcpp100')
    if not os.path.isdir(tc):
        raise SystemExit('tools/tcpp100 missing: run python3 tools/md/setup.py')
    shutil.copytree(tc, os.path.join(cdir, 'TC'))
    if masm:
        shutil.copy(masm_exe(), os.path.join(cdir, 'TC', 'MASM.EXE'))
