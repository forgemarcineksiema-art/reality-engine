# Rejestr decyzji

**Właściciel tematu:** status wyborów i ich powody. Daty zapisów opisują rozmowę i porządkowanie dokumentacji, nie daty wykonania kodu. Stan implementacji jest w [środowisku pracy](development.md).

### DEC-001 — Pierwszy rezultat

**USTALONE, użytkownik, 25.10.2026.** Grywalny fragment i laboratorium małej autonomicznej cywilizacji na własnym silniku; grafika ma najmniejszy priorytet. Odrzucamy plan, w którym rozbudowa renderera staje się warunkiem rozpoczęcia badań autonomii.

### DEC-002 — Własność technologii

**USTALONE, użytkownik, 25.10.2026.** Własny silnik, własna fizyka i nawigacja; minimum bibliotek. Unreal Engine, Jolt i Recast/Detour nie należą do planu wykonawczego. Narzędzia kompilacji i systemowe API są inną kategorią niż gotowy subsystem gry.

### DEC-003 — Zaplecze pracy

**USTALONE, użytkownik, 25.10.2026.** Użytkownik pracuje ze swoimi agentami AI, bez narzuconego terminu. Laptop i5/MX330/8 GB jest punktem odniesienia. Pomoc agentów nie zastępuje testów i przeglądu. Sprzęt oraz narzędzia odczytano lokalnie; szczegóły należą do właściwych dokumentów.

### DEC-004 — Roboczy stos pierwszej aplikacji

**PROPOZYCJA inżynierska.** C++20, CMake, Windows/Win32 i Direct3D 11. Powód: ograniczyć obowiązki obsługi GPU przed sprawdzeniem grywalnej symulacji i nie dodawać zależności runtime bez potrzeby. Nie jest to twierdzenie o większej wydajności D3D11 od D3D12. SDK, poziom funkcji i wybór adaptera trzeba potwierdzić działającym programem.

**WYNIK M00:** C++20 i CMake zostały sprawdzone w buildach MSVC x64 z Windows SDK. Renderer, Win32 i D3D11 nie zostały jeszcze zaimplementowane ani zweryfikowane na GPU. Wykorzystano systemowy toolchain bez nowych zewnętrznych bibliotek runtime.

**WYNIK M02:** okno Win32 i własny backend D3D11 zostały zaimplementowane oraz zweryfikowane na MX330, także z warstwą diagnostyczną GPU w pełnych zestawach Debug, RelWithDebInfo i ASan. Nie jest to benchmark dużego świata ani porównanie backendów. [Raport](verification/m02-window-laboratory.md) określa zakres.

