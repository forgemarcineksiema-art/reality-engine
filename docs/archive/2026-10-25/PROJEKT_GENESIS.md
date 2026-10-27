# PROJEKT GENESIS

## Koncepcja gry i programu technologicznego — wersja 0.1

**Robocza nazwa gatunku: Living Civilization Action RPG.**

**Obietnica dla gracza: „Świat żyje własnym życiem. Możesz zmienić jego bieg — i zamieszkać w konsekwencjach”.**

Dokument przedstawia autorską propozycję gry. Funkcje, liczebności i parametry są założeniami projektowymi wymagającymi prototypowania, a nie opisem istniejącego silnika ani potwierdzoną specyfikacją produktu.

Rozwinięcie: [analiza koncepcji, przegląd badań i propozycje ambitniejszego kierunku](GENESIS_ANALIZA_I_PROGRAM_BADAN.md).

Realizacja: [architektura techniczna i wykonalność](GENESIS_ARCHITEKTURA_TECHNICZNA.md), [plan rozpoczęcia prac](GENESIS_PLAN_PRAC.md).

Decyzja technologiczna: [własny Reality Engine](REALITY_ENGINE_WLASNY_SILNIK.md), obejmujący renderer, streaming i narzędzia.

Ustalenia pierwszego etapu: grywalny fragment i laboratorium małej autonomicznej cywilizacji, własna fizyka oraz nawigacja, minimum bibliotek, prosta grafika. Użytkownik pracuje ze swoimi agentami AI bez narzuconego terminu, na laptopie i5/MX330/8 GB RAM. Liczebności i zakresy dalszych etapów poniżej pozostają hipotezami do zweryfikowania.

## 1. Co właściwie tworzymy

Trzecioosobowe Action RPG, w którym gracz jest fizycznie obecną osobą wewnątrz symulowanej cywilizacji. Walczy, prowadzi śledztwa, podróżuje, nawiązuje relacje, pracuje i organizuje przedsięwzięcia. Z czasem uzyskuje możliwość wpływania na przedsiębiorstwa, instytucje i miasta.

Świat ma własne potrzeby, zasoby, historię i konflikty. Ludzie próbują osiągać cele również wtedy, gdy gracz ich nie obserwuje. Organizacje konkurują o rzeczywiste zasoby. Działania gracza zmieniają te same systemy, z których korzystają mieszkańcy.

Podstawową jednostką projektu jest **sytuacja z wieloma sposobami ingerencji i trwałym następstwem zdarzeń**. Otwieranie kolejnych obszarów mapy pozostaje częścią przygody, lecz główna głębia wynika z poznawania i zmieniania zależności.

„Tysiące konsekwencji” oznacza kombinacje oddziaływań między systemami. Nie każda akcja uruchamia tysiąc osobnych misji. Większość działań ma skutki lokalne, część rozpowszechnia się przez sieci społeczne, gospodarcze i materialne, a wyjątkowe zdarzenia zmieniają historię regionu.

## 2. Świat: Wolne Dorzecze

Akcja rozgrywa się w alternatywnej bliskiej przyszłości, w rozległym dorzeczu otoczonym górami i wybrzeżem. Porty, miasta przemysłowe, tereny rolnicze, zbiorniki retencyjne i osady górnicze tworzą jedną cywilizację zależną od wspólnej infrastruktury.

Po rozpadzie centralnej administracji miasta odzyskały samodzielność. Przetrwały drogi, zakłady, uniwersytety i technologia, ale rozpadły się dawne gwarancje dostaw, bezpieczeństwa i współpracy. Ludzie nadal pracują, zakochują się, wychowują dzieci, organizują święta i budują nowe życie. Zagrożenie miesza się z normalnością.

Głównym dziedzictwem dawnego porządku jest sieć GENESIS: zespół połączonych instalacji wodnych, energetycznych i logistycznych. Można ją odbudować na różnych zasadach własności i kontroli. Samo odzyskanie urządzeń nie rozstrzyga, komu powinny służyć.

Ten świat uzasadnia jednoczesne występowanie pościgów, broni palnej, wspinaczki, infiltracji, ratownictwa, handlu, sporów politycznych i prac inżynieryjnych. Daje też wspólny powód, dla którego odległe miasta odczuwają swoje decyzje.

Kierunek wizualny: materialny realizm, mocne sylwetki, wyraźne zużycie przedmiotów i ślady ludzkiej pracy. Dzielnica zmienia wygląd wraz z remontami, zamożnością, bezpieczeństwem i nowymi mieszkańcami. Najważniejsze zmiany muszą być widoczne podczas zwykłego spaceru.

