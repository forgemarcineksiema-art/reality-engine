# Odbiór pierwszego przyrostu przed dostawą

Status: ZALICZONE w opisanym zakresie diagnostyki, zapisu reprodukcji i pomiaru małej sceny. [Projekt](../work-items/m04-entry-design.md), [plan](../work-items/m04-entry-observability.md). Dostawa, transfer materiału i współpraca społeczna nie są zaimplementowane w tym przyroście. Nie jest to odbiór całego M04 ani docelowej autonomii.

## Dostarczony zakres

`NavigationProgressWatch` i `TraversalDiagnostics` odczytują sesję bez mutacji świata. Rozpoznają brak poprawy o 0,1 m do tego samego końca Walk przez 100 kroków. Drobna oscylacja nie jest postępem. Zmiana celu odcinka i przerwa obserwacji większa niż 40 kroków rozpoczynają nowe okno. Bezczynność, oczekiwanie, rozpoczęte przejście/przejazd, brak drogi, odmowa celu, budżet i dojście są osobnymi klasyfikacjami. To podejrzenie diagnostyczne dla określonej klasy ruchu, bez prawa do automatycznego zmieniania celu NPC.

F6 tworzy nowy katalog z normalnym `world.save` oraz `diagnostic.txt`, z przyczyną, checksumą świata, aktualnymi obserwacjami i inspektorem. Poprzednie katalogi nie są nadpisywane. Błąd ścieżki nie zmienia świata ani wcześniejszych plików. Raport jest odświeżany względem zapisywanego świata, także gdy poprzednia obserwacja była starsza. Automatyczne migawki przy wejściu w stan podejrzenia mają limit ośmiu prób na uruchomienie; ręczny F6 działa niezależnie od tego limitu. Stan obserwatora jest przejściowy i resetowany po F9; formaty świata pozostają bez zmiany.

`RuntimeMetrics` zbiera rzeczywiste odstępy między końcami prezentacji, czas aktualizacji symulacji i zaległość zegara. Podaje min, p50, p95, p99, max oraz średnią, bez odrzucania długich klatek. Pomiar ma jawne limity czasu i pamięci; raportuje pamięć procesu, warunki okna i ingerencje. Nie mierzy osobno czasu GPU ani VRAM i nie diagnozuje na tej podstawie przyczyny spowolnienia.

## Dowody czerwone i naprawy w trakcie rozwoju

Przed implementacją test rzeczywistego okna zakończył się błędem braku obserwacji nawigacji w raporcie: `build/m04-entry-red-process.log`. Pierwotny proces smoke przeszedł, a asercja nowego zachowania wykazała brak funkcji. Testy nowego API zostały napisane przed implementacją; samego braku API nie przedstawiamy jako zaliczonej reprodukcji runtime.

Dodatkowe próby wykazały błędy własnego przyrostu przed poprawkami:

- Odrzucona próbka bez zamiaru zmieniała checkpoint obserwatora; przeniesiono całą walidację przed zmianę jego stanu.
- Suma bardzo dużych, ale skończonych próbek przepełniała średnią; średnia jest teraz liczona przyrostowo bez sumy przekraczającej zakres.
- Błędny lub brakujący parametr pomiaru mógł otwierać modalne okno zamiast zwrócić kod 1; tryb bezobsługowy jest identyfikowany przed parsowaniem.
- Migawka świata z późniejszego kroku mogła zawierać starszą obserwację; capture odświeża jej kopię bez zmiany żywego obserwatora.

Dowody: `build/m04-entry-review-red.log`, `-cli-red.log` i `-snapshot-red.log`; po naprawach pełne zestawy poniżej są zielone. Testy obejmują rzeczywisty system plików, ścieżkę Unicode, istniejący katalog, ścieżkę blokowaną plikiem, transakcyjną odmowę próbki, małe oscylacje, powolny postęp, oczekiwanie rzeczywistej kolejki, reset zegara i odczyt nowego procesu. Nie użyto atrap fizyki ani fikcyjnego ukończenia ruchu.

## Konfiguracje

| Konfiguracja | Build | Pełny CTest | Dowód |
| --- | --- | --- | --- |
| Debug | PASS | 147/147 PASS | `build/m04-entry-final-debug-tests.log` |
| RelWithDebInfo | PASS | 147/147 PASS | `build/m04-entry-release-tests.log` |
| ASan / RelWithDebInfo | PASS | 149/149 PASS | `build/m04-entry-asan-tests.log` |

Polecenia: `cmake --build --preset windows-debug`, `windows-release`, `windows-asan` oraz odpowiadające `ctest --preset ... --output-log ...`. Buildy i pełne wyniki mają logi `build/m04-entry-*`. Dwa dodatkowe przypadki ASan sprawdzają działanie samej diagnostyki. Wszystkie poprzednie regresje zachowano. Nie zwiększano limitów czasu ani nie wyłączano przypadków. Test procesu obejmuje dwa procesy z diagnostyką GPU, właściwy plik F6 i krótki ograniczony pomiar. Wybrano MX330. Te krótkie próby nie są benchmarkiem.

