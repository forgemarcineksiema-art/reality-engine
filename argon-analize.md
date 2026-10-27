# Raport Architektoniczny i Strategiczny: Projekt GENESIS / Reality Engine

**Rola:** Chief Systems Architect & Head of R&D / Warstwa Strategiczna  
**Zakres badania:** Pełna dokumentacja normatywna, projektowa, weryfikacyjna i archiwalna (66 zarejestrowanych dokumentów + archiwum) oraz aktualny kod źródłowy C++20 (`reality_core`, `reality_lab_model`, `reality_graphics`, suity testowe CTest).  
**Szczególny fokus:** Trwająca implementacja transferu zasobów (`WorkWorld` v3) oraz fizycznej dostawy (`TraversalSession` v4).

---

## 0. Streszczenie Wykonawcze (Fakty vs. Założenia)

### Fakty (Potwierdzony stan repozytorium)
1. **Rygor inżynierski należy do ścisłej czołówki prototypów systemowych:** Kod w C++20 kompilowany z `/W4 /WX`, bez zewnętrznych zależności runtime, z deterministycznym zegarem (`SimulationClock` 50 ms), atomowym zapisem Win32 (`CreateFileW` + `FlushFileBuffers` + `MoveFileExW`), sumami kontrolnymi FNV-1a, profilami AddressSanitizer oraz testami na fizycznym GPU (NVIDIA MX330 z warstwą debug D3D11).
2. **Transfer w modelu domenowym (`WorkWorld`) jest już w kodzie zaawansowany:** Pliki `src/simulation/material_transfer.cpp`, `src/simulation/work_world.hpp/.cpp`, `src/persistence/world_snapshot.cpp` oraz `tests/transfer_tests.cpp` implementują `MaterialTypeId`, transakcyjny `WorkWorld::transfer()`, format `GENESIS_WORK_WORLD 3` oraz chronologiczny audyt przepływów w `validate_material_flow()`.
3. **Warstwa fizycznej dostawy (`src/lab/traversal_delivery.cpp` i `tests/delivery_tests.cpp`) jest w trakcie domykana:** Posiada już model ładunku jako rzeczywistej partii `WorkWorld` (`actor.cargo`), maszynę stanów kursu (`ToPickup`, `ToDestination`, `Returning`, `Completed`, `Cancelled`), format `GENESIS_TRAVERSAL 4` oraz obsługę klawisza `F4`. W kodzie wykryłem jednak **3 konkretne luki w walidacji przestrzennej i niezmiennikach**, które odpowiadają najnowszym przypadkom w `tests/delivery_tests.cpp`, oraz **1 lukę semantyczną po anulowaniu dostawy**.
4. **Dokumentacja kanoniczna nie została jeszcze zsynchronizowana z kodem transferu:** `docs/work-domain.md`, `docs/traversal-autonomy.md`, `docs/subsystem-contracts.md`, `docs/testing-strategy.md` oraz `docs/development.md` opisują jeszcze stan sprzed `WorkWorld` v3 i `Traversal` v4.

---

## CZĘŚĆ I: Recenzja i Audyt Bieżącej Implementacji Transferu i Dostawy Zasobów

Jako Chief Systems Architect przeanalizowałem dokładnie Twój bieżący kod w `src/simulation/material_transfer.cpp`, `src/simulation/work_world.cpp`, `src/persistence/world_snapshot.cpp`, `src/lab/traversal_delivery.cpp` oraz `src/lab/traversal_session.cpp`.

### 1.1. Co zaprojektowano wzorcowo (Architektoniczne strzały w dziesiątkę)

1. **Ładunek (`actor.cargo`) jako pierwszoklasowa partia `WorkWorld`, a nie osobny licznik (`INV-02`, `INV-06`):**
   - Zamiast dodawać `uint64_t carried_food` do `TraversalActor`, utworzyłeś dla kuriera rzeczywistą partię w `WorkWorld` (`initial = 0`, `owner = actor.id`, `kind = Material`, `material = from->record.material`).
   - **Dlaczego to kluczowe:** Eliminuje to całą klasę błędów „materiału w próżni” podczas podróży, zapisu gry, przerwania przez głód czy jazdy promem. Suma zasobów w `WorkWorld` jest niezmiennikiem w każdym podkroku.
2. **Chronologiczny replay dziennika w `validate_material_flow()` zamiast naiwnych sum `uint64_t`:**
   - W `material_transfer.cpp:70-189` walidator odtwarza stan partii (`amount`, `held`, `active`) wzdłuż osi `events_`.
   - Gdybyś sumował historyczne wpływy i odpływy do licznika `uint64_t`, wielokrotny transfer dużej ilości pomiędzy dwiema partiami spowodowałby przepełnienie (`uint64_t` overflow) przy całkowicie legalnym stanie bieżącym. Twój test `large_cycle` w `transfer_tests.cpp` dowodzi odporności tego algorytmu na 100 obiegów `UINT64_MAX`.
3. **Wspólna przestrzeń `CommandId` i silna gwarancja wyjątków (Strong Exception Guarantee):**
   - `reserve()` i `transfer()` dzielą przestrzeń identyfikatorów komend; próba użycia ID rezerwacji w transferze (lub odwrotnie) albo zmiana payloadu rzuca `std::invalid_argument` przed jakąkolwiek mutacją.
   - Wywołanie `prepare_records(transfers_, 1)` i `prepare_records(events_, 2)` przed odjęciem i dodaniem ilości (weryfikowane przez przeciążony `operator new` w teście `allocation`) gwarantuje atomowość w pamięci.