Zmiana backendu nie może wymagać przepisania gospodarki ani wiedzy NPC. Źródła techniczne: [tworzenie urządzenia D3D11](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-d3d11createdevice), [różnice obowiązków D3D11/D3D12](https://learn.microsoft.com/en-us/windows/win32/direct3d12/important-changes-from-directx-11-to-directx-12).

### DEC-005 — Zakres pierwszej geometrii i AI

**PROPOZYCJA inżynierska.** Proceduralne bryły, ograniczona geometria kolizji, własna mapa przejść i A*, potrzeby, procedury oraz ograniczony planer. Pierwszy wycinek nie wymaga lokalnego modelu językowego ani gotowego ECS. Nie oznacza to odrzucenia późniejszej rozbudowy.

### DEC-006 — Jeden plan i źródła odpowiedzialności

**ORGANIZACJA na podstawie zlecenia użytkownika.** Obowiązuje jeden plan etapów. Sześć poprzednich dokumentów zachowano w archiwum bez zmiany treści; nie są normatywne. Kontrakty, reguły, testy i budżety mają osobne dokumenty odpowiedzialne za temat. Rejestr [dokumentacji](documentation-map.json) wskazuje te granice.

### DEC-007 — Parametry i skala

**PROPOZYCJA do pomiaru.** Liczebności, limity pamięci i cel płynności są zdefiniowane w [budżetach](performance-budget.md). Nie są benchmarkami ani zobowiązaniem do osiągnięcia dowolnej skali. Zwiększenie populacji wymaga przejścia odpowiedniego etapu i porównania kosztów.

### DEC-008 — Tożsamość świata i otwarte szczegóły

**USTALONE, użytkownik, 25.10.2026; zastępuje wcześniejszy status otwartej fikcji.** Docelowy GENESIS to przyziemne, realistyczne i brutalne fantasy z magią, królestwami, potworami i autonomiczną, ewoluującą cywilizacją. Wiarygodność obejmuje społeczeństwo, gospodarkę, politykę, ekosystemy, kulturę, historię oraz fizyczne skutki działań. Fantastyczne elementy są częścią tych samych zależności i mają określone reguły, ograniczenia, koszty oraz konsekwencje. Celem jest wybitne Action RPG w takim świecie. Rozwinięcie intencji należy do [karty projektu](project-charter.md).

Porównanie do Marvel Cinematic Universe jest inspiracją skalą i spójnością uniwersum; nie zatwierdza jego postaci, fabuły, estetyki, tonu ani mechanik. Nie przenosi też do Genesis wyjątków od przyczynowości.

**NADAL OTWARTE:** nazwy, geografia, kultury, poziom technologii, pochodzenie i konkretne reguły magii, gatunki istot, rola bohatera, kamera docelowej gry, tempo czasu, model śmierci gracza i szczegółowe zasady walki/progresji. Zatwierdzenie kierunku nie zatwierdza automatycznie każdej propozycji tych elementów.

Wolne Dorzecze, Narew, bliska przyszłość i technologiczna sieć GENESIS pozostają historycznymi propozycjami, nie obowiązującą fikcją gry. Wartościowe mechaniki mostu, dostaw, pracy i infrastruktury pozostają. Historycznych dokumentów nie przepisujemy; migracja dotyczy bieżącego projektu treści. Nie wymaga zmiany ID, formatu zapisu ani kontraktów rdzenia.

### DEC-009 — Pierwszy model czynności i zapis

**WDROŻONE inżyniersko w M01, 25.10.2026.** Konkretne reguły określa [model domeny](work-domain.md). Wybrano jeden sekwencyjny właściciel stanu, monotoniczne ID świata i uchwyty z generacją, zamiast adresów lub ID slotów. Rezerwacja jest wyliczana z czynności zamiast osobnej mutowalnej ewidencji. Rejestr komend zachowuje również odmowy, co pozwala odtworzyć pierwotny wynik ponowienia; nowa próba wymaga nowego ID.

Wersjonowany tekstowy snapshot jest prosty do inspekcji i wystarcza dla małego headless modelu. Wybrano pełny stan i dziennik zamiast infrastruktury bazy danych, bez nowych zależności. Warstwa zapisu używa Windows API do pliku tymczasowego, flush i podmiany; nie uzasadnia to twierdzenia o odporności na dowolną awarię zasilania. Limity są jawne, historia nie znika po cichu. Nie projektowano jeszcze wieloprocesowych zapisów, retencji ani globalnych GUID dla rozgałęzień świata. Nie zmienia to wymagań docelowej skali: wymagają dalszego projektu i pomiarów.

### DEC-010 — Pierwsza prezentacja i polityka czasu

**WDROŻONE I ZWERYFIKOWANE w M02, 25.10.2026.** Użytkownik autoryzował zaproponowaną aplikację Win32/D3D11 ze statyczną sceną i komendami M01. `LabSession` jest konsumentem domeny, a renderer otrzymuje wyłącznie geometrię i kolory. Używamy standardowych kontrolek systemowych i małych shaderów, bez nowej biblioteki GUI lub gotowego renderera. CPU wyznacza projekcję niewielkiej sceny; jest to ograniczony początek, nie architektura renderowania dużego świata.

Laboratorium startuje w pauzie, minimalizacja i interaktywna zmiana rozmiaru zawieszają dopływ czasu rzeczywistego. Akumulator stałego kroku zachowuje już naliczoną zaległość i ogranicza rozliczenie na aktualizację. Odczyt rozpoczyna nową oś pomiaru prezentacji. To polityka narzędzia, nie docelowa decyzja o autonomii poza ekranem. Domyślnie wybieramy sprzętowy adapter według preferencji wysokiej wydajności; wskazanie indeksu nie pozwala na cichy fallback. Szczegóły i konsekwencje są w [modelu laboratorium](window-laboratory.md).

### DEC-011 — Granice wykonania nowej wizji

**USTALONE, użytkownik, 25.10.2026.** Zachowujemy Reality Engine, ukończone fundamenty i jeden plan M00–M06. Silnik pozostaje niezależny od konkretnych elementów fikcji tam, gdzie ma to uzasadnienie; zasady gry mogą zależeć od fikcji, lecz ich skutki przechodzą przez jawne kontrakty. Pierwszy prototyp nie wymaga magii, potworów ani rozbudowanej walki. Nie rozpoczynamy wielkiej produkcji zawartości fantasy przed sprawdzeniem autonomii, przyczynowości, fizyki, nawigacji i trwałości.

Scenę dwóch osad i mostu adaptujemy do nowego kontekstu, zachowując wymagania symulacyjne oraz testy. Projekt bezpośredniej rozgrywki Action RPG rozwijamy równolegle w [projekcie gry](game-design.md); jego odbiór włączamy do istniejących etapów. Laboratorium nie staje się zamiennikiem gry ani dowodem jej atrakcyjności. Szczegóły mechanik i fikcji w tym dokumencie pozostają propozycjami do sprawdzenia.

### DEC-012 — Pierwszy konsument ruchu

**WDROŻONE inżyniersko na zlecenie rozpoczęcia M03, 25.10.2026.** Wybrano kinematykę 2,5D, statyczne bryły osiowo ustawione, wysokość postaci, sweep oraz siatkowe A* używające tej samej geometrii. Alternatywy — pełna dynamika brył, navmesh i tłum kooperacyjny — wymagają osobnych potrzeb i odbioru. Nie dodano zewnętrznej fizyki, nawigacji ani bibliotek runtime. [Model ruchu](motion-navigation.md) określa jawne przybliżenia, własność i wersjonowanie.

Tryb `--motion` zachowuje M02, jego testy oraz format M01. Nowa otoczka zapisu M03 obejmuje ruch i stan M01, korzystając ze wspólnej warstwy pliku. Odczyt M01/M03 jest rozłączny i jawny, bez automatycznej utraty stanu. Kolejność M00–M06, zatwierdzona fikcja i otwarte szczegóły gry pozostają zgodne z DEC-008 i DEC-011. Izometryczny tor i kontakt `F` służą próbie, nie przesądzają kamery, skali świata ani dialogów. [Raport](verification/m03-motion-navigation.md) oddziela odbiór techniczny od człowieka.

Dopracowanie po odbiorze M03, **zatwierdzone przez użytkownika i wdrożone inżyniersko**: zachowujemy surowe A* jako referencję siatkową; rzeczywisty konsument ruchu sprawdza bezpośredni odcinek i skraca łamaną tym samym sweepem. Wynik nie jest deklarowany jako optymalny ciągle. Ponowienie tego samego celu przy tej samej geometrii zachowuje postęp. Brak nowych pól pozwala zachować format M03 w wersji 1 oraz stare zapisane punkty. Nie dodano zależności, akceleracji ani autonomii. [Brief](work-items/navigation-polish.md) i [raport](verification/navigation-polish.md) określają zakres oraz dowody; nie zmienia to DEC-008 ani DEC-011.

### DEC-013 — Praktyczne przyłącza i obraz między krokami

**WDROŻONE inżyniersko na zlecenie audytu użytkownika, 25.10.2026.** Reprodukcja wykazała, że legalna pozycja na kontakcie mogła nie uruchamiać objazdu. Przyłącza końców do sprawdzanych sąsiadów 3×3 uzupełniają A*, zachowując BFS dla centrów. Fizycznie błędny punkt i brak drogi w grafie mają odrębne wyniki. Minimum komórki to 1 mm, a walidacja uwzględnia precyzję współrzędnych. Pozostajemy w zakresie 2,5D zamiast traktować pełny solver lub navmesh jako warunek naprawy błędu obecnej geometrii.

Ruch świata pozostaje 50 ms i 3 m/s. Obraz korzysta z rzeczywistego przebiegu ostatniego kroku, z opóźnieniem do jednego kroku i kontrolą kolizji na zakręcie oraz między widokami dwóch postaci. Historia jest pochodna, nie jest drugim właścicielem pozycji i po odczycie jest synchronizowana. Lokalna próba obejścia osoby używa deterministycznej fazy zegara i ID oraz aktualnego obrysu; nie jest pełnym planowaniem tłumu. Brak nowych trwałych pól zachowuje format zapisu w wersji 1, bez obietnicy identycznego zachowania różnych wersji algorytmu. Nie dodano zależności ani innych platform. [Model](motion-navigation.md) i [raport](verification/motion-adversarial-audit.md) określają szczegóły i ograniczenia.

### DEC-014 — Rozszerzenie lokomocji i autonomii

**USTALONY zakres użytkownika, implementacja w toku.** Użytkownik zaakceptował płynność wersji po audycie, następnie zlecił rozpędzanie, hamowanie, stoki, orientację i obrót, obrócone bryły, podesty, kładki, poziomy, ruchome platformy oraz autonomiczne planowanie. „Inne platformy” zostały doprecyzowane jako konstrukcje świata gry, nie inne systemy operacyjne. Nie zmienia to zatwierdzonej fikcji ani własności silnika.

**WYBÓR INŻYNIERSKI do weryfikacji:** wspólne wypukłe graniastosłupy o nachylonym wierzchu i obrocie w poziomie oraz pionowa kapsuła wykonawcy. Siatka dawnego toru pozostaje regresją; nowy konsument wykorzysta powierzchnie, jawne fizyczne połączenia i graf widoczności z kosztami w metrach. Ograniczenia rozpędzania i obrotu należą do rzeczywistego stanu ruchu. Przejazd platformą jest czynnością z fazami, nie teleportem lub skrótem renderera. Autonomia wybiera znane cele i korzysta z rezerwacji oraz czynności M01. Nie oznacza jeszcze całego odbioru społeczności M04.

Dotychczasowe formaty pozostają czytelne przez ich dotychczasowych konsumentów; dodatkowe pola i geometria wymagają nowej wersjonowanej otoczki. [Brief](work-items/advanced-traversal.md) określa kolejność, kryteria i granice odbioru. Nie dodajemy gotowej fizyki, nawigacji ani modelu generatywnego jako właściciela świata. Nie obiecujemy pełności dla dowolnej geometrii, globalnie najkrótszych tras ani usuwania każdego zakleszczenia tłumu.

## DEC-015 — Lokalna zgoda na miejsce w rozpoczętym przejściu

**Kierunek użytkownika:** kontynuacja po naprawie krążenia i wskazaniu wzajemnej blokady. **Wybór inżynierski:** ograniczony protokół prośby, niezależna odpowiedź odbiorcy i manewr istniejącą fizyką, z jednym właścicielem stanu w sesji. Ręczne sterowanie i rozpoczęta praca odmawiają; brak miejsca jest wynikiem, nie ukrytym teleportem. Alternatywy: samo stabilne NoPath nie usuwa fizycznego konfliktu; pełny system społeczny przekracza obecny przyrost. [Plan](work-items/local-clearance.md) określa limity i odbiór, [raport](verification/local-clearance.md) wyniki.

Aktywne zgody wymagają zewnętrznego formatu 3; czytelnicy wersji 1/2 zachowują puste interakcje, a bez nowych danych serializer zachowuje te wersje. WorkWorld i starszy MotionSession nie zmieniają formatu. Nie jest to zatwierdzenie agresji, reputacji ani zakończenie M04.

## DEC-016 — Rzeczywisty materiał w dostawie

**Źródło:** bezpośrednie polecenie użytkownika rozpoczęcia transferu i dostaw. **Wybór inżynieryjny w odbiorze:** typowane partie i atomowy transfer należą do `WorkWorld`; niesiony materiał pozostaje odrębną rzeczywistą partią świata, a postać posiada jej ID i stan kursu. Zamiast licznika w rendererze lub teleportowanego skutku fizyczne pobranie i oddanie wymagają dostępności, dojścia i wspólnej operacji domeny. [Plan](work-items/material-delivery.md) oraz [raport](verification/material-delivery.md) opisują wyniki i granice.

`GENESIS_WORK_WORLD 3` zapisuje typ i transfery; `GENESIS_TRAVERSAL 4` zapisuje ładunek, kurs i stanowisko zwrotu. Późniejszy przyrost dodaje autonomiczne odłożenie nieprzypisanego ładunku w znanym zgodnym stanowisku bez nowej wersji formatu; dawne wersje pozostają czytelne, a nieokreślony historyczny materiał nie jest reinterpretowany. Sam przyrost nie rozstrzyga praw własności, recept ani zakończenia M04. Limit historii nadal wymaga osobnego rozwiązania.

## DEC-017 — Badania w prototypie, pełne M05/M06, następnie projekt 3D

**USTALONA kolejność, użytkownik, 28.10.2026.** Najpierw wykorzystać ukończone eksperymenty w działającym prototypie, następnie w pełni ukończyć M05 i M06, a dopiero później przekształcić obecny zakres 2,5D w pełne 3D w osobnym projekcie. Rozwinięcie należy do [jednego planu](prototype-plan.md), a pierwszy konkretny przyrost do [projektu integracji praktyk](work-items/practice-in-prototype.md).

Zachowuje DEC-011, własny silnik, odebrane fundamenty i wymagania ciągłości, migracji reprezentacji oraz odbioru. Nie zmienia historycznej akceptacji M04. Nowy konsument badawczy nie jest automatycznie funkcją zwykłej gry; potrzebne są wspólny świat, widok, wejście i próba użytkownika. Szczegóły urządzenia, techniczne API integracji, liczby nowego pomiaru i zakres pełnego 3D pozostają propozycjami autora do osobnego dopracowania, nie dodatkowymi decyzjami przypisanymi użytkownikowi. Nie tworzymy drugiego backlogu etapów ani nie odraczamy braków M05/M06 do projektu 3D.

Ta decyzja dotyczy planowania. Nie zmienia formatu zapisu ani zachowania kodu; migracje wymagają własnych projektów i weryfikacji.

## Jak dopisać decyzję

Podaj ID, status, źródło decyzji, problem, wybrany zakres, alternatywy i skutki. Przy zmianie opisz, który wcześniejszy wybór zastępujesz oraz jak migrują dane i konsumenci. Wynik testu jest dowodem weryfikacji, nie zastępczą zgodą na zmianę celu użytkownika.
