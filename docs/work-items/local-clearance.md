# Lokalna prośba o ustąpienie przy rozpoczętym Step

Status: ZALICZONE technicznie według [raportu](../verification/local-clearance.md); zakres nie kończy M04. Użytkownik polecił kontynuować po wskazaniu rzeczywistej blokady osoby 6. Zachowujemy dotychczasowe prace, bez commitów i nowych zależności. Stosujemy brainstorming/writing-plans, TDD i osobny review. To mała współpraca, bez reputacji, agresji lub dowolnego planowania tłumu.

## Cel i granice

Postać z fizycznie rozpoczętym i blokowanym Step może lokalnie poprosić blokującego o miejsce. Odbiorca odpowiada sam: zgoda tylko dla autonomicznego, stojącego NPC bez rozpoczętej czynności, Step, transportu, manewru lub własności wąskiego przejścia. Postać sterowana ręcznie odmawia i nie otrzymuje obcego celu. Brak bezpiecznej trasy na bok jest odrębną odmową. Prośba nie upoważnia do kolizji, teleportacji lub odwołania rozpoczętej czynności.

Obserwacja dotyczy pozostałego fizycznego profilu Step i aktualnych kapsuł w promieniu 3 m. Kolejność jest deterministyczna; jedna odpowiedź na aktualną prośbę, jeden manewr na odbiorcę. Pierwsza próba po 100 zablokowanych podkrokach; ponowienia co najmniej 40 kroków świata. Ruch na bok wyznaczany na tej samej nieruchomej podporze, maksymalnie 24 kandydatów i cztery zapytania A*. Musi omijać geometrię, ludzi, inne zarezerwowane cele manewrów i pozostały profil kroku proszącego. To propozycja obliczona przez odbiorcę, bez odczytu zdalnych zapasów.

Manewr ma najwyżej 200 kroków (10 s). Przechowuje pierwotny punkt celu, zachowując zamiar, zadanie/stanowisko i odłożone potrzeby. Po udostępnieniu miejsca NPC czeka na zakończenie uzgodnionego Step; potem planuje własną pracę od nowej pozycji. W tym czasie nie wykonuje fikcyjnej pracy przy odsuniętym stanowisku. Wygaśnięcie lub ręczne przejęcie są jawne i nie odtwarzają starej trasy. Rozpoczęty Step proszącego pozostaje chroniony.

Sesja przechowuje ostatnią odpowiedź i aktywny manewr. Nowy zewnętrzny format `GENESIS_TRAVERSAL 3` dodaje rekordy `CLEARANCE` i `YIELD` przy aktorze. Bez nowego stanu zachowujemy wersje 1/2; ich odczyt daje puste interakcje. Dotychczasowe bajty przed pierwszą interakcją pozostają. WorkWorld i MotionSession bez zmian. Zapis manewru musi kontynuować ruch oraz dokończenie prośby, nie tylko log zgody.

F3 pozwoli graczowi zlecić tę samą prośbę; automat używa tego samego API. Inspektor podaje stabilne ID, odpowiedź, powód i terminy. Diagnostyka pozostaje odczytowa. Warunkiem sukcesu jest fizyczne zakończenie kroku i kontynuacja podróży.

## Zakres i kolejność

1. Czerwony test minimalnej blokady Step oraz kontrolny wariant nieautonomicznego odbiorcy; zachować rzeczywistą migawkę i wcześniejsze czerwone dowody.
2. `traversal_session.hpp`, nowy `traversal_clearance.cpp`, integracja w `traversal_session.cpp`/`traversal_needs.cpp`: jawna odpowiedź, wyznaczenie manewru, wykonanie istniejącym ruchem, wygaśnięcie, kontynuacja zamiaru i ręczne przejęcie. Osobny moduł jest rzeczywistym konsumentem fizyki i nawigacji, bez nowego managera.
3. `traversal_codec.cpp`: format 3, ścisła kontrola referencji, ograniczeń i zgodności pary. Odczyt transakcyjny; formaty 1/2 nadal czytelne. `traversal_main.cpp`: F3 i widoczna odpowiedź.
4. `tests/clearance_tests.cpp`, CMake: zgoda i prawdziwe przejście, odmowa gracza/busy/braku miejsca, limity i wygaśnięcie, podwójna prośba, przerwanie, zapis w toku, korupcja, stare pliki i rzeczywista scena.
5. Pełne Debug/Release/ASan, styl, narzędzia/dokumentacja, rzeczywiste okno D3D11 oraz oddzielny review. Zaktualizować granice wcześniejszego testu NoPath: zachowana wersja z ręcznie kontrolowanym blokującym nadal musi być stabilna; pełna autonomiczna scena otrzymuje oddzielny test współpracy.

Kontrakty: jeden właściciel pola, lokalna wiedza, wspólna kolizja i działanie, aktualność planu, zegar, spójna kontynuacja i rzeczywisty skutek. Nie zaliczamy M04 na podstawie samej tej prośby; dostawy nadal nie istnieją.


## Doprecyzowania po reprodukcjach

Jedna aktywna zgoda na proszącego jest utrzymywana do zakończenia/wygaśnięcia/anulowania; kolejna prośba nie nadpisuje jej innym odbiorcą. Lokalne rezerwacje miejsc są ogłoszonymi punktami protokołu i są sprawdzane w pobliżu kandydata niezależnie od bieżącej odległości uczestnika. Koniec zgody po fizycznym Step zatwierdzamy również na końcu tego samego ticku; aktywny zapis wymaga identycznego, nadal rozpoczętego kroku. Te reguły mają czerwone reprodukcje w [raporcie](../verification/local-clearance.md).
