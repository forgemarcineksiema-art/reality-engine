# Doświadczenia rozdzielające — API

**Status: implementacja i kontrole w domykaniu; wynik badania wskazuje [raport](verification/discriminating-experiment.md).** [Protokół](work-items/discriminating-experiment.md) określa kryteria I/A/T; [plan](work-items/discriminating-implementation.md) zakres wykonania. Stare laboratoria i ich formaty nie zmieniają zachowania.

`ContrastAgent` od początku generuje F i gramatykę do trzech operacji. Przechowuje zgodne klasy semantyczne, a nie wybraną prawdę. Dostaje wyłącznie pomiary; wybiera wejście minimaksowo albo według wspólnej permutacji. Odczyty modeli i generowanie obciążają trwałe limity. Po wyczerpaniu dalsze deklaracje są odmową, natomiast pozostałe fizyczne próby wykonuje wspólna permutacja.

`ContrastSession` posiada dwie postacie i stanowiska w `TraversalSession`, aparat `DiscoveryWorld`, modele, fazy i dziennik. Kalibracja o zadanym dodatnim wyniku może rozpocząć istniejące Maintain, zużywające część typu2 w WorkWorld. To jawna reguła tego konsumenta badawczego, bez zmiany ogólnej konserwacji gry. Kontakt i praca używają istniejącej fizycznej dostępności. Surowa wiadomość zachowuje świadka i observed_tick; via i learned_tick wskazują rzeczywisty kontakt. Wariant bez wiadomości pokonuje tę samą drogę i ponosi ten sam czas kontaktu.

`GENESIS_CONTRAST 1` jest osobną otoczką. Odtworzenie prywatnego kandydata sprawdza checksum, kanoniczny dziennik i pełny końcowy stan przed podmianą sesji. Pochodne współrzędne kontaktu odtwarza zdarzenie kontaktu; nie mają konkurencyjnego właściciela. Dziennik rezerwuje kończenie impulsu i konserwacji; horyzont10000 kroków nie może uniemożliwić ukończenia zaakceptowanej operacji. Odtwarzanie historii kosztuje CPU i nie jest dowodem skalowalnego zapisu świata.

Nowe surowe dane używają **schema2**: wymagają czasu i współrzędnych kontaktu, wspólnej podpory oraz pomiarów kosztu. `judge_legacy` jest jawnym czytnikiem pierwszej historycznej próby rozwojowej, nigdy ścieżką nowego runnera. Niezależny sędzia sprawdza arytmetykę, modele, deklaracje, podziały, wybory, pochodzenie i koszty; płaska scena ma znaną geometrię, a rzeczywisty brak przeszkód i dostęp sprawdza sesja natywna. To nie ogólny Pythonowy solver ruchu.

```powershell
cmake --build --preset windows-debug --target reality_contrast reality_contrast_tests
ctest --preset windows-debug -R '^contrast[.]' --output-on-failure
python tools/run_contrast_experiment.py prepare build/contrast-development-NEW --exe build/msvc/RelWithDebInfo/reality_contrast.exe
python tools/run_contrast_experiment.py run build/contrast-development-NEW
python tools/run_contrast_experiment.py analyze build/contrast-development-NEW
```

Katalog próby musi być nowy. Freeze zapisuje źródła, rzeczywiste metadane MSVC/CMake/MSBuild, binarium i seedy przed generowaniem praw/zadań. Potwierdzenie wymaga dodatkowo `--mode confirmatory --review RECORD --development DIRECTORY`, czystego commitu i zgodnego rozwoju dla tych samych źródeł i binarium. `run` jest jednorazowe; `analyze` można powtarzać zamrożonym czytnikiem. Nie zmieniać zależności czytnika ani raw danych, żeby uzyskać lepszy wynik. Wynik naukowy negatywny ma kod procesu0; uszkodzone dowody lub infrastruktura są błędem.

`reality_contrast --fixture NEW_DIRECTORY` tworzy cztery rzeczywiste checkpointy. `--resume SAVE 1|2|80 OUTPUT` jest konsumentem kontynuacji fixture'a, nie ogólnym scenariuszem wznowienia każdej fazy badania. Ogólne ładowanie zapewnia API sesji. Czas CPU i ścienny obejmuje rzeczywistą pracę procesu; pamięć `process_lifetime_peak_bytes` to narastający szczyt procesu uruchamiającego R, a potem A. Nie przypisujemy go niezależnie wariantowi. Czas uczenia obejmuje konstrukcję obu początkowych umysłów; koszty B zawierają jego własną generację, choć ta następuje przed kopiowaniem ramion. Całość wariantu obejmuje obie gałęzie społeczne, czas gałęzi pokazujemy osobno. Nie jest to benchmark aplikacji 3D ani pomiar FPS.
`reality_contrast --metadata` podaje wkompilowane hashe wybranych źródeł, konfigurację, MSVC, architekturę i ASan. Identyfikator jest aktualizowany zależnością buildu, a freeze odrzuca starsze lub inaczej skompilowane binarium. Pakiet zawiera również pełne użyte CMakeCache/CMakeCXXCompiler i wynik audytu, nie tylko ich hashe.
