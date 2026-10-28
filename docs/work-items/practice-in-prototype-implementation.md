# Plan wykonania praktyk w prototypie

> **Dla wykonawcy:** użyj `superpowers:executing-plans`, wykonuj zadania po kolei, zapisuj dowody RED→GREEN i postęp. Realizacja bezpośrednio w tej sesji; jeden końcowy niezależny przegląd.

**Cel:** działający wariant wspólnej społeczności w oknie, z uczeniem, przekazem, konserwacją, obsługą gracza i kompletnym zapisem.

**Spec:** [projekt przyrostu](practice-in-prototype.md); kolejność ustalona w [jednym planie](../prototype-plan.md). Polecenie użytkownika „Kontynuuj” rozpoczyna wykonanie pierwszego przyrostu, nie automatyczny odbiór gry i nie implementację całego M05/M06 w jednym kroku.

**Architektura:** opcjonalny, domenowy `PracticeState` należy do jednej `TraversalSession`. Nie wstawiamy obok drugiego `ContrastSession`. Czystą matematykę/aparat wydzielamy na poziomie targetów CMake, aby rdzeń przestrzenny mógł użyć istniejącego agenta bez cyklu zależności. Nowy stan otrzymuje format `GENESIS_TRAVERSAL 8`; stare sceny zachowują stare bajty i odczyt1–7.

**Stos:** istniejące C++20/MSVC, Win32/D3D11 i Python stdlib, bez nowych zależności. Grafika brył i sterowanie pozostają dotychczasowe.

## Global Constraints

- Jeden zegar50 ms, jedna geometria, WorkWorld jako właściciel materiałów/pracy; żadnego drugiego świata badawczego w aplikacji.
- Urządzenie1, stanowisko konserwacji2 w wariancie społeczności; stałe prawo `cap24→loss4→cap16`, wymóg16→8. To parametry sceny, nie odpowiedź w planerze. Testy także innych praw.
- Osiem osób; role automatycznego uczenia dla dotychczasowego konserwatora3 i mieszkańca5, wiązane po EntityId. Dopuścić te same ręczne czynności pozostałym postaciom. Student ma lokalnie dostępne uprawnienie do części i znajomość stanowiska w nowej scenie; stara scena M04 bez zmiany.
- Jedna operacja urządzenia naraz. Maksymalnie8 własnych pomiarów na osobę,1024 eu,1–128; brak sukcesu lub energii daje odmowę. Pomiary jednego u dla uczenia, konfiguracje z u długości1–3 dla pracy. Zapas8192 eu bez doładowań.
- Poprawny impuls uprawnia tego wykonawcę do jednej rzeczywistej konserwacji; zużyć zgodę dopiero przy przyjęciu pracy. Część zużywa WorkWorld, czas80 kroków; dawne stanowiska bez urządzenia nie mają nowego warunku.
- Kalibracja wynika wyłącznie z zakończonego impulsu. Metoda z modeli, które zgodnie przewidują cel; brak adekwatnego modelu nie tworzy recepty.
- Zapis obejmuje fazy, zamiary, pełny stan umysłów, koszty, urządzenie i komunikację. Nie przenosić limitu10000 kroków badania jako horyzontu gry. Zgodność historyczna i F9 obcego świata wymagane.
- Impuls/konfiguracja oraz rozpoczęta konserwacja chronią wykonawcę. Ręczne polecenie może zostać odroczone; wiedza i materiał nie znikają. Głód/odpoczynek wracają do istniejących reguł na bezpiecznej granicy.
- Przekaz trwa jeden krok i wymaga utrzymanego kontaktu≤2 m na wspólnej podporze, bez przeszkód; zachować źródło i observed_tick, via/learned_tick z faktycznego kontaktu. Pierwszy zakres przekazuje bezpośrednie własne obserwacje świadka, bez wymyślania szerszej sieci nauczania.
- UI nie zna prawdy urządzenia. Własne wiadomości i historia prób są czytelne; matematyka i seedy nie są normalnym przepływem gry. Badania I/A/T pozostają niezmienione.

## Review Focus

