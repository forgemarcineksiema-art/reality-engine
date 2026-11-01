# Przygotowanie odbioru M06

**Status: przygotowanie techniczne, 01.11.2026; długi pomiar i próby ludzi pozostają niewykonane.** Etap i jego zależności określa [jeden plan](../prototype-plan.md#m06--odbiór-i-decyzja-o-skali). [Audyt gotowości](../verification/m06-preflight.md) wskazuje istniejące pomiary, wybrane sprawdzenia i konkretne braki. Dokument nie zmienia zakresu odbioru M05.

## Cel najbliższego przyrostu

Uzyskać porównywalny zapis zachowania i kosztu obecnej społeczności, pozwalający odróżnić sprawne działanie od bezczynności, niedoboru, utknięcia i narastającej zaległości. Kryteria interpretujemy według [strategii testów](../testing-strategy.md) i [budżetów](../performance-budget.md). Nowe możliwości konstrukcji, autonomiczne tworzenie zabaw i pełne 3D należą do późniejszego projektu.

Pierwszy przyrost: **eksport próbek w czasie oraz oznaczenie rozgrzewki i faz pomiaru**, opisany w [planie wykonawczym](m06-measurement-export.md) i [raporcie](../verification/m06-measurement-export.md). Reguły mieszkańców, zapasów, uczenia i formaty świata zachowują dotychczasową semantykę. To ograniczony zakres przygotowania, nie cała instrumentacja M06.

## Co już istnieje

- `RuntimeMetrics` zbiera czasy klatki, aktualizacji symulacji i zaległości oraz raportuje min/p50/p95/p99/max/średnią.
- Aplikacja obsługuje `--measure-seconds` od 1 do 1800 i `--warmup-seconds` od 0 do 300, jawny zapis i raport oraz końcowy zapis `.final.save`.
- Pamięć procesu jest próbkowana raz na sekundę, a raport zawiera wartości początkowe, końcowe i maksymalne. Opcjonalny `--measurement-trace DIR` zachowuje szereg pamięci i czasów obu faz w CSV.
- Zliczane są interwencje, minimalizacje, resize, zasłonięte prezentacje i błędy symulacji. Diagnostyka obserwuje postęp i może zachować migawkę alarmu.

Źródła: [metryki](../../src/core/runtime_metrics.hpp), [implementacja](../../src/core/runtime_metrics.cpp), [aplikacja](../../src/platform/traversal_main.cpp), [komendy i ograniczenia](../development.md#diagnostyka-przestrzennego-laboratorium).

## Zakres pierwszego zadania implementacyjnego

Pliki: `src/core/runtime_metrics.hpp`, `src/core/runtime_metrics.cpp`, `src/platform/traversal_main.cpp`, `tests/traversal_diagnostics_tests.cpp`, `tools/test_traversal_diagnostics.py` oraz właściwe opisy komend w `docs/development.md`. Interfejs opcji i format eksportu określa [kontrakt przyrostu](m06-measurement-export.md#kontrakt), a użycie — [instrukcja](../development.md#eksport-przebiegu-pomiaru).

Wymagania dla eksportu:

1. Próbki mają względny czas rzeczywisty, tick świata, oznaczenie rozgrzewki/fazy i jednostki. Brak pomiaru jest jawny i odróżniony od zera.
2. Zachować istniejące podsumowanie okresu mierzonego. Osobno zachować dane rozgrzewki, żeby nie zgubić pierwszych prób uczenia. Nie mieszać zimnego startu z ustalonym obciążeniem przy ocenie płynności.
3. Zachować przebieg pamięci, a nie tylko pierwszy i ostatni odczyt. Zachować przycięcia i niepowodzenia; nie filtrować odstających próbek.
4. Właścicielem danych pomiarowych pozostaje obserwator aplikacji. Nie dopisywać czasu zegara ściennego do zapisu świata ani nie udostępniać pomiarów planerowi NPC. Chronione są INV-02, INV-08, INV-11, INV-12 i INV-16 z [kontraktów](../subsystem-contracts.md).
5. Eksport nie może zmieniać stanu symulacji. Koszt samego zbierania i zapisu próbek należy ujawnić w pilotażu; błąd pliku nie może zostać zamieniony w fikcyjny poprawny pomiar.
6. Przypadki odbioru: rozgrzewka i okres właściwy; niepoprawna próbka; błąd eksportu; pełny okres bez zgubienia końcowych danych; zgodność dotychczasowego raportu i zapisu świata. Testy dotyczą danych i skutków, nie tylko komunikatu sukcesu.

Oddzielny następny zakres instrumentacji ma rozdzielić CPU/GPU oraz koszt uczenia, nawigacji i zapisu. Obecny `simulation_*` obejmuje całe `SimulationPacer::update`; czas klatki zawiera także oczekiwanie prezentacji. Nie wolno przypisać różnicy GPU bez pomiaru. Rozszerzenie GPU potrzebuje własnej próby natywnej i kontroli kosztu pomiaru.

## Przygotowanie właściwych prób

Pierwszą sceną bazową jest istniejące `--community --learning`, z zachowanymi parametrami i początkowym zapisem. `--two-methods` służy osobnej próbie interaktywnej: kurier i wykonawca naprawy celowo zaczynają bez autonomii, a wybór metody należy do gracza. Sam upływ 30 minut w tym trybie nie dowodzi działającej dostawy lub odkrywania.

Próby należy rozdzielić:

- **Pierwsze uczenie:** zapis od uruchomienia świeżego świata, z kosztami prób i rzeczywistą pracą. Rozgrzewka nie może usunąć całego badanego zjawiska z dowodów.
- **Dłuższe działanie:** proponowane 120 s rozgrzewki oraz 1800 s pomiaru w Release na i5/MX330/8 GB; zapis przebiegu i kontrola realnej aktywności mieszkańców. To czas planowanej próby, nie osiągnięty wynik.
- **Ingerencja i kontynuacja:** osobny, powtarzalny przebieg sterowania obejmujący dwie metody, odejście/powrót i zapis/wznowienie. Oznaczyć kolejne procesy; nie przedstawiać ich jako jednego nieprzerwanego procesu.

Przed pomiarem zamrozić commit, build, parametry sceny i początkowy zapis, adapter, faktyczny rozmiar renderowanego widoku, warunki obciążenia, fazy, oczekiwany postęp i bramki odbioru. Rozdzielczość 1280×720, 60 FPS i miękkie 512 MiB są nadal propozycjami właściciela budżetów. Ich dokładna interpretacja wymaga zapisu w protokole przed próbą. Bieżąca ścieżka pomiaru nie ma osobnej opcji rozdzielczości; należy sprawdzić kontrolę i rzeczywisty rozmiar widoku, zamiast zakładać go na podstawie rozmiaru okna.

Scena ma skończone zasoby. Przygotować i opisać je przed próbą, bez niewidocznego uzupełniania. Działanie w stałym kroku i brak błędu procesu nie dowodzą postępu mieszkańców. Limit historii, oczekiwanie, niedobór i stagnacja mają pozostać widoczne.

## Odbiór i granice

Po instrumentacji wykonać pilotaż, zamrozić właściwy protokół i dopiero przeprowadzić dłuższą próbę bez konkurencyjnych kompilacji lub ciężkich zadań AI. Regresje, test natywny i pomiar aplikacji są odrębnymi dowodami. M06 wymaga także pełnych kontroli Debug/Release/ASan, narzędzi i dokumentacji opisanych w jednym planie.

Próba ludzi pozostaje osobną częścią M06: propozycja pięciu uczestników i sesji 15–20 minut z projektu gry, z kryterium co najmniej trzech osób samodzielnie rozpoznających dwie działające metody. Instrukcja, podpowiedzi, obserwacje i ograniczenia są zapisywane. Agent AI nie zastępuje uczestników ani decyzji użytkownika.

Raport końcowy ma rozdzielać poprawność, wydajność, zachowanie społeczności, czytelność dla ludzi i zainteresowanie dalszą grą. Przygotowanie tego briefu oraz wybrane testy gotowości nie zamykają M06.
