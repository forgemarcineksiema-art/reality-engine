# Historia zaopatrzenia po zmianie świata

**Status: projekt do zatwierdzenia, 29.10.2026. Kod SC-13 nie jest jeszcze zaimplementowany.** Baza: `5462279`. Zakres realizuje SC-13 i INV-15 w [jednym planie](../prototype-plan.md) oraz [strategii testów](../testing-strategy.md#sc-13--historia-po-zmianie-świata). Odbiór całego M05 pozostaje osobny.

## Cel i wybór podejścia

Jedna autorska historia w obecnej aplikacji ma respektować rzeczywiste osoby, geometrię i materiał. Zmiana świata przed rozpoczęciem rozmowy daje określoną odmowę lub zakończenie. Zwykły wariant kończy się dopiero po fizycznej dostawie i posiłku. Nie przywracamy oczekiwanej osoby, miejsca lub zapasu.

Wybrany kierunek do zatwierdzenia: mały opcjonalny stan sceny w tej samej TraversalSession. Posiada tylko definicję ról i postęp historii; pozostali właściciele zachowują świat, zasoby, ruch i wiedzę. Uzasadnienie: zakres M05 wymaga wspólnego świata i aplikacji.

Rozważone alternatywy: samodzielna scena na WorkWorld byłaby prostsza, ale nie sprawdziłaby konsumentów przestrzeni i obecnego okna. Warunki zaszyte w UI rozdzieliłyby reguły gry i runnera. Ogólny silnik zadań/dialogów nie ma jeszcze drugiego konkretnego konsumenta. W tym przyroście nie powstaje żaden z tych trzech wariantów.

## Historia i role

Jedna sytuacja: rozmówca prosi o dostarczenie czterech jednostek żywności do odbiorcy. Świat ma cztery postacie: wykonawcę/gracza, rozmówcę, odbiorcę oraz osobę umożliwiającą kontrolowane przeniesienie zapasu. Role wskazują trwałe EntityId; nie są indeksami aktualnej tablicy ludzi. Źródło i cel wskazują rzeczywiste partie stanowisk, a miejsce spotkania konkretny statyczny element geometrii.

Stan początkowy ma 4 jednostki przy źródle, 0 w celu, puste partie niesione, nierozpoczętą historię i brak zleceń. Rozmówca, wykonawca i źródło znajdują się w rzeczywistym zasięgu potrzebnych interakcji. Miejsce spotkania jest małą statyczną budowlą na istniejącym terenie; jej usuwany element nie podtrzymuje ludzi, stanowisk, platform ani połączeń. Odbiorca zna lokalny cel i korzysta z dotychczasowego Eat. Scena nie wymaga uczenia nowej metody ani wiedzy globalnej.

Jednostki materiału są fizyczną partią obecnego modelu, nie nowym systemem unikatowych przedmiotów. Bilans obejmuje źródło, cel, każdą torbę i rzeczywiste zużycie.

## Cztery wejścia do tej samej sceny

| Przebieg | Rzeczywista zmiana przed uruchomieniem historii | Oczekiwany wynik |
| --- | --- | --- |
| Zwykły | Brak zaburzenia | Przyjęcie po kontakcie, dostawa 4/4 przez zwykłe transfery, posiłek odbiorcy, jednorazowe ukończenie |
| Usunięty rozmówca | WorkWorld.remove_actor usuwa jego właściwe ID; sesja usuwa jego lokalny rekord | Określone zakończenie bez wymagania rozmowy, przywrócenia osoby lub przypisania roli kolejnemu indeksowi |
| Usunięte miejsce | Właściciel geometrii rzeczywiście usuwa wskazany statyczny element i zmienia wersję | Zakończenie z powodu braku miejsca; element nie wraca w kolizji, rendererze lub po odczycie |
| Przeniesiony zapas | Zwykłe interact_cargo przenosi 4 jednostki do rzeczywistej torby drugiej osoby; zlecenie przenoszącego zostaje anulowane | Po lokalnym sprawdzeniu pustego źródła określona odmowa; materiał nadal istnieje w torbie, dostawa i posiłek nie są dopisywane |

Kombinacje zaburzeń mają stałe pierwszeństwo odmów: brak rozmówcy, brak miejsca, brak zaopatrzenia. To kolejność oceny wykonalności tej konkretnej sceny, bez twierdzenia, że NPC zdalnie poznał wszystkie trzy fakty.

## Granica usuwania świata

Pierwszy zakres SC-13 dotyczy zmiany świata PRZED rozpoczęciem historii. Operacje usuwania są dostępne na bezpiecznej granicy przygotowania kursu, przy ticku 0 i nierozpoczętej historii. Późniejsze próby usunięcia są jawnie odmawiane bez mutacji. Nie jest to implementacja śmierci w dowolnej fazie walki ani rozpadu konstrukcji.

Usuwanie rozmówcy korzysta z WorkWorld.remove_actor i zachowuje jego historyczne ID oraz wpis domenowy. Usuwany rozmówca nie ma ładunku, rozpoczętej pracy, wiedzy będącej źródłem cudzych przekonań, prawa, praktyk, transportu ani clearance. Nie usuwamy pierwszego wykonawcy/właściciela świata ani ostatniej osoby. Po zmianie lokalnych indeksów aktualizowane są wejścia i pochodna historia prezentacji, a role sceny nadal rozwiązuje się po ID. Dane nieobsługiwane przez tę ograniczoną operację dają jawną odmowę, nie cichy fallback. Nie dodajemy drugiego właściciela życia osoby ani flagi udającej usunięcie żywej osoby.

Usuwany element geometrii nie może być podporą, fundamentem platformy, celem stanowiska/połączenia ani częścią rozpoczętej operacji lub trasy. Faktycznie znika z bazowej i wynikowej geometrii. Odmowy zależności i zajętości są transakcyjne. To autorska operacja usunięcia statycznego elementu, bez tezy o fizycznym solverze zawalenia.

Kontrolowane przeniesienie materiału nadal używa istniejącej partii, limitu torby, wspólnej tożsamości komend oraz istniejących reguł anulowania. Role przenoszącego są rozwiązywane po ID także po usunięciu rozmówcy.

## Granica historii, komend i wiedzy

Proponowany opcjonalny StoryState posiada: stałe powiązania ról/partii/miejsca, etap, tick początku i zakończenia, znacznik komendy zaakceptowanego zlecenia, dowód lokalnego sprawdzenia i granicę zdarzeń domenowych. Nazwy konkretnych typów C++ rozstrzyga implementer.

Historia zaczyna się wyłącznie na żądanie użytkownika. Wykonawca musi być lokalną żywą osobą w fizycznym zasięgu znanego punktu spotkania na zachowanym terenie. Tę bramkę sprawdza się przed oceną brakującej osoby lub budowli, więc żądanie z daleka nie ujawnia ich stanu. Punkt spotkania nie jest usuwanym elementem. Rozmowa wymaga obecnego rozmówcy, kontaktu do 2 m, wspólnej podpory i kontroli przeszkody. Sprawdzenie zapasu wymaga interaction_accessible i własnej obserwacji stanowiska. Odległe żądanie nie czyta zapasu i pozostawia historię nierozpoczętą. Nieznana dostępność nie jest równoznaczna z brakiem materiału.

Brak strukturalnego uczestnika/miejsca daje zakończenie sceny bez udawania rozmowy. To wynik wykonalności autorskiego wiązania, nie nowa wiadomość lub wiedza NPC o odległym zdarzeniu. Widok rozróżnia tę odmowę od wypowiedzi rzeczywiście obecnej osoby. Nie zapisuje jej do cudzych beliefs.

Przyjęcie uruchamia istniejące assign_delivery dla wykonawcy, źródła i celu. Nie tworzy żywności ani dodatkowej partii z pominięciem zwykłej reguły cargo. Sterowanie nie jest przejmowane: ręczny gracz może wykonać dostawę, F2 nadal jawnie oddaje ją dotychczasowej autonomii. Historia nie obchodzi bieżących potrzeb ani ochrony ruchu.

Etapy: nierozpoczęta, przyjęta, ukończona oraz określone zakończenia odmowne. Horyzont tej nowej historii: 600 kroków świata (30 s), ustalony przed próbą. Ukończenie sprawdza prawdziwe zlecenie 4/4 i późniejszy rzeczywisty posiłek, nie tekst lub sam stan torby. Na granicy najpierw ocenia się wykonany skutek, potem termin. Niewykonane przyjęte zlecenie po terminie kończy historię niepowodzeniem i anuluje kurs zwykłą komendą, zachowując materiał i ochronę rozpoczętych operacji. Wznowienie pracy/nawigacji pozostaje zgodne ze starymi kontraktami.

Jawne ręczne anulowanie przyjętej dostawy kończy tę historię niepowodzeniem, zachowując dotychczasową fizyczną obsługę ładunku. Zakończenie jest trwałe, nie wypłaca nagrody i nie wymaga końcowej rozmowy. Ponowienie wejścia lub odczyt nie wystawia kolejnego zlecenia. Zwrot materiału po wcześniejszej odmowie nie zmienia potajemnie historii w sukces. Nowa próba oznacza jawnie nowy świat, a nie F9 zerujący postęp.

## Zapis i walidacja

Proponowana opcjonalna otoczka GENESIS_TRAVERSAL 10 występuje tylko ze stanem historii. Wersje 1–9 i stare bajty bez nowego stanu pozostają niezmienne. WorkWorld nie zmienia formatu. Przy zapisie usuniętego uczestnika historia może wskazywać jego archiwalne ID, ale nie wiąże go z nowym żywym slotem. Aktualna lista ludzi może być krótsza; nie rekonstruujemy jej z pierwotnych spawnów.

Zapis obejmuje definicję sceny, stan, czasy, powiązanie rzeczywistego zlecenia, potwierdzenia lokalnej obserwacji i zdarzeń. Walidacja odróżnia historyczną odmowę od bieżącego stanu: po odmowie materiał może fizycznie wrócić, więc czytnik nie przelicza przebiegu wyłącznie z dzisiejszego zapasu. Dla obserwowanego stanu materiału używa granicy sekwencji istniejących zdarzeń WorkWorld i transferów; nie dodaje autorskiego zapasu. Ukończenie bez rzeczywistej dostawy i posiłku, błędne ID/czasy/etapy i sprzeczna własność są odrzucane. Wczytanie zatwierdza cały kandydat dopiero po walidacji.

## Aplikacja i zakres plików

Proponowane uruchomienie: wspólny reality_traversal --story, wybór czterech początków przez launcher. Polecenie rozmowy/przyjęcia i obecny panel pokazują prosty wynik, dostawę i zakończenie. F5/F9, N, wybór żywych osób, ręczny ruch i F2 pozostają konsumentami tej samej sesji. Obcy poprawny świat po F9 używa bezpiecznego panelu bez pozostawienia ról z poprzedniej sceny.

Dotknięte pliki: nowy niewielki moduł historii w src/lab, traversal_session.hpp/.cpp, kodek, konsument geometrii/prezentacji, traversal_main.cpp; nowy runner C++, kontynuacje procesów, natywne smoke i launcher; CMake oraz odpowiedzialne dokumenty. Bez ogólnego systemu questów, instytucji, nowych przepisów, nagród, walki i grafiki jako warunku ukończenia.

## Dowód odbioru

Przed implementacją runner ma wykazać brak rzeczywistej reakcji SC-13. Zwykła scena wymaga dostawy, posiłku i bilansu po każdym kroku. Każde zaburzenie ma oddzielną regresję oraz kombinacje, bez tworzenia uczestnika, miejsca lub zasobu. Ochronę ID sprawdza także usunięcie środkowej osoby i późniejsze działanie pozostałych.

Dodatkowo: odmowa spoza kontaktu i zza ściany; bezczynny start bez automatycznej decyzji; odmowy niebezpiecznego usunięcia bez zmiany bajtów; wielokrotne żądanie, anulowanie i termin; fizyczny powrót materiału po odmowie; zapis nierozpoczętej historii po zaburzeniu, w trakcie dostawy i po każdym zakończeniu w nowych procesach; korupcja ze skorygowaną sumą kontrolną i pełna zgodność historycznych formatów.

Native Win32/D3D11 sprawdza cztery początki, prawdziwe wejście, widoczny wynik, brak usuniętego uczestnika/elementu, F5/F9, obcy zapis, obraz i diagnostykę. Osobno pełne szeregowe Debug/Release/ASan, format, dokumentacja i Python. Nie zmieniamy 240/600 kroków starych prób ani limitów procesów 8/18 s i CTest 20 s. Odbiór nowej sceny przez użytkownika pozostaje osobny.

Baza przed projektem: distant.handoff, two_methods.choice i transfer.removed 3/3 PASS, 1,89 s; [log i manifest](../verification/evidence/sc13-design-2026-10-29/manifest.json). To kontrola punktu wyjścia, bez twierdzenia, że wykonała SC-13.

## Przegląd projektu

Niezależny builder check: „nothing important left”. Bez kodowania, kompilacji lub testów przez recenzenta. Integrator doprecyzował jako własne decyzje bezpieczną lokalną bramkę punktu spotkania przed oceną brakujących ról oraz zakończenie po ręcznym anulowaniu. To doprecyzowanie zatwierdzanego projektu, nie wynik implementacji.
