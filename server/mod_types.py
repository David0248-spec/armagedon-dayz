#!/usr/bin/env python3
# Porządkuje typy z modów w armagedon_ce (w tym types_weapons.xml i types_gear.xml)
import os, re, glob, json

MIS = os.path.expanduser('~/dayz/mpmissions/dayzOffline.chernarusplus')
CE = os.path.join(MIS, 'armagedon_ce')
DZ = os.path.expanduser('~/dayz')
CFG = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'ce', 'economy.json')))
SETN = CFG.get('set_nominal', {})
MOD_CAP = CFG.get('mod_weapon_cap', 8)

def names(text):
    return re.findall(r'<type name="([^"]+)"', text)

vanilla = set(names(open(os.path.join(MIS, 'db/types.xml')).read()))

def block(name, nominal, cat, usages, tiers, lifetime=7200):
    u = ''.join('\t\t<usage name="%s"/>\n' % x for x in usages)
    v = ''.join('\t\t<value name="%s"/>\n' % x for x in tiers)
    return ('\t<type name="%s">\n\t\t<nominal>%d</nominal>\n\t\t<lifetime>%d</lifetime>\n\t\t<restock>600</restock>\n'
            '\t\t<min>%d</min>\n\t\t<quantmin>-1</quantmin>\n\t\t<quantmax>-1</quantmax>\n\t\t<cost>100</cost>\n'
            '\t\t<flags count_in_cargo="0" count_in_hoarder="0" count_in_map="1" count_in_player="0" crafted="0" deloot="0"/>\n'
            '\t\t<category name="%s"/>\n%s%s\t</type>\n') % (name, nominal, lifetime, max(1, nominal // 2), cat, u, v)

for f in glob.glob(os.path.join(CE, 'types_*.xml')):
    if f.endswith(('types_helis.xml', 'types_aga.xml')):
        continue
    s = open(f, errors='ignore').read().replace('\r', '')
    before = len(names(s))

    # Wycinamy duplikaty vanilli
    s = re.sub(r'\s*<type name="([^"]+)">.*?</type>', lambda m: '' if m.group(1) in vanilla else m.group(0), s, flags=re.S)

    # Code Lock
    s = re.sub(r'(<type name="CodeLock">.*?<nominal>)\d+(</nominal>.*?<min>)\d+(</min>)', r'\g<1>80\g<2>40\g<3>', s, flags=re.S)
    s = re.sub(r'(<type name="CodeLock">.*?)(</type>)', lambda m: m.group(1) + ('' if 'usage name="Town"' in m.group(1) else '\t<usage name="Town"/>\n\t') + m.group(2), s, count=1, flags=re.S)

    def _clean_mod(m):
        b = m.group(0)
        n = m.group(1)

        # Nigdy nie blokuj spawnu przez bazy/graczy
        b = b.replace('count_in_hoarder="1"', 'count_in_hoarder="0"').replace('count_in_player="1"', 'count_in_player="0"')

        # Skróć przesadzone lifetime (28800s -> 7200s dla broni, 3600s dla pistoletów)
        if '<category name="weapons"' in b:
            is_pistol = any(p in n for p in ('TT_33', 'Makarov', 'Glock', 'Luger', 'APS', 'PB', 'M92'))
            if is_pistol:
                b = re.sub(r'\s*<usage name="Military"\s*/>', '', b)
                b = re.sub(r'<lifetime>\d+</lifetime>', '<lifetime>3600</lifetime>', b)
            else:
                b = re.sub(r'<lifetime>\d+</lifetime>', '<lifetime>7200</lifetime>', b)

        if n in SETN:
            b = re.sub(r'<nominal>\d+</nominal>', '<nominal>%d</nominal>' % SETN[n][0], b)
            b = re.sub(r'<min>\d+</min>', '<min>%d</min>' % SETN[n][1], b)
            if 'usage name="Military"' not in b:
                b = b.replace('</type>', '\t<usage name="Military"/>\n\t\t<value name="Tier2"/>\n\t\t<value name="Tier3"/>\n\t\t<value name="Tier4"/>\n\t</type>')
            return b

        # Przytnij nadmiarowe bronie z modów (np. G36=22, M16A4=26), żeby nie wypierały AKS-74U/AK-74
        if '<category name="weapons"' in b and not n.startswith(('Mag_', 'Ammo', 'AG_Ammo')):
            nm = re.search(r'<nominal>(\d+)</nominal>', b)
            if nm and int(nm.group(1)) > MOD_CAP:
                b = re.sub(r'<nominal>\d+</nominal>', '<nominal>%d</nominal>' % MOD_CAP, b)
                b = re.sub(r'<min>\d+</min>', '<min>%d</min>' % max(1, MOD_CAP // 2), b)
        return b

    s = re.sub(r'<type name="([^"]+)">.*?</type>', _clean_mod, s, flags=re.S)
    open(f, 'w').write(s)
    print('mod types %s: %d -> %d' % (os.path.basename(f), before, len(names(s))))

# Dutch Army Clothing
dutch = os.path.join(DZ, '@dutcharmyclothing/typesxml/template.xml')
if os.path.exists(dutch):
    out = []
    for n in sorted(set(names(open(dutch, errors='ignore').read()))):
        if n in vanilla:
            continue
        low = n.lower()
        if 'platecarrier' in low or 'ballistic' in low or 'tacticalhelmet' in low:
            out.append(block(n, 2, 'clothes', ['Military'], ['Tier3', 'Tier4']))
        elif 'bag' in low:
            out.append(block(n, 2, 'clothes', ['Military'], ['Tier2', 'Tier3', 'Tier4']))
        else:
            out.append(block(n, 3, 'clothes', ['Military', 'Police'], ['Tier2', 'Tier3', 'Tier4']))
    open(os.path.join(CE, 'types_dutcharmy.xml'), 'w').write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n<types>\n' + ''.join(out) + '</types>\n')
    print('dutch: %d typow' % len(out))

# Uncuepas Civilian Clothing
unc = os.path.join(DZ, '@uncuepascivilianclothing/classes.xml')
if os.path.exists(unc):
    out = []
    for n in sorted(set(names(open(unc, errors='ignore').read()))):
        if n in vanilla:
            continue
        low = n.lower()
        if 'hunt' in low or 'woodland' in low:
            out.append(block(n, 2, 'clothes', ['Hunting', 'Village'], ['Tier1', 'Tier2', 'Tier3']))
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
