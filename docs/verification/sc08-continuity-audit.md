# Audyt ciągłości i błędów zapisu SC-08

**Status: aktualny zakres SC-08 dla prototypu zweryfikowany, 29.10.2026.** Baza 1f620fc. [Plan audytu](../work-items/m05-continuity-audit.md), [zbiorczy wynik M05](m05-acceptance.md). Audyt nie zmienia kodu C++, formatów ani progów. Nie stwierdzono luki wymagającej naprawy silnika.

## Kontrakt i przypisanie dowodów

Właścicielem stanu fizycznego, zasobów i dziennika komend pozostaje WorkWorld; TraversalSession posiada ruch, wiedzę, praktyki, zlecenia i historię. INV-05 chroni skutek najwyżej raz, INV-12 spójną kontynuację i ostatni poprawny świat. Porównujemy pełne bajty na tym samym stosie Windows/MSVC, bez obietnicy bitowej identyczności innych platform.

| Wymaganie SC-08 | Rzeczywisty dowód |
|---|---|
| Praca i rezerwacja w toku | work.snapshot_continuation/capacity_snapshot oraz work.fresh_process_continuation: rezerwacja, ukończenie, ponowienie tej samej komendy i brak ponownego zużycia; practice checkpoint 3 zapisuje rzeczywisty Maintain po 17 krokach |
| Uczenie i pamięć | practice checkpoint 0/1/2 zachowują rozpoczęty impuls, krok w toku i odebrany wynik; practice.persistence/corruption oraz pełne bajty nowych procesów obejmują wiedzę, aparat i generatory |
| Ruch i fizyczny ładunek | motion.persistence/partition/retarget_continuation, delivery.persistence; story checkpoint travel wymaga torby 4 i prędkości ponad 0,1, a dwa checkpointy metod obejmują konserwację lub fizyczny transport z partią |
| Oczekiwanie i odblokowanie | motion.waiting_continuation odtwarza NoPath i Waiting oraz późniejsze ruszenie po zwolnieniu przeszkody; clearance.persistence odtwarza rozpoczęty manewr i faktyczne przejście; distant checkpoint waiting odtwarza zablokowane przybycie do lokalnej fizyki |
| Zmiana reprezentacji | distant checkpoint travel/waiting/cancelled: rzeczywista podróż i zwrot/anulowanie, te same ID i partia; wielokrotne przełączenie kontrolują distant.* |
| Skutki zakończone i odmowy | story checkpoint speaker/site/supply/returned/complete/failed zachowują zmieniony świat i trwały wynik; WorkWorld zachowuje dziennik skutków i historyczne ID |
| Korupcja, wersje, brak resetu | work.snapshot_rejection/semantics/journal i testy corruption/legacy warstw kontrolują poprawne sumy z błędną semantyką, stare wersje oraz nienaruszoną sesję po odmowie |
| Nieudana podmiana i dalsza praca | work.save_replacement oraz dodatkowa próba pełnej TraversalSession przy rzeczywistym uchwycie Windows odmawiają podmiany, zachowują stary plik i usuwają własny plik tymczasowy; stary checkpoint nadal daje identyczną kontynuację |

Przegląd kodu: write_snapshot_atomic w src/persistence/world_snapshot.cpp tworzy nowy plik tymczasowy w tym samym katalogu przez CREATE_NEW, sprawdza każdy WriteFile, wykonuje FlushFileBuffers, zamyka uchwyt i dopiero wtedy wywołuje MoveFileExW z REPLACE_EXISTING/WRITE_THROUGH. Wyjątek nie jest zamieniany na sukces; PendingFile usuwa własny niedokończony plik przy zwykłym wyjściu przez wyjątek. TraversalSession::save używa tego samego helpera. load czyta bajty, a restore dopiero po parsowaniu, odbudowie i walidacji kandydata zastępuje sesję. To przegląd implementacji, osobny od wykonanych prób.

## Wykonanie i wyniki

