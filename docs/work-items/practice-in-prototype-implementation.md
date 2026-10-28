# Plan wykonania praktyk w prototypie

> **Dla wykonawcy:** uĹĽyj `superpowers:executing-plans`, wykonuj zadania po kolei, zapisuj dowody REDâ†’GREEN i postÄ™p. Realizacja bezpoĹ›rednio w tej sesji; jeden koĹ„cowy niezaleĹĽny przeglÄ…d.

**Cel:** dziaĹ‚ajÄ…cy wariant wspĂłlnej spoĹ‚ecznoĹ›ci w oknie, z uczeniem, przekazem, konserwacjÄ…, obsĹ‚ugÄ… gracza i kompletnym zapisem.

**Spec:** [projekt przyrostu](practice-in-prototype.md); kolejnoĹ›Ä‡ ustalona w [jednym planie](../prototype-plan.md). Polecenie uĹĽytkownika â€žKontynuujâ€ť rozpoczyna wykonanie pierwszego przyrostu, nie automatyczny odbiĂłr gry i nie implementacjÄ™ caĹ‚ego M05/M06 w jednym kroku.

**Architektura:** opcjonalny, domenowy `PracticeState` naleĹĽy do jednej `TraversalSession`. Nie wstawiamy obok drugiego `ContrastSession`. CzystÄ… matematykÄ™/aparat wydzielamy na poziomie targetĂłw CMake, aby rdzeĹ„ przestrzenny mĂłgĹ‚ uĹĽyÄ‡ istniejÄ…cego agenta bez cyklu zaleĹĽnoĹ›ci. Nowy stan otrzymuje format `GENESIS_TRAVERSAL 8`; stare sceny zachowujÄ… stare bajty i odczyt1â€“7.

**Stos:** istniejÄ…ce C++20/MSVC, Win32/D3D11 i Python stdlib, bez nowych zaleĹĽnoĹ›ci. Grafika bryĹ‚ i sterowanie pozostajÄ… dotychczasowe.

## Global Constraints

- Jeden zegar50 ms, jedna geometria, WorkWorld jako wĹ‚aĹ›ciciel materiaĹ‚Ăłw/pracy; ĹĽadnego drugiego Ĺ›wiata badawczego w aplikacji.
- UrzÄ…dzenie1, stanowisko konserwacji2 w wariancie spoĹ‚ecznoĹ›ci; staĹ‚e prawo `cap24â†’loss4â†’cap16`, wymĂłg16â†’8. To parametry sceny, nie odpowiedĹş w planerze. Testy takĹĽe innych praw.
- Osiem osĂłb; role automatycznego uczenia dla dotychczasowego konserwatora3 i mieszkaĹ„ca5, wiÄ…zane po EntityId. DopuĹ›ciÄ‡ te same rÄ™czne czynnoĹ›ci pozostaĹ‚ym postaciom. Student ma lokalnie dostÄ™pne uprawnienie do czÄ™Ĺ›ci i znajomoĹ›Ä‡ stanowiska w nowej scenie; stara scena M04 bez zmiany.
- Jedna operacja urzÄ…dzenia naraz. Maksymalnie8 wĹ‚asnych pomiarĂłw na osobÄ™,1024 eu,1â€“128; brak sukcesu lub energii daje odmowÄ™. Pomiary jednego u dla uczenia, konfiguracje z u dĹ‚ugoĹ›ci1â€“3 dla pracy. Zapas8192 eu bez doĹ‚adowaĹ„.
- Poprawny impuls uprawnia tego wykonawcÄ™ do jednej rzeczywistej konserwacji; zuĹĽyÄ‡ zgodÄ™ dopiero przy przyjÄ™ciu pracy. CzÄ™Ĺ›Ä‡ zuĹĽywa WorkWorld, czas80 krokĂłw; dawne stanowiska bez urzÄ…dzenia nie majÄ… nowego warunku.
- Kalibracja wynika wyĹ‚Ä…cznie z zakoĹ„czonego impulsu. Metoda z modeli, ktĂłre zgodnie przewidujÄ… cel; brak adekwatnego modelu nie tworzy recepty.
- Zapis obejmuje fazy, zamiary, peĹ‚ny stan umysĹ‚Ăłw, koszty, urzÄ…dzenie i komunikacjÄ™. Nie przenosiÄ‡ limitu10000 krokĂłw badania jako horyzontu gry. ZgodnoĹ›Ä‡ historyczna i F9 obcego Ĺ›wiata wymagane.
- Impuls/konfiguracja oraz rozpoczÄ™ta konserwacja chroniÄ… wykonawcÄ™. RÄ™czne polecenie moĹĽe zostaÄ‡ odroczone; wiedza i materiaĹ‚ nie znikajÄ…. GĹ‚Ăłd/odpoczynek wracajÄ… do istniejÄ…cych reguĹ‚ na bezpiecznej granicy.
- Przekaz trwa jeden krok i wymaga utrzymanego kontaktuâ‰¤2 m na wspĂłlnej podporze, bez przeszkĂłd; zachowaÄ‡ ĹşrĂłdĹ‚o i observed_tick, via/learned_tick z faktycznego kontaktu. Pierwszy zakres przekazuje bezpoĹ›rednie wĹ‚asne obserwacje Ĺ›wiadka, bez wymyĹ›lania szerszej sieci nauczania.
- UI nie zna prawdy urzÄ…dzenia. WĹ‚asne wiadomoĹ›ci i historia prĂłb sÄ… czytelne; matematyka i seedy nie sÄ… normalnym przepĹ‚ywem gry. Badania I/A/T pozostajÄ… niezmienione.

