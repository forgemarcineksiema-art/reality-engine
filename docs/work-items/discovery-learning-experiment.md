# Odkrycie zależności, generalizacja i przekaz — projekt pierwszego eksperymentu

**Status: protokół ZATWIERDZONY przez użytkownika, zapisany przed implementacją i próbą.** Data zatwierdzenia: 27.10.2026. Po przedstawieniu projektu użytkownik odpowiedział „Tak” na zatwierdzenie protokołu jako podstawy implementacji i wykonania próby. Poniższe szczegóły są wyborami inżynierskimi autora zatwierdzonymi do realizacji, a nie uzyskanymi wynikami. [Plan wykonania](discovery-learning-implementation.md) i [raport](../verification/discovery-learning-experiment.md) wskazują bieżący stan bez zmiany kryteriów.

Dokument jest właścicielem protokołu `discovery-v1`, zatwierdzonego przed implementacją. Kontekst i źródła posiada [program badań](../research-agenda.md), a sposób rozdzielenia planu i wyniku wskazuje [szablon eksperymentu](../templates/experiment-record.md). Nie zastępuje [planu etapów](../prototype-plan.md) ani historycznych raportów. Nie jest nową bramką zaliczonego M04. Bieżący zakres kodu opisuje [API](../discovery-learning.md), a rzeczywiste kontrole i wyniki [raport](../verification/discovery-learning-experiment.md); poniższy opis pozostaje kryteriami zapisanymi przed próbą.

## Pytanie i zakres twierdzenia

Czy agent potrafi, korzystając z własnych interwencji i pomiarów, rozpoznać nieznane działanie elementów urządzenia, wykorzystać je w niepróbowanych konfiguracjach i pomóc drugiemu agentowi przez lokalny przekaz danych z doświadczenia?

Badamy oddzielnie trzy hipotezy: **D** — uczenie zależności z doświadczenia, **G** — użycie jej poza zbiorem doświadczeń, **S** — korzyść drugiego agenta z przekazu. Potwierdzenie całego pytania wymaga wszystkich trzech. Jest to pierwsza próba kalibracyjna mechanizmu poznawania w Genesis: agent identyfikuje nieznane parametry i rodzinę z dostarczonego zbioru. Nie rozstrzyga otwartego problemu naukowego istnienia takiej zdolności. Nie badamy powstania świadomości, emocji, DNA, życia, własnej motywacji lub języka. Nie twierdzimy, że agent wynalazł nieznaną ludzkości teorię.

## Rozważone podejścia

| Podejście | Zaleta | Koszt i ograniczenie | Wybór dla tej próby |
| --- | --- | --- | --- |
| Nauka na zapasach i przeprawie obecnej społeczności | Najwięcej istniejącej integracji | Znane już obserwacje dostępności i wybór trasy słabo oddzielają nową zależność od gotowej reguły; ruch i potrzeby utrudniają interpretację | Zachować jako regresję i późniejsze zastosowanie |
| Mały stół pomiarowy z nieznanymi parametrami | Kontrolowane interwencje, bilans, ścisła prognoza i rozdzielenie trzech hipotez; bez treningu dużego modelu | Rodziny możliwych praw i zdolność wnioskowania dostarczamy my | **Rekomendowany projekt** |
| Samoorganizacja i replikacja z prostego podłoża | Bliżej pytania o powstawanie życia | Inny przedmiot badania i dużo trudniejsza identyfikacja jednostek, dziedziczenia i selekcji | Osobny późniejszy eksperyment, bez tworzenia go w tym zadaniu |

Wybór stołu rozwija omawiany z użytkownikiem pomysł małego środowiska z przepływem energii. Szczegółowa aparatura i metoda wnioskowania są nadal wyborem autora. Nie dokładamy modelu językowego, usług sieciowych ani gotowego silnika fizyki/nawigacji.

## Co dostarczamy, a czego agent ma się dowiedzieć

