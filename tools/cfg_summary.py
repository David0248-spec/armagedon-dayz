#!/usr/bin/env python3
"""Krótkie podsumowanie klas CfgVehicles z PBO: nazwa < rodzic | scope | displayName | cargo | attachments | model"""
import sys, os, re, subprocess, tempfile, struct
here = os.path.dirname(os.path.abspath(__file__))
pbo = sys.argv[1]
flt = sys.argv[2].lower() if len(sys.argv) > 2 else ''
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
lz = {}; exec(open(os.path.join(here, 'pbo_cat.py')).read().split('b = open(sys.argv[1]')[0], lz)
txt = None
for name, size, orig, meth in ents:
    d = b[i:i + size]; i += size
    if name.lower().endswith('config.bin'):
        if meth == 0x43707273 or (orig and orig != size): d = lz['lzss'](d, orig)
        tf = tempfile.NamedTemporaryFile(delete=False); tf.write(d); tf.close()
        txt = subprocess.run([sys.executable, '-I', os.path.join(here, 'derap.py'), tf.name], capture_output=True, text=True, errors='replace').stdout
        os.unlink(tf.name)
        break
if txt is None:
    for name, size, orig, meth in ents:
        pass
    print('brak config.bin'); sys.exit()
lines = txt.split('\n')
# klasy na poziomie 2 (wewnątrz CfgVehicles)
cur = None; depth = 0; info = {}; order = []; in_veh = False
for ln in lines:
    s = ln.strip()
    m = re.match(r'class (\w+)(?:\s*:\s*(\w+))?\s*\{', s)
    if m:
        if depth == 0: in_veh = (m.group(1) == 'CfgVehicles')
        if in_veh and depth == 1:
            cur = m.group(1); info[cur] = {'p': m.group(2) or ''}; order.append(cur)
        depth += 1
        continue
    if s.startswith('};'):
        depth -= 1
        if depth == 1: cur = None
        continue
    if cur and depth == 2:
        mm = re.match(r'(\w+)(\[\])?\s*=\s*(.*);$', s)
        if mm:
            k = mm.group(1).lower()
            if k in ('scope', 'displayname', 'itemscargosize', 'attachments', 'model', 'weight', 'itemsize', 'projectiontypename'):
                info[cur][k] = mm.group(3)
for c in order:
    d = info[c]
    if flt and flt not in c.lower() and flt not in d.get('displayname', '').lower(): continue
    if d.get('scope', '') != '2': continue
    mdl = d.get('model', '').split('\\')[-1]
    print(f"{c}<{d['p']} | {d.get('displayname','')} | c={d.get('itemscargosize','')} | a={d.get('attachments','')[:90]} | {mdl}")
