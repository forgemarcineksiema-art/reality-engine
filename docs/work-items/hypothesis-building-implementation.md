# Implementacja hypothesis-v1

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Wykonać zatwierdzony eksperyment budowania wyjaśnień, zachowując prawdziwe pomiary i wszystkie wyniki.

**Architecture:** Mały generator programów i agent nie mają dostępu do świata. Sesja wiąże wnioskowanie z rzeczywistym aparatem oraz istniejącym dostępem przestrzennym. Oddzielny Python przygotowuje przypadki i niezależnie ocenia surowe deklaracje/pomiary.

**Tech Stack:** C++20, MSVC/CMake, Python 3.11+ standard library, istniejące TraversalSession i DiscoveryWorld.

**Spec:** [Zatwierdzony protokół hypothesis-v1](hypothesis-building-experiment.md).

## Global Constraints

- 24 pomiary, wejścia 1–64, limit 1536 eu / 1000 kroków, sesja 8192 eu, krok 50 ms.
- Gramatyka: 27 operacji, długość 0–3, 20440 zapisów; dziedzina 0–128.
- Limit 100000 kandydatów prawdy na seed, 10 milionów zastosowań prymitywu i 300 milionów odczytów modeli na wariant.
- Bez dodatkowych zależności; jeden właściciel stanu; stare formaty i historyczne dowody zachowane.
- Progi H/A i seedy z protokołu nie zmieniają się pod wynik. Ocena nie aktualizuje wiedzy.

## Review Focus

1. Semantycznie błędny snapshot z poprawnym checksumem — odrzucenie transakcyjne; zadanie 3.
2. Limit w środku listy prognoz — zachowane wcześniejsze deklaracje, dalsze odmowy; zadania 2/4.
3. Równoważne drzewa i pozaobszarowa niejednoznaczność — klasy zamiast głosowania powielonymi zapisami; zadanie 1/2.
4. Niedozwolony dostęp i uczenie przy ocenie — odmowa bez skutku; zadanie 3.
5. Analiza przestawionych/nadpisanych danych — niezależne obliczenia, manifesty i jednorazowy run; zadanie 4.

## Pliki i granice

- `src/research/hypothesis_program.hpp/.cpp`: operacje, kanoniczne modele, gramatyka i interpretacja praw.
- `src/research/hypothesis_agent.hpp/.cpp`: lokalne obserwacje, filtrowanie/synteza, wybór prób, prognozy, budżety.
- `src/research/discovery_world.hpp/.cpp`: rzeczywisty konsument tabel pasywnych praw w osobnym DWORLD 2; historyczny DWORLD 1 zachowuje bajty i zachowanie.
- `src/research/hypothesis_session.hpp/.cpp`: ciało, impulsy, fazy, RNG, zapis i odczyt.
- `src/research/hypothesis_protocol.hpp/.cpp`, `src/headless/hypothesis_main.cpp`: uruchomienie przypadku i zapis surowych wyników.
- `tests/hypothesis_tests.cpp`, `tests/hypothesis_integration.py`: regresje domeny i kontynuacja w nowym procesie.
- `tools/run_hypothesis_experiment.py`, `tools/tests/test_hypothesis_experiment.py`: generator, freeze, runner, niezależny sędzia i analiza.
- CMake, bieżące API, raport, mapa dokumentacji i agenda: konsumenci oraz rzeczywisty stan.

### Task 1: Programy oraz rzeczywista aparatura

**Interfaces:** `HypothesisOp {kind, parameter}`, `HypothesisProgram = vector<HypothesisOp>`, `HypothesisTable = array<uint16_t,129>`; `hypothesis_apply(program,input)`, `hypothesis_grammar()`, `hypothesis_flat_models()`; `DiscoveryWorld(array<HypothesisTable,3>)` wykonuje i rozlicza nowe prawo, bez udostępnienia praw agentowi.

- [x] Dodać testy gramatyki, równoważności przykładu 96→40, 20440 zapisów, poza-językowych kwadratów, pasywności i księgi aparatu. Dodać CTest `hypothesis.program` / `hypothesis.apparatus`.
- [x] Build/test wykazuje brak implementacji; zachować log RED.
- [x] Zaimplementować programy i tablice. W aparaturze zachować wspólne ID, dwie fazy, rezerwację energii i chronologię. DWORLD 2 zawiera tablice; DWORLD 1 pozostaje niezmieniony.
- [x] CTest nowych przypadków i wszystkich discovery PASS; historyczna kontynuacja i ponowienie komendy bez podwójnego skutku.
- [x] Commit źródeł/testów i rzeczywistego wyniku w dzienniku poniżej.

### Task 2: Synteza i wybór prób

**Interfaces:** `HypothesisMode {FlatActive,SynthesisRandom,SynthesisActive}`; `HypothesisAgent(EntityId,HypothesisMode,HypothesisLimits={})`; `observe(DiscoveryEvidence)`, `choose_input(span<const unsigned>)`, `predict(DiscoveryChain,unsigned)`, `choose_chain(unsigned,unsigned)`, `freeze()`, `state()`.

