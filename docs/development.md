# Stan środowiska i rzeczywiste komendy

**Właściciel tematu:** co istnieje, jak to uruchomić i czego jeszcze brakuje. Aktualizuj ten dokument razem z każdą zmianą systemu budowania.

## Stan projektu

Zaliczono M00–M03 w opisanym zakresie: kinematykę 2,5D, kolizje, A*, przejścia i zapis podróży w oknie Win32/D3D11 na MX330. Istnieją również zegar, ID, czynności, rezerwacje, ograniczona autonomia potrzeb i inspektor. [Raport M03](verification/m03-motion-navigation.md#ocena-użytkownika) wskazuje wyniki oraz akceptację obecnego sterowania przez użytkownika, z zachowanymi uwagami do dalszego dopracowania. Pełnej autonomii społeczności nie ma. Lokalny Git wersjonuje punkt wyjścia na gałęzi `main`, publikowanej w publicznym `origin`; zasady opisuje [przepływ pracy](workflow.md#repozytorium-git).

Osobna scena dostawy jest w realizacji i uruchamia się poleceniem `build/msvc/Debug/reality_traversal.exe --delivery`. Po przejęciu postaci przez F2 klawisz F4 pobiera lub odkłada materiał na pobliskim dostępnym stanowisku. Testy wybranego przyrostu: `ctest --preset windows-debug -R "^(transfer|delivery)[.]" --output-on-failure`. Status i granice są w [planie przyrostu](work-items/material-delivery.md).

Sześć oryginalnych dokumentów zachowano bajtowo; lista i SHA-256 znajdują się w [manifeście archiwum](archive/manifest.json). Do wykonania M00 nie instalowano nowych pakietów ani bibliotek gry.

## Odczytane narzędzia

25.10.2026 użyto CMake 4.4.4, MSVC 19.44.35226.0, Windows SDK 10.0.26100.0 i clang-format 19.1.5 z Visual Studio Build Tools 2022. Polecenie `python` wskazuje Python 3.11.9, a CMake wykrył także Python 3.13.1 i użył go do kontroli uruchamianych przez CTest. Obie ścieżki narzędziowe sprawdzono. Ninja jest dostępna, ale obecne presety używają generatora Visual Studio.

## Budowanie i testy C++

Z katalogu projektu, bez ręcznego ustawiania środowiska developera:

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-debug
ctest --preset windows-debug
```

Konfiguracja zwykła z optymalizacją i symbolami:

```powershell
cmake --build --preset windows-release
ctest --preset windows-release
```

Pojedynczy test:

```powershell
ctest --preset windows-debug -R "^clock[.]overflow_preserves_state$"
```

Presety ograniczają build do dwóch zadań, a CTest do jednego. Brak znalezionych testów oznacza błąd. Ostrzeżenia MSVC `/W4 /WX` są włączone dla własnych targetów. Nie skonfigurowano jeszcze innych toolchainów ani platform.

## Diagnostyka pamięci

```powershell
cmake --preset windows-asan
cmake --build --preset windows-asan
ctest --preset windows-asan
```

Profil używa RelWithDebInfo z `/fsanitize=address` i wyłączonym linkowaniem przyrostowym. Nie używa Debug z niezgodnym `/RTC`. CMake kopiuje wykryty runtime ASan do katalogu testowanych programów. Nie modyfikuje globalnego PATH.

Próbka z celowym heap-buffer-overflow jest osobnym targetem testowym, budowanym tylko w profilu ASan. Test wymaga jednocześnie niezerowego wyjścia i właściwego raportu. Brak programu, timeout albo inny błąd nie są zaliczeniem diagnostyki. Próbka poprawna musi przejść. Profil nie dowodzi braku wszystkich UB i nie bada wyścigów wątków.

## Program konsolowy i styl

```powershell
./build/msvc/Debug/reality_headless.exe --steps 12 --step-ms 25
cmake --build --preset windows-debug --target format-check
```

Pierwsze polecenie daje `ticks=12 step_ns=25000000 elapsed_ns=300000000`. To jawny postęp zegara, nie symulacja cywilizacji. `--help` opisuje argumenty. Nieprawidłowe wejście jest odrzucane.

Target stylu sprawdza istniejące pliki C++ według `.clang-format`. CMake szuka formatera w PATH i instalacji Visual Studio; jego brak jest błędem konfiguracji, nie cichym pominięciem. Obecny zakres plików znajduje się w CMakeLists.txt.

## Kontrola dokumentacji

Z katalogu głównego, bez zewnętrznych pakietów Pythona:

```powershell
python tools/check_docs.py
python -m unittest discover -s tools/tests -p "test_*.py"
```

Pojedynczy test walidatora:

```powershell
python tools/tests/test_check_docs.py DocumentationValidationTests.test_rejects_missing_local_link
```

Walidator ma niezerowy kod wyjścia przy błędzie. Sprawdza rejestr dokumentów, lokalne linki i kotwice, zdefiniowane identyfikatory oraz SHA-256 źródeł archiwalnych. Obsługuje odnośniki inline Markdown użyte w dokumentach kanonicznych. Linki referencyjne i HTML zgłasza jako nieobsługiwane, zamiast pomijać. Pomija przykładowe linki w blokach kodu. Nie jest pełnym parserem Markdown, nie sprawdza dostępności zewnętrznych stron WWW ani poprawności twierdzeń naukowych.

Testy walidatora korzystają z tymczasowych małych fixture'ów i celowo uszkodzonych dokumentów. Nie modyfikują archiwum projektu.

## Demonstracja czynności i kontynuacji

Po buildzie Debug, z katalogu projektu:

```powershell
./build/msvc/Debug/reality_work_demo.exe --create ./build/m01-world.save
./build/msvc/Debug/reality_work_demo.exe --inspect ./build/m01-world.save
./build/msvc/Debug/reality_work_demo.exe --resume ./build/m01-world.save
ctest --preset windows-debug -R "^work[.]fresh_process_continuation$"
ctest --preset windows-debug -R "^work[.]partial_cancellation$"
```

`--create` tworzy scenariusz dwóch wykonawców i trzech jednostek materiału. Pierwszy rezerwuje trzy, drugi dostaje odmowę; pierwszy zużywa jedną i pracuje do kroku 2. W zapisie pozostają dwie fizyczne i dwie zarezerwowane jednostki, termin 5. `--resume` odczytuje plik, wykonuje trzy kroki i zapisuje wynik: krok 5, czynność ukończona, zero materiału. Kolejne wznowienie przesuwa czas, bez drugiego zużycia. `--inspect` nie zmienia pliku. Ponowne `--create` rozpoczyna ten scenariusz od nowa i zastępuje wskazany plik.

To inspektor i konsument [modelu domeny](work-domain.md), bez autonomii, sterowania postacią czy obrazu. Argumenty plikowe Windows obsługują Unicode. Test procesu używa odrębnych uruchomień i tymczasowego pliku, także uszkodzonego. Test `work.save_replacement` blokuje podmianę poprawnego pliku rzeczywistym uchwytem Windows i sprawdza jego zachowanie. Test `work.event_capacity` sprawdza brak częściowej zmiany po wyczerpaniu dziennika.

## Dalszy zakres

Scena przeprawy: `build/msvc/Debug/reality_traversal.exe --crossing`. F8 zamyka/otwiera najbliższe przejście po fizycznym dojściu ręcznie sterowanej postaci; R rozpoczyna konserwację przy pobliskim stanowisku, F7 przekazuje obserwacje przejść i stanowisk. Czerwony kolor kładki oznacza faktyczne zamknięcie; przekonania są w inspektorze. Testy: `ctest --preset windows-debug -R '^passage[.]' --output-on-failure`. Próba dwóch procesów: `python tools/test_crossing_native.py --exe ./build/msvc/RelWithDebInfo/reality_traversal.exe --output ./build/crossing-native-probe` (nowy katalog). Wymaga D3D11, GPU i diagnostyki. [Model](traversal-autonomy.md#przejście-konstrukcja-i-lokalna-wiedza) opisuje API/format 6, [raport](verification/passage-logistics.md) wykonane dowody; dawna scena `--delivery` pozostaje osobna.

Lokalna wiedza: `ctest --preset windows-debug -R '^knowledge[.]' --output-on-failure`. F7 w `reality_traversal` przekazuje obserwacje wybranej postaci najbliższej osobie w zasięgu kontaktu. Inspektor odróżnia przekonania `belief` od faktycznych `station`. Model, format 5 i zgodność 1–4 opisuje [autonomia](traversal-autonomy.md#lokalna-wiedza-i-przekaz), wyniki [raport](verification/local-station-knowledge.md).

M03 zaliczono w obsługiwanym zakresie po ocenie użytkownika, z zachowaniem uwag do prowadzenia do celu, szybkości i naturalności ruchu. M04–M06 są planem. Wcześniejsze wyniki opisują [raport M00](verification/m00-build-and-diagnostics.md), [raport M01](verification/m01-identity-actions-reservations.md) i [raport M02](verification/m02-window-laboratory.md). Pełny odbiór M02 zaliczono po rzeczywistym wykonaniu wcześniej zablokowanej diagnostyki GPU, bez zmiany wymagania.

## Laboratorium ruchu

Aktualny [audyt praktyczny](verification/motion-adversarial-audit.md) dodał przyłącza trasy obok zablokowanego centrum, lokalne obejście osoby i interpolację obrazu po drodze kontaktów. Sterowanie, 3 m/s, 50 ms i format zapisu 1 pozostają. Przypadki graniczne uruchamia `ctest --preset windows-debug -R "^adversarial[.]"`. Testowy eksport kontaktu do własnego pliku: `./build/msvc/Debug/reality_adversarial_tests.exe --contact-fixture ./build/motion-contact.save`; potem można wskazać ten plik przez `--motion --save ./build/motion-contact.save` i odczytać F9. Eksport nadpisuje tylko wskazany plik, więc użyj ścieżki testowej, nie własnego zapisu gry.

Złożony test `motion.hardware_gpu_continuation` uruchamia trzy procesy po kolei, każdy z limitem 15 s w runnerze Pythona. CTest daje całości 50 s: 3×15 s i 5 s narzutu uruchomienia/raportu. Pozostałe testy zachowują limit 20 s. Są to zabezpieczenia przed zawieszeniem testów, nie budżety wydajności gry; limity pojedynczych procesów i asercje nie zostały poluzowane.

Po przyroście [prowadzenia do celu](verification/navigation-polish.md) postać używa bezpośrednich odcinków i sprawdzonych skrótów, a identyczny cel zachowuje postęp. Prędkość i klawisze pozostają bez zmiany. Format zapisu nadal ma wersję 1; stare zapisane trasy zachowują punkty do następnego planowania. Odbiór techniczny tego przyrostu i próba użytkownika mają odrębne statusy w raporcie.

```powershell
./build/msvc/Debug/reality_lab.exe --motion
./build/msvc/Debug/reality_lab.exe --motion --gpu-debug --save ./build/m03-world.save
ctest --preset windows-debug -R "^motion[.]"
```

Start w pauzie. Spacja przełącza czas, N rozlicza 20 kroków podczas pauzy. WASD porusza względem kamery; krótkie naciśnięcie czeka na jeden krok. 1/2 lub LPM wybiera postać, PPM wskazuje cel na podłożu, T wybiera punkt na drugim brzegu, F sprawdza kontakt z osobą w zasięgu 1,5 m. G przełącza główną drogę, H objazd; zamknięcie na postaci jest odrzucane. Obie zamknięte drogi dają brak trasy. Q/E, strzałki góra/dół i +/- zmieniają kamerę. F5 zapisuje, F9 odczytuje i zatrzymuje świat. Przyciski czynności M01 są diagnostyką, bez reguł odległości i dostawy.

Tryby M02/M03 używają różnych formatów i domyślnych plików: `lab-world.save` oraz `motion-world.save` obok exe. Własną ścieżkę wskazuje `--save`. Odczyt niewłaściwego formatu daje błąd bez resetu. Opcje `--motion --smoke-test --gpu-debug` uruchamiają natywną próbę M03, a `--resume-smoke` służy wyłącznie jej drugiemu procesowi. Zwykłe uruchomienie bez `--motion` zachowuje M02 i jego skróty.

## Laboratorium okienkowe

Po buildzie Debug:

```powershell
./build/msvc/Debug/reality_lab.exe
```

Aplikacja startuje w pauzie z nowym scenariuszem; nie wczytuje ani nie zapisuje świata samoczynnie. Domyślny plik `lab-world.save` i log `genesis-lab.log` znajdują się obok exe. Log bieżącego uruchomienia zastępuje poprzedni. Przyciski Zapisz/Wczytaj używają tego pliku; ścieżki można wskazać jawnie:

```powershell
./build/msvc/Debug/reality_lab.exe --save ./build/moj-swiat.save --report ./build/moj-lab.log --adapter 0
```

Indeksy adapterów oraz rzeczywisty wybór są w logu. Na zweryfikowanej konfiguracji indeks 0 oznaczał MX330, 1 Intel UHD, 2 renderer programowy, który jest odrzucany. Nie zakładaj tej kolejności na innym komputerze.

| Sterowanie | Wynik |
| --- | --- |
| Kliknięcie bryły / 1, 2, 3 | Wybór A, B albo materiału |
| R / Rezerwuj | Nowa próba rezerwacji trzech jednostek |
| B / Rozpocznij | Rozpoczęcie ostatniej czynności wybranego wykonawcy |
| C / Przerwij | Przerwanie z zachowaniem już zużytego materiału |
| Spacja | Odtwarzanie/pauza; minimalizacja również zawiesza dopływ czasu |
| N / +1 s | W pauzie: 20 kroków świata |
| S / L | Zapis / odczyt z pauzą |
| Q, E / strzałki góra, dół / +, - | Obrót / nachylenie / zoom kamery |

Próba ręczna: A rezerwuje, B dostaje odmowę, A rozpoczyna; po dwóch przyrostach `+1 s` krok wynosi 40, zużyto 1, pozostały i zarezerwowany materiał wynosi 2. Zapisz, przesuń czas, wczytaj: wraca krok 40 i praca w toku. Kolejne trzy przyrosty kończą czynność w kroku 100. Kamera i statyczne pozycje znaczników są prezentacją. [Model laboratorium](window-laboratory.md) opisuje ograniczenia.

Testy z prawdziwym urządzeniem, oknem i odczytem pikseli:

```powershell
ctest --preset windows-debug -R "^lab[.]model_and_presentation$"
ctest --preset windows-debug -R "^lab[.]hardware_gpu_standard$"
ctest --preset windows-debug -R "^lab[.]hardware_gpu_integration$"
```

Ostatni test wymaga warstwy diagnostycznej, dwa pierwsze jej nie potrzebują. Pełny CTest nadal zawiera wszystkie trzy i zgłasza błąd przy brakującym komponencie. Nie traktuj uruchomienia wybranego podzbioru jako pełnego odbioru. Bezpośrednia próba z trwałym logiem i obrazem:

```powershell
./build/msvc/Debug/reality_lab.exe --smoke-test --report ./build/m02-gpu.log --save ./build/m02-gpu.save --capture ./build/m02-gpu.png
```

Tryb smoke jest testem rzeczywistego okna i ścieżki komunikatów wewnątrz aplikacji; nie zastępuje sprawdzenia wejścia OS. Dodanie `--gpu-debug` żąda diagnostyki D3D11 i nie dopuszcza automatycznego startu bez niej. Zwykłe uruchomienie nie wykonuje odczytu obrazu GPU.

### Systemowa warstwa diagnostyczna

Na początku M02 `--gpu-debug` zwrócił `0x887a002d` i nie znalazł systemowej warstwy SDK. Windows udostępnia ją w opcjonalnym składniku **Narzędzia grafiki / Graphics Tools**; [dokumentacja Microsoft](https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-devices-layers) wyjaśnia wymaganie. Administrator może zainstalować składnik w funkcjach opcjonalnych Windows albo poleceniem opisanym przez [Microsoft](https://learn.microsoft.com/en-us/windows/ai/directml/dml-debug-layer):

```powershell
Add-WindowsCapability -Online -Name "Tools.Graphics.DirectX~~~~0.0.1.0"
```

To polecenie wymaga uprawnień administratora. Po zgodzie użytkownika agent uruchomił podniesiony [skrypt](../tools/install_graphics_tools.ps1). Instalacja została zweryfikowana jako `installed`, bez konieczności restartu; [zachowany wynik](verification/evidence/m02-graphics-tools-install.json) potwierdza zakończenie. Potem wykonano test z `--gpu-debug` i pełne zestawy Debug, RelWithDebInfo oraz ASan, wszystkie zaliczone. Na innym komputerze po instalacji również należy wykonać wymagający test; sam status składnika nie dowodzi braku ostrzeżeń. ASan kontroluje inną klasę błędów i nie zastępuje diagnostyki GPU.

## Higiena środowiska

Przed zmianą obejrzyj `git status`, rzeczywisty diff i historię repozytorium. Nie zakładaj, że brak narzędzia w PATH oznacza brak instalacji. Nie aktualizuj SDK, kompilatora ani zależności przy okazji naprawy bez uzasadnienia i zapisu.

Katalogi build i cache są odtwarzalne. Dane pomiarów, zapisy regresji i raporty nie mogą znikać tylko dlatego, że narzędzie porządkuje build. Każda operacja usuwania lub przenoszenia musi mieć sprawdzony zakres i nie naruszać cudzej pracy.

## Diagnostyka przestrzennego laboratorium

`reality_traversal.exe` jest rozwijanym torem przestrzennym. F6 zapisuje osobny katalog z `world.save` i `diagnostic.txt`; F5 pozostaje zwykłym zapisem, a F9 zwykłym odczytem. Domyślny katalog diagnostyki to `traversal-diagnostics` obok ścieżki `--save`; zmienia go `--diagnostics PATH`. Istniejący katalog pojedynczego przypadku nie jest nadpisywany. Plik `world.save` można wskazać zwykłemu `--save` i wczytać klawiszem F9. `diagnostic.txt` nie jest formatem świata.

W inspektorze `navigation_actor` pokazuje klasyfikację obserwatora. `suspected_stall` oznacza brak poprawy odległości do tego samego końca Walk o 0,1 m przez 100 obserwowanych kroków. Nie jest dowodem braku dowolnej trasy ani nową decyzją NPC. Kolejka, przejazd i brak planu mają inne klasyfikacje; ich oczekiwanie nie jest alarmem tego obserwatora. Automatyczne migawki mają limit ośmiu prób na uruchomienie; F6 pozostaje dostępne po tym limicie. Błąd zapisu jest widoczny w oknie i logu.

Przykład jawnego pomiaru domyślnej sceny (odrębne, nowe ścieżki dowodów):

```powershell
./build/msvc/RelWithDebInfo/reality_traversal.exe --measure-seconds 300 --warmup-seconds 30 --report ./build/m04-measure.log --save ./build/m04-measure.save --diagnostics ./build/m04-measure-diagnostics
```

Pomiar wymaga jawnych `--save` i `--report`, czasu 1–1800 s oraz przygotowania 0–300 s. Nie łączy się ze smoke. Okno samo uruchamia świat, zapisuje stan początkowy, po czasie kończy pomiar i zapisuje `*.final.save`. Nie należy używać ścieżki ważnego zwykłego świata. Klatki są odstępami pomiędzy zakończeniami prezentacji; obejmują koszt aplikacji i oczekiwanie prezentacji. `simulation_*` mierzy tylko `SimulationPacer::update`, a `backlog_*` pozostały czas zegara. Pamięć jest próbkowana raz na sekundę: to wartości procesu, bez osobnego kosztu VRAM. Percentyle są najbliższą rangą, a maksima i średnia nie odrzucają odstających próbek.

Raport zawiera interwencje klawiaturą/myszą, minimalizacje, resize, klatki zasłonięte, błędy symulacji i faktyczną liczbę kroków. `measurement_finished=1` potwierdza zakończenie czasu, nie osiągnięcie budżetu ani spokojny przebieg. Miarodajna próba wymaga również kontroli tych warunków oraz braku konkurencyjnych ciężkich zadań. Nie wyciągamy z samych czasów klatek wniosku o wąskim gardle GPU. Po zakończeniu czytamy także stan świata i migawki, aby nie pomylić wydajnego bezruchu ze sprawnym działaniem społeczności.

Test procesu `python tools/test_traversal_diagnostics.py --exe ./build/msvc/Debug/reality_traversal.exe` wymaga GPU i Narzędzi grafiki. Sprawdza rzeczywisty raport, zapis F6, kontynuację nowego procesu, błędne argumenty oraz krótki pomiar. Jest włączony do CTest; nie jest benchmarkiem FPS ani ludzką oceną sterowania.


### Domknięcie inspektora i pilnych potrzeb

Inspektor Win32 konwertuje LF do CRLF wyłącznie na granicy widoku i zachowuje zaznaczenie oraz pionową pozycję przewijania przy odświeżaniu. Native smoke sprawdza `inspector_records=pass` i `inspector_scroll=pass`. Automatyczne F6 reaguje na nowy alarm konkretnego trwałego ID, z zachowanym limitem ośmiu prób na uruchomienie; ręczne F6 jest niezależne. Pełna semantyka obserwacji, pilności i zewnętrznego zapisu wersji 2 znajduje się w [modelu autonomii](traversal-autonomy.md). Dawny pomiar 300 s dotyczy wcześniejszej wersji programu.


Obserwacja zachowanego świata z opcjonalnymi dowodami: `reality_traversal_audit_tests --resume-course PATH --diagnostics NOWY_KATALOG`. Wykonuje 1000 walidowanych kroków, raportuje nowe alarmy i zapisuje najwyżej osiem migawek; odmawia istniejącego katalogu. Nie zapisuje pliku wejściowego. Bez `--diagnostics` pozostaje obserwacją bez zapisów. Rzeczywiste pozostałe zgłoszenie opisuje [stagnacja zachowanego świata](work-items/retained-world-navigation-stall.md).


### Lokalna prośba o przejście

F3 w `reality_traversal` prosi blokującego rozpoczęty Step o zrobienie miejsca. Ten sam kontrakt ma automat. Odpowiedź jest widoczna; zgoda nie oznacza już wykonanego przejścia. Szczegóły i zewnętrzny zapis wersji 3 opisuje [model autonomii](traversal-autonomy.md#lokalna-prośba-o-przejście).

`reality_clearance_tests --capture-retained NOWY_PLIK` zapisuje rzeczywistą migawkę w toku zgody z zachowanego świata (wersja 3), odmawiając istniejącego celu. Można wskazać ten plik w `reality_traversal --save PATH` i wczytać F9. Nie nadpisuje źródłowego materiału dowodowego. Zestaw CTest `clearance.*` obejmuje 13 scenariuszy tego przyrostu. Natywne smoke sprawdza też `clearance_shortcut=pass` dla niepotrzebnej prośby, która nie może mutować świata.
