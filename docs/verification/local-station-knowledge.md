# Weryfikacja lokalnej wiedzy o stanowiskach

Status: **ograniczony zakres techniczny zweryfikowany**, z pełnymi przebiegami Debug/Release/ASan, diagnostyką D3D11 na MX330 i niezależnym przeglądem. [Brief](../work-items/local-station-knowledge.md) określa zakres. To część INV-08, SC-05 i SC-14, bez pełnego odbioru M04.

## Czerwony dowód

Przed implementacją uruchomiono `cmake --preset windows-msvc`, build targetu `reality_knowledge_tests`, następnie `ctest --preset windows-debug -R '^knowledge[.]' --output-on-failure`. Wszystkie cztery początkowe przypadki nie przeszły na poprzednim kodzie (`build/knowledge-red.log`): brak własnej obserwacji na miejscu, brak zmiany planu po świadku wyczerpania, brak obudzenia po dostawie i brak wersjonowania wiedzy. Zapas rzeczywiście opróżniono przez `interact_cargo`, nie przez atrapy.

## Implementacja i ograniczenia

Sesja przechowuje historyczne obserwacje wolnej ilości i zużycia konstrukcji, źródło oraz ostatniego rozmówcę. Obserwacja wymaga dostępu na miejscu; sam wybór trasy nie odczytuje zapasu. Inspektor odróżnia `belief` od faktycznych `station`. Kierunkowe API i F7 używają tego samego kontaktu fizycznego, automat próbuje przez ograniczone okno po niedoborze/oddaniu materiału. Nie ma modelu słuchu, kłamstwa, zgody społecznej, prawa ani instytucji. Wspólna podpora jest konserwatywnym ograniczeniem rozmowy; nie obiecujemy rozmów między dowolnymi połączonymi poziomami.

Wersja 5 dodaje rekordy wiedzy oraz terminy automatycznego przekazu; odczyt 1–4 nie wymyśla obserwacji. WorkWorld pozostaje właścicielem zasobów i odmów. Nowsza wiadomość nie przenosi materiału i nie kończy czynności. Brak informacji pozostawia okresowy zwiad.

## Regresje i przegląd

`knowledge.*` obejmuje 19 przypadków: `no_global`, `witness`, `delivery`, `persistence`, `stale`, `contact`, `corruption`, `relay`, `scout`, `safe_action`, `bounded_auto`, `autonomous_delivery`, `safe_step`, `safe_boarding`, `safe_riding`, `safe_exiting`, `safe_boarding_expired`, `legacy4`, `safe_clearance`. `build/knowledge-complete.log` pokazuje 19/19 przed pełnymi seriami. Scenariusz autonomiczny wykonuje rzeczywistą dostawę, odbiór raportu i posiłek oraz porównuje zapis po każdym kroku z odtworzoną sesją. Wersje 1–3 zachowują wcześniejsze testy, wersję 4 pokrywa dodatkowo nieobserwowany ładunek i jego deterministyczna kontynuacja. `corruption` zmienia rekordy z ponownie policzoną sumą kontrolną: obce/brakujące ID, nieznane stanowisko, zakresy, przyszłe czasy, sprzeczne świadectwo i metadane przekazu. Odrzucenie nie zmienia poprzedniej sesji.

Testy Step/transportu inicjują historyczne przekonanie w pamięci nadawcy przez jawnie opisaną fixture. Nie edytują ruchu, pozycji, zasobu ani chronionej operacji; sam przekaz nadal przechodzi rzeczywisty kontakt API, a zakończenie wymaga fizycznego ruchu. Nie są dowodem pierwotnej obserwacji tego raportu. Nabywanie prawdziwej informacji, błędna wiedza i odmowa WorkWorld mają osobne testy integracyjne. `safe_action` obserwuje rzeczywiście zajętą pojemność i sprawdza jej zwolnienie oraz własną obserwację po ukończeniu. `safe_clearance` wymaga rzeczywistego udostępnienia Step.

