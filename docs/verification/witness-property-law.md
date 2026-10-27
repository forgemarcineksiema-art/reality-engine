# Weryfikacja świadka, informacji i prawa

Status: **ograniczony zakres techniczny ZALICZONY** — Debug/Release/ASan, D3D11 na MX330, format, dokumentacja i niezależny review. Stan na 27.10.2026. [Projekt przyrostu](../work-items/witness-property-law.md) i [model](../traversal-autonomy.md#świadek-własność-i-legalna-obsługa) określają ograniczony zakres SC-05, INV-07, INV-08 i INV-06. Nie jest to odbiór całego M04.

## Rzeczywisty przyrost

Nieautoryzowane pobranie na miejscu wykonuje tę samą operację WorkWorld::transfer i zasila prawdziwe actor.cargo. Chroniona partia należy do opiekuna w WorkWorld; reguła sesji odróżnia opiekuna, mieszkańców i przypisanego kuriera Deliver. Brak legalności nie jest błędem integralności ani fizyczną odmową transferu. Nie zmieniono formatu WorkWorld, kosztów, typów materiałów, wspólnych ID komend ani dwuetapowego testu dawnej dostawy.

Inna osoba obserwuje zdarzenie do 2 m, na tej samej podporze, z obustronnym wolnym sweepem kapsuły. Pierwotne świadectwo ma własne source_actor/via_actor i oba czasy równe tickowi transferu. Brak świadka pozostawia wszystkie pamięci puste. Samo dojście później nie ujawnia sprawcy. F7/share_knowledge i ograniczone okno automatu przekazują fakty z zachowaniem pierwotnego świadka i observed_tick; odebranie wiadomości nie uruchamia automatu pośrednika.

Poinformowany opiekun odmawia sprawcy nowej legalnej obsługi Eat/Rest/Maintain tego stanowiska. Niepoinformowany nadal obsługuje. Rozpoczęta czynność kończy się raz ze swoim kosztem. Fizyczny zwrot jest możliwy mimo odmowy, płaci rzeczywistą ilością FIFO i nie zalicza jednej wpłaty do dwóch incydentów. Wiedza o zwrocie jest osobnym świadectwem; odległy opiekun nadal zachowuje dawną odmowę do otrzymania przekazu. Świadek samej wpłaty bez wcześniejszej wiedzy o kradzieży nie poznaje jej sprawcy.

Format GENESIS_TRAVERSAL 7 zapisuje reguły, incydenty i świadectwa, waliduje powiązanie z przyjętymi transferami, FIFO, własność, ID, duplikaty, czasy i źródła. Odczyt jest kandydatowy; odrzucony plik nie zastępuje sesji. Wersje 1–6 pozostają czytelne, bez wymyślania dawnych incydentów. Inspektor rozdziela testimony_incident, incident_truth i legal_service_actor. Diagnostyka nie steruje światem.

## TDD i zachowane porażki

Punktem wyjścia był czysty lokalny commit 1aa5413 na main. Utworzono gałąź codex/witness-property-law. Nie wykonano commitów ani push.

Po dodaniu czterech testów pierwszy build zawiódł z powodu brakujących typów/API (`law-red-build.log`). Po dodaniu deklaracji, konfiguracji i zapytania legalnej obsługi, jeszcze przed rejestrowaniem incydentów/świadków, rzeczywisty CTest wykazał **0/4** (`law-red.log`): unseen — brak incydentu; isolated — brak świadectwa; relay — brak rekordu w pamięci; restitution — brak reakcji opiekuna. Po implementacji cztery przypadki przeszły (`law-green1.log`).

Kolejne dowody pozostają w katalogu evidence, także po naprawach:

- `law-cases.log`: 10/11. Ściana fixture miała górną powierzchnię na poziomie podłogi, więc jej objętość leżała pod stopami. Podniesiono górną powierzchnię do 2 m i dodano asercję rzeczywistej odmowy kontaktu. Nie zmieniano sweepu, dystansu ani asercji braku wiedzy.
- `law-review-red.log`: repeated_return i owner czerwone. 171. zwrot był blokowany zawyżoną rezerwacją miejsca na świadectwa; chroniona partia nadal wskazywała domyślnego właściciela. Limity przeniesiono do rzeczywistego dopisania, a partię tworzy teraz opiekun. Test limits sprawdza również legalnego mieszkańca przy 256 incydentach i brak częściowego transferu po odmowie 257.
- `law-extra.log`: 17/18. Fixture safe_action docierała do opiekuna dopiero po końcu 80-krokowej pracy. Rozpoczęcie przeniesiono po dojściu świadka, przed przekazem. Czas czynności i wymóg jej rzeczywistego ukończenia pozostają bez zmian.
- `law-unseen-return-red.log`: świadek samego zwrotu otrzymywał wiedzę o niewidzianej kradzieży. Naprawa wymaga wcześniejszej wiedzy o incydencie przy obserwacji uregulowania; regresja unseen_return pozostała.
- `law-validation-red-build.log`: błąd edycji sygnatury pomocnika testowego; poprawiony przed właściwą reprodukcją runtime, bez zmiany modelu.
- `law-validation-red.log`: obie końcowe korupcje czerwone. Brak partii chronionego stanowiska powodował niezerowe wyjście bez stdout zamiast jawnej odmowy; druga fixture była przyjmowana mimo poznania kradzieży dopiero po osobistej obserwacji zwrotu. Naprawy sprawdzają optional oraz learned_tick wcześniejszej wiedzy. `law-validation-green.log` pokazuje **21/21** po poprawkach.
- `law-final-release-ctest.log`: **284/285**, wyłącznie brak rejestracji nowego pliku review.md w mapie dokumentacji. Dodano wpis i ponowiono cały Release; nie wykluczono kontroli.

## Zakres testów

`law.*` obejmuje 20 przypadków C++ oraz native_diagnostics: unseen, isolated, relay, restitution, repeated_return, owner, geometry, authorized, remote_return, bounded_auto, multiple, corruption, legacy, empty_corruption, settlement_chronology, unseen_return, multi_relay, safe_action, limits, legacy_sections.

Pierwsze cztery sprawdzają wymagane kombinacje SC-05 na rzeczywistym cargo i obsłudze. Pozostałe badają ścianę/inną podporę, legalnego mieszkańca i kuriera, wygaśnięcie okna, przekaz A→B→C, brak zdalnego zamykania incydentu, wpłatę do innego stanowiska, wielokrotne naruszenia i FIFO oraz ochronę rozpoczętej czynności. Zapis po rzeczywistym relay, w trakcie pracy i po uregulowaniu jest odtwarzany; kontynuacja jest porównywana bajtowo po krokach. Korupcje mają ponownie obliczoną sumę kontrolną i wymagają zachowania starej sesji.

legacy sprawdza bieżące stare układy 1/4/6 i kontynuację; legacy_sections jawnie konstruuje historyczne układy sekcji 2/3/5 bez nowego stanu i wymaga kanonicznego wyniku po zastąpieniu sesji z prawem. Są to fixture sekcji, nie nowo odkryte historyczne pliki użytkownika. Dawne testy rzeczywistych zapisów, clearance, wiedzy i formatów pozostają w pełnych zestawach bez zmian.

## Pełne przebiegi

Wszystkie konfiguracje uruchamiano kolejno, build jobs=2, CTest jobs=1, bez zmiany limitów, pominięć lub osłabienia dawnych asercji. Polecenia z katalogu głównego:

| Polecenie | Wynik | Dowód w katalogu evidence |
| --- | --- | --- |
| cmake --preset windows-msvc | Sukces | law-configure8.log |
| cmake --build --preset windows-debug | Sukces | law-final2-debug-build.log |
| ctest --preset windows-debug --output-on-failure | 285/285, 96,11 s | law-final2-debug-ctest.log |
| cmake --build --preset windows-release | Sukces | law-final-release-build.log |
| ctest --preset windows-release --output-on-failure | 285/285, 32,56 s | law-final2-release-ctest.log |
| cmake --preset windows-asan | Sukces | law-asan-configure.log |
| cmake --build --preset windows-asan | Sukces | law-final-asan-build.log |
| ctest --preset windows-asan --output-on-failure | 287/287, 151,64 s | law-final-asan-ctest.log |
| cmake --build --preset windows-debug --target format-check | Sukces | law-final-format2.log |
| python tools/check_docs.py | Sukces | law-final-docs2.log |
| python -m unittest discover -s tools/tests -p test_*.py | 29/29 | law-final-python-tests.log |

Pierwszy pełny Debug przed dwiema końcowymi regresjami miał również 283/283 (`law-final-debug-ctest.log`); nie zastępuje końcowego przebiegu 285. Czasy CTest są czasami runnera, nie pomiarem FPS lub wydajności społeczności. Historyczne ryzyko czasu wcześniejszych przyrostów nie jest usuwane przez te wyniki. W końcowym ASan delivery.actual_outbound trwało 4,57 s, delivery.actual_completion 2,23 s, a traversal.retained_ramp_recovery 11,19 s, przy zachowanych limitach i checkpoint po 300 krokach. Próbka poprawna i kontrolne wykrycie heap-buffer-overflow również przeszły. Pełne logi, także porażki, zachowano w tym samym katalogu dowodów.

## Aplikacja i niezależny review

Nowa scena `reality_traversal.exe --law` korzysta z tych samych metod co testy. Native smoke wykonuje rzeczywiste komunikaty F4 i F7, dojście świadka do opiekuna, odmowę konserwacji i odczyt tekstu kontrolki Win32; drugi proces wczytuje wersję 7, porównuje kontynuację i wykonuje rzeczywisty zwrot poza zasięgiem poinformowanego świadka. Zachowuje zatem starszą wiedzę opiekuna. Pierwotny plik dowodowy nie jest nadpisywany. Nie jest to test wejścia człowieka.

Po ASan wykonano `python tools/test_law_native.py --exe ./build/msvc/RelWithDebInfo/reality_traversal.exe --output ./build/law-native-final` (kod 0). Oba rzeczywiste procesy wybrały **NVIDIA GeForce MX330**, hardware=1, gpu_debug=1. Pierwszy log ma law_pickup=pass, law_exchange=pass, law_refusal=pass, law_inspector=pass; drugi law_resume=pass i law_inspector=pass. Oba zakończyły TRAVERSAL SMOKE PASS z simulation_errors=0 i normal_shutdown=1. Warstwa diagnostyczna nie zgłosiła błędu. [Pierwszy log](evidence/witness-property-law-2026-10-27/native/law.log), [kontynuacja](evidence/witness-property-law-2026-10-27/native/resume.log), [zapis](evidence/witness-property-law-2026-10-27/native/law.save) i [obraz D3D11](evidence/witness-property-law-2026-10-27/native/law.png) pozostają w dowodach. Obejrzano obraz: scena 3D jest renderowana. To viewport, bez dowodu ludzkiej czytelności inspektora; tekst kontrolki sprawdza native smoke.

Niezależny agent law_review wykonał odczytowy przegląd z czystym kontekstem. Pierwszy wynik REQUEST CHANGES ujawnił zawyżoną rezerwację i brakujące scenariusze; drugi wskazał dwie walidacje. Po czerwonych reprodukcjach, naprawach i uzupełnieniu testów wynik końcowy to **APPROVE**, bez pozostałych zastrzeżeń do przejrzanego kodu i testów. [Zapis ustaleń](evidence/witness-property-law-2026-10-27/review.md) odróżnia review od poleceń uruchomionych przez autora; recenzent nie uruchamiał buildów ani GPU.

## Granice

To mały deterministyczny model własności i reakcji poprzez odmowę obsługi. Nie dodano ogólnego sądownictwa, reputacji, kłamstwa, kar, pościgu lub zmiany clearance. Kontakt jest konserwatywny, bez słuchu i widzenia między podporami. Limit 256 incydentów/512 świadectw jest jawny, bez retencji. Suma kontrolna i walidacja nie uwierzytelniają pliku ani nie odtwarzają historycznej geometrii.

Restytucja dopuszcza zgodny zastępczy materiał z tej samej torby. Jeżeli nikt znający incydent nie widział zwrotu, fizyczne zobowiązanie jest spłacone, lecz ten przyrost nie ma późniejszej procedury dochodzenia dowodu dla nadal niepoinformowanego opiekuna. Nie ma globalnego odczytu „uregulowano” w decyzji NPC.

Nie wykonano nowej ludzkiej oceny komfortu, długiego kontrolowanego pomiaru ani odbioru całego M04. Screenshot nie zastępuje bilansu i headless nie zastępuje dowodu D3D11.

[Wykaz zachowanych dowodów i sum SHA-256](evidence/witness-property-law-2026-10-27/manifest.json) obejmuje pełne logi etapów, czerwone reprodukcje, review oraz zapis i obrazy próby natywnej.
