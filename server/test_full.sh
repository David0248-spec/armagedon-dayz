#!/bin/bash
# test kompilacji: nasz mod + paczki ArmaGedonWeapons/Gear na osobnym porcie (bez wplywu na serwer)
set -u
rm -rf /tmp/agw_test; mkdir -p /tmp/agw_test/addons
AGW_DST=/tmp/agw_test bash ~/armagedon-dayz/server/weapons_pack.sh
sed 's#^cp ~/armagedon-dayz/dist/@ArmaGedonCore/addons/\*.pbo ~/dayz/@AGTest/addons/#&\ncp /tmp/agw_test/addons/*.pbo ~/dayz/@AGTest/addons/#' ~/armagedon-dayz/server/test_compile.sh > /tmp/tc_full.sh
bash /tmp/tc_full.sh
grep -ihE "requires addon|cannot|undefined|duplic|error" /tmp/agtest/prof/*.RPT | grep -v Inputs | sort | uniq -c | sort -rn | head -15
