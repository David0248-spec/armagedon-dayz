#!/bin/bash
# Paczka broni (KK.pbo, VBB_Weapons.pbo, ca_weapons.pbo) - uzyta za zgoda administracji zrodlowej paczki.
# Pobiera paczke zrodlowa z Workshopu (bez ladowania jej na serwerze) i dokleja 3 pliki z bronia do @ArmaGedonCore.
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
for p in kk.pbo vbb_weapons.pbo ca_weapons.pbo; do
  f=$(find "$WS/$SRC_ID" -maxdepth 2 -type f -iname "$p" | head -1)
  [ -z "$f" ] && { echo "BRAK $p"; exit 1; }
  cp -f "$f" "$DST/addons/$p"
  for s in "$f".*.bisign; do [ -f "$s" ] && cp -f "$s" "$DST/addons/$p.$(basename "$s" | sed 's/^.*\.pbo\.//' | tr 'A-Z' 'a-z')"; done
  echo "BRON: $p $(du -h "$DST/addons/$p" | cut -f1)"
done
find "$WS/$SRC_ID" -iname '*.bikey' -exec cp -f {} $DST/keys/ \; -exec cp -f {} ~/dayz/keys/ \; 2>/dev/null || true