---

### 1.2. Wykryte defekty i luki w bieżącym kodzie dostawy (Do natychmiastowej naprawy)

Analiza porównawcza `tests/delivery_tests.cpp` z `src/lab/traversal_delivery.cpp` i `src/lab/traversal_session.cpp` ujawnia **4 konkretne problemy**:

#### Problem A (Krytyczny — Test `delivery.wall`): Przeładunek przez litą ścianę
- **Lokalizacja:** `src/lab/traversal_delivery.cpp:65-68` (`TraversalSession::interact_cargo`) oraz `arrive_delivery` (`linia 153-156`).
- **Co jest w kodzie:**
  ```cpp
  const auto& station = scene_.stations[station_index];
  const auto point = surface_feet(geometry_, station.location, TraversalLimits{actor.size});
  if (!point || length(*point - actor.feet) > 1.1)
      return CargoStatus::OutOfReach;
  ```
- **Diagnoza:** Warunek sprawdza wyłącznie odległość euklidesową 3D (`<= 1.1 m`) oraz czy na samej pozycji stanowiska mieści się kapsuła (`surface_feet`). W teście `wall()` aktor stoi w `x = -0.4`, stanowisko jest w `x = +0.4` (odległość `0.8 m <= 1.1 m`), a między nimi w `x = 0` stoi nieprzekraczalna ściana o wysokości `2.0 m` i półgrubości `0.05 m`. Kapsuła o promieniu `0.35 m` mieści się w `x = -0.4` i w `x = +0.4`, więc `interact_cargo` **przepuszcza transfer przez ścianę**!
- **Rekomendacja naprawy:** Punkt stanowiska (`station_point = geometry_.body(station.location.surface).world(...)`) oraz `actor.feet` muszą leżeć na wspólnym, nieprzeciętym geometrią odcinku. Należy sprawdzić ciągłość podparcia i brak kolizji odcinka między `actor.feet` a stanowiskiem:
  - np. `continuous_surface_feet(geometry_, station.location.surface, actor.feet, station.location.local, TraversalLimits{actor.size})` oraz `spatial_segment_clear(geometry_, actor.feet, *station_feet, actor.size)` (z odpowiednio dobranym promieniem lub sweepem kapsuły od `actor.feet` w stronę stanowiska).

#### Problem B (Krytyczny — Test `delivery.wall_approach`): Wybór punktu podejścia po niewłaściwej stronie ściany
- **Lokalizacja:** `src/lab/traversal_session.cpp:671-715` (`TraversalSession::choose_approach`).
- **Co jest w kodzie:** `choose_approach` generuje 12 kandydatów na okręgu `reach * 0.8` wokół `destination`. Dla `walled(0.35)` stanowisko jest w `x = +0.35`, a ściana w `x = 0` (`[-0.05, +0.05]`). Kandydat pod kątem $-\pi/2$ ma `x = 0.35 - 0.72 = -0.37` (po lewej stronie ściany, tam gdzie startuje aktor!). Ponieważ `surface_feet` w `x = -0.37` jest poprawne, a trasa z `x = -0.4` do `x = -0.37` nie przecina ściany, `choose_approach` wybiera punkt po lewej stronie ściany jako poprawne podejście do stanowiska po prawej stronie ściany!
- **Rekomendacja naprawy:** W `choose_approach` każdy kandydat `candidate` (o współrzędnych świata `*feet`) musi mieć również bezpośrednią, nieprzeciętą przeszkodą łączność z punktem docelowym `destination` na tej powierzchni (nie może ich dzielić ściana).

#### Problem C (Krytyczny — Test `delivery.aliased_lots`): Brak walidacji unikalności `station.lot` po odczycie
- **Lokalizacja:** `src/lab/traversal_session.cpp:1764-1783` (`TraversalSession::validate`) oraz `src/lab/traversal_delivery.cpp:182` (`validate_delivery`).
- **Co jest w kodzie:** `validate_delivery()` sprawdza jedynie `station.lot != actor.cargo`. W `TraversalSession::validate()` sprawdzane jest `world_.resource(station.lot).has_value()`, ale **nie jest sprawdzane, czy dwa różne stanowiska w `scene_.stations` nie wskazują tego samego `EntityId lot`**!
- **Diagnoza:** W teście `aliased_lots()` zmanipulowany snapshot ustawia `stations[1].lot = stations[0].lot`. Taki zapis przechodzi `restore()` bez błędu, co pozwala sfałszować odbiór zasobu i łamie `INV-02` / `INV-06`.
- **Rekomendacja naprawy:** W `TraversalSession::validate()` dodać `std::set<EntityId> station_lots;` i wymagać `station_lots.insert(station.lot).second`, a także sprawdzić, że `station.lot` ma właściwy `ResourceKind` (`Capacity` dla `Rest`, `Material` dla `Eat`/`Maintain`).

