# Weryfikacja transferu i pierwszej dostawy

Status: **ograniczony zakres techniczny zweryfikowany, z autonomicznym zwrotem nieprzypisanego ładunku i zmierzonym marginesem ASan; ograniczenia produktu pozostają**. Zakres, umowa i ograniczenia są w [planie przyrostu](../work-items/material-delivery.md). Nie jest to odbiór M04 ani ocena wygody przez człowieka.

## Odtworzone problemy i przegląd

Późniejszy [przyrost lokalnej wiedzy](local-station-knowledge.md) ma odrębny odbiór. Poniższe opisy braku komunikacji zachowują historyczny stan weryfikacji dostawy; nie zastępują nowszego modelu obserwacji i przekazu.

`build/delivery-review-red-console.log`: trzy czerwone przypadki — dwie stacje współdzielące partię, pobranie przez litą ścianę, wybór punktu podejścia po niewłaściwej stronie ściany. Po poprawkach i ponownym przeglądzie ujawniono kolejną ścieżkę starej trasy: `build/delivery-legacy-wall-red.log` pokazuje błąd `route or transfer without a goal`. `build/delivery-review-green.log` pokazuje cztery zaliczone przypadki po zmianach. Kontrola dostępu obejmuje także wykonywanie czynności potrzeb i ponawianie ich trasy. Ogólne ręczne wskazywanie punktu zachowuje dawny kontrakt.

Niezależny przegląd statyczny rdzenia transferu nie wykazał błędów blokujących; powtórny przegląd dostawy wskazał niespójną starą trasę, a po naprawie zaakceptował ten konkretny zakres. Recenzenci nie uruchamiali testów. Sprawdzenie statycznych brył przy dostępie nie jest symulacją rąk ani widoczności, a wąska szczelina nie jest automatycznie przeszkodą dla sięgnięcia.

## Rzeczywiste próby

Pierwszy pełny przebieg `ctest --preset windows-debug --output-on-failure` zakończył się **208/208** (`build/material-delivery-debug-ctest.log`), przed dodaniem osobnego natywnego scenariusza dostawy. `cmake --build --preset windows-release -j 2` oraz `ctest --preset windows-release --output-on-failure` zakończyły się sukcesem, **209/209** (`build/material-delivery-release-build.log`, `build/material-delivery-release-ctest.log`). Wcześniejsze `build/delivery-adversarial.log` obejmuje 13 testów dostawy i 10 domenowych transferu. `python tools/check_docs.py` przeszedł; `python -m unittest discover -s tools/tests -p "test_*.py"` wykonał 29 testów.

Po ostatniej poprawce rodzaju stanowiska pełny Debug przeszedł **210/210** (`build/material-delivery-final-debug-ctest.log`), Release **210/210** (`build/material-delivery-final-release-ctest.log`), a ASan **212/212** (`build/material-delivery-final-asan-ctest.log`). Budowanie wszystkich trzech konfiguracji zakończyło się sukcesem. W ostatniej serii ASan `delivery.actual` trwał 15,04 s, przy niezmienionym limicie 20 s; wcześniejsze przekroczenia tego limitu i badanie poniżej nie znikają wskutek pojedynczego sukcesu.

Pierwszy pełny `ctest --preset windows-asan --output-on-failure` dał **210/211** (`build/material-delivery-asan-ctest.log`): `lab.hardware_gpu_standard` przekroczył wewnętrzny 3-sekundowy limit uruchomienia procesu z celowo niepoprawnym adapterem. Ponowne uruchomienie tego testu osobno przeszło (`build/material-delivery-asan-gpu-retry.log`, 2,74 s). Przyczyna pojedynczego opóźnienia nie została ustalona; wynik pierwszej serii pozostaje porażką. Uruchomiono ponowną pełną serię bez zmiany limitów.

Druga pełna seria ASan również dała **210/211** (`build/material-delivery-asan-ctest-rerun.log`): `delivery.actual` przekroczył 20 s, a osobne powtórzenie tego przypadku również przekroczyło limit (`build/delivery-asan-actual-retry.log`). W pierwszej serii ten sam przypadek przeszedł w 19,50 s. Przyczyna kosztu wymaga badania; próby nie są dowodem stabilnego przejścia ASan ani wydajności na laptopie.

