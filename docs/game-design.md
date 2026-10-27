# Projekt gry i pierwszego doświadczenia

**ZATWIERDZONA WIZJA:** przyziemne, realistyczne i brutalne fantasy z autonomiczną cywilizacją, zgodnie z DEC-008 i [kartą projektu](project-charter.md). **PROPOZYCJE poniżej:** konkretne role, sytuacje i mechaniki do prototypowania. Nie są zaimplementowane tylko dlatego, że zostały opisane. Granice wykonania ustala DEC-011, stan kodu wskazuje [środowisko pracy](development.md). Szczegółowe liczebności i limity należą do [budżetów](performance-budget.md).

## Obietnica

Gracz jest osobą wewnątrz świata. Jego działanie zmienia możliwości ludzi, a ludzie podejmują dalsze decyzje. Gracz może wrócić do miejsca i rozpoznać skutki poprzedniej interwencji.

Mieszkańcy mają własne przedsięwzięcia i potrafią tworzyć dobrobyt bez czekania na bohatera. Celem jest przygoda z ruchem, ryzykiem, odkrywaniem i osobistymi relacjami. Ambicja obejmuje królestwa, kulturę, konflikty i historię rozciągającą się poza bohatera. Rozmiar pierwszej sceny nie definiuje granic docelowego świata.

Hipoteza wyróżniającej mechaniki: **trwałe przekazywanie praktyk**. Gracz lub mieszkaniec znajduje działającą metodę, druga osoba poznaje ją przez obserwację albo naukę, a później samodzielnie stosuje w odpowiednich warunkach. Przykład: przewoźnik uczy się dostępnego objazdu i organizuje dalsze dostawy bez prowadzenia przez gracza. Dowodem jest wykonana czynność z realnym zasobem, źródłem wiedzy i kosztem oraz zachowanie po zapisie. Sam dialog nie wystarcza. Nie twierdzimy, że sama idea stanowi dowiedzioną nowość gatunkową.

Pętla: obserwacja sytuacji → poznanie ludzi i zależności → wybór metody → działanie → odczyt skutku → nowe zobowiązanie lub możliwość.

Propozycja roli w pierwszej scenie: podróżny wykonujący drobne zlecenie przewozu. Może pomóc, negocjować, podjąć ryzyko, działać siłą albo odejść. Nie otrzymuje uprawnień zarządcy osady ani pełnej prawdy inspektora. Imię, pochodzenie, profesja i docelowa kamera pozostają otwarte. Każdy odcinek pętli ma czasownik gracza: obejrzyj, podejdź, przenieś, przekaż, uzgodnij, pracuj, wycofaj. Rozmowa może ustalić zamiar; dopiero czynność zmienia zasób lub stan świata.

## Wiarygodność fantastyki

| Obszar | Zasada projektu | Przykład do późniejszego sprawdzenia |
| --- | --- | --- |
| Gospodarka i polityka | Zasoby, praca i instytucje mają właścicieli oraz interesy | Odbudowane przejście zmienia dostęp do targu i możliwość pobierania opłat |
| Magia | Zdolność ma warunki, koszt, ograniczenia i skutki uboczne określone w modelu | Zaklęcie nie naprawia mostu poza systemem konstrukcji, zużycia i wiedzy |
| Potwory i ekosystemy | Istota ma miejsce w świecie, potrzeby i możliwości przetrwania | Dostępność pożywienia może zmienić trasę drapieżnika; nie powstaje z powodu poziomu gracza |
| Kultura i historia | Praktyki i oceny rozchodzą się konkretnymi kanałami | Wieś nie zna sprawcy kradzieży bez obserwacji, dowodu lub przekazu |
| Niezwykłe moce | Różne zdolności korzystają ze wspólnych zasad skutku i trwałości | NPC z tą samą zdolnością podlega tym samym ograniczeniom co gracz |

Nie ustalamy jeszcze źródła magii, zasobu energii, szkół, listy potworów ani drzewa rozwoju. To wymagania przyszłego projektu, nie istniejące subsystemy. Spójność uniwersum i skala niezwykłych wydarzeń mogą inspirować; MCU nie jest gotowym zestawem zasad Genesis.

## Scena prototypu

Dwie osady, roboczo przy trakcie i przy młynie, łączy przeprawa mostowa. Warsztat kowalsko-ciesielski oraz mechaniczne urządzenie podnoszenia wody lub obsługi śluzy potrzebują części, pracy i transportu. To propozycje konkretnej treści, nie zatwierdzony poziom technologiczny całego świata. Urządzenie ma zwykłą funkcję gospodarczą; scena nie potrzebuje magii. Gospodarstwa mają zapasy, obowiązki i potrzeby. Mieszkańcy potrafią pracować i wykonywać ograniczone plany również bez gracza.

