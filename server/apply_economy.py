#!/usr/bin/env python3
# Nakłada mnożniki lootu (server/ce/economy.json) na czyste pliki vanilla misji.
import json, os, re, shutil, sys
mis = os.path.expanduser('~/dayz/mpmissions/dayzOffline.chernarusplus')
cfg = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'ce', 'economy.json')))
def pristine(rel):
    p = os.path.join(mis, rel); b = p + '.vanilla'
    if not os.path.exists(b): shutil.copy(p, b)
    return p, open(b).read()
# --- types.xml ---
p, s = pristine('db/types.xml')
changed = 0
def fix_type(m):
    global changed
    block = m.group(0); name = m.group(1)
    mult = 1.0
    cat = re.search(r'<category name="([^"]+)"', block)
    if cat: mult = cfg['category_mult'].get(cat.group(1), 1.0)
    for k, v in cfg['name_mult'].items():
        if k in name: mult = max(mult, v)
    if mult == 1.0: return block
    def scale(tag, b):
        mm = re.search(r'<%s>(\d+)</%s>' % (tag, tag), b)
        if not mm or int(mm.group(1)) == 0: return b
        return b.replace(mm.group(0), '<%s>%d</%s>' % (tag, max(1, round(int(mm.group(1)) * mult)), tag), 1)
    nb = scale('min', scale('nominal', block))
    if nb != block: changed += 1
    return nb
s = re.sub(r'<type name="([^"]+)">.*?</type>', fix_type, s, flags=re.S)
open(p, 'w').write(s)
print('types.xml: zmieniono %d typow' % changed)
# --- events.xml (vanilla + nasze bloki ARMAGEDON doklejane potem przez setup_ce) ---
p, s = pristine('db/events.xml')
for ev, val in cfg['event_nominal'].items():
    m = re.search(r'(<event name="%s">.*?<nominal>)(\d+)(</nominal>.*?<max>)(\d+)(</max>)' % re.escape(ev), s, flags=re.S)
    if not m: print('brak eventu', ev); continue
    nom = int(m.group(2))
    new = int(val) if val >= 2 else max(1, round(nom * val))
    mx = max(int(m.group(4)), new)
    s = s[:m.start()] + m.group(1) + str(new) + m.group(3) + str(mx) + m.group(5) + s[m.end():]
    print('event %s: %d -> %d' % (ev, nom, new))
open(p, 'w').write(s)
