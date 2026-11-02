# Jeden plan pierwszego prototypu

**Status: pierwszy prototyp techniczny M00–M06 ukończony i odebrany w opisanym zakresie, 02.11.2026.** Dalszy kierunek nie został wybrany; ustala to DEC-019 w [rejestrze decyzji](decisions.md). Użytkownik zaakceptował obecne sterowanie M03 z uwagami rozwojowymi dotyczącymi reakcji na cel, płynności trasy, szybkości i naturalności ruchu. Nie ma harmonogramu tygodniowego. Zakres produktu wynika z [karty](project-charter.md), a parametry z [budżetów](performance-budget.md).

**Zrealizowana kolejność:** integracja ukończonych eksperymentów w okienkowym prototypie → M05 → M06. [Integracja w oknie](verification/practice-in-prototype.md) została technicznie zweryfikowana i zaakceptowana 28.10.2026; jej [projekt](work-items/practice-in-prototype.md) nie jest drugim planem etapów. Dalszy projekt nie został wybrany. Wcześniejszą interpretację o obowiązkowym przejściu do pełnego 3D wycofano w DEC-019.

Etapy są uporządkowane zależnościami. Nie tworzymy drugiego równoległego backlogu E/P ani nie traktujemy większej populacji jako automatycznego następnego zadania. Dopuszczalne są niezależne małe próby, ale zaliczenie etapu wymaga pełnego wskazanego dowodu.

