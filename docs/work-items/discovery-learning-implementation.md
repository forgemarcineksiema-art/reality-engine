# Discovery learning implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax for tracking. Implementacja w bieżącej gałęzi jest autoryzowana zatwierdzeniem protokołu przez użytkownika; nie wymaga ponownego wyboru metody wykonania.

**Goal:** Wykonać zatwierdzony eksperyment D/G/S, zachowując również wynik negatywny.

**Architecture:** Aparatura posiada energię i skutki impulsów; agent posiada wyłącznie pomiary i hipotezy; sesja komponuje je z rzeczywistą TraversalSession. Konsument C++ wykonuje przypadek, a narzędzie Python tworzy/zamraża manifest i analizuje porównania parami. Sędzia i generator pozostają poza interfejsem agenta.

**Tech Stack:** C++20/MSVC, istniejące biblioteki Genesis, Python 3.11+ standard library; bez nowych zależności.

**Spec:** [Zatwierdzony protokół discovery-v1](discovery-learning-experiment.md).

## Global Constraints

- Krok 50 ms, ruch i kontakt wspólne z TraversalSession. Brak nowej fizyki brył i obrazu 3D.
- Źródło 8192 eu na odrębną sesję; model elementów i seedy dokładnie według protokołu.
- A: do 24 prób / 1536 eu / 1000 kroków; B: do 2 prób / 128 eu / 1000 kroków. Ocena: 2000 kroków.
- Pakiet do 6 rzeczywistych pomiarów odkrywania; bez modelu A i bez ocen A. Jedyny kanał to kontakt do 2 m sprawdzany geometrią.
- D: 24 prognozy; G/S: po 20 celów; 16 seedów rozwojowych i 64 potwierdzające. Progi i porównania bez zmian.
- Nie uruchamiać potwierdzających seedów przed techniczną weryfikacją, review oraz zamrożeniem kodu/protokołu/manifestu.
- Stare formaty, limity CTest i dowody zachować. Błąd wczytania nie podmienia sesji.

## Review Focus

- Powtórzenie impulsu po zapisie nie powoduje drugiego zużycia ani obserwacji (Task 1/3).
- Powtórzony symbol w łańcuchu oznacza ten sam nieznany parametr, a nie kolejne niezależne losowanie (Task 2).
- Model zamrożony do oceny nie uczy się z następnego wyniku, również po kontynuacji (Task 2/3).
- Pakiet nie zawiera ocen A, nie tworzy danych z prawdy i nie przechodzi przez ścianę (Task 3).
- Analiza nie zwiększa liczby niezależnych prób liczbą zadań, zachowuje remisy i ujemny wynik (Task 4).

---

### Task 1: Aparatura i bilans

**Files:** create `src/research/discovery_world.hpp/.cpp`, `tests/discovery_tests.cpp`; modify `CMakeLists.txt`.

**Interfaces:** `DiscoveryRule`, `DiscoveryChain`, `DiscoveryMeasurement`; `DiscoveryWorld::configure`, `pulse`, `advance`, `validate`, `serialize/restore`. Wspólne komendy mają ID; impuls trwa jeden krok po konfiguracji. W toku impulsu energia jest rezerwacją; rozliczenie następuje raz przy końcu.

- [ ] Napisać przypadki `world`, `replay`, `refusals`, `world_restore`: ręczne wyniki trzech rodzin, kolejność łańcucha, bilans, rezerwacja, konflikt komendy, odmowy bez mutacji, zapis w trakcie impulsu.
- [ ] Skonfigurować test i zachować czerwony build braku implementacji.
- [ ] Zaimplementować konkretne komendy i walidację; zachować zielone wyniki `ctest --preset windows-debug -R '^discovery[.](world|replay|refusals|world_restore)$'`.
- [ ] Przejrzeć źródła i zapisać mały commit.

### Task 2: Wiedza i decyzje

**Files:** create `src/research/discovery_agent.hpp/.cpp`; extend `tests/discovery_tests.cpp`.

**Consumes:** `DiscoveryChain`, pomiar wejścia/wyjścia ze źródłem. **Produces:** `DiscoveryAgent::observe`, `predict`, `choose_probe`, `choose_chain`, `teaching_packet`, `freeze`, `serialize/restore`.