#### Problem D (Architektoniczny / Semantyczny): „Uwięziony ładunek” po `cancel_delivery` u autonomicznego NPC
- **Lokalizacja:** `src/lab/traversal_delivery.cpp:35-40` oraz `46-58`.
- **Co się dzieje:**
  1. Kurier pobiera 4 jednostki materiału (`cargo` ma `quantity = 4`).
  2. Następuje `cancel_delivery(index)`. Zgodnie z kontraktem ładunek nie teleportuje z powrotem do źródła — zostaje w `actor.cargo` (`quantity = 4`), co chroni `INV-06` (i jest sprawdzane w `local_and_cancel`).
  3. Jednak jeśli potem wywołasz `assign_delivery(index, ...)`, linia 37 rzuca wyjątek:
     ```cpp
     check(bag && bag->record.quantity.value == 0 && bag->record.material == from->record.material,
           "delivery requires an empty compatible cargo lot");
     ```
  4. Co więcej, autonomiczny planer (`decide()` w `traversal_needs.cpp`) nie ma żadnej reguły co zrobić z niepustym `actor.cargo`, gdy `delivery` jest `Cancelled` lub `nullopt`! Jeśli gracz nie przejmie tej postaci ręcznie (`F2`) i nie opróżni jej torby (`F4`), postać nosi ładunek wiecznie i nie może dostać nowego zlecenia.
- **Rekomendacja architektoniczna:**
  - *Wariant 1 (Minimalny dla obecnego przyrostu):* Jeśli `assign_delivery` otrzymuje zlecenie dla tego samego `MaterialTypeId`, a w `actor.cargo` znajduje się już niesiony materiał tego samego typu, trzeba jawnie określić w kontrakcie: czy nowe zlecenie jest odrzucane do czasu rozładunku, czy też autonomiczny NPC bez aktywnego `delivery`, ale z niepustym `cargo`, powinien mieć prosty cel powrotu/oddania ładunku do znanego magazynu tego typu.
  - *Wariant 2 (Docelowy w M04):* Rozdzielić posiadanie przedmiotu w ekwipunku od kontraktu zlecenia (`Obligation`).

---

## CZĘŚĆ II: Badania, Architektura i Projektowanie Algorytmów (Chief Systems Architect & Head of R&D)

Poniżej przedstawiam głęboką analizę kompromisów technicznych obecnej architektury Reality Engine oraz specyfikacje algorytmiczne dla wyzwań, przed którymi zaraz staniesz.

### 2.1. Analiza złożoności i twardy mur `record_limit = 4096`

#### Fakty z kodu:
- W `src/core/bounded_records.hpp` (używanym przez `WorkWorld`) obowiązuje stały limit `record_limit = 4096` dla `actors_`, `resources_`, `actions_`, `commands_ + transfers_` oraz `events_`.
- Każda czynność (`Eat`, `Rest`, `Maintain`) tworzy: 1 `CommandRecord`, 1 `ActionRecord` i 3–4 `WorldEvent` (`Reserved`, `Started`, `[Consumed]`, `Completed`/`Cancelled`).
- Każdy kurs dostawy 12 jednostek (w partiach po 4) to 6 wywołań `transfer()`, czyli 6 `TransferRecord` i 12 `WorldEvent` (`TransferOut` + `TransferIn`).
- Przy 8–16 mieszkańcach wykonujących 1 czynność lub transfer co ~100 kroków (5 sekund), `events_` generuje ~5–10 zdarzeń na sekundę czasu rzeczywistego. **Po około 7–12 minutach ciągłej symulacji `events_.size()` osiągnie 4096 i `WorkWorld` rzuci `std::length_error("prototype record limit reached")`**, zatrzymując symulację!

#### Problem R&D: Jak pogodzić nieskończony czas działania świata z audytowalnością bilansu (`INV-06`)?
Obecnie `validate_material_flow()` weryfikuje świat od `tick = 0` (`EventKind::ResourceCreated`). Gdy wprowadzisz retencję dziennika (usuwanie starych zdarzeń i zakończonych czynności), obecny walidator straci punkt odniesienia `ResourceCreated`.

#### Specyfikacja rozwiązania (Epoch Checkpointing / Księga Główna Zasobów):
Zamiast przechowywać w pamięci RAM wszystkie zdarzenia od początku świata lub rezygnować z walidacji:
1. W `ResourceRecord` przechowuj **stan zatwierdzonego punktu kontrolnego (Checkpoint / Epoch)**:
   - `Quantity checkpoint_quantity` (zamiast samego `initial`),
   - `uint64_t checkpoint_event_sequence` (ostatni numer sekwencji zdarzenia włączony do punktu kontrolnego),
   - `Quantity total_consumed_checked`, `Quantity total_transferred_in_checked`, `Quantity total_transferred_out_checked` (jako statystyki lub liczniki epoki).
2. Operacja `compact_history(uint64_t up_to_tick)`:
   - Najpierw uruchamia pełne `validate()`.
   - Przesuwa `checkpoint_quantity` każdej partii przez wszystkie zdarzenia i zakończone akcje starsze niż `up_to_tick` (pod warunkiem, że akcja jest `Completed` lub `Cancelled`).
   - Usuwa z `events_`, `actions_`, `commands_` i `transfers_` wpisy zamknięte przed `up_to_tick` (lub przenosi je do zimnego pliku archiwum dziennika na dysku).
   - Zachowuje tablicę skrótów (lub okno ostatnich $K$ `CommandId`) dla idempotencji `reserve`/`transfer`.
