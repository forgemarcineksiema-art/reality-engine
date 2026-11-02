# Odbiór techniczny M06

**Status: technicznie ukończone i zweryfikowane, 02.11.2026; odebrane przez użytkownika w opisanym zakresie fundamentu.** Właścicielem statusu etapu pozostaje [jeden plan](../prototype-plan.md#m06--odbiór-i-decyzja-o-skali); wykonanie określa [plan techniczny](../work-items/m06-technical-acceptance.md).

## Decyzja użytkownika

02.11.2026 użytkownik wyłączył próbę grywalności i uczestników z M06, deklarując odbiór i zlecając kontrolę solidności oraz wiarygodności fundamentu: „Zrób kurwa swoje i M06 jest odebrany. Fundament ma być kurwa solidny i wiarygodny.” Deklaracja użytkownika i wykonane kontrole techniczne są oddzielnymi dowodami. Nie przypisujemy tej decyzji automatycznym testom i nie ogłaszamy potwierdzenia atrakcyjności przyszłej gry.

## Zakres

Obecny świat 2,5D: osiem trwałych osób, siedem autonomicznych i jedna ręczna; wspólne ruch/kolizja/nawigacja, potrzeby, fizyczna dostawa, konserwacja, lokalna wiedza, ograniczone praktyki badawcze oraz zapis. Bez nowych reguł gospodarki, pełnej fizyki 3D i zwiększania populacji. Przygotowany świat ma 10000 jednostek w każdym z trzech stanowisk materiałowych; dwa stanowiska odpoczynku zachowują pojemność 2. W trakcie prób niczego nie uzupełniamy.

Opcjonalny profil zachowuje dawny `samples.csv` i dodaje CPU/Present/czas CPU procesu, nawigację, obsługę praktyk i zapis, rzeczywiste przedziały GPU, przebieg stanowisk oraz każdej osoby. Szczegół komend i jednostek: [instrukcja](../development.md#profil-wykonania-m06). GPU jest obserwowany przez timestamp/disjoint i nieblokujące odczyty, zgodnie z [kontraktem D3D11](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_query); brak wyniku nie staje się zerem.

## Zachowane porażki i naprawy

- Pierwszy pełny Release: 432/434. `contrast.native_case` wykrył niezgodność podpisu binarnego ze źródłami zmienianymi podczas wykonania; `tooling.tests` znalazł nowy test przed utworzeniem modułu analizatora. Finalne źródła wymagają zamrożenia, ponownej konfiguracji i pełnego przebiegu.
- Kandydat Release: 433/434. Natywny profil wykazał brak eksportu `actors.csv`. Niezależny przegląd również wskazał tę lukę.
- Przegląd: cztery uwagi Important i jedna Minor dotycząca niepełnych argumentów testu aliasów. Reprodukcja hard linku zmniejszyła kopię wejściowego świata z 290432 do 34 bajtów. Nowa ochrona rozpoznaje tożsamość istniejących plików; regresja musi zachować wszystkie bajty źródła.
- Analizator przyjmował uciętą historię osób, zdublowane stanowisko i inny świat/protokół. Pięć regresji najpierw wykazało fałszywy sukces. Wspólny evaluator sprawdza teraz tożsamość wejścia, procesu, protokołu, kompletne unikalne szeregi i odstęp do 1500 ms. Wynik analizatora dotyczy bramek pomiarowych, a kontynuacja wymaga osobnego odtworzenia świata.
- Pierwszy profilowany pilotaż 60 s: p95 17,0017 ms, p99 20,1225 ms, 30 ukończonych czynności. Powtórzenie: p95 22,3983 ms i p99 35,2771 ms — przekroczenie wstępnych bramek. Wyniku nie usunięto ani nie podniesiono progów.
- Diagnostyczny przebieg z walidacją każdego kroku wykazał niepoprawny stan w kroku 915: po lokalnej obserwacji już naprawionego stanowiska usuwano cel konserwacji, zostawiając trasę. Regresja `m06.completed_target` najpierw odmówiła walidacji, następnie przeszła po zwolnieniu nawigacji i wyczyszczeniu zakończonej trasy. Format świata i bilanse pozostają bez zmiany.

## Wyniki końcowe

Kod pomiaru: `3b95471b682d0b00af40423859ba38c5318544eb`. Release SHA-256: `548d66e694f517dc2216876a4bb272c2a911da213751ec263e7dd275e71132f1`. Wszystkie zamrożone hashe źródeł nadal zgadzały się po próbie. [Manifest](evidence/m06-acceptance-2026-11-02/manifest.json) obejmuje dokładne bajty dowodów i binaria identyfikuje hashami, bez kopiowania exe do repozytorium.

| Kontrola finalnego kodu | Wynik | Dowód |
| --- | --- | --- |
| Debug, pełny CTest | 435/435, 237,03 s | [log](evidence/m06-acceptance-2026-11-02/workspace/test-debug-final.log) |
| Release, pełny CTest | 435/435, 87,90 s | [log](evidence/m06-acceptance-2026-11-02/workspace/test-release-final.log) |
| ASan, pełny CTest | 437/437, 332,16 s | [log](evidence/m06-acceptance-2026-11-02/workspace/test-asan-final.log) |
| Python 3.11, narzędzia | 71/71 | [log](evidence/m06-acceptance-2026-11-02/workspace/python-final.log) |
| Styl i rejestr dokumentów | PASS | logi format-final/docs-final w manifeście |
| Kontrola 38400 kroków świata | walidacja po każdym kroku, zgodna kontynuacja przez checkpoint 19200 | [log](evidence/m06-acceptance-2026-11-02/workspace/forecast3.log) |
| Rzeczywista aplikacja, 120 s + 1800 s | wszystkie zapisane bramki pomiarowe PASS, exit 0 | [protokół](evidence/m06-acceptance-2026-11-02/workspace/long/protocol.json), [analiza](evidence/m06-acceptance-2026-11-02/workspace/long/analysis.json), [proces](evidence/m06-acceptance-2026-11-02/workspace/long/process.json) |
| Końcowy świat, nowy proces headless | 200 kroków; checkpoint po 100 daje identyczny wynik | [log](evidence/m06-acceptance-2026-11-02/workspace/continuation.log) |
| Końcowy świat, nowy proces D3D11/GPU debug | exit 0, community_resume=pass, TRAVERSAL SMOKE PASS; źródło niezmienione | [proces](evidence/m06-acceptance-2026-11-02/workspace/native-resume-process.json), [log](evidence/m06-acceptance-2026-11-02/workspace/native-resume-verified.log), [obraz](evidence/m06-acceptance-2026-11-02/workspace/native-resume-verified.png) |

Komendy wykonano z katalogu repozytorium: `cmake --preset windows-msvc`, `cmake --preset windows-asan`, `cmake --build --preset windows-debug`, `windows-release`, `windows-asan`, następnie `ctest --preset windows-debug`, `ctest --preset windows-release`, `ctest --preset windows-asan --stop-on-failure`, `cmake --build --preset windows-debug --target format-check`, `python tools/check_docs.py`, `python -m unittest discover -s tools/tests -p "test_*.py"`. Kontynuacja: `reality_m06_tests --continue SOURCE OUTPUT`; natywna: `reality_traversal --community --smoke-test --resume-smoke --gpu-debug --save SOURCE --report NOWY.log --capture NOWY.png`, z oczekiwanym stanem po 200 krokach w `SOURCE.expected.save`. Dokładne argumenty, absolutne ścieżki i stany świata są w dowodach. Pierwsze uruchomienie natywne przez PowerShell nie było oczekiwane jak proces konsolowy; rozstrzygającym dowodem jest powtórzenie przez subprocess.run i jego rzeczywisty exit 0.

SC-11 obejmują pełne natywne testy wejścia, ruchu, interakcji, resize, minimalizacji/powrotu, adaptera i zakończenia; rzeczywisty końcowy świat również odtworzono i wyrenderowano. Obraz sprawdzono wzrokowo: oba brzegi, przeprawa i mieszkańcy zostały odtworzone. Nie jest to dowód docelowej fizyki 3D. SC-08/09/10 i pozostałe regresje M05, w tym oba warianty dwóch metod, wykonały się w każdym pełnym profilu. Długa scena była bezobsługowa, a próby ingerencji/metod/kontynuacji pozostają osobnymi procesami i dowodami.

## Zamrożony pomiar SC-12

[Protokół](evidence/m06-acceptance-2026-11-02/workspace/long/protocol.json) zapisano przed finalnym pilotażem i długą próbą. Wybrany adapter: sprzętowa NVIDIA GeForce MX330, Release bez GPU debug, viewport **844×781**. Propozycja 1280×720 z budżetów nie stała się osiągniętym parametrem. Laptop i warunki zapisano w environment-pretrial.json; Codex pozostawał otwarty, bez kompilacji i ciężkich zadań równoległych. Lekkie okresowe odczyty procesu są zachowane w long-monitor.jsonl. To rzeczywiste środowisko autora, bez tezy o idealnie izolowanym sprzęcie.

| Wielkość, faza pomiaru 1800 s | Bramka przed próbą | Wynik |
| --- | --- | --- |
| Klatki | zachować wszystkie | 107913, dodatkowo 7198 rozgrzewki |
| Mediana / p95 / p99 / maksimum | p95 ≤20 ms, p99 ≤33,34 ms, max ≤250 ms | 16,6665 / 16,9433 / 19,9069 / 51,8887 ms |
| Zaległość maksymalna | ≤1000 ms | 49,907 ms |
| Krok świata | ≥95% nominalnych 36000 | 36000 |
| Working set: szczyt / zmiana od pierwszej próbki pomiaru | ≤512 MiB / ≤64 MiB | 90,86 MiB / +3,98 MiB |
| Private bytes: szczyt / zmiana | ≤512 MiB / ≤64 MiB | 127,87 MiB / +34,93 MiB |
| GPU, wiarygodne znaczniki | ≥95% wszystkich klatek | 115111/115111, bez pending/skipped; p95 0,152576 ms w fazie pomiaru |
| Praca rzeczywista | wzrost w każdym oknie 300 s | 154, 155, 149, 148, 151, 155 ukończonych czynności |
| Skrajne potrzeby autonomicznej osoby | brak epizodu >60 s | najdłuższy 9,083 s, następnie ustąpił |
| Błędy/interwencje/minimalizacje/resize/zasłonięcia | 0 | 0 |

Czas procesu według zewnętrznego runnera: 1921,26 s z przygotowaniem i eksportem. Mediana oznacza nominalne około 60 klatek/s; nie obiecujemy stałych 60 FPS każdej klatki. [samples.csv](evidence/m06-acceptance-2026-11-02/workspace/long/trace/samples.csv) zachowuje oba okresy. Rozgrzewka ma p99 17,0532 ms i maksimum 96,2826 ms; nie została usunięta. Limit historii świata pozostał 4096; końcowy świat ma 3879 zdarzeń i 976 czynności. To dowód tego przebiegu, bez dowodu nieograniczonego czasu działania.

Rzeczywiste stany końcowe: 469 i 264 posiłki, 196 konserwacji oraz 47 odpoczynków (drugie stanowisko odpoczynku: 0). Zapas materiałów: 9523, 9744 i 9804 jednostki; pojemności odpoczynku nadal 2. Kurier rzeczywiście dostarczył 8 jednostek. Oba brzegi miały własny jawny zapas początkowy, więc nie przypisujemy wszystkich posiłków tej dostawie. Suma wykonanych czynności wzrosła z 64 po rozgrzewce do 976. Badacz 4 ma cztery pomiary własne, a osoba 6 cztery rekordy świadka i metodę; świat zachował źródła i czasy. To dotychczasowe ograniczone uczenie ze znanej rodziny, nie nowa otwarta fizyka.

SHA-256 końcowego świata: `e7d2010bdb2198b4911552694db3ea19c333b64a7a958876a46b6f99ffa72444`. Headless i natywny proces odtworzyły go, przeszły kontynuację i zachowały te bajty.

## Koszt obserwacji i nieudane pilotaże

Zachowano trzy wcześniejsze próby 60 s i końcową parę po 5 s rozgrzewki: profil wyłączony/włączony, to samo binarium, wejście i viewport. Finalny przebieg bez profilu ma p99 17,0657 ms. Finalny profilowany pilot ma p99 **34,557 ms**, przekracza 33,34 ms; p95 17,0946 ms i max 47,0565 ms. Wolne klatki skupiają się w 5–15 s; po 20 s wszystkie pięciosekundowe fragmenty mają p99 ≤17,12 ms. Przyczyna różnicy nie została wykazana, a jedna para nie daje niezależnego dowodu narzutu GPU/temperatury.

120 s rozgrzewki i 1800 s pomiaru były zapisane w protokole przed wynikiem tego pilotażu. Nie zmieniano ich, progów ani binarium po porażce. Długi przebieg przeszedł bramki ustalonej fazy, zachowując wszystkie początkowe próbki. Nie ogłaszamy tym naprawy krótkookresowej zmienności ani gładkiego zimnego startu.

W długiej próbie lokalny licznik starego obserwatora: 228,584 ms; nowego profilu: 11243,5 ms, około 0,098 ms na klatkę. Ten drugi obejmuje formatowanie, odczyt CPU, odbiór GPU i szeregi osób/stanowisk. Nie obejmuje całego narzutu znaczników w rendererze ani wszystkich zegarów zakresów. Dwa zapisy sesji: 100,497 ms łącznie, wywołane przed i po pętli; pierwszy odstęp klatki obejmuje również początkowy zapis; eksport trace/profili: 430,594 ms po pętli. CPU work p95 13,408 ms, Present p95 13,3308 ms; CPU/GPU działają częściowo równolegle, więc nie sumujemy ich jako czasu klatki. Koszt wyszukiwania tras 154,974 ms/2096 wywołań i obsługi praktyk 230,626 ms/68841 wywołań obejmuje oba okresy. Pełne surowe szeregi i definicje zakresów są zachowane.

## Przegląd, decyzje i granice

[Przegląd](evidence/m06-acceptance-2026-11-02/workspace/review.md) to niezależna kontrola gpt-6-astra, REQUEST CHANGES przed naprawami. Cztery Important i problem testów aliasów zostały odtworzone oraz naprawione; końcowe pełne profile są zielone. Nie było drugiego review; nie przedstawiamy finalnej wersji jako zatwierdzonej przez recenzenta. Późniejsza naprawa zakończonej konserwacji ma własny RED/GREEN i pełne regresje. Nie pozostawiono odroczonej drobnej uwagi.

Decyzje wykonawcy i koszt błędnej interpretacji:

1. Kontynuacja istniejącej gałęzi i cache buildów; przegląd zakresu od 63ae6fb. Koszt: historia zawiera wcześniejsze przygotowania M06. Publikacja/merge nie należą do zlecenia.
2. Uczestnicy i zainteresowanie grą wyłączone z M06 przez użytkownika. Koszt: przyszła atrakcyjność produktu pozostaje niezbadana.
3. Dodano wejściowy świat oraz szeregi stanowisk/osób do zakresu pomiaru, aby wykazać realny postęp i zachować wejście. Koszt: szerszy obserwator/CLI; chronią go testy tożsamości plików, brak zmiany formatu i stanów świata oraz jawny koszt obserwacji.
4. Dodano diagnostyczny forecast 38400 kroków przed właściwym pomiarem; wykryty błąd kroku915 naprawiono z regresją. Koszt: ta dodatkowa naprawa i narzędzie nie były w migawce recenzenta, lecz przeszły wszystkie końcowe profile.
5. Długi pomiar wykonano według wcześniej zamrożonych 120+1800 s mimo negatywnego pilotażu z 5 s rozgrzewki. Koszt: zimny start i przyczyna zmienności pozostają otwarte; sukces dotyczy jawnie zdefiniowanej fazy długiej próby.
6. Surowy pakiet zlecenia review zachowano jako plik .txt z identycznymi bajtami zamiast rejestrować jego nagłówek jako normatywną definicję M06. Koszt: jest czytany jako surowy tekst; reguły kontroli dokumentacji pozostają bez zmiany.

7. Odroczenia recenzenta rozstrzygnięto dowodami: pełne profile, finalny pomiar, kontrola postępu i osobny świeży proces kontynuacji. Wymuszone disjoint, awaria GPU, utrata zasilania i uniwersalna odporność sprzętowa nie zostały wykazane. Zapis buffered trace nie gwarantuje przetrwania twardego zakończenia procesu.

Wynik zamyka M06 w zakresie fundamentu obecnego prototypu. Nie jest akceptacją przyszłej gry, większej populacji, pełnej fizyki 3D ani nieograniczonej historii. [Plan prototypu](../prototype-plan.md) został zamknięty; decyzją użytkownika dalszy kierunek jest otwarty (DEC-019 w [rejestrze](../decisions.md)).

## Powtórzenie pomiaru

Zbudować Release wskazanego kodu i przygotować nowy katalog próby. Skopiować zamrożony protokół, zmienić wyłącznie ścieżki do binarium, zachowanego `initial.save` i nowego wyjścia oraz aktualny hash nowego binarium; zachować scenę, bramki, viewport i czas. Uruchomić `python tools/run_m06_acceptance.py --protocol NOWY-protocol.json`. Każda taka próba zapisuje własny protokół i wyniki; zmiana kodu lub warunków oznacza nową próbę. Przy reanalizie zachowanego pakietu użyć jego niezmienionego protocol.json i `--analyze-dir` wskazującego odpowiedni katalog. Analizator sprawdza proces, początkowy świat i kompletność historii; odtworzenie końcowego świata wykonuje osobny helper.
