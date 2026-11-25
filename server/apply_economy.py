#!/usr/bin/env python3
# Nakłada mnożniki lootu (server/ce/economy.json) na czyste pliki vanilla misji (stały balans bez faz wipe).
import json, os, re, shutil

mis = os.path.expanduser('~/dayz/mpmissions/dayzOffline.chernarusplus')
cfg = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'ce', 'economy.json')))

BUILD = cfg.get('build_items', [])
LIFE = cfg.get('lifetime', {})
REMOVED = set(cfg.get('removed', []))
STRIP_MIL = set(cfg.get('strip_military', []))
DELOOT_ONLY = set(cfg.get('deloot_only', []))
RESTOCK = {k: v for k, v in cfg.get('restock', {}).items() if not k.startswith('_')}
DEF_MIL_T = cfg.get('default_mil_tiers', [])

os.makedirs(os.path.expanduser('~/dayz/profiles/ArmaGedonCore'), exist_ok=True)
open(os.path.expanduser('~/dayz/profiles/ArmaGedonCore/phase.txt'), 'w').write('2')
print('ekonomia: staly balans Vanilla+ (fazy wipe wylaczone)')

def pristine(rel):
    p = os.path.join(mis, rel)
    b = p + '.vanilla'
    if not os.path.exists(b):
        shutil.copy(p, b)
    return p, open(b).read()

# --- types.xml ---
p, s = pristine('db/types.xml')
changed = 0

SORTED_NAME_MULT = sorted(cfg.get('name_mult', {}).items(), key=lambda x: len(x[0]), reverse=True)
SETN = cfg.get('set_nominal', {})
TSET = cfg.get('tier_set', {})
AU = cfg.get('add_usage', {})

