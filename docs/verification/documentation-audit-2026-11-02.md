# Audyt aktualności dokumentacji i porządkowanie repozytorium

**Status: audyt i korekty zweryfikowane, 02.11.2026; porządkowanie publikacji opisane niżej.** Zakres zlecił użytkownik po wykryciu starego statusu w AGENTS.md. Punktem odniesienia jest ukończony prototyp techniczny M00–M06 oraz wyjaśnienie, że dalszy kierunek nie został wybrany. Nowej funkcji silnika ani następnego projektu nie wdrażano.

## Co było nieaktualne

AGENTS.md i środowisko pracy kończyły podsumowanie na M04, README na M05, a nagłówek planu nadal mówił o planowanym M06. Było to sprzeczne z zakończoną sekcją M06 i jej raportem. Różne dokumenty powielały również nieobowiązujące bramki uczestników i przypisanie pełnego 3D jako obowiązkowego następnego projektu. GitHubowe main pozostawało na 952c8c7, podczas gdy opublikowane M05/M06 znajdowały się na gałęziach tematycznych. Wynikała z tego także nieaktualność domyślnego widoku repozytorium.

## Zakres audytu

Zinwentaryzowano i przeskanowano pełną treść **212 istniejących dokumentów**: Markdown, pliki tekstowe, raporty, plany, rozmowy, indeksy i instrukcje agentów. CMakeLists.txt sklasyfikowano oddzielnie jako konfigurację, zamiast doliczać go do dokumentów. Odczytano również **917 tekstowych członków w 17 historycznych ZIP-ach**, bez ich wypakowywania lub zmiany. Wszystkie te materiały otrzymują wynik w inwentarzu audytu.

Ręczna kontrola znaczenia dotyczyła aktualnych właścicieli tematów, statusów, odniesień do przyszłych etapów, kontraktów i znalezionych podejrzanych fragmentów. Automatyczna kontrola obejmuje rejestr dokumentów, lokalne odnośniki, identyfikatory, integralność źródeł i hashe. Nie jest to ponowne wykonanie dawnych eksperymentów ani odświeżenie wszystkich zewnętrznych publikacji i ofert modeli AI.

## Wykonane korekty

- AGENTS.md i README przedstawiają ukończony pierwszy prototyp techniczny M00–M06. AGENTS zachowuje kluczowe ograniczenia oraz kieruje do właścicieli kontraktów, zamiast powtarzać chronologię dawnych przyrostów.
- Plan, karta projektu i DEC-019 pozostawiają następny kierunek otwarty. Wycofano z bieżących instrukcji obowiązkowy projekt 3D, a DEC-017 skorygowano bez dopisywania nowej decyzji za użytkownika.
- Architektura, środowisko pracy, strategia testów i program badań uwzględniają obecne systemy oraz trzeci wykonany eksperyment. Zachowano negatywne wyniki H/A i T oraz granice odbioru.
- Plany wykonanych przyrostów mają właściwe role w mapie dokumentacji. Historyczne raporty otrzymały kontekst tam, gdzie dawne braki można było odczytać jako dzisiejszy status; nie zmieniano dawnych liczb testów, pomiarów, kryteriów ani wypowiedzi użytkownika.
- Naprawiono uszkodzone polskie znaki w planie wykonania praktyk. Nie zmieniano kodu lub reguł uczenia.
- Pięć pierwotnych rozmów/propozycji ChatGPT i Gemini/Argon przeniesiono do [źródeł historycznych](../research/sources/project-documents-2026-11-02/manifest.json), zachowując dokładne bajty. Ich dotychczasowe ścieżki są krótkimi indeksami z informacją o obecnym statusie. Manifest archiwum dodaje kontrolę nowych kopii do istniejącego walidatora.

**Zachowano wszystkie 97 wcześniej wersjonowanych surowych dokumentów historycznych bez zmiany bajtów.** Dotychczasowe dowody, archiwa ZIP, zapisy świata i fixture'y pozostają na miejscu. Rozszerzono jedynie indeks archiwum o nowo zachowane źródła. Pełne 3D nadal występuje jako rzeczywista granica obecnego silnika i w niezmienionych źródłach historycznych; nie jest poleceniem rozpoczęcia migracji.

## Kontrola i cleanup

[Inwentarz każdego dokumentu](evidence/documentation-cleanup-2026-11-02/inventory.json), [teksty w archiwach ZIP](evidence/documentation-cleanup-2026-11-02/zip-documents.json), [niezależny przegląd](evidence/documentation-cleanup-2026-11-02/review.txt) i [manifest dowodów](evidence/documentation-cleanup-2026-11-02/manifest.json) zachowują zakres, wyniki i źródła. Kontrola dokumentów PASS, Python 71/71. Trzy istotne uwagi review i pozostałe stare statusy poprawiono w jednym przebiegu. Weryfikacja Git wykazała przed poprawką zmianę czterech źródeł przez normalizację; po regule -text wszystkie pięć zachowuje bajty także w filtrze Git. Pełne porównanie indeksu jest wymagane przed commitem. Sama proza nie uzasadnia nowego benchmarku ani ponownego przypisywania bieżącej dokumentacji dawnych wyników C++. README i AGENTS uczestniczą jednak w podpisie pochodzenia binarium badawczego, dlatego jego tożsamość wymaga odświeżenia i właściwej kontroli.

Gałęzie można sprzątnąć dopiero po dowiedzeniu, że ich commity są osiągalne z opublikowanego main. Niepołączonej pracy ani niewersjonowanych danych z build nie usuwamy. Dalszy rozwój projektu pozostaje decyzją użytkownika.


## Weryfikacja dopasowana do zmiany

Wykonano `python tools/check_docs.py` oraz `python -m unittest discover -s tools/tests -p "test_*.py"`: PASS i 71/71. Poprzednie binarium badawcze prawidłowo odrzuciło nową dokumentację jako niezgodną ze swoim podpisem źródeł. Następnie odświeżono konfiguracje MSVC/ASan i wyłącznie target `reality_contrast`; `ctest --preset windows-debug -R '^contrast[.]native_case$'`, analogiczny Release i ASan przeszły po 1/1. Wyniki są w dowodach. Nie zmieniono src, testów, narzędzi ani reguł silnika; nie powtarzano całej macierzy lub benchmarku M06 dla samej prozy.

Niezależny recenzent sprawdził 579 lokalnych odsyłaczy/kotwic bez błędów, zgodność 97 dawnych surowych dokumentów oraz 5 kopii oryginałów. Review zakończyło się REQUEST CHANGES; końcowe poprawki zweryfikowano bez drugiego review. Pozostają granice: nie odnawiano zewnętrznych ofert modeli/badań, nie oceniano na nowo prawdziwości wszystkich twierdzeń dawnych rozmów, nie wnioskowano z testów dokumentacji o nowym zachowaniu silnika.

## Porządkowanie publikacji

Wszystkie 14 zastanych lokalnych/zdalnych tipów gałęzi były przodkami opublikowanego prototypu 228ec17. Publikacja zmian w main i usuwanie zbędnych gałęzi wymagają jeszcze ponownej kontroli dokładnych refów. Wynik operacji oraz zachowane wewnętrzne refy Codex zostaną dopisane po wykonaniu; same dane audytu nie są dowodem publikacji.
