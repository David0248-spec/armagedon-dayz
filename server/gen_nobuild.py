#!/usr/bin/env python3
# Strefy zakazu budowy: bazy wojskowe, szpitale, posterunki policji, zamki (z mapgrouppos.xml misji).
import re, os, json, math
mis = os.path.expanduser('~/dayz/mpmissions/dayzOffline.chernarusplus')
out = os.path.expanduser('~/dayz/profiles/ArmaGedonCore/nobuild.json')
src = open(os.path.join(mis, 'mapgrouppos.xml'), errors='replace').read()
KINDS = [
    ('military', 45, re.compile(r'^Land_(Mil_|Tisy_|Airfield_|Prison_|Guardhouse|Mil$)', re.I)),
    ('medical', 40, re.compile(r'^Land_(City_Hospital|Village_HealthCare|Medical|Hospital)', re.I)),
    ('police', 40, re.compile(r'^Land_(City_PoliceStation|Village_PoliceStation|Police)', re.I)),
    ('castle', 60, re.compile(r'^Land_Castle_', re.I)),
]
zones = []
for m in re.finditer(r'<group name="([^"]+)" pos="([-\d.]+) ([-\d.]+) ([-\d.]+)"', src):
    name, x, z = m.group(1), float(m.group(2)), float(m.group(4))
    for kind, r, rx in KINDS:
        if rx.search(name):
            # pomijamy budynki leżące w już istniejącej strefie (mniej punktów)
            if any(zz['K'] == kind and math.hypot(zz['X'] - x, zz['Z'] - z) < r * 0.5 for zz in zones):
                break
            zones.append({'X': round(x, 1), 'Z': round(z, 1), 'R': r, 'K': kind})
            break
os.makedirs(os.path.dirname(out), exist_ok=True)
json.dump({'Zones': zones}, open(out, 'w'))
cnt = {}
for zz in zones: cnt[zz['K']] = cnt.get(zz['K'], 0) + 1
print('strefy zakazu budowy:', len(zones), cnt)
