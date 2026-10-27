# Audyt i odbiór istniejącego M04

Zlecenie użytkownika 27.10.2026: domknąć techniczne sprawdzenia istniejącej całości, naprawić wykazane defekty i przygotować próbę człowieka. Bez automatycznego odbioru przez człowieka, bez rozszerzania etapu o całe sądownictwo, nowe AI, solver lub produkcję. Zmiany pozostają bez commitu do osobnego polecenia.

## Wybór i kontrakt

Rozszerzenie istniejącej sceny przeprawy o osiem osób, odpoczynek i regułę własności pozwala połączyć już istniejące subsystemy w jednej sesji. Osobne kolejne dema nie dowodzą tej integracji; projekt nowej gospodarki wykracza poza zlecony odbiór. Używamy wspólnego WorkWorld/TraversalSession, tej samej fizyki i zapisu wersji 7, bez nowego stanu symulacji lub bibliotek.

Kontrakty: INV-02/03/04/05/06/07/08/09/10/12/14/16; scenariusze SC-04/05/08/10/14 w obecnym ograniczonym zakresie. Scena nie produkuje materiału i nie jest samowystarczalną cywilizacją. Limity świata i CTest pozostają bez zmian.

Planowany konsument `make_community_course()` ma kuriera, dwóch odbiorców posiłków, konserwatora, opiekuna i mieszkańców korzystających z pojemności odpoczynku oraz ręczną osobę przy przyczółku. Stanowisko konserwacji należy do opiekuna, konserwator jest uprawniony. Pobranie przez gracza jest fizycznie możliwe; miejscowy świadek może przekazać fakt obecnemu blisko opiekunowi. W ten sam świat wchodzą zamknięcie, objazd, rzeczywista naprawa i spłata.

## Plan wykonania

1. `tests/community_tests.cpp` + CMake: oczekiwania sceny, spokojna autonomia, zaburzenie po rozpoczęciu życia świata, rzeczywisty transfer/posiłek/odpoczynek/konserwacja, lokalna wiedza i prawo, kontynuacja. Pierwszy brak sceny ma czerwony build; naprawy defektów wymagają osobnej czerwonej reprodukcji.
2. `src/lab/traversal_course.cpp/.hpp`: konkretna scena przez istniejący model; żadnych gotowych sukcesów ani sterowania NPC przez diagnostykę.
3. Testowa prosta rutyna kuriera: fizyczne dojście do źródła, pobranie, stałe waypointy środka mostu, odbiorca, powrót. Używa wspólnych metod, prędkości i cargo; nie przeskakuje mostu i nie odczytuje zapasu zdalnie. To jawny stały plan referencyjny, bez planera potrzeb/alternatyw kuriera. Porównanie pierwszej dostawy przez 240 kroków (przed progiem głodu 250), identyczne pozostałe osoby i stan początkowy. Warianty spokojne i zamknięcie po 12 krokach; nie wymagamy zwycięstwa adaptacji, zachowujemy oba wyniki, czas, materiał i przyczynę oczekiwania.
4. `src/platform/traversal_main.cpp`, `tools/test_community_native.py`: `--community`, role i skutki w panelu, F8/F4/F7/R i dwa procesy D3D11 z zachowaniem zapisu. Osobna, ograniczona instrukcja próby gry dla użytkownika.
5. Uaktualnić stan M04 w jednym planie, README, AGENTS i mapie dokumentacji. Raport macierzy kryteriów oddziela zaliczenie techniczne, nierozwiązane defekty i oczekującą ocenę człowieka. Długi benchmark i szersze badanie graczy pozostają w M06; brak pełnych instytucji nie blokuje ograniczonego M04.
6. Debug/Release/ASan, format, dokumentacja, Python, próba MX330, niezależny review według umiejętności requesting-code-review; zachować porażki i sumy dowodów.

## Kryteria przed uruchomieniem

Spokojne 600 kroków: dostawa i posiłek po prawej, konserwacja i co najmniej jeden odpoczynek, poprawny bilans i unikalne cargo. Zaburzenie po 12 krokach: poprawne zamknięcie, brak zdalnej wiedzy kuriera, fizyczna obserwacja lub przekaz, wykonany objazd/naprawa, dalsza dostawa i posiłek. Prawo: świadek wie, opiekun przed kontaktem nie wie, potem odmawia nowej pracy; zwrot nie teleportuje i zamyka reakcję po wiadomości. Kontynuacja: praca/trasa/ładunek/wiedza zachowane, porównanie kolejnych kroków, odrzucona korupcja nie podmienia świata. Referencja musi wykonać rzeczywistą spokojną dostawę; gorszy wynik w innym wariancie pozostaje wynikiem.

Status wykonania i rzeczywiste liczby dopisuje raport, nie niniejsze oczekiwania.
