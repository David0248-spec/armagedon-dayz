# Mody KarmaKrew i Salvation – pełna lista, za co odpowiada i co mamy

Stan na 25.11.2026. Zawartość każdej paczki sprawdzona przez rozpakowanie `.pbo`
(nazwy skryptów, modele, tekstury), a nie z samych nazw plików.

- **KarmaKrew** (`2864245850` + `3033180005` + `3033180335`): 80 paczek, 4,6 GB
- **Salvation** (`3421213293`): 162 paczki, 10,2 GB

Masz zgodę obu ekip, więc poniżej są także ich własne mody, nie tylko publiczne.

---

## CZĘŚĆ 1 — TO JUŻ MAMY (nasze własne odpowiedniki)

Te paczki są **niepotrzebne**. Kolumna „nasze" = plik w `mods/ArmaGedonCore`.

| ich mod | za co odpowiada | nasze |
|---|---|---|
| `KK_StaminaSoftSkill` | kondycja jako umiejętność, strzałki przy pasku | `AGA_Systems.c` + `AGA_StaminaArrows.c` |
| `KK_C4_Timer` | C4 z odliczaniem i rozbrajaniem | `AGA_C4.c` |
| `KK_IgniteCar` | podpalanie pojazdów zapalniczką | `AGA_IgniteCar.c` |
| `KK_AFK_Client` | wyrzucanie za bezczynność | `AGA_Server.c` |
| `KK_ServerRules_Client` | okno regulaminu po wejściu | `AGA_RulesMenu` |
| `WatchtowerHeightLimiter` | limit wysokości wieży | `AGA_Systems.c` |
| `KK_BrokenArms_Client` | złamana ręka, drżenie celownika | `AGA_Survival.c` |
| `KK_PlaneCrash` | wrak C-130 jako event | `AGA_Events.c` (event PLANE) |
| `KK_CL_Client` | blokada wylogowania w walce | `AGA_Mission.c` (LogoutMenu) |
| `ZenAntiCombatLogout` (Salv) | to samo, wersja Zena | `AGA_Mission.c` |
| `KK_Balance_Client`, `KK_Chernarus_Balance` | balans broni, wyłączone choroby | `Balance/config.cpp` + `AGA_Balance.c` |
| `KK_WeaponNames` | własne nazwy broni | `Names/config.cpp` |
| `KK_FootSteps_Client` | dźwięki kroków | mamy w `config.cpp` |
| `SALVATION_CodeLock_Client` | zamek na kod | `@codelock` + `AGA_Palisade.c` |
| `LBmaster_AdminTools` + `LBmaster_Core` | panel admina (380 skryptów!) | `@vppadmintools` |
| `Salvation_VehicleFlip` | obracanie wywróconego auta | `@fliptransport` |
| `LBmaster_EarPlugs` | zatyczki do uszu | `@ear-plugs` |
| `tR_Fortification`, `tR_Stockade` | dodatkowe elementy budowy | `@buildingfortifications` |
| `JLKStorage`, `LoadedStash`, `barrel_stash` | schowki, zakopywanie beczek | `@mmgbasestorage` + `AGA_ZStorage.c` |
| `KK_MountsSightsOverride` | montaże optyki | `@mountsandsights` (ale patrz niżej) |

---

## CZĘŚĆ 2 — WARTO WZIĄĆ (gotowe, lekkie, realna wartość)

### Najwyższy priorytet

| mod | skąd | rozmiar | za co odpowiada |
|---|---|---|---|
| `ZenNotes` | oba | 2,3 MB | pisanie i czytanie notatek; jest w naszym `balans.md` jako „do zrobienia" |
| `ZenCauseOfDeath` | KK | 0,03 MB | akcja „sprawdź przyczynę śmierci" na zwłokach; też w `balans.md` |
| `ViewInventoryAnimation` | KK | 0,2 MB | animacja grzebania w ekwipunku, widoczna dla innych; w `balans.md` |
| `Shino_SwapDropAtFeet` | KK | 0,01 MB | przy zamianie przedmiotu stary leci pod nogi, nie znika; w `balans.md` |
| `BedRespawn` | Salv | 0,05 MB | respawn na łóżku/śpiworze w bazie (19 skryptów, własne akcje budowy) |
| `LongerSunriseAndSunset` | Salv | 0,16 MB | dłuższy wschód i zachód; przy `serverTimeAcceleration = 2` noc zapada nagle |

### Warte dołożenia