## Review Focus

Ominiecie kalibracji przez rÄ™cznÄ…/autonomicznÄ… drogÄ™; wczytanie8â†’stary Ĺ›wiat bez aparatu; obca wiedza/duplikaty identyfikatorĂłw; gĹ‚Ăłd, F2, WASD, cel i komunikat podczas impulsu/pracy; nieudana rezerwacja czÄ™Ĺ›ci po kalibracji; jednoczesne ĹĽÄ…dania dwĂłch operatorĂłw; pochodzenie wiadomoĹ›ci po ruchu; koszt konstrukcji modeli poza aktywnÄ… klatkÄ…; ĹĽycie Ĺ›wiata po10000 krokĂłw bez sztucznej awarii. KaĹĽdy przypadek naleĹĽy do testĂłw poniĹĽej.

### Task 1: ZaleĹĽnoĹ›ci i domenowy stan urzÄ…dzenia

**Files:** CMakeLists.txt; nowe `src/lab/traversal_practice.hpp/.cpp`; `traversal_session.hpp/.cpp`, `traversal_needs.cpp`, `traversal_passages.cpp`; `tests/practice_tests.cpp`.

**Interfaces:** `PracticeSeed` (ID/wersja, stanowisko, prawo, wejĹ›cie/cel, ID rĂłl po konstrukcji sceny), `PracticeState` (aparat, umysĹ‚y, operacje, zgoda i kolejka), `PracticeView`; publiczne `has_practice()`, `practice_view(actor)`, `practice_probe(actor,chain,input)`, `share_practice(sender,recipient)` w TraversalSession. Komendy zwracajÄ… `DiscoveryOutcome` jako przyjÄ™cie/odmowÄ™, nie fikcyjne ukoĹ„czenie.

- [x] RED: nowy test `practice.calibration` wymaga prawdziwego impulsu przed rozpoczÄ™ciem obu drĂłg konserwacji; zĹ‚y wynik16â†’12 nie zuĹĽywa czÄ™Ĺ›ci, poprawne `[u,u]`16â†’8 zuĹĽywa jednÄ… i koĹ„czy po80 krokach. Najpierw uruchomiÄ‡ test przy brakujÄ…cej funkcji, zakoĹ„czyÄ‡ RED przed kodem.
- [x] WydzieliÄ‡ `reality_learning_core` z istniejÄ…cych discovery_world/agent, hypothesis_program i contrast_agent; zaleĹĽnoĹ›Ä‡ tylko od reality_core. Lab_model korzysta z niego; dotychczasowe research targety nie kompilujÄ… tych samych definicji ponownie. Algorytmy badaĹ„ bez zmiany.
- [x] ZaimplementowaÄ‡ opcjonalny stan w TraversalSession. PrzyjÄ™cie i wynik impulsu przez ten sam zegar, jeden operator, jedna zgoda zwiÄ…zana z aktorem/pomiarem, wspĂłlna bramka w `start_station_action` i Ĺ›cieĹĽce rÄ™cznej. Chronione impulsy nie przesuwajÄ… aktora. Nie inicjalizowaÄ‡ umysĹ‚Ăłw w kaĹĽdej klatce.
- [x] GREEN: `practice.calibration`, `practice.energy`, `practice.concurrent`, `practice.no_parts`, `practice.out_of_reach`; potwierdziÄ‡ stary tor `community|passage|contrast` po zmianie zaleĹĽnoĹ›ci. Commit.

