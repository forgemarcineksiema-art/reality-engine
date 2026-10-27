# Eksperyment odkrywania zależności i przekazu — raport

**Status: eksperyment wykonany; wynik wspierający D, G i S w zatwierdzonym zakresie.** Użytkownik zatwierdził implementację i wykonanie próby 27.10.2026. Właścicielem kryteriów pozostaje [protokół discovery-v1](../work-items/discovery-learning-experiment.md); [API](../discovery-learning.md) opisuje działający konsument headless. Próbę potwierdzającą wykonano raz po kontrolach, review i zamrożeniu artefaktów. Nie zmieniano progów, seedów, kosztów ani budżetów po odczycie wyniku.

## Wynik potwierdzający

64 niezależnie generowane przypadki według protokołu, seedy `0xD15C1000`–`0xD15C103F`. Jednostką porównania jest świat, nie pojedyncze zadanie. Wszystkie przypadki przeszły kontrolę integralności C++ oraz niezależne przeliczenie reguł, bilansu i metryk w Pythonie.

| Hipoteza | Badany wariant | Kontrola | Przyrost | Test znaków | Werdykt według zapisanych progów |
| --- | --- | --- | --- | --- | --- |
| D — wiedza z doświadczenia | 1536/1536 dokładnych prognoz, 100% | Przed doświadczeniem 48/1536, 3,125% | 96,875 pp | 64 wygrane, 0 remisów, p=5,4210e-20 | Wspierający |
| G — nowe konfiguracje i wejścia | 1280/1280 rzeczywistych trafień, 100% | Losowa konfiguracja 233/1280, 18,203125% | 81,796875 pp | 64 wygrane, 0 remisów, p=5,4210e-20 | Wspierający |
| S — przekaz do B | Z wiadomością 1280/1280, 100% | Bez wiadomości 928/1280, 72,5% | 27,5 pp | 56 wygranych, **8 remisów**, p=1,3878e-17 | Wspierający |

Wyniki spełniają również progi bezwzględne 0,90/0,80/0,80 oraz przyrost 0,20 przy p<=0,05/3. Porównania progów liczone są dokładnymi ułamkami; wartości dziesiętne w tabeli służą prezentacji. [Podsumowanie](evidence/discovery-learning-2026-10-27/confirmatory/summary.json), [wszystkie wiersze](evidence/discovery-learning-2026-10-27/confirmatory/rows.json) i [manifest przypadków](evidence/discovery-learning-2026-10-27/confirmatory/manifest.json) pozostają dostępne.

Osiem remisów S zachowano: seedy `d15c1003`, `d15c100f`, `d15c1016`, `d15c101f`, `d15c1020`, `d15c1026`, `d15c103b`, `d15c103d`. W tych światach B osiągał taki sam wynik bez wiadomości. Nie usunięto ich ze średnich; tylko test znaków pomija remisy zgodnie z protokołem.

## Co zostało dostarczone i co zostało nabyte

Dostarczono trzy rodziny funkcji, reprezentację szeregowego urządzenia, dokładny miernik, procedurę eliminacji hipotez, wybór interwencji, selekcję danych do komunikatu, cele i harmonogram. Agentowi nie przekazano rzeczywistych parametrów, seeda, dziennika sędziego ani modelu drugiej osoby.

A z fizycznych interwencji identyfikował nieznane działanie elementów, a następnie przewidywał i wybierał łańcuchy na nowych wejściach 48/80/112 eu. B otrzymywał jedynie własne pomiary odkrywania A i sam odtwarzał zbiór zgodnych hipotez. Model A, przepis zadania i dane jego oceny nie były przekazywane. Źródło i wiek pomiaru zostały zachowane. Zapis i dalsza kontynuacja nie wymagają ponownego uczenia ani ponownego wydania energii.

To wynik identyfikacji układu i użycia znanych rodzin mechanizmów w Genesis. Nie jest odkryciem nowej rodziny praw, samodzielnym powstaniem motywacji, języka, życia lub świadomości.