| Dostarczamy jawnie | Agent A zdobywa sam | Agent B zdobywa przez przekaz lub własne próby |
| --- | --- | --- |
| Ciało i dozwolone komendy, trzy rozróżnialne oznaczenia elementów, miernik wejścia/wyjścia | Które parametry i która z dopuszczonych rodzin opisują dany element | Dane o rzeczywiście wykonanych próbach A |
| Pamięć obserwacji, skończony zbiór hipotez i procedurę ich eliminowania | Które hipotezy odrzucić na podstawie pomiarów | Własny zbiór hipotez wyliczony z otrzymanych danych; nie gotowy model A |
| Wiedzę o możliwych rodzinach elementów oraz szeregowym składaniu urządzenia | Przewidywania dla nowych energii wejściowych i niepróbowanych łańcuchów | Czy otrzymane informacje wystarczą do nowych prognoz i działania |
| Zadany cel dostarczenia energii do odbiornika w określonym przedziale | Jaką konfigurację wybrać dla nowego celu | Jak wykorzystać doświadczenie A we własnej konfiguracji |
| Mechanizm wyboru prób i selekcji danych do komunikatu | Konkretne pomiary, pozostałe możliwości i wynikające z nich decyzje | Weryfikację otrzymanych informacji w granicach własnego budżetu |

Nie dostarczamy parametrów rzeczywistego świata, rozwiązania nowego zadania, odczytu rezerw A przez B, dostępu do seedów ani dziennika eksperymentatora. Motywacja i język komunikatu są narzucone. Jest to ograniczone rozpoznawanie układu i składanie znanych mechanizmów; model nie odkrywa rodzin funkcji od podstaw.

## Aparatura i autorytatywne prawa

Urządzenie jest abstrakcyjnym, dyskretnym układem przepływu impulsu. Nie jest modelem elektromagnetyzmu ani nową fizyką brył. Energia ma jawną jednostkę `eu`, czas ma krok 50 ms. Nazwy elementów są neutralnymi symbolami `u`, `v`, `w`, nie podpowiedziami funkcji.

Wejście `E` jest nieujemną liczbą całkowitą. Każdy element ma jedną z rodzin i jeden niezmienny parametr:

| Rodzina, znana jako możliwość | Faktyczne działanie | Dopuszczone parametry |
| --- | --- | --- |
| Proporcjonalna strata | `out = floor(E * q / 16)` | `q` od 3 do 12 |
| Stała strata | `out = max(0, E - d)` | `d` od 1 do 24 |
| Ograniczenie przepływu | `out = min(E, c)` | `c` od 8 do 48 |

Każdy symbol ma niezależnie wybraną rodzinę; nie obiecujemy agentowi, że występuje po jednej z każdej. Dla każdego symbolu początkowo istnieje 75 hipotez: 10 + 24 + 41. Parametry nie zmieniają się pomiędzy A, B i ich stanowiskami w jednym świecie. Rodziny i parametry mogą być inne w kolejnym niezależnym świecie.

Łańcuch to uporządkowany ciąg od jednego do trzech elementów, z powtórzeniami symboli. Zbiór ma 39 konfiguracji. Energia przechodzi kolejno przez każdy element. To abstrakcyjny zasób konfigurowalnych stopni urządzenia; nie twierdzimy, że postać zbudowała części lub przeniosła materiał. Ustawienie łańcucha trwa 1 krok, impuls 1 krok; podczas impulsu konfiguracja jest zamknięta. Zły zakres, aktywny impuls lub brak energii daje jawną odmowę przed zmianą stanu.

Przy każdej próbie ze źródła znika dokładnie `E`. Wyjście trafia do odbiornika, a reszta do księgi strat. Obowiązuje `źródło_początkowe = źródło_pozostałe + suma_wyjść + suma_strat`. Nie ma odzysku impulsów ani energii z wiadomości. Każda odrębna sesja pomiarowa ma początkowe 8192 eu; reset do porównania jest utworzeniem jawnej nowej sesji, nie odtworzeniem zasobu w tym samym świecie.

Wartość wyjścia po interwencji jest obserwacją. Wyjście przewidywane przed nią jest prognozą. Miernik jest dokładny; tolerancja prognozy wynosi **0 eu**. Szum i fizyka ciągła pozostają poza tą pierwszą próbą.

