# GENESIS — analiza koncepcji i program badań

Data przeglądu: 24 października 2026 r.

Dokument rozwija [pierwszą koncepcję](PROJEKT_GENESIS.md). Rozdziela wyniki przeglądu publikacji, własne wnioski projektowe, propozycje rozszerzeń i doświadczenia, które dopiero należy przeprowadzić. Nie wykonano jeszcze eksperymentów z silnikiem Genesis, pomiarów wydajności ani badań z graczami. Przegląd jest ukierunkowany, a nie wyczerpujący.

Rozpisanie realizacji: [architektura techniczna](GENESIS_ARCHITEKTURA_TECHNICZNA.md) i [plan prac](GENESIS_PLAN_PRAC.md).

## 1. Najważniejszy wniosek

Największy potencjał Genesis tkwi w zdolności świata do zachowywania historii i tworzenia nowych sposobów działania. Liczba mieszkańców, wielkość mapy i bogactwo dialogów są środkami do tego celu.

Pierwsza koncepcja dobrze opisuje konsekwencje, ale zbyt silnie koncentruje się na kryzysach infrastruktury. Świat powinien także tworzyć dobrobyt, kulturę, przyjaźnie, święta, odkrycia i przedsięwzięcia, których gracz nie zainicjował. W przeciwnym razie rozgrywka może zamienić się w nieustanne usuwanie awarii.

Ambitniejsza obietnica brzmi: **„Wejdź w historię cywilizacji, która potrafi wytwarzać własną przyszłość”.**

To wymaga umożliwienia mieszkańcom uczenia się skutecznych zachowań, przekazywania ich innym i zmieniania otoczenia w sposób, który otwiera następne możliwości.

## 2. Co rzeczywiście pokazują badania

### 2.1. Społeczności agentów: Project Sid

Autorzy opisali eksperymenty w Minecraft obejmujące specjalizację, zmianę wspólnych reguł i przekazywanie treści kulturowych. Podali także istotne ograniczenie: próby z ponad tysiącem agentów przekraczały możliwości środowiska serwerowego i powodowały brak reakcji części agentów; analizę kultury oparto na pojedynczej symulacji 500 agentów.

Autorzy zaznaczają, że modele korzystające z wcześniejszej ludzkiej wiedzy nie demonstrują narodzin instytucji od podstaw. W badaniu rozpowszechniania religii doktryna i grupa jej propagatorów zostały celowo wprowadzone.

Wniosek dla Genesis: odróżniać zachowanie wykonane od zadeklarowanego, pochodzenie nowej praktyki od wiedzy modelu i stabilność długiego działania od jednorazowej demonstracji.

