# Praktyczny audyt toru przestrzennego wielu postaci

Data: 2026-10-26. Zakres: [zatwierdzony brief](../work-items/traversal-multi-actor.md), na podstawie [propozycji użytkownika](../../traversal-multi-actor-audit.md). Odbiór techniczny opisanego zakresu: zaliczony. Dostarczona specyfikacja pozostaje zachowana; poniżej rozdzielamy wykonane próby, korekty założeń i granice dowodu.

## Reprodukcje

Na kodzie sprzed napraw nowy zestaw 11 przypadków dał 4 PASS i 7 FAIL: przeciwne kierunki na kładce, wymiana pasażerów, powtarzanie podejścia, ręczne hamowanie, obracająca się podpora, stan zajętego celu i uszkodzony zapis. Dowody: `build/multi-audit-before-tests.log`, `build/multi-audit-before-console.log`. Przechodziły pierwsze próby pojedynczego stoku, czterech mijających się postaci, 400 kroków sceny oraz podziału czasu. Przejście tych prostszych prób nie dowodziło braku błędów.

Kopie sesji, jej nagłówka, sceny, platform, nawigacji i okna zachowano w `build/multi-audit-baseline/`. Odrębny runner z zachowaną sesją/sceną/platformami, podłączony do wspólnych bibliotek geometrii/domeny, pozwolił powtórzyć późniejsze reprodukcje bez cofania bieżących plików produkcyjnych. Jest dowodem izolowanego zachowania starej sesji, nie pełnym historycznym buildem całego repozytorium. Logi `build/multi-audit-baseline-*.log` obejmują współliniowe podejście, ruchomy kontakt, komendy i podparcie.

Pierwsze szkice fixture skorygowano: edytor rekordów musiał odróżniać zagnieżdżoną sekcję `WorkWorld.ACTORS` od aktorów sesji; początkowa pozycja pasażera musiała uwzględniać obrót pokładu. Dla dwóch odpoczynków próg 240 kroków okazał się zbyt krótki — druga czynność była już w toku, a nie zakleszczona. Przyjęto 360, obejmujące dwie czynności po 60 kroków, dojście i 100 kroków ponowienia po odmowie. Stara sesja nadal nie kończyła drugiego odpoczynku w tym przedziale. Nie zmieniano tolerancji ani czasu istniejących regresji.

## Naprawy i ich granice

| Temat audytu | Wykonany przyrost i dowód |
| --- | --- |
| Kładka | Kolejka przed progiem wejścia, odsunięte punkty oczekiwania, zachowanie własności przy fizycznym zajęciu bez trasy. Cztery postacie przechodzą w przeciwnych kierunkach; test obserwuje FIFO, zapis właściciela/kolejki i wznowienie po ręcznym wyjściu osoby początkowo stojącej na kładce. |
| Prom | Oczekujący nie zajmują osi wyjścia. Końcowy `Walk` do portu uczestniczy w kolejce, zanim postać wejdzie w próg. Dojście do banku zachowuje sterowanie powierzchniowe. Boarding wymaga dojścia, zatrzymania, dopuszczalnego kroku i wolnej fizycznie drogi przez port do środka pokładu. Przejazd ma pierwszeństwo wyjścia przed kolejnym wejściem. Trzy osoby kończą wymianę, a odczyty obejmują Boarding, Riding i Exiting. |
| Podpora obrotowa i rozmiar | Rozpoczęty Step blokuje fazę ruchu do końca kroku. Przyłącze sceny skrócono z 0,60 do 0,50 m; nie zwiększano możliwości mniejszych kapsuł. Oba promienie wchodzą i schodzą, a obok stojąca postać blokuje obrót i pozwala go wznowić po odejściu. |
| Podparcie i kontakt | Rzeczywisty odcinek ruchu sprawdza ciągłe współpłaszczyznowe podparcie, nie tylko wsparty koniec. Częściowy kontakt aktualizuje podporę w rzeczywiście osiągniętym miejscu. Na ruchomej podporze odrzucony ruch własny zachowuje poprawny punkt przeniesienia; dodatkowo sprawdzane jest pozostałe przeniesienie z nowego lokalnego punktu do końca kroku świata. Usunięto obniżanie stóp przez zapas obrotu. |
| Granica i lokalne omijanie | Poszerzono ograniczony zbiór kierunków i dodano krótszy horyzont. Test obejmuje także parę o odległości środków 0,62 m. Po utracie postępu stan jest Waiting; ograniczone ponowne planowanie może uwzględnić stojące kapsuły do 3 m od wykonawcy. To nadal lokalne rozwiązanie, nie dowolny tłum. |
| Sterowanie ręczne | Kierunek gracza nie jest zastępowany bocznym omijaniem. Bezpieczna odległość przed krawędzią/przeszkodą służy hamowaniu; nieunikniony kontakt przy zbyt późnym hamowaniu pozostaje rozstrzygnięciem fizyki. Testy wymuszają też prędkość przy samej luce i kontakt tuż po zmianie płyty. |
| Komendy i stanowiska | Powtarzane podejście zachowuje wykonalny plan oraz odroczony zamiar. Zmienia tylko wejście danej postaci. Autonomia nie pozostawia starego stopu; po dokończeniu ręcznego kroku/przejazdu porzuca ręczną trasę, zamiast traktować ją jak zadanie nieistniejącego stanowiska. Punkty podejścia uwzględniają pozycje i cele innych postaci. |
| Zapis | Dwustronna zgodność właściciela usługi i fazy pasażera, ciągłość trasy, zgodność końca z celem, legalne połączenia Step/Walk i brak trasy bez celu. Testy odrzucają m.in. zmianę właściciela, zerwanie łańcucha, nakładanie kapsuł, brak celu, złą wersję i korupcję — bez podmiany aktywnego świata. |
| PPM i diagnostyka | Wybrana kapsuła nie zastępuje wskazanej powierzchni własną pozycją. W inspektorze są `passage=`, `blocked=` i `step_point=`. Natywny smoke dodatkowo sprawdza wybór powierzchni przez własną kapsułę. |