## 3. Kim jest gracz

Zaczynasz jako niezależny terenowy specjalista przybywający do portowego miasta Narew. Wybierasz pochodzenie: medycyna, transport, mechanika, służby bezpieczeństwa, handel lub praca dochodzeniowa. Pochodzenie daje kompetencje, kontakty i zobowiązanie osobiste.

Pierwsza praca polega na dostarczeniu części do przepompowni. Wypadek na trasie ujawnia konflikt między miastem, prywatnym operatorem i ludźmi odciętymi od dostaw. Masz osobisty powód, by działać, ale żadnego uniwersalnego prawa do wydawania poleceń.

Możesz zostać między innymi ratownikiem, najemnikiem, przemytnikiem, śledczym, przedsiębiorcą, organizatorem społeczności albo wpływowym pośrednikiem. Są to role wynikające z umiejętności, relacji i podejmowanych działań. Gra nie wymaga ukończenia osobnej kampanii dla każdej profesji.

Gracz ma przewagę dzięki mobilności, wiedzy, kontaktom i gotowości do ryzyka. Wpływ na miasto zdobywa przez ludzi i instytucje: właściciel warsztatu może przekonać pracowników, zlecić naprawę i kupić materiały; samotny wędrowiec musi najpierw znaleźć kogoś, kto ma takie możliwości.

## 4. Pętla rozgrywki

**Dostrzegasz problem → poznajesz ludzi i zależności → zdobywasz środki → wybierasz sposób działania → wykonujesz plan → obserwujesz następstwa → podejmujesz nowe zobowiązania.**

W skali kilkudziesięciu sekund gra oferuje ruch, strzelanie, walkę wręcz, skradanie i używanie przedmiotów. W skali kilkunastu minut: penetrację miejsca, rozmowy, zdobywanie informacji, przygotowanie i wykonanie operacji. W skali wielu godzin: rozwój relacji, organizacji i regionu.

Przykładowy wieczór gry: dowiadujesz się o zaginięciu mechaniczki, rozmawiasz z jej rodziną, śledzisz przejęty transport, przekonujesz kierowcę do współpracy, infiltrujesz magazyn, uwalniasz jeńców i organizujesz wywóz. Później poznajesz skutki dla warsztatu, dostaw i ludzi odpowiedzialnych za porwanie.

Informacja jest nagrodą: poznanie harmonogramu dostaw, faktycznej lojalności dowódcy czy sposobu zasilania posterunku może otworzyć skuteczniejszą drogę niż lepsza broń.

## 5. Action RPG, które działa bezpośrednio pod palcami

Podstawą jest kamera trzecioosobowa, precyzyjne celowanie, czytelne reakcje i szybkie używanie narzędzi. Ruch obejmuje wspinanie, przeskakiwanie przeszkód, przemieszczanie po dachach, pływanie i pojazdy odpowiednie do świata.

Walka opiera się na osłonach, pozycji, hałasie, widoczności, wyposażeniu i morale. Ogień zaporowy, uszkodzenie oświetlenia, negocjowanie kapitulacji oraz odcięcie odwrotu są realnymi sposobami rozwiązania starcia. Różnice sprzętu i wyszkolenia mają znaczenie, a rozwój nie zamienia człowieka w przeciwnika przyjmującego dziesiątki trafień bez reakcji.

Przeciwnicy mają cele: chronić transport, opóźnić natarcie, wydostać rodzinę, utrzymać magazyn. Mogą wycofać się, poprosić o wsparcie, negocjować lub poddać. Zaopatrzenie, rany i straty wpływają na ich kolejne działania.

Ta sama przestrzeń wspiera działania pokojowe i bojowe. Okno służy do obserwacji, ewakuacji i wejścia. Pojazd może być środkiem transportu, osłoną albo sposobem wyciągnięcia zablokowanej bramy. Narzędzia umożliwiają naprawę, sabotaż, otwieranie przejść i akcje ratunkowe.

Skutki porażki obejmują utratę ładunku, areszt, rany, dług lub przymusowy odwrót. Zwykły tryb pozwala zapisywać i wczytywać grę. Dobrowolny tryb dziedzictwa umożliwia kontynuowanie świata inną postacią po śmierci bohatera; nie jest warunkiem korzystania z symulacji.

## 6. Trwały człowiek zamiast odtwarzanego statysty

