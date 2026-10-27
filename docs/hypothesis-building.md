# Laboratorium budowania wyjaśnień — stan API

**Status: zaimplementowane programy, agent, sesja oraz natywne fixture'y kontynuacji. Runner całego eksperymentu i wyniki pozostają w realizacji.** Protokół [hypothesis-v1](work-items/hypothesis-building-experiment.md) jest zatwierdzony do implementacji i wykonania. [Plan](work-items/hypothesis-building-implementation.md) rozdziela wykonane zadania i dalsze prace. To osobny konsument badawczy, bez nowego odbioru M04/3D.

## Własność i granica informacji

`HypothesisLaw` opisuje prawdę aparatu. `HypothesisProgram` jest ciągiem operacji, `HypothesisTable` ma 129 wyjść dla wejść 0–128. Programy matematyczne pozostają abstrakcyjnym modelem pasywnego przepływu, bez tezy o elektromagnetyzmie.

`HypothesisAgent` przyjmuje wyłącznie własne `DiscoveryEvidence` ze źródłem i czasem. Nie posiada świata ani seedu. Początkowy F, generowanie po jego sprzeczności, kanoniczne klasy, minimaks oraz wszystkie limity mają semantykę z protokołu. `state()` jest diagnostycznym, kanonicznym stanem wnioskowania, nie kanałem obserwacji świata. Budżet wyczerpany w trakcie selekcji wraca do pierwotnego wejścia permutacji; częściowo przefiltrowane modele nie uszkadzają wcześniejszych zapisów programów.

`HypothesisSession` jest jedynym wykonawcą czasu, faz i RNG. Własny `TraversalSession` zachowuje ciało i sprawdza fizyczny dostęp do stołu. Pomiar pochodzi z zakończonego impulsu `DiscoveryWorld`, a nie z wywołania tabeli przez agenta. Dwa kroki oznaczają konfigurację i impuls; na starcie impulsu energia jest chroniona, po zakończeniu rozliczona dokładnie raz. Uczenie przyjmuje pojedynczy u i różne wejścia 1–64. Ocena wymaga zamrożenia i jawnej nowej sesji `evaluation_copy()`, z zachowaniem archiwalnej aparatury oraz wiedzy i skumulowanych liczników obliczeń. Nie jest to doładowanie starego świata.

## Formaty i kontynuacja

Nowy konstruktor tabelaryczny `DiscoveryWorld` używa `DWORLD 2`. Tablice są pasywne, impulsy przyjmują 1–128. Komendy, odmowy, ID, bilans i zapis historii mają jedną implementację wspólną z poprzednim aparatem. Konstruktor dawnych rodzin nadal zapisuje `DWORLD 1` identycznie; nie reinterpretujemy historycznych danych.

`GENESIS_HYPOTHESIS 1` jest osobną otoczką, bez zmiany `GENESIS_DISCOVERY 1` i `GENESIS_TRAVERSAL`. Zawiera prywatne prawo, seed, limity, początkowy świat, dziennik publicznych operacji i kanoniczny stan końcowy: aparat/archiwum, ruch, wiedzę, modele, fazę impulsu, ID, koszty i permutację z licznikiem RNG 41. Ładowanie odtwarza prywatnego kandydata, sprawdza wyniki deklaracji i zgodność całego stanu, a następnie publikuje kandydata. Poprawny checksum nie zastępuje walidacji semantycznej. Błędny odczyt zachowuje dotychczasowy stan. Odtwarzanie historii kosztuje CPU; nie jest darmowym obliczeniem ani dowodem skalowalności zapisu.

Strumienie generatora przypadków 40/42/43 mają należeć do eksperymentatora i manifestu próby; nie są kanałem wiedzy mieszkańca. Ich runner pozostaje w realizacji. Zegar świata nie zależy od czasu CPU zużytego na wnioskowanie.

## Istniejące polecenia

```powershell
cmake --build --preset windows-debug --target reality_hypothesis reality_hypothesis_tests
ctest --preset windows-debug -R '^hypothesis[.]' --output-on-failure
build/msvc/Debug/reality_hypothesis.exe --fixture --output NEW_DIRECTORY
build/msvc/Debug/reality_hypothesis.exe --continue CHECKPOINT --output NEW_DIRECTORY
```

`--fixture` tworzy osiem rzeczywistych checkpointów: dwa sterowniki, przed/po pierwszej sprzeczności oraz konfigurację/impuls w toku. Integracja uruchamia nowy proces dla każdego checkpointu i porównuje pełny końcowy stan z nieprzerwanym wykonaniem. To techniczny dowód kontynuacji, nie próba potwierdzająca H/A. Bieżąca pierwsza kontrola Debug: 7/7 PASS; pełne profile, review i wynik eksperymentu nie zostały jeszcze wykonane dla tego przyrostu.