3. Dzięki temu `validate()` działa w czasie $O(\text{aktywne okno})$ zamiast $O(\text{cała historia świata})$, pamięć procesu ma twardy górny pułap (zgodny z budżetem 512 MiB na laptopie 8 GB RAM), a żaden ułamek zasobu nie może zniknąć podczas kompakcji.

---

### 2.2. Ewolucja modelu zasobów: od pojedynczego `lot` do Transakcji Wielozasobowych i Produkcji

#### Ograniczenie obecnego modelu (`WorkWorld` v1–v3):
W `WorkRequest` mamy:
```cpp
struct WorkRequest {
    CommandId command;
    EntityId actor;
    EntityId lot; // <--- TYLKO JEDEN ZASÓB!
    Quantity required;
    Quantity consume_on_start;
    std::uint64_t duration_ticks = 0;
};
```
To wystarczało dla:
- Jedzenia (`1x Material` żywności),
- Odpoczynku (`1x Capacity` łóżka),
- Prostej konserwacji (`1x Material` naprawczy).

**Dlaczego to zablokuje pełne M04/M05?**
W docelowej scenie dwóch osad i mostu (warsztat kowalsko-ciesielski, naprawa przeprawy, urządzenie wodne):
1. Naprawa lub produkcja wymaga **jednocześnie**:
   - zajętości **stanowiska roboczego** (`ResourceKind::Capacity`),
   - zużycia **surowca/części** (`ResourceKind::Material`, np. drewno/żelazo),
   - obecności **narzędzia** (drugi zasób `Capacity` lub niezużywalny przedmiot).
2. Produkcja musi na końcu (`Completed`) **wytworzyć lub zasilić partię wyjściową** (`output_lot`, np. wyprodukowane części lub naprawiona konstrukcja) zgodnie z `INV-06` („Produkcja, zużycie oraz strata mają jawne wejścia, wyjścia i przyczynę”).

#### Projekt algorytmiczny (Multi-Resource Atomic Reservation & Recipe Contract):
Kiedy przejdziesz od dostawy do produkcji/naprawy, nie twórz osobnego „ProductionManagera” obok `WorkWorld`. Rozszerz kontrakt `WorkWorld` z zachowaniem pełnej kompatybilności wstecznej:
- **Atomowa rezerwacja wielozasobowa (All-or-Nothing):**
  - Komenda przyjmuje małą, uporządkowaną po `EntityId` (aby wykluczyć deadlocki i duplikaty) listę wymagań wejściowych (`inputs`: `lot`, `required`, `consume_on_start`, do np. 4 slotów) oraz opcjonalne wyjście (`output_lot`, `output_quantity`).
  - Walidacja sprawdza dostępność **wszystkich** partii wejściowych oraz brak przepełnienia `output_lot` *przed* utworzeniem jakiejkolwiek rezerwacji. Jeśli choć jednego składnika brakuje, cała komenda zwraca `InsufficientResource` bez częściowego blokowania pozostałych zasobów.

---

### 2.3. Problem „Rezerwacji w Drodze” vs. Lokalna Wiedza (`INV-04` vs. `INV-08`)

To jedno z najciekawszych zagadnień badawczych (R&D) w Twojej architekturze:
- Zgodnie z `INV-08` (Prawda i przekonania) postać **nie wie** na odległość, ile jednostek materiału zostało w magazynie, dopóki tam nie podejdzie.
- Dlatego w `TraversalSession` rezerwacja `world_.reserve()` oraz `world_.transfer()` następują **dopiero po fizycznym dojściu na miejsce (`distance <= 1.1 m`)**.
- **Konsekwencja emergentna:** Wyobraźmy sobie, że 4 głodnych mieszkańców widzi, iż są głodni, i wszyscy 4 ruszają przez kładkę i prom do tego samego stanowiska jedzenia, w którym została tylko 1 porcja. Pierwszy dochodzi i ją zjada; pozostałych 3 dowiaduje się o braku dopiero po długiej podróży. Podobnie kurier może iść na drugi koniec mapy po towar, który 2 sekundy przed jego przybyciem ktoś zużył.

#### Jak rozwiązać to bez łamania `INV-08` (bez „telepatii”)?
W rzeczywistym społeczeństwie problem ten rozwiązują **dwa jawne mechanizmy instytucjonalne**, które idealnie wpisują się w wizję GENESIS:
1. **Podział zapasu na „Publiczny (Wydawany)” i „Magazynowy / Kontraktowy (`Reserved Lot`)”:**
   - Magazyn źródłowy ma osobną partię przeznaczoną na eksport/dostawę (`MaterialTypeId = 1`, przypisaną do kontraktu dostawy), której zwykli mieszkańcy nie wyjadają jako przekąski, chyba że nastąpi kryzys lub kradzież (`INV-07`). W testach `make_delivery_course()` już intuicyjnie oddzieliłeś stację źródłową `stations[0]` (zapas 20) od pustej stacji odbiorczej `stations[1]` (zapas 0).