Każdy mieszkaniec symulowanej populacji ma trwałą tożsamość, miejsce zamieszkania, źródło utrzymania, majątek, relacje, potrzeby, zdolności i ograniczoną wiedzę. Nie wszyscy otrzymują równie bogaty dialog i tak samo szczegółową psychologię.

Planowanie rozdziela cztery horyzonty: reakcję na bezpośrednie zdarzenie, zadanie bieżące, plan na najbliższe dni i długoterminową aspirację. Pracownica może chcieć kupić mieszkanie, ale po chorobie dziecka zmienia priorytety. Plan uwzględnia czas, drogę, pieniądze, dostępne usługi i ryzyko.

Pamięć zapisuje faktyczne zdarzenia oraz ich osobistą interpretację: kto pomógł, kto obiecał zapłatę, kto był widziany przy magazynie. Kluczowe zobowiązania i wydarzenia pozostają trwałe. Rutynowe wspomnienia mogą być streszczane i tracić znaczenie.

Relacja ma kilka wymiarów: zaufanie, przywiązanie, strach, szacunek, interes i zobowiązanie. Ktoś może bać się gracza, szanować jego umiejętności i odmawiać współpracy. Podarunek nie kasuje krzywdy.

Postać wie to, czego doświadczyła lub dowiedziała się z określonego źródła. Plotka ma autora, drogę przekazania, wiek i wiarygodność. Zdarzenie może pozostać nieznane. Uznanie gracza za sprawcę wymaga rozpoznania, zeznań lub innych dowodów.

Istotne postacie mogą umrzeć. Instytucja może wybrać następcę, ale następca ma własną osobowość i nie przejmuje automatycznie osobistych wspomnień poprzednika. Niektóre historie zamykają się wraz ze śmiercią ich uczestników; inne przechodzą w śledztwo, walkę o spadek lub konflikt o sukcesję.

## 7. Organizacje jako uczestnicy świata

Organizacja ma członków, władze, zasady, pieniądze, zasoby, kompetencje i cele. Jej deklaracje wymagają wykonania przez konkretnych ludzi.

Rada miejska może ogłosić racjonowanie, ale potrzebuje magazynierów, ewidencji, transportu i akceptacji albo zdolności egzekwowania prawa. Firma ochroniarska może zaoferować eskortę, o ile ma sprawny pojazd, załogę i wynagrodzenie.

Wewnątrz organizacji występują rozbieżne interesy. Właściciel zakładu chce utrzymać produkcję, kierowniczka boi się awarii, a pracownicy protestują przeciw zaległym płacom. Gracz może zmieniać sytuację na każdym z tych poziomów.

Przejęcie organizacji oznacza również odpowiedzialność: koszty, zobowiązania, utrzymanie zaufania i wybór ludzi. Delegowanie pozwala wyznaczać cele i budżet, a podwładni realizują je zgodnie z kompetencjami i lojalnością. Zarządzanie ma wspierać przygodę w świecie.

## 8. Gospodarka i społeczeństwo

Pierwsza pełna wersja gospodarki obejmuje ograniczony katalog ważnych dóbr: żywność, wodę, paliwo, energię, leki, części i materiały budowlane. Towary są śledzone jako partie z ilością, miejscem i właścicielem. Nie trzeba symulować każdego ziarna.

Produkcja wymaga wejść, narzędzi, pracowników i czasu. Dostawa wymaga trasy oraz dostępnej przepustowości. Cena reaguje na zapasy, przewidywany popyt, konkurencję i ryzyko. Wynagrodzenia, podatki i własność rozdzielają środki według jawnych reguł. Ewentualna emisja pieniądza lub kredyt muszą mieć zdefiniowanego wystawcę i zapis transakcji.

Przerwanie jednej dostawy nie oznacza natychmiastowego głodu. Magazyny, zastępcze trasy, import i reakcje ludzi amortyzują problem. Długotrwałe zakłócenia mogą prowadzić do drożyzny, zamykania działalności, protestów, migracji i zmiany poparcia.

Prawo określa dozwolone działania, sankcje i procedury. Jego egzekwowanie zależy od instytucji, zasobów, dowodów i lokalnych praktyk. Przyjęcie przepisu i jego skuteczne stosowanie to odrębne wydarzenia.

Najważniejszy test gospodarki: po zmianie warunków istnieje wyjaśnialna droga od konkretnego braku lub nadwyżki do decyzji ludzi. Gdy zachodzi kryzys, gra pozwala szukać rozwiązań: naprawy, nowych umów, oszczędności, zastępczej produkcji lub zmiany organizacji dostaw.

