# Jeden plan pierwszego prototypu

**Status: M00–M04 zaliczone w opisanym zakresie; M05–M06 pozostają planem.** Użytkownik zaakceptował obecne sterowanie M03 z uwagami rozwojowymi dotyczącymi reakcji na cel, płynności trasy, szybkości i naturalności ruchu. Nie ma harmonogramu tygodniowego. Zakres produktu wynika z [karty](project-charter.md), a parametry z [budżetów](performance-budget.md).

**Ustalona kolejność dalszej pracy, 28.10.2026:** wykorzystanie ukończonych eksperymentów w okienkowym prototypie → pełne M05 → pełne M06 → osobny projekt przejścia do pełnego 3D. Zapis decyzji: DEC-017. Obecny wynik badań nie oznacza jeszcze wdrożenia tej zdolności do zwykłej rozgrywki. Najbliższy przyrost opisuje [projekt integracji praktyk](work-items/practice-in-prototype.md); nie jest to drugi plan etapów ani ponowne otwarcie M04.

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

Warstwa gry: gracz osobiście wpływa na dostawę albo pracę i potrafi odczytać skutek. Sprawdzamy również spokojny wariant bez katastrofy: mieszkańcy pracują i współpracują bez rozkazów. Nie uznajemy samej liczby NPC za wartość gry. Odbiór M04 został potwierdzony przez użytkownika; szersze badanie graczy i pomiar długiego obciążenia pozostają w M06.

### Najbliższy przyrost — wykorzystanie badań w istniejącym prototypie

**Status: PLAN.** Wymaga odebranego M04 i ukończonego [badania odkrywania](verification/discriminating-experiment.md). Uzasadnienie kolejności: zdolność należy najpierw zastosować w żywym świecie i uczynić czytelną dla gracza, a następnie objąć całą trwałością i historią M05. Dotychczasowych wyników I/A/T nie zmieniamy pod potrzeby pokazu.

Rezultat dla użytkownika: w tej samej małej społeczności mieszkaniec bada nieznane ustawienie urządzenia, poznaje metodę potrzebną do konserwacji i rzeczywiście pracuje. Druga osoba zdobywa pomiary przy spotkaniu i później samodzielnie korzysta z wiedzy. Gracz obserwuje próby, dostarcza części lub sam wykonuje dopuszczalne czynności, a po powrocie widzi skutek. Szczegółowy zakres, kontrakty i lista odbioru należą do [projektu przyrostu](work-items/practice-in-prototype.md).

Dowód zamknięcia: wspólna aplikacja i runner mają jeden świat oraz te same czynności; energia i części są rozliczane; potrzeby nie niszczą rozpoczętych operacji; poprawny zapis obejmuje wiedzę i pracę; rzeczywisty obraz/wejście oraz próba użytkownika potwierdzają, że da się zrozumieć sytuację. Sam headless, osobne okno dwóch agentów ani nowy opis nie zamykają tego przyrostu.

### M05 — Ciągłość, przejście i historia

**Status: PLAN.** Wymaga M04 i powyższej integracji praktyk. Zamknięcie i naprawa przejścia wpływają na trasy i pracę; migracja reprezentacji zachowuje stan. Jedna scena odnosi się do wiedzy i rzeczywistego wyniku. Dowód: SC-08, SC-09, SC-10 i SC-13 oraz odpowiednia próba aplikacji. Część przejść i kontynuacji istnieje już w M04; przed implementacją należy przypisać istniejące dowody i wskazać rzeczywiste braki zamiast budować je drugi raz.

| Kolejny zakres | Co ma zadziałać | Warunek zamknięcia |
| --- | --- | --- |
| Ciągłość wspólnego świata | Zapis w trakcie uczenia, ruchu, ładunku, oczekiwania i pracy; powrót gracza bez resetu pamięci | SC-08, stare formaty, uszkodzony odczyt, brak podwójnego zużycia i zgodna kontynuacja w nowym procesie |
| Rzeczywiste przejście szczegółowości | Jeden przewóz wychodzi z aktywnego obszaru, trwa według jawnego modelu podróży i wraca | SC-09: te same ID, ładunek, rezerwacje i postęp/czas; jeden właściciel ruchu; wielokrotne przełączenie bez duplikacji |
| Jedna sytuacja z dwiema działającymi metodami | Proponowane: przywrócić przejście dzięki poznanej metodzie albo zrealizować dostawę istniejącym objazdem | SC-10: oba warianty rzeczywiście dostarczają zasób, mają koszt/czas i pozostawiają różny stan świata |
| Historia reaguje na fakty | Nieobecny wykonawca, niedostępne miejsce albo przeniesiony zasób zmienia przebieg, odmowę lub zakończenie | SC-13: scena nie odtwarza potajemnie uczestnika/zasobu i nie wymaga niewykonalnej rozmowy |

