#!/usr/bin/env python3
# Porządkuje typy z modów w armagedon_ce:
# - broń End-Game (MOD_RARE / DELOOT_ONLY) i tłumiki: deloot="1" + Military, czyli TYLKO helicrashe/konwoje CE (nie zwykłe baraki)
# - wyłącza (nominal=0) 5 usuniętych broni: AN94_Suppressed, JD_Groza_762, AG_M76_Zastava, AWM, KS23
# - ogranicza ubrania z modów do 1 szt. i tłumiki pod deloot="1"
import os, re, glob, json

MIS = os.path.expanduser('~/dayz/mpmissions/dayzOffline.chernarusplus')
CE = os.path.join(MIS, 'armagedon_ce')
DZ = os.path.expanduser('~/dayz')
CFG = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'ce', 'economy.json')))
SETN = CFG.get('set_nominal', {})
TSET = CFG.get('tier_set', {})
AU = CFG.get('add_usage', {})
REMOVED = set(CFG.get('removed', []))
DELOOT_ONLY = set(CFG.get('deloot_only', []))
MOD_CAP = CFG.get('mod_weapon_cap', 5)
ITEM_CAP = CFG.get('mod_item_cap', 4)
OPTIC_CAP = CFG.get('mod_optic_cap', 3)
RESTOCK = {k: v for k, v in CFG.get('restock', {}).items() if not k.startswith('_')}
DEF_MIL_T = CFG.get('default_mil_tiers', [])
# Paczki tylko z ubraniami/drobiazgami: max ITEM_CAP szt. na typ (np. 150x HescoBox, 40x klapki)
CAP_FILES = ('types_coldwaruniforms.xml', 'types_windstridesclothingpack.xml')

def finish(b):
    # rzadka bron z modow: deloot=1 i zero <usage> (przy deloot=1 usage = dodatkowa strefa)
    _n = re.search(r'<type name="([^"]+)"', b)
    if _n and _n.group(1) in DELOOT_NO_USE:
        b = b.replace('deloot="0"', 'deloot="1"')
        b = re.sub(r'\s*<usage name="[^"]+"\s*/>', '', b)
    # wspólne zasady dla każdego typu z modów: restock 30 min i tiery dla czysto wojskowych rzeczy
    cat = (re.search(r'<category name="([^"]+)"', b) or [0, ''])[1]
    if RESTOCK:
        rr = re.search(r'<restock>(\d+)</restock>', b)
        lim = RESTOCK.get(cat, RESTOCK.get('default', 1800))
        if rr and int(rr.group(1)) > lim:
            b = b.replace(rr.group(0), '<restock>%d</restock>' % lim, 1)
    # magazynki z paczek modów: losowa ilość amunicji zamiast zawsze pełnych
    if re.match(r'\s*<type name="(Mag_|AGMag_)', b) and '<quantmin>' in b:
        b = re.sub(r'<quantmin>-?\d+</quantmin>', '<quantmin>20</quantmin>', b)
        b = re.sub(r'<quantmax>-?\d+</quantmax>', '<quantmax>90</quantmax>', b)
    if DEF_MIL_T and 'deloot="1"' not in b and not re.search(r'<value name=', b) and re.findall(r'<usage name="([^"]+)"', b) == ['Military']:
        b = b.replace('</type>', ''.join('\t\t<value name="%s"/>\n' % t for t in DEF_MIL_T) + '\t</type>')
    return b