## Metoda uczenia i wyboru działania

Agent filtruje własny zbiór hipotez wyłącznie pomiarami, które zaobserwował albo otrzymał dozwolonym kanałem. Hipoteza odpada, jeśli jej wynik nie zgadza się z pomiarem. Przy pustym zbiorze wynik to `model_inadequate`, nie dopisanie prawdziwego parametru ani arbitralne przywrócenie hipotez.

W odkrywaniu dozwolone są pojedyncze elementy oraz wejścia 16, 32, 64 eu, czyli 9 interwencji. Agent wybiera interwencję minimalizującą największą liczbę **kombinacji hipotez całego urządzenia** po możliwym wyniku: liczba pozostających hipotez badanego symbolu razy liczności zbiorów dwóch pozostałych symboli. Próbę niedającą gwarantowanej redukcji tego iloczynu wyklucza się. Remisy rozstrzyga kolejno mniejsza energia i symbol. Jest to dostarczona metoda aktywnego rozpoznawania, a nie wyłoniona ciekawość. Jeżeli żadna interwencja nie zmniejsza niepewności, agent kończy zbieranie danych; nie wykonuje pustych prób rozpoznanego symbolu do limitu.

Prognoza jest medianą wszystkich przewidywań pozostałych kombinacji hipotez (przy parzystej liczbie dolną medianą), z jawnym zakresem minimum–maksimum. Doboru łańcucha dokonuje agent przez ocenę 39 konfiguracji według swojego zbioru: maksymalna część kombinacji hipotez trafiających do celu, potem najmniejszy rozrzut wyniku, krótszy łańcuch i porządek leksykograficzny. Nie pyta w tym celu aparatury ani sędziego. To ograniczone planowanie w znanej reprezentacji, nie wynalezienie algorytmu planowania.

Pełne przejrzenie małego zbioru jest uczciwą pierwszą metodą. Maksimum to 75^3 = 421875 kombinacji hipotez przed obserwacjami. Każda hipoteza ma jednakowy głos w sterowniku; nie twierdzimy, że ten wybór jest optymalnym priorem dla generatora o jednakowych prawdopodobieństwach rodzin. Implementacja ma używać ograniczonych obliczeń bez globalnego cache prawdy, a ich koszt raportować. Wydajność tej metody dla większej przestrzeni pozostaje nieznana.

## Przebieg jednej pary światów

### Odkrywanie przez A

A zaczyna z pustą pamięcią i powyższym wyposażeniem. Ma do 24 fizycznych impulsów, 1536 eu i 1000 kroków sesji na odkrywanie. Wszystkie limity obowiązują jednocześnie. Pojedyncza odmowa jest zapisywana; powtarzanie odmów nie daje darmowych prób. Po zakończeniu zamrażamy model do oceny. Nie wymagamy przeprowadzenia dokładnie 24 prób.

Pierwszy pomiar zawiera znacznik interwencji, symbol/łańcuch, wejście, wyjście, ID wykonawcy, ID aparatury i `observed_tick`. Nie zawiera parametru ani nazwy rzeczywistej rodziny. Rejestruje się też prognozę sprzed pomiaru i liczbę pozostałych hipotez.

Checkpoint po odkrywaniu jest wspólnym rodzicem odrębnych gałęzi oceny A oraz kontaktu A z B. Są to jawne kopie do porównania eksperymentalnego, nie kolejne podróże jednego świata ani teleportacja między stanowiskami. Kontakt i pakiet powstają z gałęzi sprzed oceny A. Pomiary z oceny nigdy nie wchodzą do pamięci używanej przy nauczaniu.

### Nowe warunki dla A

Ocena ma dwie części. Agent składa prognozy przed wykonaniem każdego impulsu. Model pozostaje zamrożony przez całą ocenę; wyniki trafiają do oddzielnego raportu i nie zmieniają następnej decyzji.

