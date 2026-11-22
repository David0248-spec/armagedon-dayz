#!/bin/bash
# ArmaGedonWeapons - paczka broni uzyta za zgoda administracji zrodlowej paczki.
# Pobiera paczke zrodlowa z Workshopu (bez ladowania jej na serwerze) i dokleja 3 pliki z bronia do @ArmaGedonCore jako ArmaGedonWeapons_*.pbo.
set -e
SRC_ID=2864245850
DST=~/dayz/@ArmaGedonCore
WS=~/.local/share/Steam/steamapps/workshop/content/221100
[ -d ~/Steam/steamapps/workshop/content/221100 ] && WS=~/Steam/steamapps/workshop/content/221100
if [ ! -d "$WS/$SRC_ID" ] || [ "$1" = "update" ]; then
  /usr/games/steamcmd +login dawidxjar +workshop_download_item 221100 $SRC_ID +quit | tail -2
  [ -d ~/Steam/steamapps/workshop/content/221100/$SRC_ID ] && WS=~/Steam/steamapps/workshop/content/221100
fi
mkdir -p $DST/addons $DST/keys
# u nas pliki nazywaja sie ArmaGedonWeapons_*
rm -f $DST/addons/kk.pbo* $DST/addons/vbb_weapons.pbo* $DST/addons/ca_weapons.pbo*
for pair in kk.pbo:armagedonweapons_core.pbo vbb_weapons.pbo:armagedonweapons_vbb.pbo ca_weapons.pbo:armagedonweapons_ca.pbo; do
  p=${pair%%:*}; n=${pair##*:}
  f=$(find "$WS/$SRC_ID" -maxdepth 2 -type f -iname "$p" | head -1)
  [ -z "$f" ] && { echo "BRAK $p"; exit 1; }
  cp -f "$f" "$DST/addons/$n"
  s=$(ls "$f".*.bisign 2>/dev/null | head -1); [ -n "$s" ] && cp -f "$s" "$DST/addons/$n.armagedonweapons.bisign"
  echo "ArmaGedonWeapons: $n $(du -h "$DST/addons/$n" | cut -f1)"
done
k=$(find "$WS/$SRC_ID" -iname '*.bikey' | head -1); [ -n "$k" ] && cp -f "$k" $DST/keys/ArmaGedonWeapons.bikey && cp -f "$k" ~/dayz/keys/ArmaGedonWeapons.bikey || true
