#!/usr/bin/env python3
"""Audyt lootu: wykrywa typy, ktore BEDA sie respic w skupiskach - bez testowania w grze.

Jak to dziala:
  1. czyta types.xml misji + wszystkie armagedon_ce/types_*.xml
  2. czyta mapgroupproto.xml (ile punktow lootu ma dany budynek i jakie ma strefy)
     oraz mapgrouppos.xml (ile takich budynkow stoi na mapie)
  3. dla kazdego typu liczy, ile sztuk statystycznie wypadnie w JEDNYM budynku
     (lambda = nominal / liczba budynkow w jego strefach)
  4. z rozkladu Poissona liczy szanse, ze w ktorymkolwiek budynku na mapie
     znajda sie 3+ sztuki tego samego typu - to wlasnie "skupisko"

Uzycie:
  python3 tools/loot_audit.py [katalog_misji] [--csv plik] [--top N] [--all]

Domyslnie: ~/dayz/mpmissions/dayzOffline.chernarusplus
"""
import glob
import math
import os
import re
import sys

ZONES = ('Military', 'Police', 'Medic', 'Firefighter', 'Industrial', 'Farm',
         'Town', 'Village', 'Hunting', 'Coast', 'Prison', 'Office', 'School',
         'ContaminatedArea', 'SeasonalEvent')


def read(path):
    try:
        return open(path, encoding='utf-8', errors='ignore').read()
    except OSError:
        return ''


def load_types(mis):
    """Scalone typy: najpierw types.xml misji, potem pliki CE (modowe nadpisuja)."""
    files = [os.path.join(mis, 'db', 'types.xml')]
    files += sorted(glob.glob(os.path.join(mis, 'armagedon_ce', 'types_*.xml')))
    out = {}
    for f in files:
        s = read(f)
        for m in re.finditer(r'<type name="([^"]+)">(.*?)</type>', s, re.S):
            name, body = m.group(1), m.group(2)
            num = lambda t: int((re.search(r'<%s>(-?\d+)</%s>' % (t, t), body) or [0, '0'])[1])
            out[name] = {
                'nominal': num('nominal'),
                'min': num('min'),
                'restock': num('restock'),
                'lifetime': num('lifetime'),
                'cat': (re.search(r'<category name="([^"]+)"', body) or [0, '-'])[1],
                'usage': re.findall(r'<usage name="([^"]+)"', body),
                'tier': re.findall(r'<value name="Tier(\d)"', body),
                'deloot': 'deloot="1"' in body,
                'file': os.path.basename(f),
            }
    return out


def load_buildings(mis):
    """{strefa: (liczba_budynkow, liczba_punktow_lootu)} z mapgroupproto + mapgrouppos."""
    proto = read(os.path.join(mis, 'mapgroupproto.xml'))
    pos = read(os.path.join(mis, 'mapgrouppos.xml'))
    if not proto or not pos:
        return None
    grp = {}
    for m in re.finditer(r'<group name="([^"]+)"[^>]*>(.*?)</group>', proto, re.S):
        body = m.group(2)
        grp[m.group(1)] = (set(re.findall(r'<usage name="([^"]+)"', body)),
                           len(re.findall(r'<point ', body)))
    bud, slots = {}, {}
    for m in re.finditer(r'<group name="([^"]+)"', pos):
        uses, pts = grp.get(m.group(1), (set(), 0))
        for u in uses:
            bud[u] = bud.get(u, 0) + 1
            slots[u] = slots.get(u, 0) + pts
    return {u: (bud[u], slots.get(u, 0)) for u in bud}


def poisson_tail(lam, k):
    """P(X >= k) dla rozkladu Poissona - szansa na k+ sztuk w jednym budynku."""
    if lam <= 0:
        return 0.0
    acc, term = 0.0, math.exp(-lam)
    for i in range(k):
        if i:
            term *= lam / i
        acc += term
    return max(0.0, 1.0 - acc)