## 9. Jedna sytuacja: przepompownia

Miasto Narew korzysta z przepompowni, której zasilanie dzieli z fabryką leków. Operator oszczędzał na konserwacji. Zbliża się wezbranie rzeki, pracownicy żądają zapłaty, a jedna z dzielnic przygotowuje blokadę drogi.

Ręcznie napisane elementy to konkretni ludzie, ich historia, dokumenty, relacje i powód konfliktu. Wynik zależy od stanu świata i działań uczestników.

Możliwe ingerencje:

- Zdobyć części i zorganizować naprawę, uzgadniając kolejność zasilania.
- Ujawnić oszustwa operatora i doprowadzić do zmiany zarządu.
- Ochronić dostawę leków, pozwalając na czasowe ograniczenie pracy pomp.
- Przejąć sterownię siłą i narzucić priorytety.
- Wybudować obejście, znajdując materiały, fachowców i czas.
- Sprzedać informacje konkurentowi albo wycofać się z konfliktu.

Przykładowy przebieg po gwałtownym wyłączeniu zakładu:

1. Zatrzymuje się produkcja; uszkodzenie chłodzenia stwarza ryzyko utraty części zapasów.
2. Kierownictwo szuka agregatu, części lub dostaw zastępczych.
3. Jeżeli próby zawiodą, niektóre przychodnie ograniczają usługi.
4. Osoby mające pieniądze szukają innych źródeł; pozostali potrzebują pomocy.
5. Organizacja sąsiedzka uruchamia transport do innego miasta, zyskując zaufanie.
6. Ujawnione dowody mogą obciążyć operatora, napastników lub gracza. Różne grupy przyjmują różne wersje zdarzeń.
7. Podczas późniejszego kryzysu nowa sieć współpracy pomaga dzielnicy działać samodzielnie.

To jeden możliwy przebieg. Rezerwy leków, sprawny agregat, wcześniejsza umowa i działania mieszkańców mogą przerwać ten łańcuch. Powiązania przyczynowe tworzą możliwości, a nie obowiązkową sekwencję kar.

Po kilkunastu godzinach gracz może spotkać kierowcę, który dzięki tamtym wydarzeniom założył firmę transportową, albo rodzinę, która zmieniła dzielnicę. Długotrwałe skutki uzyskują twarze i miejsca.

## 10. Interakcja i materialna destrukcja

Otoczenie składa się z elementów o wspólnych właściwościach: masie, nośności, wytrzymałości, palności, przewodzeniu i połączeniach z innymi elementami. Zakres parametrów zależy od klasy obiektu i znaczenia dla rozgrywki.

Uszkodzenie podpory zmienia stan konstrukcji, kolizję, dostępne przejścia i bezpieczeństwo. Przecięty przewód przestaje dostarczać energię do połączonego urządzenia. Zablokowana droga zmienia trasę transportu. Ogień może zniszczyć dokumenty, wyposażenie i zapasy.

Najważniejsze klasy konstrukcji — budynki, mosty, instalacje przemysłowe — otrzymują narzędzia autorskie do określania połączeń i przewidywalnych sposobów uszkodzenia. Szczegółowe efekty wizualne mogą być przybliżone. Stan decydujący o działaniu świata pozostaje trwały.

Ruina nie znika wraz ze zmianą obszaru. Duży gruz może stać się trwałą przeszkodą, a drobny przejść w zbiorczy stan rumowiska. Sprzątnięcie wymaga pracy. Odbudowa może zmienić przeznaczenie miejsca, właściciela i wygląd.

Naprawa jest równie ważna jak niszczenie. Gracz może przywrócić przejście, podeprzeć konstrukcję, odłączyć zagrożoną instalację, sprowadzić ekipę lub sfinansować odbudowę. Fizyczna zmiana jest początkiem dalszej rozgrywki.

## 11. Ręcznie projektowane historie w zmiennym świecie

Autorzy projektują postacie, tajemnice, konflikty wartości, miejsca, odkrycia i mocne sceny. Narzędzia opisują warunki rozpoczęcia oraz wymagania scen: uczestników, wiedzę, przedmiot sporu i możliwe skutki.

Misja „Ostatnia zmiana” dotyczy ukrytej wady przepompowni. Jej emocjonalnym rdzeniem jest inżynierka próbująca ochronić ludzi i ujawnić zaniedbania. W zależności od świata gracz spotyka ją przed awarią, podczas ewakuacji albo po utracie zakładu.

