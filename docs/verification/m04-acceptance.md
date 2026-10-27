# Audyt domknięcia M04

Stan na 27.10.2026: **M04 ZALICZONE w opisanym zakresie technicznym i przez użytkownika**. [Brief](../work-items/m04-acceptance.md) zamyka zakres obecnego zadania; [instrukcja próby gry](m04-playtest.md) wskazuje czynności człowieka. Użytkownik 27.10.2026 potwierdził odbiór po uruchomieniu sceny; [zapis akceptacji](m04-playtest.md) zachowuje jego wypowiedź bez dopisywania szczegółowych ocen.

## Bilans kryteriów

| Kryterium M04 | Rzeczywista implementacja i dowód | Granica |
| --- | --- | --- |
| SC-04, INV-06: realne zasoby, koszt i praca | WorkWorld transfer, rezerwacje, zużycie Maintain/Eat, wielorazowy Rest; community balansuje 20 żywności i 24 części po każdym kroku | Bez produkcji, recept wielozasobowych i samowystarczalności; historyczne rozszerzenia SC-04 pozostają katalogiem rozwoju |
| SC-05, INV-07/08: świadek i legalność | law.* zachowane; community.law łączy pobranie, świadka, lokalny automat, odmowę R, fizyczny zwrot i dalszą pracę w świecie przeprawy | Deterministyczna odmowa obsługi, bez pełnego prawa/instytucji |
| SC-08: kontynuacja | community.continuation zapisuje w trakcie podróży/pracy i nierozwiązanego incydentu; 160 par kroków porównanych bajtowo; korupcja nie podmienia sesji | Nie dowodzi awarii dysku ani migracji szczegółowości M05 |
| SC-10: łańcuch skutków | Zamknięcie po 12 krokach rzeczywistego załadunku, brak zdalnej wiedzy, lokalne wykrycie, objazd i naprawa, dostawa i posiłek | Dostępność, nie fizyczne zawalenie konstrukcji |
| SC-14: spokojna i zaburzona społeczność | Osiem osób, 600 kroków, rzeczywista dostawa/jedzenie/odpoczynek/konserwacja bez poleceń do NPC; porównanie z prostą rutyną | Skończony horyzont 30 s świata; szersza stabilność i skala nie wynikają z niego |
| Wspólny rdzeń gry i runnera | make_community_course, TraversalSession i WorkWorld; natywne F8/F4/F7/R, osiem osób, wznowienie w drugim procesie | Smoke jest automatem, nie rzeczywistą oceną człowieka |
| Warstwa gry: czytelna ingerencja i spokojna autonomia | Scena --community, panel ról i skutków, odrębny launcher, trzy warianty próby | ZALICZONE przez użytkownika 27.10.2026; szczegóły próby nie zostały osobno ocenione |

Aktualny zakres M04 obejmuje małą społeczność z istniejącymi czynnościami i wiedzą. Dawne zdania „nie wdrożono dostawy / szerszego obiegu wiedzy” nie opisują już aktualnego kodu. Nie dopisujemy pełnego sądownictwa, nowych instytucji, dowolnej produkcji, pełnego solvera, skali 12–16 (propozycja budżetu) ani całego M06 jako nowych bramek tego odbioru. Ruch zachowuje wcześniejsze uwagi użytkownika M03 i granice obecnej geometrii.

## Wspólna scena i wyniki

Nowa scena rozszerza istniejącą przeprawę o osiem osób, dwa stanowiska Rest o pojemności 2 i regułę części opiekuna. Nie dodaje nowego stanu symulacji, formatu świata ani reguł AI. Świat jest ten sam w oknie i runnerze. Po każdym kroku test niezależnie sumuje pozostałą żywność/części w stanowiskach i torbach oraz rzeczywiste zużycia czynności; suma musi pozostać 20/24. WorkWorld i sesja przechodzą także pełną walidację.

W 600 krokach (30 s czasu świata), bez rozkazów mieszkańcom:

| Wariant | Dostarczono | Posiłki lewa/prawa | Odpoczynki | Konserwacje | Kurier poznał zamknięcie |
| --- | --- | --- | --- | --- | --- |
| Spokojny | 8 | 6 / 4 | 1 | 4 | Nie było zamknięcia |
| Zamknięcie przez podróżnego w kroku 12 | 4 | 6 / 2 | 1 | 4 | Tak, rzeczywista lokalna obserwacja |

To wynik tego scenariusza, nie benchmark ani dowód bezterminowej gospodarki. Gorszy wynik po zaburzeniu jest zachowany; nie poprawiano tempa głodu, kosztów, prędkości, AI lub zapasu pod liczbę sukcesów.

### SC-14: jawna prosta rutyna

