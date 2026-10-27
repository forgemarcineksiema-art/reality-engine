# Lokalna wiedza o stanowiskach

Status: ograniczony przyrost techniczny zrealizowany i zweryfikowany 27.10.2026. Poniżej zachowano projekt i kolejność wykonania; rzeczywiste wyniki oraz ograniczenia są w [raporcie](../verification/local-station-knowledge.md). To część wejścia w M04, nie odbiór etapu.

## Cel, kontrakty i wybór rozwiązania

INV-08, SC-05 i część SC-14: postać wybiera według lokalnej wiedzy, może się mylić i weryfikuje ją fizycznie. Zachowujemy INV-01–INV-06, INV-09–INV-12, ENG-03, ENG-09 oraz istniejące reguły dostawy i bezpiecznych przerwań.

Wybór implementacyjny: opcjonalny rekord na każde z maksymalnie 16 stanowisk w `TraversalActor`, bez busa wiadomości i bez kopii gospodarki. Brak rekordu oznacza brak danych. Rekord przechowuje wolną ilość, zużycie konstrukcji, czas obserwacji, czas poznania, trwałe ID świadka i ostatniego rozmówcy. Pełny dziennik rozmów byłby nadmiarowy dla bieżącego konsumenta; sama maska z timerem nie potrafi wyrazić wieku i pochodzenia informacji.

Bezpośrednia obserwacja wymaga `interaction_accessible` na miejscu: próba czynności, jej zakończenie i `interact_cargo` przed/po transferze. Tylko te miejsca odczytują bieżącą dostępność dla wiedzy. Nie ma zdalnego odświeżania.

Jawne kierunkowe `share_knowledge(sender, recipient)` przekazuje znane obserwacje, tylko gdy ich `observed_tick` jest ściśle większy. Równe czasy zachowują dotychczasową wiedzę; własna obserwacja na miejscu może ją zastąpić w tym samym ticku. Źródło pozostaje świadkiem, pośrednik ma osobne ID. Kontakt: do 2 m, wspólna podpora oraz obustronny test przejścia kapsuły przez geometrię. To konserwatywny kontakt, bez słuchu przez ściany, wzroku i pełnej rozmowy społecznej.

Automat przekazuje wiadomość bliskim mieszkańcom tylko przez 40 kroków po własnej obserwacji braku lub udanym oddaniu materiału. Nie uruchamia ogólnego rozgłaszania przy każdym mijaniu; odbiorca ręczny może otrzymać informację, ale nie zostanie przejęty. Pośrednik może przekazać rekord przez jawne API gracza. F7 wybiera najbliższego rozmówcę; wynik i wiedza wybranej osoby są widoczne w oknie.

Pusta obserwacja odsuwa zwiad do `observed_tick + 100`, bez odmładzania przez pośredników. Nowsza dostępność znosi lokalny cooldown niedostępności i budzi wybór. Wiadomość o pustym celu może unieważnić podróż potrzeby dopiero poza czynnością, rozpoczętym Step, transportem i manewrem clearance. Nie anuluje kursu ani niesionego materiału. Po horyzoncie postać znów sprawdza stanowisko, nawet bez wiadomości.

## Zakres i kolejność wykonania

1. Rejestr dokumentacji, testy scenariuszy w `tests/knowledge_tests.cpp`, CMake; czerwone wyniki przed implementacją.
2. `traversal_session.hpp`, konkretny `traversal_knowledge.cpp`: rekord, kontakt, przekaz, walidacja, ograniczony automat. Integracja w `traversal_needs.cpp`, `traversal_delivery.cpp`, `traversal_session.cpp`.
3. `traversal_codec.cpp`: zewnętrzna wersja 5, rekordy wiedzy i termin automatycznego przekazu; transakcyjna kontrola ID, czasów, zakresów i znanych stanowisk. Wersje 1–4 bez dopisywania fikcyjnych obserwacji; bez nowej wiedzy zachowujemy historyczny wybór wersji. WorkWorld bez zmiany formatu.
4. Inspektor odróżnia `belief` od `station` (prawda diagnostyczna). `traversal_main.cpp`: F7, komunikat i próba natywna.
5. Pełne Debug/Release/ASan, format-check, dokumentacja, testy Pythona, okno D3D11 z diagnostyką na rzeczywiście odczytanym adapterze. Niezależny review i poprawki z regresjami. Bez commitów.

## Dowód odbioru

Testy wymagają fizycznego dojścia i odmowy, przekazu od świadka wyczerpania i zmiany planu, dostawy oraz rzeczywistego posiłku przed dawnym terminem retry, błędnej nieaktualnej wiedzy i jej lokalnej korekty, odmowy przez ścianę/odległość/podporę, zachowania rozpoczętych działań, kontynuacji 1–5 i transakcyjnej odmowy uszkodzonych rekordów z poprawnym checksumem. Starsze regresje i limity czasu pozostają. Raport oddzieli testy modelu, próbę okna, przegląd oraz nadal niewykonany odbiór całej społeczności i komfortu przez człowieka.