- **Prognozy:** 24 ustalone w manifeście zapytania z wejściem 48, 80 lub 112 eu i łańcuchem długości 2 lub 3. Żadne z tych wejść ani łańcuchów nie wystąpiło w odkrywaniu.
- **Użyteczność:** 20 nowych zadań z tych samych nowych wejść. Sędzia ustala cel jako wynik wcześniej wylosowanego łańcucha długości 2 lub 3, z zakresem celu `[wynik, wynik]`. Agent widzi wejście i cel, ale nie łańcuch sędziego. Ma jedną fizyczną próbę na zadanie. Cel jest zatem rozwiązywalny z konstrukcji; tę pomoc eksperymentatora ujawniamy. To test zastosowania zależności, nie swobodne formułowanie problemu.

Tworzymy osobne sesje dla prognoz i zadań, każdą z 8192 eu i limitem 2000 kroków; model A przenosimy jako jawny stan tego samego badanego agenta. Zasoby nie przechodzą między sesjami. Nie ma ukrytego ponawiania nieudanej odpowiedzi.

### Przekaz i działanie B

B ma tę samą procedurę wnioskowania, pustą pamięć i własne stanowisko z tymi samymi prawami elementów. Nie obserwuje prób A ani jego konfiguracji końcowej. Wie z wyposażenia, że oznaczenia elementów na obu stanowiskach odnoszą się do tych samych praw. Ta zgodność jest założeniem, a nie odkryciem B.

A i B rzeczywiście dochodzą na wspólnej podporze do kontaktu do 2 m, bez przeszkody. Przekaz musi przejść tę samą kontrolę geometrii co istniejący przekaz wiedzy. Przed osiągnięciem kontaktu B nie otrzymuje niczego. Jedna wymiana trwa 1 krok, przenosi najwyżej 6 istniejących zapisów pomiarów A i nie uruchamia dalszego automatu pośrednika.

A wybiera podzbiór do 6 rekordów minimalizujący iloczyn liczności trzech zbiorów hipotez odbiorcy, przy założeniu znanego pustego stanu początkowego B. Selekcja korzysta **wyłącznie z własnego dziennika odkrywania**, bez nowych interwencji i bez pomiarów z ocen D/G. Przy remisie bierze mniej rekordów, potem rosnące ID pomiarów. Algorytm selekcji jest dostarczony przez autora; nie badamy powstania pedagogiki ani języka.

Wiadomość zawiera dane prób, pierwotnego świadka `source_actor`, ostatniego rozmówcę `via_actor`, czas obserwacji i czas przekazu. Nie przenosi hipotez, parametrów, funkcji celu, wag, pamięci całego A ani gotowego przepisu. B sam filtruje swój zbiór na podstawie odebranych rekordów. Czas przekazu nie odmładza obserwacji. A następnie przestaje uczestniczyć i nie wykonuje za B jego zadań.

Z identycznego początkowego stanu B wykonujemy porównanie parami: **z przekazem** i **bez przekazu**. W drugim wariancie kontakt, czas i budżety są takie same; wymiana kończy się pustym pakietem. B w obu wariantach ma do 2 własnych impulsów po 16/32/64 eu, do 128 eu i 1000 kroków na dodatkowe rozpoznanie. Następnie zamraża swój model i wykonuje własne 20 oddzielnie wylosowanych zadań z wejściami spoza odkrywania, z jednym impulsem na zadanie. Zbiór wartości wejść jest taki sam jak w ocenie A; możliwe pokrywanie się zadań A/B zachowujemy i raportujemy. B nie ma dostępu do oceny A. Ocena używa odrębnej sesji 8192 eu / 2000 kroków.

Przewaga B jest badana przy celowo ograniczonym budżecie własnych prób. Podajemy także pełny koszt A + kontaktu + B. Sukces S nie oznacza, że dwa agenty są łącznie tańsze niż jeden ani że komunikacja zawsze pomaga. Transport ustrukturyzowanych danych jest mechanizmem dostarczonym przez nas.

## Generowanie, rozdzielenie prób i zamrożenie

Zestaw rozwojowy: 16 seedów od `0xD15C0000` do `0xD15C000F`. Zestaw potwierdzający: 64 seedy od `0xD15C1000` do `0xD15C103F`. Jeden seed jest jednostką niezależnego porównania; 20 zadań tego samego świata nie traktujemy jak 20 niezależnych światów. Seedy są jawne dla badacza, niedostępne agentowi. To nie jest próba ślepa wobec autora.

