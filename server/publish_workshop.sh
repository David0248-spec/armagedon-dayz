#!/bin/bash
# Wysyła paczkę Arma-Gedon (prywatną) na Steam Workshop i aktualizuje ją przy kolejnych wywołaniach
set -e
REPO=~/armagedon-dayz
STAGE=~/ag_workshop
IDFILE=~/.ag_workshop_id
rm -rf $STAGE && mkdir -p $STAGE/addons
cp $REPO/dist/@ArmaGedonAdmin/addons/*.pbo $STAGE/addons/
cp $REPO/workshop/mod.cpp $STAGE/
ID=0
[ -f $IDFILE ] && ID=$(cat $IDFILE)
[ -f $REPO/workshop/item_id ] && ID=$(cat $REPO/workshop/item_id)
cat > /tmp/ag_workshop.vdf <<VDF
"workshopitem"
{
 "appid" "221100"
 "publishedfileid" "$ID"
 "contentfolder" "$STAGE"
 "previewfile" "$REPO/workshop/preview.png"
 "visibility" "2"
 "title" "Arma-Gedon"
 "description" "Prywatna paczka serwera ARMA-GEDON (panel administracyjny i zawartosc serwera)."
 "changenote" "$(cd $REPO && git log --oneline -1)"
}
VDF
/usr/games/steamcmd +login dawidxjar +workshop_build_item /tmp/ag_workshop.vdf +quit | tee /tmp/ag_workshop.log | tail -5
NEW=$(grep -o 'publishedfileid" "[0-9]*' /tmp/ag_workshop.vdf | grep -o '[0-9]*$')
[ "$NEW" != "0" ] && echo "$NEW" > $IDFILE
echo "WORKSHOP ID: $NEW"
