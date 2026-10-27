# Domknięcie diagnostyki i bezpieczne przerywanie potrzeb

Status: zakres zrealizowany i zweryfikowany według [raportu](../verification/diagnostics-needs-hardening.md). Otwarta [stagnacja w zachowanym świecie](retained-world-navigation-stall.md) wymaga rozstrzygnięcia przed dostawami. Praca na polecenie użytkownika „Rozpocznij”, po rekomendacji domknięcia znanych zaległości przed transferem i dostawą. Zachowujemy własny silnik, dotychczasowe testy, zapis WorkWorld i kontrolę gracza. Bez commitów, resetów i delegowania pracy.

## Projekt i kontrakt

Inspektor Win32 ma rozdzielać rekordy CRLF na granicy widoku, bez zmiany tekstu świata. Migawka alarmu ma reagować na nowo podejrzaną postać, niezależnie od alarmu innej; globalny limit ośmiu automatycznych prób na uruchomienie i niezależny F6 zostają.

Obserwator otrzyma od sesji tylko odczyt faktycznego celu marszu, miejsca oczekiwania, fazy przejścia/przejazdu i kontekstu kolejki. Osobno mierzymy marsz/dojście do parkowania i postęp usługi. Czekanie na właściciela samo nie jest awarią. Wsiadanie, wysiadanie i rozpoczęty Step mają obserwowany postęp fizyczny, a Riding postęp platformy. Przy blokadzie zadeklarowanej przez fizykę raport podaje przyczynę zamiast automatycznie uznawać brak ruchu za błąd. Detektor jest nadal nieautorytatywnym podejrzeniem, nie kompletnym dowodem zakleszczenia.

Zmiana waypointu albo przejście między Moving i Waiting nie może bez końca resetować obserwacji niezmiennego zamiaru przy braku rzeczywistego przemieszczenia. Dodatkowe okno zamiaru kredytuje co najmniej 0,5 m przemieszczenia względem podpory; nie wymaga ciągłego zbliżania do odległego celu, więc legalny objazd może prowadzić od niego. Zmiana rzeczywistego celu, odczyt świata lub luka obserwacji ponad 40 kroków rozpoczynają nowe okno. To nie pełne rozpoznawanie wielkich pętli ruchu.

Autonomia zachowuje zwykłe progi wyboru: jedzenie od głodu 250, odpoczynek przy energii do 300. Nowe, robocze progi przerywania aktywnego planu są celowo inne: głód co najmniej 750 pozwala przerwać odpoczynek/konserwację na jedzenie; energia do 100 pozwala przerwać konserwację na odpoczynek. Jedzenie ma pierwszeństwo przy obu krytycznych potrzebach. Jedzenie nie jest przerywane na konserwację/odpoczynek. To jawne strojenie laboratorium, nie docelowy balans biologii.

Nie przerywamy uruchomionej czynności WorkWorld ani rozpoczętego Step, Boarding, Riding, Exiting. Przy granicy bezpieczeństwa potrzeba jest oceniana ponownie; nie kasujemy rozpoczętego przejścia i nie teleportujemy. Waiting przed wejściem można opuścić, zwalniając nieprzydzieloną kolejkę. Fizyczne zajęcie kładki nadal utrzymuje własność do wyjścia.

Nowy plan potrzeby musi dać się wyznaczyć do znanego stanowiska, z zachowaniem lokalnego uczenia dostępności. Nie zaglądamy do zdalnego zapasu. Nieudana próba nie niszczy aktualnego planu; ponowienie po 40 krokach ogranicza koszt. Przerwanie zachowuje odłożony rodzaj zadania i stanowisko. Po obsłużeniu zwykłych potrzeb wznawiamy odłożony zamiar, ponownie planując od aktualnej pozycji. Najwyżej dwa różne odłożone zadania (Maintain, Rest); brak duplikatów. Zaspokojony już odpoczynek jest wygaszany bez dopisywania ukończenia. Komenda ręczna albo wyłączenie autonomii kasuje odłożone zamiary, bez zmiany reguł kosztów rozpoczętej czynności.