- [x] Testy RED: obalenie F, zgodna nowa klasa, niejednoznaczność i zakres prognoz, wybór minimaksowy, SR bez ukrytego aktywnego selektora, trwały limit przed przekroczeniem i odmowy po freeze.
- [x] Zaimplementować kanoniczny zbiór F oraz generowanie po jego obaleniu. Porządek i liczniki są deterministyczne; żadnych światów ani seedów w agencie.
- [x] Testować także limit podczas prognoz i wyboru konfiguracji, brak odświeżenia budżetu przy fazie oceny oraz 25 dopuszczonych konfiguracji.
- [x] Uruchomić przypadki PASS i commit.

### Task 3: Sesja i kontynuacja

**Interfaces:** `HypothesisSession(law,mode,seed,scene=make_discovery_scene(),limits={})`; `begin_probe(chain,input,learning)`, `advance(ticks)`, `choose_input()`, `predict(...)`, `choose_chain(...)`, `freeze()`, `evaluation_copy()`, `serialize()`, `restore(bytes)`, `save(path)`, `load(path)`.

- [x] Testy RED: konfiguracja/impuls/obserwacja wyłącznie na miejscu, limit 24 i 1536, 1–64 w uczeniu, zakaz oceny bez freeze i nauki po freeze, ciągłość ID i bilans.
- [x] Implementacja: TraversalSession jest jedynym właścicielem położenia i czasu; DiscoveryWorld jedyną aparaturą; agent dostaje tylko zakończony własny pomiar.
- [x] Zapis w osobnej otoczce `GENESIS_HYPOTHESIS 1`, z pełnymi generatorami i stanem obliczeń. Dopuszczalne deterministyczne odtworzenie prywatnego kandydata z historii, jeśli odczyt sprawdza kanoniczny stan i nie publikuje skutków przed walidacją.
- [x] Testy przed/po sprzeczności, konfiguracji i impulsu, niezerowego RNG oraz modyfikacji semantyki przy poprawnym checksumie. Po nieudanym odczycie stary stan jest identyczny.
- [x] Nowy proces kontynuuje rzeczywisty zapis do identycznego wyniku; PASS i commit.

### Task 4: Przypadki, niezależny sędzia i surowe dowody

**Interfaces:** `run_hypothesis_case(path,output)`, fixture/continue w `reality_hypothesis`; Python `prepare`, `run`, `analyze`, z nowym katalogiem próby i freeze/marker. Native plik przypadku zawiera prawdę dostępną tylko sesji oraz plan oceny; agent dostaje plan prognoz i cele wyłącznie w dozwolonych fazach.

- [x] Testy RED generatora i sędziego: zgodność wszystkich wejść z C++, liczności, seedy/odrzucenia, brak zmian kryteriów, dokładne granice progów, manifest i odmowa powtórnego uruchomienia.
- [x] Oddzielić strumienie 40/41/42/43, zachować ich końcowe liczniki i historię odrzuceń. Modele prawdy i zadania potwierdzające dopiero po freeze.
- [x] Native zapisuje pomiary, wyjściowe deklaracje D po 3–24 próbach, rzeczywiste G po freeze, wybory aktywne i liczniki/koszty, niezależne ledgery uczenia/oceny oraz snapshoty.
- [x] Python odtwarza działania praw, bilanse, wyniki D/G, prognozy i wybór aktywny bez zaufania podsumowaniom C++. Weryfikuje też dane/manifest/freeze przy `analyze`.
- [x] Test manipulacji dowodem musi dać błąd, wynik naukowo negatywny poprawny exit 0. PASS i commit.

### Task 5: Weryfikacja techniczna i review

- [ ] Rozwój: 16 światów, wszystkie kontrole; zachować porażki i pomiar kosztu. Bez oglądania praw potwierdzających.
- [ ] `cmake --preset windows-msvc`, build Debug/Release, pełne CTest obu profili, ASan build i pełne CTest, `format-check`, dokumentacja i testy Pythona.
- [ ] Nowe API i raport odróżniają odtwarzanie stanu, koszt wnioskowania oraz koszty fizycznych prób. Brak nowego odbioru 3D.
- [ ] Osobny świeży przegląd całej gałęzi przez recenzenta; każda materialna poprawka RED→GREEN oraz ponowna właściwa weryfikacja przed freeze. Zachować uwagi i ich rozstrzygnięcia.

### Task 6: Zamrożenie i jednorazowa próba

- [ ] Zamrozić zatwierdzony protokół, źródła, narzędzia, binaria, generator i zakończone review, z SHA-256 oraz manifestem.
- [ ] Wygenerować i wykonać 64 światy potwierdzające dopiero po freeze. Znacznik chroni przed cichym rerunem.
- [ ] Niezależnie przeliczyć wynik, zachować kontrole i wszystkie porażki, opublikować raport z H i A osobno oraz ograniczeniami kosztu i reprezentacji.
- [ ] Zarejestrować dowody i wyniki; finalna kontrola dokumentów i czystego stanu Git. Bez push/merge bez osobnego polecenia.

## Dziennik wykonania

- 27.10.2026: użytkownik zatwierdził protokół do implementacji i wykonania. Plan nie zmienia progów ani seedów. Wykonanie będzie prowadzone w tym czacie na istniejącej, czystej gałęzi tematycznej; brak potrzeby równoległej edycji. Osobny recenzent sprawdzi całość przed próbą.
- Rozmowa ABSS z załącznika jest materiałem do oceny, nie zleceniem rozszerzenia zakresu o ekologię/ALife/OEE.
