# Odkrywanie i przekazywanie praktyk w prototypie

**Status: zakres techniczny ukończony i zweryfikowany; bezpośredni odbiór użytkownika niewykonany.** Użytkownik ustalił 28.10.2026 kolejność: wykorzystać eksperymenty w prototypie, ukończyć M05 i M06, następnie osobno zaprojektować przejście do pełnego 3D. Kolejność obowiązuje w [jednym planie](../prototype-plan.md); szczegóły poniżej są specyfikacją przyrostu, a rzeczywisty stan i ograniczenia podaje [raport integracji](../verification/practice-in-prototype.md). Wyniki badań pozostają w [raporcie](../verification/discriminating-experiment.md), bez zmiany ich kryteriów ani przypisywania im odbioru gry.

## Co użytkownik ma zobaczyć

W tej samej społeczności mieszkaniec ma rzeczywistą pracę do wykonania, ale nie zna potrzebnego ustawienia urządzenia. Podchodzi, próbuje, zapamiętuje wyniki i dobiera skuteczną konfigurację. Poprawny wynik pozwala rozpocząć konserwację, zużyć część i ukończyć pracę. Druga osoba zdobywa wiedzę przy spotkaniu i później potrafi użyć jej samodzielnie.

Gracz może to obserwować, podejść do urządzenia, dostarczyć części, użyć tych samych czynności albo przeszkodzić przez istniejące działania. Po odejściu, powrocie i zapisie postęp nadal istnieje. Zwykły widok pokazuje próby, pracę i skutki; nie wymaga czytania raportu naukowego ani konsoli.

Pierwszy przyrost kończy się na jednym typie urządzenia i jednym rodzaju pracy. Nie dodajemy zarazem magii, życia od podstaw, nowych rodzin praw, walki ani większej populacji. M00–M04 pozostają odebrane w dotychczasowym zakresie. Nie wymuszamy przewagi nauczania tam, gdzie samodzielne próby wystarczają: negatywny wynik T nadal obowiązuje.

## Podejścia i wybór autora

| Podejście | Korzyść i koszt | Rozstrzygnięcie |
| --- | --- | --- |
| Osobne okno dotychczasowego eksperymentu | Szybszy pokaz, lecz odrębny świat dwóch osób | Nie realizuje integracji z prototypem |
| Jedno urządzenie we wspólnej społeczności | Łączy uczenie z potrzebami, materiałem, ruchem i ingerencją gracza | Rekomendowane do pierwszego wdrożenia |
| Od razu dowolne wynalazki wszystkich NPC | Duży zakres bez sprawdzonej czytelności i kosztu | Dalszy kierunek, poza tym przyrostem |

Propozycja organizacji: wariant tej samej aplikacji `reality_traversal`, np. `--community --learning`, i prosty launcher do próby użytkownika. To przyszłe polecenie, nie istniejąca komenda. Wariant kontrolny starego `--community` zachowuje reprodukcję M04. Docelowym wynikiem jest wspólna rozgrywka, nie kolejny samodzielny program badawczy. Po odbiorze nowy wariant może stać się zalecanym uruchomieniem prototypu.

## Zakres wspólnej sceny

Zachować obecną społeczność ośmiu osób, potrzeby, żywność, transport, przejście i prawo. Przy istniejącym stanowisku konserwacji dodać jedno urządzenie; na początek dwoje mieszkańców korzysta z nowej zdolności. Nazwy, wygląd i uzasadnienie urządzenia pozostają robocze. Abstrakcyjny przepływ energii z badania nie ustanawia systemu magii świata.

Praca ma lokalnie widoczny wymóg poprawnego wyjścia urządzenia i fizycznej części. Wymóg pracy jest dany światu; skuteczne ustawienie mieszkaniec poznaje z pomiarów. Gracz i NPC podlegają tej samej wykonalności, kosztom oraz skutkom. Kalibracja nie zużywa części konserwacyjnej; jej zużycie następuje w istniejącej czynności. Każdy impuls osobno zużywa istniejący zapas energii urządzenia. Nie mylimy części z energią ani nie tworzymy zapasu awaryjnego, żeby utrzymać pokaz.