2. **Lokalna komunikacja i tablica/przekaz wiedzy (kolejny krok w M04):**
   - Gdy mieszkaniec A odbija się od pustego stanowiska (`InsufficientResource`), zapisuje w swojej pamięci przekonanie: `Belief{station_id, stock = 0, observed_at = tick}`.
   - Gdy mija po drodze mieszkańca B (w zasięgu np. 2–3 m) zmierzającego do tego samego stanowiska, może przekazać mu tę obserwację! Wtedy mieszkaniec B zawraca do alternatywnego źródła **z udokumentowanym kanałem informacji (`INV-08`)**, a nie dzięki globalnemu odczytowi `WorkWorld`!
   - To byłby pierwszy, czysty, mierzalny dowód działania osi **EVOLUTION / wiedzy** w połączeniu z logistyką **EARTH**.

---

### 2.4. Dekompozycja `TraversalSession` — Architektura Modułowa

#### Fakt:
Biblioteka `reality_lab_model` i klasa `TraversalSession` (`src/lab/traversal_session.hpp`) stały się faktycznym sercem symulacji przestrzennej. Struktura `TraversalActor` ma już **34 pola** (fizyka, kapsuła, trasa, promy, potrzeby, pamięć stacji, zawieszone zadania, protokół `Clearance`, `cargo` i `delivery`). Wszystkie metody są metodami jednej klasy `TraversalSession`, podzielonej jedynie fizycznie na pliki `.cpp` (`traversal_session.cpp`, `traversal_needs.cpp`, `traversal_clearance.cpp`, `traversal_delivery.cpp`, `traversal_codec.cpp`).

#### Ryzyko architektoniczne (`ENG-03`, `ENG-05`):
Dopóki jesteśmy w M03/wczesnym M04 (8–16 postaci), płaski `struct TraversalActor` jest czytelny i szybki. Jeśli jednak dołożymy do niego zaraz: przekonania (`Belief`), relacje, zobowiązania (`Obligation`), walkę (zdrowie, staminę, broń, stany ataku/parowania) oraz dialogi/historię, `TraversalSession` zamieni się w nieutrzymywalną „Boską Klasę” (God Object), a zapis `GENESIS_TRAVERSAL` w wersji 7 lub 8 stanie się koszmarem migracyjnym.

#### Rekomendacja (bez tworzenia pustych frameworków, zgodnie z `ENG-03`):
Nie buduj ogólnego ECS. Zamiast tego, po domknięciu obecnego przyrostu dostawy, wydziel w obrębie `TraversalActor` / sesji wyraźne, zagnieżdżone podstruktury wartościowe (Value Objects) odpowiadające domenom:
- `LocomotionComponent / BodyState` (pozycja, support, ruch, kapsuła),
- `NavigationAgentState` (cel, trasa, `leg`, `step_point`, `transfer`, `clearance`, `yielding`),
- `AutonomyNeedsState` (`hunger`, `energy`, `known_stations`, `station_retry`, `suspended`, `finish_grace_until`),
- `LogisticsState` (`cargo`, `delivery`).

W pliku zapisu (`traversal_codec.cpp`) każda z tych podstruktur ma już własny znacznik wiersza (`ACTOR`, `GOALS`, `MEMORY`, `AUTONOMY`, `CLEARANCE`, `YIELD`, `CARGO`, `DELIVERY`, `ROUTE`) — brakuje jedynie przeniesienia tego samego czystego podziału do struktury C++ i sygnatur funkcji pomocniczych, oraz **nazwania pól lub sekcji w `ACTOR`**, aby testy nie musiały modyfikować surowego indeksu kolumny nr 19 (`field(s.serialize(), "\nACTOR ", 19, "749")`).

---

## CZĘŚĆ III: Warstwa Strategiczna — Krytyka Założeń, Analiza Ryzyka i Porównanie

### 3.1. Porównanie z wcześniejszym audytem (`opinion-project-assessment.md` i `maxing.md`)

W repozytorium znajdują się pliki `opinion-project-assessment.md` oraz `maxing.md` (z zewnętrznej rozmowy o dokumentacji na etapie M01/M02). Jak wypada projekt dzisiaj w zderzeniu z tamtą oceną?

| Wymiar | Stan w czasie dawnej oceny (M01/M02) | Stan rzeczywisty dziś (Koniec M03 / Wejście w M04) | Moja ocena strategiczna i werdykt |
| :--- | :--- | :--- | :--- |
| **Dojrzałość technologii i weryfikacji** | Tylko zegar, `WorkWorld` v1 i początek okna D3D11; brak kodu do wglądu dla recenzenta (oceniono na 2/10). | Ponad 170 automatycznych testów CTest, pełny ASan, ciągła geometria pryzmatów i kapsuł, ruchome platformy, `WorkWorld` v3 z transferem, diagnostyka F6, pomiary 300 s na MX330 (~59,84 FPS, ~77 MB RAM). | **Ogromny skok jakościowy.** Jakość kodu, determinizm, obsługa błędów i dyscyplina dowodowa (`ENG-01`..`ENG-18`) są na poziomie wzorcowym. |
| **Architektura danych i przyczynowość** | Koncepcja jednego właściciela stanu i rezerwacji M01. | Udowodniona w praktyce: wielorazowa pojemność, fizyczny ładunek jako partia `WorkWorld`, brak zdalnego podglądania zapasu, transakcyjny odczyt. | **Fundament 9.5/10** w zdefiniowanym zakresie małej skali. |
| **Równowaga: Symulator Infrastruktury vs. Action RPG** | Ostrzeżenie, że projekt jest bardziej symulatorem przepompowni niż grą Action RPG. | Rozbudowano `game-design.md` i zatwierdzono kierunek fantasy (`DEC-008`), **ale w kodzie** 90% pary poszło w mikrozarządzanie ruchem kapsuł na wąskich kładkach i promach. | **Główne ryzyko strategiczne.** Technologia ruchu i logistyki wyprzedziła rozgrywkę o kilka długości. |

