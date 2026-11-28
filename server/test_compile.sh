#!/bin/bash
# Sprawdza kompilację nowej wersji moda na osobnej, chwilowej instancji (bez restartu głównego serwera).
set -u
T=/tmp/agtest
rm -rf $T; mkdir -p $T/prof
SECS=${AGT_SECS:-150}
[ -n "${AGT_EVTEST:-}" ] && mkdir -p $T/prof/ArmaGedonCore && touch $T/prof/ArmaGedonCore/evtest.txt
MIS=~/dayz/mpmissions/dayzOffline.chernarusplus
rsync -a --exclude storage_1 "$MIS/" $T/mis/
rm -rf ~/dayz/@AGTest; mkdir -p ~/dayz/@AGTest/addons
cp ~/armagedon-dayz/dist/@ArmaGedonCore/addons/*.pbo ~/dayz/@AGTest/addons/
# paczki dolaczane przy deployu: bronie/sprzet (z obecnego moda) i budowanie bazy (z ~/.agb_cache)
cp ~/dayz/@ArmaGedonCore/addons/armagedonweapons_*.pbo ~/dayz/@ArmaGedonCore/addons/armagedongear_*.pbo ~/dayz/@AGTest/addons/ 2>/dev/null || true
cp ~/.agb_cache/armagedonbase_*.pbo ~/dayz/@AGTest/addons/ 2>/dev/null || echo "UWAGA: brak paczek budowania - najpierw: bash server/building_pack.sh build"
MODS=$(grep -v '^#' ~/armagedon-dayz/server/mods.txt | grep -v '^\s*$' | awk '{print $2}' | paste -sd';')
cd ~/dayz
timeout $SECS ./DayZServer -config=serverDZ.cfg -port=2502 -profiles=$T/prof -mission=$T/mis "-mod=$MODS;@AGTest" >/dev/null 2>&1 &
sleep $((SECS-5))
L=$(ls -t $T/prof/script*.log 2>/dev/null | head -1)
echo "LOG: $L"
grep -c "SCRIPT    (E)" "$L"
grep -A3 "SCRIPT    (E)" "$L" | head -12
grep -i "config.cpp" $T/prof/*.RPT 2>/dev/null | head -5
[ -n "${AGT_EVTEST:-}" ] && grep -E "EVENT|EVTEST" $T/prof/ArmaGedonCore/admin_log.txt | cut -c1-170
pkill -f "port=2502" 2>/dev/null
rm -rf ~/dayz/@AGTest
