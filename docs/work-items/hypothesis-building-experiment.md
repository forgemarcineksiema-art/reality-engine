# Budowanie i sprawdzanie wyjaśnień — projekt drugiego eksperymentu

**Status: protokół ZATWIERDZONY do implementacji i wykonania; bez wyników.** 27.10.2026 użytkownik napisał: „Tak, zatwierdzam ten protokół do implementacji i wykonania”. Szczegóły poniżej są zatwierdzonymi wyborami autora, a nie wynikami. Identyfikator protokołu: `hypothesis-v1`. [Plan implementacji](hypothesis-building-implementation.md) śledzi wykonanie bez zmiany kryteriów.

Dokument jest właścicielem proponowanej metody. [Program badań](../research-agenda.md) wskazuje kontekst, [pierwszy protokół](discovery-learning-experiment.md) pozostaje zamrożony, a jego [raport](../verification/discovery-learning-experiment.md) zachowuje wyniki i ograniczenia. [Przegląd projektu](../verification/hypothesis-building-design.md) zapisuje pytania recenzenta i rozstrzygnięcia autora. To osobny przyrost badawczy, bez zmiany odbioru M04 i bez otwierania całego M05/M06.

## Pytanie i możliwe odpowiedzi

Czy po obaleniu dostarczonego katalogu prostych reguł agent skonstruuje z prostszych operacji trafne wyjaśnienie, wybierze próby odróżniające konkurencyjne wyjaśnienia i wykorzysta model w nowych konfiguracjach?

Rozdzielamy **H** — korzyść z budowania wyrażeń oraz **A** — korzyść z aktywnego wyboru prób. H może otrzymać wsparcie przy braku wsparcia dla A. Sam pusty katalog nie jest odkryciem. Wygenerowany zapis matematyczny, który nie przewiduje nowych wyników, również nim nie jest.

Wynik dotyczy syntezy ograniczonych programów z doświadczenia. To nadal dostarczony język i algorytm poszukiwania, o skończonych możliwościach. Nie badamy powstania nowych operatorów, własnych celów, świadomości ani samorzutnego odkrycia praw naszego świata. Wyczerpujące generowanie krótkich programów jest celowo prostym punktem odniesienia; nie nazywamy go nieograniczoną autonomią.

## Wybór przyrostu

| Podejście | Co rozstrzyga | Decyzja |
| --- | --- | --- |
| Dopisanie czwartej gotowej rodziny | Rozpoznanie kolejnej rodziny i parametrów | Za mała zmiana wobec pierwszej próby |
| Składanie wyrażeń z operacji i testowanie ich pomiarami | Czy reprezentacja złożona i własne interwencje pomagają po obaleniu katalogu | Ten eksperyment |
| Jednoczesne uczenie języka, praw, celów i życia | Szersze pytanie, lecz bez czytelnej identyfikacji przyczyny wyniku | Osobny dalszy tor |

Zawężamy układ do **jednego nieznanego elementu**. Dwa pozostałe mają jawne reguły. Pozwala to badać budowanie wyjaśnień bez kosztu iloczynu trzech dużych zbiorów modeli. Przekaz drugiemu agentowi pozostaje pokryty pierwszym eksperymentem; jego rozszerzenie na nowe modele nie jest kryterium tej próby.

## Co dostarczamy

- Aparaturę, miernik, ciało, komendy i pamięć rzeczywiście zaobserwowanych pomiarów z ID, świadkiem i czasem.
- Znaczenie szeregowego łączenia elementów oraz dokładny, deterministyczny charakter pomiarów. Szum i zmiana prawa w trakcie przebiegu pozostają poza zakresem.
- Katalog początkowy, gramatykę wyrażeń, porządek generowania i algorytm wyboru prób. Motywacja do wyjaśniania jest nadana przez nas.
- Limit 24 interwencji, wejścia rozpoznawcze 1–64 eu oraz jawne granice reprezentacji 0–128 eu.

Agent nie otrzymuje drzewa rzeczywistego prawa, seedów, akceptacji generatora światów, kryteriów selekcji światów ani wyników sędziego. Nie dostaje także obietnicy, że rzeczywiste prawo mieści się w jego języku. Sam zdobywa pomiary, zapis sprzeczności, zgodne z nimi programy, prognozy i wybór użytecznej konfiguracji. Dostęp do obserwacji musi przechodzić przez tę samą fizyczną granicę co w [obecnym API](../discovery-learning.md).

