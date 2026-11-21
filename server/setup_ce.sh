#!/bin/bash
# Dołącza types.xml z modów do ekonomii (CE) misji + kopiuje klucze modów
set -e
MIS=~/dayz/mpmissions/dayzOffline.chernarusplus
CE=$MIS/armagedon_ce
mkdir -p $CE
rm -f $CE/types_*.xml
find ~/dayz/@*/ -maxdepth 4 -iname 'types*.xml' | grep -v '@armagedon' | while IFS= read -r f; do
  mod=$(echo "$f" | sed 's#.*/dayz/@\([^/]*\)/.*#\1#')
  cp "$f" "$CE/types_${mod}.xml"
  echo "CE: $mod"
done
cp -f ~/dayz/@*/keys/*.bikey ~/dayz/keys/ 2>/dev/null || true
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
