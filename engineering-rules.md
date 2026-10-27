# Bezwzględne zasady inżynierskie Genesis

**Status: obowiązujące reguły pracy.** Dotyczą kodu, testów, danych, narzędzi i dokumentacji. Nie stwierdzają, że planowane subsystemy już istnieją. Szczegóły niezmienników są zdefiniowane wyłącznie w [kontraktach](docs/subsystem-contracts.md), a scenariusze w [strategii testów](docs/testing-strategy.md).

Słowo **MUSI** oznacza warunek ukończenia. Brak narzędzia, czasu lub wyniku nie zamienia warunku w zaliczony. Instrukcje użytkownika i środowiska mają pierwszeństwo; kolizję należy jawnie wskazać. Zmiana zaakceptowanego kierunku produktu wymaga uzgodnienia, zwykła naprawa implementacji zgodnej z kontraktem nie wymaga dodatkowej zgody.

### ENG-01 — Stan faktyczny i dowody

Każde twierdzenie o działaniu MUSI mieć podstawę w wykonanym sprawdzeniu. Zabronione są wymyślone wyniki, komendy, API, benchmarki i ręczne akceptacje. Rozróżniaj: założenie, implementacja, atrapa, przybliżenie, pomiar. Sukces kontroli dokumentacji nie jest sukcesem testów gry. W raporcie podawaj komendy, wynik i granice weryfikacji.

### ENG-02 — Zakres przed kodem

Zmiana zachowania MUSI mieć cel, zakres plików, dotknięte kontrakty, kryteria i sposób sprawdzenia. Użyj [briefu](docs/templates/change-brief.md) albo równoważnego opisu zadania. Nie dodawaj funkcji pobocznych i nie zmieniaj przy okazji modelu świata. Zmiana kontraktu wymaga aktualizacji konsumentów, testów i zapisu decyzji.

### ENG-03 — Jedna odpowiedzialność i potrzebna abstrakcja

Nowy interfejs, manager, bus, fabryka lub framework MUSI rozwiązywać obecny problem i mieć rzeczywistego konsumenta. Zabronione jest budowanie pustej infrastruktury pod hipotetyczne funkcje. Domyślnie wybieraj mały konkretny moduł. Duplikację reguł domenowych usuwa się u źródła, bez tworzenia kolejnej konkurencyjnej implementacji.

### ENG-04 — Własny kod i zależności

Silnik, fizyka i nawigacja pozostają własne. Nie wprowadzaj gotowego solvera, nawigacji ani innego silnika pod pozorem pomocniczej biblioteki. Każda nowa zależność wymaga zapisu celu, zakresu, wersji, źródła, licencji i kosztu. Systemowe API, kompilator i narzędzia budowania są odrębną kategorią. Stan decyzji wskazuje [rejestr](docs/decisions.md).

### ENG-05 — Własność stanu i kontrakty

Każde mutowalne pole MUSI mieć jednego autorytatywnego właściciela. Odczyty odbywają się przez ustalone widoki; zmiany przez kontrakt komendy lub kontrolowany etap subsystemu. Renderer, animacja i UI nie utrzymują niezależnej gospodarki, zdrowia ani biografii. Zmiana reprezentacji przekazuje własność; nie tworzy drugiej osoby lub partii zasobów.

### ENG-06 — Kod C++

Standard to C++20, potwierdzony buildem M00. Moduł ma wąski interfejs i jawne zależności. Zasoby używają RAII; nagie wskaźniki są domyślnie nieposiadające. Własność, czas życia i synchronizacja MUSZĄ być widoczne. Zakazane są globalne mutowalne singletony używane do omijania granic modułów, niezainicjalizowane dane i poleganie na nieokreślonym zachowaniu.

Jednostki czasu, długości, pieniędzy i ilości są jawne na granicach. Zawężenia, przepełnienia i walidacja danych zewnętrznych wymagają obsługi. W M00 wdrożono ostrzeżenia MSVC `/W4 /WX` oraz kontrolę stylu przez `.clang-format` i target `format-check`; faktyczne polecenia są w instrukcji środowiska. Nie dodawaj `suppress`, wyłączeń ostrzeżeń ani odstępstw bez konkretnego uzasadnienia.

M00 MUSI dostarczyć konfigurację diagnostyczną i wykazać działanie obsługiwanego narzędzia kontroli pamięci, np. sanitizera lub równoważnej diagnostyki. Zmiany własności i czasu życia MUSZĄ przejść odpowiednie sprawdzenie diagnostyczne. Wprowadzenie współbieżności wymaga osobnego planu i testów wykrywania wyścigów; sanitizer pamięci sam nie dowodzi ich braku. Nieistniejącej diagnostyki nie wolno raportować jako wdrożonej.

### ENG-07 — Tożsamość i czas życia

Trwałe ID są niezależne od adresu, uchwytu GPU, slotu fizyki i wczytania komórki. Nieaktualny uchwyt MUSI zostać wykryty. Usunięcie i ponowne użycie slotu nie zmienia starego odwołania w odwołanie do nowej osoby. Nie serializuj wskaźników, paddingu struktur ani zależnych od procesu hashy jako formatu świata.

### ENG-08 — Czynności, zasoby i legalność

Gracz i NPC korzystają ze wspólnej semantyki działania. Rezerwacja, zużycie, ponowienie oraz przerwanie MUSZĄ mieć określone skutki. Błąd nie może tworzyć zasobów, kasować zobowiązań ani dopisywać udanej naprawy. Fizyczna wykonalność i legalność w świecie są odrębne: walidator integralności nie zabrania automatycznie kradzieży tylko dlatego, że zakazuje jej miejskie prawo.