Jeśli inżynierka zginie, historia nie udaje, że żyje. Część treści przepada, a przygotowane przez autorów ścieżki mogą prowadzić do jej dokumentacji, rodziny lub odpowiedzialnych instytucji. Nie każda rola daje się zastąpić.

System narracyjny wyszukuje warunki pasujące do napisanych historii, eksponuje istniejące konflikty i podsuwa punkty kontaktu. Nie może potajemnie dokładać brakujących dowodów, wskrzeszać postaci ani wywoływać awarii tylko dlatego, że gra potrzebuje napięcia.

Emergentne zadania powstają z rzeczywistych potrzeb: szpital zamawia transport, ponieważ brakuje leków; rodzina płaci za poszukiwania, ponieważ jej bliski nie wrócił. Powtarzalna forma ma znaczenie dzięki konkretnym uczestnikom i następstwom. Ręczne historie zapewniają dodatkowo rytm, temat i autorską jakość.

## 12. Kampania i zakończenia

Główną osią jest odbudowa współpracy między miastami przed sezonem powodziowym. Obejmuje uruchamianie węzłów GENESIS, poznawanie przyczyn rozpadu dawnego porządku i ustalanie, kto ma prawo zarządzać nową siecią.

Trzy autorskie nurty przeplatają się z symulacją: odpowiedzialność za dawną katastrofę, los rozdzielonej rodziny oraz konflikt o własność infrastruktury. Nie każda postać i scena musi przetrwać wszystkie decyzje gracza.

Kulminacja sprawdza wcześniej wypracowany stan: sprawność instalacji, dostępne ekipy, zapasy, porozumienia i gotowość miast do współpracy. Obejmuje bezpośrednią akcję — na przykład ewakuację i naprawę w uszkodzonym węźle — oraz skutki wcześniejszych zobowiązań.

Zakończenia mogą obejmować sprawną federację, monopol jednej organizacji, luźną współpracę samodzielnych miast albo region podzielony i częściowo opuszczony. Autorzy przygotowują interpretację tych stanów oraz osobiste domknięcia. Świat może działać dalej po zakończeniu kampanii.

## 13. Rozwój i trwałość

Postęp obejmuje umiejętności praktyczne, dostęp do narzędzi, wiedzę, kontakty, zaufanie i zdolność organizowania ludzi. Warsztat, łódź, bezpieczne miejsce oraz fachowy współpracownik potrafią otworzyć więcej możliwości niż kolejny poziom postaci.

Gracz rozwija własne zaplecze. Może zatrudnić ludzi, ustalić zasady działalności i delegować pracę. Zyskuje możliwości, ale również odpowiedzialność za zobowiązania. Awans społeczny nie usuwa potrzeby osobistego uczestnictwa w ważnych operacjach.

Świat zachowuje historię między sesjami. Domyślnie zatrzymuje się po wyjściu z gry, aby nie uzależniać bezpieczeństwa mieszkańców od obecności użytkownika przy komputerze.

Czas przyspiesza przy odpoczynku, podróży i uzgodnionych przeskokach. Dłuższy przeskok wymaga podsumowania spraw pilnych i świadomego wyboru gracza. Kampania obejmuje miesiące; przemiany pokoleniowe należą do opcjonalnej późniejszej kontynuacji i nie są obietnicą pierwszego wydania.

## 14. Jak gracz rozumie konsekwencje

Reakcje świata muszą być czytelne podczas gry: kolejka do studni, odwołany transport, zamknięty warsztat, nowa ekipa remontowa, zmiana rozmów na ulicy.

Osobisty dziennik rozdziela obserwacje, zasłyszane informacje i hipotezy. Przy wiadomości można poznać źródło oraz czas. Gracz nie otrzymuje wszechwiedzy o wszystkich zależnościach.

Zakładka zobowiązań przypomina, komu coś obiecaliśmy, co jest potrzebne i jaki termin faktycznie ustalono. Pilne kryzysy są komunikowane. Większość zadań pozostawia przestrzeń na eksplorację, a terminy wynikają z sytuacji.

Widok organizacyjny pokazuje informacje posiadane przez własnych pracowników i kontakty. Odrębne narzędzie deweloperskie udostępnia pełny łańcuch przyczynowy. Może służyć także opcjonalnej analizie zakończonej rozgrywki.

Brak wiedzy o następstwie może być zamierzonym elementem historii. Niezrozumiały błąd systemu nie może podszywać się pod tajemnicę.