| Dawny przykład | Kontekst sceny fantasy | Wymaganie zachowane |
| --- | --- | --- |
| Dwa skupiska zabudowy | Dwie zależne od siebie osady | Trwałe osoby, potrzeby, zasoby i praca |
| Warsztat i przepompownia | Warsztat oraz mechaniczne urządzenie gospodarki wodnej | Koszt/recepta, wykonawca, części i przerwanie |
| Zamknięty most | Niedostępna przeprawa i znany lub odkrywany objazd | Wspólna geometria, kolizja, wersja trasy i przeplanowanie |
| Transport części | Dostawa materiału i zapasów między osadami | Ładunek w konkretnym miejscu, czas podróży i skuteczna dostawa |
| Reakcja instytucji | Lokalna władza, cech lub wspólnota — szczegóły otwarte | Osobno fizyczny skutek, legalność i wiedza |

Utrata wykonawcy lub materiału przerywa pracę według wspólnych reguł. Trudność wynika ze stanu świata; nie dokładamy automatycznej katastrofy, gdy gracz pomaga. Nie zmieniamy nazw domenowych API na nazwy fikcyjnych cechów lub zaklęć. Zmiana kontekstu nie potwierdza ani magii, ani fizycznego zawalenia mostu; dotychczasowe wymagania i scenariusze testowe pozostają.

Pierwszy katalog czynności: poruszanie się, obserwacja, przenoszenie partii zasobu, przekazanie informacji, uzgodnienie dostawy, rozpoczęcie pracy i zmiana dostępności przejścia. Każda czynność musi mieć rzeczywisty koszt lub skutek oraz obsługę przerwania.

Metody rozwiązania: pomoc w dostawie, przygotowanie objazdu lub naprawy, przejęcie zasobu, wycofanie się i obserwacja samodzielnej reakcji. Zasób nie pojawia się tylko dlatego, że ktoś o nim opowiedział.

Do grywalnej próby wybieramy co najmniej dwie rzeczywiście działające metody, odpowiednio do zaimplementowanych systemów. Poniższy katalog jest propozycją; nie obiecujemy kompletu przed M06.

| Metoda | Osobiste działanie | Cena i ograniczenie | Skutek po odejściu |
| --- | --- | --- | --- |
| Dostawa objazdem | Poznać trasę i doprowadzić realny ładunek | Droga, ryzyko, ograniczony udźwig | Zakład dostaje materiał; przewoźnik może poznać trasę |
| Pomoc przy naprawie | Dostarczyć części i wykonać pracę | Zużycie oraz czas, konkurencja o zasób | Przeprawa jest dostępna również innym |
| Uzgodnienie współpracy | Przekazać informację i doprowadzić do przyjętej umowy | Rozmówca, wiedza, wykonalne zobowiązanie | Inna osoba realizuje własny plan; pozostaje dług lub relacja |
| Przejęcie zasobu | Fizycznie zdobyć i przenieść cudzy materiał | Konflikt, utrata dostępu, ewentualni świadkowie | Pierwotny właściciel ma niedobór; reakcja zależy od informacji |
| Brak interwencji | Odejść i wrócić | Utrata osobistej okazji, możliwa zmiana sytuacji | Mieszkańcy adaptują się lub ponoszą określoną porażkę |

Zapasy i recepty muszą tworzyć rzeczywisty wybór. Naprawa, dostawa i wyposażenie nie korzystają z niezależnych kopii tej samej części. Niedobór ma źródło; nie rozwiązujemy go materialną nagrodą dopisaną po komunikacie o sukcesie.

## Informacja dla gracza

Proste bryły i kolory muszą pozwalać rozpoznać osobę, przeszkodę, ładunek i stanowisko. Widok obserwacyjny pokazuje znane fakty. Oddzielny tryb deweloperski może ujawniać pełny plan i przyczyny; jego wiedza nie może przeciekać do NPC.

Gracz potrzebuje sygnału, że przejście jest zamknięte, transport czeka i dlaczego praca nie rusza. Nie otrzymuje automatycznie wiedzy o każdej odległej zmianie. Test czytelności nie jest zaliczony samym zrzutem ekranu.

## Autonomia i rytm

Społeczność ma stabilny zakres codziennego działania, rezerwy i możliwość częściowej adaptacji. Potrafi pomagać oraz tworzyć dobrobyt, a nie tylko generować kryzysy. Niepowodzenie ma możliwą do prześledzenia przyczynę.

Pilność wynika z sytuacji. Świat nie powinien wymagać ciągłego ratowania wszystkich przez gracza. Warunki ostrzegania o utracie ludzi lub miejsc pozostają pytaniem projektowym.