Pierwszy przypadek używa praw już obsługiwanych przez aparat. Przykład dydaktyczny może pokazać zadanie 16→8 i dwukrotne przejście przez nieznany element; odpowiedzi nie wpisujemy do planera. Testy muszą obejmować także inne obsługiwane prawa, aby wykluczyć sztywną receptę dopasowaną do sceny.

Udana konserwacja ma rzeczywisty wpływ na dotychczasowe stanowisko/przejście. Przed implementacją trzeba wskazać dokładne połączenie urządzenia ze stanowiskiem i stanem przejścia; sprawdzenie poprawnej kalibracji musi leżeć w domenowym kontrakcie rozpoczęcia tej pracy, nie w przycisku okna. Dawne stanowiska bez urządzenia zachowują dotychczasowe reguły.

Proponowane doprecyzowanie pierwszej sceny: urządzenie jest przypisane do stanowiska konserwacji obecnego przejścia, a udana praca korzysta z dotychczasowego skutku jego naprawy. Zgodny impuls pozwala temu samemu wykonawcy rozpocząć jedną taką czynność; nie daje globalnej, nieskończonej zgody na naprawę innym osobom. Ten związek zużywa się przy rzeczywistym przyjęciu pracy i musi być zachowany w zapisie. Jednocześnie urządzenie obsługuje jedną operację, z jawnym Busy/oczekiwaniem, bez nakładania operatorów.

Prawo urządzenia w pierwszym przyroście nie zmienia się w trakcie świata. Wiedzę przypisujemy do trwałego ID urządzenia i jego wersji, a role mieszkańców do EntityId, nie stałych indeksów 0/1 w tablicy osób. Zapobiega to użyciu obserwacji innego aparatu lub przypisaniu pamięci innemu mieszkańcowi po wczytaniu.

Scena ma pozostawić uczniowi rzeczywistą potrzebną pracę: początkowe zużycie może wymagać więcej niż jednej istniejącej czynności konserwacji. Otwarcie przejścia po pierwszej czynności nie oznacza automatycznie usunięcia całego zużycia. Plan implementacji musi sprawdzić rozdział dalszej pracy i konkurencję operatorów; nie wolno dopisać drugiej awarii ani zużywać części bez potrzeby tylko po to, aby pokazać ucznia.

## Zachowanie mieszkańca

1. Rzeczywista potrzeba pracy uruchamia zamiar. Nie uruchamiamy odkrywania tylko dlatego, że upłynął czas scenariusza.
2. Mieszkaniec dociera do urządzenia i obserwuje wymaganie oraz dostępne możliwości na miejscu.
3. Jeśli wiedza nie wystarcza do uzasadnionego wyboru, dobiera doświadczenie rozdzielające pozostałe wyjaśnienia. Wynik bierze wyłącznie z ukończonego impulsu.
4. Gdy zgodne modele przewidują właściwy skutek dla zadania, może spróbować pracy. Nie musi poznać jedynej składni całego prawa. Samo trafienie jednego z wielu sprzecznych modeli nie upoważnia do ogłoszenia pewnej metody.
5. Rzeczywisty wynik urządzenia rozstrzyga rozpoczęcie pracy. Błędny wynik pozostaje porażką; nie zamieniamy go w zakończoną konserwację.
6. Przy lokalnym spotkaniu przekazuje rzeczywiście poznane pomiary. Drugi mieszkaniec wyciąga własne wnioski i później sam wybiera konfigurację.

To nowa polityka użytkowa do zweryfikowania. Badanie wykonywało zawsze osiem prób dla uczciwego porównania; gra nie musi zmuszać postaci do ośmiu prób po poznaniu wystarczającej metody. Sufity liczbowe badania mogą być początkowym ograniczeniem, lecz nie są dowodem jakości nowej polityki gry. Brak energii, części, drogi lub adekwatnego modelu daje jawne oczekiwanie, zmianę dopuszczalnego zamiaru albo niepowodzenie.

