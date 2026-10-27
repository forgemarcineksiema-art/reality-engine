# Przejście, objazd i materialna naprawa — weryfikacja

Status: **ograniczony zakres techniczny zweryfikowany** w pełnych konfiguracjach Debug/RelWithDebInfo/ASan, z natywną próbą D3D11 na MX330. [Brief](../work-items/passage-logistics.md) określa zakres SC-10 i części SC-14. Nie jest to odbiór M04 ani fizycznego rozpadu brył.

## Wykonanie i dowody przed naprawami

Testy C++ zapisano przed implementacją API. `evidence/passage-logistics/red-build.log` zachowuje oczekiwane błędy braku PassageOutcome, make_crossing_course i metod sesji; to czerwony dowód kompilacyjny, nie wykonana reprodukcja ruchu na dawnym kodzie. Pierwsze budowanie integracji odrzuciło również przypadkową deklarację w złym miejscu oraz shadowing przy /W4 /WX (`first-build.log`), bez wyciszania ostrzeżeń. Pierwszy runtime dał 8/9 (`first-runtime.log`): fixture step_refusal miała dwie osoby w tym samym miejscu. Poprawiono wyłącznie położenie nieuczestniczącej osoby; kryteria odmowy i ruchu pozostały.

Niezależny review wskazał pierścień 1,5–2 m: target zatrzymywał niewiedzącą osobę przed zasięgiem obserwacji. `ring-red.log` odtwarza porażkę, `core-tests.log` zawiera 10/10 po naprawie. Walk może teraz dojść do fizycznego kontaktu, a rzeczywista bariera nadal blokuje wejście.

Osobno odtworzono przerywanie długiego wykonalnego objazdu przez wygaśnięcie raportu co 100 kroków (`long2-red.log`). Krótsza próba kontrolna w `long-red.log` przechodziła i nie wykazała tej usterki. Poprawka nie odmładza raportu: upływ wieku pozwala na nowy zwiad/ponowienie NoPath, ale sam nie kasuje wykonywanego objazdu. Nowsza pozytywna obserwacja może go skrócić. `refined-tests.log` zachowuje również błędne oczekiwanie testu protected_ride: po wyjściu żądano natychmiastowego przeliczenia wewnętrznej maski mimo niezmienionej wykonalnej trasy. Końcowa regresja wymaga fizycznego wyjścia, zachowanego raportu i braku przerwania/teleportacji.

Kolejny review wskazał utratę obserwacji na geometrycznym banku z historycznym Step bez etykiety kolejki. `audit-red.log` potwierdza przypadek unlabelled_bank; pozostałe trzy przypadki czynności, clearance i kosztu dostawy przechodziły. Naprawa wiąże obserwację z powierzchnią przejścia oraz końcami połączenia, zachowując możliwość odczytu dawnych Step0. Niezerowe ID połączeń nadal wymagają poprawnego powiązania z passage.

Po pierwszym pełnym Debug 263/263 (`full-debug.log`) dodano także `open_observation`: osoba przy otwartym przejściu bez linku Maintain nie zapisywała jeszcze obserwacji. `open-red.log` dowodzi braku. Końcowa obserwacja obejmuje wszystkie geometryczne przyczółki. Native dawnej sceny dostawy zbiera więc również wiedzę o przejściu i zapisuje format 6; `open-green-and-native.log` zachowuje porażkę dawnego oczekiwania wersji 5. Konsument wymaga teraz dokładnie wersji 6 oraz obu rodzajów rekordów wiedzy, zachowując kontynuację nowego procesu i ochronę pliku. Historyczne formaty sprawdzają nadal wcześniejsze regresje. Następnie powtórzono pełne konfiguracje.

Ostatnia kontrola odtworzyła przyjęcie logicznie sprzecznej maski planowania: snapshot istniejącej trasy przez kładkę otrzymał maskę wykluczającą tę kładkę, z prawidłowym checksumem (`mask-red.log`). Czytnik odrzuca teraz nie-Walk nogi trasy dotykające przejścia wykluczonego maską używaną przez ten plan. Nie odrzuca odłożonych wiadomości: chroniona operacja zachowuje dawną trasę z dawną maską. Niezależny recenzent potwierdził zgodność reguły z filtrem planera; po zmianie ponowiono pełne konfiguracje.

## Granice implementacji i testów

PassageState posiada open i opcjonalny indeks Maintain. Dane planera pochodzą z własnych passage_beliefs; globalna wersja geometrii nie jest wiadomością. Nierozpoczęta kolejka może zostać zwolniona, ale kapsuła na przejściu, rozpoczęty Step i związany clearance uniemożliwiają zamknięcie. Przy wear==1000 i zajętości otwarte przejście z progiem zużycia jest poprawnym stanem oczekującym na bezpieczną próbę. Naprawa zużywa rzeczywisty materiał typu 2 i trwa 80 kroków. Żywność typu 1 pozostaje fizyczną partią WorkWorld; wiedza nie dostarcza zasobu.

W izolowanych testach ochrony operacji seeduje się jawnie historyczny raport w pamięci świadka. Nie zmienia to ruchu, pozycji, kosztu, fazy czynności ani stanu faktycznego przejścia; kontakt share_knowledge i fizyczna kontynuacja są wykonywane. Te fixture nie dowodzą pierwotnej obserwacji raportu. Rzeczywiste nabycie informacji, objazd, dostawa, posiłek i naprawa mają osobne testy integracyjne.