Scena ma również spokojny wariant: przy sprawnej przeprawie i zasobach mieszkańcy wykonują pracę, współpracują i wykorzystują okazję, np. powtarzalną dostawę, bez katastrofy i rozkazów gracza. Sprawdzamy w ten sposób, czy autonomia tworzy możliwości, zamiast jedynie produkować awarie.

Propozycja pierwszej sesji 15–20 minut, nie wynik pomiaru: ruch i rozpoznanie miejsca → spotkanie oraz widoczna zależność → samodzielny wybór i wykonanie metody → powrót, rozpoznanie skutku, zapis/wznowienie. Nie wymuszamy kolejności skryptem, teleportacją ani zatrzymaniem autonomii.

## Walka i trwałość

Docelowa gra ma elementy akcji. Prototyp najpierw sprawdza ruch, interakcję i konsekwencje; rozbudowana walka nie jest wymaganiem pierwszej sceny. Późniejszy rytm walki musi uwzględniać śmierć trwałych mieszkańców, leczenie, poddanie i zastępowanie funkcji. Brak rozwiązania tego problemu jest [ryzykiem produktu](risks-and-open-questions.md).

Projekt Action RPG rozwijamy równolegle, przy rzeczywistych zależnościach implementacji:

- **Ruch i kamera:** czytelny kierunek, zatrzymanie, mijanie przeszkód i dojście do interakcji. Kamera nie ukrywa celu ani nie przenika przez istotne przeszkody. Najpierw odbiór obsługiwanej geometrii M03; typ kamery gry wybieramy po próbie.
- **Eksploracja:** ślady drogi, stan mostu, odgłosy pracy i informacje ludzi wskazują możliwości. Odkryty objazd zmienia dostępne działania, bez wszechwiedzącej mapy.
- **Mały test konfliktu:** propozycja przeciwnika z celem, np. zatrzymaniem ładunku. Pierwsza próba walki potrzebuje czytelnego przygotowania działania, kontaktu, kosztu, reakcji na trafienie i możliwości wycofania lub poddania. Jeden przeciwnik i kilka sprawdzonych działań poprzedzają pełny arsenał. Nie blokuje to pierwszego prototypu cywilizacji.
- **Progresja:** najpierw nowe możliwości — narzędzie, umiejętność, dostęp, trasa lub współpraca. Nagroda ma źródło w świecie i pozwala coś zrobić. Konkretne poziomy, profesje i ekwipunek pozostają otwarte.
- **Trwałość:** brutalność oznacza czytelne ryzyko dla ciała, zapasów i relacji. Nie przesądza obowiązkowego permadeath gracza. Śmierć NPC nie jest naprawiana respawnem tej samej biografii.

Potęga fantastycznych zdolności nie zwalnia z projektu sterowania, komunikacji ryzyka i reguł kontaktu. Wynik walki nie powinien zależeć od przewagi widoczności uzyskanej przez zmianę ustawień grafiki.

## Wczesna próba bezpośredniej rozgrywki

Wdrożony tor M03 jest małą próbą ruchu i kontaktu, bez fikcyjnej gospodarki oraz automatycznej oceny atrakcyjności. WASD względem kamery, PPM jako cel, blokada ruchem drugiej osoby i `F` w zasięgu 1,5 m pozwalają sprawdzić dojście do człowieka. Brak drogi i konieczność objazdu mają rzeczywisty skutek przestrzenny. Systemowa magia, potwory, walka i nazwy kultur nadal nie są częścią tego toru.

