# Odbiór reakcji na blokadę ruchu

Status: ZALICZONE w opisanym zakresie technicznym, 26 października 2026. [Zakres i dalsze zachowania społeczne](../work-items/navigation-obstruction-recovery.md). Nie jest to odbiór dowolnego tłumu, ocena komfortu przez użytkownika ani otwarcie M04.

## Obserwacja i potwierdzone przyczyny

Użytkownik przekazał trzy zrzuty postaci zmieniającej kierunek bez dojścia do celu, obok innego człowieka i kładki. Poprzednie okno zostało zamknięte bez zapisu; log `build/play-20261026-100025.log` zawiera normalne zakończenie, ale nie stan postaci. Nie odtworzono dokładnie tej sesji. Sama obecność człowieka na kładce może legalnie uniemożliwiać przejście; bezcelowe krążenie nie jest oczekiwanym ograniczeniem produktu.

Odtworzono niezależnie dwa błędy:

1. W korytarzu o szerokości 0,8 m stoi kapsuła o promieniu 0,3 m. Środki innych kapsuł muszą pozostać na podparciu, więc nie mogą wyminąć jej w tym przekroju. Po nieudanej próbie z uwzględnieniem stojącego sąsiada ponowienie po 40 krokach używało planowania bez sąsiadów. Cykl ponownie uruchamiał niewykonalną trasę. W kroku 201 wykonawca nadal przemieszczał się z prędkością 1,181 m/s, mimo niezmiennej blokady.
2. Zajęta kładka kierowała oczekującego do punktu, w którym stał inny człowiek. Wybór punktu sprawdzał geometrię, ale nie kapsuły ludzi. W kroku 301 oczekujący nadal poruszał się z prędkością 2,114 m/s obok zajętego punktu, zamiast osiągnąć stabilną pozycję oczekiwania.

Pierwsze wyniki: `build/navigation-oscillation-before-tests.log`, `-before-matrix.log` i `-before-parking.log`. Błąd zamkniętego korytarza oraz błąd parkowania były czerwone przed zmianą produkcji. Otwarte warianty ściana–sąsiad przechodziły już na bazie; nie przedstawiamy ich jako nowo naprawionego defektu. Hipoteza o zerowaniu licznika przez drobny ruch nie była potrzebna do tych dwóch reprodukcji i nie została uznana za udowodnioną przyczynę zrzutów.

## Naprawa i trwałe reprodukcje

Ponowienia po braku trasy korzystają z aktualnego widoku lokalnych stojących sąsiadów. To ponowna obserwacja, nie wieczne przechowywanie dawnego blokującego. Cel nie jest przesuwany. Kandydat na oczekiwanie przy kładce lub promie dodatkowo sprawdza kolizję końcowej kapsuły z aktualnymi ludźmi. Przy braku wolnego kandydata oczekiwanie pozostaje w aktualnej pozycji. FIFO, ograniczenia percepcji, prędkość i fizyczne sprawdzenie faktycznego ruchu są zachowane.

Inspektor dodaje nazwę wyniku planowania, termin ponowienia oraz wskazanie przejścia/usługi i właściciela podczas czekania w kolejce. Nie ma nowego stanu zapisu; `GENESIS_TRAVERSAL 1` i historyczne formaty pozostają bez zmiany.

Kod sesji sprzed naprawy zachowano w `build/navigation-oscillation-baseline/`. Zbudowano osobny `obstruction_baseline.exe`, używający starej sesji i bieżącego runnera regresji, ze wspólnymi niezmienianymi bibliotekami. To reprodukcja różnicy sesji, nie pełny historyczny build projektu. Oba przypadki ponownie zakończyły się kodem 1 i błędem ruchu, przed nowymi asercjami diagnostyki: `build/navigation-oscillation-baseline-blocked_route_settling.log` oraz `-occupied_queue_parking.log`.

Nowe przypadki CTest: `blocked_route_settling`, `occupied_queue_parking`, `wall_neighbour_recovery` i sześć wierszy `wall_neighbour_matrix_0`–`5`. Macierz obejmuje tylko fizycznie poprawne pozycje początkowe wokół obróconej ściany; pozycje wewnątrz ściany nie są scenami ruchu. Każdy wiersz musi rzeczywiście uruchomić poprawne konfiguracje. Odbiór wymaga dotarcia, nie samej obecności trasy. W zamkniętym korytarzu sprawdzamy stabilność przez 80 kroków: pozycja w promieniu 1 cm i obrót poniżej 0,05 rad, ograniczoną częstotliwość ponowień i późniejsze dotarcie po odejściu blokującego. Analogiczna stabilność w kolejce trwa 60 kroków. Oba przypadki porównują identyczną kontynuację po zapisie. Walidacja fizyki i pięciu próbek prezentacji działa w każdym kroku.