### Task 2: Zapis i semantyczne odtworzenie

**Files:** nowe `src/lab/traversal_practice_codec.cpp`; `traversal_codec.cpp`; testy C++ i nowa `tests/practice_integration.py`.

**Interfaces:** `PracticeState::serialize()` i odczyt odtwarzajÄ…cy kandydata z aparatem oraz rolami istniejÄ…cego Ĺ›wiata. Wersja8 zachowuje wszystkie wczeĹ›niejsze sekcje formatu7 i dodaje jeden jawny rekord praktyk przed checksumem. Przy braku praktyk stary wybĂłr wersji i bajty pozostajÄ… bez zmian.

- [x] RED: `practice.persistence` â€” zapis w konfiguracji, impulsie, oczekiwaniu i pracy po wczytaniu daje identyczny koĹ„cowy Ĺ›wiat. `practice.corruption` z poprawnym checksumem: obcy aktor/urzÄ…dzenie, faĹ‚szywy pomiar, zgoda zuĹĽyta albo wydana przez inny impuls, zĹ‚y stan ĹşrĂłdĹ‚a energii, koszt i model niezgodne z doĹ›wiadczeniem; nieudany odczyt zachowuje poprzedni Ĺ›wiat.
- [x] Dla umysĹ‚Ăłw zachowaÄ‡ ograniczony dziennik faktycznych wywoĹ‚aĹ„ wnioskowania i obserwacji. OdtworzyÄ‡ go z zapisanych rzeczywistych pomiarĂłw i porĂłwnaÄ‡ kanoniczny stan/liczniki. Nie zerowaÄ‡ kosztĂłw przez ponowne skonstruowanie agenta. DopasowaÄ‡ pamiÄ™Ä‡ do staĹ‚ego ID/wersji aparatu.
- [x] GREEN: stare formaty1â€“7,8â†’obca scena i odwrotnie, snapshot po10000 krokĂłw, Ĺ›wieĹĽy proces dla co najmniej czterech faz. Commit.

### Task 3: Celowa autonomia i kontakt

**Files:** `traversal_practice.cpp`, `traversal_needs.cpp`, `traversal_knowledge.cpp`, `traversal_clearance.cpp`, `traversal_session.cpp`, `traversal_course.cpp`, testy.

**Interfaces:** `make_learning_course()` tworzy wariant istniejÄ…cej spoĹ‚ecznoĹ›ci; wewnÄ™trzne `request_practice_work`, `advance_practice`, `practice_busy` Ĺ‚Ä…czÄ… aktualny zamiar z urzÄ…dzeniem. Role zapisane jako EntityId. Widok zwraca wĹ‚asne prĂłby, etap, poznanÄ… metodÄ™ i przyczynÄ™ oczekiwania.

- [x] RED: `practice.autonomy` â€” bez rÄ™cznych rozkazĂłw nauczyciel uczy siÄ™ z impulsĂłw i pracuje; `practice.teaching` â€” uczeĹ„ otrzymuje rzeczywiste dane i pĂłĹşniej sam wybiera konfiguracjÄ™; brak kontaktu nie daje wiedzy. Osobno sprawdziÄ‡ inne prawo, aby wykluczyÄ‡ sztywnÄ… receptÄ™.
- [x] DopuĹ›ciÄ‡ wczesne uĹĽycie metody, jeĹ›li zgodne modele przewidujÄ… cel. Nie wymuszaÄ‡ oĹ›miu prĂłb jak w badaniu. Puste modele/wyczerpanie budĹĽetu wyraĹşne. Po zweryfikowaniu celu zapamiÄ™taÄ‡ metodÄ™, bez ponownej peĹ‚nej analizy dla identycznej pracy; fizyczny impuls nadal jest konieczny dla kaĹĽdej nowej czynnoĹ›ci.
- [x] WspĂłlna kolejka ogranicza monopol operatora. Automatyczny kontakt przekazuje tylko nowe wĹ‚asne pomiary, przyjmuje dane po jednym kroku i nie przerywa chronionych dziaĹ‚aĹ„. Dalsza konserwacja wynika z rzeczywistego zuĹĽycia, nie nowej sztucznej awarii. Osoba nadal moĹĽe wykonaÄ‡ pracÄ™ bez nauczyciela, jeĹ›li nauczy siÄ™ samodzielnie.
- [x] GREEN: `practice.needs`, `practice.manual_override`, `practice.protected_message`, `practice.blocked_contact`, `practice.safe_queue`, dĹ‚uĹĽsza kontynuacja i stare regresje. Commit.

