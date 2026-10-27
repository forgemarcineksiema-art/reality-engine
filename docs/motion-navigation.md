# Ruch, kolizja i trasy laboratorium

**Status: M03 zaliczony w obsługiwanym zakresie, z uwagami rozwojowymi.** [Brief](work-items/m03-motion-navigation.md) określa fundament, [raport bazowy](verification/m03-motion-navigation.md#ocena-użytkownika) zapisuje jego ocenę przez użytkownika. Zatwierdzone dopracowanie tras ma [osobny brief](work-items/navigation-polish.md) i [raport](verification/navigation-polish.md); wcześniejsza ocena odczuć nie zastępuje próby nowej wersji. To wczesny konsument ruchu Action RPG, bez autonomii potrzeb i bez walki.

Aktualna implementacja obejmuje [audyt praktycznych przypadków](verification/motion-adversarial-audit.md): przyłącza końców przy ścianach, ograniczone obejście osoby i interpolację obrazu. Raporty wcześniejszych przyrostów pozostają zapisem ich ówczesnego zakresu, nie dowodem braku później wykrytych błędów.

## Własność i krok

[MotionSession](../src/lab/motion_session.hpp) przechowuje położenia, rozmiary, cele, trasy i geometrię małego toru. Ma nieposiadającą referencję do `LabSession`, który musi żyć dłużej i nadal posiada jedyny `WorkWorld` oraz zegar. Dwóch wykonawców używa rzeczywistych ID M01. Powiązanego `LabSession` nie zastępuj innym światem poza kontrolowanym odczytem `MotionSession`; komendy rezerwacji i pracy pozostają dostępne. Nie tworzymy drugiego zegara ani kopii świata dla renderera.

Krok tego laboratorium to 50 ms, prędkość obu wykonawców 3 m/s. Mniejszy wykonawca ma obrys 0,5×0,5 m i wysokość 1,2 m, większy 0,7×0,7 m i wysokość 1,8 m. Różni je rozmiar, nie reguła ruchu. Kolejność lokalnego rozliczenia jest stała: A, następnie B względem już przesuniętego A. Nie jest to bezstronny solver tłumu.

Każdy krok oblicza kandydata ruchu, zatwierdza jeden krok domeny, następnie bezalokacyjnie przenosi stan ruchu. Błąd przed zatwierdzeniem domeny zachowuje ruch tego kroku. Batch jest serią takich granic; wcześniejsze poprawne kroki pozostają wykonane, jeśli późniejszy krok zawiedzie. Przepełnienie czasu całego żądanego batcha jest sprawdzane przed pierwszym krokiem. `SimulationPacer` odejmuje z zaległości każdy faktycznie zakończony krok, również przed późniejszym błędem. Budżet nadal wynosi 20 kroków na aktualizację; reszta nie znika.

## Obsługiwana fizyka

[Kontroler kinematyczny](../src/physics/kinematic.hpp) pracuje na płaszczyźnie y=0. Postać ma osiowo ustawiony prostokątny obrys i wysokość. Przeszkoda ma prostokątny obrys oraz dolną i górną wysokość; niska postać może przejść pod belką. Geometria jest walidowana przez właściciela przed zapytaniami; publiczne `KinematicGeometry::validate()` sprawdza granice, wysokości, wersję i unikalność lokalnych kluczy. Dodatkowe przeszkody ruchu to zwalidowane obrysy drugiej postaci. Współrzędne muszą być skończone w zakresie ±1 000 000 m, wymiary dodatnie do 1000 m, kolekcja do 4096 brył, wersja niezerowa.

Sweep środka przeciw obrysowi rozszerzonemu o rozmiar postaci daje pierwszy czas kontaktu; ruch nie jest testem wyłącznie końcowej pozycji. Sterowanie bezpośrednie zatrzymuje składową normalną i zachowuje ślizg styczny. Podążanie trasą zatrzymuje się na pierwszym kontakcie, żeby ślizg nie wyniósł wykonawcy poza odcinek. Pozycja styku jest dozwolona, wnętrze przeszkody nie. Dokładne dojście do styku jest ruchem wolnym; dalszy ruch do wnętrza daje kontakt. Ruch mniejszy niż 10⁻¹² m jest traktowany jak spoczynek.

Margines przed kontaktem wynosi 0,000001 m. Do czterech kontaktów w jednym wywołaniu; wynik rozróżnia `Clear`, `Contact`, `InvalidStart` i `ContactBudget`. Początkowe nałożenie lub wyjście poza obszar daje `InvalidStart` bez zmiany pozycji. Dane niefinitywne i niepoprawne rozmiary zgłaszają błąd. Testy pozycji mają tolerancję 0,000002 m, wynikającą z marginesu i arytmetyki double.

Przy kontakcie dokładnie na końcu ruchu zachowujemy wyznaczoną płaszczyznę styku zamiast polegać na zaokrąglonym dodawaniu początku i wektora. Składowa styczna wykonuje pełny ruch. Nie jest to automatyczna korekta niepoprawnego początku; taki początek nadal daje `InvalidStart`. Test regresji sprawdza 1000 podejść i następną próbę odejścia.

Wektor ruchu ma osobny zakres ±2 000 000 m, żeby pomieścić różnicę dwóch poprawnych pozycji. `MoveResult` zawiera też do sześciu próbek rzeczywistego przebiegu kroku, z fazą od 0 do 1: start, kontakty, ślizg i koniec. Są danymi pochodnymi dla prezentacji, nie nową autorytatywną pozycją ani zmianą bilansu czasu.

Nie obsługujemy grawitacji, skoków, stoków, pięter, orientacji obrysu, tarcia, mas, pędów, więzów ani rozpadu konstrukcji. Bryły nie muszą być oddzielnymi obiektami domeny; ich lokalne klucze identyfikują elementy stałego toru, nie osoby lub zasoby świata. Nie jest to pełna fizyka brył 3D.

## Nawigacja i aktualność

[Własne A*](../src/navigation/grid_navigation.hpp), funkcja `find_route`, działa na maksymalnie 64×64 komórkach, czterech sąsiadach i jednakowym koszcie krawędzi siatki. Komórka ma co najmniej 1 mm. Siatka pokrywa granice z tolerancją zależną od wielkości i precyzji współrzędnych; mikroskopijny rozmiar komórki nie może ukryć niedopasowania. Obliczona szerokość i głębokość muszą pozostać skończone, również gdy sama komórka ma skończoną wartość. Nasz tor ma 20×14 komórek po 1 m. Każde połączenie sprawdza sweep tej samej fizyki i ten sam rozmiar; nie przechowujemy osobnej mutowalnej mapy przeszkód. Dla końców w centrach komórek surowa trasa zachowuje referencję najkrótszego wyniku siatkowego i dotychczasowe testy BFS.

Końce są najpierw łączone z własnym centrum przez sprawdzony odcinek. Jeśli centrum lub odcinek jest zablokowany, używamy sprawdzanych centrów w sąsiedztwie 3×3. Koszt przyłącza to jego długość podzielona przez rozmiar komórki. A* uwzględnia wiele przyłączy, a heurystyka bierze minimum Manhattan i kosztu końcowego przyłącza. Identyczny poprawny początek i cel daje zerową podróż. `InvalidEndpoint` oznacza fizycznie niewłaściwy punkt, a brak przyłącza poprawnego punktu daje `NoPath` w tym przybliżonym grafie. Nie dowodzi to braku drogi w przestrzeni ciągłej. Pozostałe wyniki: `Found` i `BudgetExceeded`. Budżet liczy rozwinięte węzły; brak budżetu nie staje się fikcyjnym sukcesem. Zwykły limit 4096 wystarcza na ten mały tor, lecz nie jest pomiarem wydajności.

Konsument ruchu używa `find_motion_route`: najpierw sprawdza bezpośredni sweep od obecnej pozycji do celu. Wolny odcinek daje pojedynczy punkt celu i zero rozwiniętych węzłów, także przy zerowym budżecie A*. Jeśli odcinek jest zablokowany, uruchamia surowe A* i skraca jego łamaną: z aktualnego punktu wybiera najdalszy pozostały punkt z wolnym sweepem całego obrysu i wysokości. Nie używa testu samego środka postaci. Domyślnie wykonuje do 4096 dodatkowych sprawdzeń skrótów; po wyczerpaniu limitu zachowuje poprawny pozostały fragment surowej trasy. `expanded` nadal oznacza wyłącznie rozwinięcia A*, nie te zapytania. Nie obiecujemy najkrótszej drogi w przestrzeni ciągłej ani krzywych ruchu. Ruch po skróconej łamanej nadal przechodzi kontroler kolizji co krok.

Nowy cel jest planowany synchronicznie od bieżącej pozycji, bez zmiany tej pozycji. Ponowienie identycznego celu przy tej samej wersji geometrii zachowuje trasę, kursor, stan i licznik planowań, a czyści wejście ręczne. Dotyczy to również stania u celu i oczekiwania. Zmiana geometrii unieważnia tę możliwość zachowania planu. Prędkość pozostaje 3 m/s, bez rozpędzania i hamowania; obraz jest interpolowany osobno poniżej.

Wyjątek zgodności: dawny zapis mógł zachować `InvalidEndpoint` dla fizycznie legalnego celu z błędnym przyłączem komórki. W takiej sytuacji ponowienie celu lub kontynuacja czasu podejmuje nowe planowanie. Nie utrwalamy starego błędu przez idempotentne zachowanie odmowy, która już nie odpowiada regułom planera.

Wynik zawiera początek, cel, rozmiar i wersję geometrii. `accept_route` odrzuca niewłaściwy początek/rozmiar/wersję, zły cel, limity i odcinki przecinające bryły. API sprawdza również rezultat przygotowany poza kontrolerem, choć nie uruchomiliśmy współbieżnego wyszukiwania. Po zmianie geometrii każdy zachowany cel jest planowany ponownie przed ruchem. `NoPath` zatrzymuje postać i zachowuje cel; otwarcie przejścia pozwala ją wznowić. Druga postać jest lokalną przeszkodą, nie statyczną częścią A*. Blokada daje `Waiting` i próbę kontynuacji w kolejnych krokach, bez teleportacji, przepychania i planowania kooperacyjnego.

Przy `Waiting` konsument podejmuje też ograniczoną próbę obejścia: do tymczasowej kopii geometrii dodaje aktualny obrys sąsiada (lokalny klucz 100). Nie zmienia źródłowej mapy ani pozycji sąsiada. Próbuje co 10 ticków, z fazą 0 lub 5 zależną od parzystości ID; przy wzajemnym oczekiwaniu większe ID próbuje obejścia pierwsze. Zajęty cel lub brak drogi zachowuje oczekiwanie i starą trasę. Każdy kolejny ruch nadal ma sweep względem aktualnej osoby. To lokalna reguła dwóch wykonawców, nie rezerwacja całej podróży i nie pełny solver tłumu. Zegar i ID są już w zapisie, więc ponowienia nie wymagają nowego ukrytego timera.

Tor ma dwa przejścia: główne oraz objazd. Zmiana dostępności buduje kandydata geometrii, sprawdza wszystkich wykonawców i dopiero potem zmienia wersję. Zamknięcie na postaci jest odrzucane bez skutku. Wielokrotne ustawienie tej samej wartości nie zwiększa wersji. Nie jest to zniszczenie ani naprawa mostu.

To laboratorium ma globalny widok mapy. Podążanie po zadanym celu nie jest autonomicznym NPC z wiedzą i potrzebami. Przyszły planer postaci potrzebuje osobnych kanałów poznawania przejść zgodnie z INV-08.

## Wejście i prezentacja

Tryb `reality_lab --motion` jest osobny od zachowanego domyślnego laboratorium M02. WASD działa względem kamery, a długość wejścia jest ograniczona do 1, więc przekątna nie przyspiesza. Krótkie naciśnięcie jest buforowane do jednego następnego kroku; przytrzymanie trwa do puszczenia. W pauzie impuls czeka na krok, nie przesuwa postaci w czasie rzeczywistym. Bezpośredni ruch anuluje jej cel. Zmiana wyboru, utrata aktywacji i odczyt czyszczą wejście. Klawisze nie należą do stanu zapisu.

PPM wyznacza cel na płaszczyźnie podłoża przez odwrócenie projekcji kamery; LPM wybiera widoczną postać z uwzględnieniem zasłaniania. Panel odróżnia „Ruch do celu”, „Cel osiągnięty”, „Brak drogi do celu”, „Cel niedostępny” oraz „Czeka: przeszkoda”; inspektor zachowuje techniczne nazwy stanów. `F` sprawdza drugą osobę w zasięgu 1,5 m i wolny odcinek dojścia. To lokalny kontakt testowy, bez rozmów, relacji i systemu wiedzy. Rezerwacje M01 nadal są oznaczoną diagnostyką, bez wymagań odległości, transportu lub miejsca pracy; nie dowodzą osobistej dostawy.

Renderer rysuje dokładne obrysy brył i wykonawców oraz najwyżej 128 pozostałych punktów na trasę. Pełna liczba punktów jest w inspektorze. Stara wersja trasy ma czerwone znaczniki do następnego kroku planowania. Bufor GPU ma 16 384 wierzchołki i jawny błąd przekroczenia; nie usuwa automatycznie kolizji ani postaci.

`presentation_positions(fraction)` próbkuje pochodną historię ostatniego kroku, także ślizg i przejście przez punkt trasy. Nie łączy bezwarunkowo końców kroku przez narożnik. Dla obrazu drugiej postaci uwzględnia interpolowany obrys pierwszej, zachowując rozłączność. Zapytanie nie zmienia świata. Aplikacja używa części kroku z `SimulationPacer`, ograniczonej do 0–1 bez ekstrapolacji; obraz ma do jednego kroku opóźnienia względem stanu zatwierdzonego. Pauza pokazuje stan bieżący. Wczytanie, zmiana geometrii, przełączenie czasu i resize synchronizują historię. Widoczny wybór myszą używa tej samej interpolowanej geometrii co GPU. To usuwa wymuszone rysowanie wyłącznie pozycji 20 Hz, ale nie jest benchmarkiem płynności laptopa.

## Kontynuacja

Otoczka `GENESIS_MOTION 1` zawiera długość i niezmieniony snapshot `GENESIS_WORK_WORLD 1`, flagi przejść, wersję geometrii oraz dwóch wykonawców: ID, pozycję, rozmiar, cel, stan podróży, licznik planowań, trasę i kursor. Układ toru jest częścią wersji 1; jego zmiana wymaga nowej wersji/migracji, nie cichego odczytu innych przeszkód. Geometryczne współrzędne zapisujemy jako tekst double z 17 cyframi i lokalizacją classic. Suma FNV-1a obejmuje całą otoczkę; wykrywa uszkodzenie, nie jest podpisem kryptograficznym.

Limit pliku pozostaje 4 MiB, limit punktów trasy 4097, rozwinięć 4096. Odczyt waliduje osobnego kandydata: format, identyfikatory, rozmiary, stany, bramki, wersje, pozycje, rozłączność postaci i niewykonane odcinki aktualnej trasy. Stara trasa może zawierać dziś zablokowany odcinek; nigdy nie jest użyta przed ponownym planowaniem. Błąd nie zmienia bieżącego świata. Kamera, zaznaczenie, pauza i wejście są stanem narzędzia. M03 przyjmuje wyłącznie własny format; M02 nadal przyjmuje M01.

Dopracowanie tras nie zmienia schematu wersji 1. Wczytanie zachowuje zapisane punkty; stara poprawna trasa może nadal prowadzić przez centra komórek. Dopiero nowe planowanie używa skrótów. Test sprzętowy odczytuje również rzeczywisty [zapis podróży sprzed poprawki](../tests/data/m03-v1-inflight.save), chroniony stałym SHA-256, i kontynuuje czas, ID oraz pracę bez resetu.

Audyt nie dodaje trwałych pól: ponowienia lokalnego obejścia wynikają z zegara i ID, a historia obrazu jest pochodna i odtwarzana jako nieruchoma bieżąca pozycja po wczytaniu. Format nadal ma wersję 1, ale zmieniony algorytm może zmienić dalsze zachowanie, szczególnie uprzednio zablokowanej postaci. Nie obiecujemy bitowej kontynuacji między różnymi wersjami programu; porównujemy kontynuację w tej samej wersji i chronione własności starego pliku.

[Wspólna warstwa pliku](../src/persistence/world_snapshot.hpp) udostępnia ograniczony odczyt oraz atomową podmianę poprzez istniejący mechanizm Windows. Oba kodeki używają tego samego I/O; nie dodano konkurencyjnej implementacji. Ochrona ostatniego pliku nadal podlega testowi odmowy podmiany M01. Nie twierdzimy, że sprawdzono utratę zasilania lub sprzętową awarię dysku.

## Podstawa algorytmów

Wykorzystano własny kod bez nowych bibliotek. Rozdzielenie wyszukiwania i poruszania opisuje autorskie [wprowadzenie do A*](https://www.redblobgames.com/pathfinding/a-star/introduction.html); sposób pracy A* i heurystyki opisuje [opracowanie autora](https://www.redblobgames.com/pathfinding/a-star/implementation.html). Test BFS stanowi niezależną referencję dla jednakowych kosztów, nie kopię implementacji A*. Metoda przedziałów slab ma źródło w literaturze przecięcia promienia z AABB, m.in. [Williams i in.](https://perso.univ-lyon1.fr/jean-claude.iehl/Public/educ/M1IMAGE/williams_box.pdf). Nie kopiowano przykładowego kodu ani pełnego solvera z tych źródeł.