Potrzeby i chronione operacje pozostają ważniejsze od prezentacji uczenia. Praca, impuls, Step, przejazd i clearance kończą bezpieczną fazę przed zmianą zamiaru. Ręczne przejęcie przez gracza nie teleportuje materiału, nie usuwa obserwacji i nie kończy fikcyjnie operacji. Nauczający może odejść; uczeń nie otrzymuje wiadomości na odległość. Nie wymagamy, by uczeń zawsze przegrywał bez nauczyciela.

## Wspólny świat i odpowiedzialności

Nie można po prostu postawić badawczego `ContrastSession` obok świata gry: ma własną `TraversalSession`, stałe dwie osoby i ograniczoną historię badania. W produkcyjnym przyroście musi istnieć jeden właściciel bieżącej podróży, pracy, zasobów i zegara.

Proponowany konsument integracyjny korzysta z istniejących `ContrastAgent`, `DiscoveryWorld` i jednej `TraversalSession`. Aparat posiada prawa, energię i wyniki impulsów; umysły mieszkańców posiadają modele i poznane pomiary; WorkWorld posiada części oraz wykonywane czynności. Okno/renderer czytają wynik i przekazują komendy. Włączenie kalibracji wymaga objęcia kontraktem wszystkich dróg rozpoczęcia tej konkretnej konserwacji, w tym ręcznej i autonomicznej. Stare badawcze sesje i protokoły zachowują własne API oraz dowody.

Konstrukcja modeli trwała w badaniu zauważalny czas procesu. Nie uruchamiać pełnego przebiegu badawczego w każdej klatce. Dla pierwszej sceny można przygotować umysły przed rozpoczęciem aktywnej symulacji, a późniejsze decyzje wykonywać w ograniczonej kolejce. Jeśli pojedyncza decyzja przekracza budżet aktualizacji, potrzebne są wznawialne porcje pracy z zachowaniem ich stanu. Sposób i liczbowy limit wybiera plan implementacji po pomiarze; nie zakładamy, że wynik headless gwarantuje płynność okna. Trwały zapis nie może odziedziczyć badawczego horyzontu 10000 kroków jako końca życia społeczności.

## Widok i działania gracza

Widoczne: kto próbuje, kto pracuje, czy urządzenie zadziałało, czy część została użyta oraz czy wiadomość dotarła. Krótka historia ostatnich prób pozostaje do odczytu, aby zdarzenie trwające kilka kroków nie znikało, zanim gracz je zauważy. Nie zmieniamy niejawnie kroku 50 ms ani zaakceptowanego ruchu, żeby spowolnić pokaz.

Zwykłe komunikaty mają brzmieć np. „sprawdza ustawienie”, „to ustawienie nie wystarczyło”, „zna działający sposób”, „czeka na część”. Wzory, seedy, liczby wszystkich modeli i prawda aparatu należą do wyraźnej diagnostyki. Panel gracza nie udostępnia zdalnych zapasów ani wiedzy mieszkańców, do której postać nie ma źródła.

Przygotować istniejące wejście gracza do lokalnego użycia urządzenia oraz przekazu, z czytelną odmową poza zasięgiem. Szczegółowy układ kontrolek i klawisze są zadaniem projektu interfejsu podczas implementacji; nie rezerwujemy dziś nowych skrótów kolidujących z F2/F4/F7/F9. Gra musi pozostać używalna po wczytaniu innej poprawnej sceny bez aparatu.

## Trwałość

Zapis obejmuje świat, działanie urządzenia w toku, oba umysły, źródła i czasy, części, pracę, zamiary i ewentualny przerwany etap wnioskowania. Potrzebna jest nowa wersjonowana otoczka integracyjna albo jawne rozszerzenie zapisu — szczegół wybiera plan implementacji po przeglądzie konsumentów. Nie można zapisać samej starej podróży i po F9 odtworzyć wiedzy z prawdy świata.

Odczyt historycznej podróży nie wymyśla pomiarów. Nowa scena otrzymuje stan urządzenia ze swojej definicji; historyczny zapis bez urządzenia nie musi go dostać. Pochodny widok bezpiecznie obsługuje obie możliwości. Odtworzenie kandydata i jego walidacja poprzedzają zastąpienie bieżącego świata. Działania po zapisie nie mogą ponownie zużyć części ani wykonać tego samego skutku drugi raz.

## Kolejność wykonania przyrostu