Ominiecie kalibracji przez ręczną/autonomiczną drogę; wczytanie8→stary świat bez aparatu; obca wiedza/duplikaty identyfikatorów; głód, F2, WASD, cel i komunikat podczas impulsu/pracy; nieudana rezerwacja części po kalibracji; jednoczesne żądania dwóch operatorów; pochodzenie wiadomości po ruchu; koszt konstrukcji modeli poza aktywną klatką; życie świata po10000 kroków bez sztucznej awarii. Każdy przypadek należy do testów poniżej.

### Task 1: Zależności i domenowy stan urządzenia

**Files:** CMakeLists.txt; nowe `src/lab/traversal_practice.hpp/.cpp`; `traversal_session.hpp/.cpp`, `traversal_needs.cpp`, `traversal_passages.cpp`; `tests/practice_tests.cpp`.

**Interfaces:** `PracticeSeed` (ID/wersja, stanowisko, prawo, wejście/cel, ID ról po konstrukcji sceny), `PracticeState` (aparat, umysły, operacje, zgoda i kolejka), `PracticeView`; publiczne `has_practice()`, `practice_view(actor)`, `practice_probe(actor,chain,input)`, `share_practice(sender,recipient)` w TraversalSession. Komendy zwracają `DiscoveryOutcome` jako przyjęcie/odmowę, nie fikcyjne ukończenie.

- [ ] RED: nowy test `practice.calibration` wymaga prawdziwego impulsu przed rozpoczęciem obu dróg konserwacji; zły wynik16→12 nie zużywa części, poprawne `[u,u]`16→8 zużywa jedną i kończy po80 krokach. Najpierw uruchomić test przy brakującej funkcji, zakończyć RED przed kodem.
- [ ] Wydzielić `reality_learning_core` z istniejących discovery_world/agent, hypothesis_program i contrast_agent; zależność tylko od reality_core. Lab_model korzysta z niego; dotychczasowe research targety nie kompilują tych samych definicji ponownie. Algorytmy badań bez zmiany.
- [ ] Zaimplementować opcjonalny stan w TraversalSession. Przyjęcie i wynik impulsu przez ten sam zegar, jeden operator, jedna zgoda związana z aktorem/pomiarem, wspólna bramka w `start_station_action` i ścieżce ręcznej. Chronione impulsy nie przesuwają aktora. Nie inicjalizować umysłów w każdej klatce.
- [ ] GREEN: `practice.calibration`, `practice.energy`, `practice.concurrent`, `practice.no_parts`, `practice.out_of_reach`; potwierdzić stary tor `community|passage|contrast` po zmianie zależności. Commit.

### Task 2: Zapis i semantyczne odtworzenie

**Files:** nowe `src/lab/traversal_practice_codec.cpp`; `traversal_codec.cpp`; testy C++ i nowa `tests/practice_integration.py`.

**Interfaces:** `PracticeState::serialize()` i odczyt odtwarzający kandydata z aparatem oraz rolami istniejącego świata. Wersja8 zachowuje wszystkie wcześniejsze sekcje formatu7 i dodaje jeden jawny rekord praktyk przed checksumem. Przy braku praktyk stary wybór wersji i bajty pozostają bez zmian.

- [ ] RED: `practice.persistence` — zapis w konfiguracji, impulsie, oczekiwaniu i pracy po wczytaniu daje identyczny końcowy świat. `practice.corruption` z poprawnym checksumem: obcy aktor/urządzenie, fałszywy pomiar, zgoda zużyta albo wydana przez inny impuls, zły stan źródła energii, koszt i model niezgodne z doświadczeniem; nieudany odczyt zachowuje poprzedni świat.
- [ ] Dla umysłów zachować ograniczony dziennik faktycznych wywołań wnioskowania i obserwacji. Odtworzyć go z zapisanych rzeczywistych pomiarów i porównać kanoniczny stan/liczniki. Nie zerować kosztów przez ponowne skonstruowanie agenta. Dopasować pamięć do stałego ID/wersji aparatu.
- [ ] GREEN: stare formaty1–7,8→obca scena i odwrotnie, snapshot po10000 kroków, świeży proces dla co najmniej czterech faz. Commit.

### Task 3: Celowa autonomia i kontakt

**Files:** `traversal_practice.cpp`, `traversal_needs.cpp`, `traversal_knowledge.cpp`, `traversal_clearance.cpp`, `traversal_session.cpp`, `traversal_course.cpp`, testy.

