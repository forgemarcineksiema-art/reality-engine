# Weryfikacja wyboru potrzeby po niedostępności celu

Data: 2026-10-26. Zakres: [brief](../work-items/autonomy-need-fallback.md); aktualna semantyka: [autonomia laboratorium](../traversal-autonomy.md). Odbiór techniczny tego przyrostu: zaliczony.

## Dowód przed i po

Przed zmianą produkcyjną `traversal.need_fallback_empty` i `traversal.need_fallback_unreachable` dawały porażkę. W obu przypadkach po 361 krokach nie ukończono żadnego odpoczynku. Wariant pustego magazynu zakończył się energią 66, a wariant niepołączonej powierzchni energią 100; głód doszedł do limitu 1000. Dowód: `build/need-fallback-before-tests.log`.

Test `traversal.last_meal` przechodził przed zmianą: dwie postacie zużyły dokładnie jedną porcję, posiłek ukończyła jedna. Pierwszy szkic testu błędnie oczekiwał utrzymanej rezerwacji materiału po rozpoczęciu posiłku. Kontrakt M01 zużywa porcję przy rozpoczęciu, dlatego test zapisuje rzeczywistą czynność w toku po poniesieniu kosztu. Nie uznajemy tej korekty testu za naprawę błędu produkcyjnego ani nie zmieniamy reguł zużycia.

W sesji zmieniono wyłącznie wybór rodzaju zadania: zamiast jednego rodzaju wybranego warunkowo rozważana jest kolejność pilnych potrzeb, a po braku odpowiedniego planu następny rodzaj. Zachowano istniejące rezerwacje, lokalne sprawdzanie zapasu, terminy ponowień, sterowanie, fizykę i formaty. Pierwsze testy po poprawce: 17/17 `traversal.*` PASS (`build/need-fallback-focused-tests.log`).

Nowe testy wymagają fizycznego poznania odmowy magazynu, odczytu tego stanu, rzeczywistego wykonania odpoczynku oraz identycznej kontynuacji po odczycie czynności. Brak jedzenia nie może obniżać głodu ani zwiększać liczby posiłków. Konkurencja o ostatnią porcję obejmuje bilans zdarzeń zużycia i dokładnie jedno ukończenie, również w kontynuacji po odczycie.

## Pełny odbiór

Wykonano kolejno `cmake --build --preset windows-debug`, `windows-release`, `windows-asan` i odpowiadające im pełne `ctest --preset ...`, bez filtrowania i wykluczeń.

| Konfiguracja | Build | Pełny CTest | Dowód |
| --- | --- | --- | --- |
| Debug | PASS | 111/111 PASS | `build/need-fallback-debug-tests.log` |
| RelWithDebInfo | PASS | 111/111 PASS | `build/need-fallback-release-tests.log` |
| ASan / RelWithDebInfo | PASS | 113/113 PASS | `build/need-fallback-asan-tests.log` |

Logi kompilacji są w `build/need-fallback-{debug,release,asan}-build.log`. Profil ASan obejmuje także dwie próby kontrolne diagnostyki pamięci. Wszystkie dotychczasowe regresje, w tym B/C i nierówne kapsuły, pozostają zaliczone.

Po buildach uruchomiono `reality_traversal.exe --smoke-test --gpu-debug` z osobnymi ścieżkami `--report`, `--save` i `--capture` w każdej konfiguracji. Wszystkie trzy rzeczywiste okna wybrały NVIDIA GeForce MX330 (`hardware=1 gpu_debug=1`) i zakończyły się `TRAVERSAL SMOKE PASS` oraz normalnym zamknięciem bez zgłoszenia błędu warstwy diagnostycznej. Dowody: `build/need-fallback-gpu-{debug,release,asan}.log`, zapisy `.save` i obrazy `.png`. To próba okna, obrazu, wyboru poziomu, ręcznego przerwania celu, zapisu/odczytu i minimalizacji/przywrócenia. Nie stanowi nowej oceny odczucia przez użytkownika; scenariusze wyboru potrzeb są dowiedzione przez runner wspólnej sesji, nie sam obraz.

`cmake --build --preset windows-debug --target format-check`: PASS (`build/need-fallback-format.log`). `python tools/check_docs.py`: PASS. `python -m unittest discover -s tools/tests -p "test_*.py"`: 29/29 PASS. Końcowa dokumentacja została ponownie sprawdzona po uzupełnieniu wyników.

Kod produkcyjny zmieniono wyłącznie w `src/lab/traversal_session.cpp`, w wyborze rodzaju zadania. Dodano trzy przypadki i ich rejestrację CTest, brief, opis bieżącej autonomii, raport oraz odnośniki. Nie zmieniono progów potrzeb, kontraktu zużycia, fizyki, sterowania ani formatu zapisu. Nie tworzono commitów i nie delegowano pracy.

## Pozostałe ograniczenia

To ograniczony wybór potrzeb małej sceny, bez pełnej populacji M04. Brak globalnej kompletności tras i dowodu dowolnego zakleszczenia. Przedmiotem zmiany nie jest przerywanie aktywnego planu nową potrzebą, zmiana progu śmierci lub wielorazowa pojemność stanowisk. Aktualny zasób odpoczynku nadal jest zużywany przy ukończeniu według M01; nie jest to model wielorazowego łóżka. Dalsze rozszerzenie wymaga odrębnej zmiany kontraktu domenowego.