def audit(types, zones):
    rows = []
    for name, t in types.items():
        if t['nominal'] <= 0 or t['deloot']:
            continue  # wylaczone i loot wylacznie z eventow nas tu nie dotyczy
        uses = [u for u in t['usage'] if u in zones]
        if not uses:
            continue  # brak strefy = loot z eventow/pojazdow, nie z budynkow
        # budynki moga naleze do kilku stref naraz, wiec bierzemy najwieksza pule
        b = max(zones[u][0] for u in uses)
        # tiery zawezaja pule: typ na 2 z 4 tierow ma ok. polowy budynkow
        tiers = len(t['tier']) if t['tier'] else 4
        b_eff = max(1.0, b * tiers / 4.0)
        lam = t['nominal'] / b_eff
        rows.append({
            'name': name, 'cat': t['cat'], 'nominal': t['nominal'], 'min': t['min'],
            'zones': ','.join(uses), 'tier': ''.join(t['tier']) or '1234',
            'buildings': int(b_eff), 'per_building': lam,
            'p3': poisson_tail(lam, 3), 'p2': poisson_tail(lam, 2),
            'expect3': poisson_tail(lam, 3) * b_eff, 'file': t['file'],
        })
    rows.sort(key=lambda r: -r['expect3'])
    return rows


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    mis = args[0] if args else os.path.expanduser('~/dayz/mpmissions/dayzOffline.chernarusplus')
    top = 40
    if '--top' in sys.argv:
        top = int(sys.argv[sys.argv.index('--top') + 1])
    if '--all' in sys.argv:
        top = 10 ** 9

    types = load_types(mis)
    if not types:
        sys.exit('BLAD: nie znalazlem types.xml w %s' % mis)
    zones = load_buildings(mis)
    if not zones:
        sys.exit('BLAD: brak mapgroupproto.xml / mapgrouppos.xml w %s' % mis)

    rows = audit(types, zones)
    print('Typow w ekonomii: %d | aktywnych w budynkach: %d' % (len(types), len(rows)))
    print()
    print('BUDYNKI NA MAPIE (ile miejsc ma dana strefa):')
    for u, (b, s) in sorted(zones.items(), key=lambda x: -x[1][0]):
        print('   %-16s budynkow %5d   punktow lootu %6d' % (u, b, s))
    print()
    print('=' * 108)
    print('RYZYKO SKUPISK - ile budynkow na mapie dostanie 3+ sztuki tego samego typu')
    print('=' * 108)
    print('%-28s %7s %6s %9s %7s  %-22s %s' % (
        'typ', 'nominal', 'bud.', 'na budynek', '3+ szt.', 'strefy', 'ryzyko'))
    for r in rows[:top]:
        if r['expect3'] < 0.05 and top < 10 ** 9:
            break
        risk = 'OK'
        if r['expect3'] >= 3:
            risk = 'BARDZO WYSOKIE'
        elif r['expect3'] >= 1:
            risk = 'WYSOKIE'
        elif r['expect3'] >= 0.3:
            risk = 'srednie'
        print('%-28s %7d %6d %9.3f %7.2f  %-22s %s' % (
            r['name'], r['nominal'], r['buildings'], r['per_building'],
            r['expect3'], r['zones'][:22], risk))

    bad = [r for r in rows if r['expect3'] >= 1]
    print()
    print('PODSUMOWANIE: %d typow z wysokim ryzykiem skupisk (3+ szt. w budynku).' % len(bad))
    if bad:
        print('Proponowany nominal, zeby zejsc do bezpiecznego poziomu:')
        for r in bad[:25]:
            safe = max(1, int(r['buildings'] * 0.45))
            print('   %-26s %4d -> %4d' % (r['name'], r['nominal'], safe))

    if '--csv' in sys.argv:
        path = sys.argv[sys.argv.index('--csv') + 1]
        with open(path, 'w', encoding='utf-8') as fh:
            fh.write('typ;kategoria;nominal;min;budynki;na_budynek;szansa_3plus;'
                     'budynkow_z_3plus;strefy;tier;plik\n')
            for r in rows:
                fh.write('%s;%s;%d;%d;%d;%.4f;%.4f;%.3f;%s;%s;%s\n' % (
                    r['name'], r['cat'], r['nominal'], r['min'], r['buildings'],
                    r['per_building'], r['p3'], r['expect3'], r['zones'], r['tier'], r['file']))
        print('\nCSV: %s (%d typow)' % (path, len(rows)))


if __name__ == '__main__':
    main()
