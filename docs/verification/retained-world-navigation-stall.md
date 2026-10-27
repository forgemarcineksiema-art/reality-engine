# Naprawa odzyskiwania dojścia przy blokadzie

Status: naprawa nieaktualnej trasy i ponowień ZALICZONA technicznie. Pełne profile i końcowa kontrola okna przeszły. Zakres określa [zlecenie](../work-items/retained-world-navigation-stall.md). Naprawa nie oznacza zakończenia M04 ani rozwiązania społecznego konfliktu przy rampie.

## Przyczyna i zmiana

Zachowana migawka przy 6938 odtwarza krążenie osoby 7. Ślad przed naprawą (`build/stall-before-trace.log`) pokazuje ruch i wzrost `plans` z 29 do 33, 37 i dalej przy tym samym odcinku. Odzyskiwanie jednak się uruchamiało; początkowa hipoteza, że nie może się uruchomić przez zerowane `blocked_ticks`, nie wyjaśniała całego przypadku. Po nieudanym `choose_approach` pozostawała stara trasa Found. To jest potwierdzony defekt.

Po nieudanym wyborze innego dojścia wykonawca ponownie sprawdza dotychczasowy punkt końcowy z aktualnie obserwowanymi stojącymi sąsiadami. Wynik NoPath/InvalidGoal/BudgetExceeded zastępuje nieaktualny plan. Dokładny cel gracza pozostaje; dojście w zasięg nadal może wybierać wyłącznie dozwolone punkty tego zasięgu. Nie przesuwamy innych osób i nie teleportujemy.

Ponowienia dojścia rozważają alternatywne punkty, również gdy na początku nie udało się wybrać żadnego. Wariant bez wybranego celu jest ograniczony do ręcznego NoPath, aby nie wprowadzać konkurencyjnego wykonawcy potrzeb. Próbę oddziela co najmniej 40 kroków, także po udanym wyborze. Limit odzyskiwania jednej trasy to cztery wyszukiwania kandydatów oraz jedna ponowna kontrola starego punktu, jeśli kandydaci zawiodą. Niezależny wybór potrzeb NPC może wykonywać własne wyszukiwania stanowisk i nie jest tym limitem objęty. Stąd test dopuszcza do dziesięciu wyszukiwań przez 80 kroków; asercje stabilności pozycji i obrotu pozostają niezmienione. Zapis, prędkości, kolizja, progi diagnostyki i ochrona rozpoczętego Step/przejazdu pozostają.

## Regresje i dowody

- `build/stall-red.log`: zamknięte dojście i rzeczywista migawka nie przeszły na starej implementacji; dostępne obejście przeszło.
- `build/stall-retry-red.log`: początkowo zajęty zasięg nie był ponawiany, mimo późniejszego dostępnego punktu i nadal zajętego środka.
- `approach_blocked_recovery`: płaskie/pochylone i obrócone podłoże, pojawienie się blokady po zaplanowaniu, stabilny NoPath, zachowany zamiar, nieprzesuwany sąsiad, ograniczony koszt, identyczny zapis i wznowienie po odejściu blokującego.
- `approach_recovery_detour`: rzeczywiste dojście obejściem na szerszym podłożu.
- `retained_ramp_recovery`: prawdziwa migawka, NoPath z zachowanym zamiarem, 80 kolejnych kroków bez krążenia/obracania i zgodna kontynuacja odczytu. Nie wystarcza samo opuszczenie powierzchni.
- `approach_alternative_retry`: początkowo nie ma wybranego punktu; po odejściu dużej kapsuły środek nadal jest zajęty, a dozwolona alternatywa zostaje osiągnięta.
- `approach_existing_retry`: wybrany punkt istnieje, potem powstaje NoPath; stary punkt pozostaje zajęty, a postać osiąga inny dozwolony punkt.

Pierwszy fixture ostatniego przypadku miał niedomkniętą barierę przy krawędzi podparcia (`build/stall-existing-trace.log`). Zwiększono rozmiary i odstępy zewnętrznych kapsuł, aby rzeczywiście zamknąć przejście; nie zmieniano oczekiwanego zachowania. `build/stall-existing2-trace.log` potwierdza zaliczenie poprawionego fixture. Wcześniejszy `build/stall-final-focused.log` zawiera tę dawną porażkę i nie jest końcowym dowodem sukcesu.

## Niezależny przegląd

