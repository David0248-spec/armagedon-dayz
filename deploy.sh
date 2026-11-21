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
tmux kill-session -t dayz 2>/dev/null || true
sleep 3
tmux new-session -d -s dayz "cd ~/dayz && ./DayZServer -config=serverDZ.cfg -port=2302 -profiles=profiles -mod=@ArmaGedonAdmin -dologs -adminlog -netlog -freezecheck"
echo "DEPLOY OK $(git log --oneline -1)"
