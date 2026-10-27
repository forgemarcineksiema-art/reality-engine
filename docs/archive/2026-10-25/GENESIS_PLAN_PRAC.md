# GENESIS — plan rozpoczęcia prac i kryteria przejścia

Wersja 0.3, 25 października 2026 r. Kierunek: grywalny fragment i laboratorium, własny silnik, fizyka i nawigacja.

Podstawa: [architektura techniczna](GENESIS_ARCHITEKTURA_TECHNICZNA.md). Jest to plan, nie wykaz wykonanych zadań. Użytkownik pracuje ze swoimi agentami AI, bez narzuconego terminu. Sprzęt odniesienia: i5-1035G1, MX330 i około 8 GB RAM. Grafika ma najniższy priorytet; własna fizyka i nawigacja są częścią zakresu.

Fundamenty grafiki, narzędzi i streamingu opisuje [plan własnego silnika](REALITY_ENGINE_WLASNY_SILNIK.md). Jego etapy E0–E6 uzupełniają poniższy backlog. Wcześniejsza orientacja na 12 tygodni zostaje wycofana po zwiększeniu zakresu do budowy własnego silnika.

[Rejestr decyzji i otwartych pytań](GENESIS_DECYZJE_I_OTWARTE_PYTANIA.md) zapisuje odpowiedzi użytkownika i nadal robocze decyzje o świecie, czasie oraz perspektywie gry.

## 1. Pierwszy cel

Propozycja pierwszego wycinka: około 12–16 trwałych mieszkańców, dwie małe osady, jedno przejście mostowe, warsztat, przepompownia, magazyn i transport części. Po pomiarach rozszerzamy próbę do około 60 osób. Mieszkańcy jedzą, pracują, zarabiają i podejmują ograniczone plany. Istnieją zapasy awaryjne oraz jawny model wymiany z otoczeniem.

Gracz może uszkodzić most, zdobyć części, pomóc w naprawie, negocjować dostawę, przejąć ładunek albo pozostawić sytuację mieszkańcom. Wszystkie drogi korzystają z tych samych reguł zasobów i czynności.

Przebieg obejmuje świat działający bez renderowania oraz skromną scenę 3D z proceduralnych brył. Ta sama społeczność działa w grze i runnerze. Własna fizyka zaczyna się od ruchu i kolizji prostych kształtów, a nawigacja od mapy przejścia oraz A*. Nie potrzebujemy pełnego repertuaru walki, zaawansowanego rynku finansowego ani modeli generatywnych.

## 2. Backlog zależności

| ID | Zadanie | Zależność | Dowód ukończenia |
| --- | --- | --- | --- |
| P00 | Definicja zakresu, słownik pojęć, jednostki i sprzęt odniesienia | — | Jednoznaczne znaczenie osoby, zasobu, czasu, zdarzenia i kryteriów próby |
| P01 | Powtarzalny build rdzenia i runner scenariuszy | P00 | Jedna komenda uruchamia próbę z zapisanym scenariuszem i ziarnem |
| P02 | Identyfikatory, typy danych i komendy | P01 | Usunięcie i ponowne użycie slotu nie zmienia tożsamości obiektu |
| P03 | Kontrakty działań i rezerwacje | P02 | Dwie ekipy nie zużywają tej samej części; anulowanie zwalnia prawidłowe rezerwacje |
| P04 | Zegar, kolejka zdarzeń i inspektor przyczyn | P02–P03 | Można wyjaśnić zmianę oraz odtworzyć kolejność dyskretnych skutków |
| P05 | Migawki, zapis działań w toku i wczytanie | P04 | Wczytana próba zachowuje własność, rezerwacje, postęp i przyszłe zdarzenia |
| P06 | Magazyn, produkcja, zatrudnienie i transport | P03–P05 | Bilanse zgadzają się; zerwana trasa zmienia termin i dostępność dostawy |
| P07 | Potrzeby, procedury, ograniczone planowanie i wiedza | P06 | Mieszkaniec reaguje na znane zaburzenie i wykonuje osiągalny plan zastępczy |
| P08 | Minimalny własny runtime 3D i interakcja gracza | P02–P03 oraz własne kolizje i trasy E2; obraz E1 powstaje wcześniej | Czynność w 3D zmienia ten sam zasób, który widzi runner i zapis |
| P09 | Most: uszkodzenie, trasa, naprawa, zmiana reprezentacji | P06–P08 | Po odejściu, powrocie i wczytaniu zgadzają się stan, kolizja i przejezdność |
| P10 | Świadek, informacja i jedna scena autorska | P07–P09 | Reakcja zależy od wiedzy; brak niemożliwych wymagań po śmierci uczestnika |
| P11 | Testy długiego działania i scenariuszy skrajnych | P05–P10 | Raport kosztów, zaległości, błędów oraz zakresu stabilnego działania |
| P12 | Ocena doświadczenia i decyzja o następnym etapie | P11 | Tester rozumie działanie, zauważa skutki i wykorzystuje alternatywną metodę |

