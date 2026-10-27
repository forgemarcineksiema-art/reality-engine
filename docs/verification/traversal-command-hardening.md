# Naprawa poleceń i przejść w torze przestrzennym

Data: 2026-10-26. Zakres: [zatwierdzony brief](../work-items/traversal-command-hardening.md). Odbiór techniczny tego zakresu: zaliczony. Wszystkie poniższe wyniki pochodzą z wykonanych prób na bieżącym repozytorium.

## Reprodukcje i implementacja

Zachowano poprzedni raport `build/GENESIS-HARDENING-001.md` oraz jego logi. Przed poprawką cztery testy dawały porażkę: `supersede_pending_goal`, `supersede_pending_stop`, `release_passage_ownership` i nowy `manual_touching_departure`. Dowód: `build/traversal-hardening-fixes-before.log`. Pierwsze dwa pozostawiały stare odroczone polecenie, trzeci pozostawiał właściciela mimo rzeczywistego wyjścia, czwarty zatrzymywał ręczny ruch przed stykiem współpłaszczyznowych płyt.

`set_goal` usuwa wcześniejszy odroczony zamiar także w ścieżce ponowienia bieżącego celu; zachowuje plan, jego kursor i prędkość. Dokładny cel ponownie staje się zamiarem gracza zamiast wcześniejszego podejścia w zasięg. Wspólne czyszczenie odroczonego stanu obsługuje również natychmiastowe polecenia i wykonanie odroczonego zamiaru.

Zwalnianie przejścia ma wspólną regułę uruchamianą po ruchu, postępie trasy i anulowaniu niezaczętej trasy. Obecność kapsuły jest oceniana względem obróconego obszaru i wysokości przejścia, niezależnie od identyfikatora podparcia. Bieżące zarezerwowane wejście zachowuje własność. Samo ręczne wyjście wystarcza do wznowienia kolejki; nie potrzeba kolejnej misji ani celu.

Lokalne sterowanie sprawdza ciągłe pokrycie odcinka przez współpłaszczyznowe, chodliwe podpory. Przedziały przecięcia odcinka z ich domenami kontaktu są scalane; wsparty koniec odcinka nie ukrywa luki pośrodku. Tolerancja dopasowania płaszczyzn wynosi 10 nm i służy błędom numerycznym, nie pokonywaniu stopni. Kontrola fizyczna odcinka i faktycznego ruchu pozostaje obowiązkowa.

## Zakres regresji

- Dotychczasowe reprodukcje B/C zachowują wymagania wyniku. Test przejścia dodatkowo wymaga zachowania własności przy zmianie podparcia, dopóki kapsuła pozostaje w obszarze; sprawdza odczyt zarówno zajętego, jak i zwolnionego przejścia oraz późniejsze dojście oczekującego.
- `command_roundtrip`: odczyt podczas rozpoczętego kroku z odroczonym celem, stopem lub podejściem; ponowienie właściwego celu; zachowanie prędkości i kursora; porównanie dalszych stanów z kontynuacją po kolejnym odczycie. Osobno odwołanie stopu podczas fizycznej jazdy platformą i dojście na drugi brzeg.
- `manual_touching_departure`: rzeczywiste ręczne przejście na sąsiednią płytę. `support_coverage`: styk, luka 1 cm mimo wspartych końców, inny poziom, niewchodliwa płyta i obrócony styk.
- `spatial.unequal_capsules` pozostaje testem różnych wysokości/promieni, także przy zamianie argumentów. Wycofanego zarzutu kolizji nie traktujemy jako potwierdzonego błędu.

## Sprawdzenia

Pierwszy odbiór skupiony po naprawie: 30/30 testów `traversal.*` i `spatial.*` w Debug, zapisany w `build/traversal-hardening-focused.log`. Następnie rozszerzono kontrolę odczytu zajętego przejścia i doprecyzowano obrócony styk; poniższe pełne odbiory obejmują końcowe testy.

