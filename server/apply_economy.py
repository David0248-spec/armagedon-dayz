#!/usr/bin/env python3
# Nakłada mnożniki lootu (server/ce/economy.json) na czyste pliki vanilla misji.
import json, os, re, shutil, sys
mis = os.path.expanduser('~/dayz/mpmissions/dayzOffline.chernarusplus')
cfg = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'ce', 'economy.json')))
# --- fazy wipe'a: 1 = tyg. 1-2, 2 = tyg. 3-4, 3 = tyg. 5+ ---
import datetime
_days = (datetime.date.today() - datetime.date.fromisoformat(cfg.get('wipe_start', '2026-11-22'))).days
PHASE = 1 if _days < 14 else (2 if _days < 28 else 3)
RARE = set(cfg.get('phase1_removed', []))
BUILD = cfg.get('build_items', [])
LIFE = cfg.get('lifetime', {})
REMOVED = set(cfg.get('removed', []))
os.makedirs(os.path.expanduser('~/dayz/profiles/ArmaGedonCore'), exist_ok=True)
open(os.path.expanduser('~/dayz/profiles/ArmaGedonCore/phase.txt'), 'w').write(str(PHASE))
print('faza wipe: %d (dzien %d)' % (PHASE, _days + 1))
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
    if name in LIFE:
        nb0 = re.sub(r'<lifetime>\d+</lifetime>', '<lifetime>%d</lifetime>' % LIFE[name], block)
        if nb0 != block: changed += 1
        block = nb0
    mult = 1.0
    cat = re.search(r'<category name="([^"]+)"', block)
    if cat: mult = cfg['category_mult'].get(cat.group(1), 1.0)
    for k, v in cfg['name_mult'].items():
        if k in name: mult = v
    cname = cat.group(1) if cat else ''
    tiers = re.findall(r'<value name="(Tier\d)"', block)
    hi = bool(tiers) and all(t in ('Tier3', 'Tier4') for t in tiers)
    if name in REMOVED:
        changed += 1
        return re.sub(r'<min>\d+</min>', '<min>0</min>', re.sub(r'<nominal>\d+</nominal>', '<nominal>0</nominal>', block))
    if PHASE == 1:
        if name in RARE:
            changed += 1
            return re.sub(r'<min>\d+</min>', '<min>0</min>', re.sub(r'<nominal>\d+</nominal>', '<nominal>0</nominal>', block))
        if cname == 'weapons' and hi: mult *= 0.6
        if cname == 'explosives': mult *= 0.5
    elif PHASE == 3:
        if cname == 'weapons' and hi: mult *= 1.25
        if cname == 'explosives': mult *= 1.5
        if any(k in name for k in BUILD): mult *= 1.3
    if mult == 1.0: return block
    if mult < 1.0 and int((re.search(r'<nominal>(\d+)</nominal>', block) or [0,'0'])[1]) <= 1: return block
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
    pm = 0.85 if PHASE == 1 else (1.3 if PHASE == 3 else 1.0)
    new = int(val) if val >= 2 else max(1, round(nom * val * pm))
    mx = max(int(m.group(4)), new)
    s = s[:m.start()] + m.group(1) + str(new) + m.group(3) + str(mx) + m.group(5) + s[m.end():]
    print('event %s: %d -> %d' % (ev, nom, new))
open(p, 'w').write(s)