Na każdym etapie dopuszczalne są proste atrapy brakujących elementów, jeżeli są oznaczone i nie służą jako dowód badanej własności. Ręcznie ustawiona awaria może być bodźcem testu; ręcznie dopisany ratunek nie dowodzi autonomicznej adaptacji.

## 3. Trzy równoległe strumienie pracy

**Symulacja i diagnostyka:** dane, działania, rezerwacje, zegar, gospodarka, zapis, inspektor i runner.

**Silnik, obraz i przygoda:** platforma, backend GPU, import, renderer, podstawowy edytor, ruch, kamera, interakcja i kolizja. Ten strumień zaczyna się wcześnie i łączy z symulacją na małych fragmentach. Przy własnym silniku może wymagać kilku odrębnych specjalizacji.

**Projektowanie i weryfikacja:** reguły gospodarki, scenariusze, punkty odniesienia, obserwacja graczy i porównywanie wyników. Ten strumień pracuje na rosnącym, działającym wycinku.

To podział odpowiedzialności dla użytkownika i jego agentów AI. Strumienie łączymy małymi przyrostami. Zadanie określa cel, kontrakt, pliki i test; ukończenie wymaga działającego buildu. Wspólne scenariusze ograniczają rozchodzenie się implementacji.

## 4. Mapa etapów po zbudowaniu niezbędnych fundamentów

To kolejność integracji scenariusza, a nie harmonogram tygodniowy. Własna fizyka, nawigacja i renderer wymagają pracy opisanej w etapach E0–E6. Wczesne bryły proceduralne i prosty inspektor pozwalają odłożyć rozbudowany import oraz edytor. Postęp mierzymy działaniem, poprawnością i kosztem, bez narzuconego terminu.

| Iteracja | Cel techniczny | Oglądalny wynik |
| --- | --- | --- |
| 1 | Build, dane, zegar, komenda oraz połączenie z własnym runtime | Obiekt ma stały identyfikator, można go zmienić i sprawdzić stan |
| 2 | Rezerwacje, przerwania, zapis i podstawowy inspektor | Dwie osoby konkurują o przedmiot; zapis nie dubluje skutku |
| 3 | Minimalny łańcuch produkcji i dostaw | Część dociera fizycznie/logistycznie do zakładu i zostaje zużyta |
| 4 | Potrzeby, ograniczone plany i reakcja na niedostępną trasę | Mieszkańcy wykonują plan zastępczy albo poprawnie rozpoznają brak możliwości |
| 5 | Interakcja 3D, uszkodzenie mostu, naprawa i ciągłość reprezentacji | Gracz wywołuje zdarzenie i obserwuje trwały skutek |
| 6 | Scena autorska, długie próby, pomiary i obserwacja graczy | Materiał pozwalający zdecydować: rozwijać, poprawiać albo zmienić założenie |

Brak przejścia testu zatrzymuje zwiększanie skali danego obszaru. Pozostała praca może trwać, jeśli nie opiera się na niepotwierdzonej własności.

## 5. Scenariusze obowiązkowe

### S0 — spokojny świat

Uruchomienie bez gracza z dostępnymi zasobami. Sprawdzamy przepływy, potrzeby, koszty, zaległości i przyczyny ewentualnego załamania. Celem jest znany zakres stabilności dla przyjętych parametrów.

### S1 — przerwana dostawa

Most zostaje zamknięty, gdy pojazd jest przed nim. Ładunek nadal istnieje. Plan może znaleźć objazd, wybrać inny środek transportu, oczekiwać albo zgłosić brak możliwości. Żaden wariant nie tworzy dostawy bez wykonania potrzebnych czynności.

### S2 — konkurencja i przerwanie

Dwie ekipy próbują zarezerwować ostatnią część. Zwycięska ekipa zostaje przerwana, a następnie scenariusz zapisany i wczytany. Sprawdzamy ilość części, właściciela, fazę działania i odzyskiwanie rezerwacji.

### S3 — wiedza i prawo

Gracz przejmuje ładunek. Osobna próba ma świadka, druga nie ma. Egzekwowanie prawa zależy od faktycznie przekazanej informacji; zmiana właściciela i położenia ładunku jest spójna w obu próbach.

