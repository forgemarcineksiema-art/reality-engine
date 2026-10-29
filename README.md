# Projekt Genesis / Reality Engine

**Uczenie w prototypie:** `python tools/start_practice_playtest.py` uruchamia tę samą ośmioosobową społeczność z aparatem do prób. Mieszkańcy poznają ustawienie potrzebne do konserwacji, przekazują pomiary przy spotkaniu i wykonują rzeczywistą pracę. Gracz może przejąć postać. [Zakres i dowody](docs/verification/practice-in-prototype.md); użytkownik zaakceptował obecne uczenie w prototypie 28.10.2026.

**Dwie metody dostawy (przyrost M05):** `python tools/start_practice_playtest.py --two-methods`. Napraw zużyty most dzięki poznanej metodzie albo uruchom dostawę istniejącym promem. [Sterowanie i stan weryfikacji](docs/verification/m05-two-methods.md).

**Jeden kurier lokalnie i daleko:** `python tools/start_distant_playtest.py` uruchamia tę samą aplikację z kursem pomiędzy statycznymi terminalami. W podróży kurier znika z lokalnej fizyki i wraca z tą samą partią. [Zakres i stan kontroli](docs/verification/m05-distant-delivery.md).

[Doświadczenia rozdzielające wyjaśnienia](docs/verification/discriminating-experiment.md): ukończono audyt, próbę potwierdzającą i fizyczne zadanie konserwacji dwóch mieszkańców. I i A otrzymały wsparcie; T pozostała negatywna. [API i komendy](docs/discriminating-learning.md).

Action RPG w przyziemnej, realistycznej i brutalnej cywilizacji fantasy, łączące otwarty świat z autonomiczną symulacją społeczeństwa. Własny Reality Engine, fizyka i nawigacja. Zatwierdzoną wizję i nadal otwarte szczegóły oddzielają DEC-008 i DEC-011 w [rejestrze decyzji](docs/decisions.md).