Pierwsza próba ochrony Boarding zakończyła się asercją zmiany planu po wyjściu (`build/knowledge-safety-red.log`): od obserwacji w kroku 30 do wyjścia minęło ponad 100 kroków. Planer prawidłowo dopuszczał już zwiad. Ten wariant zachowano jako `safe_boarding_expired`; świeży raport jest otrzymywany później, już na pokładzie podczas Boarding. Próba skrócenia `travel_ticks` do 5 została odrzucona przez istniejące minimum 20 (`build/knowledge-safety-green.log`); końcowe testy zachowują minimum i geometrię transportu, bez zmiany limitów lub fizyki. Świeże i przeterminowane warianty przechodzą (`build/knowledge-safety-green2.log`).

Pierwszy pełny Debug dał **234/237** (`build/knowledge-final-debug-ctest.log`). Trzy dawne oczekiwania wymagały dostosowania do nowego kontraktu:

- `delivery.wrong_station`: lokalna odmowa może zapisać obserwację. Test nadal wymaga `Denied`, identycznego całego WorkWorld, pustej partii i niezmienionej fazy kursu; dodatkowo wymaga własnej obserwacji wolnych 10 jednostek. Nie dopuszcza załadunku z nieprzydzielonego źródła.
- `delivery.return_fallback`: fixture zapominająca źródło usuwa teraz również jego `BELIEF`, zamiast zostawiać przekonanie o nieznanym miejscu. Końcowe asercje rzeczywistego oddania, miejsca i bilansu pozostają bez zmian.
- `delivery.native_diagnostics`: świat z obserwacjami musi być zapisany dokładnie w wersji 5 i zawierać `BELIEF`. Wznowienie nowego procesu i ochrona pliku wejściowego pozostają; historyczną wersję 4 sprawdza odrębny test.

Te trzy przypadki przeszły osobno (`build/knowledge-integration-green.log`), po czym powtórzono pełny zestaw. Nie usunięto przypadków, nie dodano skipów ani nie zwiększono limitów czasu.

Niezależny recenzent `knowledge_review` wykonał odczytowy przegląd rdzenia, integracji, formatu, testów i UI. Pierwszy przegląd wskazał brak dedykowanych testów ochrony operacji, zwiadu i wygaśnięcia automatu; nie zgłosił potwierdzonego defektu runtime. Po uzupełnieniu dowodów zaakceptował przyrost. Osobny kolejny przegląd trzech powyższych korekt także zakończył się akceptacją, bez osłabienia semantyki. Recenzent nie uruchamiał dodatkowych buildów/testów; wykonane polecenia poniżej należą do głównego przebiegu. Nie wykonano commitów.

## Pełne przebiegi

Z katalogu głównego, Windows/MSVC, 27.10.2026:

| Polecenie | Wynik i dowód |
| --- | --- |
| `cmake --preset windows-msvc` | Sukces, `build/knowledge-configure.log` |
| `cmake --build --preset windows-debug` | Sukces, `build/knowledge-final-debug-build.log`; późniejsza korekta fixture: `build/knowledge-integration-build.log` |
| `ctest --preset windows-debug --output-on-failure` | **237/237**, 65,02 s, `build/knowledge-final2-debug-ctest.log` |
| `cmake --build --preset windows-release` | Sukces, `build/knowledge-final-release-build.log` |
| `ctest --preset windows-release --output-on-failure` | **237/237**, 29,11 s, `build/knowledge-final-release-ctest.log` |
| `cmake --preset windows-asan` | Sukces, `build/knowledge-asan-configure.log` |
| `cmake --build --preset windows-asan` | Sukces, `build/knowledge-final-asan-build.log` |
| `ctest --preset windows-asan --output-on-failure` | **239/239**, 103,90 s, `build/knowledge-final-asan-ctest.log`; także poprawna próbka i rzeczywiste wykrycie kontrolnego heap-buffer-overflow |
| `cmake --build --preset windows-debug --target format-check` | Sukces, końcowo `build/knowledge-final-format.log` |
| `python tools/check_docs.py` | Sukces, końcowo `build/knowledge-final-docs.log` |
| `python -m unittest discover -s tools/tests -p 'test_*.py'` | **29 testów**, sukces, `build/knowledge-python-tests.log` |

