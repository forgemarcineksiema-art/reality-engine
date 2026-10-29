# Dwie metody rzeczywistej dostawy w M05

**Aktualizacja odbioru 29.10.2026:** użytkownik zaakceptował opisany zakres bez osobnego playtestu. [Zbiorczy raport M05](m05-acceptance.md#decyzja-użytkownika) zachowuje dosłowną deklarację i oddziela ją od obserwacji; wcześniejsze wzmianki o oczekującym odbiorze opisują stan przed tą decyzją.

**Status: technicznie ukończone i zweryfikowane 29.10.2026. Odbiór użytkownika tej sytuacji niewykonany.** Zakres określa [brief](../work-items/m05-two-methods.md), a pełne wymagania M05 [jeden plan](../prototype-plan.md). Akceptacja poprzedniego uczenia pozostaje w [jego raporcie](practice-in-prototype.md#ocena-użytkownika). Ten przyrost nie zamyka całego M05 ani M06.

## Uruchomienie

```powershell
cmake --build --preset windows-release
python tools/start_practice_playtest.py --two-methods
```

Ten sam program można uruchomić z `--two-methods`. Wariant zawiera osiem osób, istniejący aparat, most i prom. Start w pauzie. Most jest zużyty i zamknięty; kurier 2 i badacze 4/6 są początkowo ręczni. Pozostali zachowują własne potrzeby i autonomię.

- Objazd: wybierz 2, F2 uruchamia kuriera; Spacja uruchamia czas. Kurier sam poznaje zamknięcie i przewozi żywność promem.
- Naprawa: wybierz 4, F2 uruchamia badacza; Spacja uruchamia czas. Po rzeczywistym otwarciu mostu wybierz 2 i F2 uruchom kuriera. Badacz poznaje metodę, zużywa energię, część i czas pracy. Można też używać dotychczasowych ręcznych czynności I/O/P i R.
- F2 przełącza autonomię, F5/F9 zapisują/wczytują. Launcher tworzy odrębny katalog dowodów. Porównanie zaczyna się od świeżego uruchomienia tej samej sceny.

## Dowody początkowe

Przed zmianą wykonano `ctest --preset windows-release -R '^(community|practice)[.]' --output-on-failure`: 36/36 PASS, 11,64 s. Nowy test natywny wywołujący stare okno z `--two-methods` zakończył się kodem 1, przed powstaniem nowej sceny. Kompilacja napisanych testów C++ następnie odtworzyła brak `make_two_methods_course` (C3861). To dowody braku funkcji, nie porażka istniejącego scenariusza ani ukończony wynik integracji.

Logi bieżącej pracy pozostają w `build/m05-two-methods/`, w tym `baseline-release.log`, `red-native.log` oraz `red-build.log`. Końcowe wyniki i ograniczenia zostaną dopisane po sprawdzeniach.

## Zachowana porażka i korekta planu

Audyt istniejących dowodów przed zmianą: `practice.*` i format 8 już obejmowały działające uczenie, płatną konserwację, korupcję i kontynuację; `passage.*` oraz `community.*` obejmowały most, prom, fizyczne partie, wiedzę i potrzeby. Przyrost wykorzystuje te implementacje. Nowy dowód łączy je w jedną sytuację startową i dwie metody oraz checkpointy pełnych przebiegów. Migracja reprezentacji SC-09 i historia reagująca na zmienionych uczestników SC-13 nadal wymagają osobnych przyrostów M05.

Pierwsza próba Release: 1/5 PASS, cztery porażki `two_methods.repair`, `detour`, `fresh_process`, `native`. Przy początkowo przyjętych 600 krokach kurier dostarczał 4/8; w wariancie naprawy niósł następną partię, w objeździe wrócił i jadł. Nie było dowodu utknięcia. To autor błędnie przyjął dla dwóch partii horyzont ze starego odbioru M04, który wymagał w nim co najmniej czterech jednostek.

Dodatkowa próba diagnostyczna, bez zmiany stanu początkowego, fizyki, potrzeb ani wykonania, zakończyła pełne dostawy w krokach 756 i 967. Przed końcowymi przebiegami ustalono nowy horyzont tej sytuacji: 1200 kroków świata, czyli 60 s. Osiem dostarczonych jednostek, rzeczywisty posiłek i bilanse pozostają wymagane. Nie zmieniono dawnych 240/600 kroków, limitu CTest 20 s ani liczby części/energii. Wynik nie dowodzi ukończenia do kroku 600.

## Wynik scenariusza

W Release, start od identycznej sceny, sterowanie wyłącznie przez uruchomienie dotychczasowej autonomii wskazanych osób. W naprawie kurier zostaje uruchomiony po rzeczywistym otwarciu mostu; badacz pracuje dalej i zaspokaja własne potrzeby. Nie wpisano wyniku ani wiedzy do świata.

| Obserwacja | Poznanie metody i naprawa | Objazd |
| --- | --- | --- |
| Pierwsza fizyczna dostawa, krok | 289 | 325 |
| Osiem jednostek dostarczone i posiłek, krok | 756 | 967 |
| Rzeczywista droga kuriera | Most, bez rejsu | Prom, bez mostu |
| Ukończone konserwacje do tego momentu | 6 | 0 |
| Pozostałe części z 24 | 18 | 24 |
| Ostatni lokalny odczyt źródła energii z 8192 | 7929 | 8192 |
| Ukończone posiłki na drugim brzegu | 4 | 4 |

To koszty całych przebiegów, w tym dalszej autonomicznej konserwacji; nie minimalny koszt pojedynczego otwarcia ani ogólna przewaga naprawy. Pojedyncza czynność nadal zużywa jedną część i trwa 80 kroków. Źródło energii mierzymy tylko w fizycznym zasięgu; zero ukrytego zdalnego widoku nie jest pomiarem wyczerpania. Bilans 20 jednostek żywności i 24 części wraz ze zużyciem sprawdzany jest po każdym kroku.

Nowy zestaw Release przeszedł 5/5 (6,36 s), w tym dwa checkpointy w nowych procesach: rzeczywista konserwacja przy stanowisku 2 i rzeczywisty rejs z ładunkiem. Całe końcowe bajty są identyczne, a plik checkpointu nie jest nadpisywany. Native Win32/D3D11 na MX330: obie metody, F2, F5/F9 podczas chronionych operacji, obcy poprawny świat i powrót, panel 8/8, obraz oraz diagnostyka `hardware=1`, `gpu_debug=1`, `simulation_errors=0`. Oględziny zapisanego obrazu potwierdziły scenę dwóch brzegów, most, prom, ludzi i aparat. Computer Use nie znalazło targetowalnego okna oddzielnie uruchomionej próby; nie zaliczamy z tego bezpośrednich oględzin całego panelu ani oceny komfortu.

## Pozostałe porażki weryfikacji

Pierwszy pełny Release: 361/363 PASS. `contrast.native_case` prawidłowo wykrył nieaktualne hashe źródeł w binarium badawczym po późniejszych zmianach plików i buildzie tylko wybranych targetów. `documentation.registry` odrzucił tytuły zaczynające się od `M05 —` jako ponowne definicje etapu poza właścicielem. Poprawiono tytuły i wykonano cały build. Następny pełny Release: 363/363 PASS, 63,13 s.

Pierwszy pełny Debug: 360/363 PASS, 433,66 s. `two_methods.fresh_process` przekroczył 20 s po trzech z czterech ukończonych uruchomień, a `two_methods.native` przekroczył własny limit 18 s. Nie zwiększono limitów: rozdzielono obie metody na osobne testy procesów i okna, zachowując wszystkie kroki i asercje. Obecne nazwy to `two_methods.fresh_repair`, `fresh_detour`, `native_repair`, `native_detour`, obok `choice`, `repair`, `detour`. Pojedynczy proces kontynuacji nadal ma limit 8 s, okno 18 s, a CTest 20 s.

W tym samym pełnym Debug wystąpił także timeout istniejącego `community.native_diagnostics`. Bez zmiany tego testu, kodu lub limitu jego osobna kontrola przeszła w 5,85 s. Przyczyny zmienności czasu nie ustalono; późniejszy wynik nie usuwa pierwotnej porażki. Końcowy pełny zestaw jest wymagany ponownie.

## Koszt odtwarzania umysłów i jego poprawa

Po samym rozdzieleniu prób Debug przeszedł 5/7: `fresh_detour` nadal przekroczył własne 8 s, `native_detour` własne 18 s, a natywna naprawa trwała 17,93 s. Osobny pomiar procesu checkpointu: 8,04 s czasu rzeczywistego i 7,67 s CPU. Instrumentowany przebieg pokazał konstrukcję świata po 4,75 s, dojście do checkpointu po 5,25 s, zapis po 5,66 s, ukończenie po 7,86 s i zapis referencji po 8,48 s. Natywny objazd trwał 20,88 s, w tym 19,66 s CPU; samo odtworzenie checkpointu i późniejszy powrót po obcym zapisie kosztowały około 4,5 s każde. To diagnostyka bez zaliczenia przekroczonych bramek.

Próba równoważnego obliczania tablic operacja po operacji nie rozwiązała problemu i została wycofana. Dokładniejszy profil konstruktora: przygotowanie 0,16–0,23 s, tablice 0,08–0,11 s, deduplikacja 0,04–0,06 s na umysł. Gramatyka już powstawała w wymaganej kolejności, lecz po dołączeniu krótkiego F sortowano ponownie całość. Konstruktor teraz sortuje tylko F i scala dwie uporządkowane listy. Nie zmienia się evaluator, zbiór/kolejność modeli, zużycie limitów ani dane historycznych prób. Test `contrast.model_order` porównuje wszystkie 2370 programów i tablic z dotychczasową referencją pełnego sortowania, dokładne 20440 wygenerowanych programów i 7822044 operacji konstrukcji, zero odczytów i zachowanie budżetów 0/129.

Po zmianie selektywny Debug: 8/8 PASS, 42,57 s. Okno naprawy 8,95 s, objazdu 9,55 s (limit procesu 18 s); kontynuacje 7,18 i 8,08 s na przypadek (limit CTest 20 s, limit pojedynczego procesu 8 s). Instrumentacja czasowa jest wyłącznie diagnostyką testów. Tymczasowa instrumentacja konstruktora została usunięta; zachowano również jej przejściowy błąd kompilacji bez wyłączania ostrzeżeń. Nie jest to pomiar FPS ani gwarancja braku przyszłych timeoutów. Dodatkowy przegląd niezależny tej optymalizacji nie doszedł do skutku przez limit subagenta; wykonuje go autor, bez nazywania go niezależnym.

## Niezależny przegląd

Przegląd świeżego kontekstu GPT-6 Astra zgłosił brak Critical, jedną Important (nieosiągnięty pierwotny horyzont 600) oraz dwa usprawnienia dowodu: zdalne zero mogło pozornie dowodzić zużycia energii, a dowolna czynność mogła udawać checkpoint konserwacji. Zachowano porażki i opisano korektę planu. Test kosztu teraz wymaga początkowego lokalnego odczytu i porównuje wyłącznie kolejne lokalne odczyty. Checkpoint C++ i okna jawnie wymaga Maintain, stanowiska 2 i rzeczywistej czynności na jego partii. Nie wykonywano ponownego niezależnego review; zamknięcie sprawdzono regresjami autora. Dodatkową optymalizację oceniono w osobnym przeglądzie autora opisanym powyżej; nie przypisujemy jej niezależnej akceptacji.

## Końcowe kontrole i artefakty

Wszystkie kompilacje ukończono przed szeregowymi pełnymi zestawami; źródeł nie zmieniano między nimi. Wykonane 29.10.2026:

| Kontrola | Wynik |
| --- | --- |
| `cmake --build --preset windows-debug`, `ctest --preset windows-debug` | 366/366 PASS, 230,88 s |
| `cmake --build --preset windows-release`, `ctest --preset windows-release` | 366/366 PASS, 72,76 s |
| `cmake --preset windows-asan`, build i test `windows-asan` | 368/368 PASS, 330,34 s |
| Target `format-check` | PASS |
| `python tools/check_docs.py` | PASS |
| `python -m unittest discover -s tools/tests -p "test_*.py"` | 58/58 PASS, 8,243 s; również w każdym pełnym CTest |

W końcowym Debug natywne próby naprawy/objazdu trwały 9,25/9,56 s; w ASan 12,98/15,45 s przy niezmienionym limicie procesu 18 s. Kontynuacje obu metod w ASan trwały 10,52/10,97 s na przypadek, poniżej limitu CTest 20 s; limit pojedynczego procesu pozostał 8 s. Historyczny `community.native_diagnostics` przeszedł także w pełnym końcowym Debug (6,13 s). Wcześniejsza zmienność czasu nadal pozostaje zachowanym ryzykiem, nie ustaloną diagnozą termiczną ani gwarancją marginesu w dowolnym obciążeniu laptopa.

Po pełnych zestawach zachowano osobne checkpointy i wyniki obu metod z Release, ponowiono oba natywne warianty z obrazami i uruchomiono rzeczywisty launcher `python tools/start_practice_playtest.py --two-methods --smoke-test`. Sprawdzono także zapis objazdu sprzed optymalizacji: końcowe bajty po kontynuacji nowym programem są identyczne ze starym wynikiem referencyjnym. Dwa końcowe obrazy pokazują otwarty most po naprawie oraz czerwone zamknięte przejście po objeździe; nie zastępują oceny pełnego panelu lub komfortu przez człowieka.

[Manifest SHA-256](evidence/m05-two-methods-2026-10-28/manifest.json) opisuje [archiwum 237 artefaktów i plików źródłowych](evidence/m05-two-methods-2026-10-28/runs.zip), rozpoczęte 28.10 i zweryfikowane 29.10.2026. Zachowano porażki, diagnostykę, pełne logi profili, checkpointy, obrazy, zapis review i samodzielnego przeglądu oraz dokładne bajty źródeł związanych z metadanymi natywnego runnera. Skrypt pakowania sprawdził CRC archiwum i rozmiar/hash każdego wpisu. Manifest zawiera hashe użytych programów Release; binariów nie skopiowano do archiwum i nie budowano archiwalnego checkoutu ponownie. Oryginały pozostają w `build/m05-two-methods/`.
