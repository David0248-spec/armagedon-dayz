#!/bin/bash
# Wysyła paczkę Arma-Gedon (prywatną) na Steam Workshop i aktualizuje ją przy kolejnych wywołaniach
set -e
REPO=~/armagedon-dayz
STAGE=~/ag_workshop
IDFILE=~/.ag_workshop_id
rm -rf $STAGE && mkdir -p $STAGE/addons
cp ~/dayz/@ArmaGedonCore/addons/*.pbo $STAGE/addons/
cp ~/dayz/@ArmaGedonCore/addons/*.bisign $STAGE/addons/ 2>/dev/null || true
mkdir -p $STAGE/keys && cp ~/dayz/@ArmaGedonCore/keys/*.bikey $STAGE/keys/ 2>/dev/null || true
cp $REPO/workshop/mod.cpp $STAGE/
ID=0
[ -f $IDFILE ] && ID=$(cat $IDFILE)
[ -f $REPO/workshop/item_id ] && ID=$(cat $REPO/workshop/item_id)
# widocznosc ustawiana tylko przy tworzeniu nowej paczki; aktualizacje zostawiaja ustawienie wlasciciela
VIS=""
[ -z "$ID" ] && VIS=' "visibility" "2"'
cat > /tmp/ag_workshop.vdf <<VDF
"workshopitem"
{
 "appid" "221100"
 "publishedfileid" "$ID"
 "contentfolder" "$STAGE"
 "previewfile" "$REPO/workshop/preview.png"
$VIS
 "title" "Arma-Gedon"
 "description" "Paczka serwera ARMA-GEDON (DayZ)."
 "changenote" "$(cd $REPO && git log --oneline -1)"
}
VDF
/usr/games/steamcmd +login dawidxjar +workshop_build_item /tmp/ag_workshop.vdf +quit | tee /tmp/ag_workshop.log | tail -5
NEW=$(grep -o 'publishedfileid" "[0-9]*' /tmp/ag_workshop.vdf | grep -o '[0-9]*$')
[ "$NEW" != "0" ] && echo "$NEW" > $IDFILE
echo "WORKSHOP ID: $NEW"
