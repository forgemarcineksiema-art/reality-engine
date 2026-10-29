# Jeden przewóz w dwóch reprezentacjach

**Status: kurs zaimplementowany; pełna weryfikacja techniczna niezaliczona, 29.10.2026.** Użytkownik zatwierdził rozpoczęcie SC-09 po sytuacji dwóch metod. To przyrost [M05](../prototype-plan.md), bez zamknięcia całego etapu. Punkt wyjścia: `2e096b0`. [Raport](../verification/m05-distant-delivery.md) zachowuje wyniki i otwarte przekroczenia czasu.

## Cel i kontrakt

Jeden rzeczywisty kurier i jego fizyczna partia opuszczają aktywny obszar. W dalekiej reprezentacji upływa czas i postępuje podróż; lokalny kontroler nie wykonuje ruchu tej samej osoby. Powrót zachowuje ID, ładunek, wiedzę i zobowiązania, a dostawę wykonuje zwykły fizyczny transfer. Kamera poza ekranem nie jest migracją.

Chronimy INV-01, INV-02, INV-05, INV-06, INV-08, INV-11, INV-12 i INV-13. WorkWorld nadal posiada zasoby i komendy. TraversalSession posiada rozłączną reprezentację osoby oraz jej podróż. Daleki model nie tworzy drugiego wykonawcy lub drugiej partii, nie odczytuje zapasu do planowania i nie dopisuje obserwacji lokalnych.

## Pierwszy model i granice

- Pierwszy kurs korzysta z jawnie zdefiniowanego połączenia między nieruchomymi terminalami. Dane sceny określają jego odległość i czas. To model podróży, bez twierdzenia o identycznej fizyce z lokalnym kontrolerem.
- Przekazanie jest jawne i transakcyjne na bezpiecznej granicy: nieruchoma podpora, brak Step, promu, czynności, clearance i obsługi aparatu w toku. Stan reprezentacji ma jednego właściciela. Komenda ręcznego sterowania nie wywołuje teleportacji.
- Gdy osoba jest daleka, nie jest lokalną kapsułą blokującą świat ani lokalnym świadkiem. Nie wykonuje lokalnych interakcji, rozmów lub transferów. Pochodna prezentacja nie może udawać lokalnego ciała pozostawionego na granicy.
- Zegar pozostaje wspólny 50 ms. Potrzeby narastają; daleka podróż nie odnawia energii ani nie daje jedzenia. Podróż musi mieć jawną semantykę dotarcia do bezpiecznego terminalu przed wznowieniem lokalnego wyboru potrzeby, bez nieograniczonej ochrony nowego planowania.
- Powrót sprawdza rzeczywistą geometrię i kapsuły. Zajęty terminal oznacza oczekiwanie bez nałożenia, teleportacji do przypadkowego miejsca lub ponownego wykonania czasu podróży.
- Zmiana dostępności połączenia ma zdefiniowaną reakcję fizycznego modelu. Nie ujawnia NPC odległego zdarzenia bez źródła. Pierwszy zakres może odmawiać podróży po nieobsługiwanym połączeniu; odmowa jest jawna.
- Zapis obejmuje reprezentację, postęp, terminy i powrót w toku. Opcjonalna nowa wersja zachowuje odczyt 1–8 i dotychczasowe bajty światów bez nowego stanu. Uszkodzony odczyt nie zatwierdza części świata.

## Plan wykonania

Używamy TDD oraz `superpowers:subagent-driven-development`: jeden spójny pion integracyjny, potem przegląd i weryfikacja integracji. Dokument jest briefem i planem wykonawczym, nie drugim planem etapów. Implementer rozstrzyga szczegółowe nazwy API po odczytaniu konsumentów, zapisując decyzje i ich koszt w dzienniku poniżej.

### Zadanie 1 — kompletny kurs lokalny/daleki

**Pliki:** `src/lab/traversal_session.hpp/.cpp`, wydzielony moduł podróży dalekiej w `src/lab/`, kodek, dostawy, wiedza, clearance i przejścia według rzeczywistych konsumentów; renderer i `src/platform/traversal_main.cpp`; runner C++ i integracja nowych procesów; CMake, launcher i dokumenty właścicieli reguł.

- [x] Przeczytać całe wejście repozytorium, kontrakty, API oraz aktualne raporty. Sprawdzić podstawę w wybranych istniejących testach przed zmianą. Nie kompilować równolegle z testami.
- [x] Nazwać konkretne API i stan, zapisać definicję terminali, horyzont i obsługę potrzeb/blokady; test RED wykazuje brak rzeczywistego przekazania odpowiedzialności.
- [x] Testy prawdziwej dostawy, nie samego modelu czasu: ten sam ID/ładunek, brak podwójnego ruchu, lokalnych kolizji i obserwacji dalekiej osoby, potrzeby/czas, wiele przełączeń, chronione fazy, zajęty powrót i zwolnienie terminalu. Bilans po każdym kroku.
- [x] Wdrożyć najmniejszą reprezentację i konsumenta. Żadnego pustego frameworka streamingu, gotowej fizyki ani drugiej gospodarki.
- [x] Zapis i odtworzenie w nowych procesach w trakcie podróży oraz oczekiwania na powrót. Korupcja z poprawnym checksumem, stare formaty, brak ponownego zużycia i transakcyjna odmowa.
- [x] Udostępnić jawną próbę w tej samej aplikacji, pokazując reprezentację/postęp i rzeczywistą dostawę. Natywny obraz, wejście, F5/F9 i obcy poprawny zapis. Nie nazywać headless dowodem okna.
- [ ] Pełne Debug/Release/ASan, format, narzędzia i dokumentacja. Zachować RED, błędy i nieudane próby w raporcie; nie zwiększać historycznych limitów lub osłabiać asercji. Dzielić procesy pomiarowe tylko z zachowaniem wszystkich kroków i asercji.