## Aparatura i język

Źródło każdej odrębnej sesji zawiera 8192 eu. Energia, impuls, konfiguracja, odmowy, wspólne ID komend i księga strat zachowują semantykę pierwszego laboratorium: ustawienie trwa jeden krok, impuls jeden krok, krok trwa 50 ms, `źródło = pozostało + wyjścia + straty`. Energia jest całkowita; zgodność prognozy wymaga błędu **0 eu**. Wszystkie prawa są pasywne: `0 <= f(E) <= E`.

Symbole oznaczają: `u` — nieznany element, `v(E) = floor(E/2)`, `w(E) = min(E,48)`. Znane prawa v/w są jawne dla wszystkich wariantów. Konfiguracje długości 1–3 dopuszczają powtórzenia. W ocenie wolno wybrać tylko konfiguracje zawierające u: 25 możliwości, więc pominięcie nieznanego elementu nie rozwiązuje zadania.

Autor proponuje następującą małą gramatykę. `t` jest dotychczas zbudowanym wyrażeniem, początkowo wejściem `x`:

| Operacja składana z t | Stałe |
| --- | --- |
| `max(0, t-c)` | `c` ze zbioru C |
| `min(t,c)` | `c` ze zbioru C |
| `floor(q*t/16)` | `q` ze zbioru {4,8,12} |
| `if t >= c then t else 0` | `c` ze zbioru C |

`C = {4,8,12,16,24,32,48,64}`. Każdy program składa się z 0–3 tych operacji: przed usunięciem równoważności istnieje `1+27+27^2+27^3 = 20440` zapisów. To język złożonych ograniczeń, strat i progów, a nie ogólny język dowolnej matematyki. Porównanie i warunek są jawnymi prymitywami; agent nie wynajduje pojęcia progu.

Początkowy katalog **F** to suma 75 reguł pierwszego eksperymentu, tożsamości i wszystkich 27 pojedynczych operacji tej gramatyki, z usunięciem równoważności. Dzięki temu wynik nie może polegać tylko na otrzymaniu nowej stałej albo gotowej rodziny progowej.

Po pierwszej sprzeczności z całym F warianty syntezujące uruchamiają generowanie programów. Najpierw powstają krótsze zapisy, potem porządek operacji z tabeli, następnie rosnące parametry. Programy nie są dostarczone jako tablica gotowych złożonych rodzin. Generator buduje ich składnię w czasie działania; filtr pomiarów odrzuca niezgodne zapisy. Równoważność oznacza identyczne wyjście dla **każdego** całkowitego wejścia 0–128, obliczone z programu, bez pytań do świata. Zachowujemy najkrótszy, pierwszy leksykograficznie reprezentant każdej klasy. Agent zachowuje też dotychczasowe F: nowy zbiór jest sumą F i wygenerowanych programów, a nie wymianą poprawnych prostych reguł.

Agent przechowuje cały zgodny zbiór. Pokazuje najkrótsze wyjaśnienie jako przykład, ale nie udaje jego jednoznaczności. Prognoza to dolna mediana wyjść klas semantycznych oraz zakres min–max. Jednogłośność w obrębie języka oznacza zgodność tych modeli, a nie dowód prawdziwości świata. Pusty zbiór daje `model_inadequate`; wyczerpanie limitu obliczeń daje odrębne `search_budget_exhausted`. Żaden stan nie dopisuje prawdziwego prawa.

Przykład dydaktyczny, **wyłączony z próby potwierdzającej**: `max(0,floor(x/2)-8)`. Pomiary 16→0, 32→8, 64→24 obalają F. Możliwe wyjaśnienia obejmują również `floor(max(0,x-16)/2)`: w naszej dziedzinie to ten sam skutek, więc nie ma podstaw ogłaszać odkrycia jedynego drzewa świata. Nowa prognoza 96→40 jest sprawdzalna; konfiguracja `[u,v]` przy 96 daje 20 eu. Samo wypisanie wzoru bez tych prób nie stanowi sukcesu.

## Warianty i wspólne koszty

| Wariant | Reprezentacja | Wybór wejść po wspólnych trzech próbach |
| --- | --- | --- |
| FA | Wyłącznie katalog F | Aktywny |
| SR | F, a po sprzeczności synteza programów | Losowy |
| SA | F, a po sprzeczności synteza programów | Aktywny |