## Koszty, kontrole i wyniki ograniczające interpretację

| Wskaźnik w 64 przypadkach | Wynik |
| --- | --- |
| Własne próby A | 3–6, średnio 4,671875; średnio 218,75 eu |
| Pakiet pomiarów | 3–6 rekordów; 334–670 bajtów w zapisanej reprezentacji JSON |
| Rzeczywiste dojście i kontakt | 54 kroki / 2,7 s świata w obu wariantach; zapisany ślad ruchu |
| Dodatkowe próby B z wiadomością | 0; pełna wiedza wystarczyła do nowych zadań |
| Próby B bez wiadomości | 2; 128 eu |
| Losowa eksploracja z pamięcią | 24 próby, średnio 875 eu; D=99,609375%, G=100% |
| Praca hipotez całej pary z/bez wiadomości, wraz z oceną B | Średnio 437203,59375 / 1333935,5 ocen elementarnych |
| Czas CPU pojedynczego całego przypadku, wszystkie warianty | 250–843,75 ms, średnio 315,43 ms |
| Wewnętrzny czas rzeczywisty pojedynczego przypadku | Średnio 322,58 ms |
| Szczyt working set procesu | Maksymalnie 6254592 B, około 5,97 MiB |

**Losowa eksploracja z pamięcią także osiągnęła 100% G. Nie wykazano przewagi skuteczności aktywnego wyboru w tym punkcie.** Referencja z założenia wykonuje 24 próby i nie zatrzymuje się po rozpoznaniu. Różnica kosztu obejmuje także tę regułę zakończenia; nie jest izolowanym pomiarem samego wyboru kolejnej interwencji ani dowodem optymalności.

Koszty nauczyciela pozostają w wyniku S. Raport zawiera 14 przyrostów faz, także prognozy bez doświadczenia, wybór spośród 39 konfiguracji i ocenę. Koszt całej pary liczy wspólny checkpoint nauczyciela raz. CPU mierzy GetProcessTimes, więc krótka faza może dostać 0 z powodu rozdzielczości zegara. Bajty wiadomości dotyczą jej reprezentacji JSON, nie rzeczywistego protokołu sieciowego. Czasów nie przeliczamy na FPS, szybkość biologicznego myślenia lub skalę społeczności.

Łatwe cele zerowe nie zostały wycięte. G: 87 zerowych i 1193 niezerowych; A trafił we wszystkie, losowy dobór odpowiednio w 22 i 211. S: 104 zerowe i 1176 niezerowych; bez wiadomości B trafił w 93 i 835. Cele B pokrywały się z celami A w 2–19 zadaniach na świat, średnio 11,640625; **B nie otrzymywał ocen A**, tylko dane z wcześniejszego odkrywania. Przekaz nie był odtwarzaniem tych celów z pamięci A.

## Kontynuacja i fizyczna kontrola negatywna

CTest uruchomił osobne procesy z checkpointem w trakcie impulsu, późniejszym kontaktem, nauką B i nową konfiguracją. Wynik drugiego procesu musi być bajtowo zgodny z dalszym przebiegiem procesu oryginalnego; plik wejściowy pozostaje niezmieniony. Test obejmuje również ścieżkę Unicode i odrzucenie korupcji. Dodatkowa regresja zachowuje stan losowego sterownika po checkpointcie.

Po pełnych seriach wykonano dodatkowo Release:

```powershell
./build/msvc/RelWithDebInfo/reality_discovery.exe --fixture --output build/discovery-physical-final
./build/msvc/RelWithDebInfo/reality_discovery.exe --continue build/discovery-physical-final/checkpoint.save --output build/discovery-physical-resume
```

Oba procesy zwróciły kod 0, a SHA-256 expected.save pierwszego procesu i final.save drugiego jest zgodny. [Checkpoint](evidence/discovery-learning-2026-10-27/physical/original/checkpoint.save), [oryginalny wynik](evidence/discovery-learning-2026-10-27/physical/original/expected.save) i [kontynuacja](evidence/discovery-learning-2026-10-27/physical/resumed/final.save) są zachowane.

