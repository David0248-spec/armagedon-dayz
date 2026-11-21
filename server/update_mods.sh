#!/bin/bash
# Pobiera/aktualizuje mody z Workshopu i kopiuje je do ~/dayz (nazwy plików małymi literami - wymóg serwera Linux)
set -e
REPO=~/armagedon-dayz
DAYZ=~/dayz
WS=~/.local/share/Steam/steamapps/workshop/content/221100
[ -d "$WS" ] || WS=~/Steam/steamapps/workshop/content/221100
ARGS=""
while read -r id name; do
  [[ -z "$id" || "$id" == \#* ]] && continue
  ARGS="$ARGS +workshop_download_item 221100 $id"
done < $REPO/server/mods.txt
/usr/games/steamcmd +login dawidxjar $ARGS +quit
[ -d ~/.local/share/Steam/steamapps/workshop/content/221100 ] && WS=~/.local/share/Steam/steamapps/workshop/content/221100
[ -d ~/Steam/steamapps/workshop/content/221100 ] && WS=~/Steam/steamapps/workshop/content/221100
mkdir -p $DAYZ/keys
while read -r id name; do
  [[ -z "$id" || "$id" == \#* ]] && continue
  if [ ! -d "$WS/$id" ]; then echo "BRAK: $name ($id)"; continue; fi
  rm -rf "$DAYZ/$name"
  cp -r "$WS/$id" "$DAYZ/$name"
  # małe litery w nazwach (od najgłębszych)
  find "$DAYZ/$name" -depth -name '*[A-Z]*' | while read -r f; do
    d=$(dirname "$f"); b=$(basename "$f"); l=$(echo "$b" | tr 'A-Z' 'a-z')
    [ "$b" != "$l" ] && mv -f "$f" "$d/$l"
  done
  cp -f "$DAYZ/$name"/keys/*.bikey $DAYZ/keys/ 2>/dev/null || true
  echo "OK: $name"
done < $REPO/server/mods.txt