Każdy wariant rozpoczyna od 16, 32, 64 eu i wykonuje **łącznie 24** różne wejścia z 1–64. Brak niepewności, pusty katalog albo wygenerowanie wyjaśnienia nie skracają próby. Wspólny sufit wynosi 1536 eu i 1000 kroków; fizycznie zużywamy faktyczne wejścia, nie wyrównujemy energii fikcyjnym kosztem. Raport pokazuje osobno liczbę prób, energię i wyniki przy kolejnych prefiksach. Zrezygnowaliśmy tu z odmiennych reguł wczesnego zatrzymania, które ograniczały interpretację kosztu w pierwszym badaniu.

Wybór aktywny minimalizuje największą liczebność zbioru klas zgodnych po możliwym wyniku; remis rozstrzyga mniejsze wejście. Rozważa wyłącznie nieużyte wejścia 1–64. Jeśli żadne nie rozdziela modeli albo zbiór jest pusty, korzysta ze wspólnej, seedowanej permutacji pozostałych wejść. SR zawsze korzysta z tej permutacji. Ten sam seed daje tę samą permutację wszystkim wariantom; odpadają wcześniej wykorzystane wartości. Dla każdego zapytania SA/FA zapisujemy wielkości przewidywanych podziałów i wybór przed wykonaniem impulsu.

Budżety obliczeń na wariant i świat: najwyżej 20440 wygenerowanych zapisów, 10 milionów zastosowań prymitywu podczas tworzenia tabel oraz 300 milionów odczytów wartości modeli podczas uczenia i oceny. F jest dodatkowym zbiorem prostych modeli. Te liczniki nie mogą być resetowane między fazami. Maksymalny rozmiar pamięci procesu, czas ścienny, czas CPU i koszt wewnętrznych obliczeń są raportowane. Docelowy dodatkowy stan modeli ma zmieścić się w 256 MiB; to budżet projektu do sprawdzenia, nie zmierzony wynik. Wszystkie warianty mają te same sufity, choć nie muszą zużywać tyle samo obliczeń. Przekroczenie jest zachowanym wynikiem ograniczenia, bez cichego fallbacku.

**Rozstrzygnięcie autora po przeglądzie projektu:** przed operacją przekraczającą limit ustawiamy trwałe `search_budget_exhausted`. Nie wykonujemy już generowania, filtrowania, nowych prognoz ani planowania konfiguracji. Pozostałe z 24 fizycznych prób wykonuje sterownik według wspólnej permutacji, zapisując rzeczywiste pomiary i koszt bez wnioskowania. Status i liczniki przechodzą przez zapis/wczytanie. Wcześniej utrwalone deklaracje zachowują wynik; każda jeszcze niewydana deklaracja dostaje odmowę i zero. Dotyczy to także wyczerpania limitu w połowie listy prognoz lub podczas końcowego wyboru konfiguracji. Kolejność deklaracji jest stała: kolejne prefiksy, w nich osiem prognoz pojedynczych u rosnąco po wejściu i 16 prognoz łańcuchów w wylosowanej kolejności; po uczeniu 20 celów w wylosowanej kolejności, potem diagnostyka zera. Fizycznie wykonujemy wszystkie już zadeklarowane konfiguracje nawet po limicie obliczeń; odmowy nie zużywają impulsu oceny.

## Światy, zbiory i odcięcie prawdy

Generator eksperymentatora wybiera równomiernie długość 2 lub 3, a potem każdą z 27 operacji niezależnie i równomiernie. Przyjmuje program tylko wtedy, gdy:

1. jego tabela na 0–128 różni się od każdego modelu F;
2. żaden model F nie pasuje do **wszystkich** wyjść u dla 1–64 — obalenie F jest możliwe w dostępnym obszarze;
3. istnieją dodatnie wyjścia z konfiguracji zawierających u przy wejściach oceny, co umożliwia praktyczne zadania;
4. tabela prawa nie jest przykładem dydaktycznym ani przyjętym światem rozwojowym (ten ostatni warunek dotyczy zestawu potwierdzającego).