Osobny agent `navigation_review` otrzymał tylko zakres, kontrakty i pliki, bez historii rozumowania autora. Przegląd był odczytowy, bez zmian i bez wykonywania buildów/testów. To drugi agent tego samego systemu, nie zewnętrzny audyt ani niezależne uruchomienie.

Wskazał P1: ponowienia starego punktu bez szukania alternatyw; P2: zbyt słaba asercja prawdziwej migawki; dodatkowo termin po udanej próbie. Wszystkie uwagi zweryfikowano i uwzględniono. Końcowy werdykt: APPROVE dla zakresu poprawki, bez pozostałych istotnych uwag. Werdykt został uzupełniony finalnymi wynikami integracji poniżej; nie zastępuje odbioru człowieka.

## Kontynuacja i granice

`build/stall-after-retained.log` dokumentuje 1000 kroków od zachowanej migawki. Osoba 7 przestaje wykonywać krążącą trasę i pozostaje stabilnie w NoPath. Osoba 6 nadal czeka w rozpoczętym Step, którego koniec blokują inni mieszkańcy. Chronionego kroku nie kasowano. Ten wynik rozróżnia naprawione wykonywanie nieaktualnej trasy od rzeczywistego konfliktu przestrzennego, którego samo ponowne A* nie rozwiąże.

Nie twierdzimy, że scena odzyskała sprawną komunikację wszystkich mieszkańców. Prośby o ustąpienie, zgoda/odmowa i bezpieczne rozwiązanie wzajemnej blokady pozostają osobnym przyrostem M04. Sam brak alarmu osoby 7 nie dowodzi dostawy, gospodarki ani społecznej inteligencji. Zachowanej migawki nie zmieniano.


## Końcowa integracja

Z katalogu głównego wykonano kolejno `cmake --build --preset windows-debug` i `ctest --preset windows-debug`, analogicznie `windows-release` oraz `windows-asan`. Debug: **168/168**, RelWithDebInfo: **168/168**, ASan: **170/170**, w tym zamierzony błąd pamięci oddzielnej próbki. Wszystkie wcześniejsze regresje pozostają; nie zmieniano timeoutów ani skipów. Dowody: `build/stall-final-debug.log`, `build/stall-final-release.log`, `build/stall-final-asan.log` oraz odpowiednie `*-build.log` i `*-console.log`.

`cmake --build --preset windows-debug --target format-check` przeszedł (`build/stall-final-style.log`). `python -m unittest discover -s tools/tests -p "test_*.py"`: **29/29**, `build/stall-final-tooling.log`. Rejestr dokumentacji i linki sprawdzono poleceniem `python tools/check_docs.py`.


### Rzeczywiste okno po naprawie

RelWithDebInfo z `--gpu-debug`, MX330, widok 844×781. Z nowej testowej kopii F9 odtworzył 6938; wybrano osobę 7 i uruchomiono czas. Przy dalszej obserwacji 7546 widok pokazywał „Brak dostępnego planu”. Pauza przy 7700, F6, krok N do 7720 i ponowne F6 zachowały stabilny stan. Porównanie pełnej precyzji pozycji/orientacji aktora w automatycznej migawce 7038 oraz ręcznych 7700 i 7720 dało identyczne wartości. To potwierdza brak ukrytego drobnego krążenia, nie tylko zmianę etykiety.

Dowody: `build/stall-ui.log` i trzy katalogi `build/stall-ui-captures/`; zwykła testowa kopia `build/stall-ui.save` pozostała identyczna SHA256 z wejściową migawką (`D0767E6342DE923B6F89B8A543F03C6DAE5D5547E4E2CC73B32B2CD0319B0D37`). Warstwa D3D11 debug aktywna; normalne zamknięcie, bez zgłoszonego błędu. Nadal widoczna blokada osoby 6 nie została wyciszona: utworzyła osobną automatyczną migawkę. Ta próba techniczna nie jest benchmarkiem FPS ani nową oceną komfortu przez użytkownika.

Aktualizacja po późniejszym przyroście: [lokalna współpraca](local-clearance.md) rozwiązuje zachowaną blokadę osoby 6, a pełny pierwotny świat otrzymał osobny test rzeczywistego przejścia. Powyższe wyniki opisują wersję przed tym mechanizmem. Kontrolna regresja stabilnego NoPath zachowuje ręcznie sterowanego, odmawiającego blokującego.
