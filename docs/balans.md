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

- Podpalanie pojazdów (0.5 s przy wlewie, silnik 0, wybuch po 7 s, zasięg ×13, 60 HP / 100 szok)
- Wylogowanie w walce (okno 60 s od strzału/trafienia → licznik min. 60 s + log; ostrzeżenie i blokada przycisku przy >50 s)
- Okno regulaminu po wejściu

## Budowanie bazy (ArmaGedonBase)
- Konstrukcje: fortyfikacje (ściany, drzwi, bramy, piętra), drzwi wąskie/szerokie, brama garażowa, szopa, osłony, palisada z narożnikami, domek na drzewie – paczki `armagedonbase_*.pbo` budowane przez `server/building_pack.sh` (`tools/agb_build.py`)
- Meble: skrzynie poz. 1 → średnia → duża → poz. 4, stojaki na broń drewniane 1–3 i metalowy, stojaki na ubrania, warsztat 1–3, szafka, lodówka, grządki, zbieracz wody, zakopywane skrzynie i beczki
- Ulepszenia mebli: pierwszy stopień flaga poz. 2, kolejne poz. 3; rozbieranie tylko we własnym terytorium i puste
- Vanillowe płoty i wieże wyłączone; MMG i Building Fortifications usunięte
- Raidy (`AGA_Raid.c`, tabela jak KarmaKrew – przebicie połowy z ramą): 5.45 500/720, 5.56 550/720, 7.62x39 400/660, 7.62x54/.308 250/360, śrut 150/215 strzałów, breneka 350/450, granat 20, mina 50/60, claymore 10/15, plastik 3 (drewno/metal). Broń biała nie rani ścian; wybuch pełny do 2,5 m, od 6 m bez obrażeń. Tylko w godzinach raidów.
- Limit: 5 drzwi/bram na bazę (każde skrzydło lewe / całe drzwi / brama garażowa)

## Do zrobienia
### System budowy (duży, osobna sesja)
- Flaga z poziomami, dołączanie do flagi, zaproszenia, menu grupy (GroupManagement), ulepszanie kontenerów (L1+), stojaki na broń/ubrania, stół, palisady, barykady drzwi, panele metalowe, oznaczanie skrzyń
- Strefy zakazu budowy (koła/wielokąty, konfiguracja wysyłana z serwera), min. odstęp flag 30 m
- Nowe akcje budowy/rozbiórki, zmiany receptur (płot, flaga, wieża, skrzynia, ognisko)
- Studnia artezyjska, skrzynia na rośliny, kran z wodą
- Hipoksja / złe powietrze (bunkier)
- UWAGA: poziomy flagi, limity bram itd. są w configu serwera (JSON), nie w paczce klienta – w skryptach są tylko wartości domyślne

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