Warianty dostają identyczny przyjęty świat, ale generator, tablice prawdy i historia odrzuceń pozostają po stronie eksperymentatora. Selekcja jest celowym ograniczeniem populacji badania; nie wolno generalizować wyniku na dowolne prawa. Wszystkie odrzucenia i ich przyczyny zapisujemy. Każdy seed losuje niezależnie, ze zwracaniem względem innych seedów danego zestawu. Raport ujawnia liczbę różnych praw i powtórzenia; nie wymuszamy różnorodności zależnym od wcześniejszych wyników doborem. Limit wynosi 100000 kandydatów na seed; jego wyczerpanie oznacza problem protokołu, bez zastępowania seedu i dopełniania łatwymi przypadkami.

Zbiór rozwojowy: 16 światów z niezależnych seedów `0x48590000..0x4859000F`. Potwierdzający: 64 światy `0x48591000..0x4859103F`. Zamrożony zbiór przyjętych praw rozwojowych jest dodatkowym, stałym wykluczeniem generatora potwierdzającego; stąd zmiana rozwojowej listy wymaga nowego zamrożenia przed próbą. Dla każdego seedu odrzucone losowania kontynuują jego strumień. Generator liczb i domeny liczników wykorzystują [kontrakt pierwszego laboratorium](../discovery-learning.md); nowe tagi: 40 — prawda, 41 — permutacja prób, 42 — prognozy, 43 — zadania. Agent nie otrzymuje seedu ani tagu 40. Przekroczenie pojemności licznika jest błędem, a nie zawinięciem.

Dodatkowe, osobno raportowane kontrole:

- 16 prostych światów F, seedy `0x48592000..0x4859200F`: czy synteza nie psuje zachowania tam, gdzie katalog wystarcza. Wybór: seedowana permutacja klas F i pierwsze 16 różnych tabel. Nie wchodzą do głównego wyniku H/A.
- Cztery jawne światy poza gramatyką: `floor(x*x/d)` dla d = 128, 160, 192, 256, każdy z czterema seedami permutacji prób `0x48593000..0x4859300F` w kolejności d, potem powtórzenie. To 16 przypadków diagnostycznych. Test techniczny musi przed zamrożeniem potwierdzić brak równoważnego programu w F i gramatyce na 0–128; niespełnienie wymaga zmiany projektu, nie pominięcia kontroli. Sprawdzamy, kiedy agent rozpoznaje nieadekwatność albo pozostaje przy pozornie zgodnych wyjaśnieniach. Nie zakładamy, że brak trafnej teorii zawsze da się wykryć w 24 pomiarach.

Podział kontrolny jest widoczny tylko eksperymentatorowi. Agent i jego interfejs pozostają takie same. Owe cztery reguły są kontrolą granic języka, nie reprezentatywną próbą wszystkich praw spoza niego.

## Przebieg i nowe zadania

1. Trzy warianty otrzymują odrębne fizyczne sesje tego samego świata i te same warunki. Konfiguracja/impuls/obserwacja przebiegają przez sesję, nie przez wywołanie sędziego.
2. Każdy wykonuje 24 próby. Po każdym pomiarze zapisuje zbiór zgodnych modeli, pierwszą sprzeczność z F, status syntezy i licznik kosztu. Po próbach 3–24 zapisuje również prognozy dla ustalonej listy oceny. Lista wejść i konfiguracji nie zależy od prawdziwego prawa; nie zwraca się wyjść ani ocen podczas uczenia.
3. Po 24 próbach zamrażamy model. **Rozstrzygnięcie autora po przeglądzie projektu:** po każdym prefiksie powstają wyłącznie prognozy D; wybór konfiguracji i fizyczna ocena G odbywają się tylko po 24 próbach. Cele zależne od prawdy agent otrzymuje dopiero po zamrożeniu, po utrwaleniu końcowych prognoz D. Nie aktualizuje nimi zbioru modeli. Prognozy prefiksowe są niezmiennymi deklaracjami; sędzia liczy ich trafność dopiero po zamknięciu całego przebiegu. Nie uczymy się na odpowiedziach oceny.
4. Ocena prognoz ma 24 pozycje: 8 pojedynczych u przy wejściach 72,80,88,96,104,112,120,128 oraz 16 różnych par (wejście z {80,96,112,128}, konfiguracja długości 2/3 zawierająca u), wylosowanych z ustalonego zbioru bez zwracania. Wszystkie wejścia oceny leżą poza obszarem własnych prób.
5. Ocena użyteczności ma 20 celów. Eksperymentator wylicza zbiór różnych par (wejście z {80,96,112,128}, dodatnie osiągalne wyjście) dla 25 konfiguracji. Losuje z niego 20 par ze zwracaniem; cel wymaga dokładnie tej wartości. Raport ujawnia wielkość puli i powtórzenia. Dobór łatwych celów przez agenta jest wykluczony; generator jest ten sam dla wszystkich wariantów. Są to zadania uwarunkowane osiągalnością, co ogranicza interpretację.
6. Agent wybiera konfigurację maksymalizującą część zgodnych klas trafiających do celu, potem minimalny rozrzut, krótszy łańcuch i kolejność leksykograficzną. Wynik sprawdza rzeczywisty impuls w jawnej nowej sesji oceny, bez dopisywania wiedzy. Pusty zbiór lub limit daje odmowę prognozy/działania, liczoną jako niepowodzenie. Energia i czas oceny są oddzielnie rozliczane; nie są darmowym doświadczeniem treningowym.

