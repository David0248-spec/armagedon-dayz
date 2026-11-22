#!/bin/bash
# Dołącza types.xml z modów do ekonomii (CE) misji + kopiuje klucze modów
set -e
python3 ~/armagedon-dayz/server/apply_economy.py
python3 ~/armagedon-dayz/server/gen_nobuild.py
MIS=~/dayz/mpmissions/dayzOffline.chernarusplus
CE=$MIS/armagedon_ce
mkdir -p $CE
rm -f $CE/types_*.xml
find ~/dayz/@*/ -maxdepth 4 -type f -iname 'types*.xml' | grep -v '@armagedon' | grep -v '@mmgbasestorage' | while IFS= read -r f; do
  mod=$(echo "$f" | sed 's#.*/dayz/@\([^/]*\)/.*#\1#')
  cp "$f" "$CE/types_${mod}.xml"
  echo "CE: $mod"
done
cp -f ~/dayz/@*/keys/*.bikey ~/dayz/keys/ 2>/dev/null || true
python3 ~/armagedon-dayz/server/mod_types.py
cp -f ~/armagedon-dayz/server/ce/types_helis.xml $CE/types_helis.xml
cp -f ~/armagedon-dayz/server/ce/types_aga.xml $CE/types_aga.xml
# wpis w cfgeconomycore.xml (odtwarzany za każdym razem)
python3 - "$MIS" <<'PY'
import sys, os, re, glob
mis = sys.argv[1]
p = os.path.join(mis, 'cfgeconomycore.xml')
s = open(p).read()
s = re.sub(r'\s*<ce folder="armagedon_ce">.*?</ce>', '', s, flags=re.S)
files = sorted(os.path.basename(f) for f in glob.glob(os.path.join(mis, 'armagedon_ce', 'types_*.xml')))
block = '\n\t<ce folder="armagedon_ce">\n' + ''.join(f'\t\t<file name="{f}" type="types" />\n' for f in files) + '\t</ce>\n'
s = s.replace('</economycore>', block + '</economycore>')
open(p, 'w').write(s)
print('cfgeconomycore: %d plikow typow modow' % len(files))
PY
ls ~/dayz/keys
# --- helikoptery Expansion: typy + event + miejsca spawnu + części ---
cp -f ~/armagedon-dayz/server/ce/types_helis.xml $CE/types_helis.xml
python3 - "$MIS" ~/armagedon-dayz/server/ce <<'PY'
import sys, os, re
mis, src = sys.argv[1], sys.argv[2]
def patch(fname, block_file, closing):
    p = os.path.join(mis, fname)
    s = open(p).read()
    s = re.sub(r'\s*<!-- ARMAGEDON-START -->.*?<!-- ARMAGEDON-END -->', '', s, flags=re.S)
    block = open(os.path.join(src, block_file)).read()
    s = s.replace(closing, '\t<!-- ARMAGEDON-START -->\n' + block + '\t<!-- ARMAGEDON-END -->\n' + closing)
    open(p, 'w').write(s)
    print('patched', fname)
patch('db/events.xml', 'events_helis.xml', '</events>')
patch('cfgeventspawns.xml', 'eventspawns_helis.xml', '</eventposdef>')
patch('cfgspawnabletypes.xml', 'spawnabletypes_helis.xml', '</spawnabletypes>')
# typy helikopterów do listy plików CE
p = os.path.join(mis, 'cfgeconomycore.xml')
s = open(p).read()
if 'types_helis.xml' not in s:
    s = s.replace('\t</ce>\n</economycore>', '\t\t<file name="types_helis.xml" type="types" />\n\t</ce>\n</economycore>')
    open(p, 'w').write(s)
print('helis ok')
PY