Źródło: [Project Sid, sekcje 5.3 i 7](https://arxiv.org/html/2411.00114v1).

### 2.2. Powstawanie konwencji

Badanie opublikowane w Science Advances w 2025 r. pokazało powstawanie wspólnych konwencji w populacjach modeli na podstawie lokalnych interakcji. Środowiskiem była ograniczona gra uzgadniania nazw; nie była to pełna symulacja społeczna. Wyniki zależały także od użytego modelu.

Wniosek dla Genesis: zwyczaje mogą być przedmiotem mierzalnego eksperymentu. Nie wolno jednak traktować wyniku małej gry koordynacyjnej jako dowodu spontanicznego rozwoju całej ludzkiej kultury.

Źródło: [Ashery, Aiello i Baronchelli, 2025](https://pmc.ncbi.nlm.nih.gov/articles/PMC12077490/).

### 2.3. Uczenie się repertuaru działań: Voyager

Voyager łączy automatyczny dobór zadań, bibliotekę wykonywalnych umiejętności i poprawianie zachowania na podstawie informacji ze środowiska. Pokazuje drogę do kumulowania umiejętności bez konieczności aktualizacji wag modelu przy każdym doświadczeniu.

Wniosek dla Genesis: umiejętność powinna być sprawdzonym sposobem wykonania czynności. Jej przekazanie innej postaci musi zwiększać możliwości tej postaci w świecie. Sama informacja „nauczyłem się naprawiać” jest niewystarczająca.

Źródło: [Voyager](https://arxiv.org/abs/2305.16291).

### 2.4. Otwarte poszukiwanie: Enhanced POET

Enhanced POET bada współtworzenie nowych wyzwań i rozwiązań oraz transfer rozwiązań pomiędzy zadaniami. Publikacja podkreśla znaczenie przestrzeni możliwych środowisk i trudność mierzenia otwartej innowacji.

Wniosek dla Genesis: ogromna liczba kombinacji sama nie gwarantuje rozwoju. Potrzebne są nowe wyzwania, możliwość budowania na wcześniejszych umiejętnościach i pomiar funkcjonalnego postępu. „Nieograniczony rozwój” pozostaje hipotezą badawczą.

Źródło: [Enhanced POET](https://arxiv.org/abs/2003.08536).

### 2.5. Sztuczne życie: Avida

W badaniu Lenski i współautorów cyfrowe organizmy kopiowały się, mutowały, konkurowały i ewoluowały. Powstawały złożone funkcje logiczne. To konkretny przykład badania ewolucji w środowisku cyfrowym.

Wniosek dla Genesis: termin „sztuczne życie” ma już znaczenie naukowe. Nie wymaga ludzkiej twarzy ani przekonującej rozmowy i nie jest równoznaczny ze świadomością.

Źródło: [Lenski i in., Nature, 2003 — tekst u współautora](https://cse.msu.edu/~ofria/pubs/2003LenskiEtAl.pdf).

### 2.6. Świadomość

Artykuł Butlina i współautorów w Trends in Cognitive Sciences proponuje ocenę wskaźników wyprowadzanych z teorii świadomości. Autorzy wskazują na niepewność naukową i ryzyko zarówno nadmiernego, jak i zbyt małego przypisywania świadomości systemom.

Wniosek dla Genesis: deklaracje postaci i złożoność symulacji nie rozstrzygają istnienia przeżyć. Potencjalna ocena wymagałaby osobnych badań architektury i procesów systemu. Ocena jakości gry nie może być testem świadomości.

Źródło: [Identifying indicators of consciousness in AI systems](https://doi.org/10.1016/j.tics.2025.10.011).

## 3. Pytania, które zadałbym własnemu zespołowi

### 3.1. Co gracz robi przez pierwsze dziesięć minut?

Czy ruch, interakcja i pierwsza sytuacja są ciekawe przed ujawnieniem wielkiej symulacji? Jeśli nie, technologia nie zapewni grywalności. Pierwsza operacja powinna pokazywać kilka metod działania, rozpoznawalnych ludzi i łatwy do zaobserwowania skutek.

### 3.2. Co oznacza „żyjący” w naszym projekcie?

Trzeba oddzielnie sprawdzać ciągłość tożsamości, samodzielne planowanie, uczenie się, współpracę, reprodukcję i ewolucję. To różne własności, z których żadna sama nie potwierdza świadomości.

### 3.3. Czy świat potrafi stworzyć coś poza kryzysem?

Mieszkańcy powinni zakładać firmy, organizować opiekę, upiększać miejsca i realizować aspiracje. Dobrowolne przedsięwzięcia tworzą powody, by komuś kibicować. Zniszczenie ma większą wagę, gdy gracz poznał wartość miejsca przed katastrofą.

### 3.4. Czy system wytwarza nowe zachowanie, czy nową opowieść o starym zachowaniu?

Nazwa „spółdzielnia” nie wystarcza. Muszą zmienić się reguły podejmowania decyzji, przepływy zasobów i podział obowiązków. Potrzebujemy dowodu w stanie świata i działaniach uczestników.

### 3.5. Skąd pochodzi pomysł agenta?

Model językowy przynosi ogromną wiedzę spoza świata. Możemy badać lokalną adaptację i nowe zastosowanie wiedzy, ale znacznie trudniej dowieść samodzielnego wynalezienia pojęcia. Syntetyczne materiały, odmienne reguły i nieznane konfiguracje pomagają ograniczyć problem; nie usuwają go całkowicie.

### 3.6. Czy mieszkańcy chcą czegoś, co wynika z ich życia?

Motywacja powinna wpływać na wybór działań, a doświadczenia na jej priorytety. Nie wystarczy losowo przypisać zdania „pragnie wolności”. Rozsądna autonomia wymaga także zwyczajów, satysfakcji z utrzymania stanu i zdolności odpoczynku.

### 3.7. Ile nierówności między graczem a mieszkańcem akceptujemy?

Gracz może wczytać zapis i znać wynik poprzedniej próby. W zwykłym trybie ludzie świata nie powinni znać porzuconej gałęzi. Wyjątki muszą być świadomym elementem fikcji, a nie sposobem ukrywania błędów.

### 3.8. Czy system pozostaje ciekawy, gdy gracz nie ratuje wszystkich?

Inni ludzie muszą umieć pomagać, rozwiązywać konflikty i dokonywać odkryć. Gracz ma być ważnym uczestnikiem. Wszechobecna zależność wszystkich od jego interwencji podważa autonomię świata.

### 3.9. Co robimy ze sprzecznością swobody i reżyserii?

Autorzy mogą przygotowywać konflikty i sceny warunkowe. System rytmu może wybierać moment przekazania istniejącej wiadomości. Zmiana nastroju gracza nie powinna sama tworzyć fikcyjnego braku leków albo usuwać rezerw miasta.

### 3.10. Czy przemoc jest jedyną opłacalną metodą?

Jeżeli zniszczenie działa natychmiast, a budowa wymaga godzin monotonnego zbierania, gracze wybiorą destrukcję. Planowanie, ratownictwo i organizowanie muszą dawać bezpośrednią sprawczość, wyzwanie i atrakcyjny przebieg.

### 3.11. Jak przeżywamy zmianę pokoleń?

Szybkie starzenie w zwykłej akcji grozi utratą więzi i komizmem. Propozycja: dokładny czas przygody oraz dobrowolne przejścia między epokami, poprzedzone wyborem zobowiązań i następców.

### 3.12. Czy różne poziomy szczegółowości zmieniają wynik świata?

Uproszczenia nie będą identyczne z pełną fizyką. Trzeba określić chronione własności, porównywać wyniki prób i utrzymywać wyższą dokładność ważnych zdarzeń niezależnie od odległości kamery.

### 3.13. Jak zapobiegamy utracie różnorodności?

Wspólny model może skłaniać mieszkańców do podobnych wypowiedzi i decyzji. Potrzebujemy różnych doświadczeń, sieci kontaktów, warunków życia i historii uczenia się. Sama zmiana opisu osobowości może nie wystarczyć.

### 3.14. Jak odróżniamy znaczenie od chaosu?

Nowość powinna zmieniać możliwości działania. Losowe nowe nazwy i przypadkowe katastrofy mogą zwiększać zmienność bez zwiększania głębi. Gracz musi móc uczyć się świata i wykorzystywać tę wiedzę.

### 3.15. Czy jesteśmy gotowi zmienić własną wizję po wynikach?

Jeśli kilkuset mieszkańców daje bogatsze doświadczenie niż dziesiątki tysięcy, jakość relacji i czytelność zasługują na pierwszeństwo. Jeśli nowe reguły nie tworzą ciekawych sytuacji, ich koszt obliczeniowy nie jest uzasadniony samą ambicją.

## 4. Ambitniejszy Genesis: pięć rozszerzeń

To propozycje rozwojowe. Nie stanowią zobowiązania do umieszczenia wszystkich w pierwszym wydaniu.

### 4.1. Kultura, która zmienia zachowanie

Mieszkańcy mogą przyjmować i przekazywać zwyczaje: zasady wzajemnej pomocy, formy świętowania, symbole wspólnoty, praktyki zawodowe. Praktyka ma uczestników, koszty i skutki.

Po skutecznej oddolnej ewakuacji ludzie mogą utrzymywać wspólny fundusz ratunkowy. Pomysł przejmuje inne miasto, ale zmienia sposób wybierania opiekunów funduszu. Po latach powstają różne tradycje wywodzące się z jednego wydarzenia.

System śledzi drogę przekazania i rzeczywiste wykorzystanie środków. Dzięki temu kulturę można zobaczyć w życiu, nie tylko przeczytać w dialogu.

### 4.2. Wynalazki z działających części

Świat otrzymuje składane mechanizmy: źródła energii, napędy, przekładnie, sensory, zawory i materiały. Ich połączenia mają mierzalne działanie. Mieszkańcy mogą szukać nowych konfiguracji, testować je i przekazywać sprawdzone projekty.

Nowością może być przystosowanie znanej pompy do lokalnych warunków albo skuteczniejsze rozłożenie dostaw. System nie musi sam wynajdywać nowych praw fizyki, by tworzyć użyteczną innowację.

Pomysły mieszczą się w języku konstrukcji i reguł dopuszczonych przez silnik. Można tworzyć nowe kombinacje, ale nie dowolnie zmieniać prawa świata ani wykonywać nieograniczonego kodu systemowego.

### 4.3. Instytucje, które powstają i przetrwają założycieli

Zamiast zamkniętej listy ustrojów potrzebujemy składanych reguł: członkostwa, reprezentacji, głosowania, własności, zobowiązań i egzekwowania. Ludzie próbują różnych rozwiązań w odpowiedzi na problemy.

Powstała organizacja jest interesująca wtedy, gdy potrafi działać po odejściu założyciela, rekrutować następców i rozwiązywać nowe spory. Deklaracja w dokumencie założycielskim nie jest jeszcze działającą instytucją.

### 4.4. Epoki i archeologia własnej historii

Kampania może obejmować bliski horyzont, a kontynuacja pozwalać na świadome przejście o lata naprzód. Gracz wybiera następcę albo nową postać w tym samym świecie.

Zmiany zachowują ciąg przyczynowy: osada rozwinęła się przy uruchomionej przeprawie; dawny magazyn stał się szkołą; spór o ziemię przetrwał zmianę pokolenia. Wspomnienia prywatne, oficjalne opisy i materialne ślady mogą się różnić.

Epoka nie jest automatycznym wygenerowaniem nowego wyglądu mapy. Wymaga wykonania bądź jawnego przybliżenia działań ludzi, produkcji, migracji i budowy. Przejścia między skalami czasu to osobny problem badawczy.

### 4.5. Ekologia wpływająca na cywilizację

Rozwój osad zmienia dostęp do wody, siedliska i jakość środowiska. To z kolei wpływa na gospodarkę, zdrowie i lokalizację osiedli. W pierwszych próbach wystarczy kilka dobrze połączonych procesów.

Opcjonalne laboratorium sztucznego życia może badać populacje cyfrowych organizmów z dziedziczeniem, zmiennością i selekcją. Nie należy utożsamiać go z gotowym systemem ludzkich NPC ani zakładać, że ewolucja automatycznie wytworzy inteligencję.

## 5. Epickość wynika z uczestnictwa w wielkiej zmianie

Najbardziej widowiskowy moment może być kulminacją działania wielu autonomicznych grup: ewakuacji miasta, uruchomienia nowej przeprawy, wspólnej wyprawy, wielkiego święta albo rozpadu federacji.

Gracz uczestniczy osobiście: dociera do odciętej dzielnicy, negocjuje przejazd, naprawia mechanizm pod presją, osłania ratowników. Równolegle inne grupy wykonują własne plany i mogą odnieść sukces bez jego obecności.

Znaczenie spektaklu pochodzi z rozpoznania ludzi i rzeczy. W ewakuacji pomagają mechanicy z warsztatu poznanego wcześniej. Łodzie dostarcza firma, której rozwój obserwowaliśmy. Nieobecna grupa odmówiła z powodów, które można poznać.

Świat zyskuje skalę emocjonalną, gdy teraźniejsze wydarzenie spotyka się z długą historią konkretnych relacji.

## 6. Czy możemy stworzyć sztuczne życie?

### Cztery odrębne pytania

| Własność | Co można sprawdzać | Czego sama nie dowodzi |
| --- | --- | --- |
| Przekonujące zachowanie | Spójność reakcji i wypowiedzi dla obserwatora | Uczenia się, ewolucji, przeżyć |
| Autonomiczny agent | Samodzielne planowanie i działanie w ograniczeniach | Samopodtrzymania, reprodukcji, świadomości |
| Sztuczne życie w wybranym modelu badawczym | Dziedziczenie, zmienność, selekcję, adaptację; zależnie od definicji także inne własności | Ludzkiego umysłu i świadomego doświadczenia |
| Świadomość | Wskaźniki związane z konkurującymi teoriami i architekturą systemu | Nie daje się jej potwierdzić samą deklaracją postaci |

Operacyjny cel sztucznego życia jest sensowny, jeśli jawnie określimy, jakie własności badamy. Zdolność reprodukcji nie jest uniwersalnym wymogiem dla każdego pojedynczego żywego osobnika; jest ważna przy badaniu ewoluujących populacji.

„Przypadkowo” może pojawić się nieprzewidziane zachowanie. Uznanie go za życie lub świadomość wymaga definicji i niezależnej analizy. Nieprzewidywalność jest niewystarczająca: proste systemy też potrafią być trudne do przewidzenia.

Postać mówiąca „wiem, że jestem w symulacji” może korzystać z tropu obecnego w danych treningowych lub kontekście. Wypowiedź trzeba badać względem alternatywnych wyjaśnień. Nie stanowi samodzielnego odkrycia ani świadectwa przeżyć.

Gdyby rozwój projektu dostarczył poważnych przesłanek dotyczących świadomości, potrzebna byłaby odrębna ocena naukowa i etyczna. Nie projektowalibyśmy marketingowego testu, który z góry ma ogłosić „narodziny świadomości”.

## 7. Czy taka gra coś mówi o naszym świecie?

Może pokazać, jak z lokalnych reguł powstają złożone, zaskakujące zachowania. Może też skłonić odbiorcę do zastanowienia się nad sprawczością, pamięcią i statusem cyfrowych istot.

Powstanie przekonującej symulacji nie dowodzi, że nasz wszechświat jest symulowany. Między możliwością zbudowania określonego świata cyfrowego a twierdzeniem o pochodzeniu naszego świata pozostaje osobny problem argumentacji i dowodów.

Najsilniejszy efekt artystyczny uzyskujemy wtedy, gdy gracz sam zaczyna traktować mieszkańców jako kogoś wartego uwagi. Nie musimy przedstawiać przypuszczeń o rzeczywistości jako ustaleń naukowych.

## 8. Program doświadczeń

Poniższe liczby opisują propozycje prób, a nie wyniki. Po pilotażu należy dopasować liczby powtórzeń i czas obserwacji do zmienności badanego zjawiska.

| Doświadczenie | Próba | Dowód, którego szukamy | Co podważa tezę |
| --- | --- | --- | --- |
| Autonomia | Około 100 mieszkańców, 30 dni symulacji, seria różnych warunków początkowych | Ciągłość działań, bilanse, sensowne reakcje i stabilny zakres funkcjonowania bez gracza | Świat działa tylko dzięki dopisywanym ratunkom lub zapętla się |
| Adaptacja | Zablokowana trasa, zmiana popytu, utrata fachowca | Działające plany zastępcze i ich rzeczywiste koszty | Zmieniają się wyłącznie wypowiedzi |
| Kultura | Powstanie lokalnej praktyki, przekazanie jej nowym mieszkańcom i odejście założyciela | Praktyka wpływa na późniejsze decyzje i ma udokumentowany kanał transmisji | Zachowanie znika albo pochodzi z ukrytego globalnego polecenia |
| Wynalazczość | Nowa konfiguracja mechanizmów i warunków, bez gotowej recepty | Powtarzalnie działające rozwiązanie oraz transfer umiejętności | Sama nowa nazwa, błąd silnika albo ukryta gotowa procedura |
| Spójność świata | Uszkodzenie mostu, zmiana skali symulacji, zapis i ponowne uruchomienie | Zachowanie własności, ilości, szkód i konsekwencji dla tras | Duplikacja ładunków, teleportacja lub zniknięcie szkód |
| Wartość dla gracza | Porównanie równoważnych scenariuszy z różną głębią symulacji | Rozpoznawanie osób, zrozumienie skutków i wykorzystywanie nowych możliwości | Duży koszt bez zauważalnego wpływu na doświadczenie |

### Warunki rzetelnego porównania

Każda próba potrzebuje prostszego punktu odniesienia: rutynowych skryptów, planera bez uczenia albo wariantu bez określonego kanału pamięci. Porównanie obejmuje także koszt obliczeń; nie wystarczy dać jednej wersji znacznie większy budżet i nazwać wyniku przełomem architektury.

Powtarzamy scenariusze i zapisujemy również niepowodzenia. Ocenę kultury opieramy na zachowaniu, nie wyłącznie na etykietach nadanych przez inny model. Testy z nowymi warunkami ograniczają ryzyko dopasowania systemu do pojedynczego pokazu.

Eksperymenty z odgałęzieniami historii wymagają utrwalenia początkowego stanu i kontrolowania źródeł losowości. Przy niedeterministycznych modelach i fizyce porównujemy serie wyników, a nie zakładamy, że jedno ziarno zapewnia identyczny przebieg.

## 9. Koszt myślenia jest osobnym ograniczeniem

Przykład czysto obliczeniowy: 10 000 mieszkańców podejmujących ważną decyzję co 10 sekund to 1000 decyzji na sekundę. Jeśli tylko 1% trafia do modelu językowego, pozostaje 10 wywołań na sekundę. Nie określa to jeszcze ceny ani wydajności, bo te zależą od modelu, kontekstu i sposobu wykonania.

Potrzebujemy hierarchii: nawyki i reakcje lokalne; planowanie z ograniczeniami; rzadkie wykorzystanie modeli; wspólne narzędzia wiedzy; kontrolowane budżety czasu. Równie istotne są wyszukiwanie wspomnień, aktualizacje zależności, nawigacja i zapis stanu.

Uczenie się nie musi oznaczać treningu sieci po każdej rozmowie. Może polegać na zachowaniu skutecznej procedury, zmianie przewidywań, adaptacji priorytetów i przekazaniu umiejętności. Efekty trzeba sprawdzać w działaniu.

## 10. Dwa powiązane tory rozwoju

**Genesis Game** odpowiada za osobistą przygodę, przywiązanie do ludzi, jakość akcji, czytelność i atrakcyjną kampanię.

**Genesis Lab** odpowiada za autonomię, uczenie, przekaz kultury, składanie instytucji, otwartą innowację i opcjonalne badania sztucznego życia.

Oba tory korzystają ze wspólnego modelu świata i dziennika zdarzeń. Laboratorium może przyspieszać czas, wykonywać powtórzenia i badać uproszczoną prezentację. Gra zachowuje tempo potrzebne graczowi. Rozwiązania z laboratorium trafiają do produktu po sprawdzeniu ich wartości i kosztu.

Pierwszym mocnym celem badawczym byłaby społeczność, która po utracie ważnej trasy sama opracowuje i utrzymuje nową formę współpracy, przekazuje ją następnym mieszkańcom i wykorzystuje w innym kryzysie. Pierwszym celem gry byłaby sytuacja, w której gracz rozpoznaje tę zmianę, rozumie własny udział i otrzymuje dzięki niej nowy sposób działania.

**Dopiero połączenie tych dwóch rezultatów zaczyna dowodzić obietnicy Genesis.**