Wszystkie przebiegi były sekwencyjne, bez równoległego ciężkiego budowania podczas CTest. Zachowano dwuetapową dostawę, checkpoint po 300 krokach, łączną liczbę kroków/asercji oraz limit 20 s. W ASan `delivery.actual_outbound` trwał 2,65 s, `delivery.actual_completion` 1,85 s, a wcześniejszy `traversal.retained_ramp_recovery` 8,95 s. To wyniki tego przebiegu, nie gwarancja przyszłego czasu na rozgrzanym laptopie ani kontrolowany pomiar długotrwałej wydajności.

## Okno D3D11 i kontynuacja nowego procesu

Po zakończeniu ASan uruchomiono osobny proces `build/msvc/RelWithDebInfo/reality_traversal.exe --smoke-test --delivery --gpu-debug --adapter 0 --report build/knowledge-native.log --save build/knowledge-native.save --capture build/knowledge-native.png --diagnostics build/knowledge-native-diagnostics`. Proces zakończył się kodem **0**. Log zawiera rzeczywisty `selected_adapter=NVIDIA GeForce MX330`, `hardware=1`, `gpu_debug=1`, `knowledge_shortcut=pass`, `knowledge_exchange=pass`, `knowledge_inspector=pass`, `TRAVERSAL SMOKE PASS` i `simulation_errors=0`. Warstwa diagnostyczna zakończyła się bez zgłoszonego błędu. Obejrzano `build/knowledge-native.png`: scena 3D jest renderowana. Obraz obejmuje viewport, nie jest dowodem czytelności inspektora przez człowieka.

Smoke ma odrębną, jawną fixture fizycznej obserwacji pustego stanowiska. Wywołuje rzeczywisty komunikat klawisza F7, sprawdza rekord odbiorcy oraz tekst kontrolki Win32 ze świadkiem i czasem, po czym przywraca pierwotną scenę próby. Nie dopisuje dialogu ani udanej dostawy jako wyniku. To próba aplikacji i obsługi skrótu, nie systemowego wejścia człowieka.

Następnie `python tools/test_traversal_diagnostics.py --exe ./build/msvc/RelWithDebInfo/reality_traversal.exe --delivery-only --output ./build/knowledge-native-continuation` zakończył się kodem **0**. `build/knowledge-native-continuation/delivery.log` i `delivery-resume.log` zachowują dwa rzeczywiste procesy na MX330 z diagnostyką. Test wymaga zapisanej wersji 5 z rekordem `BELIEF`, wznowienia tego świata oraz niezmienionych bajtów pliku dowodowego. Pełne zestawy mają także wcześniejsze próby inspektora, resize, minimalizacji, F6 i kontynuacji.

## Granice odbioru

Zaliczono wskazany model lokalnej wiedzy i jego konsumentów w istniejącym laboratorium. Nie dodano zależności, globalnego podglądu zapasu w wyborze ani sterowania przez diagnostykę. Wspólny typ materiału, fizyczna partia, bilans i tożsamość komend pozostają w WorkWorld. Ręczne przejęcie, zatrzymanie i odmowy nie teleportują materiału.

Nie wykonano nowej oceny komfortu przez człowieka, długiego pomiaru wydajności ani pełnego odbioru społeczności. Brak tych dowodów nie jest zamieniany na sukces przez headless lub screenshot. Komunikacja jest konserwatywna i lokalna: wspólna podpora, 2 m i brak przeszkody; brak ogólnych negocjacji, słuchu, kłamstwa, reputacji i prawa. Czasy zestawów testów są czasami runnera, nie pomiarem FPS ani wydajności społeczności. M04 pozostaje nieukończone.