### Task 4: Widok, wejĹ›cie i launcher

**Files:** `src/platform/traversal_main.cpp`, `src/graphics/traversal_scene.cpp`, nowe `tools/start_practice_playtest.py`, natywna integracja okna.

**Interfaces:** `--community --learning`, istniejÄ…ce F5/F9 i R/F7; I zmienia rÄ™czne wejĹ›cie, O wybiera konfiguracjÄ™, P prĂłbuje. Nie zajmowaÄ‡ Q/E, ktĂłre juĹĽ obracajÄ… kamerÄ™. Dotychczasowe tryby bez zmian. Nowy launcher podaje komendÄ™ i osobny katalog dowodĂłw/zapisu.

- [x] RED: natywny test rzeczywistego okna wymaga wĹ‚Ä…czenia flagi, obrazu urzÄ…dzenia i komunikatĂłw uczenia/pracy, aktywnego F5/F9 i poprawnego obcego Ĺ›wiata.
- [x] PokazaÄ‡ aparat przy istniejÄ…cym stanowisku, wĹ‚asne ostatnie prĂłby i wyraĹşny etap. KrĂłtkie zdarzenia pozostajÄ… w historii. UI tĹ‚umaczy przyjÄ™cie/odmowÄ™ polecenia, nie deklaruje sukcesu przed domenÄ…. ZwykĹ‚e interakcje wymagajÄ… dojĹ›cia; inspektor nie przekazuje prawdy do decyzji.
- [x] GREEN: natywny GPU/input, przejÄ™cie i ruch, resize/minimalizacja/powrĂłt, brak aparatu po F9 nie powoduje awarii. Commit.

### Task 5: KoĹ„cowe domkniÄ™cie i prĂłba uĹĽytkownika

**Files:** nowy raport `docs/verification/practice-in-prototype.md`, aktualne API/autonomia/rozwĂłj/README/mapa; dowody i launcher.

- [ ] ZmierzyÄ‡ rzeczywistÄ… pracÄ™ wnioskowania w oknie Release na wskazanym adapterze; sprawdziÄ‡ postÄ™p potrzeb/pracy, przyciÄ™cia i brak blokowania klatki. Przed prĂłbÄ… zamroziÄ‡ scenÄ™, ustawienia i czas; nie nazywaÄ‡ kosztu headless pomiarem okna.
- [ ] NiezaleĹĽny koĹ„cowy review, poprawki Important/Critical REDâ†’GREEN, peĹ‚ne Debug/Release/ASan, Python, docs i format. ZachowaÄ‡ wczeĹ›niejsze poraĹĽki, stare formaty i wszystkie wyniki badaĹ„.
- [ ] ZachowaÄ‡ checkpointy, hashe i powtarzalny scenariusz; udostÄ™pniÄ‡ dziaĹ‚ajÄ…ce okno oraz krĂłtkÄ… instrukcjÄ™ uĹĽytkownikowi.
- [ ] Osobno zapisaÄ‡ bezpoĹ›redniÄ… ocenÄ™ uĹĽytkownika czytelnoĹ›ci, sterowania i widocznego skutku. Jej brak nie staje siÄ™ automatycznym zaliczeniem przyrostu. Dopiero po odbiorze przejĹ›Ä‡ do nastÄ™pnych brakĂłw M05.

Plan nie obiecuje human acceptance ani przewagi nauczania. Implementacja ma dostarczyÄ‡ dziaĹ‚ajÄ…cÄ… moĹĽliwoĹ›Ä‡, dowody oraz konkretny rezultat do sprawdzenia przez uĹĽytkownika.