Generator `discovery-v1` używa mieszania SplitMix64 w wersji licznikowej, z 64-bitowym przepełnieniem modulo 2^64. Dla 32-bitowego seeda, 8-bitowego tagu i 24-bitowego licznika `j`, początkowo 0, tworzy `key = (uint64(seed) << 32) | (uint64(tag) << 24) | j`; następnie `z = key + 0x9E3779B97F4A7C15`; `z = (z xor (z >> 30)) * 0xBF58476D1CE4E5B9`; `z = (z xor (z >> 27)) * 0x94D049BB133111EB`; wynik `z xor (z >> 31)`. Po losowaniu zwiększa j; wyczerpanie 24 bitów jest jawnym błędem, bez zawijania lub wejścia w inną domenę. Tagi: prawa 1, prognozy A 2, zadania A 3, zadania B 4, losowa eksploracja 5, losowe działanie A 6, losowe działanie B 7. Pola klucza nie nakładają się: różne pary seed/tag nie mają tych samych kluczy losowań. Nie stosujemy `seed xor tag`, które przy kolejnych seedach tworzy kolizje strumieni. To deterministyczny generator pseudolosowy, nie dowód matematycznej niezależności próbek.

Każdy wybór całkowity to `draw % count` bez odrzucania wartości; drobne odchylenie takiego mapowania jest jawne. Dla każdego z `u,v,w` losujemy rodzinę (`draw % 3`, w kolejności tabeli) i jej parametr przez minimum + `draw % count`. Konfigurację losujemy przez długość `2 + draw % 2` i kolejne symbole `draw % 3`; wejście wybiera indeks `draw % 3` z `[48,80,112]`. Powtórzenia zapytań i łatwe/zerowe cele zachowujemy, bez poprawiania zestawu po zobaczeniu wyników. Kolejność losowania pojedynczego zapytania: wejście, długość, symbole. Dla zadań wynik przepisu sędziego staje się celem.

Przed pierwszą próbą potwierdzającą zapisujemy manifest wszystkich praw, zapytań i celów z tego generatora oraz SHA-256 manifestu, źródeł, binariów, protokołu, progów, konfiguracji i środowiska. Manifest prawdy czyta wyłącznie sędzia. Binarne API sterownika nie otrzymuje manifestu ani seedów. Rozwój i debugowanie korzystają wyłącznie z zestawu rozwojowego.

Próba potwierdzająca uruchamiana jest raz dla zamrożonej wersji. Błąd infrastruktury daje nieukończony przebieg i nowy numer próby, z zachowaniem poprzedniej. Naprawa lub zmiana kryteriów po odczycie wyników oznacza nową wersję protokołu; wyniki na ponownie użytych seedach opisujemy jako powtórzenie po ujawnieniu zestawu, nie świeże potwierdzenie. Nowy zestaw wymaga wcześniejszego zapisania reguły doboru. Nie zamieniamy porażki hipotezy w usterkę bez dowodu złamania kontraktu.

## Porównania i koszty

| Wariant | Co wie i robi | Rola |
| --- | --- | --- |
| A przed doświadczeniem | Ten sam początkowy zbiór hipotez; mediany prognoz bez pomiarów | Czy D wnosi informację poza wyposażeniem |
| A uczący się | Aktywny wybór prób i eliminowanie hipotez | Badany sterownik |
| Losowe próby z pamięcią | Te same 9 interwencji i wnioskowanie, ale wybór interwencji losowy | Drugorzędna kontrola wartości aktywnego wyboru; nie wymagamy jego zwycięstwa |
| Losowy dobór konfiguracji | Jeden losowy łańcuch z 39 na zadanie, bez dostępu do sędziego | Pierwotne porównanie G i poziom trafień przypadkowych |
| B z wiadomością / bez niej | Identyczne stany, budżety i zadania, różnica tylko w danych od A | Pierwotne porównanie S |