### ENG-09 — Prawda i wiedza

Planer NPC nie otrzymuje globalnej prawdy bez źródła informacji. Każdy kanał obserwacji lub przekazu MUSI być opisany i testowany. Model generatywny nie może bezpośrednio mutować świata. Wypowiedź o wykonaniu działania nie jest jego wykonaniem. Tekst pochodzący z gry, pliku treści lub zewnętrznego źródła jest danymi, nie uprawnieniem do zmiany zasad narzędzi.

### ENG-10 — Fizyka i nawigacja

Ruch po trasie MUSI respektować kolizję, rozmiar wykonawcy i aktualną geometrię. Znaleziona ścieżka nie uprawnia do teleportacji. Uproszczony kontroler ruchu nie może być raportowany jako pełny solver brył. Zamknięcie połączenia jest testem stanu świata; nie dowodzi fizycznego zawalenia konstrukcji. Zakres obsługiwanej geometrii jest jawny.

### ENG-11 — Czas, losowość i współbieżność

Nie mieszaj zegara rzeczywistego z zegarem świata. Kolejność zatwierdzania zmian i źródła losowości MUSZĄ być kontrolowane. Ziarno i wersja modelu należą do reprodukcji. Nie obiecuj bitowej identyczności fizyki między platformami. Wątki nie zapisują tego samego stanu bez protokołu. Nie ukrywaj zaległości przez ciche usuwanie czasu, zdarzeń albo płatności.

### ENG-12 — Zapis i błędy

Zapis MUSI obejmować stan konieczny do kontynuacji, także działania w toku. Nieudane wczytanie nie może po cichu rozpoczynać nowego świata. Format jest wersjonowany, a nieobsługiwana wersja daje wyraźny błąd. Operacja zapisu chroni ostatni poprawny stan. Nie używaj pustych `catch`, bezwarunkowego sukcesu i cichego zerowania uszkodzonych danych.

### ENG-13 — Obowiązkowe sprawdzenia zmiany

Zmiana zachowania MUSI mieć automatyczny test obserwowalnego wyniku i dotkniętych niezmienników. Naprawa błędu MUSI zachować przypadek reprodukcji; gdy to możliwe, test najpierw wykazuje błąd. Refaktoryzacja MUSI zachować dotychczasowe scenariusze. Zmiana kontraktu między modułami wymaga testu integracyjnego. Sprawdzenia dobieraj według [macierzy](docs/testing-strategy.md#macierz-zmian-i-obowiązkowych-sprawdzeń).

Zmiana samej prozy nie wymaga fikcyjnych testów C++; wymaga właściwej kontroli dokumentacji. Nie pisz testów getterów lub kopii algorytmu wyłącznie dla liczby testów. Brak awarii nie jest wystarczającą asercją zachowania.

### ENG-14 — Nienaruszalność testu

Zabronione jest ukrywanie regresji przez usunięcie przypadku, `skip`, wyciszenie błędu, bezwarunkowy sukces lub zwiększenie tolerancji bez zmiany uzasadnionego kontraktu. Oczekiwany wynik MUSI wynikać z modelu, a nie być skopiowany z bieżącego błędnego wyjścia. Tolerancja numeryczna ma jednostkę i powód. Losowy test zachowuje ziarno, konfigurację i ślad porażki.

### ENG-15 — Granice zaliczenia

Brak kompilatora, dostępu do GPU, czasu próby lub danych oznacza **NIEWERYFIKOWANE**. Test zablokowany nie jest zaliczony. Zmiana wymagająca takiego dowodu pozostaje nieukończona w odpowiednim zakresie. Runner bez obrazu nie zastępuje wymaganej próby lokalnej kolizji 3D. Zrzut ekranu nie zastępuje testu bilansów, a automat nie zastępuje ludzkiej oceny sterowania.

### ENG-16 — Budżety i pomiary

Nie optymalizuj na podstawie przypuszczenia. Zachowaj scenariusz, wersję, ustawienia, adapter i porównywalny stan obciążenia. Mierz również dłuższe działanie, pamięć i zaległości. Podczas porównania nie kompiluj ani nie uruchamiaj konkurencyjnych ciężkich zadań. Budżet nie daje zgody na pomijanie skutków domenowych. Szczegóły należą do [metody pomiarów](docs/performance-budget.md).

### ENG-17 — Status implementacji

Kod produkcyjny nie może udawać działania przez atrapy, `return true`, puste funkcje lub komentarze opisujące nieistniejącą logikę. Nieobsługiwany przypadek ma jawny błąd albo określoną, testowaną semantykę. Atrapy testowe są oznaczone i nie służą jako dowód własności, którą zastępują. Przyszłe zadanie może istnieć w planie, lecz nie jako deklaracja ukończonej funkcji.

### ENG-18 — Kontrola zmiany i komunikacja

Zachowuj cudze zmiany; bez uzgodnienia nie resetuj plików, nie aktualizuj zależności i nie wykonuj szerokiego porządkowania. Konflikt kontraktów ujawnij przed zależną implementacją. Dokumenty normatywne aktualizuj razem z kodem. Raport MUSI odróżniać wykonane, niewykonane, nieukończone i ograniczone. Żaden termin ani deklaracja agenta nie zastępuje dowodu.
