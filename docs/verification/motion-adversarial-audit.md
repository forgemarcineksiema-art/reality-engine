# Audyt praktycznych przypadków nawigacji, kolizji i obrazu

Aktualizacja po odbiorze: użytkownik potwierdził „Płynność teraz jest okej”. To akceptacja płynności tej wersji. Następnie zlecił [rozszerzenie ruchu i autonomii](../work-items/advanced-traversal.md); jego przyszłe zachowanie wymaga osobnej oceny, a poniższy raport zachowuje zapis wcześniejszego odbioru technicznego.

Werdykt dla poprzedniej implementacji: **REQUEST CHANGES**, pewność **HIGH** dla odtworzonych przypadków. Po poprawkach: **APPROVE w opisanym zakresie technicznym**; ocena odczuć nowej wersji przez użytkownika pozostaje niewykonana. [Brief](../work-items/motion-adversarial-audit.md) określa zakres; poprzednie zaliczenie testów nie stanowiło pokrycia wszystkich praktycznych scenariuszy. Audyt całego wskazanego subsystemu i poprawki wykonano na zlecenie użytkownika. Użyto umiejętności code-review; nie wykonano niezależnej recenzji drugiego autora.

## Wyniki przeglądu

| Priorytet | Problem poprzedniej implementacji | Miejsce | Stan poprawki |
| --- | --- | --- | --- |
| P1 | Fizycznie poprawna pozycja na kontakcie była odrzucana, gdy centrum jej komórki wpadało w ścianę; zmiana celu nie mogła uruchomić objazdu | [Łączenie końców A*](../../src/navigation/grid_navigation.cpp) | Przyłącza do wolnych sąsiednich centrów, sprawdzane całym obrysem |
| P1 | Arytmetyczne dojście do dokładnego styku mogło skończyć się kilka ulp wewnątrz przeszkody i zatruć następny ruch jako `InvalidStart` | [Kontakt końcowy](../../src/physics/kinematic.cpp) | Dokładna płaszczyzna styku przy końcu ruchu, bez zwiększania tolerancji |
| P1 | Renderer pokazywał bezpośrednio pozycje zmieniane co 50 ms, co dawało kroki obrazu przy wyższej częstotliwości klatek | [Ruch](../../src/lab/motion_session.cpp), [scena](../../src/graphics/lab_scene.cpp), [Win32](../../src/platform/lab_main.cpp) | Interpolacja rzeczywistej drogi ostatniego kroku, nie cięciwy przez narożnik |
| P2 | Oczekiwanie za stojącą osobą nie podejmowało próby obejścia mimo wolnej podłogi | [Podążanie trasą](../../src/lab/motion_session.cpp) | Ograniczone ponowienie z aktualnym obrysem sąsiada |
| P2 | Różnica dwóch poprawnych współrzędnych była walidowana jak pojedyncze położenie, odrzucając poprawny duży odcinek | [Wektor ruchu](../../src/physics/kinematic.cpp) | Osobny limit wektora do ±2 000 000 m |
| P2 | Bardzo mała komórka i bezwzględna tolerancja pokrycia mogły dopuścić niespójną siatkę i niebezpieczne zawężenie indeksu; przepełniony rozmiar siatki również przechodził walidację | [Walidacja siatki](../../src/navigation/grid_navigation.cpp) | Minimum 1 mm, tolerancja uwzględniająca skalę oraz precyzję współrzędnych i jawne odrzucenie nieskończonych rozmiarów |

### Pozycja przy ścianie

[Reprodukcja przed naprawą](evidence/motion-audit-before.log): A doszedł do x≈−0,600001 m przy zamkniętej głównej drodze. Kolizja uznawała pozycję za poprawną, lecz centrum komórki x=−0,5 m leżało we wnętrzu rozszerzonej przeszkody. Planer zwracał `InvalidEndpoint`, zanim próbował objazdu. Naprawa zachowuje punkt i dodaje lokalne, sprawdzane połączenia do centrów sąsiednich komórek. Cel blisko ściany otrzymał osobny test. To reprodukcja odpowiadająca zgłoszeniu; nie twierdzimy, że znamy dokładną sekwencję wcześniejszej próby użytkownika.

