# Mody KarmaKrew i Salvation – analiza i lista do wdrożenia

Stan na 25.11.2026. Paczki sprawdzone lokalnie w `steamapps/workshop/content/221100`:
KarmaKrew `2864245850` + `3033180005`, Salvation `3421213293`.

---

## 1. BŁĄD KONDYCJI (pasek znika i się cofa) – do naprawy u nas

### Przyczyna
W `mods/ArmaGedonCore/Scripts/4_World` są **dwa niezależne systemy kondycji**, oba
nadpisujące `StaminaHandler` i oba zmieniające maksymalny pasek:

| plik | klasa / zmienna | zapis |
|---|---|---|
| `AGA_Survival.c` | `PlayerBase.m_AGAStam`, `AGA_SetStam` | `AGA_StamSave` → `<uid>.json` |
| `AGA_Systems.c` | `PlayerBase.m_AGA_StaminaSkill`, `AGA_SetStaminaSkill` | `AGA_Stamina` → `stamina.json` |

Oba robią `RecalculateStaminaCap()` i **oba dodają swój bonus do `m_StaminaCap`**:
- `AGA_Survival.c:156` → `m_StaminaCap + m_Player.AGA_Stam()`
- `AGA_Systems.c:64` → `m_StaminaCap + m_Player.AGA_GetStaminaSkill()`

Enforce Script łączy oba `modded class` w jedną klasę, więc oba `override` się wykonują.
Bonus jest liczony **podwójnie**, a każdy tick (co 20 s) nadpisuje `m_StaminaCap` inną wartością
→ pasek skacze, znika i się cofa. Dodatkowo oba liczą ten sam tick co 20 s, czyli kondycja
rośnie/spada 2× szybciej niż w ustawieniach.

### Jak to robi KarmaKrew (`KK_StaminaSoftSkill.pbo`)
1. **Jeden** `modded class StaminaHandler`, jedno miejsce liczenia.
2. Wartość trzymana jako **PlayerStat** (`RegisterStat(EPlayerStats_KK.STAMINASOFTSKILL, ... EPSstatsFlags.SYNCED)`),
   nie jako własna zmienna. DayZ sam ją zapisuje z postacią i synchronizuje.
3. Osobna zmienna `m_StaminaBuff` z `RegisterNetSyncVariableFloat(..., 0)` (0 miejsc po przecinku)
   tylko do wyświetlania na kliencie.
4. `RecalculateStaminaCap()` **przepisany od zera** (nie `super.` + dodatek) – liczy
   wagę i bonus w jednym miejscu, więc nie ma podwójnego dodawania.
5. Strzałki przy pasku: `hud.DisplayStaminaArrows(softSkillValue)` pod `#ifndef SERVER`.
6. Zapis przy śmierci (`EEKilled`) tylko gdy wartość jest ujemna – kara przechodzi
   między życiami, bonus nie.

### Co zrobić u nas
- **Usunąć system z `AGA_Survival.c`** (`m_AGAStam`, `AGA_SetStam`, `AGA_StamStep`, `AGA_StamSave`,
  `LoadStam`/`SaveStam`) i zostawić **tylko** ten z `AGA_Systems.c`.
- W `RecalculateStaminaCap()` nie wołać `super.` i dodawać bonusu, a policzyć cap od zera
  jak KarmaKrew (inaczej każdy inny mod dodający do capa znowu się zdubluje).
- Rozważyć przejście na `PlayerStat` zamiast własnego JSON-a – DayZ zapisuje to z postacią,
  więc znika problem z plikami per-uid.
- Strzałki wzrostu/spadku przy pasku (`DisplayStaminaArrows`) – u nas nie ma, gracz nie widzi,
  czy kondycja rośnie.

---

## 2. CO MAMY WŁASNE, A ONI MAJĄ JAKO MOD

Nasze odpowiedniki działają – te paczki są **niepotrzebne**, służą tylko jako wzór:

| KarmaKrew | nasz odpowiednik |
|---|---|
| `KK_C4_Timer.pbo` | `AGA_C4.c` |
| `KK_IgniteCar.pbo` | `AGA_IgniteCar.c` |
| `KK_AFK_Client.pbo` | AFK kick w `AGA_Server.c` |
| `KK_ServerRules_Client.pbo` | `AGA_RulesMenu` |
| `WatchtowerHeightLimiter.pbo` | limit wieży w `AGA_Systems.c` |
| `KK_StaminaSoftSkill.pbo` | `AGA_Systems.c` (patrz pkt 1) |
| `KK_BrokenArms_Client.pbo` | złamana ręka w `AGA_Survival.c` |
| `KK_PlaneCrash.pbo` | event PLANE w `AGA_Events.c` |
| `KK_ToolBox_Client.pbo` | mamy w planach (`docs/balans.md`) |
| `ZenAntiCombatLogout` (Salvation) | logout w walce w `AGA_Mission.c` |

---

## 3. LISTA MODÓW DO DODANIA – POSORTOWANA PO WARTOŚCI

> **Skąd brać:** większość z tych modów to cudze mody publiczne (Zen, Shino, JLK itd.),
> które KarmaKrew i Salvation tylko dołączyli do swoich paczek. Bierzemy je **z oryginalnego
> Workshopu autora**, a nie wypakowane z paczek KK/Salvation – zgoda KarmaKrew obejmuje ich
> własne mody, nie cudze. Tak samo dostajemy aktualizacje od autora.