**Stan:** zaliczono M00–M04 w opisanym zakresie: fundament, czynności oraz własny ruch kinematyczny 2,5D, kolizje, A*, zmienne przejścia i zapis podróży. Tryb `--motion` działa w oknie Win32/D3D11 na MX330. [Raport M03](docs/verification/m03-motion-navigation.md) obejmuje akceptację obecnego sterowania przez użytkownika, z uwagami do płynności prowadzenia do celu, szybkości i naturalności ruchu. Istnieje ograniczona autonomia potrzeb, pierwszy zakres dostaw w laboratorium przestrzennym oraz autonomiczny zwrot nieprzypisanego ładunku; wspólna mała społeczność ma [zaliczony odbiór M04](docs/verification/m04-acceptance.md), potwierdzony przez użytkownika 27.10.2026. Git używa gałęzi `main`, publikowanej w publicznym `origin` na GitHubie, i wersjonuje punkt wyjścia projektu. Zasady pracy, gałęzi tematycznych i zakres archiwum opisuje [przepływ pracy](docs/workflow.md#repozytorium-git).

**Pierwsza dostawa:** ograniczony [transfer i fizyczna dostawa](docs/verification/material-delivery.md) w osobnej scenie `reality_traversal --delivery`. F2 przejmuje sterowanie, a F4 służy do lokalnego pobrania lub oddania materiału. Po anulowaniu kursu autonomiczna postać odkłada niesiony materiał w znanym, zgodnym typem stanowisku; ręczne przejęcie zatrzymuje ruch, ale nie teleportuje partii. Scenariusz pełnej dostawy jest testowany dwuetapowo z checkpointem po 300 krokach, aby każdy proces ASan zmieścił się w niezmienionym limicie 20 s; łączna liczba kroków i asercji pozostaje bez zmian. Ocena zewnętrznych dokumentów Argon i znane ograniczenia są w [planie przyrostu](docs/work-items/material-delivery.md); przyrost nie jest pełnym odbiorem M04.

Zatwierdzony przyrost [prowadzenia do celu](docs/verification/navigation-polish.md) dodaje bezpośrednie odcinki, skróty sprawdzane kolizją i stabilne ponawianie celu przy zachowanej prędkości. Raport odróżnia jego weryfikację od wcześniejszej oceny użytkownika dla bazowego M03.

[Późniejszy audyt praktyczny](docs/verification/motion-adversarial-audit.md) odtworzył i naprawił utknięcie na kontakcie przy ścianie, bezterminowe czekanie za osobą na otwartej podłodze i walidację skrajnych danych. Obraz jest teraz interpolowany po rzeczywistej drodze ostatniego kroku, także przez zakręty. Zakres nadal jest 2,5D; trwa etapowe przygotowanie wejścia w M04.

Build i pierwsze testy, z katalogu projektu:

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-debug
ctest --preset windows-debug
./build/msvc/Debug/reality_lab.exe
./build/msvc/Debug/reality_lab.exe --motion
```

Pełny CTest wymaga sprzętowego GPU i systemowych Narzędzi grafiki, zainstalowanych podczas odbioru M02. Bez tego składnika test diagnostyki zwraca błąd. Pozostałe konfiguracje i sterowanie opisuje [środowisko pracy](docs/development.md). Obecny zakres określają [model czynności](docs/work-domain.md), [laboratorium okienkowe](docs/window-laboratory.md) oraz [ruch i nawigacja](docs/motion-navigation.md). Dowody wcześniejszych etapów: [M00](docs/verification/m00-build-and-diagnostics.md), [M01](docs/verification/m01-identity-actions-reservations.md), [M02](docs/verification/m02-window-laboratory.md).

Pierwszy rezultat ma łączyć grywalny fragment i laboratorium małej autonomicznej społeczności. Grafika ma najniższy priorytet; czytelność i sterowanie pozostają wymaganiami. Użytkownik pracuje ze swoimi agentami AI na laptopie i5/MX330/8 GB RAM, bez narzuconego terminu.

## Laboratorium odkrywania

Osobny [eksperyment odkrywania i przekazu](docs/discovery-learning.md) rozwija laboratorium przez nowy konsument headless `reality_discovery`. Agent poznaje nieznane parametry z pomiarów, sprawdza nowe konfiguracje i przekazuje dane przy fizycznym kontakcie. Rodziny praw i metoda uczenia są dostarczone przez autora. [Raport](docs/verification/discovery-learning-experiment.md) oddziela kontrole techniczne od wyniku D/G/S; nie jest to świadomość ani nowy odbiór społeczności.

## Odbiór M04

[Aktualny audyt społeczności](docs/verification/m04-acceptance.md) łączy potrzeby, dostawę, przeprawę, wiedzę i prawo we wspólnej scenie `--community` z ośmioma osobami. `python tools/start_m04_playtest.py` uruchamia świeży świat z osobnym katalogiem zapisu i logu. Panel pokazuje role i faktyczne skutki. [Krótka próba użytkownika](docs/verification/m04-playtest.md) zawiera jego akceptację M04 z 27.10.2026, odrębną od kontroli technicznych.

## Od czego zacząć

[Przejście i odtworzenie logistyki](docs/verification/passage-logistics.md) łączy bezpieczne zamknięcie kładki, lokalną wiedzę, rzeczywisty objazd promem i naprawę zużywającą materiał. Nową scenę uruchamia `reality_traversal --crossing`: F8 zmienia pobliskie przejście ręcznie sterowanej postaci, R rozpoczyna lokalną konserwację, F7 przekazuje także obserwacje przejść. Inspektor rozdziela prawdę i przekonanie. Nowy stan używa formatu 6 z odczytem 1–5. To kontrolowany zakres SC-10, bez pełnego odbioru M04 ani symulacji zawalenia konstrukcji.

[Lokalna wiedza o stanowiskach](docs/verification/local-station-knowledge.md) dodaje historyczne przekonania postaci, fizyczną obserwację na miejscu i przekaz między bliskimi osobami. F7 przekazuje obserwacje wybranej postaci najbliższemu rozmówcy; inspektor pokazuje osobno `belief` ze świadkiem, pośrednikiem i czasem oraz faktyczne `station`. Nowsza informacja o dostawie może przyspieszyć ponowienie, a wiadomość o braku zmienić bezpieczny plan. Zapis wiedzy używa `GENESIS_TRAVERSAL 5`, z odczytem 1–4. To ograniczony przyrost wejścia w M04; stan weryfikacji i granice wskazuje raport.

[Pierwszy przyrost przed dostawą](docs/verification/m04-entry-observability.md) dodał obserwację postępu, osobne migawki F6 i rzeczywisty pomiar aplikacji. Raport zawiera pięciominutową próbę na MX330, kontynuację końcowego świata i ograniczenia ówczesnego wyboru potrzeb. Transfer i ograniczona współpraca powstały w późniejszych przyrostach opisanych niżej.

[Naprawa reakcji na blokadę](docs/verification/navigation-obstruction-recovery.md) usuwa powtarzanie planu ignorującego stojącą osobę oraz wybór zajętego miejsca oczekiwania. Raport zawiera dwie czerwone reprodukcje, sprawdzenie obejść i próbę rzeczywistego zwolnienia kładki. Negocjowanie ustąpienia pozostaje osobnym projektem dalszego zachowania.

[Praktyczny audyt toru przestrzennego](docs/verification/traversal-multi-actor.md) weryfikuje kolejki, mijanie, hamowanie, podparcie i zapis nowej wersji. Raport odróżnia wyniki techniczne oraz próbę rzeczywistego wejścia od nadal osobnej oceny komfortu przez użytkownika.

W rozwijanym torze przestrzennym `reality_traversal` [raport napraw poleceń i przejść](docs/verification/traversal-command-hardening.md) opisuje odrębny odbiór regresji: odroczone cele, ręczne wyjście z przejścia i ciągłe podparcie między sąsiednimi płytami. Pełne rozszerzenie geometrii/autonomii pozostaje w realizacji; nie zmienia to statusu M04.

Ograniczony [model potrzeb laboratorium](docs/traversal-autonomy.md) łączy fizyczne dojście ze wspólnymi rezerwacjami i skutkami M01. [Przyrost wyboru zadania](docs/verification/autonomy-need-fallback.md) pozwala wybrać potrzebny odpoczynek po niedostępności jedzenia; nie daje postaci wiedzy o zdalnym zapasie.

Nowe sesje mają [wielorazową pojemność odpoczynku](docs/verification/reusable-capacity.md): miejsce jest zajęte podczas czynności i zwalniane po jej zakończeniu, bez zużywania go jak materiału. Historyczne zapisy materiałowe zachowują dawne reguły. Jest to dalszy fundament przed pełnym M04.

1. [AGENTS.md](AGENTS.md) — instrukcja pracy dla agentów i współtwórców.
2. [engineering-rules.md](engineering-rules.md) — obowiązujące zasady implementacji i dowodzenia poprawności.
3. [Karta projektu](docs/project-charter.md) — ustalenia użytkownika i granice pierwszego celu.
4. [Projekt gry](docs/game-design.md) — doświadczenie, które ma zapewnić pierwszy fragment.
5. [Architektura](docs/architecture.md) i [kontrakty subsystemów](docs/subsystem-contracts.md).
6. [Strategia testów](docs/testing-strategy.md) i [jeden plan etapów](docs/prototype-plan.md).

## Pozostała dokumentacja

| Dokument | Odpowiada za |
| --- | --- |
| [Środowisko pracy](docs/development.md) | Rzeczywiście dostępne komendy i stan narzędzi |
| [Budżety i pomiary](docs/performance-budget.md) | Sprzęt, proponowane limity i metodologia porównań |
| [Decyzje](docs/decisions.md) | Wybory zatwierdzone, propozycje i ich uzasadnienia |
| [Przepływ pracy](docs/workflow.md) | Zadania, współpraca z agentami i warunki ukończenia |
| [Ryzyka i pytania](docs/risks-and-open-questions.md) | Problemy wymagające decyzji lub eksperymentu |
| [Program badań](docs/research-agenda.md) | Źródła, hipotezy i ograniczenia dowodów |
| [Szablon zadania](docs/templates/change-brief.md) | Cel, zakres i kryteria zmiany |
| [Szablon weryfikacji](docs/templates/verification-report.md) | Dowód wykonanych sprawdzeń i ograniczeń |
| [Szablon eksperymentu](docs/templates/experiment-record.md) | Pytanie, kontrola, wyniki i interpretacja |
| [Archiwum](docs/archive/README.md) | Sześć oryginalnych dokumentów zachowanych bez zmiany treści |
| [Wynik kontroli dokumentacji](docs/verification/documentation-baseline.md) | Wykonane sprawdzenia i ich ograniczenia |

## Działające kontrole dokumentacji

Wymaganie narzędzi dokumentacyjnych: Python 3.11 lub nowszy, wyłącznie biblioteka standardowa. Z katalogu projektu:

```powershell
python tools/check_docs.py
python -m unittest discover -s tools/tests -p "test_*.py"
```

Pierwsze polecenie sprawdza dokumenty, lokalne odnośniki, identyfikatory kontraktów i integralność archiwum. Drugie sprawdza narzędzia kontroli dokumentów i procesów. Żadne nie jest testem silnika ani potwierdzeniem FPS. Nie sprawdzają dostępności zewnętrznych stron WWW.

Mapa odpowiedzialności dokumentów znajduje się w [rejestrze](docs/documentation-map.json). Nowa implementacja musi aktualizować rzeczywisty stan projektu oraz instrukcję uruchomienia.

[Domknięcie diagnostyki i bezpieczne potrzeby](docs/verification/diagnostics-needs-hardening.md) rozwija obserwację per ID, odłożenie zadania oraz zgodną kontynuację zapisu. Nie wdraża jeszcze transferu materiałów ani dostawy.

[Naprawa odzyskiwania dojścia](docs/verification/retained-world-navigation-stall.md) usuwa wykonywanie starej trasy przy blokadzie i poprawia ponowienia punktów w zasięgu. Późniejszy przyrost lokalnej współpracy poniżej rozwiązuje zachowany konflikt przy Step; ogólne konflikty tłumu pozostają poza tym zakresem. M04 pozostaje nieukończone.

[Lokalna prośba o ustąpienie](docs/verification/local-clearance.md) pozwala stojącemu NPC samodzielnie udostępnić miejsce przy rozpoczętym Step, zachowując własny zamiar. F3 używa tego samego API. Osoba sterowana ręcznie odmawia. To ograniczona współpraca w M04, bez pełnego modelu społecznego; dostawa jest osobnym późniejszym przyrostem.


[Świadek i prawo](docs/verification/witness-property-law.md) rozszerzają lokalną pamięć o naruszenie własności i uregulowanie. `build/msvc/Debug/reality_traversal.exe --law` uruchamia osobną scenę: osoba 1 pobiera/oddaje F4, osoba 2 jest świadkiem, osoba 3 opiekunem, osoba 4 uprawnionym mieszkańcem/kurierem. F7 przekazuje przy fizycznym kontakcie, R próbuje legalnej konserwacji. Brak autoryzacji nie blokuje fizycznej kradzieży; reakcja opiekuna zależy od otrzymanej wiedzy. Inspektor rozdziela prawdę, pamięć i obsługę. Format 7 czyta 1–6. M04 pozostaje nieukończone.