def cap(b, n0):
    nm = re.search(r'<nominal>(\d+)</nominal>', b)
    if nm and int(nm.group(1)) > n0:
        b = re.sub(r'<nominal>\d+</nominal>', '<nominal>%d</nominal>' % n0, b)
        b = re.sub(r'<min>\d+</min>', '<min>%d</min>' % max(1, n0 // 2), b)
    return b

MOD_RARE = set(CFG.get('mod_rare_deloot', [])) | DELOOT_ONLY
DELOOT_NO_USE = set(CFG.get('deloot_no_usage', []))
MOD_CIVIL = set(CFG.get('mod_civil_weapons', []))

def names(text):
    return re.findall(r'<type name="([^"]+)"', text)

vanilla = set(names(open(os.path.join(MIS, 'db/types.xml')).read()))

def block(name, nominal, cat, usages, tiers, lifetime=5400):
    u = ''.join('\t\t<usage name="%s"/>\n' % x for x in usages)
    v = ''.join('\t\t<value name="%s"/>\n' % x for x in tiers)
    return ('\t<type name="%s">\n\t\t<nominal>%d</nominal>\n\t\t<lifetime>%d</lifetime>\n\t\t<restock>%d</restock>\n'
            '\t\t<min>%d</min>\n\t\t<quantmin>-1</quantmin>\n\t\t<quantmax>-1</quantmax>\n\t\t<cost>100</cost>\n'
            '\t\t<flags count_in_cargo="0" count_in_hoarder="0" count_in_map="1" count_in_player="0" crafted="0" deloot="0"/>\n'
            '\t\t<category name="%s"/>\n%s%s\t</type>\n') % (name, nominal, lifetime, RESTOCK.get(cat, RESTOCK.get('default', 1800)), max(1, nominal // 2), cat, u, v)

for f in glob.glob(os.path.join(CE, 'types_*.xml')):
    if f.endswith(('types_helis.xml', 'types_aga.xml')):
        continue
    s = open(f, errors='ignore').read().replace('\r', '')
    before = len(names(s))

    # Wycinamy duplikaty vanilli
    s = re.sub(r'\s*<type name="([^"]+)">.*?</type>', lambda m: '' if m.group(1) in vanilla else m.group(0), s, flags=re.S)

    # Code Lock (głównie budynki przemysłowe Industrial, rzadziej miasta: 25 szt.)
    s = re.sub(r'(<type name="CodeLock">.*?<nominal>)\d+(</nominal>.*?<min>)\d+(</min>)', r'\g<1>25\g<2>12\g<3>', s, flags=re.S)

    def _clean_mod(m):
        # wspólne zasady (restock, tiery) nakładamy na wynik, niezależnie od ścieżki wyjścia
        b = finish(_mod_rules(m))
        # paczki ubraniowe: tną ilości na typ (HescoBox 150 -> ITEM_CAP itd.)
        if os.path.basename(f) in CAP_FILES and '<category name="weapons"' not in b:
            b = cap(b, ITEM_CAP)
        if any(k in m.group(1) for k in ('Optic', 'optic', 'Trijicon')) and 'deloot="1"' not in b:
            b = cap(b, OPTIC_CAP)
        return b

    def _mod_rules(m):
        b = m.group(0)
        n = m.group(1)

        if n in REMOVED:
            b = re.sub(r'<nominal>\d+</nominal>', '<nominal>0</nominal>', b)
            b = re.sub(r'<min>\d+</min>', '<min>0</min>', b)
            return b

        b = b.replace('count_in_hoarder="1"', 'count_in_hoarder="0"').replace('count_in_player="1"', 'count_in_player="0"')

        # Ubrania z paczek modów: max 1 szt. na wariant kolorystyczny
        if '<category name="clothes"' in b:
            nm = re.search(r'<nominal>(\d+)</nominal>', b)
            if nm and int(nm.group(1)) > 1:
                b = re.sub(r'<nominal>\d+</nominal>', '<nominal>1</nominal>', b)
                b = re.sub(r'<min>\d+</min>', '<min>1</min>', b)
            return b

        # Tłumiki z modów: wyłącznie deloot="1" (helicrashe/konwoje + eventy skryptowe), zero zwykłych baraków
        if 'Suppressor' in n or 'suppressor' in n:
            b = b.replace('deloot="0"', 'deloot="1"')
            b = re.sub(r'\s*<usage name="[^"]+"\s*/>', '', b)
            if n in AU:
                tags = ''.join('\t\t<usage name="%s"/>\n' % u for u in AU[n].get('usage', []))
                b = b.replace('</type>', tags + '\t</type>')
            b = re.sub(r'<nominal>\d+</nominal>', '<nominal>2</nominal>', b)
            b = re.sub(r'<min>\d+</min>', '<min>1</min>', b)
            b = re.sub(r'<lifetime>\d+</lifetime>', '<lifetime>10800</lifetime>', b)
            return b

        # Włączamy wyzerowane wcześniej dodatki i optykę z paczek modów
        if any(k in n for k in ('Optic', 'optic', 'Trijicon', 'Bipod', 'Bttstck', 'Hndgrd', 'Light', 'Wrap')):
            nm = re.search(r'<nominal>(\d+)</nominal>', b)
            if nm and int(nm.group(1)) == 0:
                b = re.sub(r'<nominal>\d+</nominal>', '<nominal>3</nominal>', b)
                b = re.sub(r'<min>\d+</min>', '<min>1</min>', b)

        # Obsługa 34 aktywnych broni z modów
        is_weapon_item = '<category name="weapons"' in b and not n.startswith(('Mag_', 'Ammo', 'AG_Ammo', 'Ammobox', 'AmmoBox', 'AGMag')) and not any(k in n for k in ('Optic', 'optic', 'Trijicon', 'Bttstck', 'Hndgrd', 'Mount', 'Light', 'Bipod', 'Suppressor', 'suppressor', 'Wrap', 'Grenade'))
        if is_weapon_item:
            if n in MOD_RARE:
                b = b.replace('deloot="0"', 'deloot="1"')
                b = re.sub(r'\s*<usage name="[^"]+"\s*/>', '', b)
                b = re.sub(r'<lifetime>\d+</lifetime>', '<lifetime>10800</lifetime>', b)
            elif n in MOD_CIVIL:
                b = re.sub(r'\s*<usage name="Military"\s*/>', '', b)
                pass  # cywilna bron z modow: lifetime jak w paczce moda
            # pozostale bronie z modow: lifetime bez zmian

        if n in AU:
            if n not in MOD_RARE:
                b = b.replace('deloot="1"', 'deloot="0"')
            tags = ''.join('\t\t<usage name="%s"/>\n' % u for u in AU[n].get('usage', []) if 'usage name="%s"' % u not in b) + \
                   ''.join('\t\t<value name="%s"/>\n' % t for t in AU[n].get('tiers', []) if 'value name="%s"' % t not in b)
            b = b.replace('</type>', tags + '\t</type>')

        if n in TSET:
            vals = ''.join('\t\t<value name="%s"/>\n' % t for t in TSET[n])
            b = re.sub(r'\s*<value name="Tier\d"\s*/>', '', b)
            b = b.replace('</type>', vals + '\t</type>')

        if n in SETN:
            b = re.sub(r'<nominal>\d+</nominal>', '<nominal>%d</nominal>' % SETN[n][0], b)
            b = re.sub(r'<min>\d+</min>', '<min>%d</min>' % SETN[n][1], b)
            if is_weapon_item and n not in MOD_RARE and n not in MOD_CIVIL and 'usage name="Military"' not in b and 'usage name="Police"' not in b:
                b = b.replace('</type>', '\t\t<usage name="Military"/>\n\t</type>')
            return b

        if is_weapon_item:
            nm = re.search(r'<nominal>(\d+)</nominal>', b)
            if nm and int(nm.group(1)) > MOD_CAP:
                b = re.sub(r'<nominal>\d+</nominal>', '<nominal>%d</nominal>' % MOD_CAP, b)
                b = re.sub(r'<min>\d+</min>', '<min>%d</min>' % max(1, MOD_CAP // 2), b)
        return b

    s = re.sub(r'<type name="([^"]+)">.*?</type>', _clean_mod, s, flags=re.S)
    open(f, 'w').write(s)
    print('mod types %s: %d -> %d' % (os.path.basename(f), before, len(names(s))))

# Dutch Army Clothing (po 1 szt. na wariant)
dutch = os.path.join(DZ, '@dutcharmyclothing/typesxml/template.xml')
if os.path.exists(dutch):
    out = []
    for n in sorted(set(names(open(dutch, errors='ignore').read()))):
        if n in vanilla:
            continue
        low = n.lower()
        if 'platecarrier' in low or 'ballistic' in low or 'tacticalhelmet' in low:
            out.append(block(n, 1, 'clothes', ['Military'], ['Tier3', 'Tier4']))
        elif 'bag' in low:
            out.append(block(n, 1, 'clothes', ['Military'], ['Tier2', 'Tier3', 'Tier4']))
        else:
            out.append(block(n, 1, 'clothes', ['Military', 'Police'], ['Tier2', 'Tier3', 'Tier4']))
    open(os.path.join(CE, 'types_dutcharmy.xml'), 'w').write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n<types>\n' + ''.join(out) + '</types>\n')
    print('dutch: %d typow' % len(out))

# Uncuepas Civilian Clothing (po 1 szt. na wariant)
unc = os.path.join(DZ, '@uncuepascivilianclothing/classes.xml')
if os.path.exists(unc):
    out = []
    for n in sorted(set(names(open(unc, errors='ignore').read()))):
        if n in vanilla:
            continue
        low = n.lower()
        if 'hunt' in low or 'woodland' in low:
            out.append(block(n, 1, 'clothes', ['Hunting', 'Village'], ['Tier1', 'Tier2', 'Tier3']))
        elif 'firefight' in low:
            out.append(block(n, 1, 'clothes', ['Firefighter'], ['Tier1', 'Tier2', 'Tier3']))
        elif 'surgical' in low or 'medic' in low:
            out.append(block(n, 1, 'clothes', ['Medic'], ['Tier1', 'Tier2', 'Tier3']))
        elif 'gorka' in low or 'holster' in low or 'jungle' in low:
            out.append(block(n, 1, 'clothes', ['Military', 'Police'], ['Tier2', 'Tier3']))
        else:
            out.append(block(n, 1, 'clothes', ['Town', 'Village'], ['Tier1', 'Tier2', 'Tier3']))
    open(os.path.join(CE, 'types_uncuepas.xml'), 'w').write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n<types>\n' + ''.join(out) + '</types>\n')
    print('uncuepas: %d typow' % len(out))