Pomiar diagnostyczny z tymczasowymi licznikami, bez limitu CTest, wykonał 608 kroków: 11 158,5 ms w `advance` i 442,2 ms w dodatkowym `validate` (`build/delivery-asan-profile-before.log`); w innym przebiegu 9 880,5 ms i 386,3 ms (`build/delivery-asan-profile-ctest-env.log`). Powolne kroki występowały również bez wzrostu liczników planowania (`build/delivery-asan-profile-slow.log`). To nie lokalizuje przyczyny w konkretnej funkcji i nie uzasadnia zmiany limitu, usunięcia walidacji ani twierdzenia o stabilnej wydajności. Tymczasowe liczniki usunięto z testu.

`delivery.station_kind` pokazał dodatkowo, że zmieniony semantycznie zapis pozwalał powiązać stanowisko jedzenia z pojemnością odpoczynku (`build/delivery-station-kind-red.log`). Po kontroli rodzaju ten przypadek przeszedł (`build/delivery-station-kind-green.log`); stary materiałowy odpoczynek o nieokreślonym typie pozostaje dozwolony.

Osobny proces `reality_traversal.exe --smoke-test --delivery --gpu-debug` zakończył się kodem 0 na NVIDIA GeForce MX330; `build/delivery-native.log` zawiera `TRAVERSAL SMOKE PASS`, `gpu_debug=1`, `simulation_errors=0`, stan kuriera i jego partii. To próba uruchomienia i diagnostyki, nie obserwacja całego kursu ani pomiar długotrwałej wydajności. Początkowe dopisanie dwóch procesów do istniejącego natywnego testu przekroczyło dotychczasowy limit 20 s (`build/delivery-native-regression.log`); nowy scenariusz wydzielono jako osobny test bez zmiany limitu i bez usunięcia istniejących sprawdzeń. `delivery.native_diagnostics` przeszedł w konfiguracji Release w 5,89 s (`build/material-delivery-release-ctest.log`), weryfikując także odczyt zapisu wersji 4 w nowym procesie i niezmienność pliku dowodowego.

## Autonomiczny zwrot ładunku i margines ASan (kolejny przyrost)

### Czerwone przypadki i implementacja

Przed zmianą zachowania dodano pięć przypadków w `tests/delivery_tests.cpp`: `return_cancelled`, `return_hunger`, `return_detour`, `return_step`, `return_manual`. Początkowa seria zakończyła się oczekiwanymi porażkami:

- `return_cancelled` — `autonomous return did not deposit at the cancelled source`,
- `return_hunger` — `fixture is not autonomously returning cargo`,
- `return_detour` — `detour return was not planned`,
- `return_step` — `autonomous return did not finish after a committed Step`,
- `return_manual` — `autonomy did not physically return the manually loaded batch`.

Po implementacji wszystkie pięć przypadków przeszło. Dodano też `return_fallback` dla reguły „najbliższe znane zgodne stanowisko”, gdy źródło anulowanego kursu nie jest znane, `return_interaction` dla bezpośredniego oddania podczas oczekującego zwrotu (bez pozostawienia zadania z pustą torbą) oraz rozszerzono `return_cancelled` o ręczne przejęcie i ponowne włączenie autonomii. Dotychczasowe `local_and_cancel` i `cancel_step` nadal sprawdzają ręczne zatrzymanie: ładunek pozostaje w torbie i nie jest teleportowany. W sumie CTest ma teraz 218 przypadków w Debug/Release i 220 w ASan (dwa dodatkowe testy diagnostyczne).

Kontrakt zwrotu: wybór następuje wyłącznie przy `actor.autonomous`, braku aktywnego kursu i dodatniej ilości w `actor.cargo`; progi potrzeb są takie same jak dla wyboru dostawy (głód < 250, energia > 300). Źródło anulowanego kursu jest preferowane, jeśli jest znane, zgodne typem i nie ma aktywnego `station_retry`; inaczej wybierane jest najbliższe znane zgodne stanowisko bez aktywnego ponowienia. Krytyczny głód/energia nadal przerywają zwrot, a rozpoczęty Step, transport i czynność `WorkWorld` pozostają chronione. Oddanie używa wyłącznie `interact_cargo`/`world_.transfer()`. Stan mieści się w `GENESIS_TRAVERSAL 4`; nie dodano wersji 5.

