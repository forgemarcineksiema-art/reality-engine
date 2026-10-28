# Uczenie i konserwacja w prototypie

**Status: wdrożenie działa; trwa końcowa weryfikacja. Odbiór użytkownika: niewykonany.** Nie zamyka M05 ani M06. Kierunek i kolejność określa [jeden plan](../prototype-plan.md), zakres [projekt przyrostu](../work-items/practice-in-prototype.md).

## Co działa

W `reality_traversal --community --learning` działa osiem dotychczasowych osób, te same dostawy, potrzeby, geometria i zegar. Dwie osoby mają dodatkowe zadanie: przy konserwacji muszą poznać użyteczne ustawienie aparatu. Własny pomiar ogranicza konkurencyjne wyjaśnienia. Mieszkaniec wybiera kolejną próbę i może użyć metody, gdy pozostałe modele zgodnie przewidują potrzebny wynik. Nie czeka obowiązkowo na osiem prób.

Poprawny fizyczny impuls uprawnia tylko jego wykonawcę do jednej konserwacji. WorkWorld zużywa jedną część typu2 i odmierza80 kroków; ukończenie zmniejsza zużycie stanowiska o250. Każda następna praca wymaga nowego impulsu, także po poznaniu metody. Zły wynik, brak części, zajętość i brak zasięgu nie tworzą pracy. Nowa scena zaczyna ze zużyciem500, które dalej narasta według istniejących zasad. Nie dodaje drugiej wyreżyserowanej awarii.

Przekaz trwa jeden krok i wymaga kontaktu obu osób na wspólnej, nieruchomej podporze, do2 m, bez przeszkody. Przenosi własne pomiary świadka, z zachowanym źródłem i czasem obserwacji. Odbiorca ponownie wylicza metodę; nie otrzymuje gotowej odpowiedzi ani parametrów prawa. Pierwszy zakres nie obsługuje przekazu przez pośredników ani rozmów na poruszającej się platformie. Dowody wcześniejszych badań I/A/T pozostają niezmienione; to wdrożenie możliwości, nie nowe potwierdzenie przewagi nauczania.

## Jak uruchomić

```powershell
cmake --build --preset windows-release
python tools/start_practice_playtest.py
```

Start w pauzie. Spacja uruchamia czas. Klawisze4/6 wybierają wykonawców; F2 przejmuje postać, WASD i PPM prowadzą ją jak wcześniej. Przy turkusowym aparacie I zmienia wejście, O kolejność modułów A/B/C, P rozpoczyna próbę. R próbuje konserwacji, F7 rozpoczyna przekaz pomiarów najbliższej osobie. F5/F9 zachowują zwykły zapis i odczyt, Q/E kamerę. Launcher tworzy oddzielny katalog dowodów bez nadpisania wcześniejszej próby.

Panel pokazuje własne próby, ostatni wynik, źródło danych, poznaną metodę i etap. Energia jest dostępna tylko lokalnie. Inspektor jest obserwatorem; jego dane nie sterują NPC.

## Trwałość i ograniczenia

Opcjonalny format8 zachowuje wszystkie sekcje7 i dodaje praktyki. Światy bez urządzenia zachowują dotychczasowe bajty i wersje1–7. Odtworzenie porównuje aparat z ponownym wykonaniem zapisanych impulsów, a umysły z dziennikiem rzeczywistych obserwacji i wywołań wnioskowania, łącznie z modelami oraz kosztami. Zgoda na pracę wiąże pomiar z wykonawcą i rzeczywistą czynnością; zużytej zgody nie można wczytać ponownie. Błędny odczyt nie zatwierdza części stanu.

Budżet własnego uczenia wynosi8 prób/1024 jednostki energii na osobę. Źródło aparatu ma8192 jednostki, bez doładowania. Zapisano ograniczenia dzienników:2048 impulsów,128 zdarzeń umysłu,24 obserwacje na osobę i256 kontaktów. Nowe polecenie odmawia przed wyczerpaniem miejsca potrzebnego do ukończenia rozpoczętej operacji. Nie ma badawczego limitu10000 kroków życia świata. Modele są tworzone podczas ładowania sceny; kopiowanie sesji współdzieli niezmienione umysły i odłącza je przy mutacji.

To uczenie w znanej, skończonej rodzinie modeli, nie dowód świadomości ani dowolnej wynalazczości. Gra nadal korzysta z własnego ograniczonego ruchu2,5D, bez pełnego solvera brył. Ruchu nie uznajemy za docelowy na podstawie tego przyrostu.

## Dowody i stan kontroli

Dotychczasowy przebieg:20/20 prób praktyk przeszło w Debug, w tym prawdziwe okno D3D11, P/R/F5/F9, obcy stary świat, konserwacja, uczenie, kontakt, kolejka, inne prawo oraz cztery kontynuacje w świeżym procesie. Rozszerzone testy uszkodzeń i przejęcia wymagają końcowego przebiegu. Pełne Debug/Release/ASan, pomiar okna i niezależny przegląd pozostają do odnotowania po wykonaniu.

Zachowane porażki: początkowa odmowa zapisu bez formatu8, przekroczenie20 s przez10001 kroków z kopiowaniem modeli, błędy kompilacji przejściowych wersji, zbyt nisko umieszczona ściana fixture oraz zbyt wczesna próba operatora po dojściu w teście okna. Nie zmieniono progów kontaktu, limitów czasu ani liczby kroków. Po poprawieniu kosztu kopiowania ten sam długi przebieg trwał około3,6 s. Headless nie jest pomiarem wydajności aplikacji.

Bezpośrednia ocena czytelności, sterowania i widocznego skutku przez użytkownika pozostaje niewykonana. Automatyczna próba oraz oględziny przez agenta nie zastępują tej akceptacji.