Każdy cel jest dodatni: nie można poprawić podstawowego wyniku przez wybieranie znanego wyzerowania przepływu. Cztery dodatkowe zadania z celem zero, jeśli są osiągalne (po jednym na wejście), mają odrębny wynik diagnostyczny i nie zmieniają średniej głównej.

## Kryteria zapisane przed próbą

Jednostką niezależnej analizy jest **świat**, nie pojedynczy impuls. Dla świata liczymy D = udział dokładnych prognoz po 24 próbach i G = udział fizycznie osiągniętych celów. Brak odpowiedzi i przekroczenie budżetu dają zero za odpowiednią pozycję; nie znikają z mianownika. Przerwanie infrastruktury unieważnia wykonanie, bez zastępowania światem o nowym seedzie. Wyniki technicznie poprawne, ale niespełniające hipotezy, pozostają prawidłowym wynikiem badania.

**Wsparcie H** wymaga łącznie na 64 światach:

- SA zarejestrował rzeczywistą sprzeczność z F i zgodny z doświadczeniem program spoza F w co najmniej 90% światów;
- średnia D(SA) >= 90% i G(SA) >= 80%;
- średnie sparowane przyrosty D i G wobec FA wynoszą co najmniej 20 punktów procentowych każdy;
- każdy z dwóch przyrostów przechodzi jednostronny dokładny test znaków z progiem p <= 0,05/3. Remisy odpadają wyłącznie z testu znaków, nigdy ze średniej.

**Wsparcie A** ocenia się osobno: dla każdego świata liczymy średnią D po prefiksach 3–24 (22 wartości). SA ma przewagę nad SR, jeśli średni przyrost tej miary wynosi co najmniej 5 punktów procentowych i test znaków spełnia p <= 0,05/3. Stała liczba prób umożliwia taką samą ocenę obu wariantów. Nie oznacza to równego zużycia energii: dodatkowo publikujemy wykres trafności względem zużytej energii, bez odrębnego twierdzenia o oszczędności na podstawie samego A.

Progi są wyborami autora przed badaniem, nie wyprowadzonymi z danych pierwszego eksperymentu gwarancjami mocy. Wszystkie średnie, różnice i porównania progowe mają dokładną arytmetykę wymierną. Publikujemy też pełny rozkład wyników, liczbę remisów, błędy na pojedynczym u i łańcuchach, wielkość pozostałej niepewności oraz wszystkie kontrole, również gdy test główny jest zielony.

## Miejsce na wynik nieoczekiwany

- Różne drzewa dają to samo prawo w badanej dziedzinie: raportujemy klasę równoważności zamiast przypisywać agentowi odkrycie wewnętrznej konstrukcji aparatu.
- H otrzymuje wsparcie, A nie: korzyść daje język i doświadczenie, a badany wybór prób nie wykazał przewagi nad losowaniem.
- Trafne pojedyncze elementy, nietrafne łańcuchy: sprawdzamy błąd składania, księgowania lub uogólnienia; nie nazywamy go emergencją.
- Model jest zgodny z pomiarami, ale myli się poza obszarem prób: zachowujemy prognozę, zakres i konkurencyjne wyjaśnienia. To ważny wynik o ograniczeniu identyfikowalności.
- Kontrola spoza języka wywołuje pewne, błędne prognozy albo model przypadkiem trafia część zadań: publikujemy oba fakty, bez dopisywania brakującego operatora podczas próby.
- Limit obliczeń przerywa syntezę: uznajemy koszt reprezentacji za ujawnione ograniczenie. Nowa gramatyka, metoda lub budżet wymagają nowej wersji protokołu i osobnej próby.

