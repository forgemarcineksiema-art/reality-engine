# Diagnostyka i bezpieczne przerywanie potrzeb

Status: zakres diagnostyki i bezpiecznych potrzeb ZALICZONY technicznie; pozostała stagnacja nawigacji ma osobne otwarte zgłoszenie i blokuje uznanie sceny za domkniętą przed dostawami. Zakres określa [plan](../work-items/diagnostics-needs-hardening.md). Bez commitów i delegowania, zachowano poprzednie zmiany oraz formaty WorkWorld/MotionSession.

## Zachowanie

Inspektor oddziela rekordy CRLF wyłącznie w widoku Win32, zachowuje przewijanie i zaznaczenie. Nowe alarmy są przypisane do trwałego ID i nie giną podczas trwającego alarmu innej osoby. Limit ośmiu automatycznych prób pozostaje, ręczne F6 jest niezależne.

Odczytowy kontekst sesji rozróżnia marsz, parkowanie, Step, wsiadanie/wysiadanie i jazdę. Obserwacja kredytuje rzeczywisty postęp w układzie podpory oraz fazę usługi; przestawianie waypointu i krótkie Waiting nie kasują stagnacji zamiaru. Jawna blokada fizyczna, właściciel i kolejka trafiają do raportu. Sama diagnostyka nie steruje postaciami.

Krytyczne potrzeby bezpiecznie przerywają niepilny plan dopiero po znalezieniu nowej trasy. Zachowują rodzaj i stanowisko odłożonej pracy. Zwykłe progi wyboru, przyrost potrzeb, 3 m/s i zegar pozostają. Chronione są rozpoczęta czynność oraz fizyczny Step/przejazd. Jednorazowe 120 kroków na końcowe dojście do 6 m zapobiega powtarzanemu zawracaniu tuż przed stanowiskiem, ale wygasa nawet bez ukończenia. Komendy gracza usuwają odłożone obowiązki.

Zewnętrzny zapis 2 zawiera nowe terminy i odłożone zamiary. Czytnik zachowuje wersję 1; serializer bez nowego stanu emituje dawne bajty. Historyczna próbka `tests/fixtures/traversal-v1-needs.save` pochodzi z wcześniejszego `build/m04-entry-ui.save`.

## Dowody przed naprawą

- `build/diagnostics-needs-before.log`: testy krytycznego głodu i wyczerpania oraz natywnego podziału rekordów nie przeszły na bazie.
- `build/diagnostics-needs-churn-red.log`: zmiana waypointu/Waiting ukrywała stacjonarny zamiar.
- `build/diagnostics-needs-independent-red.log`: przestawienie obserwowanych osób ujawniło wiązanie obserwacji z indeksem zamiast ID.
- `build/diagnostics-needs-resume-red.log` i `build/diagnostics-needs-resume-trace.log`: pierwsza wersja pilności zawracała przed konserwacją i nie kończyła pracy. Zachowano wymaganie rzeczywistego posiłku, wznowienia i ukończenia, dodając ograniczoną ochronę końcowego dojścia.
- `build/diagnostics-needs-scroll-red/native.log`: `ERROR: live inspector update reset the reading position`, rzeczywista warstwa D3D11 debug na MX330.

Próba `diagnostics_live_view` z całą sceną i serializacją każdego kroku przekroczyła 20 s (`build/diagnostics-needs-expanded.log`). Zmniejszono scenę do pojedynczego użytkownika rzeczywistego promu, zachowując 600 kroków, wymóg Parking/Precision/Riding oraz bajtową kontrolę braku mutacji na każdym kroku. Nie podniesiono timeoutu. Pierwsza asercja terminu ochrony myliła zegar przed decyzją z zegarem po kroku; poprawiono fixture do rzeczywistej granicy tick, bez zmiany implementacji lub długości ochrony.

## Kontrole

Wykonano z katalogu głównego, bez wyłączeń i podnoszenia limitów czasu:

| Kontrola | Rzeczywisty wynik | Dowód |
| --- | --- | --- |
| `cmake --build --preset windows-debug`, `ctest --preset windows-debug` | 163/163 | `build/diagnostics-needs-final-debug-build.log`, `build/diagnostics-needs-final-debug.log` |
| `cmake --build --preset windows-release`, `ctest --preset windows-release` | 163/163 | `build/diagnostics-needs-release-build.log`, `build/diagnostics-needs-release.log` |
| `cmake --build --preset windows-asan`, `ctest --preset windows-asan` | 165/165, w tym wykrycie zamierzonego overflow osobnej próbki | `build/diagnostics-needs-asan-build.log`, `build/diagnostics-needs-asan.log` |
| `cmake --build --preset windows-debug --target format-check` | ZALICZONE | `build/diagnostics-needs-final-style.log` |
| `python tools/check_docs.py` | ZALICZONE | ponowione po końcowej edycji dokumentów |
| `python -m unittest discover -s tools/tests -p "test_*.py"` | 29/29 | także w pełnych CTest powyżej |

