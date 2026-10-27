# Odbiór wielorazowej pojemności

Data: 2026-10-26. Zakres: [brief](../work-items/reusable-capacity.md). Odbiór techniczny tego przyrostu: zaliczony. Nie jest to odbiór M04.

## Reprodukcja i implementacja

Przed zmianą test `traversal.reusable_rest` odtworzył brak ponownego wykorzystania miejsca odpoczynku po zapisie: `rest place was not reusable after load`. Dowód: `build/capacity-before-tests.log`. Nie zmieniono kryterium dwóch kolejnych ukończeń ani bilansu.

`ResourceKind` jest niezmienną właściwością zasobu `WorkWorld`. Domyślne tworzenie pozostaje materiałem. Pojemność wymaga zerowego zużycia początkowego, jest zajęta przez `Reserved` i `Running`, a ukończenie lub anulowanie zwalnia ją bez uzupełnienia ilości. Usunięcie uczestnika przerywa zajęcia istniejącym mechanizmem. Walidacja faz i bilansu oraz dziennik odrzucają zużycie pojemności.

Nowe sesje tworzą odpoczynek jako pojemność; jedzenie i konserwacja pozostają materiałami. Rodzaj i aktywne zajęcie kontynuują po odczycie. Inspektor rozróżnia zapas od pojemności i wolnych miejsc. Nie dodano zależności, konkurencyjnej własności, liczników zajętości ani nowego zegara.

Nowy zapis świata z pojemnością używa wersji 2. Materiałowe światy nadal używają wersji 1 bez zmiany bajtów; wersja 1 jest odczytywana z dawną semantyką. Historyczny odpoczynek materiałowy nie jest migrowany ani refundowany. W teście nieobsługiwanej wersji podniesiono numer z 2 do 3, ponieważ 2 jest teraz obsługiwana; osobne testy sprawdzają prawdziwą wersję 2, rodzaj 99 i nielegalne zużycie przy poprawnym checksum.

## Sprawdzenia

Pierwszy odbiór przed dodaniem osobnych testów domeny pojemności: 40/40 `work.*` i `traversal.*` PASS (`build/capacity-focused-tests.log`). Następnie wykonano pełne buildy i CTest bez filtrowania ani wykluczania testów:

| Konfiguracja | Build | Pełny CTest | Dowód |
| --- | --- | --- | --- |
| Debug | PASS | 115/115 PASS | `build/capacity-debug-tests.log` |
| RelWithDebInfo | PASS | 115/115 PASS | `build/capacity-release-tests.log` |
| ASan / RelWithDebInfo | PASS | 117/117 PASS | `build/capacity-asan-tests.log` |

Komendy: `cmake --build --preset windows-debug`, `windows-release`, `windows-asan` oraz odpowiadające im `ctest --preset ...`. Logi kompilacji: `build/capacity-{debug,release,asan}-build.log`. Dwie dodatkowe próby ASan potwierdzają czystą fixture i wykrycie celowego przepełnienia sterty. Wszystkie dotychczasowe regresje, w tym historyczny zapis M03, B/C i wybór potrzeb, przechodzą.

Nowe testy: `work.capacity_lifecycle`, `work.capacity_removal`, `work.capacity_snapshot` i `traversal.reusable_rest`. Pokrywają konkurencję, ponowne użycie, anulowanie, przerwanie przez usunięcie przed/po rozpoczęciu, zerowe zużycie, idempotencję, mieszany świat materiał/pojemność, odczyt działania w toku i wersję 1.

Po buildach wykonano `reality_traversal.exe --smoke-test --gpu-debug` w każdej konfiguracji, z osobnymi `--report`, `--save` i `--capture`. Każde rzeczywiste okno wybrało NVIDIA GeForce MX330 (`hardware=1 gpu_debug=1`) i zakończyło się `TRAVERSAL SMOKE PASS` oraz normalnym zamknięciem. Dowody: `build/capacity-gpu-{debug,release,asan}.log`, obrazy `.png` i zapisy `.save`. Te próby obejmują nowy zagnieżdżony świat wersji 2 oraz wewnętrzny zapis/odczyt, obraz, wybór poziomu i minimalizację/przywrócenie. Nie zastępują ludzkiej oceny sterowania.

Osobny nowy proces Debug uruchomiono przez `--smoke-test --resume-smoke --gpu-debug` na zachowanym `build/need-fallback-gpu-debug.save` sprzed wprowadzenia pojemności. Suma kontynuacji pozostała taka sama jak w `build/need-fallback-gpu-debug.log`; dowód nowej próby: `build/capacity-gpu-legacy.log`. Odczyt nie nadpisywał historycznego zapisu. Potwierdza to kontynuację realnego poprzedniego świata, niezależnie od syntetycznego testu wersji 1.

`cmake --build --preset windows-debug --target format-check`: PASS (`build/capacity-format.log`). `python tools/check_docs.py`: PASS. `python -m unittest discover -s tools/tests -p "test_*.py"`: 29/29 PASS. Po wpisaniu wyników wykonano ponowną kontrolę końcowej dokumentacji.

Produkcja: `work_world.hpp/.cpp`, `world_snapshot.cpp` i konsument/inspektor w `traversal_session.cpp`; testy: `work_world_tests.cpp`, `traversal_tests.cpp` i rejestr CMake. Dokumenty normatywne i stan planu zaktualizowano razem z kodem. Nie zmieniano historycznych raportów, nie odrzucano plików, nie tworzono commitów i nie delegowano pracy.

## Granice

To fundament domenowy przed odbiorem całego rozszerzenia ruchu i M04. Jedna czynność nadal rezerwuje jeden zasób. Nie zaimplementowano wspólnej transakcji wielu zasobów, dynamicznej zmiany pojemności, grafiku, własności łóżek w fikcji ani animacji spania. Zniszczenie stanowiska jest usunięciem zasobu, nie dowodem fizycznej destrukcji. Człowiek nie oceniał ponownie komfortu sterowania.