Losowa eksploracja ma do 24 impulsów i 1536 eu jak A; nie kończy wcześniej z powodu wiedzy, ale kończy na limicie kroków/energii. Losowy dobór konfiguracji jest jednostajny po 39 konfiguracjach w kolejności: długość, leksykograficzny ciąg symboli. Nie premiujemy krótszego łańcucha samym sposobem losowania długości.

Raport pokazuje liczbę fizycznych prób, zużytą energię, czas świata, czas CPU, szczyt pamięci, rozmiar danych w wiadomości i liczbę ocen hipotez. Wariant bez doświadczenia i losowe działanie mają mniejszy koszt uczenia; nie nazywamy tego porównaniem równych kosztów całkowitych. W porównaniu S liczymy osobno koszt odbiorcy i całej pary. Czasy runnera nie są FPS, czasem myślenia biologicznego ani wynikiem większej populacji.

## Kryteria zapisane przed próbą

Metryki są obliczane najpierw osobno dla każdego z 64 światów, potem ich średnia arytmetyczna. Progi poniżej są operacyjnymi wyborami autora, nie prawami nauki. Raport podaje surowe wartości, nie wyłącznie etykietę zaliczenia.

| Hipoteza | Warunek wspierający w badanym zakresie |
| --- | --- |
| D: zależność nabyta z doświadczenia | Średni udział dokładnych prognoz A w 24 nowych zapytaniach co najmniej 0,90; przyrost względem własnego stanu przed doświadczeniem co najmniej 0,20; dodatni wynik porównania parami według reguły poniżej |
| G: użyteczna generalizacja | Średni udział rzeczywistych trafień A w 20 nowych celach co najmniej 0,80; przyrost względem losowego doboru konfiguracji co najmniej 0,20; dodatni wynik porównania parami |
| S: użyteczny przekaz | Średni udział trafień B z wiadomością co najmniej 0,80; przyrost względem B bez wiadomości co najmniej 0,20; dodatni wynik porównania parami; brak dostępu B do prób A poza kontaktowym pakietem |

Przyrost 0,20 oznacza 20 punktów procentowych. Porównanie parami to jednostronny dokładny test znaków na różnicach wyniku świata. Remisy odrzucamy wyłącznie z testu, pozostawiając je w średnich i tabelach. Dla `n` nieremisowych par i `w` dodatnich wyników `p = sum(comb(n,k), k=w..n) / 2^n`; przy `n=0` przyjmujemy `p=1`. Trzy pierwotne porównania mają próg `p <= 0,05/3`. Brak formalnej analizy mocy pozostaje ograniczeniem; nie wykonano jej przez napisanie tej specyfikacji. Nie używamy liczby zadań jako sztucznego zwiększenia n.

Wynik jest **wspierający**, jeśli spełnia progi i wszystkie kontrole integralności. **Niespełnienie** progu w poprawnie wykonanej próbie podważa tę wersję metody w tych warunkach, nie możliwość uczenia lub komunikacji w ogóle. Raport może zawierać wynik mieszany: np. D i G wspierające, S niespełnione. **Nieweryfikowane** oznacza brak ukończonych danych. **Nieważne** oznacza wykazany przeciek prawdy, błędny sędzia, naruszony bilans lub inny błąd integralności; taki przebieg nie wspiera hipotezy.

Sukces D/G bez przewagi nad losową eksploracją z pamięcią potwierdza uczenie, lecz nie przewagę aktywnego wyboru prób. Taką równoważność lub porażkę zachowujemy. Nie ma obowiązku wygrania z prostszą metodą.

## Kontrole i wyniki nieoczekiwane

Przed uruchomieniem zestawu potwierdzającego techniczne testy mają wykazać:

- Brak wglądu sterownika w seed, parametry, dziennik sędziego i stan innego agenta; zmiana nieobserwowanej prawdy przy identycznym wejściu obserwacyjnym nie zmienia decyzji.
- Prognoza powstaje przed impulsem i nie jest przepisywana po wyniku; ocena nie aktualizuje zamrożonego modelu.
- Bilans i skutek komendy najwyżej raz, także po ponowieniu i kontynuacji zapisu; odmowa nie zmienia źródła, hipotez ani czasu obserwacji.
- B przed kontaktem nie ma danych A; ściana, odległość i inna podpora blokują przekaz; dane pochodzą z naprawdę wykonanych pomiarów, a nie wygenerowanej prawdziwej tabeli.
- Transfer zachowuje źródło i czas, a duplikat nie dodaje nowego dowodu. B odtwarza wnioskowanie z rekordów, nie z modelu A. Usunięcie A po kontakcie nie kasuje legalnie zdobytej wiedzy B.
- Sędzia jest kontrolowany osobno na ręcznie rozliczonych przypadkach wszystkich rodzin, progów, kolejności i całego bilansu. Techniczna kontrola pozytywna z jawnym rozwiązaniem sędziego dowodzi tylko wykonalności aparatury, nie zdolności agenta; nie wchodzi do statystyk.
- Kontrola negatywna: dodatkowa sesja z elementem `out = max(0, min(E, 2*E - 32))`, spoza rodzin sterownika. Ręcznie wymuszona sekwencja 16/32/64 dla tego symbolu ma opróżnić jego zbiór hipotez i zwrócić `model_inadequate`. Jest to test obsługi nieadekwatności, a nie spontanicznego wykrywania nowej fizyki.

Wyniki mogą pokazać przewagę losowej eksploracji, łatwość celów zerowych, zbędność części wiadomości, niejednoznaczne parametry dające trafne prognozy lub brak korzyści B. Nie usuwamy takich światów. Raport pokazuje wynik wszystkich celów oraz osobny opisowy podział zero/niezero, bez zmiany pierwotnych progów. Kontrola spoza rodziny nie wchodzi do 64 światów potwierdzających.

Obserwację nieprzewidzianą zapisujemy z seedem, pełnym śladem i hipotezą wyjaśnienia jako **eksploracyjną**. Następnie tworzymy osobny eksperyment sprawdzający tę hipotezę. Nie zmieniamy wstecznie definicji odkrycia ani sukcesu i nie nazywamy błędu bilansu nową fizyką.

## Integracja z istniejącym Genesis

Odczytany stan kodu: `SimulationClock` posiada jawny stały krok; `WorkWorld` posiada materiały/rezerwacje/czynności; `TraversalSession` posiada ruch i wiedzę lokalną. Obecne `share_knowledge()` przekazuje stanowiska, przejścia i świadectwa incydentów. Nie przekazuje pomiarów aparatury, nie uczy modelu i nie jest implementacją tego eksperymentu.

Projekt przewidział mały moduł `src/research/discovery_world.*` posiadający wyłącznie źródła, impulsy i wynik fizycznego modelu aparatury; `discovery_agent.*` posiada wiedzę i decyzje; `discovery_session.*` łączy aparaturę ze zwykłą sesją przestrzenną; osobny konsument runnera wykonuje protokół. W chwili zatwierdzenia były to projektowane pliki; bieżącą implementację wskazuje osobny dokument API. Nie tworzymy ogólnego frameworka świadomości lub ewolucji.

Sesja przestrzenna pozostaje właścicielem ID, geometrii, pozycji i jedynego zegara (50 ms z `WorkWorld`). Aparatura i kontroler używają przekazanego ticku, bez drugiego zegara rzeczywistego lub automatycznego przesuwania świata przez wnioskowanie. `DiscoveryWorld` posiada energię jako nową, jawnie odrębną wielkość aparatury; nie kopiuje jedzenia, części ani pojemności z WorkWorld i nie zamienia ich jednostek w eu. W tej próbie nie dodajemy takiej konwersji.

Nowa sesja komponuje `TraversalSession` z aparaturą i pamięcią badawczą; nie dodaje fizyki w rendererze. Nowe wąskie, tylko odczytowe zapytania o dostęp do stanowiska i kontakt ujawniają istniejące kontrole `interaction_accessible` i `knowledge_contact`, zamiast drugiej interpretacji geometrii. Kontroler otrzymuje tylko miejscowy widok i własne doświadczenie. Pełny inspektor prawdy pozostaje po stronie badacza. Własne potrzeby starej autonomii są wyłączone w tej kontrolowanej scenie, z jawnym oznaczeniem; nie jest to próba samodzielnej społeczności.