### A. Warto wziąć od razu (gotowe, lekkie, bez pisania kodu)

| mod | skąd | po co |
|---|---|---|
| `ZenNotes` | KK + Salvation | notatki dla graczy; mamy to w planach jako „mod publiczny Zen" |
| `ZenCauseOfDeath` | KarmaKrew | przyczyna śmierci przy sprawdzaniu zwłok; też jest w naszych planach |
| `ViewInventoryAnimation` | KarmaKrew | animacja oglądania ekwipunku; w planach |
| `Shino_SwapDropAtFeet` | KarmaKrew | upuszczanie pod nogi przy zamianie; w planach |
| `KK_MountsSightsOverride` | KarmaKrew | konfiguracja montaży optyki (mamy `@mountsandsights`, to override) |
| `ZenZombieDoors` | Salvation | zombie otwierają drzwi – realizm, zero konfiguracji |
| `BedRespawn` | Salvation | respawn na łóżku w bazie – mocno poprawia życie przy 115 graczach |
| `LongerSunriseAndSunset` | Salvation | dłuższy wschód/zachód; przy `serverTimeAcceleration = 2` noc jest nagła |
| `BetterSmokes` | Salvation | lepsze granaty dymne (mamy je na eventach) |
| `BulletHoles` | Salvation | dziury po kulach – wizualny detal |

### B. Warte rozważenia (zmieniają rozgrywkę, trzeba decyzji)

| mod | skąd | uwaga |
|---|---|---|
| `KK_SleepingBag_Client` | KarmaKrew | śpiwór; pasuje do bazowania, ale to kolejny system |
| `KK_BoatStrandedToxic` | KarmaKrew | strefa toksyczna na statku – mamy już strefy skażone |
| `Salvation_Medical` (12 skryptów) | Salvation | rozbudowana medycyna; duża zmiana balansu |
| `RadioRework` | Salvation | radio działające na kanałach; fajne do grup |
| `KeycardSystem` | Salvation | karty dostępu do bunkrów/questów |
| `AdmiralsRaidDrillMod` | Salvation | wiertło do raidów – alternatywa dla C4 |
| `tR_Fortification`, `tR_Stockade` | Salvation | dodatkowa budowa; mamy `@buildingfortifications` |
| `JLKStorage`, `LoadedStash`, `barrel_stash` | Salvation | schowki; mamy `@mmgbasestorage` + własne skrzynie |
| `MoreDurableWeapons` | Salvation | bronie się rzadziej psują (tylko config, 0 skryptów) |
| `ZenFirewood`, `ZenFireplaceStick` | Salvation | drobne usprawnienia ogniska |
| `Salvation_VehicleFlip` | Salvation | obracanie auta; mamy `@fliptransport` |

### C. Czego NIE brać

| mod | dlaczego |
|---|---|
| `KK.pbo` (647 MB), `KK_Core`, `KK_Sprays_Client` (1,35 GB) | rdzeń ich serwera, nie da się wyciąć kawałka |
| `KK_BaseBuilding`, `KK_Balance_*`, `BP_Scripts_Expansion` | ich własny system budowy i balansu – kolidowałby z naszym |
| `bastard_*` (UAZ, Defender, Tigr, LIAZ, Dnepr, quadbike, BTR-90, Humvee) | pojazdy, łącznie kilka GB; mamy helikoptery Expansion |
| `StalkerZ_*`, `AR_*`, `Loaded_*`, `SFP*`, `gray_zone_pack`, `AsmondWeapons` | paczki broni i ubrań Salvation, po 300 MB – 1,2 GB każda; mamy już swoje |
| `SalvationBunker`, `3RNO`, `NPKOT_DugoutHouse`, `BZE_*` | własne budynki/mapy – wymagają zmian w mapie |
| `SalvationQuests`, `Quest_PC`, `Salvation_UI` | system questów z własnym interfejsem; osobny projekt |
| `LBmaster_AdminTools` | mamy VPP Admin Tools |
| `SALVATION_CodeLock_Client` | mamy `@codelock` |
| `ZenDiscordAPI` | tylko jeśli chcemy powiadomienia na Discordzie |

---

## 4. KOLEJNOŚĆ PRAC – PROPOZYCJA

1. **Naprawa kondycji** (pkt 1) – błąd, który widzą gracze; nic nie trzeba dokładać.
2. **Paczka „drobne usprawnienia"**: `ZenNotes`, `ZenCauseOfDeath`, `ViewInventoryAnimation`,
   `Shino_SwapDropAtFeet` – wszystkie są w `docs/balans.md` jako „do zrobienia", a istnieją
   jako gotowe mody publiczne.
3. **`BedRespawn` + `LongerSunriseAndSunset`** – największy wpływ na komfort przy 115 graczach.
4. **`ZenZombieDoors`, `BetterSmokes`, `BulletHoles`** – realizm, zero ryzyka.
5. Dopiero potem decyzje o dużych systemach z grupy B.

Każdy dodany mod to pozycja w `server/mods.txt` + wpis w `server/setup_ce.sh` (jeśli ma `types.xml`)
i ponowne pobranie paczki przez graczy.