- [ ] Napisać przypadki `learning`, `minimax`, `composition`, `frozen`, `outside`: początkowe 75 hipotez, własne pomiary odrzucają sprzeczności, minimax całego układu, powtórzony symbol, deduplikacja i źródło, brak uczenia po freeze, jawne model_inadequate.
- [ ] Zachować czerwony build/test przed implementacją.
- [ ] Zaimplementować skończony model i wybór maksymalnie sześciu pomiarów. Żadnej referencji agenta do świata/sędziego ani seeda.
- [ ] Uruchomić powyższe testy w Debug; oczekiwane wszystkie PASS. Zapisać commit.

### Task 3: Sesja przestrzenna i kontynuacja

**Files:** create `src/research/discovery_session.hpp/.cpp`, `src/research/discovery_codec.cpp`; extend test; modify `src/lab/traversal_session.hpp/.cpp` przez wąskie odczytowe zapytania i CMake.

**Consumes:** świat i agent; `TraversalSession` jako jedyny właściciel ruchu/ID/zegara. **Produces:** `DiscoverySession::advance`, `probe`, `train`, `meet_and_share`, `serialize/restore`, kontrolowany widok agenta.

- [ ] Napisać `access`, `contact`, `isolation`, `continuation`, `corruption`, `blocked_share`, `budgets`: dojście do stołu, kontakt po drodze, odmowa przez ścianę/inną podporę/odległość, brak danych przed kontaktem, źródło i czas, równe decyzje przy nieobserwowanej różnicy prawdy, zamrożony model i kopie checkpointu.
- [ ] Zachować czerwony build/test; implementować istniejące kontrole zamiast konkurencyjnej geometrii.
- [ ] Nowy zapis `GENESIS_DISCOVERY 1` zawiera otoczkę podróży, aparaturę, pamięć, fazę/liczniki i generatory, z checksumem i odczytem kandydatowym. Odtworzyć hipotezy z legalnych danych, bez zaufania do zapisanych parametrów agenta.
- [ ] Zielony zestaw discovery w Debug i commit. Brak teleportacji/postępów świata w samym wnioskowaniu.

### Task 4: Runner, zamrożenie i dowód

**Files:** create `src/headless/discovery_main.cpp`, `src/research/discovery_protocol.hpp/.cpp`, `tools/run_discovery_experiment.py`, `tools/tests/test_discovery_experiment.py`, `tests/discovery_integration.py`; modify CMake and documentation.

**Consumes:** sesja i protokół. **Produces:** `reality_discovery --case PATH --output NEW_DIRECTORY`, tryb kontynuacji fixture; Python `prepare`, `run`, `analyze` z jawnymi trybami development/confirmatory i plikami manifestu.

- [ ] Napisać najpierw testy Pythona generatora/domen, testu znaków z remisami i wyniku negatywnego; zachować czerwony import/test.
- [ ] Runner wykonuje D, G, S, losową eksplorację i losowe działanie; zapisuje surowe prognozy/konfiguracje/pomiary oraz koszty. Weryfikować spodziewane wyniki tylko na seedach rozwojowych.
- [ ] Integracja świeżego procesu porównuje dalszą kontynuację z oryginalnym procesem, w tym impuls w toku i niezależne źródło wiedzy; wadliwy plik daje błąd bez sukcesu CLI.
- [ ] Wykonać Debug/Release/ASan, format, dokumentację i narzędzia; zachować pełne logi i wcześniejsze porażki. Bez zmiany dawnych limitów i bez podmiany porażek hipotez na zielone testy.
- [ ] Osobny review całej zmiany przed próbą potwierdzającą; naprawić wykazane usterki z czerwonymi regresjami i powtórzyć właściwe kontrole.
- [ ] Zamrozić źródła, binarium Release, protokół i manifest SHA-256, uruchomić 64 przypadki raz, obliczyć wyniki według protokołu. Każde dalsze uruchomienie oznaczyć jako powtórzenie, zachować oryginał.
- [ ] Raport `docs/verification/discovery-learning-experiment.md`: rzeczywiste komendy, dane, D/G/S, koszty, kontrola negatywna, ograniczenia i nieoczekiwane obserwacje. Aktualizować `docs/development.md`, program badań, mapę i wejście README; commit bez push/merge.

## Ledger i decyzje wykonania

Postęp oraz rzeczywiste czerwone/zielone przebiegi zachowujemy w `build/discovery-implementation/`, a istotne logi i wyniki kopiujemy do wersjonowanych dowodów raportu. Jest to dostosowanie narzędzi pracy do istniejącego repo i PowerShell, nie zmiana protokołu.

Plan jest listą wykonania tego eksperymentu, nie drugim planem etapów Genesis. Zatwierdzenie metody przez użytkownika jest autoryzacją implementacji i wykonania tej próby.
