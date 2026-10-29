# Kontrakty i niezmienniki subsystemów

**Status: normatywny w obsługiwanym zakresie i dla dalszej implementacji.** Konkretny zakres fundamentu czynności wskazuje [model domeny](work-domain.md). Zmiana wymaga jawnego zapisu, aktualizacji konsumentów i testów; nazwanie regresji emergencją nie zmienia kontraktu.

## Granica komendy

Komenda wskazuje identyfikator, źródło, wykonawcę, cel i oczekiwany kontekst lub wersję. Wynik odróżnia: przyjęcie, rozpoczęcie, wykonanie, odrzucenie, przerwanie i oczekiwanie. Nazwy konkretnych typów C++ ustali implementacja; poniższa semantyka jest wiążąca.

Rezerwacja nie jest jeszcze zużyciem. Wykonanie może obejmować wiele faz. Każda faza definiuje zmiany możliwe do cofnięcia i nieodwracalne koszty. Niedostępna droga może spowodować oczekiwanie lub zmianę planu, nigdy fikcyjne doręczenie.

### INV-01 — Trwała tożsamość

Osoba, zasób i miejsce zachowują ID między reprezentacjami i zapisami. Stare odwołanie do usuniętego slotu nie może wskazać nowego obiektu. Właściciel: rejestr tożsamości; konsumenci nie generują zastępczych biografii.

### INV-02 — Jeden właściciel pola

Każde mutowalne pole ma jeden autorytet. Przekazanie położenia pomiędzy fizyką a podróżą wyłącza starego wykonawcę i potwierdza nowego. Renderer i UI są konsumentami widoku.

Partia niesiona jest własnością `WorkWorld`; sesja przechowuje wyłącznie jej ID i fazę kursu. Każde stanowisko wskazuje odrębną partię. Przyjęty transfer księguje obie strony w jednej operacji, z uwzględnieniem rezerwacji i typu materiału; interakcja przestrzenna sprawdza dostęp na miejscu. Szczegóły i granice obecnego przyrostu: [material-delivery](work-items/material-delivery.md).

### INV-03 — Wspólna czynność gracza i NPC

To samo działanie używa tych samych wymagań, kosztów i skutków. Ewentualna różnica sterowania lub czasu ma jawne uzasadnienie; nie może być ukrytym obejściem gospodarki.

### INV-04 — Dostępność i rezerwacja

Zarezerwowana ilość nie jest dostępna drugiemu wykonawcy. Definicje: `ilość_fizyczna >= 0`, `0 <= zarezerwowane <= ilość_fizyczna`, `wolne = ilość_fizyczna - zarezerwowane`. Nowa rezerwacja korzysta z `wolne`. Rezerwacja całej partii jest poprawna i pozostawia zero wolnych jednostek. Wygaszenie, przerwanie lub śmierć wykonawcy zwalnia właściwe rezerwacje, bez odtwarzania już zużytego materiału.

W zaimplementowanym rozróżnieniu materiał/pojemność rodzaj zasobu jest niezmienny. Pojemność pozostaje zajęta przez całą aktywną czynność i nie może być zużywana. Ukończenie lub anulowanie zwalnia zajęcie bez tworzenia zasobu; usunięcie obiektu przerywa aktywne zajęcia. Te same reguły obowiązują gracza i NPC. Jedna obecna czynność korzysta z jednego zasobu, nie z wielu jednoczesnych wymagań.

### INV-05 — Skutek najwyżej raz

Ponowienie komendy, wczytanie i wznowienie nie powodują drugiej wypłaty, naprawy ani zużycia. Jeśli komenda dopuszcza wiele częściowych skutków, każda faza ma własną tożsamość i status wykonania.

### INV-06 — Bilans zasobów

Transfer zachowuje ilość oraz jednoznaczną własność i położenie. Produkcja, zużycie oraz strata mają jawne wejścia, wyjścia i przyczynę. Nie wymagamy zachowania liczby przedmiotów podczas zdefiniowanej konwersji. Niedobór nie jest naprawiany przez niewidoczne tworzenie zasobu.

### INV-07 — Fizyczna możliwość i prawo

