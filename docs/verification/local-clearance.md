# Lokalna współpraca przy zablokowanym Step

Status: ZALICZONE technicznie w opisanym zakresie, z pełnymi profilami, rzeczywistym oknem i oddzielnym review. [Plan](../work-items/local-clearance.md) określa zakres. To ograniczona współpraca przy rozpoczętym kroku, bez pełnego modelu społecznego i bez odbioru M04.

## Wykonane

Wspólne API prośby, dobrowolny ruch autonomicznego odbiorcy, odmowy controlled/busy/no_space, ochrona rozpoczętej pracy i kroku, zachowanie zamiaru, wygaśnięcie i ręczne przejęcie. Istniejąca fizyka i nawigacja wykonują manewr Walk. F3 zgłasza tę samą prośbę; feedback i inspektor oddzielają zgodę od faktycznego udostępnienia przejścia. Zapis zewnętrzny 3 zachowuje aktywne uzgodnienie. Wersje 1/2, WorkWorld i MotionSession pozostają czytelne.

## Dowody przed/po

- `build/clearance-red.log`: stary kod nie kończy minimalnie zablokowanego Step; kontrolny gracz pozostaje nieruchomy.
- `build/clearance-expanded.log`: ręczne ponowienie pierwotnego zamiaru ujawniło zachowywanie tymczasowego punktu. Wspólne zakończenie manewru teraz odtwarza zamiar i unieważnia jego trasę przed oceną idempotencji komendy.
- `build/clearance-review-red.log`: brak walidacji pary pozwalał wczytać zmieniony rekord. Dodano zgodność obustronną, aktywny Step i transakcyjne odrzucanie.
- `build/clearance-reserved3-red.log`: dwa manewry przy wąskim podparciu, odbiorcy 3,1 m od siebie, rezerwowały nakładające się cele. Teraz cel zgłoszony w protokole sprawdzany jest niezależnie od dystansu obecnej kapsuły. Pierwsze dwie wersje fixture miały nieużywalne połączenia/za mały odstęp przed krokiem; zachowano logi ich porażek i skorygowano geometrię, bez zmiany wymagania rozłącznych celów.
- `build/clearance-stale-red.log`: uzgodniona, ale dawno opuszczona faza kroku była dopuszczana, jeśli proszący znajdował się na tej samej docelowej bryle. Koniec zgody zatwierdzany po fizyce umożliwił ścisłą walidację aktualnego Step, bez tolerancji lub fikcyjnej historii.

13 nowych przypadków obejmuje faktyczne dojście, kontrolowanego odbiorcę, rzeczywistą migawkę, zapis w toku i duplikat prośby, brak miejsca, rozpoczętą czynność i koszt, ręczne przejęcie, ponowienie tego samego zamiaru, korupcję pary, równoległe rezerwacje, wygaśnięcie, zachowanie pracy i odrzucanie przestarzałej fazy. Test wygaśnięcia odczytuje spójny stan ostatniego kroku 200-krokowego okresu; nie jest próbą wydajności ani dowodem dowolnego długiego zakleszczenia.

Wcześniejszy test `retained_ramp_recovery` zachowuje mocne asercje NoPath/stabilności i odczytu, z ręcznie kontrolowanym blokującym (odmowa współpracy). Bez tej kontroli oczekiwanie NoPath stało się niewłaściwe: nowa współpraca faktycznie otwiera przejście. Oddzielny `clearance.retained` ładuje oryginalną niezmienioną migawkę i wymaga fizycznego dotarcia osoby 6 na podest. Nie usunięto wcześniejszej regresji starej trasy, małych przypadków ani śladów czerwonych.

## Rzeczywisty długi świat

`reality_traversal_audit_tests --resume-course docs/verification/evidence/retained-world-stall-6938/world.save`: 1000 walidowanych kroków od 6938 do 7938; `build/clearance-retained-course.log`. Osoba 6 skończyła blokowany Step i przy końcu jechała promem; osoba 7 zjadła, odpoczęła i wróciła do konserwacji. Łącznie stan końcowy ma 23 posiłki, 6 odpoczynków i 8 konserwacji, czyli +11/+2/+1 względem migawki 6938. Nowy obserwator nie zgłosił stagnacji w tej próbie. Nie jest to gwarancja na dowolnie długą grę, pełny tłum, samowystarczalność lub dostawę materiału. Część potrzeb może być nadal wysoka w chronionym przejeździe.