Zachowano 16 par checkpoint/continue Release: cztery fazy praktyk, dwie metody dostawy, trzy fazy dalekiego kuriera i siedem historii. Każde continue jest nowym procesem. Wszystkie pełne końcowe pliki są identyczne z referencją; żaden checkpoint nie zmienił bajtów. Lista, komendy, czasy i SHA-256 są w run-02/fresh-results.json i processes.jsonl w archiwum.

Dodatkowa próba pełnego zapisu v10:

1. Zachowano poprawny checkpoint rzeczywistego ruchu z torbą4. Proces nadrzędny otworzył go przez CreateFileW z FILE_SHARE_READ, bez zgody na usunięcie/podmianę.
2. Drugi proces rzeczywiście ukończył historię i próbował zastąpić plik przez TraversalSession::save. Zwrócił kod1 i błąd replace snapshot; ostatni poprawny plik pozostał identyczny, nie powstał plik wyniku ani osierocony tmp.
3. Po zamknięciu uchwytu stary checkpoint dał identyczną kontynuację jak referencja. Ponowiona próba zapisu zakończonej historii powiodła się i rzeczywiście zmieniła bajty pliku.
4. Brak katalogu, ucięty plik, nieobsługiwana wersja 0 z poprawnym checksumem i brak pliku wejściowego zostały jawnie odrzucone. Poprawny chroniony plik wyniku oraz istniejące błędne wejścia pozostały nienaruszone.

Łącznie pięć oczekiwanych odmów I/O/odczytu i udana kontrola po odblokowaniu. Niezależny przegląd audytu potwierdził zgodność źródeł, logów i SHA-256 wszystkich 16 par; wynik APPROVE, bez istotnych findings. Pierwsze wykonanie audytora zatrzymał błędny oczekiwany tekst ostatniej odmowy: oczekiwano cannot open traversal snapshot zamiast rzeczywistego cannot open snapshot. Był to błąd asercji audytora, nie błąd silnika. Początkowy skrypt/logi pozostają; run-02 po poprawieniu tekstu przeszedł cały zakres.

Szeregowo wykonano build i 123 istniejące testy dla każdego profilu: Debug 123/123 PASS (88,35 s), Release 123/123 PASS (20,06 s), ASan 123/123 PASS (130,61 s). Dokładny regex i lista znajdują się w selected-tests.json; komendy, kody i czasy w matrix.jsonl. Nie jest to nowy pełny CTest 420/422. Selekcja obejmuje wszystkie story/distant/two_methods/practice oraz istniejące przypadki zapisu, korupcji, wersji i kontynuacji. Nie podniesiono limitów 8/18/20 s ani historycznych wyjątków. Czasy są czasami kontroli, nie FPS lub pomiarem trwałości nośnika.

## Granice i wynik audytu

Spełniono normatywny scenariusz SC-08 w obecnym zakresie prototypu: stan w toku, zgodna kontynuacja, zgodność historyczna, jawne błędy i zachowanie ostatniego poprawnego świata. Nie znaleziono wymaganej brakującej implementacji. Nie wykonywano fizycznej awarii dysku, utraty zasilania, wypełnienia nośnika ani twardego zabicia procesu w wybranej instrukcji zapisu. Nie nadajemy tym przypadkom zaliczenia ani gwarancji odporności sprzętu. Pozostają dalszym zakresem katalogu testów; nie są nową bramką zamykanej implementacji M05.

## Zachowane dowody

[Manifest SHA-256](evidence/m05-audit-2026-10-29/manifest.json), [archiwum](evidence/m05-audit-2026-10-29/runs.zip): źródła użytych runnerów, audytor i jego początkowa wersja, obie fazy wykonania, checkpointy/referencje/wyniki, błędy, macierz oraz kontrola dokumentów. Weryfikacja CRC i każdego wpisu zachowuje dokładne bajty. Pełna wcześniejsza macierz i obrazy mają odrębne [dowody SC-13](m05-story.md).