| Krok | Co ma powstać | Dowód ukończenia |
| --- | --- | --- |
| 1. Jeden konsument świata i zapis | Aparat oraz wiedza w tej samej sesji co osiem osób; domenowy warunek kalibracji; nowa otoczka i historyczny odczyt | Runner tego samego konsumenta, bilanse, brak podwójnych osób, odmowa bez energii/części i kontynuacja impulsu/pracy |
| 2. Mieszkaniec uczy się, bo potrzebuje wykonać pracę | Zamiar, dojście, próby, zastosowanie metody i bezpieczne odłożenie zamiaru przy innych potrzebach | Bez ręcznych poleceń NPC wykonuje pracę na kilku różnych prawach; awarie nie tworzą sukcesu |
| 3. Drugi mieszkaniec przejmuje wiedzę | Kontakt i raw pomiary z pochodzeniem, samodzielna późniejsza praca drugiej osoby | Zablokowany kontakt nie przekazuje nic; odejście nauczyciela nie kasuje nauki; bez wiadomości nie wymuszamy porażki |
| 4. Gracz widzi i może uczestniczyć | Urządzenie w dotychczasowym oknie, lokalne czynności, komunikaty i trwały ślad prób | Rzeczywiste wejście/obraz na GPU; poprawne F9, przejęcie sterowania i brak zablokowania interfejsu |
| 5. Wspólna próba i odbiór użytkownika | Spokojna społeczność, ingerencja, powrót i kontynuacja w tym samym świecie | Pełne regresje, kontrolowany pomiar aktywnego uczenia i bezpośrednia próba użytkownika |

Przed kodem powstaje plan implementacji z dokładnymi API, plikami, testami i wybranym formatem. Naturalne obszary zmiany to `src/research/contrast_agent.*`, nowy konsument w `src/lab/`, tworzenie sceny społeczności, `traversal_main.cpp`, `traversal_scene.cpp`, zapis i testy. Nie jest to zgoda na szeroki refaktor wszystkich wcześniejszych laboratoriów. Ten dokument jest projektem zakresu, nie deklaracją istnienia nowego API.

## Kiedy przyrost jest skończony

- [ ] Użytkownik uruchamia okienkowy prototyp i obserwuje jedną społeczność z nowym urządzeniem.
- [ ] Pierwszy mieszkaniec sam rozpoczyna potrzebne próby, poznaje metodę i kończy rzeczywistą konserwację, z energią i częścią rozliczonymi w odpowiednich domenach.
- [ ] Drugi mieszkaniec może zdobyć pomiary podczas spotkania i później sam wykonać pracę; nie ma dostępu do prywatnej prawdy ani odpowiedzi nauczyciela.
- [ ] Gracz może wpłynąć na sytuację, a zwykły widok pozwala zrozumieć skutek bez czytania wzorów.
- [ ] Głód, odpoczynek, transport, blokada, brak zasobu i ręczne sterowanie nie niszczą rozpoczętych operacji.
- [ ] Zapis/wczytanie w trakcie impulsu, dojścia, przekazu i pracy zachowuje wynik; historyczny lub inny poprawny świat działa bez pozornej wiedzy.
- [ ] Regresje starego prototypu i laboratoriów przechodzą, pomiar okna obejmuje rzeczywiste uczenie, a użytkownik osobno ocenia czytelność i sterowanie.

To bramka wejścia w dalsze M05. Nie jest nowym odbiorem M04 ani automatycznym zaliczeniem M05/M06. Następna praca badawcza wynika z ujawnionego problemu gry; nie zastępuje ukończenia tego przyrostu kolejnym osobnym laboratorium.

## Sprawdzenie projektu

Świeży przegląd wykonawczy nie wskazał sprzeczności ani brakującej decyzji produktowej blokującej następny plan implementacji. Pozostawione jawnie wybory inżynierskie to konkretne API bramki kalibracji, format zapisu, budżet wykonywania wnioskowania oraz szczegóły sterowania i sceny. Osobne briefy migracji M05 i próby M06 doprecyzują ich własne kontrakty przed kodem. To sprawdzenie dokumentu, nie review przyszłej implementacji ani odbiór użytkownika.
