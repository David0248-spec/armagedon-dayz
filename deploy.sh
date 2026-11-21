#!/bin/bash
# Wgrywa mod z repo na serwer i restartuje DayZ (uruchamiane ręcznie)
set -e
cd ~/armagedon-dayz && git pull -q
rm -rf ~/dayz/@ArmaGedonAdmin && cp -r dist/@ArmaGedonAdmin ~/dayz/
tmux kill-session -t dayz 2>/dev/null || true
sleep 3
tmux new-session -d -s dayz "cd ~/dayz && ./DayZServer -config=serverDZ.cfg -port=2302 -profiles=profiles -mod=@ArmaGedonAdmin -dologs -adminlog -netlog -freezecheck"
echo "DEPLOY OK $(git log --oneline -1)"