Końcowe próby procesu powtórzono z trwałymi logami dla wszystkich trzech konfiguracji: `build/m04-entry-native-{debug,release,asan}/`. Każda zawiera dwa udane procesy diagnostyki GPU z MX330, migawkę, kontynuację i krótki pomiar. Po dodaniu wyłącznie obserwacyjnego polecenia runnera zbudowano go także w Debug i ASan oraz wykonano tę samą kontynuację końcowego pliku w trzech konfiguracjach; wszystkie walidacje przeszły, a liczniki ukończeń były zgodne. Dowody `build/m04-entry-measure-continuation*.log`, hashe runnera `build/m04-entry-observation-tool-hashes.json`. Nie zmieniano po pomiarze kodu produkcyjnego ani mierzonego exe.

Końcowy `format-check`: PASS (`build/m04-entry-final-debug-build-format.log`). Kontrola dokumentacji i testy narzędzi w pełnych zestawach: PASS; kontrola dokumentów jest powtarzana po uzupełnieniu końcowych wyników. Hashe źródeł, testów, CMake i binarnego RelWithDebInfo: `build/m04-entry-source-hashes.json`. Odczyt CPU: `build/m04-entry-hardware.json`.

Po dodaniu obserwacyjnego polecenia i aktualizacji dokumentów ponowiono `format-check`: PASS (`build/m04-entry-final-format.log`), a końcowe `python tools/check_docs.py`: PASS. Podsumowanie liczb pomiaru: `build/m04-entry-measure-summary.json`.

## Rzeczywiste wejście i ograniczenie narzędzia

Na rzeczywistym oknie RelWithDebInfo/MX330 z debug layer wykonano N do kroku 20, F5 oraz F6. Powstał osobny katalog `build/m04-entry-ui-diagnostics/case-150934968-1/`, z poprawnym `world.save` i obserwacjami z kroku 20. Instrukcja F6 i legenda były widoczne w panelu. Po kolejnym N narzędzie przy próbie F9 zgłaszało równoległe wejście i zminimalizowane okno; nie uznajemy tej próby rzeczywistego F9 za wykonaną. Automatyczna próba natywnego F9, jednostkowy reset obserwatora oraz odczyt migawki nowym procesem przeszły.

Po uzgodnieniu z użytkownikiem dostępności komputera poprzedni proces testowy zakończono kontrolą procesu po zachowaniu F5/F6. Nie ma dla niego dowodu normalnego zamknięcia przez UI. Nie przedstawiamy tego jako zaliczenia brakującego wejścia F9.

Po zakończeniu pomiaru zresetowano sesję narzędzia Computer Use i wykonano osobną końcową próbę: świeże okno pokazało krok 0, rzeczywisty klawisz F9 wczytał zapis kroku 20 i wyświetlił „Wczytano świat. Pauza.”. Następnie Alt+F4 zakończyło okno normalnie. Dowód: obrazy w historii narzędzi i `build/m04-entry-ui-final.log` (`gpu_debug=1`, MX330, `normal_shutdown=1`). W ten sposób brakujący odbiór rzeczywistego F9 został wykonany, bez reinterpretowania wcześniejszych nieudanych prób. Nie jest to nowa ocena komfortu przez użytkownika.

## Pomiar dłuższego działania

Próba rozpoczęta 26.10.2026 o 13:12:48 +01:00. RelWithDebInfo, i5-1035G1, MX330, domyślne osiem postaci, widok 844×781, bez warstwy debug GPU; 30 s przygotowania i 300 s pomiaru. Użytkownik potwierdził dostępność widocznego okna przez około sześć minut. W tym czasie nie uruchamiano buildów i innych testów ani nie sterowano oknem. Nie izolowano systemu operacyjnego i wszystkich usług w tle; to pojedyncza kontrolowana próba aplikacji na laptopie, bez porównania przyczynowego z inną wersją.

Proces zakończył się kodem 0 i normalnym zamknięciem. Raport: `build/m04-entry-measure-300.log`; stan początkowy: `build/m04-entry-measure-300.save`; końcowy: `build/m04-entry-measure-300.save.final.save`; czas uruchomienia: `build/m04-entry-measure-start.txt`. Parametry: `--measure-seconds 300 --warmup-seconds 30 --report ... --save ... --diagnostics build/m04-entry-measure-diagnostics`.

