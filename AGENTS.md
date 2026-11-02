# Repository Guidelines

## Aktualny stan i decyzje

**Pierwszy prototyp techniczny M00–M06 został ukończony i odebrany w opisanym zakresie.** Status etapów i ich dowody posiada wyłącznie [plan prototypu](docs/prototype-plan.md). [Raport M06](docs/verification/m06-acceptance.md) dokumentuje pełne profile, długi pomiar aplikacji i kontynuację końcowego świata. Zaliczenie prototypu nie oznacza ukończenia docelowej gry ani całej autonomicznej cywilizacji.

**Dalszy kierunek pozostaje otwarty.** Użytkownik 02.11.2026 wyjaśnił, że nie wybrał jeszcze najlepszego następnego kroku. Pełne 3D nie jest zatwierdzonym następnym projektem ani obowiązującym etapem nadrzędnym. Nie rozpoczynaj nowej fizyki, migracji przestrzeni, większej skali lub nowego programu badań na podstawie dawnych propozycji. Aktualną korektę zapisuje DEC-019 w [rejestrze decyzji](docs/decisions.md).

Obecny silnik ma własny kinematyczny ruch 2,5D, kolizje/nawigację, potrzeby, fizyczne dostawy, lokalną wiedzę i świadków, ograniczone uczenie, konserwację, zmianę reprezentacji jednego kuriera, historię oraz zapis. Pełny solver brył i pełne 3D nie istnieją. [Odbiór M03](docs/verification/m03-motion-navigation.md#ocena-użytkownika) zachowuje uwagi do reakcji na cel, płynności trasy, szybkości i naturalności ruchu; nie rozszerzaj go na docelową lokomocję. M04 i M05 mają własne [raporty](docs/verification/m04-acceptance.md) oraz [zakres akceptacji](docs/verification/m05-acceptance.md).

## Kolejność lektury i właściciele

Przed pracą przeczytaj [README](README.md), [reguły inżynierskie](engineering-rules.md) i [kartę projektu](docs/project-charter.md). Następnie wybierz dokument odpowiedzialny za temat według [mapy dokumentacji](docs/documentation-map.json). [Kontrakty](docs/subsystem-contracts.md), [testy](docs/testing-strategy.md), [budżety](docs/performance-budget.md) i plan etapów mają odrębne odpowiedzialności. Datowane raporty i źródła rozmów opisują stan swojej próby; nie zastępują aktualnych ustaleń.

- `WorkWorld` posiada zasoby, rezerwacje, koszty, transfery i ich trwałość: [model domeny](docs/work-domain.md).
- `TraversalSession` posiada potrzeby, lokalną wiedzę, plany, podróż i historię: [model autonomii](docs/traversal-autonomy.md).
- Fizyka/nawigacja posiadają ruch i kolizje: [model ruchu](docs/motion-navigation.md). Renderer, UI i diagnostyka są konsumentami stanu.
- Komendy, presety i interpretacja pomiarów: [środowisko pracy](docs/development.md). Liczby z dawnych raportów nie są aktualną liczbą wszystkich testów ani benchmarkiem FPS.

## Kontrakty wymagające zachowania

Zachowuj fizyczną partię ładunku, niezmienny typ materiału, chronologiczne bilanse, wspólną przestrzeń ID komend oraz odczyt historycznych formatów. Anulowanie zachowuje materiał; automatyczny zwrot wymaga znanego zgodnego stanowiska. Ręczne przejęcie i `stop()` nie teleportują partii. Odpoczynek używa wielorazowej pojemności, materiał jest zużywany. Starszych zapisów materiałowych nie reinterpretuj jako pojemności. Szczegóły: [transfer i dostawa](docs/verification/material-delivery.md), [pojemność](docs/verification/reusable-capacity.md).

NPC korzystają z miejscowej obserwacji i kontaktu, nie z globalnego zapasu ani prawdy obserwatora. Zachowuj świadka, pośrednika i osobne czasy obserwacji/poznania; przekaz nie odmładza informacji. Fizyczna możliwość przejęcia ładunku i prawo pozostają rozdzielone. Chronione Step, transport, czynność i clearance nie są przerywane nową wiadomością lub zmianą zamiaru. Obowiązują lokalne przekonania o przejściach, bezpieczna zajętość i rzeczywisty koszt konserwacji. Właściciele: [wiedza](docs/work-items/local-station-knowledge.md), [przejścia](docs/work-items/passage-logistics.md), [prawo](docs/work-items/witness-property-law.md), [współpraca](docs/work-items/local-clearance.md).

Zachowuj odłożone zamiary potrzeb, terminy ponowień i jednorazową granicę końcowego dojścia. Przy zakończeniu celu zwalniaj nawigację i nie zostawiaj trasy bez celu. Diagnostyka jest obserwatorem, nie sterowaniem NPC. Wcześniejsze reprodukcje i nieudane pomiary pozostają dowodami. Szczegóły: [bezpieczne potrzeby](docs/work-items/diagnostics-needs-hardening.md), [odzyskiwanie](docs/verification/retained-world-navigation-stall.md), raport M06.

Utrzymuj wspólną geometrię kolizji i tras oraz historię rzeczywistego przebiegu kroku do prezentacji; nie zastępuj jej cięciwą przez narożnik. Surowe A* ma referencję BFS, skracanie używa tego samego sweepu. Historyczny tor `MotionSession` ma osobną wersję zapisu i zachowuje zegar 50 ms/prędkość 3 m/s. Przybliżenia i inne parametry określają modele, nie deklaracja większej naturalności. [Audyt ruchu](docs/verification/motion-adversarial-audit.md) i [audyt wielu osób](docs/verification/traversal-multi-actor.md) pozostają wymaganymi regresjami.

W uczeniu agent otrzymuje pomiary, a nie seed i parametry świata. Zachowuj rozdział odkrywania/oceny, fizyczny kontakt, źródła/czasy, pełny stan generatorów oraz koszt nauczyciela. Nie zmieniaj potwierdzających zbiorów ani kryteriów pod wynik. Wyniki negatywne są wynikami badania; znana rodzina modeli nie dowodzi świadomości lub nieznanej ludzkości fizyki. Właściciele: [laboratorium](docs/discovery-learning.md), [praktyki](docs/work-items/practice-in-prototype.md) i właściwe protokoły.

Przejście lokalny/daleki ma jednego wykonawcę i tę samą partię; historyczna kotwica nie jest lokalnym ciałem ani świadkiem. Historia reaguje na rzeczywistych uczestników, miejsce i zasób, bez odtwarzania braków. Zachowaj transakcyjne wczytanie, chronione operacje oraz zgodną kontynuację. Szczegóły: [SC-09](docs/work-items/m05-distant-delivery.md), [historia](docs/story-scene.md), [audyt SC-08](docs/verification/sc08-continuity-audit.md).

## Sposób pracy i dowód ukończenia

Przed zmianą zachowania nazwij cel, zakres plików, kontrakty i dowód odbioru. Zachowaj cudze zmiany. Nie dodawaj pustych frameworków, atrap produkcji ani zależności omijających własną fizykę/nawigację. Gracz i NPC mają wspólną semantykę, a każde pole stanu jednego właściciela. Zmiana kontraktu obejmuje konsumentów, testy i dokumentację. Zwykłe naprawy zgodne z kontraktem wykonuj bez zbędnych pytań; zmianę kierunku uzgodnij z użytkownikiem.

Nie usuwaj porażek przez skip, osłabienie asercji lub cichy fallback. Zachowuj dotychczasowe regresje i limity, w tym dwuetapowy test dawnej dostawy z checkpointem po 300 krokach i limitem 20 s. Niewykonane oznacza nieweryfikowane. Test headless nie dowodzi działania obrazu, zrzut nie dowodzi bilansów, a automat nie zastępuje oceny człowieka tam, gdzie jest ona rzeczywiście wymagana. M06 odebrano jako fundament techniczny, bez bramki uczestników/grywalności. Wynik innego agenta wymaga przeglądu i sprawdzenia integracji.

Repozytorium jest publikowane w `origin` na GitHubie. Pracuj na `codex/<temat>`, a status bieżącej gałęzi sprawdzaj Git-em. Main, publikację i bezpieczne sprzątanie opisuje [workflow](docs/workflow.md#repozytorium-git). Nie usuwaj dowodów lub niepołączonej pracy; archiwa zachowują dokładne bajty. Silnik/systemy pozostają priorytetem, grafika ma najniższy priorytet. Kierunek fantasy z DEC-008 nie zatwierdza implementacji całej magii, instytucji lub walki.

## Rzeczywiste kontrole

Z katalogu głównego:

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-debug
ctest --preset windows-debug
python tools/check_docs.py
python -m unittest discover -s tools/tests -p "test_*.py"
```

Dobierz kontrole do zmiany według reguł i strategii testów. Sama proza wymaga kontroli dokumentacji; nie uzasadnia ponownego benchmarku ani pozornych testów C++. Profile ASan, pojedyncze testy i styl określa instrukcja środowiska. Raportuj rzeczywiste komendy, wyniki, ograniczenia i pozostałe części.