---

### 3.2. Uzasadniona krytyka strategiczna (Gdzie projekt traci pęd lub ryzykuje ślepą uliczkę?)

Jako Twój Chief Architect mam obowiązek kwestionować niepotrzebną złożoność i wskazywać miejsca, gdzie inżynierski perfekcjonizm może opóźnić dowód samego produktu:

#### Krytyka 1: Spirala mikrozarządzania kolizjami w wąskich gardłach (Prawo malejących przychodów w M03+)
Spójrzmy na historię ostatnich przyrostów po bazowym M03:
1. `navigation-polish`
2. `motion-adversarial-audit`
3. `advanced-traversal`
4. `traversal-command-hardening`
5. `autonomy-need-fallback`
6. `reusable-capacity`
7. `traversal-multi-actor`
8. `navigation-obstruction-recovery`
9. `m04-entry-observability`
10. `diagnostics-needs-hardening`
11. `retained-world-navigation-stall`
12. `local-clearance`

**Diagnoza:** Scena `make_traversal_course()` została skonstruowana jako ekstremalny tor przeszkód: 8 kapsuł o promieniu `0.35 m` na kładce o półszerokości `0.65 m` i jednym jednomiejscowym promie, z ciągłym narastaniem głodu co krok. W tak ciasnym środowisku każda drobna zmiana reguły planowania ujawniała nową geometryczną konfigurację blokady (osoba 6 na `Step`, osoba 7 przy rampie itd.).
- To świetnie zahartowało kontroler ruchu i dało Ci protokół `Clearance` (`DEC-015`).
- **Ale uwaga:** W prawdziwej grze Action RPG (nawet w *Kingdom Come: Deliverance*, *Dwarf Fortress* czy *Kenshi*) nie rozwiązuje się każdego N-osobowego zakleszczenia tłumu w 1,3-metrowym gardle przez coraz bardziej skomplikowane lokalne drzewa przeszukiwań geometrycznych. Od pewnego progu zagęszczenia stosuje się:
  1. szersze ciągi komunikacyjne w level designie (np. most o szerokości pozwalającej na swobodne minięcie 2 osób lub jawne mijanki),
  2. miękkie zmniejszenie promienia kolizji między samymi NPC w fazie wymijania (`ClearanceMove`), lub
  3. prostą wymianę pozycji (swap) po uzgodnieniu zgody.
- **Rekomendacja:** Uznać obecny stan ruchu i `local-clearance` za **wystarczający i zamknięty na potrzeby M04**. Nie wchodzić w kolejne pętle łatania rzadkich przypadków brzegowych tłumu przed udowodnieniem łańcucha konsekwencji M04 i rozgrywki M05.

#### Krytyka 2: Zbyt szybki metabolizm laboratoryjny zaburza obserwację logistyki i pracy
- W `TraversalSession` każdy krok (50 ms) zwiększa głód o 1. Oznacza to, że próg głodu `250` jest osiągany po **12,5 sekundy**, a głód krytyczny `750` po **37,5 sekundy**!
- Z kolei sama podróż promem w jedną stronę (`travel_ticks = 120`, `dwell_ticks = 80`) i przejście kładką zajmują kilkanaście sekund.
- W efekcie w 5-minutowym pomiarze (`m04-entry-observability.md`) mieszkańcy spędzają większość życia w kolejkach po jedzenie lub w stanie maksymalnego głodu `1000`, a kurier w trakcie jednego kursu dostawy (12 jednostek = 3 obroty po 4 jednostki) niemal natychmiast wpada w przerwanie żywieniowe (`hunger >= 750`).
- **Wniosek:** Tak wysokie tempo głodu było dobre do szybkich testów jednostkowych (by w 400 krokach wymusić posiłek), ale przy testowaniu społeczności M04 (SC-10, SC-14) zamienia osadę w obóz ciągłego głodu zamiast w stabilną społeczność pracującą przy warsztacie/przeprawie. Warto sparametryzować tempo narastania potrzeb w `TraversalScene` lub `LocomotionTuning` (z zachowaniem dotychczasowych wartości jako domyślnych dla starych testów regresyjnych!).

