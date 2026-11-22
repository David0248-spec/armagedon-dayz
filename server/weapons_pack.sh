#!/bin/bash
# ArmaGedonWeapons + ArmaGedonGear - paczki uzyte za zgoda administracji zrodlowej paczki.
# Pobiera paczke zrodlowa z Workshopu (bez ladowania jej na serwerze), przenosi bronie pod nasze nazwy (AG_*)
# i dokleja pliki do @ArmaGedonCore jako armagedonweapons_*.pbo / armagedongear_*.pbo.
set -e
SRC_ID=2864245850
DST=${AGW_DST:-$HOME/dayz/@ArmaGedonCore}
REPO=~/armagedon-dayz
WS=~/.local/share/Steam/steamapps/workshop/content/221100
[ -d ~/Steam/steamapps/workshop/content/221100/$SRC_ID ] && WS=~/Steam/steamapps/workshop/content/221100
if [ ! -d "$WS/$SRC_ID" ] || [ "$1" = "update" ]; then
  /usr/games/steamcmd +login dawidxjar +workshop_download_item 221100 $SRC_ID +quit | tail -2
  [ -d ~/Steam/steamapps/workshop/content/221100/$SRC_ID ] && WS=~/Steam/steamapps/workshop/content/221100
fi
mkdir -p $DST/addons
rm -f $DST/addons/kk.pbo* $DST/addons/vbb_weapons.pbo* $DST/addons/ca_weapons.pbo* $DST/addons/armagedonweapons_* $DST/addons/armagedongear_*; [ -z "$AGW_DST" ] && rm -f $DST/keys/ArmaGedonWeapons.bikey ~/dayz/keys/ArmaGedonWeapons.bikey || true
CACHE=~/.agw_cache; mkdir -p $CACHE
for pair in kk.pbo:armagedonweapons_core.pbo:1 vbb_weapons.pbo:armagedonweapons_vbb.pbo:1 ca_weapons.pbo:armagedonweapons_ca.pbo:1 spur_bagz.pbo:armagedongear_bags.pbo:0 validol_retextures.pbo:armagedongear_clothes.pbo:0 windstride_retex.pbo:armagedongear_colors.pbo:0; do
  IFS=: read p n rb <<< "$pair"
  f=$(find "$WS/$SRC_ID" -maxdepth 2 -type f -iname "$p" | head -1)
  [ -z "$f" ] && { echo "BRAK $p"; exit 1; }
  if [ "$rb" = "1" ]; then
    # przebudowa tylko gdy zrodlo sie zmienilo
    if [ ! -f "$CACHE/$n" ] || [ "$f" -nt "$CACHE/$n" ] || [ $REPO/tools/agw_rebrand.py -nt "$CACHE/$n" ]; then
      python3 $REPO/tools/agw_rebrand.py "$f" "$CACHE/$n"
    fi
    cp -f "$CACHE/$n" "$DST/addons/$n"
  else
    cp -f "$f" "$DST/addons/$n"
  fi
  echo "ArmaGedon: $n $(du -h "$DST/addons/$n" | cut -f1)"
done