Wyjście poza kamerę lub zapis/wczytanie nie jest dowodem migracji reprezentacji. Pierwszy daleki model może dotyczyć jednego kursu, bez ogólnego streamingu świata, ale musi rzeczywiście przejąć i oddać odpowiedzialność za ten sam przewóz. Przekazanie w chronionej fazie jest odroczone albo odmówione. Porównujemy zadeklarowane chronione własności modeli; nie udajemy identycznej fizyki obu reprezentacji. Szczegół wymaga własnego briefu i testów przed kodem.

Zmiana dostępności mostu nie potwierdza fizycznego rozpadu. Rozszerzenie o dynamikę i zerwanie elementów musi osobno przejść scenariusze fizyki i spójności konstrukcji.

Warstwa gry: sytuacja udostępnia co najmniej dwie zaimplementowane metody, z kosztami oraz trwałymi skutkami. Mały test bezpośredniego konfliktu Action RPG można dołączyć po jego własnym briefie i odbiorze; nie blokuje wymaganej autonomii i nie staje się rozbudowaną produkcją walki.

### M06 — Odbiór i decyzja o skali

**Status: PLAN.** Wymaga pełnego M05. Dłuższe działanie, pomiar budżetów i próba z człowiekiem. Dowód: SC-11 i SC-12 oraz obserwacja kryteriów z [projektu gry](game-design.md). Raport obejmuje również słabe wyniki i ograniczenia.

Pracę dzielimy na: zamrożenie sceny i kryteriów odbioru; kontrolowany dłuższy pomiar aplikacji; próbę z ludźmi; naprawę wykrytych wad i ponowną kontrolę dotkniętego zakresu; końcowy raport i decyzję użytkownika. Propozycja pomiaru autora to 2 minuty rozgrzewki oraz 30 minut rejestrowanego działania w Release na i5/MX330/8 GB, z fazą spokojną, uczeniem, ingerencją, odejściem/powrotem i zapisem. Czas, fazy i progi należy zapisać w protokole przed próbą, odwołując się do [właściciela budżetów](performance-budget.md). Nie są to osiągnięte parametry ani zmiana dotychczasowych limitów testów.

Mierzymy rozkład klatek i przycięcia, CPU/GPU, pamięć i jej trend, zaległość aktualizacji, koszt wnioskowania/nawigacji i zapisu oraz postęp pracy mieszkańców. Do oceny czasu nie używamy ASan ani uruchomionych równolegle kompilacji. Część techniczna wymaga również pełnych regresji Debug/Release/ASan oraz kontroli narzędzi/dokumentacji. Wynik pomiaru niespełniający zapisanej bramki wymaga poprawy i powtórzenia porównywalnej próby, nie usunięcia złych próbek.

Odbiór obejmuje też czytelność sterowania, odkrywanie metod, zrozumienie konsekwencji oraz zainteresowanie dalszą przygodą. Automatyczne testy bilansów nie zastępują tej próby. Kryteria i wnioski zapisujemy przed rozbudową skali, bez automatycznego przypisywania ocen 10/10.

Zachowujemy istniejącą propozycję próby z co najmniej trzema osobami z [projektu gry](game-design.md): samodzielne rozpoznanie dwóch metod, zrozumienie skutków i kontynuacji bez gracza oraz konkretne uzasadnienie chęci dalszej gry. Oddzielnie zapisujemy bezpośredni odbiór użytkownika. Jeśli tych prób nie wykonano, odpowiednia część pozostaje nieweryfikowana; agent AI ani test automatyczny nie zastępują uczestnika. Nieudana ocena czytelności lub sterowania nie staje się zaliczeniem przez samo ukończenie czasu próby.

Dopiero ten etap uzasadnia wybór następnego kierunku: większa populacja, głębsza fizyka, lepsza historia albo poprawa istniejącego zachowania. Hipoteza kolejnej liczebności znajduje się w budżetach. Nie rozwijamy wszystkich wymiarów jednocześnie.

### Po M06 — osobny projekt pełnego 3D

**KIERUNEK USTALONY PRZEZ UŻYTKOWNIKA, szczegóły nadal otwarte.** Po zamknięciu całego prototypu następuje osobny projekt przekształcenia obecnego zakresu 2,5D w pełne 3D. Obecne renderowanie brył nie jest dowodem pełnej fizyki/nawigacji 3D. Granice nowej geometrii, ruchu, kamery, interakcji i ewentualnej dynamiki muszą zostać opisane oraz przyjęte przed implementacją.

Nowy projekt rozpoczyna się od audytu tego, co zachowujemy z silnika i domen, oraz planu migracji zapisów i odbioru tych samych sytuacji. Nie zakładamy napisania całości od zera ani prostego podmienienia grafiki. M05/M06 nie mogą przenosić swoich braków do tego projektu tylko po to, aby uznać prototyp za zakończony. Projekt 3D nie staje się teraz dodatkową bramką M06, a jego techniczne szczegóły nie są dziś zatwierdzoną implementacją.

## Zasada aktualizacji

Status etapu można zmienić na wykonany dopiero z odnośnikiem do rzeczywistego raportu i dostępnego polecenia reprodukcji. Dokumentacja musi wskazywać, które scenariusze nadal nie istnieją. Zmiana kolejności wymaga uzasadnienia zależności, nie tylko nowej listy nazw.
