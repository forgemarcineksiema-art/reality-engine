# Obserwacja ruchu przed dostawą — plan wykonania

Status: wykonano pierwszy przyrost; [raport wyników i ograniczeń](../verification/m04-entry-observability.md). Użytkownik zatwierdził kierunek i polecił rozpocząć realizację; wykonanie samodzielne w dotychczasowym katalogu. Przygotowanie dostawy ma osobny [projekt transferu materiału](material-transfer-design.md), bez deklaracji wdrożenia.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** rzeczywista diagnostyka postępu i lokalne reprodukcje oraz pomiar pracy obecnej sceny przed rozbudową dostawy.

**Architecture:** obserwator laboratoryjny odczytuje `TraversalSession`; okno jest jego konsumentem. Nie zapisuje nowego autorytatywnego stanu świata. Metryki wykonania są oddzielone od zegara symulacji.

**Tech Stack:** własny C++20, Win32, D3D11, MSVC x64, Python standardowy do testu procesu.

**Spec:** [zatwierdzony kierunek i projekt przyrostu](m04-entry-design.md).

## Global Constraints

- Własna fizyka i nawigacja, bez nowych bibliotek.
- Zegar 50 ms, limit prędkości 3 m/s; bez zmiany fizyki lub formatów świata.
- Zachować wcześniejsze testy i ich limity czasu; nie tworzyć commitów i nie odrzucać plików.
- Praca w bieżącym katalogu: repo ma niezacommitowane fundamenty i nie ma HEAD. Nie tworzymy worktree wymagającego nieistniejącego commitu.
- Wykonanie samodzielne zgodnie z wcześniejszym ograniczeniem użytkownika. Przegląd końcowy autora będzie jawnie opisany; nie udajemy niezależnego review.

## Review Focus

- Drobne oscylacje oraz ponawianie planu nie oznaczają postępu — próba obserwatora z niezmiennym końcem Walk.
- Oczekujący na właściciela kładki nie jest utkniętym wykonawcą — test na rzeczywistej kolejce.
- Wczytanie lub duży skok obserwacji nie dziedziczą starego alarmu — próba resetu i przerwy.
- Błąd katalogu migawki nie uszkadza zwykłego zapisu — test systemu plików i rollbacku sesji.
- Minimizacja, zatrzymany zegar i diagnostyka GPU zmieniają warunki pomiaru — raportować warunki i nie ogłaszać celu 60 FPS na podstawie samego builda.

## Task 1: Obserwator i migawki

Pliki: nowe `src/lab/traversal_diagnostics.hpp/.cpp`, `tests/traversal_diagnostics_tests.cpp`; zmiany `CMakeLists.txt`.

Interfejsy: `TraversalDiagnostics::observe(const TraversalSession&)`, `reset()`, `report()`, `suspected_stall()`, `capture(session, directory, reason)`. Migawka odczytywalna jako `directory/world.save`; nie istniejący wcześniej katalog jest wymagany, aby nie nadpisywać dowodów.

- [x] Dodać czerwony test rzeczywistego procesu smoke: raport nie zawiera jeszcze klasyfikacji postępu ani metryk.
- [x] Dodać testy obserwatora: 100 kroków bez poprawy 0,1 m, oscylacja, ciągły postęp, replanning, legalne oczekiwanie, reset i duża przerwa. Sprawdzić runtime po implementacji API; początkowy brak API nie jest zaliczonym runtime red.
- [x] Wdrożyć obserwator i migawki, bez mutacji sesji.
- [x] Sprawdzić wczytanie migawki oraz jawne odrzucenie zajętej ścieżki i brak mutacji świata przez odczyt/raport.

## Task 2: Konsument okienkowy i metryki

Pliki: `src/platform/traversal_main.cpp`, nowe `src/core/runtime_metrics.hpp/.cpp`, testy metryk w runnerze diagnostyki; `tools/test_traversal_diagnostics.py` dla testu procesu.

Interfejsy: F6 → `capture`; `--diagnostics PATH` określa katalog bazowy; `--measure-seconds N` i `--warmup-seconds N` określają czas pomiaru 1–1800 s i przygotowania 0–300 s. Domyślnie pomiar jest wyłączony. `--smoke-test` wymusza krótki raport kontrolny. `RuntimeMetrics` gromadzi próbki czasów w ms i oblicza percentyle na końcu, z jawnym limitem pamięci.

- [x] Wdrożyć testowane statystyki: min, p50, p95, p99, max, średnia i odrzucanie błędnych próbek.
- [x] Włączyć obserwację okna, F6, ograniczone migawki alarmów i raport stanu w inspektorze. Po F9 resetować obserwator.
- [x] Wdrożyć pomiar rzeczywistej pętli i pamięci, raportować pauzę/minimalizację/ingerencję podczas pomiaru.
- [x] Uruchomić czerwony test procesu ponownie: wymagane PASS, migawka z F6 odtwarzalna, brak niejawnego resetu świata.

## Task 3: Odbiór i pomiar

Pliki: dokumentacja środowiska i autonomii, nowy raport odbioru; dowody w `build/m04-entry-*`.

- [x] Pełne Debug, RelWithDebInfo, ASan, format-check, kontrola dokumentacji i testy narzędzi.
- [x] Próba rzeczywistego F6/F9 i diagnostyki GPU na MX330.
- [x] Zapisać hashe źródeł i warunki. Wykonać 30 s przygotowania + 300 s pomiaru bez równoległych buildów/testów, z zachowaniem negatywnych wyników.
- [x] Osobny przegląd kodu przez autora, dokumentacja ograniczeń i aktualizacja tego planu. M04 jako całość pozostaje nieodebrany; po wynikach przechodzimy do transferu materiału.
