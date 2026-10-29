# Historia zaopatrzenia w sesji przestrzennej

**Właściciel tematu:** opcjonalny kontrakt historii SC-13, API, granice domen i zapis. Zakres zatwierdza [projekt](work-items/m05-story-design.md); wykonane dowody i ograniczenia należą do [raportu](verification/m05-story.md).

## Własność i role

TraversalSession posiada opcjonalny StoryState, inicjalizowany z StorySeed przez make_story_course(). Role wykonawcy, rozmówcy i odbiorcy wskazują trwałe EntityId, źródło/cel wskazują rzeczywiste partie stanowisk Eat, materiał typu 1, cel cztery jednostki. Rola miejsca wskazuje statyczny element geometrii i niezmienny kontekst jego definicji. Kontekst nigdy nie wraca do fizyki, renderera lub bazowej geometrii; służy odróżnieniu usuniętego wiązania od nieznanego ID.

StoryState posiada etap, czasy, granice sekwencji zdarzeń, komendę i torbę zlecenia, historyczny kontakt/odczyt oraz ID rzeczywistego posiłku. WorkWorld zachowuje jedyną prawdę życia osoby, materiał, transfery i pracę. Historia nie przekazuje globalnych odczytów do planera NPC. Podpory punktu spotkania, źródła, celu i samo miejsce muszą być statyczne; nieobsługiwana definicja daje jawną odmowę walidacji.

## Przygotowanie świata

remove_actor(EntityId) i remove_static_body(uint32_t) są ograniczonymi transakcyjnymi komendami przygotowania. Działają przy ticku 0 i nierozpoczętej historii. Usuwany rozmówca nie może posiadać ładunku, zlecenia, pracy, świadectw, zależnych przekonań lub chronionego sterowania. Nie usuwamy pierwszego ani ostatniego aktora; praktyki, prawo, aktywne kolejki oraz złożone powiązania spoza tego zakresu odmawiają operacji. Usunięcie korzysta z WorkWorld.remove_actor, a sesja usuwa lokalny rekord i aktualizuje pochodne tablice wejścia/prezentacji. Historyczne ID zostaje w zdarzeniach domeny także po ponownym wykorzystaniu slotu rejestru.

Statyczny element nie może podtrzymywać osoby, stanowiska, połączenia, platformy, terminalu lub chronionej operacji. Faktycznie znika z bazowej geometrii; jej wersja rośnie. Odmowa nie zmienia sesji. To autorskie usunięcie elementu, bez symulowania rozpadu konstrukcji lub śmierci w dowolnej fazie gry.

Przeniesienie zapasu w początkowej scenie używa assign_delivery/interact_cargo/cancel_delivery innej ręcznej osoby. Cztery jednostki zostają w jej rzeczywistej torbie. Warunki sceny i launcher nie ustawiają arbitralnie zapasu ani wyniku historii.

## Żądanie i postęp

begin_story(size_t) zwraca Accepted, Ended, OutOfReach, Busy lub Unavailable. Indeks jest wyłącznie wejściem bieżącego sterowania; wykonawca jest porównywany po ID. Pierwsza bramka wymaga lokalnej żywej osoby na podporze znanego punktu spotkania, w fizycznym zasięgu interakcji. Dopiero potem oceniane są strukturalne wiązania rozmówcy i miejsca. Brak wiązania zamyka scenę bez udawania rozmowy i bez dopisywania wiedzy NPC. Pierwszeństwo odmów: MissingSpeaker, MissingSite, MissingSupply.

Rozmowa z obecną osobą wymaga knowledge_contact: do 2 m, wspólna podpora i obustronna kontrola przeszkód. Odczyt źródła wymaga interaction_accessible i własnej observe_station. Dalekie żądanie pozostawia Dormant i nie czyta zapasu. Ruch, kolejka wejścia, zaplanowana czynność, chroniona operacja lub własny ładunek daje Busy bez przejęcia lub kasowania zamiaru.

Przyjęcie wystawia zwykłe zlecenie czterech jednostek. Nie włącza autonomii. F2 pozostaje jawnym wyborem użytkownika; ręczny ruch i F4 korzystają z tej samej domeny. Ponowienie przyjętej historii tylko potwierdza jej etap, nie tworzy nowej komendy/torby. Pierwotny rekord zlecenia wykonawcy pozostaje związany z historią również po jej zakończeniu.

Completed wymaga rzeczywistych przyjętych transferów z przypisanej torby do celu po początku zlecenia, pełnych 4/4 i ukończonej czynności odbiorcy na tym stanowisku, rozpoczętej po dostawie. Sam tekst, zapełnienie celu przez inną osobę lub inny rodzaj pracy nie wystarczą. Historia nie wypłaca nagrody.

Ręczne anulowanie lub niewykonanie w 600 krokach (30 s, wspólny zegar 50 ms) daje Failed. Na granicy najpierw oceniany jest rzeczywisty sukces. Zwykłe anulowanie kursu zachowuje torbę, rozpoczęty Step/transport/czynność i kontrakt późniejszego zwrotu. Zakończenia są trwałe; późniejszy powrót zapasu nie zmienia odmowy w sukces.

## Otoczka i walidacja

Opcjonalny GENESIS_TRAVERSAL 10 występuje tylko ze StoryState. Zachowuje wspólne sekcje 9, dopuszcza zero dalekich połączeń, ma OPTIONAL_PRACTICE oraz ścisłe rekordy STORY, STORY_SITE, STORY_STATE i STORY_CONTACT. WorkWorld nie zmienia formatu. Odczyt starego 1–9 usuwa poprzedni stan historii i zachowuje dawne bajty; nie odtwarza listy ludzi z pierwotnych spawnów.

Źródło historycznej dostępności to istniejący prefiks zdarzeń WorkWorld: utworzenie, transfery, rezerwacje, zużycie, zwolnienie i usunięcie. Źródłem walidacji usuniętej osoby jest ActorCreated w dzienniku, nie aktualny slot, który mógł zostać ponownie wykorzystany. Granice zdarzeń nie mogą sięgać w przyszłość względem zapisanych czasów. Przyjęty etap musi nadal wskazywać właściwy kurs, a sukces rzeczywistą dostawę i posiłek. Błędne role, kontekst miejsca, czasy, materiały lub etapy są odrzucane. Cały kandydat jest zatwierdzany dopiero po walidacji.

Znacznik `order` jest unikalną rezerwacją we wspólnej przestrzeni komend. `order_end` zapisuje wyłączną granicę transferów zakończonej historii; późniejsze zlecenie nie może dostarczyć jej dowodu sukcesu. Historia sprawdza rzeczywiste transfery źródło→torba→cel oraz późniejszy posiłek. Failed wymaga zakończonego zlecenia lub upływu terminu po już zakończonej dostawie.

Rekord dostawy przypisanej do tej pojedynczej historii pozostaje związany z wykonawcą także po zakończeniu. assign_delivery odmawia jego zastąpienia; ręczny transport, ruch, potrzeby i zwykły zwrot nadal działają. Zapis sprawdza trwały rekord wraz z przedziałem komend i fizycznymi transferami. To ograniczenie jednej sceny, nie historia wszystkich zleceń świata.

Po rzeczywistym sukcesie anulowanie pozostałego powrotu zamyka wykonany kurs jako Completed, używając zwykłego stop() i ochrony rozpoczętych operacji. Nie zmienia zakończenia historii ani WorkWorld; nie oznacza sukcesu kursu anulowanego przed posiłkiem.