### Blokada przez drugą osobę

Stary konsument zatrzymywał się na fizyce i próbował tego samego zablokowanego odcinka bez planowania obejścia. Nowa próba dodaje aktualny obrys sąsiada do tymczasowej geometrii planera, bez drugiego autorytetu pozycji. Próby są rozłożone w czasie według ticka świata i ID, a przy wzajemnym oczekiwaniu pierwszeństwo obejścia ma większe ID. Niedostępny objazd pozostawia `Waiting`. Cel zajęty przez nieruchomą osobę nie jest ogłaszany jako osiągnięty i nie powoduje przeniesienia postaci.

### Zaokrąglenie dokładnego styku

Dodatkowy test wykazał [porażkę już w pierwszej operacji](evidence/motion-audit-contact-rounding-before.log), seed `0x636f6e74`: samo `start + delta` umieszczało wynik minimalnie wewnątrz rozszerzonej ściany. Naprawa zachowuje dokładne płaszczyzny wyznaczonego kontaktu końcowego i pełną składową styczną. Nie zwiększono tolerancji asercji, nie dodano wypychania niepoprawnego startu ani tolerowania faktycznego nakładania brył. 1000 podejść pod skosem sprawdza teraz także wolny odcinek do styku, legalny wynik i możliwość odejścia w następnym ruchu.

### Obraz pomiędzy krokami

Symulacja pozostaje przy 50 ms i 3 m/s. Kinematyka zwraca do sześciu punktów przebiegu ruchu w kroku, z czasem kontaktu oraz ślizgiem. Konsument składa je także przez zakręty trasy. Renderer próbkuje tę pochodną historię według części kroku pozostałej w akumulatorze, z opóźnieniem do jednego kroku i bez ekstrapolacji. Obraz drugiej osoby jest dodatkowo ograniczony względem obrazu pierwszej, żeby kolejność integracji nie powodowała ich wizualnego przenikania.

Historia nie zmienia świata i nie jest zapisywana. Odczyt, pauza/wznowienie, resize i zmiana geometrii synchronizują ją z aktualnymi pozycjami. Prosta interpolacja samych końców została odrzucona: test narożnika wykazuje, że jej środek znalazłby się wewnątrz przeszkody, podczas gdy próbki rzeczywistej drogi pozostają legalne.

## Sprawdzenia praktyczne i graniczne

[Nowe testy](../../tests/motion_adversarial_tests.cpp) obejmują:

- ponowne planowanie z kontaktu przy ścianie i cel blisko ściany;
- obejście stojącej osoby, mijanie i zamianę miejsc dwóch osób, z kontrolą rozłączności obrazów na pięciu fazach każdego kroku;
- 200 zmian celu, ponowienia i odczyty w trakcie, z porównaniem kontynuacji;
- 1000 sweepów przy seedzie `0x4d303341`, z 17 próbkami każdego segmentu i niezależnym testem przecięcia prostokątów; w razie błędu zachowywany jest indeks operacji;
- interpolację prostego ruchu, rzeczywisty zakręt, brak mutacji świata przez prezentację, reset po odczycie oraz niedozwolone fazy;
- duży odcinek między poprawnymi punktami, mikroskopijną niespójną siatkę, przepełniony rozmiar siatki, poprawną siatkę przesuniętą blisko granicy współrzędnych i niefinitywny cel.

Osobny syntetyczny rekord wersji 1 modeluje dawny stan `InvalidEndpoint` dla legalnej pozycji i celu. Odczyt nie ukrywa go, lecz kontynuacja albo ponowne wskazanie tego samego celu podejmuje nowe planowanie zamiast utrwalać błąd starego algorytmu. Tego syntetycznego rekordu nie przedstawiamy jako pliku wytworzonego starą aplikacją; historyczny plik w teście GPU jest osobnym dowodem.

