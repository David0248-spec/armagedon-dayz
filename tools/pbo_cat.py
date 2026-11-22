#!/usr/bin/env python3
"""Wypisuje pliki z PBO (z obsługą kompresji LZSS). Użycie: pbo_cat.py plik.pbo [fragment_nazwy...]"""
import sys, struct
def rd(b, i):
    j = b.index(b'\0', i); return b[i:j].decode('latin1'), j + 1
def lzss(src, n):
    out = bytearray(); i = 0
    while len(out) < n and i < len(src):
        flags = src[i]; i += 1
        for bit in range(8):
            if len(out) >= n or i >= len(src): break
            if flags & (1 << bit):
                out.append(src[i]); i += 1
            else:
                lo, hi = src[i], src[i + 1]; i += 2
                rpos = len(out) - (lo | ((hi & 0xF0) << 4)); rlen = (hi & 0x0F) + 3
                for _ in range(rlen):
                    out.append(out[rpos] if rpos >= 0 else 0x20); rpos += 1
    return bytes(out[:n])
b = open(sys.argv[1], 'rb').read(); i = 0; ents = []
while True:
    name, i = rd(b, i)
    meth, orig, res, ts, size = struct.unpack_from('<5I', b, i); i += 20
    if name == '' and meth == 0x56657273:
        while True:
            k, i = rd(b, i)
            if k == '': break
            v, i = rd(b, i)
        continue
    if name == '': break
    ents.append((name, size, orig, meth))
pats = [p.lower() for p in sys.argv[2:]]
for name, size, orig, meth in ents:
    d = b[i:i + size]; i += size
    if not pats:
        print(name, size, orig, hex(meth)); continue
    if any(p in name.lower() for p in pats):
        if meth == 0x43707273 or (orig and orig != size): d = lzss(d, orig)
        print('=====', name); sys.stdout.write(d.decode('latin1')); print()