`repair` sprawdza koszt materiału i rzeczywiste ukończenie, potem przekaz otwarcia, fizyczne pobranie czterech jednostek żywności, przejście z partią przez naprawioną kładkę i oddanie odbiorcy. Jest to ręcznie prowadzona dostawa przez przywróconą trasę. Autonomię sprawdzają osobno discovery oraz community: wariant spokojny i zaburzony wykonują po 550 kroków z konserwacją, dostawą i posiłkiem. Test delivery_cost porównuje pierwszą rzeczywistą dostawę przy wyłączonym autonomicznym naprawiaczu w obu wariantach: **krok 172 przez otwartą kładkę, 325 po zamknięciu i objazdem promem**, różnica 153 kroków / 7,65 s czasu świata (`delivery-cost.log`). Nie jest to pomiar wydajności aplikacji.

Niezależny `passage_review` wykonał kilka odczytowych przeglądów. Wskazał dwie odtworzone luki ruchu/obserwacji oraz braki dowodów dla clearance, czynności i korupcji; zostały uzupełnione. Końcowe APPROVE obejmuje także dokumentację, rozszerzenie repair o prawdziwy ładunek oraz regułę zgodności maski z trasą. Recenzent nie wykonywał buildów/testów; odczytał logi, a wszystkie komendy wykonał główny przebieg. Nie przypisujemy recenzentowi prób GPU ani oceny komfortu.

## Pełne kontrole

Wszystkie przebiegi wykonano sekwencyjnie, na kodzie zawierającym także ostatnią regułę zgodności maski planowania z trasą (`mask-red.log`). Nie zmieniono limitów CTest, nie dodano skipów i nie skrócono scenariuszy. Dowody zachowujemy w `docs/verification/evidence/passage-logistics/`; porażki pozostają obok końcowych wyników. Review jest odczytowy; buildy wykonuje główny przebieg.

| Polecenie | Wynik i dowód |
| --- | --- |
| `cmake --build --preset windows-debug` | Sukces, `verified-debug-build.log` |
| `ctest --preset windows-debug --output-on-failure` | **264/264**, 105,83 s, `verified-debug-tests.log` |
| `cmake --build --preset windows-release` | Sukces, `verified-release-build.log` |
| `ctest --preset windows-release --output-on-failure` | **264/264**, 65,66 s, `verified-release-tests.log` |
| `cmake --build --preset windows-asan` | Sukces, `verified-asan-build.log` |
| `ctest --preset windows-asan --output-on-failure` | **266/266**, 140,61 s, `verified-asan-tests.log`; osobna próbka kontrolna wykryła zamierzony heap-buffer-overflow |
| `cmake --build --preset windows-debug --target format-check` | Sukces (kod 0), `format-check-final.log` |
| `python tools/check_docs.py` | PASS, `docs-check-final.log` |
| `python -m unittest discover -s tools/tests -p "test_*.py"` | **29/29**, `python-tests-final.log` |

Zestaw `passage.*` obejmuje 27 przypadków, w tym `discovery`, `witness`, `repair`, `community`, `delivery_cost`, `long_detour`, `observation_ring`, `unlabelled_bank`, `open_observation`, `corruption` i `native_diagnostics`.

Czasy wybranych przypadków w pełnych seriach (nie benchmark wydajności):

- Debug: `delivery.actual_outbound` 1,80 s, `delivery.actual_completion` 1,73 s, `passage.native_diagnostics` 1,68 s.
- Release/RelWithDebInfo: 0,55 s, 0,54 s, 2,56 s.
- ASan: 4,13 s, 2,15 s, 2,91 s; `traversal.retained_ramp_recovery` 8,62 s przy niezmienionym limicie 20 s.

Dwuetapowy scenariusz dostawy zachowuje checkpoint po 300 krokach, łączną liczbę kroków i asercji oraz limit 20 s.

### Natywne okno D3D11

`python tools/test_crossing_native.py --exe ./build/msvc/RelWithDebInfo/reality_traversal.exe --output ./docs/verification/evidence/passage-logistics/native-final` zakończył się kodem 0 (`native-probe-final.log`). Pierwszy proces `--crossing --smoke-test --gpu-debug` wybrał `selected_adapter=NVIDIA GeForce MX330` z `hardware=1` i `gpu_debug=1`, zapisał `passage_shortcut=pass`, `maintenance_shortcut=pass`, `crossing_delivery=pass`, `passage_inspector=pass`, `TRAVERSAL SMOKE PASS`, `simulation_errors=0` oraz `normal_shutdown=1` (`native-final/crossing.log`). Zapis rozpoczyna się od `GENESIS_TRAVERSAL 6` i zawiera `PASSAGE_BELIEF`. Drugi, świeży proces wznowił ten sam plik: `passage_resume=pass`, `simulation_errors=0`, a bajty pliku wejściowego pozostały niezmienione (`native-final/resume.log`).

Obrazy `native-final/crossing.png` i `native-final/crossing.png.closed.png` pokazują scenę 3D przed i po zamknięciu: najpierw ciemne, otwarte przęsło z postacią na nim, potem czerwona powierzchnia już bez osoby na przejściu. To dowód uruchomienia, renderu i obsługi skrótów, nie systemowe wejście człowieka ani ocena czytelności.

### Ograniczenia końcowych kontroli

Czasy pełnych serii zależą od chwilowej częstotliwości i polityki zasilania laptopa; wcześniejsze, wolniejsze przebiegi tego samego kodu pozostają w katalogu dowodów. Rekordy `delivery.actual_*` i `traversal.retained_ramp_recovery` są czasami runnera, nie FPS. Natywna próba obejmuje skrypt smoke i dwa procesy, nie pełną sesję człowieka. Zamknięta kładka jest blokadą użytkowania powierzchni, nie symulacją fizycznego zniszczenia bryły. Commit i publikacja nie zastępują odbioru M04, oceny komfortu ani pomiaru długotrwałej wydajności.
