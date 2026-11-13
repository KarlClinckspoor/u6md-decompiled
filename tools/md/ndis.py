"""ndisasm's output, made with capstone (no NASM needed): lines "OFFSET  HEXBYTES  INSTRUCTION",
offsets as 8 hex digits, the bytes in upper case, `-k` skip ranges as "skipping 0xN bytes".
The instruction text is close to ndisasm's where the tools look at it: `[cs:bx+0x12]`
memory operands, `call 0xSEG:0xOFF` for a direct far call, `call far [..]` for an
indirect one, `db 0xNN` for a byte that is no instruction."""
import re
from capstone import Cs, CS_ARCH_X86, CS_MODE_16

_md = Cs(CS_ARCH_X86, CS_MODE_16)


def _text(ins):
    m, op = ins.mnemonic, ins.op_str
    op = re.sub(r'\b(byte|word|dword|qword) ptr ', lambda g: g.group(1) + ' ', op)
    op = re.sub(r'\b([cdefgs]s):\[([^\]]*)\]', lambda g: '[%s:%s]' % (g.group(1), g.group(2)), op)
    op = re.sub(r'\[([^\]]*)\]', lambda g: '[' + g.group(1).replace(' ', '') + ']', op)
    if m in ('lcall', 'ljmp'):
        base = m[1:]
        m, op = (base, op.replace(', ', ':')) if re.match(r'0x[0-9a-f]+(, |:)0x', op) else (base + ' far', op)
    return (m + ' ' + op).strip()


def ndisasm(blob, origin=0, skips=()):
    skip = {o: n for o, n in skips}
    out = []
    pos = 0
    n = len(blob)
    while pos < n:
        if pos in skip:
            out.append('%08X  skipping 0x%X bytes' % (origin + pos, skip[pos]))
            pos += skip[pos]
            continue
        # the next skip bounds this run
        end = min([o for o in skip if o > pos] + [n])
        got = False
        for ins in _md.disasm(blob[pos:end], origin + pos):
            got = True
            out.append('%08X  %-16s  %s' % (ins.address, ins.bytes.hex().upper(), _text(ins)))
            pos = ins.address - origin + ins.size
        if pos < end:  # undecodable byte, or an instruction cut by the end
            out.append('%08X  %-16s  db 0x%02x' % (origin + pos, '%02X' % blob[pos], blob[pos]))
            pos += 1
        elif not got:
            pos = end
    return out
