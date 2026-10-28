# Dostawa przez naprawione przejście albo objazdem w M05

**Status: technicznie ukończone i zweryfikowane 29.10.2026; odbiór użytkownika nowej sytuacji niewykonany.** Użytkownik polecił rozpocząć ten przyrost po akceptacji obecnego uczenia 28.10.2026. Właścicielem kolejności i pełnego zakresu M05 pozostaje [jeden plan](../prototype-plan.md). Wyniki i ograniczenia podaje [raport](../verification/m05-two-methods.md).

## Cel i granica

W istniejącej społeczności ośmiu osób dostarczyć osiem jednostek żywności na drugi brzeg: po poznaniu metody i rzeczywistej naprawie mostu albo istniejącym promem. Obie metody muszą działać fizycznie, mieć koszt i czas oraz zostawiać różny stan świata. To ograniczony zakres SC-10 oraz kontynuacji SC-08, bez migracji reprezentacji SC-09 i pełnej historii SC-13.

Obecne API już zapewnia kalibrację, konserwację, otwarcie po ukończeniu pracy, fizyczny transfer, prom, lokalną wiedzę i zapis 8. Brakuje wspólnej sytuacji startowej i jej odbioru obiema metodami. Wybrano wariant istniejącej sceny zamiast nowego planera lub drugiego modelu świata.

## Ustalenia wykonawcze

- `make_two_methods_course()` rozszerza `make_learning_course()`: most zaczyna zamknięty, zużycie wynosi 1000. Stan oznacza niedostępność, nie fizyczne zawalenie.
- Kurier (2) i badacze (4/6) zaczynają w trybie ręcznym; pozostali zachowują autonomię. To jawna decyzja autora, aby przed wyborem gracza nikt nie rozstrzygnął sytuacji. F2 uruchamia dotychczasową autonomię wybranej osoby. Nie dodajemy reżysera dopisującego rezultaty.
- Objazd: uruchomić kuriera 2; badacze pozostają ręczni. Naprawa: uruchomić badacza 4, po otwarciu mostu uruchomić kuriera 2. Można też wykonywać dotychczasowe ręczne czynności. Most i aparat nie dostają darmowych części lub energii.
- Osiem jednostek wymaga istniejących partii po najwyżej cztery; oczekiwany wynik to fizycznie oddany materiał i rzeczywisty posiłek odbiorcy. Nie wymagamy, żeby naprawa była szybsza od objazdu.
- Kontynuacja i F9 zachowują format 8 oraz działanie dawnych scen. Interfejs pokazuje aktualną dostawę, naprawy, stan mostu i wiedzę; nie obiecuje sukcesu wybranej metody.

## Własność i kontrakty

WorkWorld posiada materiał, transfery i czynności. TraversalSession posiada podróż, dostępność przejścia i lokalne przekonania; PracticeState zachowuje pomiary i jednorazową zgodę na pracę. Obowiązują INV-03, INV-05, INV-06, INV-08, INV-10, INV-12 i INV-14. Nie zmieniamy progów potrzeb, ochrony Step/przejazdu/pracy ani naukowych wyników I/A/T.

## Plan wykonania

Używamy `superpowers:executing-plans` oraz TDD. Zakres plików: fabryka w `src/lab/traversal_course.cpp` i deklaracja w `traversal_session.hpp`; flaga, pomoc i próba okna w `src/platform/traversal_main.cpp`; launcher `tools/start_practice_playtest.py`; nowy runner `tests/two_methods_tests.cpp` i test natywny; CMake oraz dokumenty odpowiedzialne za uruchomienie i wynik.

- [x] Odtworzyć RED: stare okno odmawia `--two-methods`; zachować wynik. Przed implementacją napisać testy dwóch metod i kontynuacji. Punkt wyjścia: 36/36 testów Release `community.*`/`practice.*`.
- [x] Dodać wariant sceny, bez zmian domenowego wykonywania. Testy obserwują po każdym kroku bilans 20 jednostek żywności i 24 części wraz ze zużyciem, autentyczną podróż mostem/promem, osiem oddanych jednostek oraz posiłek. Naprawa wymaga poznanej metody i ukończonej pracy; objazd zostawia most zamknięty i części niewydane.
- [x] Kontynuować obie metody w nowym procesie: checkpoint w pracy oraz z ładunkiem podczas rejsu, identyczne końcowe bajty, brak ponownego zużycia. Sprawdzić odmowę otwarcia zużytego mostu i pracy bez kalibracji, a także brak zdalnej wiedzy.
- [x] Udostępnić `reality_traversal --two-methods` i launcher z tą flagą. Natywna próba używa F2, F5/F9, sprawdza oba przebiegi, obcy poprawny zapis, obraz i diagnostykę D3D11. Dokonać oględzin widoku (zapisany obraz; pełny panel nie był dostępny przez Computer Use).
- [x] Wykonać Debug/Release/ASan, format, narzędzia i dokumentację, osobny przegląd zmiany oraz raport z wynikami i ograniczeniami. Odbiór użytkownika nowej sytuacji pozostaje osobny; dodatkowa optymalizacja ma przegląd autora, bez niezależnego review.