Techniczne uprawnienie do wykonania komendy, fizyczna możliwość i legalność w fikcyjnym świecie są odrębnymi ocenami. Kradzież może się wydarzyć, a jej ujawnienie zależy od dowodów i kanałów wiedzy. Ograniczony [model sesji](traversal-autonomy.md#świadek-własność-i-legalna-obsługa) wykonuje ten sam transfer cargo mimo braku autoryzacji; legalna obsługa zależy wyłącznie od świadectw w pamięci opiekuna. Fizyczny zwrot i wiedza o uregulowaniu są odrębne.

### INV-08 — Prawda i przekonania

NPC korzysta z obserwacji i otrzymanych informacji. Przekonanie ma źródło i czas; może być błędne. Dostęp do inspektora lub globalnego dziennika nie jest kanałem wiedzy postaci. Właściciel: model wiedzy, nie renderer ani narrator.

W ograniczonym torze `TraversalSession` jest właścicielem opcjonalnych przekonań o stanowiskach. Własna obserwacja wymaga fizycznego dostępu; kierunkowy przekaz wymaga bliskości, wspólnej podpory i obustronnej kontroli geometrii. `source_actor` pozostaje trwałym ID pierwotnego świadka, `via_actor` wskazuje ostatniego rozmówcę, a `observed_tick` i `learned_tick` rozdzielają wiek informacji od czasu otrzymania. Przekaz zastępuje wyłącznie ściśle starszą obserwację, nie czyta zapasu i nie odmładza horyzontu niepewności. Prawda może zmienić się zdalnie bez zmiany przekonania; rezerwacja na miejscu nadal rozstrzyga rzeczywistą możliwość. Model, odmowy, granice bezpiecznego replanningu i format 5 określa [autonomia](traversal-autonomy.md#lokalna-wiedza-i-przekaz).

### INV-09 — Kolizja i przestrzeń przejść

Nawigacja i fizyka odnoszą się do zgodnej definicji przeszkód w obsługiwanej geometrii. Trasa uwzględnia rozmiar, wysokość i połączenia wykonawcy. Ruch nadal przechodzi kontrolę kolizji; wygładzanie ścieżki nie może przecinać przeszkody.

### INV-10 — Aktualność planu

Wynik planowania zawiera kontekst potrzebny do sprawdzenia aktualności. Zmiana zasobu, geometrii lub wykonawcy unieważnia zależny plan przed niedopuszczalnym skutkiem. Spóźniona odpowiedź wyszukiwania lub modelu nie jest automatycznie wykonywana.

### INV-11 — Zegar i budżet pracy

Kroki fizyki, terminy świata i czas rzeczywisty nie są mieszane bez jawnego przeliczenia. Brak budżetu daje opóźnienie lub określony brak rozwiązania. Nie usuwa po cichu czasu symulacji, długu, dostawy ani szkody.

### INV-12 — Spójna kontynuacja

Po wczytaniu wracają tożsamości, ilości, własność, wiedza, zobowiązania, stan reprezentacji i działania w toku zgodnie z wersją formatu. Błąd zapisu lub odczytu nie może nadpisać ostatniego poprawnego świata niezauważonym resetem.

### INV-13 — Migracja reprezentacji

Przejście lokalny/daleki zachowuje chronione własności: ID, zasoby, rezerwacje, istotny stan działania i położenie zgodne z modelem podróży. Powrót nie losuje nowego ładunku. Różnice modeli przybliżonych wymagają osobnej oceny, nie bitowej identyczności wszystkich pozycji.

### INV-14 — Stan konstrukcji

Zmiana konstrukcji musi powiadomić zależne kolizje, przejścia, instalacje, plany i prezentację. Zmiana flagi przejezdności nie jest dowodem symulacji fizycznego rozpadu. Odbudowa jawnie zużywa pracę i materiał.

Ograniczony kontrakt `TraversalSession` łączy `PassageState.open` z opcjonalnym stanowiskiem Maintain i typem materiału 2. Zmiana zwiększa wersję geometrii/topologii, zwalnia nieprzydzieloną kolejkę, zmienia widok i fizyczną wykonalność ruchu. Odmowa na osobie, rozpoczętym Step lub związanym clearance nie zmienia sesji. Przy zużyciu 1000 próba jest ponawiana do bezpiecznej granicy. Otwarcie po naprawie wymaga ukończonej czynności WorkWorld i zużytego materiału, a nie samej wiadomości. NPC planuje według lokalnej pamięci; rzeczywista blokada jest sprawdzana niezależnie od przekonań. API, bezpieczne ponowienia i format 6 określa [model autonomii](traversal-autonomy.md#przejście-konstrukcja-i-lokalna-wiedza).

### INV-15 — Historia zgodna ze światem

Scena nie wymaga rozmowy z martwą osobą ani przedmiotu, którego nie da się uzyskać, bez określonego rozwiązania lub zakończenia. Nie tworzy dowodów i nie wskrzesza uczestników, by przywrócić założony przebieg.

### INV-16 — Dowód niezależny od prezentacji

Zaliczenie logistyki wymaga sprawdzenia rzeczywistej dostawy i zużycia; zaliczenie fizyki wymaga wyniku ruchu i kontaktu. Animacja, etykieta roli, dialog lub log „sukces” nie zastępują skutku. Wspólny runner i gra mają te same reguły domenowe, a odmienne modele fizyki muszą być oznaczone.

## Rozszerzanie kontraktów

Nowy subsystem zapisuje: posiadane dane, komendy wejściowe, widoki wyjściowe, niezmienniki, odmowy, przerwania, wersjonowanie oraz testy konsumentów. Nie trzeba projektować pełnego API wszystkich dalszych funkcji, zanim powstanie pierwszy przypadek użycia.


Aktualny kontrakt sesji przestrzennej obejmuje odłożone zamiary i bezpieczne granice przerwania opisane w [modelu autonomii](traversal-autonomy.md). Stan należy do sesji, koszty i skutki nadal do WorkWorld. Odczytowy `navigation_view` i diagnostyka nie przydzielają kolejki ani nie podejmują decyzji. Nowy stan wymaga zewnętrznej wersji 2; wersja 1 pozostaje czytelna bez reinterpretacji.


Lokalne ustąpienie przy rozpoczętym Step jest jawnym protokołem sesji według [modelu autonomii](traversal-autonomy.md#lokalna-prośba-o-przejście). Zgoda i manewr mają ścisłe powiązanie; sterowanie ręczne oraz rozpoczęte czynności nie są przejmowane. Fizyczny koniec uzgodnionego kroku zamyka zgodę przed zapisem. Rezerwacje miejsc manewrów nie są zasobami materiałowymi WorkWorld i nie przenoszą gospodarki do nawigacji. Ich lokalny widok jest jedyną dodatkową wiedzą protokołu.

Opcjonalny stan praktyk należy wyłącznie do TraversalSession. Aparat rozlicza energię i pomiary; umysły korzystają z lokalnych obserwacji, nie prawa sceny. WorkWorld zachowuje części, czas i skutek pracy. Ręczny i autonomiczny Maintain przechodzą wspólną bramkę jednorazowej kalibracji. Kontakt przenosi pomiary świadka z czasami, a odczyt formatu8 rekonstruuje stan oraz koszty. Pełny kontrakt i ograniczenia znajdują się w [modelu autonomii](traversal-autonomy.md#integracja-praktyk).

Opcjonalna podróż daleka w sesji realizuje rozłączność INV-02/INV-13: jeden rekord osoby i partii, jedna reprezentacja ruchu. Lokalni konsumenci wymagają `TraversalActor::local()`. Historyczna kotwica nie stanowi kapsuły ani źródła obserwacji. Model ograniczonego czasu, koszt, bezpieczne przekazanie i transakcyjny format 9 opisuje [model autonomii](traversal-autonomy.md#podróż-poza-lokalną-reprezentacją). Nie zmienia to kontraktu WorkWorld ani formatu światów bez nowego stanu.

Opcjonalna historia zaopatrzenia posiada wyłącznie role po ID i postęp. [Kontrakt historii](story-scene.md) określa lokalne żądanie, fizyczny wynik, ograniczone przygotowanie świata i otoczkę10. Historyczne wiązanie usuniętej osoby lub miejsca nie odtwarza go w świecie; odmowa i późniejszy zwrot materiału pozostają rozdzielone.
