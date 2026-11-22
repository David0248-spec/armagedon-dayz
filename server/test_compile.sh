#!/bin/bash
# Sprawdza kompilację nowej wersji moda na osobnej, chwilowej instancji (bez restartu głównego serwera).
set -u
T=/tmp/agtest
rm -rf $T; mkdir -p $T/prof
MIS=~/dayz/mpmissions/dayzOffline.chernarusplus
rsync -a --exclude storage_1 "$MIS/" $T/mis/
rm -rf ~/dayz/@AGTest; mkdir -p ~/dayz/@AGTest/addons
cp ~/armagedon-dayz/dist/@ArmaGedonCore/addons/*.pbo ~/dayz/@AGTest/addons/
MODS=$(grep -v '^#' ~/armagedon-dayz/server/mods.txt | grep -v '^\s*$' | awk '{print $2}' | paste -sd';')
cd ~/dayz
timeout 70 ./DayZServer -config=serverDZ.cfg -port=2502 -profiles=$T/prof -mission=$T/mis "-mod=$MODS;@AGTest" >/dev/null 2>&1 &
sleep 65
L=$(ls -t $T/prof/script*.log 2>/dev/null | head -1)
echo "LOG: $L"
grep -c "SCRIPT    (E)" "$L"
grep -A3 "SCRIPT    (E)" "$L" | head -12
grep -i "config.cpp" $T/prof/*.RPT 2>/dev/null | head -5
pkill -f "port=2502" 2>/dev/null
rm -rf ~/dayz/@AGTest
