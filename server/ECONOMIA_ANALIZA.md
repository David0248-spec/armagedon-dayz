# ARMA-GEDON DayZ – ekonomia Vanilla+ (115 graczy)

Liczby poniżej to wynik `apply_economy.py` + `mod_types.py` na plikach vanilla Chernarus
(bez typów z paczek ubrań/optyk z Workshopu). Stan: 28.11.2026.

## Jak to działa
1. `tools/econ_plan.py` wylicza `set_nominal` dla każdego typu: vanilla × mnożnik kategorii,
   ale nie więcej niż limit skupisk (liczba budynków w strefie).
2. `manual_nominal` w `economy.json` to wartości ustawione ręcznie – **mają pierwszeństwo**
   przed `set_nominal` i `econ_plan.py --apply` ich nie nadpisuje.
3. `restock` zostaje jak w vanilli (większość typów 0 = dosypywanie od razu po spadku poniżej `min`).
   `lifetime` też jak w vanilli.

## Vanilla → teraz (nominal)
| | Vanilla | Teraz |
|---|---|---|
| Bronie (same lufy) | 931 | 1386 |
| Bronie w barakach (Military, bez eventów) | 171 | 284 |
| Magazynki | 624 | 1043 |
| Jedzenie | 831 | 1161 |
| Narzędzia | 6482 | 7710 |

Magazynki są dopasowane do broni: ok. 1,5 magazynka na broń, która z nich korzysta.
Magazynki do broni tylko z eventów (SVT-40, MK12, SR-2M, SV-98, FN SPR) – po 4–6 szt. w barakach.

## Bronie
- Baraki T2–T4: AK74 14, AKS74U 16, AKM 10, AK101 9, M16A2 11, M16A4 8, FAMAS 12, G36/G36C 8,
  MP5K 24, UMP45 18, PP19 20, Saiga 16, FNX45 22 + reszta paczki po 5–8.
- Magazynki do FAMAS (25), AK101 (30) i M14 leżą w barakach T3–T4 (w vanilli były tylko na eventach,
  a same bronie są w barakach).
- End-game tylko z helicrashy/konwojów i eventów skryptowych (deloot): M4A1, FAL, SVD, VSS, AS VAL,
  SCAR, AEK, AN-94, SR-2M, M24, SVT-40, SV-98, MK12, FN SPR, tłumiki, NVG.

## Raidy
- Vanillowy improwizowany ładunek: Protective Case + Electronic Repair Kit, do tego ładunek
  (plastik/granaty) i zapalnik (zdalny / budzik / minutnik).
- `Plastic_Explosive` 6 szt. (min 2), tylko strefy Industrial – rzadki.
- Zapalnik zdalny 8, Protective Case 80, Electronic Repair Kit 60, budziki 3×24, minutniki 30.
- Własne C4 (`AGA_TimedExplosive`) wyłączone – plastik nie ma slotu na budzik.

## Medyczne
x1,25 vanilli, tylko strefa Medic: bandaże 50, adrenalina 36, morfina 19, sól 30, zestaw IV 36,
worki na krew 24, węgiel/przeciwbólowe/tetracyklina 24, apteczki 18.
(Audyt skupisk pokazuje je jako "wysokie ryzyko", bo szpitali jest mało, ale to duże budynki
z wieloma punktami lootu – 2–3 bandaże w szpitalu są w porządku.)

## Eventy
- CE: helicrash 5, konwój wojskowy 3, pociąg 3, policja 5; pojazdy ×1,25.
- Skryptowe (stale aktywne): myśliwi 2, cywilne 2, budowa 2, heli 2, konwój 1, konwój wojskowy 1,
  policja 1, samolot 1. Najwyżej jedna rzadka broń na event.

## Co obserwować po starcie
- Czy w barakach trafiają się bronie (nie same magazynki) – jeśli nadal mało, podnieść
  `manual_nominal` broni o 20%.
- Czy plastiku nie jest za dużo (raidy co chwilę) – wtedy `Plastic_Explosive` 6 → 4.
- Jedzenie: jeśli brakuje, +20% w `set_nominal` kategorii food.