## 15. Technologia: jeden świat, wspólny stan

GENESIS jest nazwą programu i gry. **Reality Engine** to docelowy zintegrowany silnik, którego moduły pracują na wspólnym modelu świata.

| Moduł | Odpowiedzialność | Przykład wyniku widocznego w grze |
| --- | --- | --- |
| EARTH | Populacja, gospodarka, infrastruktura, środowisko | Niedostępna droga opóźnia dostawę i zmienia pracę zakładu |
| EVOLUTION | Potrzeby, pamięć, wiedza, relacje, plany i instytucje | Świadek zgłasza zdarzenie, jeśli je zna i ma powód działać |
| FORGE | Materiały, połączenia konstrukcyjne, uszkodzenia i odbudowa | Uszkodzony most ma ograniczoną przepustowość lub staje się nieprzejezdny |
| CHRONICLE | Historie autorskie, role, warunki scen i pochodzenie zdarzeń | Rozmowa odnosi się do rzeczywistego losu zakładu |
| Warstwa prezentacji i sterowania | Rendering, animacja, dźwięk, kamera, wejście | Ten sam uszkodzony most jest widoczny i fizycznie nieprzejezdny |

Most ma jeden identyfikator i uzgodniony stan. Rendering, fizyka, nawigacja, logistyka i fabuła korzystają z odpowiednich reprezentacji tego samego obiektu. Przeliczenie uszkodzenia uruchamia jawny zestaw zmian, zamiast pozostawiać pięć rozbieżnych wersji rzeczywistości.

Reality Engine jest własnym silnikiem z własnym rendererem, streamingiem, fizyką, nawigacją i narzędziami. Zależności biblioteczne ograniczamy do minimum. Integracja oznacza wspólne zasady własności danych, aktualizacji i komunikacji. Zakres fundamentów oraz kolejność realizacji opisuje dokument własnego silnika.

### 15.1. Hierarchiczna symulacja

Blisko gracza działają szczegółowa fizyka, percepcja, animacja i zachowanie taktyczne. Dalej utrzymujemy tożsamości oraz ważne działania przy rzadszej aktualizacji. Na poziomie regionu korzystamy z harmonogramów, przepływów i zdarzeń.

Ważność zależy również od konfliktu, zobowiązań i wpływu na gracza. Śledzony konwój wymaga dokładniejszej obsługi niż spokojny zakład za rogiem.

Przejście między szczegółowościami musi zachować czas podróży, ilości, własność i historię. Uproszczony konwój rezerwuje ładunek i przemieszcza go po trasie; po zmaterializowaniu w pobliżu gracza nie powstaje druga kopia. Zablokowanie drogi zmienia plan także poza widokiem.

Daleka symulacja nie oznacza wyliczania każdej kolizji. Przeniesienie skutku między skalami wymaga spójności stanu, a przybliżenia muszą być jawnie zdefiniowane i sprawdzane.

### 15.2. AI działająca w regułach gry

Szybkie reakcje i rutynę prowadzą systemy zachowań, oceny priorytetów i planowania. Modele generatywne mogą wspierać interpretację rozmowy, rzadkie przeplanowanie i formułowanie wypowiedzi. Nie są wymagane do każdej decyzji mieszkańca.

Przepływ decyzji: **obserwacja → propozycja celu → plan → sprawdzenie warunków → wykonanie dozwolonych czynności → zapis wyniku.**

Model otrzymuje tylko wiedzę postaci. Zwraca ustrukturyzowaną propozycję, na przykład ofertę zapłaty lub prośbę o eskortę. Silnik sprawdza środki, uprawnienia, trasę, terminy i zgodę drugiej strony. Sama wypowiedź o przelewie nie tworzy pieniędzy.

Rozmowa tekstowa lub głosowa jest opcją. Typowe wybory i czynności muszą być dostępne w interfejsie. Gdy model jest niedostępny lub przekracza budżet czasu, gra korzysta z lokalnego planera i przygotowanych wypowiedzi opartych na stanie. Walka i sterowanie nie oczekują na usługę sieciową.

### 15.3. Zapis, odtwarzanie i narzędzia

Świat wymaga okresowych migawek, dziennika ważnych zdarzeń i kontroli wersji zapisu. Zapis obejmuje własność, rezerwacje, zobowiązania, szkody i istotne wspomnienia.