#### Krytyka 3: Brak bezpośredniego „Czasownika Gracza” poza samym noszeniem i proszeniem o przejście
- Obecnie w `reality_traversal` gracz może: wybrać postać (`1–8`), przejąć sterowanie (`F2`), chodzić (`WASD`/`PPM`), poprosić o ustąpienie (`F3`) i przeładować towar (`F4`).
- Aby spełnić **Pierwszy dowód produktu** z `docs/project-charter.md` („Gracz wpływa na dostępność przejścia, co zmienia realny transport, dostępność zasobu i działanie mieszkańców”) oraz SC-10/SC-14, gracz (i świat) potrzebuje w `TraversalSession` odpowiednika operacji na samej przeprawie/infrastrukturze (zamknięcie/uszkodzenie/odbudowa połączenia lub odblokowanie zatrzymanej platformy/mostu przy użyciu dostarczonego materiału konserwacyjnego!).
- Zauważ: w `TraversalSession` istnieje już stacja `TraversalTask::Maintain` ze zmienną `wear` (zużycie konstrukcji 0..1000), a w `WorkWorld` istnieje materiał konserwacyjny (`MaterialTypeId = 2` w `make_delivery_course()`), ale `station.wear` **nie wpływa jeszcze na nic w świecie** (gdy `wear` dochodzi do 1000, most się nie zamyka ani prom nie zwalnia/nie staje!). Połączenie `station.wear` z dostępnością przejścia lub urządzenia domknęłoby pętlę przyczynową w sposób organiczny!

#### Krytyka 4: Zarządzanie repozytorium (Brak commitów Git)
- `AGENTS.md` i `development.md` odnotowują: „Lokalny Git ma gałąź main, bez commitów i remote”.
- Przy tak rygorystycznym projekcie, w którym masz już kilkadziesiąt plików źródłowych, ponad 170 testów i 4 wersje snapshotów, brak lokalnych commitów Git po każdym zaliczonym przyroście stanowi poważne ryzyko operacyjne (utrudnia `git diff`, bisekcję regresji i bezpieczny rollback podczas pracy z agentami). Nawet bez zdalnego repozytorium (remote), lokalne commity po każdym domkniętym etapie są wysoce wskazane.

---

## CZĘŚĆ IV: Kierunki i Mapa Drogowa — Co robić teraz i w jakiej kolejności?

Poniżej proponuję konkretny, wykonalny plan działań podzielony na **Horyzont Natychmiastowy (domknięcie obecnej pracy)**, **Horyzont M04 (społeczność i pętla przyczynowa)** oraz **Horyzont M05 / Action RPG**.

### Krok 1 (Teraz): Domknięcie bieżącego przyrostu Transferu i Dostawy (`material-delivery`)
Jesteś o krok od pełnego ukończenia tego etapu. Aby go zamknąć zgodnie z `ENG-01`..`ENG-18`:
1. **Naprawić 3 warunki w kodzie `TraversalSession` / `traversal_delivery.cpp` pod testy w `tests/delivery_tests.cpp`:**
   - W `TraversalSession::validate()` (`src/lab/traversal_session.cpp:1764`) wymusić unikalność `station.lot` pomiędzy wszystkimi stacjami w `scene_.stations` (dla testu `delivery.aliased_lots`).
   - W `TraversalSession::interact_cargo()` oraz `arrive_delivery()` (`src/lab/traversal_delivery.cpp`) oprócz `length(*point - actor.feet) <= 1.1` wymagać, aby odcinek między stopami aktora a punktem stanowiska na tej samej powierzchni był wolny od przeszkód (`continuous_surface_feet` / `spatial_segment_clear`) — dla testu `delivery.wall`.
   - W `TraversalSession::choose_approach()` (`src/lab/traversal_session.cpp:688`) odrzucać kandydatów, z których odcinek do samego `destination` przecina przeszkodę (`!spatial_segment_feet` / `!spatial_segment_clear`) — dla testu `delivery.wall_approach`.
2. **Dodać weryfikację `delivery` do natywnego testu procesu (`tools/test_traversal_diagnostics.py` / `traversal_main.cpp`)** i wykonać pełne przebiegi:
   - `cmake --build --preset windows-debug`, `windows-release`, `windows-asan`,
   - `ctest` dla wszystkich trzech presetów + `format-check`.
3. **Zaktualizować dokumentację kanoniczną i rejestr (`docs/documentation-map.json`):**
   - `docs/work-domain.md` (sekcja o `MaterialTypeId`, `transfer()`, walidacji chronologicznej i `GENESIS_WORK_WORLD 3`),
   - `docs/traversal-autonomy.md` (sekcja o `TraversalTask::Deliver`, `actor.cargo`, `interact_cargo` / `F4` oraz `GENESIS_TRAVERSAL 4`),
   - `docs/testing-strategy.md` (aktualizacja tabeli SC-04 i opis `reality_transfer_tests` oraz `reality_delivery_tests`),
   - `docs/development.md` (opcja `--delivery` i skrót `F4`),
   - raport weryfikacyjny `docs/verification/material-delivery.md` i wpis w `docs/documentation-map.json`, a następnie `python tools/check_docs.py`.

---

### Krok 2 (Domknięcie M04): Połączenie Dostawy, Infrastruktury i Wiedzy w Pełny Łańcuch Konsekwencji (`SC-05`, `SC-10`, `SC-14`)
Gdy `material-delivery` będzie zielone, masz w ręku prawie wszystkie klocki do zaliczenia pełnego **M04**. Zamiast rozbudowywać geometrię, połącz istniejące klocki w trzy małe, wysokowartościowe przyrosty:

1. **Prawdziwa konsekwencja infrastrukturalna (`wear` / dostępność przeprawy):**
   - Połącz istniejące `station.wear` (lub jawny stan mostu/kładki/promu) z dostępnością przejścia i zużyciem materiału konserwacyjnego (`MaterialTypeId = 2`).
   - Gdy przejście zostaje zamknięte (przez awarię `wear == 1000` lub ingerencję gracza), kurier z dostawą żywności (`MaterialTypeId = 1`) musi użyć objazdu (np. promu zamiast kładki) albo czekać na naprawę, co realnie opóźnia dostawę i wpływa na posiłki drugiej osady (`SC-10`).
   - W wariancie spokojnym (`SC-14`): gdy przejście działa, kurier regularnie zasila drugą osadę, konserwator utrzymuje urządzenie dzięki dostawom materiału `2`, a mieszkańcy obu stron żyją i pracują bez katastrofy i bez rozkazów gracza.
2. **Minimalny kanał przekazu wiedzy między ludźmi (`INV-08`, `SC-05`):**
   - Wprowadź małą strukturę `ActorBelief` w sesji: lokalna obserwacja stanu przejścia (zamknięte/otwarte) oraz zapasu stanowiska (puste/dostępne) z czasem obserwacji (`observed_at`).
   - Dodaj przekazywanie obserwacji podczas kontaktu dwóch postaci (lub naciśnięcia klawisza interakcji przez gracza): mieszkaniec dowiaduje się od przewoźnika lub gracza, że most jest zamknięty lub że w magazynie A pojawiła się dostawa, zanim sam odbije się od ściany.
   - To udowodni hipotezę wyróżnika z `docs/game-design.md` (**trwałe przekazywanie praktyk / informacji zmieniające plan drugiej osoby**) przy minimalnym koszcie kodu!

---

### Krok 3 (Wejście w M05 i Warstwę Gry Action RPG): Bezpośrednia Rozgrywka i Pierwszy Konflikt
Zgodnie z `DEC-008`, `DEC-011` i `docs/game-design.md`, po domknięciu logistyki M04 największym priorytetem strategicznym przestaje być symulacja kolejek, a staje się **bezpośrednie doświadczenie gracza jako uczestnika świata**:

1. **Własność, legalność i świadek (`INV-07`, `SC-05`):**
   - W `WorkWorld` każda partia ma już pole `EntityId owner`! Obecnie w `TraversalSession` wszystkie stacje mają przypisanego `actors_.front().id` jako właściciela.
   - Przypisanie rzeczywistych właścicieli do partii zasobów pozwala natychmiast rozróżnić:
     - **Legalny odbiór/zakup lub zlecenie kurierskie** (za zgodą właściciela),
     - **Fizyczne zabranie cudzego zasobu (kradzież / przejęcie ładunku)** — technicznie wykonalne przez ten sam `world_.transfer()`, ale generujące zdarzenie obserwowane przez świadków w zasięgu wzroku (`spatial_segment_clear` + kąt widzenia `heading`).
2. **Mały, mięsisty prototyp bezpośredniego starcia / interakcji Action RPG:**
   - Zamiast budować od razu 50 rodzajów broni i zaklęć, zaprojektować na tej samej kapsułowej fizyce `LocomotionState` jeden precyzyjny kontrtakt fizyczny: **Zatrzymanie / Ochrona ładunku** (1 gracz, 1–2 strażników/rabusiów z jawnym celem przejęcia `cargo`).
   - Wykorzystać istniejącą orientację (`heading`), prędkość, dystans, energię (`energy` jako staminę) oraz geometrię osłon: przygotowanie ruchu, zasięg trafienia/pchnięcia, koszt energii, utrata równowagi/poddanie się NPC (zamiast obowiązkowej śmierci niszczącej populację!) i realny skutek dla niesionego `actor.cargo`.

---

### Podsumowanie dla Ciebie

Twój projekt ma obecnie **jeden z najzdrowszych rdzeni symulacyjnych**, jakie można zbudować w C++20 bez zewnętrznych silników: twarde niezmienniki, brak ukrytego stanu, pełną deterministyczną odtwarzalność i weryfikację od poziomu alokatora pamięci po piksele z D3D11. Implementacja transferu w `WorkWorld` v3 (`material_transfer.cpp`) jest zaprojektowana po mistrzowsku — szczególnie decyzja o reprezentowaniu niesionego ładunku jako partii `WorkWorld` oraz chronologiczny audyt przepływów.

Teraz najważniejsze to:
1. **Domknąć 3 brakujące kontrole geometryczne/walidacyjne w `traversal_delivery.cpp` i `traversal_session.cpp`** (pod testy `wall`, `wall_approach` i `aliased_lots`) oraz zaktualizować dokumentację.
2. **Nie dać się wciągnąć w dalsze polerowanie mijania tłumu na kładkach**, lecz natychmiast wykorzystać nowy mechanizm transferu i dostawy do zamknięcia pętli przyczynowej **M04** (wpływ dostawy i stanu przejścia na życie dwóch osad, z pierwszym kanałem przekazu wiedzy między postaciami).

Jeśli chcesz, w kolejnym kroku mogę przygotować dokładną specyfikację zmian dla domknięcia `delivery.wall`, `delivery.wall_approach` i `delivery.aliased_lots` albo przejść do szczegółowego zaprojektowania konkretnego podsystemu z powyższej mapy drogowej.