25.10.2026 użytkownik zaakceptował sterowanie, komfort i czytelność obecnego prototypu, zgłaszając potrzebę bardziej płynnego i logicznego prowadzenia do celu oraz dopracowania szybkości i naturalności ruchu. [Zapis odbioru](verification/m03-motion-navigation.md#ocena-użytkownika) oddziela jego wypowiedź od interpretacji i testów technicznych. Nie zatwierdza to docelowej kamery ani prędkości postaci.

Zatwierdzony przez użytkownika [przyrost prowadzenia do celu](work-items/navigation-polish.md) wdrożył bezpośrednie odcinki i usuwanie zbędnych punktów wyłącznie po sprawdzeniu sweepem całego ciała. Nowy cel startuje od bieżącego położenia, identyczny zachowuje postęp, a komunikaty odróżniają dojście od blokady. [Raport przyrostu](verification/navigation-polish.md) zachowuje trzy konkretne reprodukcje przed naprawą, porównanie tras oraz aktualny odbiór. Surowe A* i jego test BFS pozostają odrębne od skracania.

Do dalszej oceny pozostaje tempo na rzeczywistych odległościach i zadaniach. Prędkość nadal wynosi 3 m/s. Rozpędzanie, hamowanie i zwalnianie przed zakrętem są propozycjami; nie dobieramy arbitralnie nowej prędkości na podstawie ogólnego odczucia. Ewentualna prędkość i faza ruchu jako stan świata muszą przeżyć zapis, odczyt i podział kroków. Wcześniejsza akceptacja M03 nie oznacza wykonanej próby nowego odczucia po skróceniu tras.

Po kolejnym zgłoszeniu użytkownika [audyt praktyczny](verification/motion-adversarial-audit.md) odtworzył utknięcie na kontakcie z poprawną pozycją i brak obejścia stojącej osoby. Naprawiono te przypadki oraz dodano interpolację rzeczywistej drogi pomiędzy krokami 50 ms. To konkretna poprawa prowadzenia i prezentacji, bez twierdzenia o ukończeniu naturalnego chodu, animacji lub pełnej inteligencji. Nie rozszerzamy geometrii i platform wyłącznie po to, by zamaskować błędy aktualnego kontraktu. Nowa próba użytkownika powinna sprawdzić bliskie kliknięcia przy ścianie, kolejne cele podczas marszu, mijanie i odczucie obrazu po tej zmianie.

Do kolejnej próby z człowiekiem zachować scenariusz: dojść do drugiej postaci, użyć kontaktu, znaleźć objazd po zamknięciu przejścia i zapisać/wznowić podróż. Porównać bieżącą oraz poprawioną wersję, odnotować pomyłki kierunku kamery, zasłonięcie celu, zbędne zakręty, cofanie i zrozumienie oczekiwania. Wypowiedź użytkownika nie potwierdza wykonania każdego zadania z tego protokołu. Swoboda metod i satysfakcja z konsekwencji wymagają późniejszych realnych systemów M04–M06; sam kontakt laboratoryjny nie jest gotowym dialogiem lub questem.

## Fikcja i dłuższa wizja

Wolne Dorzecze, miasto Narew, alternatywna bliska przyszłość i technologiczna sieć GENESIS są historycznymi propozycjami; nie stanowią obowiązującej fikcji. Zachowujemy ich wartościowe mechaniki dostaw, infrastruktury i konsekwencji. Zatwierdzony kierunek fantasy nie przesądza nazw i mapy świata, kultur, ustrojów, poziomu technologii, reguł magii, gatunków istot, bohatera, kamery, rytmu czasu oraz szczegółów walki, śmierci i rozwoju.

Dalsza wizja obejmuje kulturę zmieniającą zachowanie, wynalazki z działających elementów, instytucje trwające po odejściu założycieli oraz archeologię własnych konsekwencji. Zagadnienia te mają osobny [program badań](research-agenda.md).

## Ocena gry

Obserwujemy, czy gracz rozumie sytuację, znajduje więcej niż jedną metodę, rozpoznaje mieszkańców i potrafi wskazać dowody skutków. Sprawdzamy też błędne przypisywanie sobie zdarzeń niezależnych. Liczba wygenerowanych dialogów i zleceń nie jest miarą jakości.

Przed próbą zapisujemy wersję sceny, instrukcję, ustawienia, obserwacje i niepowodzenia. Proponowana pierwsza próba z pięcioma osobami bada kierunek, nie dowodzi uniwersalnej jakości:

| Hipoteza | Obserwacja potrzebna | Sygnał do poprawy |
| --- | --- | --- |
| Czytelna interakcja | Ruch i interakcja bez ciągłego przejmowania sterowania przez prowadzącego | Powtarzalna walka z kamerą lub wyborem obiektu |
| Swoboda metody | Co najmniej trzy osoby samodzielnie wskazują dwie metody, obie z dowodem wykonania w prototypie | Alternatywa istnieje tylko w opisie |
| Zrozumiała przyczynowość | Rozpoznany skutek ingerencji i zdarzenie niezależne | Wszystkie zmiany przypisywane bohaterowi albo brak widocznych skutków |
| Wartościowa autonomia | Odejście i powrót pokazują kontynuację oraz możliwość dalszej przygody | Bez gracza nic się nie dzieje lub świat stale się rozpada |
| Powód do powrotu | Dobrowolna dalsza eksploracja i konkretne uzasadnienie zainteresowania | Podziw technologii bez zainteresowania własnym działaniem |

Po M03 poprawiamy ruch i kamerę przed pogłębianiem geometrii. W M04 sprawdzamy ingerencję i spokojną autonomię. W M05 oceniamy sytuację i historię po zmianie świata. M06 łączy próbę gry z poprawnością, trwałością i pomiarami. Nie tworzymy drugiego planu ani nie przyznajemy 10/10 za sam opis.
