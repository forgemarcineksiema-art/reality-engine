# Strategia testów i odbioru

**Status:** normatywne wymagania oraz katalog rozwoju. M00 obejmuje zegar, CLI, narzędzia i początkowy zakres SC-15. M01 pokrywa SC-01 i SC-02 dla modelu wykonawca/partia/czynność oraz ograniczone części SC-03, SC-04 i SC-08 opisane niżej. M02 dodaje aplikację, a M03 geometrię, ruch, trasy i kontynuację podróży w opisanym zakresie. Aktualny odbiór ograniczonego M04 opisuje [audyt](verification/m04-acceptance.md); zakres M05 i jego akceptację opisuje [raport zbiorczy](verification/m05-acceptance.md). Pozostałe niezrealizowane części katalogu pozostają planem. Rzeczywiste komendy wskazuje [środowisko pracy](development.md).

## Obecny zakres domenowy

Poniższa tabela zachowuje zakres fundamentu M01; późniejsze podróże, wiedza i integracja mają własne dowody niżej oraz w aktualnym audycie M04.

[Testy C++](../tests/work_world_tests.cpp) i [integracja procesu](../tests/work_demo_integration.py) sprawdzają [model czynności](work-domain.md). [Raport M01](verification/m01-identity-actions-reservations.md) zawiera wykonane konfiguracje i dowody.

| Scenariusz | Wdrożony zakres | Nadal niewykonane |
| --- | --- | --- |
| SC-01 | Recykling slotów, nieaktualny uchwyt, granice generacji/ID, zachowanie ID i liczników po zapisie | Streaming i zmiana reprezentacji |
| SC-02 | Konkurencja wykonawców, pełna partia, utrwalone sukcesy i odmowy, konflikt payloadu, skutek najwyżej raz | Pieniądze i płatności |
| SC-03 | Przed rozpoczęciem i po zużyciu, usunięcie wykonawcy i partii, powtarzane przerwanie | Narzędzia i biologiczna śmierć; przed rezerwacją brak czynności do anulowania |
| SC-04 | Zużycie, strata, niedobór, transfer typowanego materiału, kontrola bilansów, seria 100 operacji z seedem `0x4d3031` | Recepty i produkcja |
| SC-08 | Rezerwacja/praca, uczenie, podróż/ładunek i oczekiwanie; nowe procesy, korupcja, wersje, dziennik i rzeczywista odmowa podmiany pełnego pliku; [audyt](verification/sc08-continuity-audit.md) | Awaria sprzętu, utrata zasilania i nagłe przerwanie procesu w trakcie zapisu |

Są też granice czasu, limitów kolekcji i dziennika, kolejność równoczesnych terminów oraz porównanie skoku czasu z pojedynczymi krokami. Nie zaliczają pełnych przyszłych subsystemów. Nie wykonano testów współbieżności ani pomiaru wydajności symulacji.

## Obecny zakres okna i prezentacji

M02 dodaje [testy laboratorium](../tests/lab_tests.cpp) i [test sprzętowy](../tests/lab_gpu_integration.py). Pokrywają część SC-11: rzeczywiste urządzenie, obraz, zmianę rozmiaru, minimalizację/powrót, komendy i zakończenie. Zapis i odczyt kontrolują również bilans M01. Własna kamera i wybór są sprawdzane niezależnie od rendererowego odczytu pikseli. Ruch oraz kolizje dodaje M03 opisany poniżej.

Odczyt pikseli nie potwierdza rzeczywistego wejścia myszy: osobna próba Computer Use sprawdza kontrolki, scenę i skróty. Nie jest to ludzka ocena wygody gry. Nie ma benchmarku wydajności. Test `lab.hardware_gpu_integration` wymaga debug layer; po zainstalowaniu składnika Windows został zaliczony na MX330 w każdej konfiguracji. Zwykły `lab.hardware_gpu_standard` pozostaje odrębną próbą. Pełny Debug 45/45, RelWithDebInfo 45/45 i ASan 47/47 zaliczono bez wykluczania testów. Dowody zawiera [raport M02](verification/m02-window-laboratory.md).

## Obecny zakres ruchu

