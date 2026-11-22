#!/usr/bin/env python3
"""Lista modeli .p3d w modach serwera, filtrowana słowami kluczowymi."""
import os, sys, struct, re, glob
KEYS = sys.argv[1].split(',') if len(sys.argv) > 1 else []
def names(path):
    b = open(path, 'rb').read(); i = 0; out = []
    while True:
        j = b.index(b'\0', i); n = b[i:j].decode('latin1'); i = j + 1
        meth = struct.unpack_from('<I', b, i)[0]; i += 20
        if n == '' and meth == 0x56657273:
            pref = ''
            while True:
                j = b.index(b'\0', i); k = b[i:j].decode('latin1'); i = j + 1
                if k == '': break
                j = b.index(b'\0', i); v = b[i:j].decode('latin1'); i = j + 1
                if k == 'prefix': pref = v
            out.append(('#prefix', pref)); continue
        if n == '': break
        out.append(('f', n))
    return out
for pbo in sorted(glob.glob(os.path.expanduser('~/dayz/@*/addons/*.pbo'))):
    try: ents = names(pbo)
    except Exception as e: print('ERR', pbo, e); continue
    pref = ''
    mod = pbo.split('/@')[1].split('/')[0]
    for t, n in ents:
        if t == '#prefix': pref = n; continue
        if n.lower().endswith('.p3d'):
            full = (pref + '\\' + n) if pref else n
            if not KEYS or any(k in full.lower() for k in KEYS):
                print(mod + ' | ' + full)