Pomiary przy stole wymagają fizycznego dojścia na miejsce; kontakt po odkrywaniu wymaga rzeczywistego ruchu. Harmonogram faz i dojście do znanych miejsc są dostarczone, nie wyłonione. Oddzielny test numeryczny może izolować wnioskowanie, ale sam nie zalicza kontaktowego przekazu w Genesis. Końcowy wynik S wymaga także śladu przejścia A do B przez wspólną geometrię.

Nowa otoczka zapisu sesji badawczej ma własną wersję i zawiera istniejący zapis podróży, aparaturę, pamięć A/B, stan generatorów używanych przez sterowniki, fazę i liczniki. Nie zwiększa `GENESIS_TRAVERSAL 7`, nie zmienia bajtów dawnych światów i nie udaje, że nową otoczkę czyta stare F9. Brak wsparcia okienkowego jest jawny. Kontynuację nowego eksperymentu należy porównać z oryginalnym procesem, a uszkodzony zapis nie może podmienić sesji. Dziennik badacza i protokół są odrębnymi dowodami, nie kanałem wiedzy NPC.

Dotknięte kontrakty przyszłej implementacji: INV-01/02/03/05/06/08/09/11/12/16. Obecne reguły nie wymagają zmiany; ich nowi konsumenci wymagają testów. Nowy format i API będą opisane w dokumencie odpowiedzialnym za zrealizowany moduł, bez przedstawiania tej propozycji jako istniejącego API.

## Kolejność prac i dowód odbioru

1. Zatwierdzić konkretny protokół; zapisać jego wersję. Następnie przygotować plan implementacji w granicach jednego [programu badań](../research-agenda.md).
2. Napisać testy aparatury, własności wiedzy, prognozy przed wynikiem i braku gotowych parametrów w sterowniku. Oczekiwane braki API/implementacji zachować jako dowód techniczny, nie porażkę hipotezy naukowej.
3. Zrealizować najmniejszy kompletny ciąg: próba A → obserwacja → hipotezy → prognoza → fizyczne wykonanie nowej konfiguracji. Potem rzeczywisty kontakt i osobne wnioskowanie B.
4. Uzupełnić kontynuację, ograniczenia zasobów, referencje i negatywne kontrole; uruchamiać wyłącznie zestaw rozwojowy. Osobny przegląd szuka przecieków, ukrytych podpowiedzi i zależności kryteriów od wyniku.
5. Wykonać właściwe kontrole techniczne Debug/Release/ASan, format, dokumentację i narzędzia Pythona według [środowiska pracy](../development.md). Zachować dawne regresje i limity; żadnych nowych zależności. Dla headless nie przypisywać sobie dowodu nowego obrazu 3D lub komfortu człowieka.
6. Zamrozić manifest i wersję, wykonać zestaw potwierdzający, raportować wszystkie wyniki D/G/S i koszty. Raport badawczy jest zakończeniem zadania także przy poprawnie uzyskanym wyniku negatywnym.

Przyszły zakres plików: małe moduły `src/research/`, konsument w `src/headless/`, wąskie zapytania w `src/lab/traversal_session.hpp/.cpp`, testy badawcze i CMake, narzędzie runnera/analizy z biblioteki standardowej Pythona oraz dokumentacja właścicielska. Bez zmian starej gospodarki, potrzeb, mostu, fikcji lub kosztów pod wynik.

## Stan dowodów

W chwili zatwierdzenia projektu **D, G i S były NIEWERYFIKOWANE**; nie wykonano przebiegów ani analizy mocy. Kontrola dokumentacji i [przegląd projektu](../verification/discovery-learning-design.md) potwierdzały tylko spójność opisu. Aktualny stan, konkretne polecenia, hashe i wyniki zapisuje odrębny [raport wykonania](../verification/discovery-learning-experiment.md), bez nadpisywania kryteriów tego protokołu.