### Zadanie 2 — kontrola integracji i odbiór techniczny

- [x] Przejrzeć implementację względem własności, wszystkich lokalnych konsumentów osoby i zapisów. Wynik innego agenta nie wystarcza bez odczytania zmian i rzeczywistej kontroli integracji.
- [x] Utrwalić dowody z hashami i ograniczeniami; zaktualizować jeden plan bez zaliczania M05. Osobno pozostawić odbiór użytkownika nowego kursu.

## Dziennik

29.10.2026: odczyt kodu wykazał, że `tick()` porusza wszystkie osoby w pięciu podkrokach, a walidacja wymaga każdej kapsuły lokalnie. Kolizje platform, kolejki, obserwacje i renderer również konsumują wszystkie osoby. Dlatego flaga rysowania lub samo zatrzymanie kontrolera nie wystarczają. Konieczny jest audyt wszystkich konsumentów reprezentacji oraz spójna walidacja obu wariantów.


29.10.2026: decyzja API: `DistantConnection` ma dwa statyczne terminale, długość >= odległości euklidesowej i czas >= ceil(długość / 0,15 m), ograniczony do 1200 kroków. `TraversalActor::journey` przekazuje jedyną reprezentację; `local()` bramkuje wszystkich lokalnych konsumentów. `depart()` odmawia ruchowi/czynności/Step/promowi/clearance/aparatowi. Zamknięcie blokuje kolejne wejście, rozpoczęta podróż dociera do terminalu. Zajęte wyjście czeka z nasyconym postępem. Potrzeby upływają jeden raz; po dojściu bezpieczna granica od razu przywraca zwykłe planowanie potrzeb. Automatyczna dostawa zna autorską topologię, nie odległy zapas ani dostępność. Format 9 występuje tylko z nowym stanem, starsze ścieżki pozostają bajtowo bez zmian.

Bazowe `ctest --preset windows-debug -R 'two_methods.choice|delivery.transfer|practice.contact'`: 2/2 PASS. RED kompilacji braku API zachowany w `build/distant-red.log`; następnie przygotowano scenę i stan przed implementacją przekazania do próby RED zachowania.


29.10.2026: zestaw rozszerzony 12/12 PASS (Debug), w tym prawdziwy świadek kradzieży, trzy fazy promu, aparat/praca, ręczna podróż i oba świeże procesy. Odczyt koordynatora wskazał dodatkowo dalekie wejście z odłożonym Eat/Rest/Maintain oraz wejście z kolejki ruchu. Nowy RED `distant-review-red.log` wykazał oba przyjęcia (Departed=0). Wybrano odmowę Busy bez kasowania zamiaru/wejścia. Rozpoczęty pełny build Debug przerwano jawnie przed CTest, aby wykonać regresję i naprawę przed końcowym zamrożeniem źródeł; nie raportujemy go jako ukończonego.


29.10.2026: 13/13 nowych prób Debug PASS po poprawkach odmów. Po zamrożeniu źródeł rozpoczęto pełny build i CTest. Niezależny przegląd zgłosił jedną Important: anulowana daleka dostawa z wiedzą wyłącznie o źródle nie używa dalekiego planowania dla zwrotu nieprzypisanej partii. Źródła pozostają zamrożone do końca bieżącego CTest; potem obowiązuje RED z prawdziwą anulowaną podróżą i ograniczoną wiedzą, naprawa wspólnego planowania zwrotu, nowe pełne profile. Nie rozszerzamy semantyki wyboru stanowiska i nie wymuszamy źródła, jeśli znane jest zgodne stanowisko lokalne.


29.10.2026: zgodnie z decyzją koordynatora przerwano niekońcowy CTest po 44 zaliczonych przypadkach, podczas `practice.fresh_process`; log ma jawny marker INTERRUPTED/partial. Nie ukończono ani nie zaliczono pełnego zestawu. Procesy testów zatrzymano przed kolejną kompilacją (pozostał wyłącznie bezczynny serwer MSBuild nodeReuse). Nowy RED `distant-cancel-red.log` odtworzył brak fizycznego zwrotu anulowanej partii przy known_stations=1. Naprawa zachowuje wybrane przez dotychczasową regułę stanowisko i używa tej samej dalekiej trasy także dla zwrotu nieprzypisanego ładunku. Dodano nowy proces kontynuacji anulowanej podróży i asercję rzeczywistych ID obiektów renderera bez dalekiej kapsuły.