### Pomiar przed zmianą

Pomiar przed optymalizacją, wykonany osobnym przebiegiem tego samego scenariusza `delivery.actual` (608 kroków), bez limitu CTest:

- Debug: pętla 6 050,45 ms, w tym `advance` 5 840,02 ms i `validate` 209,66 ms; 20 kopii sesji 0,97 ms, 20 walidacji 6,77 ms (`build/delivery-return-profile-before-debug.log`).
- ASan: pętla 10 811,9 ms, w tym `advance` 10 401,2 ms i `validate` 410,1 ms; 20 kopii sesji 2,81 ms, 20 walidacji 11,96 ms (`build/delivery-return-profile-before-asan.log`).
- Podział Debug: `tick` 5 823 ms, `substeps` 5 170 ms, `move` 5 129 ms, `steer` 2 662 ms, `legal` 1 624 ms, `platform` 292 ms; kopia sesji 27 ms, przypisanie 15 ms (`build/delivery-return-profile-sections-debug.log`). Z tego wynika, że powtarzane `validate()` w pętli testu i kopia sesji nie były głównym kosztem; dominowały przeszukiwanie sterowania i kontrole kolizji wewnątrz `advance`.

Historyczny limit CTest pozostał 20 s. Wcześniejsze pełne serie ASan miały `delivery.actual` 15,04 s, 19,50 s i przekroczenia ponad 20 s; nie zmieniono limitu.

### Wykonane optymalizacje

- `capsule_paths_clear` odrzuca pary o rozłącznych powiększonych zakresach, zanim policzy dokładną odległość; wynik dokładny pozostaje ten sam.
- `sweep_capsule` i `spatial_occupiable` pomijają bryły o rozłącznym zachowawczym AABB; `prism_motion_clear` ma analogiczny test.
- `continuous_surface_feet` używa stałego bufora 64 przedziałów zamiast alokacji wektorowej, hoistuje normalną i skalę bryły oraz wcześnie odrzuca bryły poza zakresem.
- `separating_face` liczy jedną rotację dla obu punktów i normalnej; kolejność operacji odtwarza `world() - origin`.
- `spatial_segment_clear` nie powtarza kontrole początku, którą wykonuje `sweep_capsule`; kontrola końca pozostaje.
- Porównanie par w doradczym sterowaniu odbywa się przed kosztownym `spatial_segment_clear`; zbiór wymaganych warunków i wynik są identyczne.

Nie usunięto walidacji niezmienników, nie zmniejszono liczby kroków ani podziału fizyki 50 ms, nie zmieniono limitu 20 s.

### Wynik po zmianie i podział scenariusza

Pojedynczy proces 608 kroków po optymalizacji osiągał w ASan 5,5–6,5 s osobno, ale w pełnej serii pierwszy test wydłużał się do 12–17 s, a jedna seria zakończyła się przekroczeniem 19,99 s (`build/delivery-return-final2-asan-ctest-1.log`). Próbkowanie `% Processor Performance` pokazało około 225% przez pierwsze ~7 s, potem około 150% w trakcie długiego testu i powrót po jego zakończeniu (`build/perf-fullsuite.txt`). To ograniczenie zasilania/termiczne hosta, a nie koszt walidacji: ten sam plik wykonywał się szybciej jako krótki proces.

Dlatego scenariusz 608 kroków jest teraz dwuetapowy, bez zmiany liczby kroków i asercji:

- `delivery.actual_outbound` — 300 kroków z walidacją co krok, wymaga fizycznego pobrania i zapisuje checkpoint,
- `delivery.actual_completion` — wczytuje checkpoint, wykonuje pozostałe 308 kroków i wymaga fizycznej dostawy.

