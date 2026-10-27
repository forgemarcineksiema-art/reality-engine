# Laboratorium budowania wyjaśnień — stan API

**Status: zaimplementowane i zweryfikowane programy, agent, sesja, runner i niezależny sędzia; próba wykonana.** [Raport](verification/hypothesis-building-experiment.md) zachowuje wyniki, niespełnione kryteria H/A i ograniczenia. Protokół [hypothesis-v1](work-items/hypothesis-building-experiment.md) pozostaje właścicielem metody. [Plan](work-items/hypothesis-building-implementation.md) wskazuje wykonanie. To osobny konsument badawczy, bez nowego odbioru M04/3D.

## Własność i granica informacji

`HypothesisLaw` opisuje prawdę aparatu. `HypothesisProgram` jest ciągiem operacji, `HypothesisTable` ma 129 wyjść dla wejść 0–128. Programy matematyczne pozostają abstrakcyjnym modelem pasywnego przepływu, bez tezy o elektromagnetyzmie.

`HypothesisAgent` przyjmuje wyłącznie własne `DiscoveryEvidence` ze źródłem i czasem. Nie posiada świata ani seedu. Początkowy F, generowanie po jego sprzeczności, kanoniczne klasy, minimaks oraz wszystkie limity mają semantykę z protokołu. `state()` jest diagnostycznym, kanonicznym stanem wnioskowania, nie kanałem obserwacji świata. Budżet wyczerpany w trakcie selekcji wraca do pierwotnego wejścia permutacji; częściowo przefiltrowane modele nie uszkadzają wcześniejszych zapisów programów.

`HypothesisSession` jest jedynym wykonawcą czasu, faz i RNG. Własny `TraversalSession` zachowuje ciało i sprawdza fizyczny dostęp do stołu. Pomiar pochodzi z zakończonego impulsu `DiscoveryWorld`, a nie z wywołania tabeli przez agenta. Dwa kroki oznaczają konfigurację i impuls; na starcie impulsu energia jest chroniona, po zakończeniu rozliczona dokładnie raz. Uczenie przyjmuje pojedynczy u i różne wejścia 1–64. Ocena wymaga zamrożenia i jawnej nowej sesji `evaluation_copy()`, z zachowaniem archiwalnej aparatury oraz wiedzy i skumulowanych liczników obliczeń. Nie jest to doładowanie starego świata.

## Formaty i kontynuacja

Nowy konstruktor tabelaryczny `DiscoveryWorld` używa `DWORLD 2`. Tablice są pasywne, impulsy przyjmują 1–128. Komendy, odmowy, ID, bilans i zapis historii mają jedną implementację wspólną z poprzednim aparatem. Konstruktor dawnych rodzin nadal zapisuje `DWORLD 1` identycznie; nie reinterpretujemy historycznych danych.

`GENESIS_HYPOTHESIS 1` jest osobną otoczką, bez zmiany `GENESIS_DISCOVERY 1` i `GENESIS_TRAVERSAL`. Zawiera prywatne prawo, seed, limity, początkowy świat, dziennik publicznych operacji i kanoniczny stan końcowy: aparat/archiwum, ruch, wiedzę, modele, fazę impulsu, ID, koszty i permutację z licznikiem RNG 41. Ładowanie odtwarza prywatnego kandydata, sprawdza wyniki deklaracji i zgodność całego stanu, a następnie publikuje kandydata. Poprawny checksum nie zastępuje walidacji semantycznej. Błędny odczyt zachowuje dotychczasowy stan. Odtwarzanie historii kosztuje CPU; nie jest darmowym obliczeniem ani dowodem skalowalności zapisu.

Strumienie generatora przypadków 40/42/43 należą do eksperymentatora i manifestu próby; zapis przypadku zachowuje ich liczniki, plan oceny i historię odrzuceń. Nie są kanałem wiedzy mieszkańca. Zegar świata nie zależy od czasu CPU zużytego na wnioskowanie.

## Istniejące polecenia

```powershell
cmake --build --preset windows-debug --target reality_hypothesis reality_hypothesis_tests
ctest --preset windows-debug -R '^hypothesis[.]' --output-on-failure
build/msvc/Debug/reality_hypothesis.exe --fixture --output NEW_DIRECTORY
build/msvc/Debug/reality_hypothesis.exe --continue CHECKPOINT --output NEW_DIRECTORY
build/msvc/Debug/reality_hypothesis.exe --case CASE_FILE --output NEW_DIRECTORY
python tools/run_hypothesis_experiment.py prepare --mode development --exe build/msvc/RelWithDebInfo/reality_hypothesis.exe --output build/hypothesis-development-NEW
python tools/run_hypothesis_experiment.py run build/hypothesis-development-NEW
python tools/run_hypothesis_experiment.py analyze build/hypothesis-development-NEW
```

`--fixture` tworzy osiem rzeczywistych checkpointów: dwa sterowniki, przed/po pierwszej sprzeczności oraz konfigurację/impuls w toku. `--continue` wykonuje kontynuację tego fixture'a. Ogólne ładowanie stanu zapewnia API sesji. Integracja uruchamia nowy proces dla każdego checkpointu i porównuje pełny końcowy stan z nieprzerwanym wykonaniem. Osobna integracja przypadku sprawdza surowe deklaracje, wybory prób, koszty i bilanse niezależnym kodem Pythona oraz odrzuca zmieniony pomiar. Dziesięć nowych przypadków, pełne profile oraz wynik z danymi opisuje [raport](verification/hypothesis-building-experiment.md).

Runner wymaga nowego katalogu, kopiuje binarium i zamrożone źródła z SHA-256 przed generowaniem przypadków, a `run` zapisuje znacznik jednorazowego wykonania. `analyze` odtwarza wyniki z pomiarów, sprawdza generowanie przypadków i dokładne progi, manifesty oraz związanie freeze z markerem. Zmieniony później skrypt lub jego dwie lokalne zależności odmawiają analizy dawnych danych; można uruchomić kopię `frozen-source/tools/run_hypothesis_experiment.py` z danej próby. Czytnik nie tworzy bytecode cache w zamrożonym pakiecie. Tryb potwierdzający wymaga strukturalnego rekordu review JSON przez `--review PATH`: dokładny `status: READY`, protokół, commit review i zgodny komplet hashy zweryfikowanych źródeł. Wymaga także poprawnego rozwojowego dowodu końcowego kodu przez `--development DIRECTORY` i czystych zapisanych źródeł. Freeze zawiera konfigurację, kompilator MSVC, CMake, MSBuild, architekturę i flagę ASan z rzeczywistego buildu. Wynik negatywny nie jest błędem infrastruktury.

Sesja rezerwuje miejsce w ograniczonym dzienniku na oba pozostałe kroki przyjętego impulsu. Prognozy nie mogą zużyć tej rezerwy. Regresje obejmują także przerwanie listy deklaracji: wcześniejsze odpowiedzi pozostają, wyczerpany budżet przechodzi przez zapis i ocenę, a fizyczne pomiary nadal są wykonywane do limitu 24.