| Wskaźnik | Wynik |
| --- | --- |
| Próbki klatek | 17 952 |
| Mediana / p95 / p99 | 16,6673 / 17,0142 / 17,3912 ms |
| Najdłuższa klatka / średnia | 51,4367 / 16,7112 ms |
| Średnie tempo prezentacji | około 59,84 FPS |
| Aktualizacja symulacji p95 / p99 / max | 1,8725 / 3,3141 / 13,6145 ms na wywołanie aktualizacji |
| Maksymalna pozostałość zegara | 49,8490 ms, poniżej jednego kroku 50 ms |
| Kroki w części mierzonej | 6000 |
| Faktyczny czas próbek | 299 999 ms |
| Working set: pierwszy / ostatni / szczyt próbkowany | 64 434 176 / 76 955 648 / 76 955 648 B |
| Prywatna pamięć: szczyt próbkowany | 97 447 936 B |
| Ingerencje / minimalizacje / resize / zasłonięte klatki / błędy symulacji | 0 / 0 / 0 / 0 / 0 |

Wynik potwierdza wykonanie bieżącej małej sceny blisko tempa 60 Hz, z zachowaniem wszystkich długich klatek. Nie dowodzi stałych 60 FPS, widoku 1280×720, kosztu większej populacji ani braku sporadycznych przycięć. Wzrost working set około 12 MiB nie jest sam w sobie dowodem wycieku; obejmuje między innymi rozwój stanu aplikacji i próbek. Nie przypisujemy długiej klatki GPU, temperaturze lub konkretnemu subsystemowi bez osobnego pomiaru.

### Co zrobił świat

Koniec próby to krok 6599. Ukończono 9 posiłków, 4 odpoczynki i 7 konserwacji. Pozostało 77 i 74 jednostki jedzenia; część mieszkańców miała maksymalny głód, kontynuując wcześniejsze zadania lub oczekując na transport. Nie stwierdzono alarmu tego ograniczonego detektora Walk i nie powstała automatyczna migawka. To nie oznacza dobrego zaspokajania potrzeb ani rozwiązania wszystkich kolejek.

Aby odróżnić chwilowy obraz kolejki od zakleszczenia, po pomiarze dodano do runnera testowego odrębne polecenie obserwacyjne `--resume-course PATH`: wczytuje wskazany plik, wykonuje i waliduje 1000 kroków, drukuje stan i nie zapisuje pliku wejściowego. Wykonano je na końcowym świecie w RelWithDebInfo; `build/m04-entry-measure-continuation.log`. Świat doszedł do kroku 7599 i ukończył dalsze 4 posiłki, 2 odpoczynki i konserwację. Osoby wcześniej obecne na kładce i w fazie wysiadania przeszły dalej; prom i właściciele przejść zmieniły stan. Nie jest to reprodukcja trwałego zakleszczenia.

Wniosek projektowy: kolejka ma koszt czasu, a obecny wybór potrzeb nie przerywa aktywnego planu tylko dlatego, że w podróży wzrósł głód. Przed dostawą należy jawnie rozstrzygnąć pilność, koszt przerwania i bezpieczne dokończenie rozpoczętego kroku/przejazdu. Nie zmieniano arbitralnie tempa głodu, prędkości, kolejki ani fizyki w celu uzyskania lepszego wyniku. Scena nie produkuje żywności i nie jest samowystarczalną gospodarką.

## Przegląd i granice

Przegląd kodu wykonał autor, zgodnie z zachowanym ograniczeniem pracy bez delegowania. To nie jest niezależny review. Sprawdzono: brak mutacji świata przez obserwację, zgodność zapisu, odrzucanie danych przed zmianą checkpointu, przygotowanie pamięci przed dopisaniem próbek, skończone statystyki, brak nadpisania katalogu dowodów, kontrolę Unicode i jawne ograniczenia pomiaru. Wymienione błędy rozwoju mają dowody przed/po.

Obserwator nie rozpoznaje każdego zakleszczenia. Ciągła zmiana końca odcinka lub przechodzenie w Waiting mogą resetować okno; legalne oczekiwanie nie ma jeszcze niezależnego miernika skuteczności parkowania. Automatyczny zapis reaguje na przejście sumarycznego alarmu z false na true, więc nowy problem innej osoby podczas już trwającego alarmu może wymagać ręcznego F6. Migawka nie jest historią wszystkich dawnych poleceń i trzymanych klawiszy. Wydajność jednej sceny nie dowodzi liczebności całej cywilizacji ani samowystarczalnej gospodarki.

Drobna poprawa interfejsu pozostaje odłożona: wielowierszowy inspektor Win32 zawija długi tekst i nie rozdziela czytelnie wszystkich rekordów. Dane w pliku `diagnostic.txt` zachowują osobne wiersze. Nie traktujemy tego jako docelowej jakości inspektora; formatowanie samego widoku nie jest powodem do zmiany świata lub jego zapisu.

Przygotowano [projekt transferu materiału](../work-items/material-transfer-design.md), wskazujący także brak domenowych typów materiału i wymagane rozszerzenie bilansu. Pozostaje propozycją, nie działającą dostawą.