def fix_type(m):
    global changed
    block = m.group(0)
    name = m.group(1)

    cat = re.search(r'<category name="([^"]+)"', block)
    cname = cat.group(1) if cat else ''

    if name in REMOVED:
        changed += 1
        return re.sub(r'<min>\d+</min>', '<min>0</min>', re.sub(r'<nominal>\d+</nominal>', '<nominal>0</nominal>', block))

    if cname in ('weapons', 'explosives', 'clothes', 'tools') or any(k in name for k in BUILD):
        block = block.replace('count_in_hoarder="1"', 'count_in_hoarder="0"')
        block = block.replace('count_in_player="1"', 'count_in_player="0"')

    # KRYTYCZNE: dla DELOOT_ONLY ustawiamy deloot="1" i wycinamy wszystkie stare <usage> (np. Military),
    # aby broń/dodatki NIE respiły się w zwykłych barakach (zostaje tylko deloot + ewentualne ContaminatedArea).
    if name in DELOOT_ONLY:
        block = block.replace('deloot="0"', 'deloot="1"')
        block = re.sub(r'\s*<usage name="[^"]+"\s*/>', '', block)

    if name in STRIP_MIL:
        block = re.sub(r'\s*<usage name="Military"\s*/>', '', block)

    # Czasy zycia (lifetime) zostawiamy jak w vanilli: skracanie ich powodowalo, ze przedmioty
    # znikaly szybciej niz CE nadazalo dosypywac - budynki byly puste i bez urozmaicenia.
    # Skracamy tylko to, co jawnie wymieniono w "lifetime" w economy.json.
    if name in LIFE:
        block = re.sub(r'<lifetime>\d+</lifetime>', '<lifetime>%d</lifetime>' % LIFE[name], block)
    elif not cfg.get('keep_vanilla_lifetime'):
        if cname == 'weapons':
            lt = 10800 if name in DELOOT_ONLY else (3600 if name in STRIP_MIL else 7200)
            block = re.sub(r'<lifetime>\d+</lifetime>', '<lifetime>%d</lifetime>' % lt, block)
        elif cname == 'food':
            block = re.sub(r'<lifetime>\d+</lifetime>', '<lifetime>4800</lifetime>', block)
        elif cname in ('tools', 'clothes'):
            block = re.sub(r'<lifetime>\d+</lifetime>', '<lifetime>5400</lifetime>', block)

    mult = cfg['category_mult'].get(cname, 1.0)
    for k, v in SORTED_NAME_MULT:
        if k in name:
            mult = v
            break

    MQ = cfg.get('mag_quant', {})
    if name.startswith(('Mag_', 'AGMag_')) and '<quantmin>' in block:
        block = re.sub(r'<quantmin>-?\d+</quantmin>', '<quantmin>%d</quantmin>' % MQ.get('min', 15), block)
        block = re.sub(r'<quantmax>-?\d+</quantmax>', '<quantmax>%d</quantmax>' % MQ.get('max', 85), block)
    elif (cname == 'food' or name.startswith(('Tetracycline', 'Charcoal', 'Multivitamin', 'Painkiller', 'Disinfectant'))) and '<quantmin>' in block:
        block = re.sub(r'<quantmin>-?\d+</quantmin>', '<quantmin>35</quantmin>', block)
        block = re.sub(r'<quantmax>-?\d+</quantmax>', '<quantmax>100</quantmax>', block)

    # Maksymalny czas odnowienia: restock dłuższy niż limit (30 min, jedzenie 15 min) jest skracany.
    # restock 0 = CE dosypuje od razu po spadku poniżej <min> - tego NIE podnosimy (inaczej mapa stoi pusta po resecie).
    if RESTOCK:
        rr = re.search(r'<restock>(\d+)</restock>', block)
        lim = RESTOCK.get(cname, RESTOCK.get('default', 1800))
        if rr and int(rr.group(1)) > lim:
            block = block.replace(rr.group(0), '<restock>%d</restock>' % lim, 1)

    # Rzeczy wyłącznie wojskowe bez tieru respiły się też na wybrzeżu (T1) - dostają domyślne tiery
    if DEF_MIL_T and cname in ('weapons', 'clothes', 'explosives', 'tools', 'containers') and not re.search(r'<value name=', block):
        us = re.findall(r'<usage name="([^"]+)"', block)
        if 'Military' in us:
            block = block.replace('</type>', ''.join('\t\t<value name="%s"/>\n' % t for t in DEF_MIL_T) + '\t</type>')

    if name in AU:
        if name not in DELOOT_ONLY:
            block = block.replace('deloot="1"', 'deloot="0"')
        if 'usage' in AU[name]:
            block = re.sub(r'\s*<usage name="[^"]+"\s*/>', '', block) if name in ('Mosin9130', 'CZ527', 'CZ550', 'Winchester70', 'SKS') else block
        tags = ''.join('\t\t<usage name="%s"/>\n' % u for u in AU[name].get('usage', []) if 'usage name="%s"' % u not in block) + \
               ''.join('\t\t<value name="%s"/>\n' % t for t in AU[name].get('tiers', []) if 'value name="%s"' % t not in block)
        block = block.replace('</type>', tags + '\t</type>')
        changed += 1

    if name in TSET:
        vals = ''.join('<value name="%s"/>' % t for t in TSET[name])
        b2 = re.sub(r'\s*<value name="Tier\d"\s*/>', '', block)
        um = list(re.finditer(r'<usage name="[^"]+"\s*/>', b2))
        if um:
            k = um[-1].end()
            b2 = b2[:k] + '\n\t\t' + vals + b2[k:]
            block = b2
            changed += 1

    if name in SETN:
        n0, m0 = SETN[name]
        block = re.sub(r'<nominal>\d+</nominal>', '<nominal>%d</nominal>' % n0, block)
        block = re.sub(r'<min>\d+</min>', '<min>%d</min>' % m0, block)
        changed += 1
        return block

    if mult == 1.0:
        return block
    if mult < 1.0 and int((re.search(r'<nominal>(\d+)</nominal>', block) or [0, '0'])[1]) <= 1:
        return block

    def scale(tag, b):
        mm = re.search(r'<%s>(\d+)</%s>' % (tag, tag), b)
        if not mm or int(mm.group(1)) == 0:
            return b
        return b.replace(mm.group(0), '<%s>%d</%s>' % (tag, max(1, round(int(mm.group(1)) * mult)), tag), 1)

    nb = scale('min', scale('nominal', block))
    if nb != block:
        changed += 1
    return nb

s = re.sub(r'<type name="([^"]+)">.*?</type>', fix_type, s, flags=re.S)