`actual_single` pozostaje ręcznym przypadkiem poza CTest do pomiaru całego procesu. CTest wiąże oba etapy przez `FIXTURES_SETUP`/`FIXTURES_REQUIRED`; brak checkpointu jest błędem, a nie cichym nowym światem.

Wyniki po podziale:

- Debug: outbound 2,63 s, completion 4,77 s w pełnej serii; **218/218** (`build/delivery-return-final-debug-ctest.log`).
- Release/RelWithDebInfo: outbound 0,92 s, completion 0,89 s; **218/218** (`build/delivery-return-final-release-ctest.log`).
- ASan osobno: outbound 2,99–3,14 s, completion 3,56–3,62 s. W trzech kolejnych pełnych seriach po schłodzeniu: outbound 2,81–2,83 s, completion 7,59–8,24 s; każda seria **220/220** (`build/delivery-return-final5-asan-ctest-1.log`, `...-2.log`, `...-3.log`). Wcześniejsza seria w rozgrzanym stanie miała osobny timeout `traversal.retained_ramp_recovery` (`build/delivery-return-split-asan-ctest-2.log`), co potwierdza zmienność ograniczenia hosta, a nie regresję dostawy; po przerwie ten sam przypadek przeszedł w 8,6–9,3 s.
- Ręczny `actual_single` w ASan: 5,70 s w osobnym przebiegu (`build/delivery-return-actual-single-asan.log`).
- Natywne okno: `reality_traversal.exe --smoke-test --delivery --gpu-debug` zakończyło się kodem 0 na NVIDIA GeForce MX330, `gpu_debug=1`, `TRAVERSAL SMOKE PASS`, `simulation_errors=0` (`build/delivery-return-native-final2.log`, `build/delivery-return-native-final2.png`). Migawka smoke ma jeszcze `cargo_quantity=0` i fazę `ToPickup`, więc ta próba nie dowodzi zwrotu ładunku; zwrot jest pokryty testami headless. To próba okna i diagnostyki, nie pomiar długotrwałej wydajności.
- Bitowa zgodność modelu: ten sam scenariusz dał identyczne sumy kontrolne zapisu po 100–600 krokach i na końcu: `4ecfbda3c8789d91`, `13d2ed5e6697f1e0`, `76260d1f80b1be1e`, `6a5b0127abf9f900`, `22b3c34af472fadb`, `5234bc86edabb38d`, `4977674a36000a57`; zapis po zmianie w `build/delivery-return-snapshot-final2-debug.log` i `...-final2-asan.log`, wcześniejsze w `build/delivery-return-profile-before-debug2.log`.

### Granice pomiaru

Czasy pełnych serii zależą od chwilowej częstotliwości i polityki zasilania laptopa: ten sam plik ASan wykonywał pojedynczy scenariusz w 5,5–6,5 s, a jego wcześniejszą wersję w 12–17 s jako pierwszy test długiej serii. Podział na dwa procesy nie zmniejsza liczby kroków ani asercji; ogranicza tylko czas jednego procesu, który na tym hoście przekraczał próg trwałego ograniczenia zasilania. Nie zmieniono limitu 20 s, nie usunięto walidacji i nie skrócono symulacji. Zapis `GENESIS_TRAVERSAL 4` obejmuje stan zwrotu bez nowej wersji; starsze wersje pozostają czytelne.

## Granice

Kurier może zostać przerwany potrzebą, a anulowanie pozostawia rzeczywisty materiał w torbie do czasu autonomicznego zwrotu albo ręcznego oddania. Zwrot wymaga znanego, zgodnego typem stanowiska; bez takiego stanowiska postać zachowuje ładunek i wraca do innych zadań, zamiast tworzyć materiał poza `WorkWorld`. Komunikacja wiedzy, własność/prawo, produkcja, wielozasobowe recepty i pełna ekonomia pozostają otwarte. Limit 4096 rekordów oraz brak retencji ograniczają długość symulacji; czas osiągnięcia limitu zależy od rzeczywistego obciążenia i nie jest tutaj zmierzony. Współpraca nie gwarantuje rozwiązywania dowolnych zakleszczeń.