## Ryzyka do przeglądu

Przedwczesna automatyczna naprawa; test, który dowodzi wyłącznie wybranego planu zamiast przejechanej drogi; zapis dublujący części lub dostawę; globalna prawda zastępująca obserwację; panel po F9 zakładający pierwotny świat. Przypadki te należą do testów sceny, przebiegów, kontynuacji i okna wymienionych powyżej.

## Dziennik

28.10.2026: praca na `codex/m05-two-methods`, z zachowaniem czterech zastanych zmian dokumentujących akceptację uczenia. Wybrano wykonanie w dotychczasowym checkoutcie i osobnej gałęzi, aby zachować lokalne buildy oraz te zmiany. Bez publikacji. Wyniki i kolejne ustalenia będą uzupełniane wraz z wykonaniem.

Korekta planu po zachowanej porażce: początkowe 600 kroków nowej próby nie wystarczało do przewiezienia dwóch partii. W obu wariantach było 4/8 i rzeczywisty dalszy postęp, bez utknięcia. Historyczne M04 wymagało w 600 krokach co najmniej czterech jednostek, nie ośmiu. Diagnostyczne przedłużenie tego samego przebiegu, bez zmiany domeny i danych, dało dostawę 8/8 z posiłkiem przy 756 (naprawa) i 967 (objazd). Dla nowej sytuacji ustalono przed końcową weryfikacją horyzont 1200 kroków świata (60 s), obejmujący powrót, drugą partię i potrzeby. Nie zmienia to dawnych prób 240/600 ani limitu CTest 20 s. Koszt tej decyzji: nowy odbiór dowodzi ukończenia do 1200, nie do 600 kroków; nie ma twierdzenia o poprawie szybkości.

Korekta organizacji po pełnym Debug: wspólny test czterech procesów przekroczył limit CTest 20 s, a wspólna próba obu metod w oknie przekroczyła własne 18 s. `two_methods.fresh_repair`/`fresh_detour` i `native_repair`/`native_detour` wykonują odtąd osobno obie niezmienione metody. Zachowano całe przebiegi, asercje, limity 8 s na proces kontynuacji, 18 s na proces okna i 20 s na CTest. Dodatkowy start procesu zwiększa koszt całego zestawu, lecz nie zmienia pracy świata. Flaga `--repair-smoke` wybiera wyłącznie natywną próbę naprawy i wymaga `--two-methods --smoke-test`; zwykłe uruchomienie gry nie korzysta z niej.

29.10.2026, dalsza praca po poleceniu „Kontynuuj”: sam podział nie zapewnił marginesu (objazd nadal przekraczał 8 s w procesie i 18 s w oknie). Pomiar CPU wskazał koszt konstrukcji i odtwarzania umysłów. Próba zmiany obliczania tablic nie wystarczyła i została wycofana. Profil konstruktora wykazał około 0,16–0,23 s przygotowania, 0,08–0,11 s tablic i 0,04–0,06 s deduplikacji na umysł. Zakres rozszerzono o `src/research/contrast_agent.cpp`, `tests/contrast_tests.cpp` i opis API: sortowanie krótkiego F oraz scalenie z już uporządkowaną gramatyką. Pełne sortowanie pozostaje niezależną referencją testu. Kontrakty kolejności, budżetu, tablic i zapisu pozostają bez zmian; nie ma cache pomijającego obliczenia. Selektywny Debug 8/8 PASS po tej zmianie, z oknem 8,95/9,55 s i kontynuacją 7,18/8,08 s na przypadek. To pomiar testów, nie FPS aplikacji. Dodatkowy niezależny przegląd tej optymalizacji nie został wykonany z powodu limitu subagenta; wymagany jest osobny przegląd autora i pełne regresje.
