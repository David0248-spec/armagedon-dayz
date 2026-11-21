#!/bin/bash
# Wgrywa mod z repo na serwer i restartuje DayZ
set -e
cd ~/armagedon-dayz
git fetch -q origin
if [ -n "$(git status --porcelain)" ]; then
  git stash push -u -q -m "lokalne-zmiany-$(date +%F_%H%M)" && echo "Lokalne zmiany odlozone: git stash list"
fi
git reset -q --hard origin/main
rm -rf ~/dayz/@ArmaGedonAdmin && cp -r dist/@ArmaGedonAdmin ~/dayz/
mkdir -p ~/dayz/keys && cp -f dist/keys/*.bikey ~/dayz/keys/ 2>/dev/null || true
bash server/setup_ce.sh || true
tmux kill-session -t dayz 2>/dev/null || true
sleep 3
MODS=$(grep -v '^#' server/mods.txt | awk 'NF==2 {print $2}' | while read m; do [ -d ~/dayz/$m ] && printf "%s;" "$m"; done)
MODS="${MODS}@ArmaGedonAdmin"
echo "MODY: $MODS"
tmux new-session -d -s dayz "cd ~/dayz && ./DayZServer -config=serverDZ.cfg -port=2302 -profiles=profiles '-mod=$MODS' -dologs -adminlog -netlog -freezecheck"
# paczka Workshop: wysyłana tylko gdy zmienił się mod
SUM=$(sha1sum dist/@ArmaGedonAdmin/addons/*.pbo | sha1sum | cut -c1-40)
if [ "$SUM" != "$(cat ~/.ag_ws_sum 2>/dev/null)" ]; then
  bash server/publish_workshop.sh > ~/ws_publish.log 2>&1 && echo "$SUM" > ~/.ag_ws_sum && echo "WORKSHOP: zaktualizowano paczke Arma-Gedon"
fi
echo "DEPLOY OK $(git log --oneline -1)"