Późniejsze rozszerzenie obserwacyjnego `--resume-course` o opcjonalne zapisy nowych katalogów sprawdzono rzeczywistym wywołaniem RelWithDebInfo z pięcioma migawkami. Po jego integracji przebudowano runner ASan i kontynuowano zachowaną migawkę od 6938 do 7938 bez błędu pamięci lub walidacji (`build/diagnostics-needs-final-audit-asan-build.log`, `build/diagnostics-needs-final-audit-asan.log`). Otwarty problem osoby 7 jest nadal obserwowany; nie zastępowano go zaliczoną asercją o osiągnięciu celu.

### Rzeczywiste okno

Własna kontrola techniczna przez Computer Use, RelWithDebInfo z `--gpu-debug`, NVIDIA MX330, widok 844×781: F9 wczytał rzeczywistą migawkę wersji 2 przy 6938; przewinięto inspektor do rekordów kolejnych postaci, N wykonało 20 kroków do 6958 bez cofnięcia widoku. F6 utworzył osobny katalog, F5 zapisał testową kopię, kolejne N przeszło do 6978, F9 przywrócił 6958. Okno zamknięto normalnie. Dowody: `build/diagnostics-needs-ui.log`, `build/diagnostics-needs-ui.save`, `build/diagnostics-needs-ui-captures/case-156245921-1/`. To obserwacja autora, nie nowa akceptacja odczucia przez użytkownika.

Osobny nowy proces `reality_traversal --smoke-test --resume-smoke --gpu-debug` wczytał powyższy zapis wersji 2, wykonał dalszą symulację i zakończył `TRAVERSAL SMOKE PASS`, `inspector_records=pass`, `inspector_scroll=pass`, `normal_shutdown=1`. Porównanie SHA256 przed/po potwierdziło brak nadpisania wejściowej migawki; `build/diagnostics-needs-v2-native.log`. Warstwa D3D11 debug była aktywna, bez zgłoszonego błędu. Pięciominutowego pomiaru na tej wersji nie wykonywano.

## Kontynuacja wcześniejszego świata

RelWithDebInfo, `reality_traversal_audit_tests --resume-course build/m04-entry-measure-300.save.final.save --diagnostics build/diagnostics-needs-retained-captures`: wczytanie kroku 6599, 1000 walidowanych kroków do 7599. Stan źródłowy pozostał bez zapisu. Łącznie 16 posiłków, 5 odpoczynków i 7 konserwacji, czyli +7/+1/+0 względem wejścia. Ta sama scena z poprzednią polityką dawała +4/+2/+1. To jednostkowa deterministyczna kontynuacja, nie statystyczny dowód lepszego bilansu społeczności.

Nowy detektor zgłosił 5 epizodów dla czterech osób. Przy końcu dla osoby 7 nadal wykazuje 761 kroków stagnacji zamiaru od 6838; inne osoby odzyskały postęp. Zachowano [osobne otwarte zgłoszenie i rzeczywistą migawkę](../work-items/retained-world-navigation-stall.md). Ten wynik nie pozwala uznać wszystkich blokad za rozwiązane ani rozpocząć dostaw bez rozstrzygnięcia problemu. Wykrycia nie usuwano przez zmianę progu. To dowód użyteczności obserwatora oraz pozostałej luki nawigacji, nie zakończenie naprawy tej luki.

## Ograniczenia i przegląd

Przegląd autora, bez niezależnego review: własność stanu, zwalnianie nieprzydzielonych kolejek, ochrona zajętej kładki, brak zdalnego odczytu zapasów, koszty rozpoczętej czynności, walidacja stosu/referencji, transakcyjny odczyt i stabilne ID obserwacji. Finalne kontrole integracji opisano powyżej; ograniczenie braku niezależnego review pozostaje jawne.

Detektor nadal może nie rozpoznać wielkiej pętli przestrzennej i nie dowodzi kompletności nawigacji. Legalne oczekiwanie na właściciela nie jest samo alarmem; raport umożliwia powiązanie go z obserwacją właściciela, bez pełnej analizy cykli zależności. Skończone zapasy, ograniczona wiedza i istniejące miejsca pracy nie tworzą gospodarki. Nie ma jeszcze transferu materiału, dostaw ani negocjacji. Nie zmieniano ruchu i fizyki w celu udawania ich realizacji.

Dawne 300 s pomiaru dotyczy poprzedniego programu. Ta zmiana wymaga własnych danych dla nowego twierdzenia o wydajności; krótkie kontrole GPU i headless go nie zastępują. Odbiór techniczny nie jest oceną komfortu przez użytkownika ani zakończeniem M04.
