# Kierunek fantasy i odpowiedź na ocenę projektu

Data: 25.10.2026. Zakres: analiza dostarczonych [opinii o projekcie](../../opinion-project-assessment.md) i [celów jakości](../../maxing.md), następnie wprowadzenie doprecyzowania użytkownika do dokumentacji oraz sprawdzenie konkretnej implementacji M02. Oba pliki użytkownika pozostają zachowane bez zmiany treści; są opiniami i propozycjami, nie nadrzędną specyfikacją ani niezależnym dowodem testów.

[Snapshot materiałów wejściowych](evidence/assessment-sources.json) zawiera ich SHA-256. Kontrola rejestru, odnośników, identyfikatorów i bajtowej integralności archiwum przeszła po zmianach. Dotychczasowe scenariusze pozostają w zestawach. Po instalacji Narzędzi grafiki zaliczono pełny Debug 45/45, RelWithDebInfo 45/45 i ASan 47/47, z warstwą diagnostyczną GPU, bez filtrów ani pominięć. Wcześniejsze podzbiory oraz porażki zachowano w [raporcie M02](m02-window-laboratory.md), wraz z granicami pełnego odbioru tego etapu.

## Co przyjmujemy z oceny

Najważniejsza luka jest trafnie nazwana: poprawna infrastruktura nie dowodzi jakości Action RPG. Wcześniejszy projekt gry opisywał głównie ogólną pętlę i scenariusz techniczny. [Rozwinięty projekt gry](../game-design.md) określa teraz rolę w pierwszej sytuacji, osobiste działania, alternatywne metody, koszty, eksplorację, kierunek progresji, rytm sesji i obserwowalne kryteria doświadczenia. Są to propozycje do realizacji i prób, nie istniejąca rozgrywka.

Zachowujemy też nacisk na wspólne reguły Game/Lab, małą społeczność przed skalą, pozytywną autonomię, inspektor oraz weryfikację przerwań i zapisu. Większość tych zasad już była w architekturze; brakującym wynikiem jest ich działanie w dalszych etapach. Dopisanie drugiej listy subsystemów nie wypełniłoby tej luki.

## Co wymaga korekty interpretacji

- Liczby 9/10, 8,5/10, 6/10 i 2/10 są subiektywną oceną rozmowy o dokumentach, bez odtworzenia kodu przez autora tej oceny. Nie są benchmarkiem, procentem ukończenia ani skalą, na której można automatycznie zatwierdzić 10/10.
- Różnica między modelami użytymi w rozmowie nie dowodzi poprawności ich wniosków. Oceniamy argument, źródła i wykonaną próbę.
- Autor opinii nie miał kodu ani wyników do niezależnej kontroli. Obecny przegląd może je odczytać i uruchomić, ale przegląd własnej implementacji nadal nie jest niezależnym audytem drugiej osoby.
- Fantastyczny kierunek jest już zatwierdzony przez użytkownika w DEC-008. Propozycje Wolnego Dorzecza pozostają historyczne. Nie wymagają przepisywania silnika.
- „Ewolucja cywilizacji” jest celem produktu; nie dowodzi jej dzisiejszy renderer, nazwa EVOLUTION, dialog ani wykonanie jednej zaplanowanej czynności. Otwarta innowacja pozostaje osobnym zagadnieniem badawczym.

## Pięć obszarów i potrzebne dowody

Powtórzoną w wiadomości kategorię projektu gry rozumiemy wraz z `maxing.md` jako pięć obszarów, w tym potwierdzoną dojrzałość technologii. Nie nadajemy im nowych ocen liczbowych.