Końcowy Walk w odległości do 6 m lub Arrived może jednorazowo odroczyć przerwanie o 120 kroków. Chroni to końcówkę dojścia przed powtarzanym zawracaniem. Ochrona nie odnawia się, nawet bez przemieszczenia; rozpoczęta czynność pozostaje osobno chroniona.

Nowy stan odłożonych zadań i terminów próby oraz ochrony należy do sesji. `GENESIS_TRAVERSAL 2` dodaje rekord autonomii przy postaci; czytnik zachowuje wersję 1 z pustymi odłożonymi zamiarami. Gdy nowego stanu nie ma, serializer zachowuje wersję 1 i dotychczasowe bajty. Zagnieżdżone formaty WorkWorld oraz stary MotionSession pozostają bez zmiany. Wznowienie nie zapisuje dawnej trasy jako nadal aktualnej.

Kontrakty: INV-02, INV-03, INV-08, INV-09, INV-10, INV-11, INV-12, INV-16. Scenariusze: SC-03, SC-07, SC-08, SC-11, SC-14. Nie implementujemy jeszcze ładunku, dostaw, komunikacji ani agresji.

## Plan wykonania

Wykonanie samodzielne według writing-plans/executing-plans i test-driven-development. Zatwierdzone zlecenie obejmuje realizację tej rekomendacji; nie prosimy ponownie o zgodę na rutynowe decyzje zgodne z zakresem. Repo bez HEAD: pozostajemy w bieżącym katalogu, zachowując kopie bazowe w `build/diagnostics-needs-baseline/`. Stan wykonania w `build/diagnostics-needs-progress.txt`.

### 1. Inspektor i alarmy

Pliki: `src/platform/traversal_main.cpp`, `src/lab/traversal_diagnostics.hpp/.cpp`, `tests/traversal_diagnostics_tests.cpp`, `tools/test_traversal_diagnostics.py`.

- [x] Czerwony test natywnego inspektora i niezależnych alarmów dwóch obserwowanych postaci.
- [x] CRLF wyłącznie w widoku; nowe alarmy per ID, bez zmiany limitu i bez mutacji świata.
- [x] Czerwone przypadki częstej zmiany waypointu/Moving–Waiting bez przemieszczania, potem rzeczywisty postęp i poprawny objazd.
- [x] Odczytowy kontekst sesji, pomiar parkowania/Step/transportu, jednostkowe i rzeczywiste kolejki.

### 2. Pilność, odłożenie i kontynuacja

Pliki: `src/lab/traversal_session.hpp/.cpp`, nowy `src/lab/traversal_needs.cpp`, `src/lab/traversal_codec.cpp`, nowy `tests/traversal_needs_tests.cpp`, `CMakeLists.txt`.

- [x] Czerwony test przerwania konserwacji przy krytycznym głodzie i późniejszego wznowienia.
- [x] Wspólny wybór znanych stanowisk, preemption tylko na bezpiecznej granicy i po znalezieniu zastępczej trasy; zwykłe progi zachowane.
- [x] Testy wyczerpania, niedostępnej potrzeby, braku przełączania co krok, ręcznego przejęcia i trwającej czynności.
- [x] Wersja 2 z kontrolą stosu i referencji; stare pliki, kontynuacja w toku odłożenia/przejazdu oraz korupcja z poprawną checksumą.

### 3. Integracja i odbiór

- [x] Pełne Debug/RelWithDebInfo/ASan bez wyłączeń i zwiększania timeoutów; styl, dokumentacja i testy narzędzi.
- [x] Natywna diagnostyka GPU i rzeczywiste sprawdzenie czytelności inspektora oraz F6/F9.
- [x] Analiza kontynuacji zachowanego długiego świata; nie przypisywać starego benchmarku nowej wersji.
- [x] Przegląd autora, raport dowodów i wszystkich pozostałych ograniczeń. Dopiero potem transfer materiału.