`SpatialPrism::support_local` współdzieli wzór podparcia z zapytaniem światowym i usuwa zbędny obrót punktu tam i z powrotem w `platform_anchor`. Domena tolerancji wysokości stóp jest wspólna z dotychczasowym walidatorem sesji: 30 µm. Dopasowanie płaszczyzn nadal ma 10 nm; nie zwiększono dopuszczalnego stopnia ani nie zasypano luk. W trakcie Step nie kotwiczymy ponownie stóp na płaszczyźnie, ponieważ kasowałoby to rzeczywisty profil uniesienia. Prezentacja zachowuje lokalną wysokość pasażera.

Test powtarzanych obrotów wykazał także narastanie błędu brzegowego po pojedynczym poprawnym przeniesieniu (`build/multi-audit-corner-tests.log`). Po sprawdzeniu podparcia lokalny punkt w zaakceptowanym marginesie numerycznym jest normalizowany do domeny rzeczywistego kontaktu. Test obejmuje 200 przeniesień. Na współpłaszczyznowej podporze zachowujemy bieżący identyfikator, jeśli nadal poprawnie podpiera stopy; kolejność brył w tablicy nie odbiera pasażerowi ruchomego pokładu. Osobna próba obejmuje pokład na wysokości jego fundamentu i zachowanie lokalnej kotwicy. Końcowe profile zapisu sprawdzają również zakresy kursorów Step oraz Boarding/Exiting.

## Nowe kontrole

Osobny `tests/traversal_audit_tests.cpp` i target `reality_traversal_audit_tests`: 16 zarejestrowanych przypadków `traversal.*`, obok wszystkich wcześniejszych. Obejmują także wejście i zejście po łańcuchu podłoga–rampa–podest, różne wysokości pod niskim stropem, pasażerów chodzących na ruchomej podporze, wymuszony kontakt z przeszkodą, impulsy wejścia innej postaci, 120 zmian poleceń i ich kontynuację. Zapytania prezentacji próbkują 0, 0,25, 0,5, 0,75 i 1 kroku; sprawdzają pary i geometrię. Osobny checkpoint potwierdza brak mutacji przez odczyt obrazu.

Domyślna scena przechodzi 400 kroków, zapis oraz identyczne kontynuacje po 5 kroków. Nie wymagamy odpoczynku postaci, które nie są jeszcze wyczerpane. W dodatkowym, jawnie uruchomionym eksperymencie `--observe-course 1000` domyślna scena ukończyła posiłki i konserwację. `--observe-needs-course 1000` obniża początkową energię jednej postaci do 100, bez zmiany produkcyjnej sceny, i wymaga rzeczywistych ukończeń wszystkich trzech rodzajów. Wyniki są w `build/multi-audit-course-1000.log` i `build/multi-audit-needs-course-1000.log`. To dodatkowe obserwacje Debug, poza liczbą automatycznych CTest; nie są pomiarem wydajności ani dowodem samowystarczalności gospodarki. Podział czasu jest testowany również na scenie z ruchomymi podporami i autonomią.