| Konfiguracja | Build | Pełny CTest | Log testów |
| --- | --- | --- | --- |
| Debug | PASS | 108/108 PASS | `build/traversal-hardening-debug-tests.log` |
| RelWithDebInfo | PASS | 108/108 PASS | `build/traversal-hardening-release-tests.log` |
| ASan / RelWithDebInfo | PASS | 110/110 PASS | `build/traversal-hardening-asan-tests.log` |

Wykonano `cmake --build --preset windows-debug`, `windows-release`, `windows-asan` oraz odpowiadające im `ctest --preset ...` bez filtrowania i wykluczeń. Profil ASan skonfigurowano przez `cmake --preset windows-asan`. Logi buildów mają te same prefiksy i zakończenie `-build.log`; konfiguracja ASan jest w `build/traversal-hardening-asan-configure.log`. Dwa dodatkowe testy ASan obejmują czystą próbę kontrolną i wykrywanie celowego przepełnienia sterty.

Poza CTest uruchomiono rzeczywiste `reality_traversal.exe --smoke-test --gpu-debug` w każdej z trzech konfiguracji, z osobnymi ścieżkami `--report`, `--save` i `--capture`. Każdy wariant uruchomiono ponownie w oddzielnym procesie przez `--smoke-test --resume-smoke --gpu-debug` z tym samym zapisem. Wszystkie sześć procesów wybrało sprzętowy NVIDIA GeForce MX330, włączyło warstwę diagnostyczną i zakończyło się `TRAVERSAL SMOKE PASS` oraz `normal_shutdown=1`. Dla każdego wariantu porównanie sumy kontynuacji pierwszego i drugiego procesu dało identyczny wynik `15825416920717782246`.

Dowody GPU: `build/traversal-hardening-gpu-{debug,release,asan}.log` oraz odpowiednie `-resume.log`, zapisy `.save` i obrazy `.png`. Próby obejmują wybór górnej powierzchni, przerwanie celu ruchem ręcznym, zapis/odczyt, obraz i minimalizację/przywrócenie. Obraz Debug obejrzano: przedstawia scenę i postacie. To automatyczna próba okna, nie nowa próba fizycznej myszy/klawiatury przez użytkownika ani ocena komfortu.

`cmake --build --preset windows-debug --target format-check`: PASS, log `build/traversal-hardening-format.log`. `python tools/check_docs.py`: PASS. `python -m unittest discover -s tools/tests -p "test_*.py"`: 29/29 PASS. Te kontrole nie zastępują wyników silnika.

Kontrola stylu wskazała zastane nieformatowane wyrażenie kosztu stopnia w `tests/spatial_tests.cpp`; sformatowano je bez zmiany asercji. Konfiguracje użytkownika `.opencode/agents/` wpisano do rejestru dokumentów bez zmiany treści i bez delegowania im pracy. Nie zmieniono kontroli dokumentacji, limitów czasu ani kryteriów testów.

Porównanie SHA-256 z zachowanym `build/genesis-hardening-001-before.json` wykazało zmiany produkcyjne wyłącznie w `src/lab/traversal_session.cpp`, `src/navigation/surface_navigation.cpp` i jego nagłówku. Pozostałe różnice z tej listy dotyczą testów i ich rejestracji w CMake. Dodano brief, raport i odnośniki w aktualnej dokumentacji. Nie utworzono commitów, nie odrzucono cudzych plików i nie uruchamiano delegacji.

## Granice

Formaty zapisu i stare regresje M03 pozostają bez zmiany. Ta naprawa nie jest pełnym odbiorem [rozszerzenia](../work-items/advanced-traversal.md), dowodem kompletności nawigacji ciągłej, rozwiązaniem dowolnego zakleszczenia, pełnym solverem fizycznym ani akceptacją odczucia przez człowieka. Zapytanie o podparcie nie zastępuje jawnych połączeń między poziomami i planowania tras. Nie wykonano pomiaru wydajności ani testu dużego tłumu.
