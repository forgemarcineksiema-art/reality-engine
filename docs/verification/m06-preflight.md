# Audyt gotowości pomiaru przed odbiorem M06

**Status: wykonano ograniczony audyt źródeł i wybrane testy Release, 01.11.2026.** Długiego pomiaru, prób natywnych Win32/D3D11 i prób z ludźmi w tym audycie nie wykonano. W dniu tego przeglądu M06 był planem; późniejszy [odbiór M06](m06-acceptance.md) zamknął etap. Aktualny status podaje [plan](../prototype-plan.md#m06--odbiór-i-decyzja-o-skali). Następny ograniczony zakres opisuje [brief przygotowania](../work-items/m06-acceptance.md).

## Wersja i dowody

Badany kod: `82395a3` na gałęzi `codex/verified-capabilities-plan`. Istniejące niezacommitowane zmiany dotyczyły dokumentacji kierunku projektu; audyt nie zmienia C++ ani formatów świata. Zachowano [log budowania](evidence/m06-preflight-2026-11-01/build-release.log), [log wybranych testów](evidence/m06-preflight-2026-11-01/ctest-release.log) i [manifest dowodów](evidence/m06-preflight-2026-11-01/manifest.json).

Wykonane komendy z katalogu projektu:

```powershell
cmake --build --preset windows-release --target reality_traversal_diagnostics_tests reality_practice_tests
ctest --preset windows-release -R '^(traversal\.diagnostics_.+|practice\.(autonomy|teaching|community|persistence|evidence_capacity|frozen_pending))$'
```

Build zakończył się kodem 0. Wybrany zestaw **17/17 PASS**: 11 testów obserwatora/metryk i 6 testów praktyk, obejmujących zapis, autonomię, przekaz, społeczność, limit dowodów oraz zamrożone oczekujące działanie. To nie pełna macierz CTest, profil ASan, benchmark ani odbiór człowieka. Limitów i asercji nie zmieniono.

Pierwsza kontrola dokumentacji odrzuciła nagłówek raportu zaczynający się od identyfikatora etapu jako definicję poza właścicielem (`identifier defined outside owner: M06`). Poprawiono tytuł, zachowując walidator i jednego właściciela statusu. Testy narzędzi: 58/58 PASS.

## Ustalenia z rzeczywistych źródeł

| Obszar | Stan sprawdzony | Znaczenie dla M06 |
| --- | --- | --- |
| Klatki i symulacja | `RuntimeMetrics` posiada próbki frame/simulation/backlog i podsumowania min/p50/p95/p99/max/średnia | Podstawowego pomiaru nie trzeba budować drugi raz. Raport nie eksportuje próbek z oznaczeniami faz |
| Okres próby | `--measure-seconds` obsługuje 1–1800 s, `--warmup-seconds` 0–300 s; aplikacja rozpoczyna pracę zegara i zapisuje stan końcowy | Planowane 120+1800 s mieści się w istniejącym interfejsie. Samo zakończenie czasu nie zalicza budżetu |
| Rozgrzewka | Metryki są rejestrowane dopiero po jej końcu; symulacja pracuje także wcześniej | Pierwsze uczenie może wypaść poza oceniany zapis czasów. Wymagany osobny ślad początkowego uczenia lub oznaczone dane rozgrzewki |
| Pamięć | Odczyt WorkingSet/PrivateUsage raz na sekundę; zachowane wybrane wartości i szczyty | Brakuje eksportowanego szeregu potrzebnego do oceny przebiegu, a nie tylko początku/końca |
| Koszty etapów | Mierzony jest cały `SimulationPacer::update`; w przejrzanym kodzie brak rozdzielnych czasów uczenia, tras i zapisu oraz pomiarów timestamp GPU | Nie przypisywać przycięć konkretnemu subsystemowi bez uzupełnienia instrumentacji |
| Scena dwóch metod | Natywna kontrola początkowego stanu wymaga nieautonomicznego kuriera i wykonawcy naprawy; wybór uruchamia wejście F2 | Nie stosować jako gotowej bezobsługowej sceny aktywnej logistyki |
| Warunki pomiaru | Log zawiera adapter, viewport, interwencje, resize/minimalizacje, zasłonięcie, błędy i liczbę kroków | Warunki oceniać razem z postępem społeczności; nie uznawać wydajnego bezruchu za działającą autonomię |

Podstawa audytu: [metryki](../../src/core/runtime_metrics.cpp), [aplikacja](../../src/platform/traversal_main.cpp), [testy diagnostyki](../../tests/traversal_diagnostics_tests.cpp), [natywny runner](../../tools/test_traversal_diagnostics.py) i [obecna instrukcja pomiaru](../development.md#diagnostyka-przestrzennego-laboratorium). Runner natywny odczytano, ale go nie uruchomiono.

## Decyzja o następnym kroku

Najpierw uzupełnić istniejące pomiary o eksport próbek w czasie, pamięci i oznaczenie faz/rozgrzewki, zachowując bieżące podsumowania i semantykę świata. Następnie osobno uzupełnić przypisanie kosztów CPU/GPU i subsystemów wymagane przez M06, przeprowadzić pilotaż i zamrozić właściwy protokół. Zakres pierwszej paczki i dowód odbioru podaje brief.

Przed pomiarem właściwym sprawdzić wyposażenie i aktywność sceny `--community --learning`, zamiast zakładać, że będzie aktywna przez 30 minut. Próbę dwóch metod, ingerencji oraz kontynuacji przygotować odrębnie. Nowe systemy rozgrywki, większa populacja i pełna fizyka konstrukcji nie są potrzebne do zamknięcia tej luki pomiarowej.
