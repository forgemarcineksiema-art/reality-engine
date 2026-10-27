# Repository Guidelines

## Stan i kolejność lektury

Projekt ma zaliczone M00–M04 w opisanym zakresie: ruch kinematyczny 2,5D, wspólną geometrię kolizji/A*, zmienne przejścia oraz zapis podróży w `reality_lab --motion`. [Raport M03](docs/verification/m03-motion-navigation.md#ocena-użytkownika) określa dowody i akceptację obecnego sterowania przez użytkownika, z uwagami do reakcji na cel, płynności trasy, szybkości i naturalności ruchu. Zachowaj te uwagi przy dalszych pracach; nie przedstawiaj odbioru prototypu jako akceptacji docelowej lokomocji. [Model domeny](docs/work-domain.md), [laboratorium](docs/window-laboratory.md) i [model ruchu](docs/motion-navigation.md) określają API. Rozwijany tor przestrzenny ma ograniczoną [autonomię potrzeb](docs/traversal-autonomy.md) i [transfer oraz pierwszą dostawę](docs/verification/material-delivery.md); aktualny [odbiór małej społeczności M04](docs/verification/m04-acceptance.md) łączy te systemy i został zaakceptowany przez użytkownika 27.10.2026. Pełny solver brył nie istnieje. Lokalny Git wersjonuje projekt na gałęzi `main` i publikuje ją w publicznym `origin` (GitHub). Zasady repozytorium, gałęzi tematycznych i zachowania dowodów opisuje [przepływ pracy](docs/workflow.md#repozytorium-git). Nie przedstawiaj planów jako implementacji.

W transferze zachowaj niezmienny typ materiału, bilans chronologiczny, wspólne ID komend, fizyczną partię ładunku i zgodność historycznych formatów. Anulowanie zachowuje materiał; autonomiczna postać odkłada go w znanym, zgodnym typem stanowisku po tych samych progach potrzeb, a ręczne przejęcie lub `stop()` nie teleportują partii. Stan zwrotu mieści się w `GENESIS_TRAVERSAL 4`; wersje 1–3 pozostają czytelne. Test długiej dostawy pod ASan jest dwuetapowy (checkpoint po 300 krokach) i ma zmierzony margines poniżej niezmienionego limitu 20 s; łączna liczba kroków i asercji się nie zmienia. [Raport](docs/verification/material-delivery.md) nadal oddziela wyniki od ograniczeń i nie zamyka M04.

Przed pracą przeczytaj [README.md](README.md), [engineering-rules.md](engineering-rules.md) i [kartę projektu](docs/project-charter.md). Następnie odczytaj dokument odpowiedzialny za temat według [mapy](docs/documentation-map.json). [Kontrakty](docs/subsystem-contracts.md), [testy](docs/testing-strategy.md) i [jeden plan](docs/prototype-plan.md) mają odrębne odpowiedzialności. Archiwum jest historyczne; nie nadpisuje aktualnych zasad.

Obecny przyrost [diagnostyki i bezpiecznych potrzeb](docs/work-items/diagnostics-needs-hardening.md) zachowuje odłożony zamiar Maintain/Rest, bezpieczne Step/transport/czynność i jednorazowy termin końcowego dojścia. Zachowaj historyczny zapis wersji 1 i nowy zewnętrzny format 2, lokalne uczenie dostępności i niezależne alarmy per ID. Nie odczytuj zdalnego zapasu ani nie używaj diagnostyki do sterowania NPC.

[Naprawa odzyskiwania dojścia](docs/verification/retained-world-navigation-stall.md) usuwa wykonywanie starej trasy osoby 7 i ponawia dozwolone alternatywy po NoPath. Zachowaj pięć regresji oraz rzeczywistą migawkę przy 6938. Późniejsza [lokalna współpraca](docs/verification/local-clearance.md) odblokowuje rozpoczęty Step osoby 6. Zachowaj 13 regresji clearance, kontrolę ręcznie sterowanego blokującego i zgodność formatów 1/2/3. Nie przedstawiaj tego ani osobnego review jako zakończenia M04.

[Audyt odbioru M04](docs/verification/m04-acceptance.md) i [brief](docs/work-items/m04-acceptance.md) łączą istniejące systemy w `--community`, ze spokojnym/zaburzonym przebiegiem, rzeczywistą rutyną referencyjną i kontynuacją. Zachowaj wyniki negatywne porównania, brak tezy o wyższości planera, stałe 240/600 kroków i limity. F9 poprawnego innego świata wymaga bezpiecznego panelu. Aktualny status etapu podaje jeden plan; dawne raporty nie są nadpisywane. [Ocena użytkownika](docs/verification/m04-playtest.md) potwierdza zaliczenie M04 27.10.2026; zachowaj jej rzeczywistą treść i ograniczony zakres raportu. Nie przypisuj tej akceptacji automatowi ani nie rozszerzaj jej na przyszłe systemy. Nie dodawaj pełnych instytucji ani całego M06 jako nowych bramek M04.

## Granice projektu

[Laboratorium odkrywania](docs/discovery-learning.md) ma zatwierdzony [protokół discovery-v1](docs/work-items/discovery-learning-experiment.md) i osobny [raport](docs/verification/discovery-learning-experiment.md). Agent otrzymuje pomiary, nie parametry świata ani seed. Zachowaj rozdział odkrywania/oceny, fizyczny kontakt, źródła i czasy oraz pełny stan generatorów w nowej otoczce; stare zapisy podróży pozostają bez zmian. Kryteria i zbiory potwierdzające zapisano przed próbą. Nie zmieniaj ich pod wynik, nie pomijaj kosztu nauczyciela ani nie nazywaj identyfikacji ze znanej rodziny powstaniem świadomości lub odkryciem nieznanej ludzkości fizyki. Wynik negatywny jest wynikiem badania. Nie jest to nowa bramka M04 ani automatyczne ukończenie M05/M06.

[Przejście i logistyka](docs/work-items/passage-logistics.md) mają osobny [raport](docs/verification/passage-logistics.md). `PassageState` posiada dostępność i opcjonalny Maintain; materiał typu 2 i skutek rzeczywistej czynności pozostają w WorkWorld. Globalna wersja nie przekazuje NPC prawdy: plan korzysta z lokalnych `passage_beliefs`, wykonanie sprawdza fizyczną barierę. Zamknięcie odmawia na kapsule, rozpoczętym Step i związanym clearance. Zużycie 1000 z zajętością pozostawia przejście otwarte do bezpiecznej próby. Nie przerywaj chronionych operacji nową wiadomością ani wykonalnego objazdu samym wiekiem raportu. Zachowaj obserwację na geometrycznym przyczółku także przy historycznym Step0, źródło/czasy i F7. Format 6 czyta 1–5 i zachowuje stare bajty bez nowych danych. `--crossing`, F8 i R nie zastępują starego `--delivery` oraz jego dwuetapowego testu. Stan dostępności nie dowodzi fizycznego zniszczenia ani zakończenia M04.

[Lokalna wiedza o stanowiskach](docs/work-items/local-station-knowledge.md) ma własny [raport](docs/verification/local-station-knowledge.md). `beliefs` należy do `TraversalSession`, nie jest widokiem bieżącego zapasu. Zachowaj fizyczną obserwację przez `interaction_accessible`, świadka `source_actor`, ostatniego rozmówcę `via_actor`, rozdział `observed_tick`/`learned_tick` i ścisłe pierwszeństwo nowszych obserwacji. Przekaz/F7 wymaga kontaktu do 2 m na wspólnej podporze, bez przeszkody. Automat ma ograniczone okno po własnym niedoborze/oddaniu materiału; pośrednik nie uruchamia go od otrzymanej wiadomości. Nie odmładzaj pustej obserwacji przez przekaz; po 100 krokach pozostaje zwiad. Zmiana zamiaru nie przerywa Step, przejazdu, czynności ani clearance i nie teleportuje ładunku. Wiedza wymaga formatu 5; odczyt 1–4 nie wymyśla historycznych obserwacji. Nie utożsamiaj przyrostu z prawem, pełną komunikacją społeczną lub odbiorem M04.

Użytkownik polecił rozpocząć [wejście w M04](docs/work-items/m04-entry-design.md) od [diagnostyki i pomiaru](docs/work-items/m04-entry-observability.md). `TraversalDiagnostics` jest obserwatorem, bez prawa mutacji świata i bez zmiany jego formatu. F6 tworzy osobne dowody, a automatyczne migawki są ograniczone. Nie utożsamiaj tego przyrostu z implementacją dostaw, transferu materiału, próśb o ustąpienie lub odbiorem całej społeczności. Raport pomiaru musi odróżniać wydajność aplikacji od postępu potrzeb i pracy mieszkańców.

Zgłoszenie użytkownika o krążeniu przy blokadzie ma [osobny przyrost odzyskiwania ruchu](docs/work-items/navigation-obstruction-recovery.md). Zachowaj regresje zamkniętego korytarza oraz zajętego miejsca w kolejce. Ponowienie braku drogi uwzględnia aktualnie obserwowanych stojących sąsiadów; miejsce oczekiwania sprawdza kapsuły innych ludzi. Ograniczone prośby i odmowy przy rozpoczętym Step są jawnym protokołem sesji według [modelu](docs/traversal-autonomy.md#lokalna-prośba-o-przejście), nie ukrytą funkcją nawigacji. Ogólne negocjacje pozostają dalszą pracą.

Wykonano techniczny zakres [praktycznego audytu wielu postaci](docs/work-items/traversal-multi-actor.md). Dostarczony dokument pozostaje propozycją; [raport](docs/verification/traversal-multi-actor.md) oddziela reprodukcje, naprawy i granice odbioru. Zachowaj 16 nowych przypadków w `tests/traversal_audit_tests.cpp`, wcześniejsze dowody i limity czasu. To dalszy odbiór fundamentu, bez automatycznego otwarcia M04. Ocena komfortu przez użytkownika i kontrolowany pomiar dłuższego działania pozostają odrębnymi pracami.

Wykonano [wielorazową pojemność stanowiska](docs/work-items/reusable-capacity.md). Nowe sesje używają pojemności dla odpoczynku; materiały nadal są zużywane. [Model domeny](docs/work-domain.md) określa niezmienny rodzaj zasobu oraz zapis wersji 2. Wersję 1 zachowujemy jako materiałową, bez reinterpretacji historycznych kosztów; [raport](docs/verification/reusable-capacity.md) dokumentuje odbiór i zgodność. Jedna czynność nadal rezerwuje jeden zasób. To fundament przed pełnym M04.

Wykonano [wybór potrzeby po niedostępności celu](docs/work-items/autonomy-need-fallback.md). Zachowaj lokalne poznawanie niedoboru i istniejące terminy ponowień; nie zastępuj ich odczytem zdalnych zapasów. [Raport](docs/verification/autonomy-need-fallback.md) opisuje stan historyczny tego przyrostu, sprzed dodania pojemności.

Po przerwanym badaniu regresji użytkownik zatwierdził [naprawę odroczonych poleceń i przejść](docs/work-items/traversal-command-hardening.md). Zakres, dowody przed/po i granice odbioru wskazuje [raport](docs/verification/traversal-command-hardening.md). Nie utożsamiaj tej naprawy z zakończeniem całego rozszerzenia geometrii i autonomii; zachowaj wcześniejsze logi GENESIS-HARDENING-001 oraz regresje B/C.

Zatwierdzone [rozszerzenie ruchu i autonomii](docs/work-items/advanced-traversal.md) obejmuje DEC-014. Użytkownik zaakceptował płynność poprzedniego audytu. Podesty, kładki, poziomy i ruchome platformy są geometrią świata, nie zleceniem portowania na inne systemy. Rozszerzenie jest w realizacji; nie przedstawiaj nowych prymitywów fizyki jako ukończonej lokomocji, platform lub autonomii. Zachowaj regresje starego toru i formatów.

Po bazowym odbiorze M03 użytkownik zatwierdził [dopracowanie nawigacji](docs/work-items/navigation-polish.md). `find_route` pozostaje surowym A* z referencją BFS; `find_motion_route` dodaje bezpośredni sweep i skracanie z limitem zapytań. Identyczny cel i wersja mapy zachowują plan. Format zapisu oraz 3 m/s pozostają bez zmiany. Aktualne dowody i nadal osobną ocenę odczucia wskazuje [raport przyrostu](docs/verification/navigation-polish.md). Nie przypisuj nowej wersji wcześniejszej akceptacji człowieka.

Pierwszy rezultat łączy grywalny fragment i laboratorium małej autonomicznej cywilizacji. Silnik, fizyka i nawigacja są własne; zależności minimalne. Grafika ma najniższy priorytet. Użytkownik pracuje z agentami AI na laptopie i5/MX330/8 GB, bez terminu. Stos i parametry oznaczone jako propozycje pozostają hipotezami, nie wynikami.

DEC-008 zatwierdza realistyczne, brutalne fantasy z systemową magią i autonomiczną cywilizacją; szczegóły nadal są otwarte. DEC-011 zachowuje M00–M06 i fundamenty. Nie wdrażaj magii, potworów ani rozbudowanej walki jako warunku pierwszej sceny. Wolne Dorzecze jest historyczną inspiracją mechanik, nie obowiązującą fikcją. Rozwijaj projekt Action RPG obok symulacji.

## Obowiązkowy sposób pracy

Po zgłoszeniu utknięcia i skoków obrazu wdrożono [audyt praktyczny](docs/verification/motion-adversarial-audit.md). Własne A* ma lokalne przyłącza końców 3×3, komórki od 1 mm i BFS dla centrów. `Waiting` może okresowo planować obejście aktualnego obrysu sąsiada; to nie pełne planowanie tłumu. `presentation_positions` jest pochodnym widokiem rzeczywistego przebiegu ostatniego kroku, bez zmiany świata i bez zapisu tej historii. Nie zastępuj go cięciwą przez narożnik. Zegar pozostaje 50 ms, prędkość 3 m/s, format zapisu 1. Odbiór techniczny nie oznacza oceny odczucia przez użytkownika.

Przed zmianą zachowania nazwij cel, zakres plików, kontrakty i dowód odbioru. Zachowuj cudze zmiany. Nie dodawaj pustych frameworków, atrap udających produkcję ani zależności omijających własną fizykę lub nawigację. Reguły gracza i NPC mają wspólną semantykę, a każde pole stanu jednego właściciela.

Zmiana kontraktu obejmuje konsumentów, testy i dokumentację. Ujawnij konflikt z ustaleniami użytkownika; zwykłe naprawy zgodne z kontraktem wykonuj bez zbędnych pytań. Wynik innego agenta wymaga przeglądu i weryfikacji integracji.

## Rzeczywiste kontrole

Build i testy C++ z katalogu głównego:

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-debug
ctest --preset windows-debug
```

Z katalogu głównego, Python 3.11+ bez pakietów zewnętrznych:

```powershell
python tools/check_docs.py
python -m unittest discover -s tools/tests -p "test_*.py"
```

Pojedynczy test, profil ASan i kontrolę stylu opisuje [środowisko pracy](docs/development.md). Polecenia Pythona sprawdzają narzędzia oraz dokumentację; testy C++ obejmują obecny fundament, nie przyszłą grę.

## Dowód ukończenia

Nie usuwaj porażek przez skip, osłabianie asercji ani cichy fallback. Nie wykonano oznacza nieweryfikowane. Raportuj rzeczywiste komendy, wyniki, ograniczenia i nieukończone części. Zrzut ekranu nie dowodzi bilansów, a headless nie dowodzi działania 3D. Szczegóły wymagań są w regułach inżynierskich; nie powielaj ich w nowych plikach.