| mod | skąd | rozmiar | za co odpowiada |
|---|---|---|---|
| `ZenZombieDoors` | Salv | 0,02 MB | zombie otwierają i walą w drzwi |
| `ZenFirewood` | Salv | 0,03 MB | stosy drewna do rąbania w świecie |
| `ZenFireplaceStick` | Salv | 0,01 MB | pieczenie na patyku wsadzonym w ognisko |
| `BetterSmokes` | Salv | 0,05 MB | większe, gęstsze granaty dymne (mamy je na eventach) |
| `BulletHoles` | Salv | 1,5 MB | dziury po kulach na ścianach |
| `AnimatedCan` | Salv | 0,17 MB | animacja otwierania konserwy |
| `DreamBatteryCharger9V` | Salv | 1,8 MB | ładowarka do baterii 9V |
| `MoreDurableWeapons` | Salv | 0,04 MB | bronie wolniej się psują (sam config, 0 skryptów) |
| `KK_ToolBox_Client` | KK | 0,01 MB | skrzynka narzędziowa; w `balans.md` |
| `QuickLoader` | KK | 0,04 MB | szybsze ładowanie magazynków |
| `Salvation_Nights` | Salv | 0,02 MB | jaśniejsze noce (1 skrypt: `WorldLighting`) |
| `SchanaModAutorun` | Salv | 0,01 MB | autobieg na klawisz |
| `relife_VehicleInfo` | Salv | 0,03 MB | prędkościomierz i paliwo na ekranie |
| `CheckIdentity` | Salv | 0,01 MB | sprawdzanie tożsamości gracza (do RP/grup) |
| `FenceOverride` | Salv | 0,03 MB | wyłączenie wieży z vanilli (1 skrypt) |

---

## CZĘŚĆ 3 — DUŻE SYSTEMY (wymagają decyzji)

| mod | skąd | rozmiar | za co odpowiada | uwaga |
|---|---|---|---|---|
| `KK_SleepingBag_Client` | KK | 15 MB | śpiwór do rozkładania | dubluje się z `BedRespawn` |
| `Salvation_Medical` | Salv | 6,8 MB | opaski zaciskowe (CAT), szyny aluminiowe | zmienia balans leczenia |
| `Salvation_SoftSkills` | Salv | 0,14 MB | umiejętności: walka wręcz, używanie przedmiotów | osobny system od kondycji |
| `RadioRework` + `Client` + `Defines` | Salv | 42 MB | radio na kanałach, szyfrowanie, PTT | dobre do grup |
| `KeycardSystem` | Salv | 11 MB | karty dostępu do drzwi i bunkrów | wymaga miejsc docelowych |
| `AdmiralsRaidDrillMod` | Salv | 20 MB | wiertło do raidów | alternatywa dla C4 |
| `tR_ExplosiveCharge` | Salv | 30 MB | ładunki wybuchowe do raidów | jak wyżej |
| `AdmiralsElevatorMod` | Salv | 71 MB | windy w budynkach | tylko z własnymi budynkami |
| `BP_Workbench`, `Salvation_Workbench` | Salv | 56 MB | stół warsztatowy z craftingiem | mamy swój w planach budowy |
| `ArmoredVehicles`, `STR_ModularCarArmor` | Salv | 387 MB | pancerz montowany na autach | duże, zmienia PvP pojazdów |
| `R22R_Ammo_Crates` | Salv | 212 MB | skrzynie i szafki na amunicję (61 skryptów) | konkuruje z `@mmgbasestorage` |
| `tR_Modules`, `tR_TreeHouse`, `tR_Dependence` | Salv | 120 MB | drzwi, szopy, domek na drzewie | własny system budowy |
| `NPKOT_DugoutHouse` | Salv | 12 MB | ziemianka | jak wyżej |
| `Salvation_Cooking` | Salv | 0,03 MB | gotowanie potraw, pikle | wymaga ich przedmiotów |
| `Salvation_Items` | Salv | 0,02 MB | taśma, klej epoksydowy, zestaw elektroniczny, ciepło w budynkach | ciekawe, małe |
| `SalvationMagazines` | Salv | 0,02 MB | szybsze przeładowanie, podgląd stanu magazynka | |
| `Salvation_Sounds` | Salv | 1,3 MB | dźwięki ekwipunku i celowania (126 plików) | |
| `SalvationTips` | Salv | 0,44 MB | własne paski statystyk na ekranie | |
| `DripZ_Client` | Salv | 9,5 MB | podgląd stroju postaci | |
| `Factions` | Salv | 0,69 MB | opaski frakcyjne | pasuje do naszych grup |
| `ZenDiscordAPI` | Salv | 37 MB | powiadomienia na Discord, alarm raidowy | tylko z webhookiem |
| `ServerSentryDayZMod` | Salv | 5,5 MB | wieżyczki strażnicze (194 skrypty) | mamy `AGA_Guard.c` |
| `Salvation_UI` | Salv | 37 MB | własny interfejs, ekran śmierci, poradnik | duża zmiana wizualna |
| `KK_BaseBuilding` + `BP_Scripts_Expansion` | KK | 3,5 MB | **ich cały system budowy**: studnia artezyjska, poziomy flagi, ulepszanie | 216 skryptów, kolidowałby z naszym; za to **jest to dokładnie to, co mamy w `balans.md` jako „System budowy (duży, osobna sesja)"** — warto przeanalizować jako wzór |
| `0_CodeLockCompat`, `KK_FC` | KK | 0,03 MB | zgodność CodeLock z ich budową | tylko z `KK_BaseBuilding` |