Referencja steruje wyłącznie kurierem wspólnymi set_goal/interact_cargo: źródło → środek mostu → odbiorca → środek mostu → źródło. Każdy transfer wymaga rzeczywistego dojścia i wspólnej fizyki; NPC odbiorców/pracy/odpoczynku pozostają takie same. Kurier referencji nie ma wyboru objazdu, autonomicznego okna przekazu ani planera potrzeb. 240 kroków jest poniżej progu głodu 250 i obu wersjom pozostawia ten sam czas. To porównanie całych sterowników, nie izolowany eksperyment nad algorytmem tras: referencja zatrzymuje się w dokładnych waypointach, a adaptacja korzysta z choose_approach i reach .9. Późniejsza wiedza odbiorcy również może się różnić.

| Sterownik / wariant | Pierwsza dostawa, tick | Dostarczono po 240 | Posiłki po prawej | Faktyczna jazda promem |
| --- | --- | --- | --- | --- |
| Adaptacja, spokojny | 172 | 4 | 2 | Nie |
| Stały most, spokojny | 192 | 4 | 0 | Nie |
| Adaptacja, zaburzony | Brak w horyzoncie | 0 | 0 | Tak |
| Stały most, zaburzony | Brak w horyzoncie | 0 | 0 | Nie |

Brak dostawy w 240 krokach nie jest zaliczony jako dostawa ani matematyczny dowód trwałego utknięcia. W zaburzeniu adaptacja fizycznie używa promu i niesie cztery jednostki; stała rutyna zachowuje plan mostowy. Żadna nie dowozi w oknie, więc ten wynik nie potwierdza przewagi w skuteczności. Oddzielny 600-krokowy test wykazuje późniejszą rzeczywistą dostawę adaptacji. Nie wydłużono porównania po odczycie wyniku ani nie zmieniono asercji spokojnej dostawy.

## Czerwone dowody i naprawy

- `m04-red-build.log`: brak make_community_course przed implementacją sceny. Czerwony build jest dowodem brakującego konsumenta, nie błędem dawnego silnika.
- `m04-first-tests.log`: 8/9; prosta rutyna nie dowiozła spokojnie w240. `m04-reference-diagnostic.log` pokazał, że dotarła do ostatniego odcinka dopiero po wielu dodatkowych zatrzymaniach. Zastąpiono osiem waypointów czterema opisującymi właściwą prostą trasę. Pozostają wspólna fizyka, rzeczywisty transfer, horyzont240 i wymóg spokojnej dostawy. `m04-reference-tests.log`: 9/9.
- `m04-native-red.log`: istniejące okno nie obsługiwało --community przed dodaniem konsumenta.
- Niezależny review wykrył P1 nowego panelu: po rzeczywistym F9 z poprawnym zapisem --law panel odczytywał nieistniejące stanowiska/delivery. `m04-panel-red.log` zachowuje niezaliczoną próbę (proces przekroczył18s). Naprawa dobiera panel do struktury wczytanego świata; nie odrzuca prawidłowych obcych zapisów. Ta sama regresja F9 przechodzi w `m04-panel-green.log`. Limity18s subprocess/20s CTest nie zmieniły się.

Nie wykazano potrzeby zmiany starego planera, bilansów, potrzeb lub prawa; naprawiono rzeczywistą usterkę nowego konsumenta UI. Dawne logi i fixture pozostają bez zmian.

## Natywna kontynuacja i odbiór

Nowy smoke sprawdza F9 obcego świata, F8 po rozpoczęciu załadunku, F4 przejęcia i zwrotu, F7 fizycznego przekazu obserwacji zapasu oraz R odmowy/pracy. Wiadomość prawna do opiekuna biegnie od świadka ograniczonym automatem; nie przypisujemy jej F7 podróżnego. Panel Win32 pokazuje rolę, ilość dostawy, posiłki, pracę, odpoczynek, most i obsługę podróżnego.

Pierwszy proces zapisuje checkpoint240 oraz odrębny expected.save z rzeczywistą dalszą kontynuacją200 kroków. Drugi proces wczytuje checkpoint, porównuje własne kroki z kopią oraz po200 krokach wymaga dokładnych bajtów expected.save pierwszego procesu i rzeczywistego posiłku. Python sprawdza wersję7, osiem osób, passage_belief, incydent/świadectwo oraz brak zmiany obu dowodów. To porównanie z oryginalnym procesem, nie wyłącznie dwóch kopii wczytanego pliku.

### Pełne końcowe przebiegi

Konfiguracje uruchamiano kolejno, z jobs2 dla builda i jobs1 dla CTest. Nie zmieniono dawnych limitów, checkpoint dostawy po300 krokach, asercji ani fixture. Polecenia z katalogu głównego; logi w katalogu dowodów:

| Polecenie | Wynik | Log |
| --- | --- | --- |
| cmake --preset windows-msvc | Sukces | m04-configure2.log |
| cmake --build --preset windows-debug | Sukces | m04-final-debug-build.log |
| ctest --preset windows-debug --output-on-failure | 295/295, 106,16s | m04-final-debug-ctest.log |
| cmake --build --preset windows-release | Sukces | m04-final-release-build.log |
| ctest --preset windows-release --output-on-failure | 295/295, 38,33s | m04-final-release-ctest.log |
| cmake --preset windows-asan | Sukces | m04-asan-configure.log |
| cmake --build --preset windows-asan | Sukces | m04-final-asan-build.log |
| ctest --preset windows-asan --output-on-failure | 297/297, 166,41s | m04-final-asan-ctest.log |
| cmake --build --preset windows-debug --target format-check | Sukces | m04-final-format.log |
| python tools/check_docs.py | Sukces | m04-final-docs.log |
| python -m unittest discover -s tools/tests -p test_*.py | 29/29 | m04-final-python-tests.log |

ASan community.calm trwało2,53s, disturbed2,72s, native_diagnostics8,81s. Dawne delivery.actual_outbound3,12s i actual_completion1,97s pozostają dwuetapowe. Próbka poprawna i kontrolne wykrycie heap-buffer-overflow przeszły. To czasy runnera, nie pomiar FPS, skalowania ani długiego działania aplikacji.

Pierwszy check dokumentów odmówił nagłówka „M04 — ...” w instrukcji, bo identyfikator etapu definiuje wyłącznie prototype-plan. Zmieniono nazwę nagłówka instrukcji, bez zmiany kryteriów; m04-docs-first.log pozostaje. Wszystkie wcześniejsze czerwone przebiegi także zachowano.

### MX330 i launcher

Po pełnych seriach wykonano `python tools/test_community_native.py --exe ./build/msvc/RelWithDebInfo/reality_traversal.exe --output ./build/m04-native-final` (kod0). Oba procesy rzeczywiście wybrały NVIDIA GeForce MX330 z hardware1 i gpu_debug1. Pierwszy potwierdził foreign_save, passage, share, law, work i panel; drugi resume oraz panel. Oba mają TRAVERSAL SMOKE PASS, simulation_errors0, normal_shutdown1; diagnostyka D3D11 nie zgłosiła błędu. [Pierwszy log](evidence/m04-acceptance-2026-10-27/native/community.log), [drugi log](evidence/m04-acceptance-2026-10-27/native/resume.log), [checkpoint](evidence/m04-acceptance-2026-10-27/native/community.save) i [wynik oryginalnej kontynuacji](evidence/m04-acceptance-2026-10-27/native/community.save.expected.save) są zachowane.

Obejrzano [obraz viewportu](evidence/m04-acceptance-2026-10-27/native/community.png): dwie podpory, kładka, prom, osiem osób i stanowiska są renderowane. Obraz nie obejmuje tekstu panelu; jego kontrolkę weryfikuje smoke. Nie jest to ludzka ocena czytelności.

`python tools/start_m04_playtest.py --smoke-test` również zakończyło się kodem0 (`m04-launcher-smoke.log`): launcher rzeczywiście utworzył nowy osobny katalog i uruchomił właściwą scenę. To kontrola launchera w trybie smoke; nie zapisano jej jako wykonanego playtestu człowieka.

### Niezależny przegląd

Osobny agent m04_acceptance_review z czystym kontekstem wykonał odczytowy przegląd. Wskazał odtworzony P1 panelu oraz braki F7/porównania z oryginalnym procesem; poprawiono je i zachowano regresje. Zwrócił też uwagę na nieizolowany charakter porównania172/192 oraz liczbę uruchomień w instrukcji. Końcowy wynik to **APPROVE dla kodu**. Recenzent nie uruchamiał buildów ani GPU; pełne komendy powyżej wykonał autor. [Zapis review](evidence/m04-acceptance-2026-10-27/review.md) zachowuje oba etapy oceny. [Manifest](evidence/m04-acceptance-2026-10-27/manifest.json) podaje bajty i SHA-256 dowodów oraz źródeł. Nie wykonano commitu ani push.

## Odbiór i dalsze granice

Użytkownik potwierdził odbiór M04 po uruchomieniu przygotowanej sceny; [zapis akceptacji](m04-playtest.md) jest dowodem tej części, odrębnym od automatycznych testów. M04 jest zaliczone w ograniczonym zakresie opisanym w niniejszym raporcie. Nie dopisano niepodanych przez użytkownika ocen kamery, poszczególnych wariantów ani listy wykonanych czynności.

Nie wykonano nowego długiego benchmarku ani szerszego badania graczy M06. Historia opiekuna po niewidzianej spłacie ma udokumentowane granice wcześniejszego przyrostu prawa. Skończone zapasy, ograniczony kontakt i jawne limity historii pozostają właściwościami tego prototypu; zaliczenie nie jest deklaracją pełnej symulacji cywilizacji. M05 i M06 pozostają kolejnymi etapami planu, bez automatycznego rozpoczęcia nowej implementacji.