Osobna rzeczywista aparatura spoza rodzin wykonała 16/32/64 eu i dała 0/32/64. Po drugim pomiarze model stał się nieadekwatny; trzecia wymuszona próba miała jawną nieznaną prognozę. Bilans: źródło 8080, wyjście 96, strata 16, razem 8192 eu. [Surowe pomiary](evidence/discovery-learning-2026-10-27/physical/original/negative-control.json) i [zapis](evidence/discovery-learning-2026-10-27/physical/original/negative-control.save) pokazują granicę modelu; nie dodano nowej rodziny pod wynik.

## Historia wykonania przed zamrożeniem

Zachowujemy czerwone buildy brakujących modułów i konsumenta, pierwszy błąd zapisu komendy w planie, nieudaną fixture ściany oraz nieudany build po przestawieniu zależnych nagłówków Windows/PSAPI przez formatter. Ściana miała początkowo górę na poziomie podłogi; podniesienie góry do 2 m poprawiło fixture, bez zmiany kryterium blokowania kontaktu. Pierwszy pełny CTest 313/313 ruszył mimo nieudanego builda, dlatego nie stanowi końcowej weryfikacji aktualnych źródeł.

Niezależny odczytowy review wskazał sprzeczny stan pending po wczytaniu, błąd porównania dokładnej granicy 0,20 w float, pominiętą kontrolę integralności w CLI analyze, niepełne koszty faz i brak fizycznej kontroli spoza rodzin. Każdy brak otrzymał czerwony test. Poprawki nie zmieniły progów ani seedów. Po poprawkach i kontroli generatorów recenzent zwrócił **READY**; nie uruchamiał próby potwierdzającej. [Pełna historia review](evidence/discovery-learning-2026-10-27/review.md) zachowuje także ograniczenia tej oceny.

Przed próbą potwierdzającą wykonano 16 seedów rozwojowych: D=100%, G=100%, S z wiadomością=100%, bez niej=71,5625%. [Wynik rozwojowy](evidence/discovery-learning-2026-10-27/development/summary.json) pozostaje odrębny; nie użyto go do zmiany kryteriów.

## Pełne kontrole techniczne

Build jobs=2, CTest jobs=1; konfiguracje i pomiary uruchamiano kolejno. Zachowano dawne limity 20 s (oraz istniejący złożony test M03 z 50 s), wszystkie wcześniejsze przypadki i dwuetapową dostawę. W katalogu [technical](evidence/discovery-learning-2026-10-27/manifest.json) są również pierwsze porażki i wcześniejsze zielone serie.

| Polecenie z katalogu głównego | Rzeczywisty wynik końcowy | Log |
| --- | --- | --- |
| `cmake --preset windows-msvc` | Sukces | configure.log |
| `cmake --build --preset windows-debug` | Sukces | final-debug-build.log |
| `ctest --preset windows-debug --output-on-failure` | 314/314, 91,37 s | final-debug-tests.log |
| `cmake --build --preset windows-release` | Sukces | final-release-build.log |
| `ctest --preset windows-release --output-on-failure` | 314/314, 30,88 s | final-release-tests.log |
| `cmake --preset windows-asan` + build windows-asan | Sukces | verified-asan-configure.log, final-asan-build.log |
| `ctest --preset windows-asan --output-on-failure` | 316/316, 153,43 s | final-asan-tests.log |
| `cmake --build --preset windows-debug --target format-check` | Sukces | final-format.log |
| `python tools/check_docs.py` | PASS | final-docs.log |
| `python -m unittest discover -s tools/tests -p "test_*.py"` | 38/38 | final-python.log |