---

## CZĘŚĆ 4 — CZEGO NIE BRAĆ

| mod / grupa | rozmiar | dlaczego |
|---|---|---|
| `KK.pbo` | 617 MB | **częściowo już używamy**: `weapons_pack.sh` przerabia go na `armagedonweapons_core.pbo` (same bronie pod nazwami AG). Całej paczki nie ładujemy — to rdzeń ich serwera, 73 skrypty, nie da się wyciąć kawałka |
| `KK_Core.pbo` | 314 MB | rdzeń ich serwera, 47 skryptów |
| `KK_Sprays_Client` | 1,29 GB | malowanie broni i ubrań (817 tekstur) |
| `z_KK_Loading_Generic` | 137 MB | ich ekran ładowania i kolejka |
| `Land_Tisy_Control_Panel` | 160 MB | ich własny obiekt na Tisy |
| `Structures`, `CS_Buildables`, `camonet`, `grass`, `ContainerRetexture` | 407 MB | ich obiekty i tekstury świata |
| `bastard_*` (UAZ, Defender, Tigr, LIAZ, Dnepr, quad, BTR-90, Humvee, BMW, G55, T4, zazik) | ~1,5 GB KK + ~1,4 GB Salv | pojazdy; mamy helikoptery Expansion |
| `StalkerZ_*`, `AR_*`, `Loaded_*`, `SFP*`, `gray_zone_pack`, `AsmondWeapons`, `PrizWeaponMod`, `Old_Weapons`, `ShotShotShot_*` | ~6 GB | paczki broni i ubrań Salvation; mamy własne `ArmaGedonWeapons`/`Gear` |
| `SALVATION_Weapons` | 647 MB | jak wyżej |
| `VBB_Weapons`, `ca_weapons`, `Spur_BagZ`, `Validol_Retextures`, `Windstride_Retex` | 710 MB | **już je mamy** przez `weapons_pack.sh` jako `armagedonweapons_vbb/ca` i `armagedongear_bags/clothes/colors` |
| `Weapons.pbo` (8 MB), `Gear.pbo` (31 MB) | 39 MB | ich dodatkowe bronie i sprzęt, których `weapons_pack.sh` **nie** bierze — do sprawdzenia, czy jest tam coś, czego nie mamy |
| `SalvationBunker`, `3RNO`, `BZE_*`, `evg_campSites`, `Metal_Doors`, `LabLights` | ~400 MB | własne budynki i mapy, wymagają zmian w misji |
| `SalvationQuests`, `Quest_PC` | 40 MB | system questów, osobny projekt |
| `Salvation_PremiumSkins` | 0,004 MB | skórki dla wspierających |
| `SalvationMapLoader` | 5,8 MB | ich ładowanie mapy |
| `*Fix` (`Salvation_DoorFix`, `tRChargeFix`, `KCSPeekFix`, `R22RLockFix`, `tRTreeHouseFix`, `RecipeGuard`) | <0,02 MB | łatki do **ich** modów, dla nas bez sensu |

---

## PODSUMOWANIE LICZB

| | KarmaKrew | Salvation |
|---|---|---|
| paczek razem | 80 | 162 |
| rozmiar | 4,6 GB | 10,2 GB |
| mamy odpowiednik | 14 | 8 |
| warto wziąć | 6 | 15 |
| do decyzji | 3 | 25 |
| nie brać | ~57 | ~114 |

**Paczki w obu zestawach** (czyli sprawdzone u obu): `ZenNotes`, `LBmaster_AdminTools`,
`LBmaster_Core`, `BastardAnims_*`, `bastard_framework`, `bastard_uaz`, `bastard_defender`,
`bastard_quadbike`.

Jeśli weźmiemy całą grupę „warto wziąć" (6 + 15 paczek), gracze dociągną około **6 MB**,
a dostaniemy 4 rzeczy z naszej listy „do zrobienia" plus respawn w bazie i dłuższy zmierzch.
