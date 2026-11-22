# KarmaKrew 1:1 – inwentaryzacja i status

Zasada: kod piszemy od zera w ArmaGedonCore, wartości balansu 1:1 jak u KarmaKrew.
Publiczne mody, które KarmaKrew ma wpakowane w swoje paczki, podpinamy z Workshopu.
Loot (types/events) nie jest w paczkach klienta – ustawiamy sami (vanilla+ i fazy wipe).

## Zrobione
- Obrażenia/balistyka amunicji (CfgAmmo) 1:1 – `config.cpp`
- Bronie: initSpeedMultiplier, dispersion, recoil, ChanceToJam, hitpoints 1:1 – `config.cpp`
- Magazynki: ChanceToJam 1:1
- Mgła Chernarus `volFogOffset = -200`
- Code Lock `itemSize 2x2`
- Helikoptery Expansion (Merlin/UH1H/MH6): fuelConsumption 150, Projectile damage 1.35, altitude 800, ładownie 10x30/10x20/10x10, koła physLayer item_small
- Wyłączone: zapalenie płuc, metale ciężkie; heli z pełnym olejem; brak plandeki na pojazd i tankowania heli w zawisie – `AGA_Balance.c`
- AFK kick 600 s
- Building Fortifications (Workshop 2670506982)
- Dynamiczne eventy (samolot, konwój medyczny, obozy, plac budowy, helicrashe), restarty 2x dziennie, godziny raidów, fazy wipe

## Do zrobienia (wartości z paczek KK)
| System | Wartości KK | Plan |
|---|---|---|
| Stamina soft skill | zakres ±15 pkt max staminy, tick 20 s; sprint +0.04, ruch +0.02, stanie −0.03, teren bazy −0.035; czerwona stamina ×1.15; ujemny skill → do 50% większe zużycie jedzenia/wody; < −7.5 drenaż ×1.5; zapis między sesjami | własny kod |
| C4 timer | odliczanie 10 s, rozbrajanie 6 s, pikanie 1.0 s → 0.2 s (wykładnik 1.75), koszt naprawy 50 | własny kod |
| Watchtower height limit | domyślnie 1.0 m | własny kod |
| Anty-freecam | co 9 s, kamera 5–1300 m od gracza → raport do serwera | własny kod |
| Zombie Spetsnaz | KK_ZmbM_Spetznas_Normal: hitpoints 230 (baza ZmbM_eastSoldier_normal_Navy); Officer: skinning 10 steaków, palec | własne klasy w config |
| Regulamin przy wejściu | okno z regulaminem (KK_ServerRules) | własne menu |
| Upuszczanie pod nogi przy zamianie | Shino_SwapDropAtFeet | własny kod / mod |
| Przyczyna śmierci (sprawdzanie zwłok) | ZenCauseOfDeath | mod publiczny Zen |
| Notatki | ZenNotes | mod publiczny Zen |
| Animacja oglądania ekwipunku | ViewInventoryAnimation | mod publiczny |
| Podpalanie aut | KK_IgniteCar | własny kod |
| Toolbox, logout | KK_ToolBox, KK_CL | własny kod |
| Mounts & Sights override | konfiguracja montaży | do analizy |

## Nie do odtworzenia od zera (modele/tekstury)
KK.pbo, KK_Core.pbo, Structures, Gear, VBB_Weapons, Weapons (bp_m92), ca_weapons, KK_Sprays (1.3 GB), BTR_90, pojazdy Bastard (UAZ, Tigr, Liaz, Dnepr, Quad, Defender), Spur_BagZ, Validol/Windstride retekstury, ekrany ładowania, LBmaster AdminTools (płatny).
Tu można tylko podpiąć publiczne odpowiedniki z Workshopu, jeśli istnieją.
