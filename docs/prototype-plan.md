# Jeden plan pierwszego prototypu

**Status: M00–M03 zaliczone w opisanym zakresie; M04–M06 są planem.** Użytkownik zaakceptował obecne sterowanie M03 z uwagami rozwojowymi dotyczącymi reakcji na cel, płynności trasy, szybkości i naturalności ruchu. Nie ma harmonogramu tygodniowego. Zakres produktu wynika z [karty](project-charter.md), a parametry z [budżetów](performance-budget.md).

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

[Kontrolowany przyrost przeprawy](verification/passage-logistics.md) wiąże dostępność kładki, zużycie i materialną naprawę, lokalną wiedzę, rzeczywisty objazd oraz dostawę umożliwiającą posiłek. W scenie `--crossing` gracz działa na miejscu przez F8/R i przekazuje obserwacje F7. Raport oddziela sprawdzenia SC-10 i części SC-14 od nadal otwartego odbioru całej społeczności, komfortu człowieka i długiego pomiaru. Nie zmienia to kolejności M00–M06.

Kolejny ograniczony przyrost [lokalnej wiedzy](verification/local-station-knowledge.md) dodaje obserwacje stanowisk, przekaz przy fizycznym kontakcie i weryfikację nieaktualnej informacji. Pokrywa część SC-05 i SC-14; nie zmienia warunków pełnego odbioru M04 ani pozostałych etapów.

**Stan bieżący: NIEZAKOŃCZONE.** Dostarczono ograniczoną autonomię potrzeb, pojemność stanowisk, diagnostykę i przyrosty ruchu. Działa ograniczona [prośba o ustąpienie przy Step](verification/local-clearance.md). Nie wdrożono jeszcze szerszego obiegu wiedzy i współpracy ani pełnego dowodu ingerencji gracza oraz odbioru społeczności. Osobny [review naprawy dojścia](verification/retained-world-navigation-stall.md) nie jest odbiorem M04.

Aktualizacja: ograniczony zakres [transferu i pierwszej dostawy](verification/material-delivery.md) został zaimplementowany i sprawdzony; wcześniejsze opisy braku dotyczą stanu przed tym przyrostem. Szerszy obieg wiedzy, pełny dowód ingerencji gracza i odbiór społeczności pozostają otwarte.

26.10.2026 użytkownik zatwierdził [kierunek wejścia w M04](work-items/m04-entry-design.md): skończony odbiór ruchu, prawdziwa dostawa, lokalna współpraca i ingerencja gracza. Pierwszy realizowany [przyrost diagnostyki i pomiaru](work-items/m04-entry-observability.md) przygotowuje ten kierunek. Nie oznacza implementacji transferu materiału, dostaw ani negocjacji. Kolejne przyrosty zależą od jego wyników; nie zmieniają kolejności ani kryteriów całego M04.

Przed tym etapem użytkownik zatwierdził mały [przyrost prowadzenia do celu](work-items/navigation-polish.md). Bezpośrednie odcinki, krótsza łamana, stabilne ponowienie celu i czytelny wynik mają [odrębny raport](verification/navigation-polish.md). To dopracowanie M03 w tym samym planie; nie wdraża autonomii ani nie zmienia prędkości postaci. Ocena nowego odczucia pozostaje osobną próbą użytkownika.

Następnie użytkownik zlecił [audyt praktycznych przypadków](work-items/motion-adversarial-audit.md) przed M04. Przyłącza przy ścianach, obejście osoby i interpolacja rzeczywistej drogi kroku mają [osobny odbiór](verification/motion-adversarial-audit.md). To nadal wzmacnianie fundamentu ruchu; nie jest rozpoczęciem społeczności M04 ani automatyczną akceptacją odczuć.

Wymaga M01 i M03. Wspólne reguły potrzeb, pracy, zasobów, transportu i wiedzy działają w aplikacji i runnerze. Parametry początkowej populacji wskazują budżety. Gracz ingeruje w ten sam świat. Dowód: SC-04, SC-05, SC-08, SC-10 i SC-14.

Przyrosty [potrzeb](verification/autonomy-need-fallback.md) i [pojemności stanowisk](verification/reusable-capacity.md) przygotowały ten etap. Późniejsze dostawy i lokalna wiedza mają odrębne raporty wskazane powyżej; nadal nie stanowią pełnego odbioru M04 ani pełnego łańcucha skutków działania społeczności. Trwa dopracowanie [rozszerzenia fundamentu](work-items/advanced-traversal.md), z osobnymi granicami weryfikacji.

[Praktyczny audyt wielu postaci](verification/traversal-multi-actor.md) jest odbiorem technicznym wybranego zakresu tego rozszerzenia: kolejek, ruchu, podparcia i kontynuacji. Nowa ocena komfortu przez użytkownika i kontrolowany pomiar dłuższego działania są odrębnymi dowodami. Nie uznajemy ich automatycznie za zaliczone na podstawie testów.

Domyślna społeczność potrafi działać bez gracza w określonym zakresie parametrów. Nie wstawiamy gotowej udanej reakcji, by zaliczyć autonomię.

Warstwa gry: gracz osobiście wpływa na dostawę albo pracę i potrafi odczytać skutek. Sprawdzamy również spokojny wariant bez katastrofy: mieszkańcy pracują i współpracują bez rozkazów. Nie uznajemy samej liczby NPC za wartość gry.

### M05 — Ciągłość, przejście i historia

Wymaga M04. Zamknięcie i naprawa przejścia wpływają na trasy i pracę; migracja reprezentacji zachowuje stan. Jedna scena odnosi się do wiedzy i rzeczywistego wyniku. Dowód: SC-08, SC-09, SC-10 i SC-13 oraz odpowiednia próba aplikacji.

Zmiana dostępności mostu nie potwierdza fizycznego rozpadu. Rozszerzenie o dynamikę i zerwanie elementów musi osobno przejść scenariusze fizyki i spójności konstrukcji.

Warstwa gry: sytuacja udostępnia co najmniej dwie zaimplementowane metody, z kosztami oraz trwałymi skutkami. Mały test bezpośredniego konfliktu Action RPG można dołączyć po jego własnym briefie i odbiorze; nie blokuje wymaganej autonomii i nie staje się rozbudowaną produkcją walki.

### M06 — Odbiór i decyzja o skali

Wymaga M05. Dłuższe działanie, pomiar budżetów i próba z człowiekiem. Dowód: SC-11 i SC-12 oraz obserwacja kryteriów z [projektu gry](game-design.md). Raport obejmuje również słabe wyniki i ograniczenia.

Odbiór obejmuje też czytelność sterowania, odkrywanie metod, zrozumienie konsekwencji oraz zainteresowanie dalszą przygodą. Automatyczne testy bilansów nie zastępują tej próby. Kryteria i wnioski zapisujemy przed rozbudową skali, bez automatycznego przypisywania ocen 10/10.

Dopiero ten etap uzasadnia wybór następnego kierunku: większa populacja, głębsza fizyka, lepsza historia albo poprawa istniejącego zachowania. Hipoteza kolejnej liczebności znajduje się w budżetach. Nie rozwijamy wszystkich wymiarów jednocześnie.

## Zasada aktualizacji

Status etapu można zmienić na wykonany dopiero z odnośnikiem do rzeczywistego raportu i dostępnego polecenia reprodukcji. Dokumentacja musi wskazywać, które scenariusze nadal nie istnieją. Zmiana kolejności wymaga uzasadnienia zależności, nie tylko nowej listy nazw.