## Niezależny przegląd

Oddzielny agent `clearance_review`, bez historii rozumowania autora, wykonał odczytowe review. Wykrył brak kontroli pary, błędny filtr odległości przy rezerwacjach oraz zbyt szerokie uznawanie zakończonego Step. Reprodukcje i poprawki opisano wyżej. Końcowy werdykt APPROVE, bez kolejnych istotnych uwag. Reviewer nie wykonywał buildów/testów; wykonanie i integracja po stronie autora. To nie zewnętrzny audyt całego silnika.

## Integracja

Release: 181/181 (`build/clearance-final-release.log`), bez skipów i zmiany timeoutów. Wcześniejszy zestaw `build/clearance-integration.log` miał porażkę starego fixture oczekującego NoPath mimo już odblokowanego przejścia; wyjaśnienie zmiany kontroli podano powyżej. Debug: **181/181**, ASan: **183/183**, w tym zamierzony błąd pamięci oddzielnej próbki. Polecenia: `cmake --build --preset windows-debug`, `ctest --preset windows-debug` i analogicznie `windows-asan`; `build/clearance-final-debug.log`, `build/clearance-final-asan.log`, odpowiednie `*-build.log` / `*-console.log`. Release budowano `cmake --build --preset windows-release`, testowano `ctest --preset windows-release`.

`cmake --build --preset windows-debug --target format-check` zaliczone (`build/clearance-final-style.log`). `python -m unittest discover -s tools/tests -p "test_*.py"`: **29/29** (`build/clearance-final-tooling.log`). `python tools/check_docs.py` sprawdza rejestr, linki, identyfikatory i archiwum; ponowione po końcowej edycji dokumentów.

## Rzeczywiste okno i nowy proces

RelWithDebInfo, `--gpu-debug`, MX330, widok 844×781. F9 wczytał rzeczywistą zgodę w toku przy 6939 z `build/clearance-active.save`. Wybrano osobę 6, F3 wyświetliło „zgoda na zrobienie miejsca” i zarejestrowało `clearance_request_actor=6 response=accepted`. F6 przy 6939 zachował świat identyczny bajtowo z wejściem; ponowna prośba nie zmieniła trasy ani uzgodnienia. Pomoc F3, przyciski i inspektor pozostały widoczne po zwiększeniu wysokości pomocy.

Po uruchomieniu czasu i pauzie przy 7682 osoba 6 znajdowała się na powierzchni 6 po drugiej stronie. Druga migawka ma `response=completed`, bez aktywnego manewru odbiorcy. To dalsza praca po rzeczywistym udostępnieniu przejścia, a nie tylko etykieta zgody. Dowody: `build/clearance-ui.log`, `build/clearance-ui-captures/case-160885953-1/` i `case-160936890-2/`. Okno zamknięto normalnie; brak zgłoszonego błędu warstwy D3D11.

Nowy proces `reality_traversal --smoke-test --resume-smoke --gpu-debug --save build/clearance-active.save ...` wznowił aktywną zgodę, wykonał dalsze kroki i zakończył `TRAVERSAL SMOKE PASS`, `simulation_errors=0`, `normal_shutdown=1`. Potwierdził `inspector_records`, `inspector_scroll`, `clearance_shortcut`. Porównano SHA256 pliku wejściowego przed/po: bez nadpisania (`build/clearance-v3-native.log`). Zapisano również `build/clearance-v3-native.png`.

To próba techniczna autora, nie nowa akceptacja komfortu przez użytkownika. Nie wykonywano nowego pomiaru 300 s ani nie przypisujemy tej wersji wyniku FPS wcześniejszego programu.

## Granice

Jeden proszący ma jedną aktywną zgodę. Nie ma przekazywania prośby przez łańcuch osób, negocjacji ceny, reputacji, agresji ani ustępowania dowolnej przeszkodzie. Nie rozwiązuje to wszystkich cykli zależności, wąskich dróg lub problemów próbkowania nawigacji. Odmowa i timeout nie są ukrytym sukcesem. Nowy manewr nie zmienia kosztów WorkWorld, prędkości ani prawa do sterowania graczem. M04 nadal wymaga dostaw i szerszej grywalnej współpracy.