Pierwsza próba zawierała cztery przypadki: trzy porażki i jeden sukces. Początkowy wariant testu celu przy ścianie miał wolne centrum, więc już przechodził; następnie rozszerzono dane o faktycznie zablokowane centrum. Nie przypisujemy temu pierwotnemu przypadkowi zaobserwowanej porażki. Dotychczasowe BFS, wąskie przejścia, prześwity, cienka ściana, stare zapisy, przerwania pracy i budżet czasu pozostają w regresjach.

Końcowy przegląd wykazał dodatkowo [akceptację przepełnionego rozmiaru](evidence/motion-audit-grid-overflow-before.log): skończone `double::max()` pomnożone przez 64 dawało nieskończoność zarówno w rozmiarze, jak i tolerancji, więc porównanie nie odrzucało danych. Walidacja sprawdza teraz skończoność obliczonej szerokości i głębokości. Po tej ostatniej zmianie ponowiono pełne trzy zestawy; wyniki sprzed niej nie są odbiorem końcowego kodu.

Kolejny pełny przebieg ASan miał [79/80 i timeout testu GPU](evidence/motion-audit-asan-timeout-before.log). Wszystkie trzy procesy gry zakończyły się z oczekiwanym wynikiem i runner wydrukował końcowe `PASS`, ale nadrzędny CTest przekroczył 20 s. Nie uznano tej próby za zaliczoną. Test złożony miał trzy kolejne limity po 15 s, więc limit rodzica był niespójny z ich sumą. Wyłącznie dla `motion.hardware_gpu_continuation` ustalono 50 s: 3×15 s oraz 5 s narzutu Pythona/raportu. Limity procesów, warunki sukcesu, asercje geometrii i pozostałe testy z limitem 20 s pozostały bez zmiany. To korekta kontraktu runnera z jawnym uzasadnieniem, nie dowód wydajności gry; po niej ponowiono wszystkie trzy pełne zestawy.

Próbkowanie nie dowodzi pokrycia wszystkich ciągłych trajektorii. Znane figury, analityczne przypadki cienkiej ściany i narożnika oraz seria losowa są różnymi dowodami. Sam wspólny kod fizyki i nawigacji nie zapewnia niezależnej weryfikacji — dlatego dodatkowy test penetracji nie korzysta z `can_occupy`.

## Próba rzeczywistego okna

Fixture kontaktu wyeksportowano testowym runnerem przez rzeczywiste `MotionSession`: zamknięcie przejścia, ruch do kontaktu, zapis. To nie jest ręczne dopisanie sukcesu planowania. F9 odczytał krok 60 i pozycję przy ścianie; T dał [trasę czteropunktową](evidence/motion-audit-ui-contact-route.png). Spacja uruchomiła objazd; następnie PPM wskazał nowy cel i A [dotarł do niego](evidence/motion-audit-ui-arrived.png). F9 [przywrócił kontakt i pauzę](evidence/motion-audit-ui-load-reset.png), bez ciągnięcia starej historii obrazu z odległego miejsca. [Log](evidence/motion-audit-ui.log) zachowuje MX330 i debug layer oraz normalne zamknięcie okna.

To próba Computer Use, nie ocena odczucia użytkownika. Szybką serię zmian między krokami sprawdza automat; opóźnienia narzędzia UI nie pozwalają udawać takiej samej częstotliwości kliknięć w ręcznej próbie.

Próba Computer Use poprzedzała ostatnie zaostrzenie walidacji przepełnionego rozmiaru siatki. Po nim ponowiono automatyczny test rzeczywistego okna i GPU: [log](evidence/motion-audit-gpu.log) zachowuje `hardware=1 gpu_debug=1`, `MOTION SMOKE PASS`, normalne zamknięcie i próbkę `previous_x=-2.97862`, `halfway_x=-2.9044`, `current_x=-2.83018`. Zachowano [klatkę pośrednią](evidence/motion-audit-gpu.png.interpolated.png), [początek](evidence/motion-audit-gpu.png.before.png) i [koniec](evidence/motion-audit-gpu.png). Obejrzano przechwycony obraz; sam PNG nie jest pomiarem płynności.

Polecenie tej próby z katalogu repozytorium:

