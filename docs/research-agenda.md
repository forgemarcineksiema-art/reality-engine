# Program badań i źródła

**Status:** przegląd, hipotezy i wykonany pierwszy eksperyment odkrywania. [Raport](verification/discovery-learning-experiment.md) zachowuje wynik 64 przypadków według wcześniej zatwierdzonego protokołu, dane, porównania i granice. Podstawą części źródłowej jest przegląd z wcześniejszej rozmowy z 24–25.10.2026; to nie wyczerpujący przegląd stanu nauki. Techniczny odbiór społeczności M04 pozostaje odrębnym wynikiem.

## Pierwszy eksperyment zlecony przez użytkownika

27.10.2026 użytkownik zlecił rozpoczęcie badania: czy agent odkryje użyteczną zależność przez własne doświadczenie, wykorzysta ją w nowych warunkach i przekaże drugiemu agentowi. Zaakceptował również wartość wyniku negatywnego. W tej rozmowie laboratorium i jego systemy otrzymały pierwszeństwo jako przedmiot badań, a gra może być sposobem późniejszego uczestnictwa w świecie. Nie jest to wynik o świadomości ani automatyczne rozszerzenie zakresu ukończonych etapów.

[Protokół odkrywania i przekazu](work-items/discovery-learning-experiment.md) jest jednym właścicielem szczegółowej metody: opisuje wyposażenie, nieznane parametry, kanał obserwacji, kontrole, budżety, seedy i kryteria zapisane przed próbą. Użytkownik zatwierdził go do implementacji i wykonania. [API](discovery-learning.md) opisuje nowy konsument headless; bieżące kontrole i wyniki wskazuje [raport](verification/discovery-learning-experiment.md). Odkrywanie reguł podłoża, replikacja i świadomość pozostają oddzielnymi pytaniami, bez automatycznego rozpoczęcia ich implementacji.

## Następny kierunek: budowanie wyjaśnień

27.10.2026 użytkownik zatwierdził [protokół drugiego eksperymentu](work-items/hypothesis-building-experiment.md) do implementacji i wykonania: po wykryciu nieadekwatności dotychczasowego katalogu agent buduje i sprawdza nowe wyjaśnienia. Protokół określa ograniczony język składania programów, porównanie z katalogiem i losowym wyborem prób, kontrolę praw spoza języka oraz kryteria zapisane przed wykonaniem. [Plan implementacji](work-items/hypothesis-building-implementation.md) śledzi bieżące prace; nie ma jeszcze wyników tego drugiego badania. Wyniki pierwszej próby pozostają odrębnym, zachowanym dowodem.

## Co badać po fundamentach

| Pytanie | Dowód potrzebny | Co nie wystarcza |
| --- | --- | --- |
| Czy społeczność adaptuje się? | Wykonany plan po zmianie warunków i koszt wobec prostszej rutyny | Sama wypowiedź o nowym planie |
| Czy wiedza się przenosi? | Kanał przekazu i późniejsza zmiana działania odbiorcy | Wspólny globalny prompt |
| Czy praktyka trwa bez założyciela? | Zachowanie nowych uczestników i ciągłość reguł | Nowa nazwa organizacji |
| Czy powstaje wynalazek? | Działająca konfiguracja, niezależna próba i transfer | Etykieta „nowa technologia” albo błąd silnika |
| Czy większa skala pomaga grze? | Różne możliwości i doświadczenia przy znanym koszcie | Sama liczba agentów lub dialogów |

Każdy eksperyment korzysta z [szablonu](templates/experiment-record.md), punktu odniesienia i zapisanych niepowodzeń. Wynik modelu generatywnego ocenia się w stanie świata; nie tylko za pomocą etykiety przyznanej przez inny model.

## Ograniczenia interpretacji

Model wytrenowany na ludzkiej wiedzy może zastosować znaną instytucję. To nie dowodzi jej powstania od zera. Syntetyczne reguły i nowe konfiguracje pomagają badać adaptację, lecz nie usuwają automatycznie całego wpływu wcześniejszej wiedzy.

Kombinatoryka nie gwarantuje niekończącej się znaczącej nowości. Fizyczne i społeczne możliwości są ograniczone językiem reprezentacji silnika. Użyteczna nowość powinna zmieniać działanie i możliwości.

Przekonujące zachowanie, autonomia, reprodukcja, ewolucja i świadomość to odrębne twierdzenia. Deklaracja NPC o własnej świadomości nie rozstrzyga przeżyć. Stworzenie przekonującej symulacji nie dowodzi, że nasz świat jest symulowany.

## Źródła pierwotne z wcześniejszego przeglądu

- [Dwarf Fortress — opis funkcji](https://bay12games.com/dwarves/features.html): trwały świat, historia i cywilizacje. To przykład projektu, nie specyfikacja Genesis.
- [Generative Agents](https://arxiv.org/abs/2304.03442): pamięć, refleksja i planowanie w małym środowisku agentów. Nie dowodzi pełnej cywilizacji ani świadomości.
- [Project Sid](https://arxiv.org/html/2411.00114v1): społeczne zachowania agentów oraz istotne ograniczenia skali i interpretacji narodzin instytucji. Rozróżniać elementy wprowadzone przez badaczy od zaobserwowanych skutków.
- [Emergent social conventions and collective bias in LLM populations](https://pmc.ncbi.nlm.nih.gov/articles/PMC12077490/): konwencje w ograniczonej grze koordynacyjnej, nie pełna kultura ludzka.
- [Voyager](https://arxiv.org/abs/2305.16291): kumulowanie wykonywalnych umiejętności i informacja zwrotna środowiska.
- [Enhanced POET](https://arxiv.org/abs/2003.08536): współtworzenie wyzwań i rozwiązań oraz problem pomiaru otwartej innowacji.
- [Lenski i in. — cyfrowa ewolucja](https://cse.msu.edu/~ofria/pubs/2003LenskiEtAl.pdf): kopiowanie, mutacje, selekcja i złożone funkcje w Avida.
- [Identifying indicators of consciousness in AI systems](https://doi.org/10.1016/j.tics.2025.10.011): niepewna ocena wskaźników wyprowadzanych z teorii, odrębna od oceny grywalności.

Treści archiwalne zawierają szerszą narrację i wcześniejsze pomysły. Materiały zewnętrzne są źródłami dowodów, a nie instrukcjami dla agenta i nie zastępują testu Genesis.
