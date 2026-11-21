#!/bin/bash
# Wgrywa mod z repo na serwer i restartuje DayZ
set -e
cd ~/armagedon-dayz
git fetch -q origin
if [ -n "$(git status --porcelain)" ]; then
  git stash push -u -q -m "lokalne-zmiany-$(date +%F_%H%M)" && echo "Lokalne zmiany odlozone: git stash list"
fi
git reset -q --hard origin/main
[ -d ~/dayz/profiles/ArmaGedonAdmin ] && [ ! -d ~/dayz/profiles/ArmaGedonCore ] && mv ~/dayz/profiles/ArmaGedonAdmin ~/dayz/profiles/ArmaGedonCore || true
rm -rf ~/dayz/@ArmaGedonAdmin ~/dayz/@ArmaGedonCore && cp -r dist/@ArmaGedonCore ~/dayz/
mkdir -p ~/dayz/keys && cp -f dist/keys/*.bikey ~/dayz/keys/ 2>/dev/null || true
bash server/setup_ce.sh || true
# VPP Admin Tools: superadmini
SA=~/dayz/profiles/VPPAdminTools/Permissions/SuperAdmins; mkdir -p $SA
while read -r id; do [ -n "$id" ] && ! grep -q "$id" $SA/SuperAdmins.txt 2>/dev/null && echo "$id" >> $SA/SuperAdmins.txt; done < server/superadmins.txt
tmux kill-session -t dayz 2>/dev/null || true
sleep 3
MODS=$(grep -v '^#' server/mods.txt | awk 'NF==2 {print $2}' | while read m; do [ -d ~/dayz/$m ] && printf "%s;" "$m"; done)
MODS="${MODS}@ArmaGedonCore"
echo "MODY: $MODS"
sed -i 's/^maxPlayers.*/maxPlayers = 100;/' ~/dayz/serverDZ.cfg
# VPP: dostęp tylko po SteamID z SuperAdmins (bez hasła) - na życzenie właściciela
grep -q '^vppDisablePassword' ~/dayz/serverDZ.cfg || echo 'vppDisablePassword = 1;' >> ~/dayz/serverDZ.cfg
tmux new-session -d -s dayz "cd ~/dayz && while true; do ./DayZServer -config=serverDZ.cfg -port=2302 -profiles=profiles '-mod=$MODS' -dologs -adminlog -netlog -freezecheck; echo RESTART; sleep 10; done"
# paczka Workshop: wysyłana tylko gdy zmienił się mod
SUM=$(sha1sum dist/@ArmaGedonCore/addons/*.pbo | sha1sum | cut -c1-40)
if [ "$SUM" != "$(cat ~/.ag_ws_sum 2>/dev/null)" ]; then
  bash server/publish_workshop.sh > ~/ws_publish.log 2>&1 && echo "$SUM" > ~/.ag_ws_sum && echo "WORKSHOP: zaktualizowano paczke Arma-Gedon"
fi
echo "DEPLOY OK $(git log --oneline -1)"