```powershell
./build/msvc/Debug/reality_lab.exe --motion --smoke-test --gpu-debug --report docs/verification/evidence/motion-audit-gpu.log --save build/motion-audit-gpu.save --capture docs/verification/evidence/motion-audit-gpu.png
```

Uruchomienie przez `Start-Process -Wait -PassThru` potwierdziło kod zakończenia 0. Próba używa własnego pliku w `build/`; nie nadpisuje zapisu użytkownika.

## Odbiór końcowy

Data: 2026-10-25, Windows/MSVC x64, NVIDIA GeForce MX330. Konfiguracja i kompilacja `windows-msvc`, `windows-debug`, `windows-release` oraz `windows-asan` zakończyły się powodzeniem. Wersja release oznacza tutaj `RelWithDebInfo`. Po ostatnich zmianach konfiguracji runnera wykonano pełne zestawy, bez filtrów i skipów:

| Polecenie | Wynik | Dowód |
| --- | --- | --- |
| `ctest --preset windows-debug` | ZALICZONE, 78/78 | [Log Debug](evidence/motion-audit-debug-ctest.log) |
| `ctest --preset windows-release` | ZALICZONE, 78/78 | [Log RelWithDebInfo](evidence/motion-audit-release-ctest.log) |
| `ctest --preset windows-asan` | ZALICZONE, 80/80 | [Log ASan](evidence/motion-audit-asan-ctest.log) |

Dodatkowo zaliczono `cmake --build --preset windows-debug --target format-check`, `python tools/check_docs.py` oraz `python -m unittest discover -s tools/tests -p "test_*.py"` — 29/29 testów narzędzi po końcowej aktualizacji raportu. Kontrola dokumentów sprawdza rejestr, lokalne linki, identyfikatory i sumy archiwum; nie jest testem zachowania silnika.

Każde polecenie CTest wykonano z `--output-log` wskazującym odpowiedni powyższy plik. Dwa dodatkowe przypadki ASan sprawdzają poprawną fixture oraz wykrycie celowego przepełnienia bufora sterty. Nie są dwoma dodatkowymi scenariuszami ruchu. Przyrost zawiera 13 nowych przypadków `adversarial.*`; pełne zestawy zachowują dotychczasowe regresje domeny i laboratorium. Test GPU obejmuje również kontynuację w nowym procesie oraz niezmieniony historyczny zapis wersji 1.

[Manifest SHA-256](evidence/motion-audit-sources.json) obejmuje 41 plików kodu, konfiguracji i testów. Zapisano go po ostatniej zmianie CMake, przed końcowymi pełnymi zestawami; ponowne porównanie wykazało zero różnic. Dokumentacja i pliki dowodów nie są częścią tego manifestu. Nie ma lokalnego commitu ani zdalnego repozytorium, więc nie przypisujemy wynikom wymyślonej rewizji Git. Wcześniejsze porażki i timeout pozostają jawnie zachowane powyżej.

## Nadal otwarte

Rozpędzanie, hamowanie, orientacja ciała i animacja chodu nie zostały wdrożone. To świadomie ograniczony przyrost: potwierdzone problemy wymagały przyłączy, obsługi blokady i obrazu między krokami, a nie zmiany prędkości. Komfort poprawionego obrazu pozostaje do oceny użytkownika.

Siatka z lokalnymi przyłączami nadal nie zapewnia kompletności w przestrzeni ciągłej; wąska droga może nie mieć odpowiedniego próbkowania. Skracanie nie gwarantuje najkrótszej drogi w metrach. Dwie osoby nie stanowią dowodu działania tłumu lub rozwiązywania dowolnego zakleszczenia. Fizycznie zajęty cel jest odmową albo oczekiwaniem, bez automatycznego wybierania zastępczego celu za gracza.

Nie rozszerzano zakresu na stoki, piętra, obracane bryły, dynamikę konstrukcji ani inne platformy. Ich brak nie usprawiedliwiał odtworzonych błędów obecnego kontraktu, ale naprawy nie potwierdzają przyszłych klas geometrii. Nie wykonano benchmarku FPS, badania opóźnienia wejścia, długiego obciążenia ani testu populacji M04. M04 pozostaje nierozpoczęty.