## Odbiór konfiguracji i aplikacji

Końcowe buildy i pełne CTest po ostatnich zmianach, bez filtrów i wykluczeń:

| Konfiguracja | Build | Pełny CTest | Dowód |
| --- | --- | --- | --- |
| Debug | PASS | 131/131 PASS | `build/multi-audit-debug-tests.log` |
| RelWithDebInfo | PASS | 131/131 PASS | `build/multi-audit-release-tests.log` |
| ASan / RelWithDebInfo | PASS | 133/133 PASS | `build/multi-audit-asan-tests.log` |

Wykonano `cmake --build --preset windows-debug`, `windows-release`, `windows-asan` oraz odpowiadające im `ctest --preset ...`. Logi buildów: `build/multi-audit-{debug,release,asan}-build.log`. Dodatkowe dwa przypadki ASan są kontrolami czystej fixture i wykrycia celowego przepełnienia sterty. Wszystkie dawne regresje, w tym M03, B/C, różne kapsuły i pojemność, pozostały zaliczone.

Pierwsze pełne próby ujawniły duplikat wpisu audytu w rejestrze dokumentów oraz przekroczenie 20 s przez długi przebieg ASan; wyniki tych prób są widoczne w historii narzędzi rozmowy. Duplikat usunięto z zachowaniem wcześniejszego wpisu. Sterowanie przestało badać gorsze warianty po znalezieniu kierunku o najniższej możliwej ocenie 0. Nie zmienia to wyboru ruchu ani czasu świata. Końcowy ASan zmieścił długi przebieg w tym samym limicie; limitów nie podnoszono i nie wykluczano testów. To wynik kontroli czasu testu, nie benchmark FPS.

Po końcowych buildach wykonano `reality_traversal.exe --smoke-test --gpu-debug` w trzech konfiguracjach oraz osobny proces `--smoke-test --resume-smoke --gpu-debug` dla każdego zapisu. Wszystkie sześć procesów wybrało sprzętowy NVIDIA GeForce MX330, włączyło debug layer i zakończyło się `TRAVERSAL SMOKE PASS` oraz `normal_shutdown=1`. Każde pierwsze uruchomienie potwierdziło `self_surface_picking=pass`. Porównania sum kontynuacji między pierwszym i drugim procesem dla każdej konfiguracji dały zgodność. Dowody: `build/multi-audit-gpu-{debug,release,asan}.log`, odpowiednie `-resume.log`, `.save` i `.png`.

`cmake --build --preset windows-debug --target format-check`: PASS (`build/multi-audit-format.log`). `python tools/check_docs.py`: PASS. `python -m unittest discover -s tools/tests -p "test_*.py"`: 29/29 PASS. Po uzupełnieniu wyników ponownie sprawdzono dokumentację. Sumy SHA-256 końcowych plików źródłowych, testu i CMake zapisano w `build/multi-audit-source-hashes.json`. Nie tworzono commitów, nie odrzucano cudzych plików i nie delegowano pracy.

Próba rzeczywistego wejścia Computer Use na oknie MX330: PPM przez wybraną kapsułę wyznaczył powierzchnię różną od pozycji postaci; N wykonało ruch do celu, Tab zmieniło postać, W wyłączyło jej autonomię i dostarczyło impuls ruchu. Zapis F5 wykonano w kroku 40, następnie N do kroku 60, a F9 przywróciło krok 40 i stan postaci. Okno zamknięto normalnie. Dowody: `build/multi-audit-interactive-visible.log` i `build/multi-audit-interactive.save`; zrzuty i odczyty interfejsu są w historii narzędzi tej rozmowy. Początkowy proces uruchomiony w ukryciu nie udostępnił okna i został zakończony przed właściwą próbą. To nie jest ocena komfortu dokonana przez użytkownika.

## Pozostałe ograniczenia

Nie deklarujemy pełnego navmesha, globalnie najkrótszej drogi, rozwiązania dowolnego zakleszczenia, pełnego solvera brył ani gotowej społeczności M04. Kolejki i odsunięte punkty zależą od dostępnego miejsca; cel gracza nie zostaje samowolnie przeniesiony, aby zwolnić przejście. Człowiek nadal osobno ocenia odczucie nowej wersji. Nie wykonano kontrolowanego benchmarku liczebności/FPS ani szerszej próby graczy. Zachowano stare formaty M03 oraz materiał/pojemność i wersje ich zapisów.