### S4 — świat po powrocie

Gracz oddala się od konwoju i uszkodzonego mostu. Po upływie czasu wraca. Czas podróży, zużyte zasoby, wykonana praca i zmiany konstrukcji zgadzają się z dziennikiem oraz zadeklarowanym modelem uproszczeń.

### S5 — historia w zmienionych warunkach

Kluczowy uczestnik umiera albo przedmiot zostaje zabrany przed uruchomieniem sceny. Historia przechodzi do zdefiniowanego stanu albo poprawnie się zamyka. Nie pozostaje wymaganie rozmowy z nieistniejącą osobą.

## 6. Co mierzymy

Poprawność: brak podwójnego wydania zasobu, nieprawidłowych odwołań, zgubionych zobowiązań i nieuzasadnionych transferów. Produkcja i straty mają jawne reguły, więc nie wymagamy zachowania liczby przedmiotów podczas legalnej konwersji materiałów.

Wydajność: czasy klatek i ich percentyle, obciążenie CPU/GPU, pamięć, czas planowania, zaległości aktualizacji, zapis i wczytanie, koszty migracji reprezentacji. Testy obejmują dłuższe działanie, nie tylko start.

Zachowanie: ukończone działania, źródła porażek, adaptacja po zaburzeniu i rzeczywiste kanały informacji. Porównujemy prostsze procedury z bardziej złożonym planowaniem przy znanym koszcie.

Gra: zrozumienie sytuacji, użycie alternatywnych metod, rozpoznawanie mieszkańców i zauważanie skutków. Nie zakładamy, że większa liczba zdarzeń oznacza lepsze doświadczenie.

## 7. Co dalej po przejściu pierwszej próby

Najbliższa próba skali zwiększa populację z około 12–16 do około 60 osób. Około 400 osób pozostaje dalszą hipotezą zależną od pomiarów, a nie automatycznie zatwierdzonym następnym zadaniem. Najpierw zwiększamy jeden wymiar obciążenia, potem łączymy obciążenia. Osobno badamy kulturę i transfer umiejętności.

Każde przejście skali wymaga ponownego pomiaru. Średni koszt mieszkańca w spokojnej osadzie nie prognozuje kosztu tłumu podczas pożaru.

Nie uruchamiamy jednocześnie pełnej kampanii, dowolnej destrukcji, rozwoju pokoleń, generowanego głosu, multiplayera i otwartej ewolucji. Każda z tych funkcji ma osobny dowód wartości oraz zależności.

## 8. Artefakty pracy

Rekomendowany układ przyszłego repozytorium, jeszcze nieutworzony:

```text
genesis/
  core/          reguły i dane domenowe
  runner/        uruchamianie scenariuszy bez obrazu
  platform/      okno, wejście i integracja systemu
  graphics/      własny renderer i backend GPU
  world/         streaming i reprezentacje
  assets/        import, kompilator zasobów i pakiety
  animation/     szkielety, klipy i skinning
  physics/       własne kolizje, ruch i solver rozwijany etapami
  navigation/    własna mapa przejścia, A* i graf regionu
  runtime/       wspólne wykonanie silnika i gry
  game/          projekt gry i assety
  tools/         inspektor, walidatory, analiza
  scenarios/     dane wejściowe i kryteria prób
  tests/         regresje, bilanse i zapis
  docs/adr/      decyzje architektoniczne
```

Zadanie jest ukończone, kiedy działa w buildzie, ma wymagany test, daje się zdiagnozować i mieści się w ustalonym budżecie albo ujawnia jego przekroczenie. Pokaz wideo jest dodatkiem do wykonywalnego scenariusza i danych z pomiaru.

## 9. Decyzje do rozstrzygnięcia przed kodem produkcyjnym

- Cel płynności i szczegółowy budżet obrazu na potwierdzonym laptopie.
- Główny rodzaj osobistej rozgrywki w pierwszym wycinku.
- Docelowy zakres materiałowej destrukcji.
- Oczekiwana długość życia jednego zapisu i zakres kompatybilności aktualizacji.
- Czy model generatywny ma być całkowicie opcjonalny także w docelowym produkcie.
- Potwierdzenie roboczego startu Windows/D3D11; każda dodatkowa biblioteka wymaga uzasadnienia. Własna fizyka i nawigacja są już ustalone.

Można rozpocząć ograniczone próby techniczne z jawnymi założeniami. Wiarygodny koszt i termin pełnej produkcji powstają dopiero po potwierdzeniu najważniejszych ryzyk oraz zmierzeniu pracy nad grywalnym wycinkiem.
