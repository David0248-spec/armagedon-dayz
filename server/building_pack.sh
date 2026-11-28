#!/bin/bash
# ArmaGedonBase - budowanie bazy i meble (paczki uzyte za zgoda administracji zrodlowej paczki).
#   building_pack.sh build    -> pobiera zrodlo (raz), buduje armagedonbase_*.pbo do ~/.agb_cache
#   building_pack.sh install  -> kopiuje zbudowane paczki do @ArmaGedonCore/addons
#   building_pack.sh update   -> wymusza ponowne pobranie zrodla i przebudowe
# Bez tych paczek ArmaGedonCore sie nie skompiluje, wiec deploy przerywa sie przy bledzie.
set -e
SRC_ID=3421213293
REPO=~/armagedon-dayz
DST=${AGB_DST:-$HOME/dayz/@ArmaGedonCore}
CACHE=~/.agb_cache
MODE=${1:-build}

find_src() {
  for ws in ~/.local/share/Steam/steamapps/workshop/content/221100 ~/Steam/steamapps/workshop/content/221100; do
    for sub in Addons addons; do
      [ -f "$ws/$SRC_ID/$sub/tR_Dependence.pbo" ] && { echo "$ws/$SRC_ID/$sub"; return; }
    done
  done
}

if [ "$MODE" = "build" ] || [ "$MODE" = "update" ]; then
  SRC=$(find_src)
  if [ -z "$SRC" ] || [ "$MODE" = "update" ]; then
    echo "ArmaGedonBase: pobieranie paczki zrodlowej (duza, tylko raz)..."
    /usr/games/steamcmd +login dawidxjar +workshop_download_item 221100 $SRC_ID validate +quit
    SRC=$(find_src)
  fi
  [ -z "$SRC" ] && { echo "BLAD: brak paczki zrodlowej $SRC_ID"; exit 1; }
  mkdir -p $CACHE
  STAMP=$CACHE/.built
  NEED=0
  [ -f "$STAMP" ] || NEED=1
  [ "$MODE" = "update" ] && NEED=1
  [ $NEED = 0 ] && [ -n "$(find "$SRC" -maxdepth 1 -name '*.pbo' -newer "$STAMP" | head -1)" ] && NEED=1
  [ $NEED = 0 ] && [ $REPO/tools/agb_build.py -nt "$STAMP" ] && NEED=1
  if [ $NEED = 1 ]; then
    rm -rf $CACHE/new && mkdir -p $CACHE/new
    python3 -I $REPO/tools/agb_build.py "$SRC" $CACHE/new
    rm -f $CACHE/*.pbo && mv $CACHE/new/*.pbo $CACHE/ && rm -rf $CACHE/new
    touch $STAMP
  fi
  ls $CACHE/armagedonbase_core.pbo >/dev/null
  echo "ArmaGedonBase: gotowe ($(ls $CACHE/*.pbo | wc -l) paczek)"
fi

if [ "$MODE" = "install" ] || [ "$MODE" = "update" ]; then
  ls $CACHE/armagedonbase_core.pbo >/dev/null
  mkdir -p $DST/addons
  rm -f $DST/addons/armagedonbase_*.pbo
  cp -f $CACHE/armagedonbase_*.pbo $DST/addons/
  echo "ArmaGedonBase: zainstalowano w $DST"
fi
