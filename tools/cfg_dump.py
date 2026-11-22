#!/usr/bin/env python3
"""Wypisuje fragment config.bin z PBO: cfg_dump.py plik.pbo NazwaKlasy [linie]"""
import sys, os, subprocess, tempfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import importlib.util, struct
pbo, cls = sys.argv[1], sys.argv[2]
n = int(sys.argv[3]) if len(sys.argv) > 3 else 120
here = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('pc', os.path.join(here, 'pbo_cat.py'))
b = open(pbo, 'rb').read(); i = 0; ents = []
def rd(b, i):
    j = b.index(b'\0', i); return b[i:j].decode('latin1'), j + 1
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
src = open(os.path.join(here, 'pbo_cat.py')).read()
lz = {}; exec(src.split('b = open(sys.argv[1]')[0], lz)
for name, size, orig, meth in ents:
    d = b[i:i + size]; i += size
    if name.lower().endswith('config.bin'):
        if meth == 0x43707273 or (orig and orig != size): d = lz['lzss'](d, orig)
        tf = tempfile.NamedTemporaryFile(delete=False); tf.write(d); tf.close()
        txt = subprocess.run([sys.executable, '-I', os.path.join(here, 'derap.py'), tf.name], capture_output=True, text=True, errors='replace').stdout.split('\n')
        os.unlink(tf.name)
        for k, line in enumerate(txt):
            if line.strip().startswith('class ' + cls) and '{' in line:
                print('=====', name); print('\n'.join(txt[k:k + n]))