DEC-008 i DEC-011 zmieniają kontekst produktu na realistyczną cywilizację fantasy, zachowując kolejność, zależności i wymagania tego planu. Scena dwóch osad i mostu korzysta z [nowego kontekstu](game-design.md#scena-prototypu). Magia, potwory i rozbudowana walka nie są dodatkowymi bramkami pierwszego prototypu. Projekt Action RPG rozwijamy równolegle, a jego próby włączamy do poniższych etapów zamiast tworzyć drugi plan.

### M00 — Budowanie i testy rdzenia

Wymaga: obecnej dokumentacji i narzędzi deweloperskich. Rezultat: minimalny projekt C++, rzeczywisty build i runner testowy, jawne ostrzeżenia, konfiguracja diagnostyczna oraz instrukcja uruchomienia jednego testu. Weryfikacja obejmuje poprawny build, SC-15 dla początkowej kontroli pamięci oraz wykazanie, że celowo niezaliczony test daje niezerowy kod wyjścia. Aktualizuje stan w [środowisku pracy](development.md).

**WYNIK:** build Debug, RelWithDebInfo i AddressSanitizer działają; testy wykazują również błąd asercji, odmowę pustego zestawu oraz faktyczne wykrycie błędu pamięci. Wdrożony moduł to zegar symulacji i jego konsument konsolowy. Nie powstały puste moduły świata. [Raport M00](verification/m00-build-and-diagnostics.md) oraz [komendy](development.md) wskazują dokładny zakres.

### M01 — Tożsamość, czynności i kontynuacja

Wymaga M00. Wspólny rdzeń ma trwałe ID, komendy, rezerwacje, przerwania, czas i minimalny zapis. Powstaje inspektor tekstowy przyczyn i stanu. Dowód: SC-01, SC-02, SC-03, SC-04 i SC-08 w obsługiwanym zakresie. Odmowy i awarie są częścią testów.

**WYNIK:** wdrożono model zużywania jednej partii materiału, trwałe ID, uchwyty z generacją, rezerwacje, częściowe zużycie, przerwanie, usunięcie uczestnika, terminy i ponowienia najwyżej raz. Zapis zachowuje pracę oraz historię komend; demo wznawia ją w nowym procesie. [Raport M01](verification/m01-identity-actions-reservations.md) wskazuje dowody i granice scenariuszy. [Konkretny model](work-domain.md) nie obejmuje jeszcze produkcji, narzędzi, podróży ani autonomii.

### M02 — Minimalne okno i obraz

Wymaga M00; może rozwijać się niezależnie od części M01. Własny backend rysuje proceduralną geometrię, reaguje na wejście i resize oraz raportuje adapter. Dowód: odpowiednia część SC-11. Brak obrazu albo brak testu na GPU nie może być ukryty jako headless sukces.

To mała warstwa prezentacji. PBR, pełny importer i edytor 3D nie są wymaganiami.

**WYNIK — ZALICZONE:** aplikacja Win32/D3D11 działa na MX330, pokazuje stan M01, obsługuje komendy, kamerę, wybór, zapis i odczyt. Zainstalowano systemowe Narzędzia grafiki; warstwa diagnostyczna D3D11 przeszła bez zgłoszonych ostrzeżeń lub błędów. Pełny Debug 45/45, RelWithDebInfo 45/45 i ASan 47/47 zaliczono bez filtrów oraz pominięć. [Raport M02](verification/m02-window-laboratory.md) wskazuje dowody i granice tego odbioru, który nie obejmuje jeszcze ruchu ani autonomii.

### M03 — Własny ruch i nawigacja

Wymaga M01 i M02 do pełnego odbioru aplikacji. Własne kolizje ograniczonych kształtów, kontroler ruchu oraz mapa przejść z A* współdzielą geometrię. Dowód: SC-06, SC-07 i ruch z SC-11. Początkowy zakres geometrii jest zapisany, a nieobsługiwany przypadek odrzucany jawnie.

Nie nazywać tego pełnym solverem brył ani pełnym navmeshem. Są to późniejsze rozszerzenia z własnym odbiorem.

Warstwa gry: wczesna próba osobistego ruchu, kamery i dojścia do interakcji z człowiekiem. Powtarzalne problemy sterowania poprawiamy przed rozbudową geometrii. Wynik techniczny i ocena wygody są raportowane osobno.

**WYNIK — ZALICZONE Z UWAGAMI ROZWOJOWYMI:** `reality_lab --motion` łączy kontroler 2,5D i A* z tą samą geometrią, rozmiarem postaci, wersją przejść i stałym krokiem. Ruch ma kolizję, objazd, brak drogi, oczekiwanie na drugą postać i kontynuację po zapisie. PPM, WASD i lokalny kontakt `F` są wczesną próbą gry; M01 pozostaje diagnostyką bez wymagań dojścia do pracy. [Raport M03](verification/m03-motion-navigation.md#ocena-użytkownika) zapisuje rzeczywistą ocenę użytkownika: obecne sterowanie, komfort i czytelność są akceptowalne; prowadzenie do celu, szybkość i naturalność pozostają do dopracowania. Podstawą tej części odbioru jest wypowiedź użytkownika, odrębna od testów automatycznych i Computer Use. Dalsze poprawki ruchu mają własny brief i odbiór w ramach tego samego planu; nie rozpoczynają automatycznie M04.

### M04 — Mała społeczność w grze i laboratorium

**Stan bieżący: ZALICZONE w opisanym zakresie, z akceptacją użytkownika 27.10.2026.** [Audyt aktualnego M04](verification/m04-acceptance.md) wiąże kryteria z istniejącym kodem, wspólną sceną ośmiu osób, spokojną autonomią, zaburzeniem gracza, porównaniem z prostą rutyną i kontynuacją. Stan sprawdzeń technicznych podaje raport. [Próba użytkownika](verification/m04-playtest.md) zawiera bezpośrednią akceptację etapu; nie została zastąpiona wynikiem automatu.

Wymaga M01 i M03. Wspólne reguły potrzeb, pracy, zasobów, transportu i wiedzy działają w aplikacji i runnerze. Gracz ingeruje w ten sam świat. Dowód: SC-04, SC-05, SC-08, SC-10 i SC-14 w ograniczonym zakresie opisanym w raporcie. Propozycje większej populacji w budżetach pozostają propozycjami.

Wdrożone fundamenty mają wcześniejsze raporty: [potrzeby](verification/autonomy-need-fallback.md), [pojemność](verification/reusable-capacity.md), [dostawa i zwrot](verification/material-delivery.md), [lokalne ustąpienie](verification/local-clearance.md), [wiedza o stanowiskach](verification/local-station-knowledge.md), [przeprawa i logistyka](verification/passage-logistics.md) oraz [świadek i prawo](verification/witness-property-law.md). Dawne opisy braku tych systemów były stanem historycznym i nie są aktualnym zadaniem. Raporty historyczne zachowują własne wyniki i ograniczenia.

Kierunek wejścia w M04 z 26.10.2026 obejmował istniejący ruch, prawdziwą dostawę, minimalną współpracę i ingerencję gracza. Bieżący odbiór łączy te elementy; nie rozszerza zakresu na pełne instytucje, nowe solverowe bryły, nieskończoną gospodarkę lub całe M06. M03 zachowuje wcześniejszą ocenę użytkownika i jej uwagi do reakcji na cel, płynności oraz naturalności; automat nie potwierdza nowego komfortu.

Warstwa gry: gracz osobiście wpływa na dostawę albo pracę i potrafi odczytać skutek. Sprawdzamy również spokojny wariant bez katastrofy: mieszkańcy pracują i współpracują bez rozkazów. Nie uznajemy samej liczby NPC za wartość gry. Odbiór M04 został potwierdzony przez użytkownika; dłuższy pomiar wykonano w technicznym M06. Szersze badanie graczy nie było jego bramką i pozostaje zagadnieniem przyszłego produktu.

### Przyrost po M04 — wykorzystanie badań w istniejącym prototypie

**Status: technicznie wdrożone i zweryfikowane; obecne uczenie w prototypie zaakceptowane przez użytkownika 28.10.2026.** Treść i zakres odbioru zapisano w [raporcie](verification/practice-in-prototype.md#ocena-użytkownika). Wymaga odebranego M04 i ukończonego [badania odkrywania](verification/discriminating-experiment.md). Uzasadnienie kolejności: zdolność należy najpierw zastosować w żywym świecie i uczynić czytelną dla gracza, a następnie objąć całą trwałością i historią M05. Dotychczasowych wyników I/A/T nie zmieniamy pod potrzeby pokazu.

Rezultat dla użytkownika: w tej samej małej społeczności mieszkaniec bada nieznane ustawienie urządzenia, poznaje metodę potrzebną do konserwacji i rzeczywiście pracuje. Druga osoba zdobywa pomiary przy spotkaniu i później samodzielnie korzysta z wiedzy. Gracz obserwuje próby, dostarcza części lub sam wykonuje dopuszczalne czynności, a po powrocie widzi skutek. Szczegółowy zakres, kontrakty i lista odbioru należą do [projektu przyrostu](work-items/practice-in-prototype.md).

Dowód zamknięcia: wspólna aplikacja i runner mają jeden świat oraz te same czynności; energia i części są rozliczane; potrzeby nie niszczą rozpoczętych operacji; poprawny zapis obejmuje wiedzę i pracę; rzeczywisty obraz/wejście oraz próba użytkownika potwierdzają, że da się zrozumieć sytuację. Sam headless, osobne okno dwóch agentów ani nowy opis nie zamykają tego przyrostu.

### M05 — Ciągłość, przejście i historia

**Status: ZALICZONE 29.10.2026 w opisanym zakresie.** [Zbiorczy raport](verification/m05-acceptance.md) obejmuje audyt SC-08, techniczne dowody SC-09/SC-10/SC-13 i deklarację akceptacji użytkownika bez osobnego playtestu. Wymaga M04 i powyższej integracji praktyk. Zamknięcie i naprawa przejścia wpływają na trasy i pracę; migracja reprezentacji zachowuje stan. Jedna scena odnosi się do wiedzy i rzeczywistego wyniku. Dowód: SC-08, SC-09, SC-10 i SC-13 oraz odpowiednia próba aplikacji. Część przejść i kontynuacji istnieje już w M04; przed implementacją należy przypisać istniejące dowody i wskazać rzeczywiste braki zamiast budować je drugi raz.

Technicznie ukończono i zweryfikowano [sytuację z dwiema metodami dostawy](work-items/m05-two-methods.md): naprawę dzięki poznanej metodzie albo istniejący objazd. Użytkownik zaakceptował ten zakres bez osobnej próby — zgodnie ze zbiorczym raportem. [Raport przyrostu](verification/m05-two-methods.md) oddziela wyniki od pozostałych wymagań M05, w tym migracji reprezentacji i historii reagującej na zmiany świata.

Zaimplementowano [jeden przewóz lokalny/daleki](work-items/m05-distant-delivery.md), obejmujący rzeczywistą zmianę reprezentacji SC-09 w ograniczonym modelu statycznych terminali. Po [naprawie kosztu uczenia](verification/learning-cost.md) pełne Debug/Release/ASan przeszły; ograniczony techniczny zakres SC-09 jest zweryfikowany. [Raport kuriera](verification/m05-distant-delivery.md) zachowuje także wcześniejsze porażki. Akceptację użytkownika i domknięcie pozostałych wymagań opisuje zbiorczy raport M05.

Zaimplementowano i technicznie zweryfikowano zatwierdzony [zakres historii SC-13](work-items/m05-story-design.md): jeden fizyczny kurs i posiłek oraz rzeczywiste usunięcie rozmówcy/miejsca lub przeniesienie zapasu przed startem. [Raport](verification/m05-story.md) obejmuje pełne profile, nowe procesy, aplikację i granice. To ograniczone przygotowanie jednej autorskiej sceny; akceptację użytkownika oraz całe zaliczenie M05 opisuje zbiorczy raport.

| Zakres M05 | Co ma zadziałać | Warunek zamknięcia |
| --- | --- | --- |
| Ciągłość wspólnego świata | Zapis w trakcie uczenia, ruchu, ładunku, oczekiwania i pracy; powrót gracza bez resetu pamięci | SC-08, stare formaty, uszkodzony odczyt, brak podwójnego zużycia i zgodna kontynuacja w nowym procesie |
| Rzeczywiste przejście szczegółowości | Jeden przewóz wychodzi z aktywnego obszaru, trwa według jawnego modelu podróży i wraca | SC-09: te same ID, ładunek, rezerwacje i postęp/czas; jeden właściciel ruchu; wielokrotne przełączenie bez duplikacji |
| Jedna sytuacja z dwiema działającymi metodami | Proponowane: przywrócić przejście dzięki poznanej metodzie albo zrealizować dostawę istniejącym objazdem | SC-10: oba warianty rzeczywiście dostarczają zasób, mają koszt/czas i pozostawiają różny stan świata |
| Historia reaguje na fakty | Nieobecny wykonawca, niedostępne miejsce albo przeniesiony zasób zmienia przebieg, odmowę lub zakończenie | SC-13: scena nie odtwarza potajemnie uczestnika/zasobu i nie wymaga niewykonalnej rozmowy |

Wyjście poza kamerę lub zapis/wczytanie nie jest dowodem migracji reprezentacji. Pierwszy daleki model może dotyczyć jednego kursu, bez ogólnego streamingu świata, ale musi rzeczywiście przejąć i oddać odpowiedzialność za ten sam przewóz. Przekazanie w chronionej fazie jest odroczone albo odmówione. Porównujemy zadeklarowane chronione własności modeli; nie udajemy identycznej fizyki obu reprezentacji. Szczegół wymaga własnego briefu i testów przed kodem.

Zmiana dostępności mostu nie potwierdza fizycznego rozpadu. Rozszerzenie o dynamikę i zerwanie elementów musi osobno przejść scenariusze fizyki i spójności konstrukcji.

Warstwa gry: sytuacja udostępnia co najmniej dwie zaimplementowane metody, z kosztami oraz trwałymi skutkami. Mały test bezpośredniego konfliktu Action RPG można dołączyć po jego własnym briefie i odbiorze; nie blokuje wymaganej autonomii i nie staje się rozbudowaną produkcją walki.

### M06 — Odbiór i decyzja o skali

**Status: WYKONANE W OPISANYM ZAKRESIE, 02.11.2026.** [Odbiór techniczny](verification/m06-acceptance.md) obejmuje SC-11/SC-12, pełne Debug/Release 435/435 i ASan 437/437, zamrożone 120 s rozgrzewki +1800 s pomiaru na MX330 oraz kontynuację końcowego świata w osobnych procesach. Użytkownik odebrał fundament techniczny; nie wymaga próby grywalności ani uczestników. Negatywne pilotaże i ograniczenia pozostają w raporcie.

[Przygotowanie odbioru](work-items/m06-acceptance.md) określa najbliższy zakres instrumentacji i rozdzielenie prób. [Audyt gotowości z 01.11.2026](verification/m06-preflight.md) zawiera kontrolę istniejących źródeł oraz wybrane 17/17 testów Release; nie jest długim pomiarem, próbą natywną ani odbiorem całego M06.

[Pierwszy przyrost eksportu](verification/m06-measurement-export.md) dodaje opcjonalny szereg czasów i pamięci obu faz. Sam nie zamykał pomiarów CPU/GPU i dłuższej próby; ich końcowe dowody zawiera raport odbioru technicznego.

Zakres wykonano w oddzielnych próbach: zamrożona scena i kryteria; kontrolowany pomiar aplikacji 120 s rozgrzewki +1800 s rejestracji na i5/MX330/8 GB; techniczne próby ingerencji i obu metod; kontynuacja końcowego świata w osobnych procesach; naprawy wykrytych wad i pełne regresje. Krótkie ujemne pilotaże pozostają w raporcie. Protokół i bramki zamrożono przed długą próbą, bez zmiany dotychczasowych limitów testów. Osiągnięty viewport 844×781 nie potwierdza propozycji 1280×720 ani większej skali.

Mierzymy rozkład klatek i przycięcia, CPU/GPU, pamięć i jej trend, zaległość aktualizacji, koszt wnioskowania/nawigacji i zapisu oraz postęp pracy mieszkańców. Do oceny czasu nie używamy ASan ani uruchomionych równolegle kompilacji. Część techniczna wymaga również pełnych regresji Debug/Release/ASan oraz kontroli narzędzi/dokumentacji. Wynik pomiaru niespełniający zapisanej bramki wymaga poprawy i powtórzenia porównywalnej próby, nie usunięcia złych próbek.

Decyzja użytkownika z 02.11.2026: M06 ma potwierdzić solidność i wiarygodność fundamentu. Użytkownik zadeklarował odbiór; nie wymaga uczestników ani próby grywalności obecnego prototypu 2,5D. Techniczne kontrole zostały wykonane i mają własne dowody w raporcie. Wcześniejsza propozycja uczestników oraz zainteresowania dalszą grą nie jest już bramką M06. Ocena przyszłej gry pozostaje oddzielnym zagadnieniem [projektu gry](game-design.md#ocena-gry).

Dopiero ten etap uzasadnia wybór następnego kierunku: większa populacja, głębsza fizyka, lepsza historia albo poprawa istniejącego zachowania. Hipoteza kolejnej liczebności znajduje się w budżetach. Nie rozwijamy wszystkich wymiarów jednocześnie.

### Po prototypie — kierunek otwarty

**DECYZJA UŻYTKOWNIKA, 02.11.2026.** Plan pierwszego prototypu M00–M06 jest zamknięty w opisanym zakresie. Użytkownik nie wybrał jeszcze najlepszego kierunku dalszych prac. Pełne 3D nie jest zatwierdzonym następnym projektem ani nadrzędną migracją. Wcześniejszy zapis przypisujący taką kolejność został wycofany w DEC-019. Dalsze propozycje badawcze i techniczne wymagają wyboru celu oraz osobnego zakresu przed wdrażaniem. Ten dokument nie ustanawia nowego backlogu.

## Zasada aktualizacji

Status etapu można zmienić na wykonany dopiero z odnośnikiem do rzeczywistego raportu i dostępnego polecenia reprodukcji. Dokumentacja musi wskazywać, które scenariusze nadal nie istnieją. Zmiana kolejności wymaga uzasadnienia zależności, nie tylko nowej listy nazw.
