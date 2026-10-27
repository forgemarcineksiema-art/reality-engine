# Przegląd projektu eksperymentu odkrywania

**Status: projekt protokołu przygotowany i przejrzany; implementacja oraz D/G/S niewykonane.** Data: 27.10.2026. Właścicielem szczegółów pozostaje [projekt eksperymentu](../work-items/discovery-learning-experiment.md). Ten raport dotyczy wyłącznie przygotowania projektu przed próbą, bez zmiany statusu M04 lub przypisywania akceptacji metodzie badawczej przez użytkownika.

## Wykonane rozpoznanie

Odczytano README, reguły inżynierskie, kartę projektu, mapę dokumentacji, program badań, kontrakty i rzeczywiste API `SimulationClock`, `WorkWorld`, `TraversalSession`, obserwacji oraz kontaktu. Nowe pomiary i kontroler uczący się nie istnieją w tym kodzie. Stan wyjściowy: czyste `main`, commit `96e6c11`; dokumenty przygotowano na gałęzi `codex/discovery-experiment-design`. Nie wykonywano push.

Zakres dokumentacyjny: nowy protokół, niniejszy raport, odnośnik w programie badań i rejestracja obu dokumentów w mapie. Nie zmieniono kodu produkcyjnego, testów silnika, formatów ani historycznych dowodów.

## Odczytowy przegląd przez osobnego agenta

Skill `brainstorming` wymagał sprawdzenia, czy projekt pozwala przygotować plan realizacji bez zgadywania istotnych reguł. Osobny agent `discovery_design_review` otrzymał czysty kontekst, ścieżkę projektu oraz instrukcję pracy wyłącznie odczytowej. Odczytał projekt i właściwe API; nie uruchamiał seedów, buildów, GPU ani eksperymentu.

Pierwsza ocena wskazała trzy istotne luki; zachowujemy je jako historię projektu:

| Luka pierwszego projektu | Dlaczego istotna | Korekta |
| --- | --- | --- |
| Minimax liczył hipotezy tylko mierzonego symbolu | Rozpoznany symbol z jedną hipotezą mógł wygrywać z nieznanymi i powodować puste próby | Minimalizacja najgorszej liczby kombinacji całego układu; wykluczenie prób bez redukcji |
| Generator inicjalizował strumienie przez `seed xor tag` | Dla kolejnych seedów różne przeznaczenia w różnych światach otrzymywały ten sam strumień | Rozłączne pola seeda, tagu i licznika, jawna funkcja mieszająca i błąd wyczerpania licznika |
| Opis wymagał zadań B innych niż A, generator dopuszczał pokrycie | Zakres generalizacji był silniejszy niż rzeczywisty dobór danych | Oddzielne losowanie z tego samego zakresu, jawne dopuszczenie i raportowanie pokrycia; B nie ogląda oceny A |

Dodatkowo wyraźnie odseparowano gałęzie checkpointu oraz ograniczono wiadomość do własnych pomiarów A z etapu odkrywania. Dane ocen nie wracają do nauczania.

Po ponownym odczycie zmienionego projektu recenzent zwrócił: **„nothing important left”**, potwierdzając zamknięcie trzech luk oraz brak dalszych istotnych przeszkód przed planem. To ocena projektu, nie niezależne wykonanie programu lub potwierdzenie hipotez. Autor przejrzał i zintegrował poprawki.

W przeglądzie zauważono również, że pomiary pojedynczego elementu przy 16 i 64 eu pozwalają konstrukcyjnie rozróżnić wszystkie 75 proponowanych hipotez. Nie jest to wynik przebiegu agenta. Pokazuje ograniczony charakter próby: dostępny mechanizm jest przygotowany przez autora, a badamy jego nabywanie z legalnych danych, generalizację, koszt i przekaz w Genesis. Nie przedstawiamy tego jako oryginalnego odkrycia nowego prawa przyrody.

## Kontrole dokumentów

W katalogu głównym projektu autor wykonał:

| Polecenie | Wynik |
| --- | --- |
| `python tools/check_docs.py` | PASS: rejestr, lokalne odnośniki, identyfikatory i sumy archiwum |
| `python -m unittest discover -s tools/tests -p "test_*.py"` | 29 testów, OK |
| `git diff --check` | Brak błędów białych znaków |

Kontrole powtórzono po końcowej edycji dokumentów przed ich oddaniem. Nie uruchamiano C++: zmiana obejmuje wyłącznie dokumentację. Nie wykonano obserwacji D/G/S, próby GPU, pomiaru wydajności ani analizy mocy. Szczegółowa metoda pozostaje propozycją do zatwierdzenia przed implementacją.
