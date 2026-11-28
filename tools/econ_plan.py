#!/usr/bin/env python3
"""Generator ekonomii Vanilla+ pod 115 graczy - liczony, nie na wyczucie.

Dla KAZDEGO typu lootu (vanilla + wszystkie mody) liczy gorny limit nominalu tak,
zeby statystycznie nie powstawaly skupiska (3+ sztuki tego samego typu w jednym
budynku). Limit bierze sie z liczby budynkow w strefach danego typu, odczytanej
z mapgroupproto.xml / mapgrouppos.xml - czyli z realnej mapy, nie z zalozen.

    limit = budynki_w_strefach * tier_udzial * CLUSTER_TARGET

Nastepnie kazdy typ jest podnoszony pod 115 graczy mnoznikiem swojej kategorii,
ale NIGDY ponad ten limit. Efekt: wiecej lootu niz w vanilli (ktora jest robiona
pod 60 graczy), a mimo to bez serii tego samego przedmiotu w jednym miejscu.

Rzadkosc broni (bardzo rzadkie / rzadkie / zwykle) bierze sie z listy w
economy.json: deloot_no_usage = tylko eventy, rare_tier4 = tylko Tier 4 itd.

Uzycie:
  python3 tools/econ_plan.py <katalog_misji> [--apply] [--csv plik]

Bez --apply tylko pokazuje plan. Z --apply zapisuje wyliczone wartosci do
server/ce/economy.json (sekcja set_nominal), zeby apply_economy.py je nalozyl.
"""
import json
import math
import os
import re
import sys

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ECON = os.path.join(HERE, 'server', 'ce', 'economy.json')

# docelowa liczba sztuk na budynek: ponizej tego progu skupiska sa rzadkie
MAX_CLUSTERS = 0.8   # ile budynkow na mapie moze miec 3+ szt. tego samego typu

# mnozniki pod 115 graczy (vanilla jest liczona pod ~60)
PLAYER_MULT = {
    'food': 1.45,         # jedzenie i napoje - najszybciej sie zuzywa
    'weapons': 1.25,      # bronie, magazynki, amunicja
    'tools': 1.2,         # narzedzia, czesci, materialy
    'clothes': 1.1,       # ubrania i plecaki
    'containers': 1.15,   # torby, skrzynki, pojemniki
    'explosives': 1.0,    # granaty i miny - jak w vanilli
    'lootdispatch': 1.2,  # czesci pojazdow
    'none': 1.2,          # butelki, pakunki NBC
}

ZONES = ('Military', 'Police', 'Medic', 'Firefighter', 'Industrial', 'Farm',
         'Town', 'Village', 'Hunting', 'Coast', 'Prison', 'Office', 'School',
         'ContaminatedArea', 'SeasonalEvent')


def poisson_tail(lam, k):
    """P(X >= k) - szansa na k+ sztuk w jednym budynku."""
    if lam <= 0:
        return 0.0
    acc, term = 0.0, math.exp(-lam)
    for i in range(k):
        if i:
            term *= lam / i
        acc += term
    return max(0.0, 1.0 - acc)


def read(path):
    try:
        return open(path, encoding='utf-8', errors='ignore').read()
    except OSError:
        return ''


def load_types(mis):
    import glob
    base = os.path.join(mis, 'db', 'types.xml')
    if os.path.exists(base + '.vanilla'):
        base += '.vanilla'   # oryginal Bohemii, nie wynik poprzedniego przebiegu
    files = [base]
    files += sorted(glob.glob(os.path.join(mis, 'armagedon_ce', 'types_*.xml')))
    out = {}
    for f in files:
        s = read(f)
        for m in re.finditer(r'<type name="([^"]+)">(.*?)</type>', s, re.S):
            name, body = m.group(1), m.group(2)
            num = lambda t: int((re.search(r'<%s>(-?\d+)</%s>' % (t, t), body) or [0, '0'])[1])
            out[name] = {
                'nominal': num('nominal'), 'min': num('min'),
                'cat': (re.search(r'<category name="([^"]+)"', body) or [0, 'none'])[1],
                'usage': [u for u in re.findall(r'<usage name="([^"]+)"', body) if u in ZONES],
                'tier': re.findall(r'<value name="Tier(\d)"', body),
                'deloot': 'deloot="1"' in body,
                'mod': 'armagedon_ce' in f,
            }
    return out