Dla odtwarzalności zapisujemy zatwierdzone wyniki decyzji modeli oraz autorytatywne skutki istotnych zdarzeń fizycznych. Stałe ziarno losowania nie gwarantuje samo w sobie identycznej fizyki na różnych urządzeniach.

Edytor świata musi pokazywać zależności infrastruktury, źródła wiedzy, plany postaci i historię każdej zmiany. Autor powinien móc zapytać: „Dlaczego ta osoba wyszła z pracy?” i otrzymać sprawdzalną odpowiedź.

## 16. Co jest już znane, a co stanowi program badawczy

Istnieją ważne podstawy. Dwarf Fortress opisuje trwały świat z cywilizacjami, historią i zmianami zachowywanymi między rozgrywkami. Badanie Generative Agents przedstawiło pamięć, refleksję i planowanie w środowisku 25 agentów. Dokumentacja MassGameplay jest przykładem porównawczym reprezentacji i poziomów szczegółowości; Genesis implementuje własne rozwiązania i nie zależy od tej technologii.

Źródła pierwotne:

- Bay 12 Games, opis funkcji Dwarf Fortress: https://bay12games.com/dwarves/features.html
- Park i in., Generative Agents: Interactive Simulacra of Human Behavior: https://arxiv.org/abs/2304.03442
- Epic Games, MassGameplay Overview: https://dev.epicgames.com/documentation/en-us/unreal-engine/overview-of-mass-gameplay-in-unreal-engine

Materiały te potwierdzają istnienie części składowych. Nie dowodzą wykonalności całego Genesis ani nie rozstrzygają, czy podobna gra już powstaje gdzie indziej.

Własny program badawczy dotyczy przede wszystkim: zachowania spójności przy zmianie skali, planowania z rzeczywistymi ograniczeniami, łączenia autorskich historii z nieodwracalnymi zdarzeniami, przenoszenia destrukcji na gospodarkę oraz utrzymania czytelnej i responsywnej gry akcji.

Nowość należy oceniać na podstawie działającego połączenia i mierzalnych rezultatów. Nazwa modułu nie stanowi dowodu nowej technologii.

## 17. Skala docelowa i droga rozwoju

Hipoteza dla docelowej gry: jedno spójne dorzecze, około sześciu miast, osady i tereny między nimi, rząd kilkudziesięciu tysięcy trwałych mieszkańców. Wszystkie te liczby wymagają weryfikacji. Cała cywilizacja oznacza tu kompletny model tego regionu wraz z jawnie określonymi granicami wymiany z otoczeniem.

Najpierw potrzebujemy dowodu zachowania, potem skali i oprawy.

### Etap A — zamknięta próba cywilizacji

Pierwszy przyrost obejmuje około 12–16 mieszkańców, małą przestrzeń, gospodarstwa domowe, magazyn, warsztat, przepompownię i transport. Po pomiarach rozszerzamy próbę do około 60 osób i dodatkowych usług. Grafika jest prosta. Gracz może pomóc, zmienić dostępność przejścia, przejąć ładunek i przekazać informację. Pełny fizyczny rozpad konstrukcji wymaga kolejnego etapu własnego solvera.

Cel: wykazać ciąg przyczynowy, reakcje ludzi, możliwość naprawy oraz zachowanie stanu po zapisie. Osobny scenariusz bez gracza sprawdza, czy społeczność potrafi działać bez stałej ingerencji.

### Etap B — grywalny wycinek „Narew”

Małe miasto i dwie osady, około 400 trwałych mieszkańców, kilka organizacji, jedna przygotowana historia, kilka łańcuchów gospodarczych, podstawowa walka, pojazd, naprawy i destrukcja wybranych konstrukcji.

Gracz może doprowadzić do co najmniej trzech zasadniczo różnych, wyjaśnialnych stanów: współpracy, monopolu albo podziału dostaw. Świat sam reaguje na ingerencje, a zakończenie scenariusza wynika z jego stanu.

### Etap C — próba skali

Rosnąca populacja i liczba połączeń przy zachowaniu tego samego katalogu reguł. Pomiar czasu aktualizacji, kolejek planowania, pamięci, wielkości zapisów i jakości przejść między szczegółowościami. Duża populacja sama w sobie nie potwierdza jakości zachowań.

### Etap D — produkcja pełnej gry

Rozbudowa regionu, kampanii, animacji i biblioteki konstrukcji dopiero po przejściu prób. Rozwój technologii, narzędzi autorskich i treści jest wspólnym programem wielozespołowym. Wiarygodny harmonogram i budżet wymagają wyników wcześniejszych etapów.

