# Balans i systemy ARMA-GEDON – status

Cały kod autorski w ArmaGedonCore. Loot: vanilla+ i fazy wipe.

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

- Kondycja (soft skill) ±15, tick 20 s, +0.04/+0.02/−0.03/−0.035, ×1.15, kara jedzenie/woda do 50%, próg −7.5 → ×1.5, zapis między życiami – `AGA_Systems.c`
- Limit wysokości zestawu wieży: 1 m nad terenem
- Kontrola kamery co 9 s (5–1300 m) → log i powiadomienie adminów
- Zombie AG_ZmbM_Spetsnaz (230 HP) na wojskowych eventach

## Do zrobienia
| System | Wartości | Plan |
|---|---|---|
| C4 timer | odliczanie 10 s, rozbrajanie 6 s, pikanie 1.0 s → 0.2 s (wykładnik 1.75), koszt naprawy 50 | własny kod |
| Regulamin przy wejściu | okno z regulaminem | własne menu |
| Upuszczanie pod nogi przy zamianie | własny kod | własny kod / mod |
| Przyczyna śmierci (sprawdzanie zwłok) | własny kod | mod publiczny Zen |
| Notatki | własny kod | mod publiczny Zen |
| Animacja oglądania ekwipunku | ViewInventoryAnimation | mod publiczny |
| Podpalanie aut | własny kod | własny kod |
| Toolbox, logout | własny kod | własny kod |
| Mounts & Sights override | konfiguracja montaży | do analizy |

## Poza zakresem
Nowe modele, tekstury i obiekty 3D.