| Obszar | Zmiana wykonana teraz | Dowód, którego nadal potrzeba |
| --- | --- | --- |
| Wizja i oryginalność systemów | Zatwierdzony kierunek fantasy; hipoteza wyróżnika: trwałe przekazywanie wykonywalnych praktyk | Odtwarzalne sytuacje, w których zdobyta wiedza zmienia samodzielne działania i możliwości gracza; porównanie z prostszą rutyną |
| Architektura | Zachowane kontrakty i własność stanu; oddzielona fikcja od rdzenia; rzeczywisty konsument M01 w oknie | Zintegrowane fizyka, nawigacja, gospodarka, wiedza i zapis bez utraty niezmienników w M03–M05 |
| Planowanie i weryfikacja | Jeden plan M00–M06 uzupełniony o bramki jakości bezpośredniej gry; reprodukcja i naprawa błędu trybu testowego oraz pełny odbiór M02 | Dalsze scenariusze, pomiary i decyzje oparte na wynikach, a nie obietnicach |
| Projekt gry | Konkretna scena fantasy, metody i koszty, ruch/kamera, eksploracja, progresja, ryzyko, rytm i kryteria próby z ludźmi | Wykonane alternatywy w prototypie i obserwacja rzeczywistych graczy; kontrola, czy interesuje ich przygoda, nie tylko technologia |
| Dojrzałość technologii | Działająca aplikacja sprzętowa, zainstalowana i sprawdzona diagnostyka GPU, pełne konfiguracje i testy rdzenia/platformy M02 | Późniejszy długi przebieg, wydajność docelowej sceny, pakowanie i odtwarzanie błędów w zdefiniowanym zakresie |

Wysoka jakość dotyczy zdefiniowanego produktu. Nie oznacza dowolnej skali, kompletnej fizyki całej rzeczywistości ani gwarancji, że każdy recenzent przyzna maksymalną ocenę. Kryteria mogą wykazać również, że mechanika nie działa i wymaga uproszczenia.

## Zatwierdzona wizja a wykonanie

DEC-008 i DEC-011 zapisują bezpośrednią decyzję użytkownika. Zaktualizowane zostały karta projektu, projekt gry, architektura, ryzyka, instrukcja agentów, README, strategia testów i istniejący plan. Nie zmieniono semantyki M01, nie zastąpiono Reality Engine i nie dodano drugiego planu produkcji. Archiwalne pliki oraz dwa dostarczone materiały nie zostały przerobione na aktualną fikcję.

Scena dwóch osad i mostu korzysta z warsztatu oraz mechanicznej gospodarki wodnej zamiast obowiązującego przemysłowego kontekstu bliskiej przyszłości. To robocza adaptacja treści; poziom technologii świata, kultury, nazwy, konkretna magia i gatunki nadal wymagają projektu. Nie wdrożono magii, potworów ani rozbudowanej walki. Nie są też nowym warunkiem odbioru M00–M06.

## Code Review

**Verdict:** APPROVE w zakresie przejrzanego przyrostu M02. W pierwszym przejściu zgłoszono REQUEST CHANGES; reprodukowany problem poprawiono i zachowano regresję. Końcowe pełne zestawy, w tym diagnostyka GPU, przeszły według [raportu M02](m02-window-laboratory.md). Nie jest to ocena całej przyszłej technologii.

**Confidence:** MEDIUM. Przegląd integratora obejmuje kod i konsumentów M02, nie niezależny audyt całej przyszłej technologii.

### Summary

Okno i renderer korzystają z rzeczywistego modelu czynności. Znaleziono konkretny błąd zachowania nieinteraktywnego procesu przy błędnym argumencie uruchomienia.

### Findings

| Priorytet | Problem i status | Lokalizacja |
| --- | --- | --- |
| P1 | POPRAWIONE: parser rzucał wyjątek przed przeniesieniem flagi smoke do konfiguracji; obsługa błędu otwierała modalne okno i zatrzymywała automat | [Obsługa opcji i błędów](../../src/platform/lab_main.cpp) |

### Details

`--smoke-test --report FILE --adapter invalid` zatrzymywał się na oknie błędu. [Próba przed naprawą](evidence/m02-invalid-options-before-fix.log) pokazuje rzeczywisty timeout. Poprawka ustala tryb prezentacji błędów i ścieżkę raportu przed walidacją wartości, a błąd zapisuje także przy nieukończonym parsowaniu. [Próba po naprawie](evidence/m02-invalid-options-after-fix.log) przechodzi. Sprawdzenie pozostaje w [teście procesu](../../tests/lab_gpu_integration.py), bez wydłużenia timeoutu, skip lub osłabienia oczekiwania.

### Recommendation

Instalację i pełny odbiór M02 zakończono rzeczywistymi testami. Następny etap to M03 z wczesną próbą ruchu i kamery. Rozwijać projekt gry przy tych samych etapach, bez przedwczesnej produkcji magii i zawartości.