## Wyniki

| Konfiguracja | Build | Pełny CTest | Log |
| --- | --- | --- | --- |
| Debug | PASS | 140/140 PASS | `build/navigation-oscillation-debug-tests.log` |
| RelWithDebInfo | PASS | 140/140 PASS | `build/navigation-oscillation-release-tests.log` |
| ASan / RelWithDebInfo | PASS | 142/142 PASS | `build/navigation-oscillation-asan-tests.log` |

Polecenia: `cmake --build --preset windows-debug`, `windows-release`, `windows-asan` i odpowiadające im `ctest --preset ... --output-log ...`. Dodatkowe dwa testy ASan sprawdzają diagnostykę. Żadnego wcześniejszego przypadku nie wyłączono; limitów czasu nie zwiększono. Nową macierz rozdzielono na sześć samodzielnych przypadków, a próbę dotarcia kończymy po rzeczywistym osiągnięciu celu zamiast wykonywać zbędne kroki po sukcesie. Końcowe 9/9 nowych przypadków Debug sprawdzono ponownie po uporządkowaniu kolejności asercji diagnostycznych: `build/navigation-oscillation-final-focused.log`.

`cmake --build --preset windows-debug --target format-check`: PASS. Kontrola dokumentacji oraz testy narzędzi w pełnych CTest: PASS. Odrębna końcowa kontrola dokumentacji po uzupełnieniu raportu: PASS. Sumy źródeł i testu: `build/navigation-oscillation-source-hashes.json`.

## Okno i GPU

Wykonano rzeczywistą próbę wejścia przez Computer Use na RelWithDebInfo z `--gpu-debug`, NVIDIA GeForce MX330. Runner `--capture-queue-wait build/navigation-oscillation-queue.save` tworzy zapis po 360 krokach stabilnej sceny oczekiwania. F9 wczytało go w oknie, N wykonało 20 kroków: pozycja oczekującego pozostała `(-3,955; 0; 0,838)`, prędkość 0, yaw 0,670. Właścicielem kładki była postać 2.

Wybrano postać 2 klawiszem 2 i PPM zadano jej cel na wschodnim brzegu. Po uruchomieniu czasu rzeczywistego zeszła fizycznie z kładki, a postać 1 samodzielnie przejęła przejście i osiągnęła swój wcześniejszy cel. Pauza w kroku 926 i F5 zachowały wynik; obie postaci miały stan Arrived i prędkość 0 na powierzchni 2. Trzecia postać pozostała w dawnym zajętym miejscu. Okno zamknięto normalnie.

Dowody: `build/navigation-oscillation-interactive.log` (`gpu_debug=1`, MX330, `normal_shutdown=1`), początkowy `build/navigation-oscillation-queue-reproducer.save`, końcowy `build/navigation-oscillation-queue.save` oraz obrazy/odczyty w historii narzędzi. Próba potwierdza wykonanie ruchu w aplikacji; nie jest nową akceptacją komfortu przez użytkownika.

Ponadto wykonano `reality_traversal.exe --smoke-test --gpu-debug` i osobny proces `--smoke-test --resume-smoke --gpu-debug` dla Debug, RelWithDebInfo i ASan. Wszystkie sześć procesów przeszło diagnostykę D3D11 na MX330, zakończyło się `TRAVERSAL SMOKE PASS` i normalnym zamknięciem. Dla każdej konfiguracji suma stanu kontynuacji z nowego procesu zgadzała się z pierwszym (`12755068505866310387`). Logi i obrazy: `build/navigation-oscillation-gpu-{debug,release,asan}.*` oraz odpowiednie `-resume.*`. To próby poprawności, bez pomiaru FPS i skalowania populacji.

## Granice i dalsze zachowanie

Nie dowiedziono rozwiązania każdego układu z trzema lub większą liczbą poruszających się ludzi. Lokalny wybór miejsca nie rezerwuje go przed przyszłymi ruchami, a lokalne sterowanie i graf widoczności nadal mają ograniczenia. `NoPath` pozostaje wynikiem ograniczonego wyszukiwania w aktualnym modelu. Nie jest gwarancją braku dowolnego ciągłego obejścia. Dokładny przypadek użytkownika wymagałby zapisu sesji.

Komunikacja, prośby o ustąpienie, zgoda, odmowa i konflikt społeczny pozostają jawnym projektem w briefie. Naprawa nie nadaje postaci prawa przesuwania innej osoby ani wymuszania ruchu gracza. Zatrzymanie i ponowienie są obecną bezpieczną podstawą, nie docelową pełną autonomią społeczną.