def load_buildings(mis):
    proto = read(os.path.join(mis, 'mapgroupproto.xml'))
    pos = read(os.path.join(mis, 'mapgrouppos.xml'))
    if not proto or not pos:
        return None
    grp = {}
    for m in re.finditer(r'<group name="([^"]+)"[^>]*>(.*?)</group>', proto, re.S):
        grp[m.group(1)] = set(re.findall(r'<usage name="([^"]+)"', m.group(2)))
    bud = {}
    for m in re.finditer(r'<group name="([^"]+)"', pos):
        for u in grp.get(m.group(1), ()):
            bud[u] = bud.get(u, 0) + 1
    return bud


def safe_cap(t, name, bud):
    """Gorny limit nominalu dla typu.

    Limit liczymy tak, zeby na CALEJ mapie bylo srednio najwyzej MAX_CLUSTERS
    budynkow z 3+ sztukami tego samego typu. Prog musi zalezec od wielkosci
    strefy: przy 4900 budynkach nawet 0.2 szt./budynek daje 5 skupisk, a przy
    40 budynkach to samo 0.2 daje 0.05 - czyli praktycznie zero.
    """
    if not t['usage']:
        return None
    b = max(bud.get(u, 0) for u in t['usage'])
    if b <= 0:
        return None
    tiers = len(t['tier']) if t['tier'] else 2   # brak tieru = ostroznie, jak T3-4
    b_eff = max(1.0, b * tiers / 4.0)
    limit = MAX_CLUSTERS
    if name.startswith(('Ammo_', 'AG_Ammo_')):
        limit = MAX_CLUSTERS * 0.5          # amunicja luzem: ostrozniej
    # szukamy najwiekszej lambdy, przy ktorej liczba skupisk <= limit
    lam = 0.02
    while lam < 1.2 and poisson_tail(lam + 0.01, 3) * b_eff <= limit:
        lam += 0.01
    return max(1, int(round(b_eff * lam)))


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if not args:
        sys.exit('uzycie: econ_plan.py <katalog_misji> [--apply] [--csv plik]')
    mis = args[0]
    types = load_types(mis)
    bud = load_buildings(mis)
    if not types or not bud:
        sys.exit('BLAD: brak types.xml albo mapgroupproto/mapgrouppos w %s' % mis)

    cfg = json.load(open(ECON, encoding='utf-8'))
    tier_set = cfg.get('tier_set', {})
    def_mil = cfg.get('default_mil_tiers', [])
    for n, t in types.items():
        if n in tier_set:
            t['tier'] = [x[-1] for x in tier_set[n]]
        elif not t['tier'] and def_mil and t['usage'] == ['Military']:
            t['tier'] = [x[-1] for x in def_mil]
    global MOD_GUN_CAP, MOD_OPTIC_CAP, MOD_ITEM_CAP
    MOD_GUN_CAP = cfg.get('mod_weapon_cap', 5)
    MOD_OPTIC_CAP = cfg.get('mod_optic_cap', 3)
    MOD_ITEM_CAP = cfg.get('mod_item_cap', 4)
    rare_event = set(cfg.get('deloot_no_usage', []))
    removed = set(cfg.get('removed', []))

    plan, stats = {}, {'podniesione': 0, 'przyciete': 0, 'bez_zmian': 0}
    rows = []
    for name, t in sorted(types.items()):
        if name in removed or t['nominal'] <= 0:
            continue
        if t['deloot'] or name in rare_event:
            continue  # loot wylacznie z eventow - nie dotyczy budynkow
        cap = safe_cap(t, name, bud)
        if cap is None:
            continue
        mult = PLAYER_MULT.get(t['cat'], 1.3)
        want = int(round(t['nominal'] * mult))
        new = min(want, cap, max(1, t['nominal'] * 2))
        if t['mod']:
            # przedmioty z paczek modowych: te same limity co w mod_types.py,
            # inaczej bron modowa bylaby liczniejsza niz vanillowa
            is_optic = any(k in name for k in ('Optic', 'optic', 'Trijicon', 'Sight',
                                               'Bipod', 'Bttstck', 'Hndgrd', 'Light', 'Wrap'))
            is_gun = (t['cat'] == 'weapons' and not is_optic
                      and not name.startswith(('Mag_', 'AGMag_', 'Ammo', 'AG_Ammo',
                                               'AmmoBox', 'Ammobox', 'AG_AmmoBox', 'AG_Ammobox')))
            if is_gun:
                new = min(new, MOD_GUN_CAP)
            elif is_optic:
                new = min(new, MOD_OPTIC_CAP)
            elif t['cat'] in ('clothes', 'containers'):
                new = min(new, MOD_ITEM_CAP)
        if new < 1:
            new = 1
        if new > t['nominal']:
            stats['podniesione'] += 1
        elif new < t['nominal']:
            stats['przyciete'] += 1
        else:
            stats['bez_zmian'] += 1
        plan[name] = [new, max(1, int(new * 0.3))]
        rows.append((name, t['cat'], t['nominal'], want, cap, new,
                     ','.join(t['usage']), ''.join(t['tier']) or '1234', t['mod']))

    print('Typow w ekonomii: %d | zaplanowanych: %d' % (len(types), len(plan)))
    print('  podniesione pod 115 graczy: %d' % stats['podniesione'])
    print('  przyciete (limit skupisk):  %d' % stats['przyciete'])
    print('  bez zmian:                  %d' % stats['bez_zmian'])
    print()
    cut = [r for r in rows if r[5] < r[3]]
    cut.sort(key=lambda r: r[3] - r[5], reverse=True)
    print('NAJMOCNIEJ OGRANICZONE PRZEZ LIMIT SKUPISK (chcielismy wiecej, ale strefa mala):')
    print('%-26s %-11s %6s %6s %6s  %s' % ('typ', 'kategoria', 'teraz', 'chcemy', 'limit', 'strefy'))
    for r in cut[:18]:
        print('%-26s %-11s %6d %6d %6d  %s' % (r[0], r[1], r[2], r[3], r[4], r[6][:26]))
    print()
    up = [r for r in rows if r[5] > r[2]]
    up.sort(key=lambda r: r[5] - r[2], reverse=True)
    print('NAJMOCNIEJ PODNIESIONE (duza strefa, jest miejsce):')
    for r in up[:12]:
        print('%-26s %-11s %6d -> %-6d strefy %s' % (r[0], r[1], r[2], r[5], r[6][:26]))

    if '--csv' in sys.argv:
        path = sys.argv[sys.argv.index('--csv') + 1]
        with open(path, 'w', encoding='utf-8') as fh:
            fh.write('typ;kategoria;teraz;chcemy;limit_skupisk;nowy;strefy;tier;z_moda\n')
            for r in rows:
                fh.write('%s;%s;%d;%d;%d;%d;%s;%s;%s\n' % (
                    r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 'tak' if r[8] else 'nie'))
        print('\nCSV: %s (%d typow)' % (path, len(rows)))

    if '--apply' in sys.argv:
        old = cfg.get('set_nominal', {})
        keep = {n: v for n, v in old.items()
                if v[0] == 0 or (n in types and types[n]['mod']) or n not in types}
        keep.update(plan)
        cfg['set_nominal'] = keep
        cfg['_plan'] = ('nominal wyliczony przez tools/econ_plan.py pod 115 graczy; '
                        'limit liczony tak, by na mapie bylo najwyzej %.1f budynku z 3+ szt. '
                        'tego samego typu' % MAX_CLUSTERS)
        out = json.dumps(cfg, ensure_ascii=False, indent=2)
        out = re.sub(r'\[\s+([^\[\]{}]*?)\s+\]',
                     lambda m: '[' + re.sub(r'\s*\n\s*', ' ', m.group(1)) + ']', out)
        open(ECON, 'w', encoding='utf-8', newline='\n').write(out + '\n')
        print('\nZAPISANO do %s: %d typow w set_nominal' % (ECON, len(keep)))


if __name__ == '__main__':
    main()