**Interfaces:** `make_learning_course()` tworzy wariant istniejącej społeczności; wewnętrzne `request_practice_work`, `advance_practice`, `practice_busy` łączą aktualny zamiar z urządzeniem. Role zapisane jako EntityId. Widok zwraca własne próby, etap, poznaną metodę i przyczynę oczekiwania.

- [ ] RED: `practice.autonomy` — bez ręcznych rozkazów nauczyciel uczy się z impulsów i pracuje; `practice.teaching` — uczeń otrzymuje rzeczywiste dane i później sam wybiera konfigurację; brak kontaktu nie daje wiedzy. Osobno sprawdzić inne prawo, aby wykluczyć sztywną receptę.
- [ ] Dopuścić wczesne użycie metody, jeśli zgodne modele przewidują cel. Nie wymuszać ośmiu prób jak w badaniu. Puste modele/wyczerpanie budżetu wyraźne. Po zweryfikowaniu celu zapamiętać metodę, bez ponownej pełnej analizy dla identycznej pracy; fizyczny impuls nadal jest konieczny dla każdej nowej czynności.
- [ ] Wspólna kolejka ogranicza monopol operatora. Automatyczny kontakt przekazuje tylko nowe własne pomiary, przyjmuje dane po jednym kroku i nie przerywa chronionych działań. Dalsza konserwacja wynika z rzeczywistego zużycia, nie nowej sztucznej awarii. Osoba nadal może wykonać pracę bez nauczyciela, jeśli nauczy się samodzielnie.
- [ ] GREEN: `practice.needs`, `practice.manual_override`, `practice.protected_message`, `practice.blocked_contact`, `practice.safe_queue`, dłuższa kontynuacja i stare regresje. Commit.

### Task 4: Widok, wejście i launcher

**Files:** `src/platform/traversal_main.cpp`, `src/graphics/traversal_scene.cpp`, nowe `tools/start_practice_playtest.py`, natywna integracja okna.

**Interfaces:** `--community --learning`, istniejące F5/F9 i R/F7; I zmienia ręczne wejście, O wybiera konfigurację, P próbuje. Nie zajmować Q/E, które już obracają kamerę. Dotychczasowe tryby bez zmian. Nowy launcher podaje komendę i osobny katalog dowodów/zapisu.

- [ ] RED: natywny test rzeczywistego okna wymaga włączenia flagi, obrazu urządzenia i komunikatów uczenia/pracy, aktywnego F5/F9 i poprawnego obcego świata.
- [ ] Pokazać aparat przy istniejącym stanowisku, własne ostatnie próby i wyraźny etap. Krótkie zdarzenia pozostają w historii. UI tłumaczy przyjęcie/odmowę polecenia, nie deklaruje sukcesu przed domeną. Zwykłe interakcje wymagają dojścia; inspektor nie przekazuje prawdy do decyzji.
- [ ] GREEN: natywny GPU/input, przejęcie i ruch, resize/minimalizacja/powrót, brak aparatu po F9 nie powoduje awarii. Commit.

### Task 5: Końcowe domknięcie i próba użytkownika

**Files:** nowy raport `docs/verification/practice-in-prototype.md`, aktualne API/autonomia/rozwój/README/mapa; dowody i launcher.

- [ ] Zmierzyć rzeczywistą pracę wnioskowania w oknie Release na wskazanym adapterze; sprawdzić postęp potrzeb/pracy, przycięcia i brak blokowania klatki. Przed próbą zamrozić scenę, ustawienia i czas; nie nazywać kosztu headless pomiarem okna.
- [ ] Niezależny końcowy review, poprawki Important/Critical RED→GREEN, pełne Debug/Release/ASan, Python, docs i format. Zachować wcześniejsze porażki, stare formaty i wszystkie wyniki badań.
- [ ] Zachować checkpointy, hashe i powtarzalny scenariusz; udostępnić działające okno oraz krótką instrukcję użytkownikowi.
- [ ] Osobno zapisać bezpośrednią ocenę użytkownika czytelności, sterowania i widocznego skutku. Jej brak nie staje się automatycznym zaliczeniem przyrostu. Dopiero po odbiorze przejść do następnych braków M05.

Plan nie obiecuje human acceptance ani przewagi nauczania. Implementacja ma dostarczyć działającą możliwość, dowody oraz konkretny rezultat do sprawdzenia przez użytkownika.