# Warianty kolorystyczne i kolbowe broni (max 1-2 szt. na wariant, dziedziczą strefy broni bazowej)
if cfg.get('weapon_variants'):
    base = {m.group(1): m.group(0) for m in re.finditer(r'<type name="([^"]+)">.*?</type>', s, flags=re.S)}
    def var_fix(m):
        global changed
        name, block = m.group(1), m.group(0)
        mm = re.match(r'^([A-Za-z0-9]+)_(Black|Green|Camo|Tan|Woodland|Wood|Olive|Desert|Grey|Snow|White|Brown|Coyote|Bttstck)$', name)
        if not mm or mm.group(1) not in base or '<category name="weapons"' not in block:
            return block
        bb = base[mm.group(1)]
        bn = int(re.search(r'<nominal>(\d+)</nominal>', bb).group(1))
        if bn == 0:
            return block
        n0 = max(1, min(2, bn // 6))
        nb = re.sub(r'<nominal>\d+</nominal>', '<nominal>%d</nominal>' % n0, block)
        nb = re.sub(r'<min>\d+</min>', '<min>1</min>', nb)
        nb = nb.replace('count_in_hoarder="1"', 'count_in_hoarder="0"').replace('count_in_player="1"', 'count_in_player="0"')
        rb = re.search(r'<restock>\d+</restock>', bb)
        if rb:
            nb = re.sub(r'<restock>\d+</restock>', rb.group(0), nb, count=1)
        if 'deloot="1"' in bb:
            nb = nb.replace('deloot="0"', 'deloot="1"')
        nb = re.sub(r'\s*<(usage|value) name="[^"]+"\s*/>', '', nb)
        tags = ''.join('\n\t\t' + t for t in re.findall(r'<(?:usage|value) name="[^"]+"\s*/>', bb))
        nb = nb.replace('</type>', tags.lstrip('\n') + '\n\t</type>') if tags else nb
        changed += 1
        return nb
    s = re.sub(r'<type name="([^"]+)">.*?</type>', var_fix, s, flags=re.S)

open(p, 'w').write(s)
print('types.xml: zmieniono %d typow' % changed)

# --- events.xml ---
p, s = pristine('db/events.xml')
for ev, val in cfg['event_nominal'].items():
    m = re.search(r'(<event name="%s">.*?<nominal>)(\d+)(</nominal>.*?<max>)(\d+)(</max>)' % re.escape(ev), s, flags=re.S)
    if not m:
        print('brak eventu', ev)
        continue
    nom = int(m.group(2))
    new = int(val) if val >= 2 else max(1, round(nom * val))
    mx = max(int(m.group(4)), new)
    s = s[:m.start()] + m.group(1) + str(new) + m.group(3) + str(mx) + m.group(5) + s[m.end():]
    print('event %s: %d -> %d' % (ev, nom, new))
open(p, 'w').write(s)

# --- cfgspawnabletypes.xml ---
p, s = pristine('cfgspawnabletypes.xml')
ch = cfg.get('spawn_mag_chance', 0.4)
added = 0
for w, mag in cfg.get('weapon_mags', {}).items():
    if not mag:
        continue
    m = re.search(r'<type name="%s">(.*?)</type>' % re.escape(w), s, flags=re.S)
    att = '\t\t<attachments chance="%.2f">\n\t\t\t<item name="%s" chance="1.00" />\n\t\t</attachments>\n' % (ch, mag)
    if m:
        body = m.group(1)
        if 'Mag_' in body:
            nb = re.sub(r'<attachments chance="[\d.]+">(\s*<item name="Mag_)', lambda q: '<attachments chance="%.2f">%s' % (ch, q.group(1)), body)
            if nb == body:
                continue
            s = s[:m.start(1)] + nb + s[m.end(1):]
        else:
            s = s[:m.end() - len('</type>')] + att + '\t</type>' + s[m.end():]
    else:
        s = s.replace('</spawnabletypes>', '\t<type name="%s">\n%s\t</type>\n</spawnabletypes>' % (w, att))
    added += 1

sc = cfg.get('stock_chance')
if sc:
    s = re.sub(r'<attachments chance="[\d.]+">(\s*<item name="[^"]*(?:Bttstck|Hndgrd|Buttstock|Handguard)[^"]*")', lambda q: '<attachments chance="%.2f">%s' % (sc, q.group(1)), s)
open(p, 'w').write(s)
print('cfgspawnabletypes: magazynki dodane do %d broni' % added)

# --- globals.xml ---
p, s = pristine('db/globals.xml')
for k, v in cfg.get('globals', {}).items():
    s = re.sub(r'(<var name="%s" type="\d+" value=")[^"]*(")' % k, r'\g<1>%s\2' % v, s)
open(p, 'w').write(s)
print('globals.xml: %d ustawien' % len(cfg.get('globals', {})))
