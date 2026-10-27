# Dopracowanie prowadzenia do celu

Status: implementacja i zakres techniczny ZALICZONE; [raport](../verification/navigation-polish.md) zawiera dowody i osobno otwartą ocenę nowego odczucia przez użytkownika. Przyrost po odbiorze M03 został zatwierdzony przez użytkownika. Cel: bezpośredniejsze trasy, przejmowanie nowego celu od bieżącej pozycji i stabilne zakończenie ruchu. M04 pozostaje następnym etapem. Nie zmieniamy prędkości 3 m/s, kroku 50 ms, zakresu fizyki, kamery ani autonomii.

## Zakres i kontrakty

Własne A* pozostaje czterokierunkową referencją najkrótszej drogi po siatce. Nowy rzeczywisty konsument planowania ruchu najpierw sprawdza bezpośredni odcinek; po wyszukiwaniu usuwa zbędne punkty tylko przez sweep całego obrysu i wysokości. Skracanie ma limit sprawdzeń, po którym zachowuje poprawny pozostały odcinek trasy. Nie obiecujemy najkrótszej drogi w przestrzeni ciągłej ani wygładzania krzywymi.

Zmiana celu planuje od aktualnej pozycji bez jej zmiany. Ponowienie tego samego celu w tej samej wersji geometrii zachowuje postęp i plan. Zmiana geometrii nadal wymaga walidacji przed ruchem. `Waiting`, `NoPath` i `InvalidEndpoint` pozostają jawnymi wynikami; nie odpychamy postaci i nie przenosimy jej do celu. Prowadzenie po trasie nadal przechodzi fizykę przy każdym kroku.

Dotknięte kontrakty: INV-02, INV-03, INV-09, INV-10, INV-11, INV-12 i INV-16; scenariusze SC-06, SC-07, SC-08 oraz ruch z SC-11. Pliki: `src/navigation/grid_navigation.*`, `src/lab/motion_session.cpp`, testy ruchu i sprzętowej integracji, CMake oraz dokumenty opisujące stan. Zmiana wymaga odrębnego przeglądu integratora.

## Kontynuacja i porównanie

Format `GENESIS_MOTION 1` przechowuje już punkty trasy i kursor, więc nie dodajemy pól ani nie zmieniamy ich znaczenia. Istniejące poprawne zapisy zachowują zapisane trasy; nowe planowanie może wyznaczyć krótszą. Przygotować rzeczywisty zapis podróży starym programem i sprawdzić odczyt w nowym procesie, bez resetu ID, pracy i czasu.

Przed zmianą zachować lokalny program bazowy i jego SHA-256 oraz reprodukcje mechanicznego dojścia przez centra komórek i powtarzanego planowania identycznego celu. Porównanie dotyczy punktów, długości trasy i przebiegu ruchu, nie FPS ani rzekomej inteligencji NPC.

## Kryteria odbioru

- Otwarty odcinek daje jeden cel i ruch zgodny z kierunkiem od bieżącego położenia, bez wstępnego dojścia do centrum komórki.
- Nowy cel podczas marszu nie przenosi postaci; identyczny cel nie resetuje kursora ani licznika planowań. Zmiana geometrii nie powoduje zbędnego cofania na wolnym odcinku.
- Skrócone odcinki respektują narożniki, rozmiar, prześwit i cienkie przeszkody; nie są dłuższe niż wejściowa łamana. Test BFS surowego A* pozostaje bez osłabienia.
- Dojście kończy ruch, kolejne kroki nie wywołują krążenia. Blokada przez drugą postać daje stabilne oczekiwanie, a brak przejścia i niedozwolony cel są jawne.
- Zapis/odczyt w podróży, przy blokadzie i zmianie celu zachowują kontynuację; stary plik przechodzi nowy proces.
- Pełne Debug, RelWithDebInfo i ASan, format, kontrola dokumentów, GPU z debug layer oraz próba rzeczywistej myszy i klawiatury. Oceny nowego odczucia przez człowieka nie zastępujemy automatem ani wcześniejszą akceptacją wersji bazowej.