19 testów discovery obejmuje aparaturę, odmowy, ponowienia, wiedzę, składanie, freeze, nieadekwatność, dostęp, kontakt, izolację, kontynuację, korupcje z poprawnym checksumem, budżety i dwa natywne testy procesu. ASan zawiera także kontrolne wykrycie celowego heap-buffer-overflow. Brak zmiany dawnego silnika ruchu zachowuje jego wcześniejsze granice; pełne testy GPU nie są nową oceną użytkownika.

## Zamrożenie i rzeczywiste uruchomienie

Kod badany: **cf8c7515fc24dc7460642cc784b3f46cd5832e15**. SHA-256 zamrożonego Release exe: **2aa3b1922a885cd3d6f0c80291a470066b163d3399e1c0209a4cf332c46a1b0d**. [Freeze](evidence/discovery-learning-2026-10-27/confirmatory/freeze.json) zawiera hashe wszystkich źródeł, protokołu, manifestu, plików przypadków i review oraz środowisko. [Znacznik startu](evidence/discovery-learning-2026-10-27/confirmatory/RUN_STARTED.json) wiąże wykonanie z freeze przed pierwszym przypadkiem.

```powershell
python tools/run_discovery_experiment.py prepare --mode development --exe build/msvc/RelWithDebInfo/reality_discovery.exe --review docs/verification/evidence/discovery-learning-2026-10-27/review.md --output build/discovery-development-01
python tools/run_discovery_experiment.py run build/discovery-development-01
python tools/run_discovery_experiment.py prepare --mode confirmatory --exe build/msvc/RelWithDebInfo/reality_discovery.exe --review docs/verification/evidence/discovery-learning-2026-10-27/review.md --output build/discovery-confirmatory-01
python tools/run_discovery_experiment.py run build/discovery-confirmatory-01
python tools/run_discovery_experiment.py analyze build/discovery-confirmatory-01
```

Polecenia zwróciły kod 0. Analyze ponownie sprawdził freeze, pliki, surowe dane i metryki. Dodatkowe wywołanie run na tej samej próbie zostało **odrzucone przed uruchomieniem agentów** przez istniejący RUN_STARTED; zachowano repeat-refusal.log. Nie był to drugi przebieg 64 światów.

Surowe pliki z obu prób, checkpointy, wyniki negatywne i logi są wersjonowane w katalogu evidence. [Manifest dowodów](evidence/discovery-learning-2026-10-27/manifest.json) podaje rozmiary i SHA-256. Zamrożone exe pozostały w oryginalnych katalogach build, ich hashe są w freeze; kopia repozytorium bez tych lokalnych binariów nie zapewnia bezpośredniego uruchomienia analyze z zapisanymi absolutnymi ścieżkami. To ograniczenie opakowania, nie brak zachowanych wyników lub kodu.

## Granice wyniku

Rodziny praw, reprezentacja układu, miernik bez szumu, motywacja, cel możliwy do wykonania i algorytm wnioskowania były dostarczone. Parametry były nieznane agentowi i stałe w świecie. Nie testowano odkrycia nowej rodziny, zmieniającej się fizyki, dowolnych wynalazków, spontanicznej komunikacji lub samopodtrzymywania życia.

64 seedy są deterministycznym zbiorem pseudolosowym, jawnym dla autora, bez ślepej procedury i bez formalnej analizy mocy. Wnioskowanie dotyczy tego generatora i budżetów. Nie zastępuje szerokiego badania inteligencji ani publikacji zewnętrznie recenzowanej. Wynik S jest korzyścią z danych przy ograniczeniu dwóch własnych prób B; nie dowodzi, że dwie osoby zawsze są skuteczniejsze lub tańsze od jednej.

Nowa funkcja działa headless z istniejącą fizyką i nawigacją. Nie wykonano nowego renderu urządzenia 3D, ludzkiego playtestu, długiego pomiaru społeczności ani skalowania populacji. Kontrola zapisu nie dowodzi awarii dysku ani uwierzytelnienia historycznego świata. Nie jest to nowy odbiór M04 lub ukończenie M05/M06.
