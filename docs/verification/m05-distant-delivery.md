# Jeden kurier poza lokalnym obszarem

**Aktualizacja odbioru 29.10.2026:** użytkownik zaakceptował opisany zakres bez osobnego playtestu. [Zbiorczy raport M05](m05-acceptance.md#decyzja-użytkownika) zachowuje dosłowną deklarację i oddziela ją od obserwacji; wcześniejsze wzmianki o oczekującym odbiorze opisują stan przed tą decyzją.

**Status: ograniczony kurs zweryfikowany technicznie po [naprawie kosztu uczenia](learning-cost.md), 29.10.2026.** [Brief](../work-items/m05-distant-delivery.md) określa zakres. Ten raport dotyczy ograniczonego kursu SC-09 i kontynuacji SC-08; nie zalicza całego M05/M06, pełnego 3D ani odbioru użytkownika.

## Model i wykonanie

Model/API/format 9 określa [autonomia](../traversal-autonomy.md#podróż-poza-lokalną-reprezentacją). Kurier przechodzi pomiędzy lokalnym ruchem a czasowym modelem 18 m / 120 kroków. Zostaje ten sam aktor, zlecenie i partia w WorkWorld. Podróż nie odnawia potrzeb. Zajęty terminal czeka, a materiał oddawany jest wyłącznie fizycznie po powrocie. Nie ma pojazdu do rezerwacji, streamingu ani symulowanej geometrii trasy dalekiej.

## Zachowane próby negatywne

Bazowy selektywny Debug (`two_methods.choice`, `practice.contact_support`) przeszedł 2/2. Pierwszy test nowego API nie kompilował się (brak `make_distant_course`/`local`). Po dodaniu sceny i typu, przed wykonaniem przekazania, uruchomiony test zgłosił `courier never handed off to distant ownership`. Logi RED są zachowane w `build/distant-red*.log`.

Pierwszy rozszerzony zestaw Debug: 4/7 PASS. Test czasu przyjął błędnie początkowy elapsed=0 zamiast odczytanego 1; zastąpiono to niezmienionym wymaganiem rzeczywistego przyrostu 20. W teście zajętego terminalu ręcznie sterowany blokujący po zwolnieniu nie mógł sam zjeść; jawnie wznowiono jego autonomię po rzeczywistym odejściu. Test odmowy ruchu obserwował tylko 50 ms i nie osiągał progu 0,01 m/s; fixture teraz wykonuje 250 ms ruchu. Nie zmieniono modelu, wymaganej dostawy/posiłku ani limitów. Oddzielne błędy kompilacji obejmowały błędne miejsce wstawienia inspektora i nazwę metody rysowania. Oryginalne logi pozostają dowodami przebiegu.

Drugi zestaw Debug: 8/10 PASS. Scena pełnego toru w teście ochrony promu nie osiągnęła fazy rejsu; zastąpiono ją istniejącą minimalną geometrią promu i wymaganiem wszystkich trzech faz. Kontynuacja z zajętym terminalem wymagała tego samego jawnego wznowienia autonomii odbiorcy w obu procesach. Trzeci zestaw: 12/12 PASS, 12,32 s. Kolejna regresja chroni powrót przed fikcyjną rezerwacją terminalu przez cudzy cel NoPath.

Przegląd koordynatora wskazał dwie rzeczywiste luki: `depart` mógł przyjąć zaplanowane jedzenie przed rozpoczęciem czynności i oczekujące lokalne wejście ruchu. RED wykazał oba `Departed=0`. Naprawa odmawia Busy bez utraty planu lub wejścia. Rozpoczęty pełny build Debug został jawnie przerwany przed CTest w celu wykonania tej poprawki; nie zaliczono go. Koszt energii przeniesiono do jednego właściciela postępu dalekiej podróży, aby pierwszy tick automatycznego wejścia i ręczny start płaciły identycznie, a czekanie nie płaciło ponownie. Asercje obejmują obie granice.

Niezależny przegląd świeżego kontekstu wskazał jedną Important/P2: anulowana partia przy wiedzy wyłącznie o źródle nie używała dalekiej trasy zwrotnej. Brak innych Critical/Important. Uruchomiony pełny Debug przerwano jawnie po 44 zaliczonych przypadkach, podczas `practice.fresh_process`, przed poprawką; log jest oznaczony INTERRUPTED/partial, nie pełnym sukcesem. Nowy RED odtworzył utknięcie prawdziwej anulowanej partii (known_stations=1). Wspólny planer teraz obejmuje także wybrane znane stanowisko zwrotu. Regresja wymaga tej samej osoby/partii, fizycznego transferu do jedynego znanego stanowiska, dwóch różnych komend, braku fikcyjnej dostawy i bilansu po każdym kroku; trzeci checkpoint wraca w nowym procesie. To poprawka i weryfikacja autora po review, bez przypisywania jej ponownego niezależnego odbioru.

## Weryfikacja końcowa

Pełne profile wykonywane są szeregowo, bez zmiany źródeł ani historycznych limitów. Pierwszy pełny Debug: 376/381 PASS, 515,55 s. Limity procesów przekroczyły `two_methods.fresh_repair`, `fresh_detour` (8 s), `two_methods.native_repair`, `native_detour` i `practice.native` (18 s). Pierwszy pełny Release: 380/381 PASS, 158,13 s; `tooling.tests` przekroczył limit CTest 20 s. Wszystkie 15 nowych `distant.*` przeszło w obu profilach. Logi są w `build/distant-final/`; te przebiegi nie stanowią pełnego zaliczenia.

Koordynator odczytał implementację, diff i lokalnych konsumentów w osobnym przejściu integracyjnym: ruch/kolizje, platformy i kolejki, wiedzę/prawo/praktyki, kodek, dostawy i renderer/Win32. Poprawka zwrotu zachowuje dotychczasowy wybór stanowiska, a daleki planer jedynie realizuje drogę do wybranego celu. Nie wykazano dodatkowego błędu kontraktu. To przegląd integratora po wcześniejszym niezależnym review, nie ponowny niezależny odbiór poprawki. Odbiór nowego kursu przez użytkownika pozostaje osobny.

Pełny ASan: 378/383 PASS, 612,23 s, z tymi samymi pięcioma przekroczeniami czasu co w Debug. Wszystkie nowe `distant.*`, stare formaty w pełnych zestawach oraz diagnostyczna próbka wykrywania przepełnienia przeszły. Pierwszy proces budowania ASan urwał się po wyczerpaniu limitu wykonującego agenta; koordynator wznowił build do kodu 0, a następnie wykonał cały CTest. Nie traktujemy częściowego logu kompilacji jako ukończonego buildu.

Jeden kontrolny pełny Debug, bez zmiany kodu/limitów: 377/381 PASS, 458,80 s. Ponownie nie przeszły cztery próby `two_methods.fresh_repair`, `native_repair`, `fresh_detour`, `native_detour`. `practice.native` i `tooling.tests` tym razem przeszły. Nie wykonujemy kolejnych pełnych powtórzeń tylko w celu uzyskania zielonego wyniku. Osobna kontrola `ctest --preset windows-release -R '^tooling[.]tests$'`: 1/1 PASS, 14,11 s; nie jest to nowy pełny Release. Target `cmake --build --preset windows-debug --target format-check`: PASS.

Osobna diagnostyka checkpointu naprawy w Debug zachowała całe wykonanie: 8,327 s czasu rzeczywistego, 7,844 s CPU; konstrukcja 3,753 s, checkpoint zapisany po 4,474 s, ukończenie świata po 7,648 s i referencja zapisana po 8,223 s. Próba zakończyła się poprawnie, ale przekroczyła historyczny limit pojedynczego procesu 8 s. Diagnostyczny limit 30 s nie jest zmianą limitu testu ani jego zaliczeniem. Koszt CPU stanowi większość czasu; przyczyny zmienności między starszymi i obecnymi przebiegami nie ustalono. Osobny `python -m unittest discover -s tools/tests -p "test_*.py"`: 58/58 PASS, 14,860 s według unittest, 15,613 s całego procesu. Późniejszy sukces nie usuwa pierwotnego timeoutu Release.

Koordynator wykonał osobno `ctest --preset windows-release -R '^distant[.]'`: 15/15 PASS, 4,22 s. `python tools/test_distant_native.py --exe build/msvc/RelWithDebInfo/reality_traversal.exe --output build/distant-native-final` oraz rzeczywisty launcher `python tools/start_distant_playtest.py --smoke-test`: PASS. Native Win32/D3D11 raportuje MX330, `hardware=1`, `gpu_debug=1`, `simulation_errors=0`, checkpoint F5/F9, poprawny obcy świat i dostawę 8/8 z rzeczywistym posiłkiem. Obejrzano oba obrazy GPU: w podróży są dwie lokalne osoby i terminale, po powrocie ponownie trzy. Obraz obejmuje viewport; nie jest bezpośrednim odbiorem czytelności całego panelu ani komfortu przez człowieka.

Zachowano trzy rzeczywiste pary `reality_distant_tests.exe checkpoint/continue`: `travel`, `waiting`, `cancelled`. Każde `continue` jest nowym procesem; pełne końcowe pliki porównano SHA-256 z referencją, wszystkie identyczne. `python tools/check_docs.py`: PASS. Żaden z tych sukcesów nie zastępuje niezaliczonych pełnych profili.

## Zachowane dowody

[Manifest SHA-256](evidence/m05-distant-delivery-2026-10-29/manifest.json) opisuje [archiwum](evidence/m05-distant-delivery-2026-10-29/runs.zip) z dokładnymi bajtami źródeł, raportu, RED/GREEN, przerwanych i pełnych przebiegów, pomiaru CPU, kontroli integratora, checkpointów oraz obrazów. Pakowanie sprawdza CRC i każdy rozmiar/hash wpisu. Manifest podaje również hashe programów Debug/Release/ASan; binariów nie skopiowano i nie budowano odrębnego archiwalnego checkoutu. Oryginały pozostają w `build/`. Archiwizacja zachowuje porażki, nie nadaje pełnego zaliczenia.

## Próba użytkownika i dalsza praca

Po buildzie Release uruchom z katalogu głównego `python tools/start_distant_playtest.py`. Launcher wypisuje osobny katalog dowodów. Spacja uruchamia czas, N dodaje sekundę, 1 wybiera kuriera; F5/F9 zapisuje i odtwarza świat. Panel pokazuje postęp dalekiej podróży, potrzeby, partię i dostawę. W podróży lokalny ruch jest jawnie niedostępny. Po kilku kursach odbiorca powinien otrzymać 8 jednostek i rzeczywiście zjeść. Ocena czytelności tego przejścia przez użytkownika pozostaje niewykonana.

Historyczna bramka przekroczeń czasu została rozwiązana i ponownie sprawdzona w pełnych profilach opisanych w [raporcie naprawy](learning-cost.md). Powyższe porażki zachowują stan wcześniejszych prób; nie są usuwane przez późniejszy sukces. Nie dodano streamingu, ogólnej sieci dalekich tras, fizyki między terminalami ani dalszego scenariusza historii SC-13. Są to granice przyrostu, nie ukryte wyniki testów.
