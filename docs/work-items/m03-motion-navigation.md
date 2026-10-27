# Brief ruchu, kolizji i nawigacji — etap M03

Status: implementacja, odbiór techniczny i ocena użytkownika wykonane; M03 zaliczony w obsługiwanym zakresie z uwagami rozwojowymi dotyczącymi reakcji na cel, płynności, szybkości i naturalności ruchu. [Raport](../verification/m03-motion-navigation.md#ocena-użytkownika) zawiera wyniki, dowody i granice akceptacji. Zależności M01 i M02 są zaliczone. Kontrakty: INV-01, INV-02, INV-03, INV-09, INV-10, INV-11, INV-12 i INV-16. Odbiór: SC-06, SC-07, ruch i wejście z SC-11, kontynuacja z SC-08 w obsługiwanym zakresie oraz regresje wcześniejszych etapów.

## Zakres przed implementacją

Własny kontroler kinematyczny na płaskim podłożu y=0, prostokątny obrys osiowo ustawionej postaci, jawna wysokość, nieruchome bryły osiowo ustawione i ograniczenia obszaru. Ciągły sweep punktu przeciw bryle rozszerzonej o obrys wykonawcy zapobiega przeskakiwaniu przez cienkie przeszkody. Do czterech kontaktów na ruch, zatrzymanie i ślizg; jawny wynik po wyczerpaniu budżetu. Początkowe nałożenie lub pozycja poza obszarem jest odrzucana, bez automatycznego przenoszenia. Kontakt ma margines 0,000001 m; testy pozycji używają tolerancji 0,000002 m. Wartości niefinitywne i niedozwolone rozmiary są błędem.

To ograniczona fizyka 2,5D, prezentowana w 3D. Bez grawitacji, skoków, stoków, pięter, rotujących brył, pędów, tarcia, więzów i fizycznego zawalenia. Nawigacja uwzględnia obrys oraz wysokość względem przeszkód i niskich prześwitów. Inne rodzaje geometrii nie są przyjmowane przez typy tego kontraktu; nie udajemy pełnego solvera ani navmesha.

Nawigacja: własne A* na siatce czterech sąsiadów, jednakowy koszt krawędzi, heurystyka Manhattan, deterministyczne rozstrzyganie remisów. Siatka do 64×64, jawny budżet rozwinięć i oddzielne wyniki: trasa, brak drogi, niewłaściwy punkt końcowy, brak budżetu. Komórki i połączenia są sprawdzane tą samą geometrią co ruch, bez drugiej listy przeszkód. Siatka jest próbkowaniem przestrzeni; nie obiecujemy kompletności wyszukiwania każdej ciągłej szczeliny pomiędzy próbkami.

Wynik zawiera wersję geometrii, rozmiar wykonawcy i początek. Przyjęcie odrzuca stare wyniki i wynik dla innej pozycji/rozmiaru. Ruch po trasie nadal przechodzi sweep kolizji. Zmiana geometrii wymaga ponownego planowania przed następnym ruchem. Zamknięcie przejścia na wykonawcy daje odmowę bez zmiany wersji, bez teleportacji. Dwie postacie korzystają z tego samego kontrolera; kolizja lokalna respektuje także drugą postać. Nie implementujemy jeszcze kooperacyjnego planowania tłumu.

## Konsument

Tryb `reality_lab --motion` zachowuje dotychczasową scenę M02 jako domyślny tryb i dodaje małą scenę testową M03. Dwa wykonawcy zachowują ID istniejącego świata M01. Kontroler ruchu jest jedynym właścicielem położeń, celów i tras; domena M01 nadal posiada zasoby, czynności i zegar. Kamera i renderer czytają stan. Klawiatura steruje bezpośrednio, cel trasy jest osobnym wejściem, a zamknięcie głównego przejścia i objazdu umożliwia demonstrację zmiany trasy oraz braku drogi. Są to komendy testowe geometrii, nie zniszczenie mostu.

Stały krok pozostaje 50 ms i wspólny z M01. Ruch jest rozliczany po jednym kroku, także przy przyspieszeniu. Pauza i minimalizacja zachowują reguły laboratorium M02. Utrata aktywacji okna czyści chwilowe wejście klawiatury, żeby nie pozostawić ruchu po puszczeniu klawisza w innym programie.

Zapis M03 ma osobną wersję otoczki, zawiera niezmieniony snapshot M01 oraz stan przejść, wersję geometrii, położenia, rozmiary, cele, pozostałe trasy i ich wersje. Odczyt waliduje kandydata przed zastąpieniem świata. Chwilowe klawisze nie są stanem świata. Współdzielimy istniejący mechanizm ograniczonego rozmiaru i atomowej podmiany pliku; nie kopiujemy implementacji I/O. Zapisy M01 nadal działają w M02, a tryb M03 jawnie odrzuca niewłaściwy format.

## Dowód i pliki

Nowe moduły `src/physics`, `src/navigation`, kontroler sceny ruchu i testy. Rozszerzenie geometrii/kamery prezentacji oraz wejścia Win32. Niewielkie rozszerzenia istniejącego konsumenta, pacera i persistence mają rzeczywistych konsumentów; zmiana obejmuje testy obu stron.

Testy: spoczynek, styk i początkowe nałożenie, ślizg, narożnik i wiele kontaktów, granice świata, szczelina, niska belka, szybki ruch przez cienką ścianę; A* wobec niezależnego BFS, brak drogi i budżetu, różne rozmiary, stary wynik, zamknięcie podczas ruchu, rzeczywiste dojście objazdem oraz oczekiwanie; save/load w podróży i ze starą trasą, uszkodzony format i brak częściowej zmiany. Build/testy Debug, RelWithDebInfo, ASan, styl i dokumentacja. Rzeczywista próba GPU z debug layer oraz wejścia myszy/klawiatury przez Computer Use; osobny przegląd integratora.

Nie dodajemy autonomii potrzeb, ekonomii transportu, magii, potworów ani walki. Wygląd i sterowanie są roboczym izometrycznym laboratorium; nie rozstrzygają docelowej kamery ani nie zastępują ludzkiej oceny grywalności.

Uzupełnienie zakresu narzędzia: nazwę katalogu w źródłowym URL publikacji walidator błędnie uznał za ID etapu. Kontrola identyfikatorów pomija teraz wyłącznie cele linków, nadal sprawdzając ich widoczne etykiety. Dwa przypadki regresji chronią prawidłowy URL oraz odrzucenie nieznanego kontraktu w etykiecie. Walidacja istnienia lokalnych linków i definicji ID pozostaje obowiązkowa.
