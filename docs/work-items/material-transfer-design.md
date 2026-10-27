# Przygotowanie transferu materiału dla dostawy

Status: historyczna propozycja techniczna sprzed implementacji. Bieżącą realizację i aktualne ograniczenia opisuje [material-delivery](material-delivery.md); poniższy opis stanu odnosi się do chwili przygotowania projektu. Nie jest alternatywnym planem etapów.

## Co rzeczywiście jest w kodzie

`WorkWorld` tworzy partię z niezmiennym właścicielem, ilością początkową, bieżącą ilością, stratą oraz rodzajem materiał/pojemność. Rezerwacja jest wyliczana z aktywnych czynności. Dla partii obowiązuje bilans początkowa = pozostała + zużyta + utracona. Nie ma transferu między partiami ani identyfikatora rodzaju materiału: żywność i materiał konserwacyjny są obecnie rozróżnione przez stanowisko konsumenta, nie typ zasobu domenowego.

Przeniesienie samej liczby między dowolnymi partiami pozwoliłoby zamieniać materiał konserwacyjny w żywność. Zmiana `initial` w celu zamknięcia bilansu fałszowałaby historię. Usunięcie źródłowej partii i utworzenie nowej udawałoby produkcję oraz zniszczenie. Nie wybieramy tych skrótów.

## Proponowane granice pierwszej implementacji

Silnik zna ogólny identyfikator rodzaju materiału, np. `MaterialTypeId`; nazwy żywności i surowców są danymi sceny. Historyczne partie pozostają typem nieokreślonym 0. Nowa scena dostawy jawnie nadaje zgodne typy źródłu, ładunkowi i odbiorcy; nie reinterpretujemy dawnych zapisów.

Transfer przenosi dodatnią ilość między dwiema różnymi, aktywnymi partiami materiału tego samego typu. Nie przenosi pojemności. Źródło musi mieć wolną ilość po odjęciu rezerwacji; cel musi pomieścić ilość bez przepełnienia. ID inicjatora i uczestników muszą być poprawne. Domena księguje skutek, a sesja ruchu sprawdza fizyczne dojście i możliwość pobrania/oddania. Własność, fizyczne posiadanie i legalność nie są jedną regułą — transfer domenowy nie ma ukrytego zakazu kradzieży ani prawa teleportowania zasobu w grze.

Proponowany interfejs: `transfer(TransferRequest)` z ID komendy, inicjatorem, partią źródłową, docelową i ilością; wynik rozróżnia przyjęcie, niepoprawne wejście, brak uczestnika, niedobór, niezgodny typ i przepełnienie. Przestrzeń ID komend jest wspólna z rezerwacją: ten sam identyfikator nie może znaczyć innej operacji. Identyczne ponowienie odtwarza pierwotny wynik, także odmowę; inny payload pod zajętym ID daje błąd przed zmianą świata.

Zatwierdzony transfer odejmuje i dodaje ilość w jednej transakcji. Historia zachowuje powiązaną parę zdarzeń odpływu i wpływu oraz rekord komendy. Pamięć i limity dziennika są sprawdzane przed zmianą obu stron. Nie dodajemy drugiego licznika żywności do `TraversalSession` lub renderera.

## Bilans i zapis wymagają rzeczywistej zmiany kontraktu

Walidator musi sprawdzać chronologiczny bilans z wpływami i odpływami, zużyciem oraz stratą. Nie wystarczy dodać niezależnych sum `uint64_t`: wielokrotny obieg tej samej dużej partii może przepełnić sumę przepływów mimo poprawnego aktualnego stanu. Odtwarzanie zmian ilości w kolejności dziennika unika tego fałszywego przepełnienia i pozwala odrzucić transfer sprzed utworzenia partii albo po jej usunięciu.

Partia może otrzymać więcej niż jej historyczne `initial`; po usunięciu jej rzeczywista strata może więc przekroczyć własną ilość początkową. Dzisiejszy warunek `lost <= initial` trzeba rozszerzyć dla materiału z przepływem, zachowując reguły pojemności. Nie można osłabić walidacji do sprawdzenia sumy globalnej: brak materiału w jednej partii i nadmiar w drugiej muszą być wykryte.

Propozycja nowego `GENESIS_WORK_WORLD 3`: typ materiału, historia transferów i powiązanie nowych zdarzeń. Czytnik zachowuje wersje 1/2; zapis bez nowych danych może zachować dotychczasowe bajty. Zewnętrzny format podróży wymaga przeglądu dopiero, gdy dodamy stan ładunku i dostawy. Nie zapisujemy dodatkowych pól „na zapas”.

## Testy wymagane przed uznaniem transferu za działający

- Źródło 10, cel 0, transfer 3: ilości 7 i 3, bez utworzenia materiału; normalne zużycie otrzymanej partii.
- Czynność rezerwuje 8 z 10: transfer 3 jest odmową, transfer 2 jest możliwy, zakończenie czynności nie narusza bilansu.
- Ponowienie przyjętej i odrzuconej komendy, kolizja ID z rezerwacją i inny payload pod tym samym ID.
- Różne rodzaje materiału, pojemność, identyczne źródło i cel, zero, brak uczestnika, przepełnienie celu — bez częściowej zmiany.
- Przekazanie, kolejne przekazanie, zużycie u odbiorcy, usunięcie partii po wpływie oraz usunięcie inicjatora po zakończeniu transferu.
- Wyczerpany limit historii, nieudana alokacja w obsługiwanym modelu błędów, obieg dużej ilości i kontrola obu stron transakcji.
- Nowy proces po zapisie, stare wersje 1/2 i semantyczna korupcja nowego rekordu z poprawną sumą kontrolną.

Ten kontrakt nie jest jeszcze niesieniem: późniejsza sesja musi umieścić faktyczny zasób przy wykonawcy, ograniczyć ładunek, rozstrzygnąć przerwanie i oddanie oraz udowodnić skutek niedostarczenia dla posiłków. Pierwsza implementacja tego dokumentu wymaga dokładnego planu API i aktualizacji właścicieli kontraktów razem z kodem.

Pomiar i kontynuacja opisane w [raporcie wejścia w M04](../verification/m04-entry-observability.md#co-zrobił-świat) pokazały realny koszt kolejki i narastanie pilnej potrzeby podczas aktywnego zadania. Plan dostawy musi określić, kiedy wykonawca odkłada niepilną pracę na rzecz jedzenia/odpoczynku i co dzieje się z niesionym zasobem oraz zobowiązaniem. Bezpiecznie rozpoczęte przejście/przejazd nadal poprzedza zmianę celu; nie można porzucać ładunku przez wyzerowanie pola zadania.
