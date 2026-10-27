# Model tożsamości, czynności i zapisu

**Status: M01, wielorazowa pojemność i ograniczony transfer materiału.** Ten dokument jest właścicielem konkretnego API i formatu modelu. Ogólne wymagania pozostają w [kontraktach](subsystem-contracts.md). Polecenia uruchomienia są w [środowisku pracy](development.md), a dowody w raportach [M01](verification/m01-identity-actions-reservations.md), [pojemności](verification/reusable-capacity.md) i [transferu](verification/material-delivery.md).

## Własność i granice

[WorkWorld](../src/simulation/work_world.hpp) posiada wykonawców, partie materiału, czynności, rejestr komend, dziennik, zegar i [rejestr tożsamości](../src/core/entity_registry.hpp). Zmiany są sekwencyjne, przez jego metody. Widoki zasobu i czynności są kopiami. Referencje do zegara, rejestru i dziennika są tylko do odczytu i obowiązują do zmiany lub zastąpienia właściciela; nie należy przechowywać iteratorów przez mutację.

To model czynności korzystającej z jednego zasobu: zużywanego materiału albo wielorazowej pojemności, bez produktu wyjściowego. Transfer księguje przemieszczenie między partiami; fizyczna podróż i ograniczony konsument potrzeb należą do [sesji przestrzennej](traversal-autonomy.md). Nie ma pieniędzy ani wielozasobowej produkcji. Jedna czynność nie łączy jeszcze materiału i osobnego stanowiska. `remove_actor` usuwa wykonawcę z aktywnego świata; nie jest modelem biologicznej śmierci. `remove_resource` klasyfikuje pozostałą ilość jako stratę; dla pojemności oznacza zniszczenie miejsc. Własność pozostaje historycznym odwołaniem także po usunięciu właściciela; dziedziczenia jeszcze nie ma.

## Typowany materiał i transfer w wersji 3

`MaterialTypeId` jest niezmienny; zero oznacza historyczny nieokreślony materiał, który nie może być transferowany. `transfer(TransferRequest)` przenosi dodatnią ilość między różnymi aktywnymi partiami tego samego niezerowego typu, sprawdzając wolną ilość po rezerwacjach i przepełnienie. Pojemność nie jest materiałem. Wynik i komenda są zapisywane, również dla odmów; wspólna z `reserve` przestrzeń `CommandId` zapewnia identyczny wynik ponowienia i odrzuca inne dane pod zajętym ID. Dwie sąsiadujące pozycje dziennika księgują odpływ i wpływ. Chronologiczny audyt odtwarza także rezerwacje, zużycie i usunięcie bez narastającej sumy obrotu podatnej na overflow. `GENESIS_WORK_WORLD 3` dodaje typ i rejestr transferów; wersje 1 i 2 pozostają czytelne. [Zakres i obecny odbiór](work-items/material-delivery.md) są oddzielone od pełnego M04.

