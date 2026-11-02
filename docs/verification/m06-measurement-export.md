# Eksport przebiegu pomiaru — pierwszy przyrost przygotowania

**Status: opisany zakres techniczny wykonany i sprawdzony 02.11.2026; rozpoczęty 01.11.2026.** To ograniczony zakres [briefu](../work-items/m06-acceptance.md) i [planu wykonawczego](../work-items/m06-measurement-export.md). Status całego etapu pozostaje u [jednego właściciela](../prototype-plan.md#m06--odbiór-i-decyzja-o-skali); ten przyrost nie zamyka M06.

## Zachowanie

`--measurement-trace DIR` eksportuje rzeczywiste czasy klatek, aktualizacji, zaległości i odczyty pamięci procesu do nowego `samples.csv`. Zachowuje również rozgrzewkę. Dotychczasowy raport nadal obejmuje okres właściwy. Obserwator nie trafia do zapisu świata ani do wiedzy mieszkańców. [Instrukcja](../development.md#eksport-przebiegu-pomiaru) określa format, opcje, koszty i granice.

Istniejące dowody nie są nadpisywane. Obsłużone przerwanie zachowuje dostępny ślad z oznaczeniem niepełnej próby. Osobny `status.txt` pozwala odczytać wynik także po awarii raportu. Błąd eksportu daje kod 1 i nie może otrzymać `normal_shutdown=1`.

## Końcowa weryfikacja

| Profil | Wynik pełnego CTest | Czas wykonania testów |
| --- | --- | --- |
| Debug | **431/431 PASS** | 205,06 s |
| Release | **431/431 PASS** | 77,56 s |
| ASan | **433/433 PASS** | 297,39 s |

To czasy CTest, a nie benchmark FPS. Nie zmieniono limitów ani nie pominięto dawnych przypadków. Końcowy `two_methods.fresh_detour` pod ASan przeszedł w 14,24 s łącznie, przy zachowanych oddzielnych limitach procesów 8 s i limicie CTest 20 s. `contrast.initial_state` przeszedł w tym profilu w 0,38 s. Format i kontrola dokumentacji: PASS. Testy narzędzi: **58/58 PASS**. Nowy zakres ma cztery testy modelu próbek, sześć nowych prób natywnych i test zgodności inicjalizacji umysłów.

Wykonane polecenia z katalogu głównego:

```powershell
cmake --build --preset windows-debug
ctest --preset windows-debug
cmake --build --preset windows-release
ctest --preset windows-release
cmake --build --preset windows-asan
ctest --preset windows-asan --stop-on-failure
cmake --build --preset windows-debug --target format-check
python tools/check_docs.py
python -m unittest discover -s tools/tests -p "test_*.py"
```

Końcowa próba natywna `--community --learning --gpu-debug --warmup-seconds 1 --measure-seconds 2 --measurement-trace DIR` zachowała 178 wierszy: 58 rozgrzewki i 120 pomiaru, z 40 krokami świata w okresie mierzonym. Liczniki zbierania i eksportu wyniosły odpowiednio 0,3942 ms oraz 4,7398 ms; są pojedynczym odczytem sekcji obserwatora i zapisu, bez izolowanego porównania z wyłączoną instrumentacją. Oba pliki mają status poprawnego zapisu, a proces zakończył się kodem 0.

## Historia weryfikacji

Baza kodu tego przyrostu: `3bf2d60f3c9183d1b7d74eefabf3f07fd3e42ae6`. Początkowe wybrane regresje Release: 17/17 PASS. Nowy test natywny na starej aplikacji zakończył się kodem 1, ponieważ brakowało opcji eksportu. Testy modelu najpierw wykazały brak nowego API w kompilacji. Po implementacji testy diagnostyki: 15/15 PASS; natywne próby szeregu, odmów ścieżek, WM_CLOSE i konkurencyjnego CSV: PASS.

Pierwsza pełna macierz CTest: Debug **428/428 PASS** (235,46 s), Release **428/428 PASS** (76,99 s), ASan **430/430 PASS** (367,19 s). W każdym profilu wykonano cztery nowe przypadki modelu i cztery nowe próby natywne. Format i kontrola dokumentacji: PASS. Testy narzędzi: **58/58 PASS**.

Dodatkowa próba ujawniła błędną odmowę nowego katalogu z końcowym separatorem. Zachowano czerwony test natywny; normalizacja końcowego pustego komponentu ścieżki dała wynik zielony. Rozszerzono również próbę odmów o alias Win32 `\\?\` — ochrona działała już przed tą poprawką. Po tej zmianie diagnostyka i próby natywne przeszły 20/20 w każdym z trzech profili. Zachowane logi rozdzielą pierwszy przebieg, poprawkę ścieżek i końcowy zestaw po review.

## Niezależny przegląd i poprawka

Recenzent `gpt-6-astra` ocenił zakres `3bf2d60f..f2493a32` w świeżym kontekście, wyłącznie odczytowo. Wynik: REQUEST CHANGES, jedna uwaga Important, bez Critical i Minor. Nie jest to aprobata późniejszej poprawki ani niezależne wykonanie testów.

Przed próbą zapisu CSV `export_trace` pisał do raportu. Gdy sam raport był źródłem wyjątku, ta operacja ponownie rzucała i blokowała ratowanie bufora. Nowy test natywny blokuje rzeczywisty plik raportu przez Win32 `LockFileEx` po inicjalizacji aplikacji. Na recenzowanym kodzie wykazał brak CSV przy kodzie 1. Po poprawce zachowano 176 wierszy oraz niezależny status `exported=1`, `complete=0`; kod procesu pozostał 1, a początkowy zapis świata bez zmian.

Dane i status są teraz zapisywane przed logowaniem, z osobnym zabezpieczeniem przed nadpisaniem. Drugi test RED→GREEN sprawdza konkurencyjny plik statusu: pozostaje nietknięty, gotowy CSV jest zachowany, a aplikacja zgłasza błąd. Jawne opróżnianie raportu poprzedza zwrócenie sukcesu. Poprawkę zweryfikowały TDD i końcowa pełna macierz, bez ponownego review; recenzent nie oceniał późniejszego usprawnienia inicjalizacji.

Dodatkowe sprawdzenie zgodności pamięci wykazało przeplatanie dwóch terminów próbkowania po rozgrzewce. Regresja natywna najpierw odrzuciła odstęp krótszy niż sekunda we właściwej fazie. Po synchronizacji terminu eksportera przy odczycie pamięci uzyskano wynik zielony: pozostały odczyty nominalnie raz na sekundę oraz pierwszy odczyt fazy, a pierwsza/ostatnia/szczytowa pamięć i szczyt private bytes zgadzają się dokładnie ze starym raportem. Tolerancja czasu w teście to 1 ns wynikający z sześciu miejsc po przecinku w milisekundach CSV. Zachowano surowe porażki, wybrane wcześniejsze pełne macierze oraz dziennik powtórzeń; końcowa macierz dotyczy poprawionej wersji.

## Granice

Krótkie próby natywne są kontrolą kontraktu i działania Win32/D3D11, bez oceny FPS. Nie wykonano dłuższej próby M06, izolowanego porównania narzutu włączonego/wyłączonego eksportu ani sesji z ludźmi. Pamięć dotyczy procesu, bez VRAM. Nie wdrożono osobnych liczników GPU, uczenia, nawigacji i zapisu. Bufor nie gwarantuje przetrwania twardego zabicia procesu, utraty zasilania ani awarii nośnika.

## Ujawnione przekroczenie czasu i zakres usprawnienia

Pełny ASan po korekcie próbkowania zakończył się wynikiem **431/432**, ponieważ starszy `two_methods.fresh_detour` przekroczył niezmieniony limit 8 s procesu checkpointu. Osobna próba z tym samym limitem przeszła: checkpoint 5,52182 s, cały test kontynuacji 10,27 s. Kolejny pełny przebieg ponownie przekroczył limit i został przerwany jako niezaliczony; próba z zatrzymaniem na pierwszej porażce również odtworzyła timeout. Nie usunięto tych wyników i nie podniesiono limitów.

Sondy diagnostyczne tego samego binarium pokazały checkpoint 4,956 s / CPU 4,922 s przy zajętości systemu 17,00% oraz 9,204 s / CPU 8,938 s przy 56,98%. Powtórzenie prefiksu natywnego dało 5,550 s / CPU 5,406 s przy 18,12%. Ich zewnętrzny limit 30 s służył wyłącznie dokończeniu pomiaru; nie jest kryterium odbioru i nie zastępuje limitu 8 s. Checkpointy i wyniki obu czasów były identyczne bajt w bajt. Zajętość systemu jest obserwacją, bez dowodu przyczyny zmienności.

Mikropomiar wykazał koszt wielokrotnego tworzenia tych samych 20440 opisów gramatyki w konstruktorach umysłów. Sprawdzono szybsze wyszukiwanie duplikatów z pełną równością tablic. Wdrożono niemutowalną składnię kodu i stabilne scalanie opisów, a każdy umysł nadal osobno oblicza wszystkie opłacone tablice i zachowuje własne modele, obserwacje oraz budżety. Nie ma cache pomiarów ani tablic modeli i nie zmieniono reguł uczenia. [Właściciel API](../discriminating-learning.md) opisuje ten zakres.

Niezależna referencja pełnego sortowania potwierdza wszystkie 2370 modeli. Sześć kontrolnych hashy pełnego stanu dla budżetów 0, 128, 129, 1000, 100000 i 10000000 pozostało zgodnych, a obserwacja i zamrożenie pierwszego umysłu nie zmieniają drugiego. Osiemsekundowa próba początkowo przeszła checkpoint, lecz przekroczyła limit kontynuacji; tę porażkę również zachowano. Po dodaniu obserwacji etapów para natywna/kontynuacja przeszła: checkpoint 4,99335 s, odczyt i dokończenie w drugim procesie 7,06665 s. Kroki, asercje i limity 8/20 s pozostały bez zmian.

Sonda po usprawnieniu dała 3,627 s / CPU 3,609 s przy 17,22% zajętości, wobec 4,956 s / CPU 4,922 s przy 17,00% przed zmianą. To pojedyncze porównanie inicjalizacji i scenariusza, bez gwarancji stałego przyspieszenia i bez wniosku o FPS. SHA-256 obu plików przed/po jest identyczny: checkpoint `9d1b0f7a5d3bc6177187640ec7164d24bfcadcf403fc8d5133a0a57e064d610d`, wynik `e9c24d7db5c28d0352737e9254327a8bce6e1f6570e01004cab7d73aacb5b768`. Kontynuacja starego checkpointu z nową inicjalizacją dała również identyczny plik wynikowy. Końcowa pełna macierz jest zielona; przyczyna zmienności czasów pod obciążeniem pozostaje nieustalona i jest otwartym ryzykiem M06.
