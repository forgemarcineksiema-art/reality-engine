# Plan implementacji eksportu próbek pomiarowych

> Wykonanie: `executing-plans` i TDD, samodzielna implementacja oraz jeden niezależny przegląd całej zmiany.

**Cel:** eksportować próbki czasu i pamięci z rozgrzewki oraz właściwego pomiaru, zachowując dotychczasowy raport i semantykę świata.

**Architektura:** aplikacja posiada opcjonalny `MeasurementTrace`, niezależny od `RuntimeMetrics` obsługującego istniejące podsumowania. Eksport powstaje wyłącznie w nowym, jawnie wskazanym katalogu. Obserwator nie jest częścią zapisu świata i nie steruje mieszkańcami.

**Stos:** istniejące C++20, Win32/D3D11 i narzędzia Python standard library; bez nowych zależności.

**Specyfikacja:** [zatwierdzony zakres przygotowania M06](m06-acceptance.md#zakres-pierwszego-zadania-implementacyjnego).

## Kontrakt

- Plan dotyczy pierwszego przyrostu eksportu. Oddzielne pomiary GPU, uczenia, tras i zapisu oraz długi benchmark pozostają dalszym zakresem M06.
- Nowa opcja `--measurement-trace DIR` jest dozwolona tylko z pomiarem. Katalog nie może istnieć; jego rodzic musi istnieć. Błąd powoduje kod wyjścia 1, bez uznania eksportu za wykonany.
- `DIR/samples.csv` i `DIR/status.txt` są tworzone bez nadpisywania. Zarezerwowanych ścieżek nie można jednocześnie wskazać jako raportu, zapisu, końcowego zapisu, zrzutu lub katalogu diagnostyki. Status jest osobnym dowodem również wtedy, gdy raport nie działa.
- CSV wersji 1 ma nagłówek `elapsed_ms,tick,phase,frame_ms,simulation_ms,backlog_ms,working_set_bytes,private_bytes`. `elapsed_ms` oznacza koniec klatki względem początku pracy pętli; faza wynika z początku klatki, zgodnie z dotychczasowym kryterium okresu mierzonego.
- Fazy to `warmup` i `measurement`. Tick jest odczytem świata po aktualizacji i może się cofnąć po odczycie zapisu; porządek próbek wyznacza monotoniczny czas rzeczywisty.
- Pamięć jest odczytywana na końcu odpowiedniej klatki, nominalnie raz na sekundę. Nieodczytane wartości pozostają pustymi polami, a nie zerem. Dotychczasowe statystyki pamięci dotyczą nadal tylko okresu mierzonego.
- Nowy obserwator waliduje dane przed ich dodaniem. Obejmuje istniejący limit 1 000 000 próbek; nie usuwa starych ani wolnych próbek po osiągnięciu limitu.
- Eksport następuje przy normalnym zakończeniu oraz przy obsłużonym przerwaniu/błędzie pętli, z jawnym statusem kompletności. Błędy eksportu są widoczne i nie nadpisują zachowanego pliku świata. Nagłe zakończenie procesu przez system nie gwarantuje opróżnienia bufora.
- W logu podać liczbę próbek, status i czas eksportu. Pełny koszt eksportu po zamknięciu okna pomiaru jest odrębny od czasów klatek.
- Dane i `status.txt` muszą zostać zapisane przed próbą logowania eksportu. Błąd raportu nadal daje kod 1; nie może blokować ratowania sprawnego CSV. Końcowe opróżnienie raportu jest jawne. Ten warunek doprecyzował niezależny przegląd i regresja z blokadą pliku.
- Chronione są INV-02, INV-08, INV-11, INV-12 i INV-16 z [kontraktów](../subsystem-contracts.md), stare formaty oraz istniejące limity CTest.

## Ryzyka do sprawdzenia w końcowym przeglądzie

- Kolizja nowych ścieżek z raportem, zapisem lub istniejącym dowodem; także ścieżki względne i różnice wielkości liter na Windows.
- Błąd eksportu po poprawnym działaniu świata nie może otrzymać kodu sukcesu ani komunikatu poprawnego zamknięcia.
- Przywrócenie zapisu może cofnąć tick; obserwator nie może przez to blokować poprawnego świata.
- Próbki rozgrzewki nie mogą powiększyć istniejących liczników/podsumowań okresu mierzonego.
- Zamknięcie okna przed końcem czasu ma zachować dostępny ślad z oznaczeniem niepełnej próby.

## Task 1: Eksport próbek w aplikacji

Pliki: `src/core/runtime_metrics.hpp`, `src/core/runtime_metrics.cpp`, `src/platform/traversal_main.cpp`, `tests/traversal_diagnostics_tests.cpp`, `tools/test_traversal_diagnostics.py`, `CMakeLists.txt`, instrukcja uruchomienia i raport przyrostu.

Interfejsy: `MeasurementPhase`, `MeasurementSample`, `MeasurementTrace::record`, `MeasurementTrace::csv`, `MeasurementTrace::size`; obecne `RuntimeMetrics::record/frames/report` pozostają zgodne. Konsument natywny przekazuje rzeczywisty czas, tick i odczyty procesu.

### Ujawniony w weryfikacji koszt inicjalizacji

Powtarzalny timeout istniejącego `two_methods.fresh_detour` wymaga ograniczonego usprawnienia przed odbiorem całej macierzy. Dodatkowe pliki: `src/research/contrast_agent.cpp`, `tests/contrast_tests.cpp` i właściciel API `docs/discriminating-learning.md`. Limity 8 s per proces i 20 s CTest, kroki i asercje pozostają bez zmiany. Nie zmienia się zapisu świata ani reguł uczenia.

Wąski zakres: stałe opisy gramatyki są generowane raz jako niemutowalne dane kodu; każdy umysł nadal osobno oblicza tablice, pobiera pełne opłaty i przechowuje własne modele/obserwacje. Dokładna równość całej tablicy rozstrzyga duplikaty, a kolejność modeli nadal wynika z kanonicznej kolejności programów. Nie współdzieli się wyników pomiarów ani poznanej prawdy świata. Odbiór: istniejący timeout RED→GREEN; niezależna referencja pełnego sortowania; kontrolne hashe pełnego stanu dla sześciu budżetów zebrane przed zmianą; niezależność dwóch umysłów; identyczne bajty checkpointu i wyniku objazdu; pełne profile i zachowane porażki. To usprawnienie obliczeń, bez nowego wyniku naukowego I/A/T.

- [x] Zachować stan bazowy i wykonać kontrolę dokumentacji oraz istniejących wybranych testów diagnostyki.
- [x] Dodać próbę natywną nowego eksportu i uruchomić ją na starej aplikacji. Oczekiwane: brak obsługi opcji, kod 1 i czerwony test wymagający CSV.
- [x] Dodać przypadki jednostkowe formatu, walidacji, limitu i braku mutacji świata; wdrożyć model próbek i uzyskać wynik zielony.
- [x] Włączyć opcję, zbieranie obu faz, pamięć i zapis CSV bez nadpisywania. Wymagać rzeczywistych danych w próbie natywnej i zgodności starych podsumowań.
- [x] Zweryfikować odmowy, błąd pliku i przedwczesne zamknięcie; poprawić wykazane błędy bez zmiany niezmienników.
- [x] Wykonać właściwe buildy, pełne CTest Debug/Release/ASan, format, kontrolę dokumentacji i testy narzędzi. Zachować wcześniejsze porażki i ograniczenia.
- [x] Uzyskać niezależny przegląd, rozstrzygnąć uwagi i zakończyć raport. Nie zamykać całego M06 przez ten przyrost.