## 18. Kryteria, które mogą obalić koncepcję

1. **Trwałość:** zapis i ponowne uruchomienie zachowują osoby, własność, szkody oraz zobowiązania.
2. **Przyczynowość:** ważną zmianę można prześledzić od zdarzenia przez kolejne działania do wyniku.
3. **Ograniczenia:** transakcje nie wydają zarezerwowanych środków drugi raz; agent nie korzysta z nieposiadanej wiedzy ani uprawnienia.
4. **Spójność skali:** przejazd konwoju poza widokiem zachowuje czas, ładunek i ryzyko zgodnie z modelem; zmiana reprezentacji nie dubluje zasobów.
5. **Autonomia:** społeczność ma stabilny zakres działania bez gracza oraz potrafi adaptować się do części zaburzeń. Załamanie ma identyfikowalną przyczynę.
6. **Odporność historii:** śmierć uczestnika, wcześniejsza kradzież przedmiotu i zmiana właściciela miejsca nie pozostawiają zadań wymagających niemożliwego stanu.
7. **Czytelność:** testerzy potrafią wskazać dowody łączące ich działania z kilkoma zauważonymi skutkami; pomiar obejmuje także przypadki, gdy błędnie przypisują sobie zdarzenia niezależne.
8. **Jakość akcji:** symulacja w tle nie blokuje wejścia, kamery i walki. Wydajność sprawdzamy w długich sesjach oraz przy skumulowanych kryzysach.
9. **Wartość rozgrywki:** różne metody działania dają odmienne doświadczenia i sensowne koszty. Gracze chcą poznać dalszy los ludzi również po wykonaniu operacji.

Parametry populacji i oprawy dopasowujemy do wyników. W pierwszym grywalnym wycinku trzeba wyznaczyć sprzęt odniesienia oraz mierzyć pełny rozkład czasów klatek, opóźnienia reakcji i koszty symulacji; samo średnie FPS nie wystarczy.

## 19. Największe ryzyka projektowe

**Kaskady niszczące świat za szybko.** Odpowiedź: rezerwy, alternatywy, czas reakcji i rzeczywiste działania naprawcze mieszkańców.

**Gracz nie widzi, że cokolwiek się zmieniło.** Odpowiedź: lokalne ślady, powracające osoby, źródła informacji i konkretne zobowiązania.

**Powtarzalność zleceń mimo głębokiej symulacji.** Odpowiedź: autorskie konflikty, różne drogi dostępu i znaczące osobiste stawki; liczba wygenerowanych zadań nie jest miarą jakości.

**Stała presja terminów.** Odpowiedź: czytelnie komunikowane pilne sytuacje, przestrzeń na spokojne życie i możliwość delegowania.

**Mikrozarządzanie.** Odpowiedź: zakresy odpowiedzialności, budżety i współpracownicy zamiast ręcznego zatwierdzania każdego kursu dostawcy.

**Niespójna mowa generatywna.** Odpowiedź: oddzielenie zatwierdzonego działania od jego opisu, ograniczenie dostępnej wiedzy i przygotowany sposób zastępczy.

**Koszt produkcji przewyższający wartość symulacji.** Odpowiedź: zamknięty katalog dóbr, materiałów i działań w pierwszych etapach oraz bramki jakości przed powiększaniem świata.

## 20. Scena, która sprzedaje obietnicę

Wracasz po dłuższej wyprawie do dzielnicy, w której wcześniej pomogłeś przywrócić wodę. Na rogu działa warsztat mechaniczki uratowanej z transportu. Zatrudniła kierowcę, którego przekonałeś do współpracy. Przy stacji stoją nowe zbiorniki, kupione ze składek mieszkańców po dawnym kryzysie.

Ktoś rozpoznaje cię i zaprasza na otwarcie. Inna osoba nie chce rozmawiać, ponieważ w czasie tej samej operacji straciła pracę. Wieczorem zjawia się przedstawiciel sąsiedniego miasta: chce zamówić podobną instalację, zanim nadejdą deszcze.

Każdy z tych elementów ma podstawę w stanie świata. Ludzie korzystali z konkretnych środków, odbyli konkretne rozmowy i wykonali pracę. Twoja dawna interwencja zmieniła ich możliwości. Teraz ich własne decyzje tworzą następną przygodę.

**Genesis ma sprawiać, że gracz chce wrócić do miejsca, ponieważ ciekawi go, co stało się z ludźmi, których tam zostawił.**
