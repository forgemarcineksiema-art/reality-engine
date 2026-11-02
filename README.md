# Projekt Genesis / Reality Engine

**Stan na 02.11.2026: pierwszy prototyp techniczny M00–M06 jest ukończony i odebrany w opisanym zakresie.** [Plan prototypu](docs/prototype-plan.md) wskazuje etapy, a [raport M06](docs/verification/m06-acceptance.md) łączy pełne testy, 32-minutowy przebieg aplikacji i zgodną kontynuację końcowego świata. To fundament docelowego projektu, nie ukończona gra ani pełna symulacja cywilizacji.

**Kolejny kierunek jest otwarty.** Pełne 3D nie jest zatwierdzonym następnym etapem. Aktualne cele i granice: [karta projektu](docs/project-charter.md), DEC-019 w [rejestrze decyzji](docs/decisions.md). Historyczne propozycje nie są automatycznie backlogiem.

Genesis rozwija własny Reality Engine dla trwałego świata i autonomicznej społeczności, z długofalową wizją fantasy/Action RPG. Silnik, fizyka i nawigacja są własne; grafika ma najniższy priorytet. Użytkownik pracuje z agentami AI na i5/MX330/8 GB, bez narzuconego terminu.

## Co działa

| Obszar | Obecny zakres i źródło dowodów |
| --- | --- |
| Domena i trwałość | ID, czynności, rezerwacje, materiały/pojemność, fizyczny transfer i zgodny odczyt starych zapisów: [model](docs/work-domain.md), [SC-08](docs/verification/sc08-continuity-audit.md) |
| Ruch i przestrzeń | Kinematyka 2,5D, wspólna geometria kolizji/tras, A*, podparcie, Step i ruchoma przeprawa: [model](docs/motion-navigation.md), [autonomia](docs/traversal-autonomy.md) |
| Mała społeczność | Potrzeby, dostawy, lokalne przekonania, kontakt/świadkowie, ograniczone prawo, współpraca i konserwacja: [M04](docs/verification/m04-acceptance.md) |
| Praktyki i uczenie | Miejscowe pomiary, wybór metody, przekaz i rzeczywista konserwacja: [integracja](docs/verification/practice-in-prototype.md) |
| Dwie metody i historia | Most lub prom z fizyczną dostawą; jedna sytuacja zaopatrzenia reagująca na nieobecne osoby/miejsca/zasoby: [M05](docs/verification/m05-acceptance.md) |
| Zmiana reprezentacji | Jeden kurier pomiędzy statycznymi terminalami, zachowujący tożsamość, czas i ładunek: [SC-09](docs/verification/m05-distant-delivery.md) |
| Odbiór fundamentu | Debug/Release 435/435, ASan 437/437; dłuższy pomiar na MX330, viewport 844×781; granice i nieudane próby: [M06](docs/verification/m06-acceptance.md) |

Uwagi użytkownika do docelowej naturalności ruchu pozostają w [raporcie M03](docs/verification/m03-motion-navigation.md#ocena-użytkownika). Pełny solver brył, pełna przestrzeń 3D, dowolna skala i nieograniczona historia nie są zaimplementowane. M06 nie obejmował badania zainteresowania graczy. Nie rozszerzamy wyników poza opisane sceny.

## Od czego zacząć

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-debug
ctest --preset windows-debug
python tools/check_docs.py
python -m unittest discover -s tools/tests -p "test_*.py"
```

Natywne kontrole wymagają sprzętowego GPU i systemowych Narzędzi grafiki; instrukcje, ASan, Release oraz dokładne komendy są w [środowisku pracy](docs/development.md). [Reguły inżynierskie](engineering-rules.md) i [AGENTS.md](AGENTS.md) określają sposób zmian. [Mapa dokumentacji](docs/documentation-map.json) wskazuje właścicieli tematów.

| Przykład uruchomienia | Cel |
| --- | --- |
| `python tools/start_practice_playtest.py` | Wspólna ośmioosobowa scena z pomiarami, uczeniem i konserwacją |
| `python tools/start_practice_playtest.py --two-methods` | Dwie metody fizycznej dostawy: naprawa lub objazd promem |
| `python tools/start_distant_playtest.py` | Ograniczona podróż między lokalną i daleką reprezentacją |
| `python tools/start_story_playtest.py` | Historia zaopatrzenia i warianty zmienionego świata |
| `build/msvc/Debug/reality_lab.exe --motion` | Historyczny, osobny tor bazowego ruchu M03 |

Launchery wymagają binarium Release; w razie potrzeby zbuduj `cmake --build --preset windows-release`. Tworzą własne katalogi próby. Nie używaj prywatnego zapisu jako wyjścia testu.

## Laboratorium odkrywania

Wykonano trzy odrębne eksperymenty: [odkrywanie i przekaz](docs/verification/discovery-learning-experiment.md), [budowanie wyjaśnień](docs/verification/hypothesis-building-experiment.md) oraz [doświadczenia rozdzielające](docs/verification/discriminating-experiment.md). Zachowane są również negatywne kryteria H/A drugiego eksperymentu i T trzeciego. Autor dostarcza aparaturę oraz język modeli; wyniki nie dowodzą świadomości ani nieograniczonego rozwoju.

## Odbiór M04

[M04](docs/verification/m04-acceptance.md) zaakceptowano 27.10.2026; [M05](docs/verification/m05-acceptance.md) 29.10.2026, a techniczny [M06](docs/verification/m06-acceptance.md) 02.11.2026. Datowane raporty przyrostów zachowują stan swoich prób. Ich dawne „M04 nieukończone” nie jest aktualnym statusem projektu.

## Repozytorium i dokumenty źródłowe

Repozytorium jest publiczne w `origin`; `main` jest linią integracyjną, a `codex/<temat>` służą pracy. Sprawdzaj rzeczywisty stan Git zamiast wyciągać go z dawnego raportu. Publikację, dowody i sprzątanie opisuje [workflow](docs/workflow.md#repozytorium-git).

Rozmowy ChatGPT/Gemini, zewnętrzne analizy i archiwa zachowują pochodzenie oraz historyczne założenia. Ich interpretację i aktualność określają bieżący plan, karta projektu i rejestr decyzji. Nie przepisujemy surowych źródeł ani wyników pod dzisiejszy stan.