Każdy aktywny wykonawca korzysta z tej samej ścieżki. Model nie rozróżnia gracza i NPC. Nie sprawdza prawa do cudzego materiału, odległości ani wiedzy: fizyczny dostęp, regułę własności i lokalne świadectwa ocenia [sesja przestrzenna](traversal-autonomy.md#świadek-własność-i-legalna-obsługa). Sam nieautoryzowany transfer jest poprawnym stanem integralności. Inspektor jest narzędziem deweloperskim, nie obserwacją postaci.

## Tożsamości i uchwyty

`EntityId` to dodatni licznik 64-bitowy wspólny dla wykonawców, partii i czynności. Zero jest niepoprawne. Kolejna wartość jest zapisywana; usunięte ID nie wraca do puli. To gwarancja w obrębie historii jednego świata. Niezależne światy i rozgałęzione kopie zapisu nie otrzymują automatycznie rozłącznych przestrzeni ID. Cofnięcie do starszego zapisu cofa całą historię; uchwyty z późniejszej gałęzi trzeba odrzucić u konsumenta.

`EntityHandle` zawiera indeks slotu i generację. Usunięcie unieważnia generację, ponowne użycie slotu wydaje nowe ID. Osiągnięcie granicy generacji trwale wycofuje slot, bez zawijania licznika. `UINT64_MAX` licznika ID jest znacznikiem wyczerpania i nie jest wydawany. Odczyt przez nieaktualny uchwyt zwraca brak. Uchwyty mają sens tylko względem rejestru, który je wydał; nie są kluczami między światami. Trwałe odwołania w danych używają ID.

## Komendy i fazy

Opis zużycia w tabeli dotyczy materiałów. Dla wielorazowej pojemności obowiązuje rozszerzenie poniżej; pozostałe reguły tożsamości, czasu, komend i błędów są wspólne.

`reserve(WorkRequest)` przyjmuje ID komendy, wykonawcę, partię, wymaganą ilość, część zużywaną na początku i czas w krokach. ID komendy nadaje konsument, unikalnie dla tego świata. To osobna przestrzeń od ID encji.

| Operacja | Skutek |
| --- | --- |
| `reserve` | Waliduje wejście, uczestników i wolny zapas; tworzy czynność `Reserved` i zobowiązanie do materiału |
| `begin` | Wyznacza termin względem obecnego kroku, zużywa część początkową, przechodzi w `Running` |
| `advance` | Przetwarza należne zakończenia według terminu, następnie ID; zużywa pozostałą część i przechodzi w `Completed` |
| `cancel` | Kończy `Reserved` albo `Running` jako `Cancelled`; zwalnia resztę bez zwrotu już zużytego materiału |
| Usunięcie uczestnika | Przerywa wszystkie jego aktywne czynności z jawną przyczyną |

Wymagana ilość i czas muszą być dodatnie; część początkowa mieści się w wymaganej. Pusta partia jest dozwolona. Odmowa domenowa zwraca `InvalidRequest`, `ActorUnavailable`, `ResourceUnavailable` lub `InsufficientResource`, bez czynności. Kolejność tych kontroli jest kolejnością pierwszej zgłaszanej odmowy.

Identyczna komenda odtwarza pierwotny wynik, również odmowę, nawet jeśli świat później się zmienił. Nowa próba planu wymaga nowego ID komendy. Zmiana payloadu pod zajętym ID i zerowe ID komendy rzucają `invalid_argument`. Rozpoczęcie już uruchomionej lub ukończonej czynności oraz ponowne przerwanie przerwanej zwracają `Unchanged`. Niedozwolona faza daje `InvalidState`, brak czynności `Missing`, niemożliwy termin `TimeOverflow`.

Nie ma osobno mutowanego licznika rezerwacji: widok sumuje `required - consumed` aktywnych czynności. Zużycie na początku zmniejsza jednocześnie fizyczną ilość i pozostałe zobowiązanie. Wolna ilość nie zwiększa się z powodu samego rozpoczęcia pracy. Czynność może być w toku po zużyciu całego wymaganego materiału; wtedy jej rezerwacja wynosi zero, ale czas pracy nadal obowiązuje.

## Czas, bilans i błędy

Świat używa dodatniego stałego kroku `SimulationClock`, domyślnie 50 ms; czas rzeczywisty nie steruje postępem. Rezerwacja nie ustala terminu: ustala go `begin`. `advance(0)` nie zmienia świata. Duży skok rozlicza zdarzenia z ich faktycznymi terminami, nie czasem końca skoku.

Dla partii bez transferów walidacja wymaga: ilość początkowa = ilość pozostała + zużycie wszystkich jej czynności + jawna strata. Dla partii uczestniczących w transferze odtwarza chronologicznie także wpływy i odpływy, sprawdzając stan po każdej operacji. Sprawdza też referencje, fazy, terminy, komendy i zgodność cyklu życia oraz zużycia z dziennikiem. Nie odtwarza pełnego historycznego planowania z dziennika.

Przepełnienie, wyczerpanie ID, limit danych i błąd alokacji nie mogą częściowo zatwierdzać operacji. Pamięć potrzebna do dopisania rekordów i zdarzeń jest przygotowywana przed zmianą domeny. Nie ma współbieżnych zapisów stanu. Nieoczekiwane błędy są wyjątkami; konsument nie zamienia ich w sukces.

Limit prototypu wynosi 4096 rekordów na kolekcję, w tym historię komend i zdarzeń oraz sloty rejestru. Wyczerpanie jest jawnym błędem, bez retencji i automatycznego wyrzucania historii. Nie jest to rozwiązanie dla wieloletniej symulacji. Liniowe wyszukiwania i pełna walidacja są świadomym zakresem małego modelu; nie wykonano benchmarku skali.

## Format i plik

[Codec i I/O](../src/persistence/world_snapshot.hpp) udostępniają `serialize_world`, `deserialize_world`, `save_world` i `load_world`. Wersja 1 to tekst z klasycznym locale, liczbami dziesiętnymi i nagłówkiem `GENESIS_WORK_WORLD 1`. Sekcje w kolejności: `CLOCK`, `REGISTRY`, `ACTORS`, `RESOURCES`, `ACTIONS`, `COMMANDS`, `EVENTS`, `END`, `CHECKSUM`. Kolejność i pola rekordów definiuje obecny codec; zmiana znaczenia lub układu wymaga nowej wersji i jawnej obsługi starszej.

Zapis obejmuje generacje i licznik ID, historycznych uczestników, właścicieli, ilości, straty, pełne fazy i payloady czynności, terminy, komendy z odmowami, zegar oraz dziennik. Rezerwacje odtwarzają się z czynności. Nie zapisuje adresów, paddingu ani pojemności wektorów.

Limit wynosi 4 MiB, każda kolekcja jest ograniczona przed alokacją. Liczby są parsowane ściśle, bez ujemnych wartości, przepełnień i cichego obcięcia. FNV-1a 64 obejmuje bajty przed sekcją `CHECKSUM`; wykrywa przypadkową korupcję, nie uwierzytelnia pliku. Po sprawdzeniu wersji i checksum powstaje osobny kandydat, który przechodzi walidację. Wyjątek odczytu nie zastępuje istniejącego świata; konsument może go podmienić dopiero po sukcesie.

Zapis na Windows tworzy przez `CREATE_NEW` własny plik tymczasowy obok docelowego, zapisuje i opróżnia go przez [FlushFileBuffers](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-flushfilebuffers), zamyka uchwyt i dopiero wtedy zastępuje plik przez [MoveFileExW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexw). Błąd podmiany jest zgłaszany, a własny plik tymczasowy usuwany przez RAII. Po nagłym zakończeniu procesu może pozostać plik tymczasowy; nie jest automatycznie uznawany za zapis świata. Nie obiecujemy odporności na awarię dysku, dowolne odcięcie zasilania ani transakcji wielu procesów. Test rzeczywistej blokady podmiany zachowuje poprzedni poprawny zapis.

## Konsument i dalsze rozszerzanie

### Wielorazowa pojemność i wersja 2

`create_resource(owner, quantity, kind)` przyjmuje niezmienny `ResourceKind::Material` albo `ResourceKind::Capacity`; domyślny argument zachowuje materiał dla dotychczasowych konsumentów. Zero pojemności jest dozwolone, ale nie pozwala zarezerwować dodatniej liczby miejsc. Nieprawidłowy rodzaj jest odrzucany przed zmianą świata. Pojemność nie jest automatycznie odnawianym materiałem.

Dla `Capacity` część zużywana początkowo musi wynosić zero, a `consumed` zawsze pozostaje zerem. Cała wymagana liczba miejsc jest zajęta w fazach `Reserved` i `Running`. Ukończenie albo anulowanie zwalnia ją bez zmiany fizycznej pojemności i bez zdarzenia `Consumed`. Próba zużywania pojemności daje `InvalidRequest`. Usunięcie zasobu lub wykonawcy używa tych samych przyczyn anulowania co materiał. `Completed` w dzienniku kończy zajęcie; nie dodajemy drugiego licznika ani konkurencyjnej historii zwolnień. Bilans początkowej ilości pozostaje sprawdzany, z zerowym zużyciem pojemności.

Wersja 2 dodaje na końcu każdego rekordu `RESOURCES` rodzaj: 0 dla materiału, 1 dla pojemności. Gdy nie są potrzebne dane wersji 3, serializer wybiera wersję 2 dla świata z pojemnością, a wersję 1 dla historycznego świata wyłącznie materiałowego. Czytnik obsługuje wszystkie trzy wersje. Wszystkie zasoby wersji 1 pozostają materiałami; nie zmieniamy ich historycznych kosztów ani nie odtwarzamy zużytej ilości. Nieobsługiwana wersja i rodzaj zasobu są odrzucane. [Raport pojemności](verification/reusable-capacity.md) obejmuje odczyt aktywnego zajęcia oraz semantycznie uszkodzoną wersję 2 z poprawną sumą kontrolną.

[Demo](../src/headless/work_demo.cpp) pokazuje dwóch wykonawców, ostatnie trzy jednostki materiału, odmowę drugiej rezerwacji, zapis w trakcie pracy i kontynuację. `--inspect` tylko odczytuje. Tekst podaje ID, fizyczną ilość, rezerwację, dostępność, fazę, zużycie, termin, przyczynę anulowania, wyniki komend i zatwierdzone zdarzenia.

Nowy rodzaj skutku, narzędzia, transport lub planer musi rozszerzyć rzeczywisty kontrakt i testy, bez dopisywania udanych rezultatów do demo. Ten etap dostarcza fundament czynności; nie dowodzi autonomicznej cywilizacji ani działania 3D.
