# Eksport przebiegu pomiaru — pierwszy przyrost przygotowania

**Status: implementacja i krótkie próby wykonane 01.11.2026; końcowe regresje po poprawce ścieżek i niezależny przegląd w toku.** To ograniczony zakres [briefu](../work-items/m06-acceptance.md) i [planu wykonawczego](../work-items/m06-measurement-export.md). Status całego etapu pozostaje u [jednego właściciela](../prototype-plan.md#m06--odbiór-i-decyzja-o-skali).

## Zachowanie

`--measurement-trace DIR` eksportuje rzeczywiste czasy klatek, aktualizacji, zaległości i odczyty pamięci procesu do nowego `samples.csv`. Zachowuje również rozgrzewkę. Dotychczasowy raport nadal obejmuje okres właściwy. Obserwator nie trafia do zapisu świata ani do wiedzy mieszkańców. [Instrukcja](../development.md#eksport-przebiegu-pomiaru) określa format, opcje, koszty i granice.

Istniejące dowody nie są nadpisywane. Obsłużone przerwanie zachowuje dostępny ślad z oznaczeniem niepełnej próby. Błąd eksportu daje kod 1 i nie może otrzymać `normal_shutdown=1`.

## Weryfikacja w toku

Baza kodu tego przyrostu: `3bf2d60f3c9183d1b7d74eefabf3f07fd3e42ae6`. Początkowe wybrane regresje Release: 17/17 PASS. Nowy test natywny na starej aplikacji zakończył się kodem 1, ponieważ brakowało opcji eksportu. Testy modelu najpierw wykazały brak nowego API w kompilacji. Po implementacji testy diagnostyki: 15/15 PASS; natywne próby szeregu, odmów ścieżek, WM_CLOSE i konkurencyjnego CSV: PASS.

Pierwsza pełna macierz CTest: Debug **428/428 PASS** (235,46 s), Release **428/428 PASS** (76,99 s), ASan **430/430 PASS** (367,19 s). W każdym profilu wykonano cztery nowe przypadki modelu i cztery nowe próby natywne. Format i kontrola dokumentacji: PASS. Testy narzędzi: **58/58 PASS**.

Dodatkowa próba ujawniła błędną odmowę nowego katalogu z końcowym separatorem. Zachowano czerwony test natywny; normalizacja końcowego pustego komponentu ścieżki dała wynik zielony. Rozszerzono również próbę odmów o alias Win32 `\\?\` — ochrona działała już przed tą poprawką. Końcowe kontrole i niezależny przegląd są w toku; zachowane logi rozdzielą oba przebiegi.

## Granice

Krótkie próby natywne są kontrolą kontraktu i działania Win32/D3D11, bez oceny FPS. Nie wykonano dłuższej próby M06, izolowanego porównania narzutu włączonego/wyłączonego eksportu ani sesji z ludźmi. Pamięć dotyczy procesu, bez VRAM. Nie wdrożono osobnych liczników GPU, uczenia, nawigacji i zapisu. Bufor nie gwarantuje przetrwania twardego zabicia procesu, utraty zasilania ani awarii nośnika.