[Testy ruchu](../tests/motion_tests.cpp) obejmują SC-06 i SC-07 dla [ograniczonej geometrii 2,5D](motion-navigation.md): sweep, spoczynek, styk, nałożenie, ślizg, narożnik, sekwencyjne kontakty, granice, rozmiar, prześwit, cienką ścianę, brak drogi, budżet i odrzucenie starego wyniku. Referencją A* jest niezależny BFS na maskach 8×8 dla seedów 1–24, z kosztem i osiągalnością wynikającymi z maski. Nie dowodzi to pełnej kompletności w przestrzeni ciągłej.

Konsument sprawdza dojście objazdem, zmianę geometrii podczas ruchu, odmowę zamknięcia na postaci, lokalną blokadę przez drugiego wykonawcę, wznowienie po odblokowaniu, krótki impuls klawiatury, przekątną i wspólny czas. SC-08 rozszerzono o podróż z pracą M01, starą trasę, `NoPath` i `Waiting`, niepoprawną wersję oraz semantycznie sprzeczny plik z poprawną sumą kontrolną. Test akumulatora wstrzykuje błąd między krokami i sprawdza zachowanie nierozliczonego czasu.

[Integracja sprzętowa M03](../tests/motion_gpu_integration.py) dodaje do SC-11 natywne krótkie/przytrzymane wejście, utratę aktywacji, ruch, wybór, resize i minimalizację. Dwa rzeczywiste procesy D3D11 kontynuują ten sam zapis i porównują wynik ruchu oraz pracy. Próbę rzeczywistego wejścia myszy i klawiatury przez Computer Use oraz wszystkie konfiguracje opisuje [raport M03](verification/m03-motion-navigation.md). [Ocena użytkownika](verification/m03-motion-navigation.md#ocena-użytkownika) akceptuje sterowanie, komfort i czytelność obecnego prototypu z uwagami do prowadzenia do celu oraz naturalności. Szersza próba graczy, SC-12 i autonomia SC-14 nadal pozostają niewykonane.

Przyrost [prowadzenia do celu](verification/navigation-polish.md) zachowuje test BFS surowego A*, a oddzielnie sprawdza skróty: bezpośredni odcinek, znaną ścianę, szerokość, prześwit, budżet skracania i poprawny ogon trasy. Reprodukcje resetu identycznego celu i cofania do centrum po zmianie geometrii przed naprawą dawały porażkę. Testy dojścia i blokady chronią brak dryfu oraz kontynuację po odczycie. Sprzętowy test uruchamia także trzeci proces z rzeczywistym, chronionym SHA-256 zapisem podróży ze starego programu; nie generuje tego pliku nową implementacją.

[Audyt praktyczny](verification/motion-adversarial-audit.md) rozszerza regresje o [adwersarialny runner](../tests/motion_adversarial_tests.cpp): kontakt i cel przy ścianie, obejście osoby, mijanie i zamianę celów, 200 zmian celu z odczytami, 1000 sweepów z niezależną kontrolą prostokątów i próbkowanie obrazu na zakręcie. Próbki obrazu nie mogą mutować świata ani przenikać przez narożnik lub drugą osobę. Test dawnej odmowy końca trasy obejmuje także syntetyczny poprawny rekord wersji 1; odróżniamy go od rzeczywistego historycznego pliku w próbie sprzętowej. Żaden z tych testów nie jest dowodem pokrycia wszystkich możliwych scenariuszy.

## Rodzaje dowodów

[Audyt toru przestrzennego wielu postaci](verification/traversal-multi-actor.md) dodaje 16 przypadków: FIFO i przeciwny ruch na kładce, wymianę pasażerów z odczytem faz, bliskie i czteroosobowe mijanie, hamowanie ręczne, stoki i poziomy, obracającą się podporę, kontakt na ruchomym pokładzie, semantykę komend i uszkodzone zapisy. Rzeczywiste pozycje i próbki prezentacji mają odrębne asercje. Pełna scena działa przez 400 kroków z kontynuacją; dodatkowe obserwacje 1000 kroków nie są doliczane do automatycznych testów ani używane jako benchmark.

Rozszerzenie [pojemności](verification/reusable-capacity.md) wymaga dwóch kolejnych odpoczynków w jednym miejscu, blokady równoczesnego zajęcia, zerowego zużycia i zwolnienia po ukończeniu/anulowaniu lub usunięciu uczestnika. Testy domeny obejmują też ponowienie komendy, odczyt aktywnej czynności, mieszane materiały/pojemności, uszkodzony rekord z poprawną sumą oraz zachowanie wersji 1.

Regresje ograniczonej [autonomii potrzeb](traversal-autonomy.md) sprawdzają wybór odpoczynku po lokalnej odmowie jedzenia oraz po braku odpowiedniej trasy do jedzenia, odczyt terminów ponowień i kontynuację czynności odpoczynku. Dwie postacie konkurują też o jedną porcję: wymagane są dokładnie jedno zużycie i jeden ukończony posiłek. [Raport](verification/autonomy-need-fallback.md) nie przypisuje tym próbom odbioru społeczności M04 ani wielorazowej pojemności stanowisk.

W rozwijanym torze przestrzennym `TraversalSession` regresje obejmują nadpisanie odroczonego celu, podejścia i zatrzymania podczas jawnego kroku oraz przejazdu platformą, a także kontynuację po zapisie. Kolejka przejścia jest sprawdzana przy ręcznym wyjściu właściciela, również po zmianie podparcia wewnątrz nakładających się płyt i po odczycie zajętego przejścia. Ręczne przekroczenie współpłaszczyznowego styku ma osobną reprodukcję; zapytanie o podparcie odrzuca lukę, inną wysokość i niewchodliwą powierzchnię. [Raport napraw](verification/traversal-command-hardening.md) rozdziela ten odbiór od nadal rozwijanego pełnego zakresu geometrii i autonomii.

- Test modułowy sprawdza regułę i przypadek brzegowy.
- Test integracyjny sprawdza rzeczywiste przekazanie stanu między właścicielami.
- Test własności sprawdza niezmiennik na serii kontrolowanych wejść.
- Scenariusz domenowy uruchamia wspólny rdzeń gry i runnera.
- Próba aplikacji obejmuje prawdziwe wejście, kolizję i renderer tam, gdzie są częścią twierdzenia.
- Próba z człowiekiem sprawdza czytelność, sterowanie i wartość doświadczenia.

Żaden rodzaj nie zastępuje automatycznie pozostałych. Wstrzyknięcie rezultatu „most zniszczony” może badać logistykę, ale nie fizyczny rozpad. Atrapa transportu nie dowodzi dostawy. Test ma obserwować stan i skutek, a nie jedynie log sukcesu.

W realizowanym [przyroście dostawy](work-items/material-delivery.md) testy `transfer.*` badają m.in. bilans, rezerwacje, powtórzenia, przepełnienie, alokacje i błędne zapisy; `delivery.*` obserwują fizyczny ładunek, potrzeby, konsekwencję posiłku, blokadę, prom, anulowanie oraz dostęp przez ścianę i odczyt dawnych tras. Wyniki prób nie stanowią automatycznie odbioru całego SC-04, SC-10 ani M04.

## Macierz zmian i obowiązkowych sprawdzeń

| Zmiana | Wymagane minimum przed ukończeniem |
| --- | --- |
| Dokumentacja | Walidator dokumentów i kontrola merytoryczna; testy walidatora, jeśli go zmieniono |
| Reguła domenowa | Test wyniku i dotkniętych niezmienników; scenariusz konsumenta |
| Naprawa regresji | Zachowana reprodukcja oraz test wykazujący naprawę |
| Kontrakt między subsystemami | Test obu stron i rzeczywistego przepływu, w tym odmowy lub przerwania |
| Format danych / zapis | Wczytanie, działania w toku, uszkodzony zapis i jawna obsługa wersji |
| Własność / pamięć / czas życia C++ | Konfiguracja diagnostyczna i właściwe kontrole pamięci; przy współbieżności także sprawdzenia wyścigów |
| Ruch / fizyka / trasy | Przypadki geometryczne i zmiana przestrzeni podczas działania; próba aplikacji, jeśli zmienia się sterowanie |
| Renderer / platforma | Faktyczne uruchomienie, resize, wybór adaptera i diagnostyka błędów; GPU tylko wtedy, gdy było użyte |
| Optymalizacja | Zachowanie wyniku oraz porównanie według metody pomiarów |
| Refaktoryzacja | Dotychczasowe testy zachowania i odpowiednia integracja; wynik nie może zależeć od zmienionej reprezentacji |

Nie wymagamy arbitralnego procentu pokrycia. Wymagamy dowodu ważnych zachowań, granic i awarii. Test nie może być kopią algorytmu implementacji.

## Katalog scenariuszy

### SC-01 — Tożsamość

Utwórz osobę, zachowaj odwołanie, usuń ją i ponownie użyj miejsca w pamięci. Stare odwołanie musi zostać odrzucone. Zapis i wczytanie zachowują ID pozostałych osób. Chroni INV-01 i INV-02.

### SC-02 — Konkurencja i ponowienie

Dwie ekipy chcą ostatniej części. Tylko jedna uzyskuje skuteczną rezerwację. Sprawdź również rezerwację całej partii: wolna ilość wynosi zero, a istniejąca rezerwacja pozostaje poprawna. Powtórz tę samą komendę przed i po zapisie. Nie powstaje druga zapłata ani drugie zużycie; druga ekipa otrzymuje określony wynik. Chroni INV-03, INV-04, INV-05 i INV-06.

### SC-03 — Przerwanie działania

Przerwij pracę przed rezerwacją, po rezerwacji i po częściowym zużyciu. Powtórz dla śmierci wykonawcy lub zniszczenia narzędzia. Porównaj ilość, własność, rezerwacje i fazę z niezależnie określonym wynikiem. Zużyty materiał nie wraca bez reguły odzysku. Chroni INV-04 i INV-05.

### SC-04 — Bilanse i produkcja

Wykonaj transfer, receptę, stratę i odmowę z powodu niedoboru. Sprawdź wejścia, wyjścia, przyczynę straty, rezerwacje i właścicieli. Dla serii komend sprawdzaj bilans po każdej granicy zatwierdzania. Nie porównuj wyłącznie sumy końcowej, która mogłaby ukryć chwilowe podwójne wydanie. Chroni INV-06.

Ograniczone [prawo i świadkowie](verification/witness-property-law.md) dodają scenariusze `law.*`: cztery warianty SC-05, geometrię, legalnego kuriera/mieszkańca, rzeczywistą obsługę, ochronę rozpoczętej pracy, wieloetapowy przekaz, ograniczony automat, FIFO restytucji, limity, zgodność 1–6 i korupcję formatu 7. Próba aplikacji używa F4/F7, inspektora i dwóch procesów D3D11. Nie jest to odbiór pełnego M04.

### SC-05 — Świadek, informacja i prawo

Ograniczony przyrost [lokalnej wiedzy](verification/local-station-knowledge.md) pokrywa obserwację, przekaz, nieaktualną informację i fizyczną korektę w `knowledge.*`; nie implementuje oceny legalności ani instytucji. Testy sprawdzają drogę do opróżnionego zdalnie stanowiska bez przekazu, zmianę planu po świadku, dostawę i rzeczywisty posiłek przed dawnym retry, źródło/pośrednika/czasy, ścianę/zasięg/podporę, ponowny zwiad, ograniczony automat, bezpieczne Step/trzy fazy transportu/czynność/clearance, kontynuację oraz transakcyjną odmowę korupcji z poprawną sumą kontrolną. Część SC-14 obejmuje kurs i przekaz bez gracza; pełna społeczność pozostaje poza odbiorem tego przyrostu. Próby ochrony Step/transportu celowo inicjują historyczny raport w pamięci świadka, aby izolować bezpieczną granicę; nabycie prawdziwej obserwacji i fizyczny przekaz mają odrębne testy integracyjne. Natywne smoke wymaga `knowledge_shortcut`, `knowledge_exchange` i `knowledge_inspector`, obejmując klawisz, skutek i kontrolkę Win32.

Przejmij ładunek w wariancie ze świadkiem i bez. Następnie przekaż i nie przekaż wiadomości. Ocena legalności i reakcja instytucji nie mogą korzystać z nieznanej informacji. Kontrola fizycznej możliwości pozostaje odrębna. Chroni INV-07 i INV-08.

### SC-06 — Kolizja w obsługiwanej geometrii

Sprawdź spoczynek, początkowe nałożenie, narożnik, wiele kontaktów, wąską szczelinę i szybki ruch wobec cienkiej przeszkody. Oczekiwane ograniczenia ruchu wynikają z prostych znanych figur. Tolerancja ma jednostkę i uzasadnienie. Nie zaliczaj translacyjnego kontrolera jako solvera obracających się brył. Chroni INV-09 i INV-16.

### SC-07 — Trasa i nieaktualny wynik

Sprawdź brak drogi, drogę zastępczą, zbyt wąskie przejście i zamknięcie mostu w czasie podróży lub wyszukiwania. Trasa respektuje rozmiar wykonawcy; stary wynik jest ponownie sprawdzony. Na małych mapach wynik porównaj z niezależną referencją, np. BFS dla jednakowych kosztów. Ruch po wyniku nadal ma kolizję. Chroni INV-09 i INV-10.

### SC-08 — Kontynuacja i awaria zapisu

Zapisz podczas pracy, podróży, oczekiwania na trasę i obowiązującej rezerwacji. Wczytaj oraz porównaj chronione dane z opisanym stanem. Wstrzyknij błąd zapisu i uszkodzony albo nieobsługiwany format. Ostatni poprawny świat pozostaje dostępny; program nie zgłasza sukcesu i nie resetuje stanu potajemnie. Chroni INV-05 i INV-12.

[Audyt SC-08](verification/sc08-continuity-audit.md) przypisuje istniejące asercje do faz i zachowuje 16 nowych kontynuacji, pięć oczekiwanych odmów oraz trzy profile po 123 testy. Nie utożsamia odmowy podmiany pliku z awarią nośnika.

### SC-09 — Przejście szczegółowości

Konwój opuszcza aktywny obszar, podróżuje, a gracz wraca. Sprawdź ID, ładunek, rezerwację pojazdu, postęp i czas zgodny z modelem. Wielokrotne przełączenie nie dubluje zasobów. Nie wymagaj bitowej równości modeli o innej fizyce; porównaj ich zadeklarowane własności osobno. Chroni INV-02 i INV-13.

### SC-10 — Łańcuch konsekwencji

[Sytuacja dwóch metod](verification/m05-two-methods.md) łączy istniejące uczenie i konserwację z transportem. `two_methods.*` sprawdza nierozstrzygnięty początkowy wybór, rzeczywisty most/prom, osiem jednostek dostawy i posiłek, bilans po każdym kroku oraz kontynuację obu metod w nowych procesach. Horyzont nowej sceny wynosi 1200 kroków świata; historyczne próby M04 zachowują 240/600 kroków i dotychczasowe limity. To zakres SC-10/SC-08, bez dowodu migracji reprezentacji lub pełnej historii.

[Przyrost przejścia i logistyki](verification/passage-logistics.md) ma runner `reality_passage_tests` i natywny test `passage.native_diagnostics`. Sprawdza niewiedzę o odległym zamknięciu, fizyczne odkrycie, świadka, objazd z partią i rzeczywisty posiłek, koszt czasowy obu dróg, materiał typu 2 i ukończoną naprawę, krótszą trasę po wiadomości, progi zużycia i bezpieczne odmowy, kolejkę, WASD, Step, trzy fazy promu, czynność, clearance, spokojny/zaburzony przebieg i transakcyjny zapis. Zachowuje czerwone regresje pierścienia obserwacji, długiego objazdu i przyczółka bez etykiety FIFO. Korupcja z poprawnym checksumem obejmuje maskę, ID, czasy, duplikaty, link/typ materiału i zamkniętą zajętość/kolejkę. Ochronę operacji izoluje także jawne seedowanie historycznej pamięci; nie zastępuje ono osobnego testu rzeczywistej obserwacji. To określony zakres SC-10 i części SC-14, nie pełny odbiór M04.

Zmień dostępność mostu przez właściwą komendę. Uruchom rzeczywiste planowanie, transport, rezerwę zakładu i pracę. Sprawdź wersję trasy, miejsce ładunku, czas dostawy, zużycie zapasu i źródło wiedzy. Zapisz i kontynuuj. Żaden etap nie może być dopisanym oczekiwanym wynikiem. Chroni INV-06, INV-08, INV-10, INV-12, INV-14 i INV-16.

### SC-11 — Aplikacja, wejście i obraz

Uruchom rzeczywisty program. Sprawdź ruch, interakcję, resize, minimalizację/powrót, wybór adaptera i zakończenie. Jeśli twierdzenie dotyczy sprzętu MX330, zapisz faktycznie wybrany adapter. Obraz programowego renderera lub innej karty nie jest takim dowodem. Człowiek osobno ocenia sterowanie i czytelność.

### SC-12 — Dłuższe obciążenie

Uruchom ustalony scenariusz i konfigurację przez okres określony przed próbą. Zapisz rozkład czasów, przycięcia, pamięć, zaległości i opóźnienie aktualizacji. Porównanie wymaga podobnego stanu nagrzania i braku konkurencyjnego obciążenia. Nie ukrywaj złych próbek. Chroni INV-11; metoda jest w [budżetach](performance-budget.md).

### SC-13 — Historia po zmianie świata

Przed uruchomieniem sceny usuń uczestnika, zniszcz miejsce albo przenieś przedmiot. Scena przechodzi do określonego wariantu, odmowy lub zakończenia. Nie wymaga niemożliwej rozmowy i nie przywraca świata do oczekiwanego scenariusza. Chroni INV-15.

### SC-14 — Społeczność bez gracza

[Wspólny odbiór M04](verification/m04-acceptance.md) uruchamia community.*: osiem osób, 600 kroków spokojnych/zaburzonych z bilansem na każdej granicy, prawo w tym samym świecie, zapis podczas pracy/podróży/incydentu, oraz cztery warianty porównania240 kroków z prostą rutyną. Wynik porównania pozostaje jawny, bez wymagania zwycięstwa adaptacji. Dwa procesy natywne sprawdzają controls i zgodność expected.save; nie zastępują [oceny użytkownika](verification/m04-playtest.md).

Uruchom model bez interwencji gracza, potem wprowadź kontrolowane zaburzenie. Sprawdź wykonane działania, przyczyny porażek i zakres stabilności. Porównaj z prostszą rutyną; zachowaj także nieudane przebiegi. Deklaracja planu nie jest jego realizacją. Chroni INV-03, INV-06, INV-08 i INV-16.

Wariant spokojny również jest wymagany: sprawne przejście i zasoby pozwalają na pracę oraz współpracę bez katastrofy i poleceń gracza. Jeśli twierdzenie dotyczy nauczenia się praktyki, trzeba wykazać kanał informacji, późniejszą wykonaną czynność i kontynuację po zapisie; nie zaliczamy tego na podstawie dialogu. Brak nowej instytucji nie jest porażką pierwszego modelu, jeśli jej powstanie nie należy do zadanego zakresu.

### SC-15 — Diagnostyka pamięci i współbieżności

Wybierz narzędzie wspierane przez rzeczywisty kompilator i platformę. Oddzielna kontrolowana fixture z błędem pamięci ma wykazać, że konfiguracja diagnostyczna go wykrywa; fixture nie należy do produkcyjnej ścieżki gry. Właściwe testy kodu mają przejść bez takiego błędu. Po wprowadzeniu współbieżności określ narzędzie lub metodę wykrywania wyścigów, granice pokrycia i test kolejności. Brak wymaganego narzędzia oznacza NIEWERYFIKOWANE, nie zaliczenie. Nie obiecujemy całkowitego wykrycia UB ani wszystkich wyścigów.

**WYNIK M00:** profil AddressSanitizer wykrywa heap-buffer-overflow w oddzielnej próbce; poprawna próbka oraz zwykłe testy przechodzą. Nie wprowadzono współbieżności symulacji i nie badano wyścigów. Dokładny dowód zawiera [raport](verification/m00-build-and-diagnostics.md).

## Reprodukcja i liczby

Każda losowa próba zapisuje seed, dane wejściowe, wersję modelu i sekwencję komend. Przy niedeterministycznej fizyce lub usługach zapisuje się istotne wyniki albo porównuje serie według ustalonej metody. Hash stanu może obejmować znormalizowane dane dyskretne; nie surowe bajty obiektów C++ i nie nieopisane zaokrąglenia.

Przed testem ustal asercje, tolerancje i warunek zakończenia. Zmiana tolerancji po porażce wymaga wyjaśnienia modelu i pozostawienia śladu. Zmniejszaj przypadek regresji, zachowując jego przyczynę.

## Status wyniku i raport

Zmiana fikcji według DEC-008 nie kasuje scenariuszy technicznych. Magia, potwory i rozbudowana walka nie są wymagane w pierwszym prototypie; ich przyszłe testy powstają wraz z właściwymi kontraktami. Projekt Action RPG wymaga osobnych obserwacji człowieka wskazanych w [projekcie gry](game-design.md#ocena-gry), włączonych w M03–M06. Wynik obrazu, bilansów albo autonomii nie dowodzi sam w sobie satysfakcji z przygody.

Dozwolone statusy: ZALICZONE, NIEZALICZONE, NIEWYKONANE, ZABLOKOWANE, NIE DOTYCZY z uzasadnieniem. Obowiązkowego testu nie można oznaczyć NIE DOTYCZY tylko dlatego, że brakuje narzędzia. Skorzystaj z [raportu](templates/verification-report.md).

Nie wolno ukrywać porażki przez skip, zmianę oczekiwań na bieżące wyjście lub zmianę ścieżki wykonania na atrapę. Gdy dowodu nie da się uzyskać, raportuj ograniczenie i pozostaw odpowiednią część zadania nieukończoną.


Przyrost [diagnostyki i bezpiecznych potrzeb](work-items/diagnostics-needs-hardening.md) ma osobny runner `reality_traversal_needs_tests`: krytyczny głód/wyczerpanie, rzeczywiste wznowienie pracy, ochrona Step i trzech faz promu, rozpoczęta czynność z pojedynczym kosztem/skutkiem, ręczne przejęcie, niedostępna potrzeba z ograniczonym ponawianiem, wygaśnięcie ochrony dojścia i transakcyjny zapis. `tests/fixtures/traversal-v1-needs.save` to zachowany wcześniejszy świat wersji 1 z `build/m04-entry-ui.save`, sprawdzany bajtowo i w kontynuacji. Diagnostyka sprawdza niezależność ID, zmiany kolejności/liczby aktorów, brak resetu przez replanning/Waiting, parking, fazy usługi i odczyt rzeczywistej sesji bez mutacji. Natywny test sprawdza podział linii i zachowanie przewijania inspektora.


`clearance.*` sprawdza [ograniczoną współpracę](verification/local-clearance.md): faktyczne zakończenie Step, odmowy, zachowanie sterowania/pracy/zamiaru, współbieżne miejsca, limity, korupcję pary i zgodną kontynuację formatu 3. Rzeczywista migawka nie jest zastępowana samą zmianą etykiety. Wcześniejszy test stabilnego NoPath ma jawnego ręcznie kontrolowanego blokującego, aby nadal badał odmowę niezależnie od nowego dobrowolnego ustępowania.

`practice.*` sprawdza opcjonalną [integrację uczenia](verification/practice-in-prototype.md): prawdziwy impuls przed pracą, jedną zgodę/jedną część, brak energii/części/zasięgu, współbieżność i kolejkę, inne prawo, potrzeby i ręczne przejęcie, blokadę rozmowy przez ścianę, zapis faz i odrzucanie semantycznych uszkodzeń. Cztery fazy mają porównanie pełnych bajtów w świeżym procesie. `practice.native` uruchamia rzeczywiste Win32/D3D11, P/R/F5/F9 i odczyt obcego świata. Nie zastępuje bezpośredniego odbioru użytkownika ani pomiaru długiego działania M06.


`distant.*` sprawdza [ograniczoną podróż daleką](verification/m05-distant-delivery.md): rzeczywistą dostawę i posiłek, bilans po każdym kroku, tożsamość i partię podczas wielu przełączeń, brak lokalnego ruchu/kapsuły/kontaktu/obserwacji, wspólny czas i potrzeby, chronione fazy, zamknięcie po starcie, zajęty powrót oraz fizyczne zwolnienie terminalu. Checkpointy podróży i czekania wracają w nowych procesach z porównaniem pełnych końcowych bajtów. Korupcja z poprawnym checksumem ma transakcyjną odmowę. Native używa tej samej aplikacji, wejścia N/F5/F9, sprzętowego obrazu i poprawnego obcego świata. To ograniczony kurs SC-09/SC-08, bez odbioru pełnego M05 ani pełnego solvera 3D.

`hypothesis.table_equivalence` chroni [optymalizację kosztu uczenia](verification/learning-cost.md): dokładną składnię i kolejność dawnej gramatyki, wszystkie wejścia 0–128 wobec niezmienionego ewaluatora skalarnego oraz odmowy niepoprawnych programów. Wspólnie z `contrast.model_order` zachowuje konkurentów, kanonicznych reprezentantów i logiczne budżety badania. Pomiar CPU nie zastępuje historycznych limitów procesów ani pełnych zestawów testów.

`story.*` sprawdza [historię SC-13](verification/m05-story.md): rzeczywiste 4/4 i posiłek, trzy zaburzenia i ich kombinacje, role po trwałych ID mimo ponownego użycia slotu, lokalny kontakt, sterowanie, termin600 i rozpoczęty Step, historyczne odmowy, korupcję oraz kontynuacje siedmiu wariantów w nowych procesach. Cztery native sprawdzają T, F2, F5/F9, kontrolkę wyniku, GPU i obcy świat. [API](story-scene.md) ogranicza usunięcia do przygotowania świata; nie jest to solver rozpadu ani ogólny system zadań.