Dla nieprzewidzianego zachowania zapisujemy rzeczywisty stan i próbę reprodukcji. Późniejsze wyjaśnienie może być analizą eksploracyjną, ale nie staje się wstecznie hipotezą potwierdzoną przez tę samą próbę.

## Zakres implementacji i granice kontraktów

Planowane są osobne pliki `src/research/hypothesis_*`, konsument headless, testy C++ i niezależny sędzia/runner Python bez zewnętrznych pakietów. Nazwy klas i podział plików ustala wykonawca. Reprezentacja programów świata należy do aparatury; modele i historia wnioskowania do agenta; fazy, czas, budżet i ruch do sesji; interpretacja do obserwatora. Nie przenosimy prawdy do NPC ani diagnostyki do sterownika.

Obecne rodziny aparatury nie wykonują dowolnych programów. Wykonawca ma dodać rzeczywistą obsługę proponowanych reguł i użyć wspólnej semantyki komend/bilansu oraz istniejących zapytań dostępu przestrzennego, bez atrap zwracających wyniki z raportu. Wspólny kod można wydzielić dopiero po sprawdzeniu regresji. Pierwszy protokół, jego surowe dane, sumy i historyczne źródła pozostają niezmienione. Obecny skrypt pierwszego badania sprawdza hashe źródeł; dalszy rozwój może wymagać odtwarzania pierwszej analizy w czystym checkoutcie zamrożonego commitu. Nie aktualizujemy dawnych sum do nowego kodu.

Nowy zapis jest osobnym, wersjonowanym formatem badania; nie reinterpretujemy `GENESIS_DISCOVERY 1` ani `GENESIS_TRAVERSAL`. Musi zachować program prawdy, budowane programy i ich porządek, pochodzenie pomiarów, status niewystarczalności/limitu, liczniki obliczeń, strumienie RNG, konfigurację i impuls w toku. Odczyt odrzuca semantyczną sprzeczność także przy poprawnym checksumie. Stare zapisy i wszyscy dotychczasowi konsumenci nadal działają. Dowód kontynuacji obejmuje moment przed/po pierwszej sprzeczności, obie fazy impulsu i niezerowe liczniki wszystkich losujących wariantów.

## Warunki rozpoczęcia i odbioru

Przed wykonaniem potwierdzającym: zatwierdzenie projektu, plan implementacji, testy gramatyki/równoważności i bilansu, niezależna zgodność C++/Python na wszystkich wejściach 0–128, testy odcięcia prawdy, budżetów, prognoz przed obserwacją, braku uczenia podczas oceny, kontroli poza językiem oraz kontynuacji. Regresje pierwszego badania i C++/Python/dokumentacji przechodzą bez osłabiania asercji i limitów. Wymagane jest review kodu i protokołu przed zamrożeniem.

Próba rozwojowa służy poprawności i pomiarowi kosztu; jej dane i porażki zostają zachowane. Zamrożenie obejmuje protokół, źródła, binaria, generator, dokładne wersje narzędzi i listę seedów. Przygotowanie potwierdzających praw i zadań odbywa się dopiero po zamrożeniu; nie oglądamy ich wcześniej. Manifest wiąże odrzucone kandydaty, zaakceptowane światy, deklaracje agentów sprzed pomiaru, fizyczne dzienniki i wyniki sędziego. Każdy confirmatory run ma znacznik jednorazowego wykonania; awaria lub konieczna naprawa zostają jawnie zapisana i nie są zastępowane cichym rerunem.

Raport ma umożliwić niezależne odtworzenie tabeli wyników z pomiarów, ponowne sprawdzenie wyborów aktywnych i kontynuację stanów. Nie wystarczy samo podsumowanie CSV ani zrzut obrazu. To przyrost headless bez nowego widoku 3D i bez ludzkiego odbioru gry. Dopiero po wyniku rozstrzygamy, czy przenosić tę zdolność do mieszkańców społeczności, rozszerzać język, czy zmienić metodę poszukiwania.
