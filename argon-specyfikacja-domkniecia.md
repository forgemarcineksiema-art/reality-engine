Poniżej przedstawiam dwie kompletne, gotowe do implementacji specyfikacje inżynierskie:

1. **Specyfikacja Techniczna A (Natychmiastowa):** Dokładne domknięcie `delivery.aliased_lots`, `delivery.wall`, `delivery.wall_approach`, spójności kontraktu `cancel_delivery` oraz synchronizacji dokumentacji dla trwającego przyrostu `material-delivery`.
2. **Specyfikacja Architektoniczna B (Kolejny podsystem z mapy drogowej M04):** Szczegółowy projekt podsystemu **Lokalnej Wiedzy (`Belief`) i Ciągłości Infrastruktury (`Infrastructure & Consequence Loop`)**, który łączy Twój nowy transfer/dostawę zasobów w pełny dowód produktu (`SC-05`, `SC-10`, `SC-14`).

---

# SPECYFIKACJA A: Domknięcie `material-delivery` (`aliased_lots`, `wall`, `wall_approach` i dokumentacja)

## A.1. Analiza matematyczno-geometryczna trzech przypadków w `tests/delivery_tests.cpp`

W `tests/delivery_tests.cpp` znajdują się trzy testy weryfikujące granice fizycznej i domenowej integralności dostawy:

1. **`delivery.aliased_lots` (`tests/delivery_tests.cpp:311–328`):**
   - **Wejście:** Zapis `GENESIS_TRAVERSAL`, w którym w sekcji `STATIONS` wiersz `stations[1]` ma podmienione pole `lot` na wartość `stations[0].lot.value` (i przeliczony `CHECKSUM`).
   - **Obecne zachowanie:** `TraversalSession::restore()` buduje `candidate` i wywołuje `candidate.validate()`. W `src/lab/traversal_session.cpp:1764–1783` sprawdzane jest jedynie `world_.resource(station.lot).has_value()`, a w `validate_delivery()` (`src/lab/traversal_delivery.cpp:182`) tylko `station.lot != actor.cargo`. Duplikat `station.lot` między dwiema stacjami przechodzi bez błędu.
   - **Wymagany niezmiennik (`INV-02`, `INV-06`):** Każde stanowisko w `scene_.stations` musi posiadać unikalną partię `EntityId lot` w `WorkWorld`, rozłączną zarówno z partiami innych stanowisk, jak i z `actor.cargo` każdego wykonawcy.

2. **`delivery.wall` (`tests/delivery_tests.cpp:329–341`):**
   - **Wejście (`walled(0.4)`):** Płyta `body 1` (`y = 0`, `half_x = 30`, `half_z = 4`). Wykonawca `actors[0]` (`radius = 0.3`, `height = 1.8`) stoi w `{-0.4, 0, 0}`. Stanowisko `stations[0]` leży w `{1, {0.4, 0}}` (`{0.4, 0, 0}`). Pomiędzy nimi stoi nieprzechodnia ściana `body 2` (`origin = {0, 2, 0}`, `half_x = 0.05`, `half_z = 1.0`, `depth = 2.0`, `walkable = false`), zajmująca $x \in [-0.05, +0.05]$, $y \in [0, 2]$.
   - **Obecne zachowanie:** `TraversalSession::interact_cargo(0, 0)` oblicza `point = surface_feet(geometry_, station.location, TraversalLimits{actor.size})`, co daje `{0.4, 0, 0}` (gdyż $0.4 - 0.05 = 0.35 > 0.30$). Następnie sprawdza wyłącznie `length(*point - actor.feet) > 1.1`. Ponieważ $0.8 \le 1.1$, transfer przechodzi przez ścianę!
   - **Wymagany niezmiennik (`INV-09`, `INV-16`):** Przeładunek (`interact_cargo` oraz `arrive_delivery`) wymaga nie tylko odległości $\le 1.1\text{ m}$, ale także **ciągłego podparcia na powierzchni i braku kolizji ze ścianą/bryłą na odcinku od `actor.feet` do punktu stanowiska**.

3. **`delivery.wall_approach` (`tests/delivery_tests.cpp:342–351`):**
   - **Wejście (`walled(0.35)`):** Stanowisko `stations[0]` leży w `{1, {0.35, 0}}` (w punkcie $x = 0.35$ kapsuła o $r = 0.30$ styka się ze ścianą $x = 0.05$: $0.35 - 0.05 = 0.30$). Wykonawca startuje w `{-0.4, 0, 0}`.
   - **Obecne zachowanie:** `plan_delivery(0)` wywołuje `choose_approach(0, {1, {0.35, 0}}, 0.9)`. Pętla generuje 12 kandydatów na okręgu o promieniu $0.9 \times 0.8 = 0.72\text{ m}$ wokół $\{0.35, 0\}$ i sortuje je rosnąco według odległości od `actor.feet` ($\{-0.4, 0, 0\}$).
     - Pierwszym (najbliższym) kandydatem jest kierunek $\pi$ ($\text{direction} = 6$): $x = 0.35 - 0.72 = -0.37, z = 0$.
     - W punkcie $x = -0.37$ kapsuła ($r = 0.30$) zajmuje $[-0.67, -0.07]$, więc nie koliduje ze ścianą $[-0.05, +0.05]$! `surface_feet` zwraca `{-0.37, 0, 0}`, a `find_surface_route` znajduje trasę $0.03\text{ m}$ po lewej stronie ściany.
     - Gdybyśmy zablokowali ścianę tylko w `interact_cargo`, a nie w `choose_approach`, kurier pojechałby do `{-0.37, 0}`, dostał odmowę `OutOfReach` w `arrive_delivery()`, po 40 krokach znów wybrał `{-0.37, 0}` i utknął na zawsze po złej stronie ściany.
   - **Wymagany niezmiennik (`INV-09`, `INV-10`):** `choose_approach` musi odrzucać każdego kandydata `candidate`, z którego bezpośredni odcinek do `destination` jest przecięty przeszkodą geometryczną lub pozbawiony ciągłego podparcia. Dzięki temu kandydat `{-0.37, 0}` zostanie odrzucony *przed* wywołaniem `find_surface_route` (bez zużycia budżetu 4 przeszukiwań `searches`), a `choose_approach` wybierze kandydata po prawej stronie ściany ($x \ge 0.35$) i wyznaczy trasę omijającą ścianę przy $|z| > 1.3\text{ m}$.

---

## A.2. Dokładna specyfikacja zmian w kodzie

### 1. Walidacja unikalności partii stanowisk (`src/lab/traversal_session.cpp`)

W `TraversalSession::validate()` (`src/lab/traversal_session.cpp`, około linii 1764) rozszerz pętlę walidacji stanowisk o zbiór widzianych `EntityId` partii oraz zgodność `ResourceKind`:

```cpp
    std::set<EntityId> station_lots;
    for (const auto& station : scene_.stations) {
        const auto lot = world_.resource(station.lot);
        check(station.kind != TraversalTask::None &&
                  static_cast<unsigned>(station.kind) <=
                      static_cast<unsigned>(TraversalTask::Maintain) &&
                  station.wear <= 1000 && lot.has_value() &&
                  station_lots.insert(station.lot).second &&
                  lot->record.kind == (station.kind == TraversalTask::Rest
                                           ? ResourceKind::Capacity
                                           : ResourceKind::Material),
              "invalid traversal station");
```

*Skutek:* Każda próba wczytania snapshotu (lub skonstruowania sceny), w której dwa stanowiska współdzielą `station.lot`, zostaje natychmiast odrzucona przed podmianą sesji.

---

### 2. Wspólny predykat bezpośredniej osiągalności na powierzchni (`src/lab/traversal_session.hpp` / `.cpp` / `traversal_delivery.cpp`)

Dodaj prywatną metodę pomocniczą w `TraversalSession` (lub funkcję w `traversal_session.cpp` wykorzystywaną przez `choose_approach` i `interact_cargo`):

```cpp
// Sprawdza, czy kapsuła aktora stojąca w `feet` ma bezpośredni, nieprzecięty bryłą
// i ciągle podparty dostęp do punktu `destination` na powierzchni.
bool TraversalSession::direct_access_clear(std::size_t index, SpatialPoint feet,
                                           SurfaceLocation destination) const {
    const auto& actor = actors_.at(index);
    const auto lim = limits(actor);
    const auto& body = geometry_.body(destination.surface);
    const auto anchor_world = body.world({destination.local.x_m, 0, destination.local.z_m});
    const auto supported =
        continuous_surface_feet(geometry_, destination.surface, feet,
                                {anchor_world.x, anchor_world.z}, lim);
    if (!supported) {
        return false;
    }
    // Jeżeli w samym `destination` mieści się pełna kapsuła (np. stanowisko),
    // sprawdzamy pełny sweep kapsuły. Jeżeli `destination` jest kotwicą przy samej
    // przeszkodzie (np. Shift+PPM na ścianę), używamy cienkiej sondy o wysokości aktora,
    // która wykrywa każdą przegrodę pomiędzy `feet` a `destination`.
    const auto exact_feet = surface_feet(geometry_, destination, lim);
    if (exact_feet) {
        return spatial_segment_clear(geometry_, feet, *exact_feet, actor.size);
    }
    const CapsuleSize probe{std::min(actor.size.radius, .01), actor.size.height};
    return spatial_segment_clear(geometry_, feet, *supported, probe);
}
```

#### Zastosowanie w trzech miejscach:

1. **W `TraversalSession::choose_approach` (`src/lab/traversal_session.cpp:688–695`):**
   ```cpp
   for (const auto candidate : candidates) {
       const auto feet = surface_feet(geometry_, candidate, limits(actor));
       if (!feet) {
           continue;
       }
       if (!direct_access_clear(index, *feet, destination)) {
           continue;
       }
       if (!approach_point_free(index, *feet)) {
           continue;
       }
   ```
   oraz przy sprawdzeniu zachowania dotychczasowej trasy (`src/lab/traversal_session.cpp:739–743`):
   ```cpp
   if (actor.requested_goal == destination && actor.requested_reach == reach && actor.goal &&
       actor.route.status == RouteStatus::Found &&
       actor.route.geometry_version == geometry_.version && !actor.pending_goal && prior_goal &&
       direct_access_clear(index, *prior_goal, destination) &&
       approach_point_free(index, *prior_goal)) {
   ```

2. **W `TraversalSession::interact_cargo` (`src/lab/traversal_delivery.cpp:65–68`):**
   ```cpp
   const auto& station = scene_.stations[station_index];
   const auto point = surface_feet(geometry_, station.location, TraversalLimits{actor.size});
   if (!point || length(*point - actor.feet) > 1.1 ||
       !direct_access_clear(index, actor.feet, station.location))
       return CargoStatus::OutOfReach;
   ```

3. **W `TraversalSession::arrive_delivery` (`src/lab/traversal_delivery.cpp:153–159`):**
   Ponieważ `arrive_delivery` deleguje do `interact_cargo(index, station)`, sprawdzenie w `interact_cargo` automatycznie zabezpiecza również autonomiczne dostawy (a dla czytelności warunek `if (point && length(*point - actor.feet) <= 1.1)` przed wywołaniem `interact_cargo` zachowuje tę samą semantykę).

#### Dlaczego to rozwiązanie jest bezpieczne dla wszystkich 170+ istniejących testów:
- Dla `walled(0.4)` (`delivery.wall`): `exact_feet` istnieje (`{0.4, 0, 0}`), `spatial_segment_clear` z `{-0.4, 0, 0}` do `{0.4, 0, 0}` uderza w `body 2` i zwraca `false` $\rightarrow$ `CargoStatus::OutOfReach`.
- Dla `walled(0.35)` (`delivery.wall_approach`):
  - Kandydat `{-0.37, 0}` ma `direct_access_clear == false` (odcinek `{-0.37, 0, 0} -> {0.35, 0, 0}` uderza w ścianę `[-0.05, +0.05]`). Nie zużywa nawet licznika `searches`!
  - Kandydat `{0.35, 0}` (oraz kandydaci po stronie $x > 0$) ma `direct_access_clear == true`. A* znajduje objazd wokół końca ściany ($|z| > 1.3\text{ m}$), kurier podchodzi od strony $x > 0$ i ładuje materiał.
- Dla `occupied_goal` i `approach_recovery` / `approach_alternative_retry`: w tamtych testach w punkcie `destination` stoi *inny aktor*, a nie statyczna bryła `geometry_.bodies`. `direct_access_clear` bada wyłącznie `geometry_`, więc obecność innego aktora w `destination` nie blokuje wyboru kandydata na okręgu `reach * 0.8`.

---

## A.3. Lista aktualizacji dokumentacji wymagana do zamknięcia przyrostu (`check_docs.py`)

Po przejściu wszystkich testów C++ musisz zaktualizować dokładnie te dokumenty (zgodnie z `ENG-02`, `ENG-18` i `docs/documentation-map.json`):

1. **`docs/work-domain.md`:**
   - Dodać sekcję **„Typowany materiał, transfer i wersja 3”**: opis `MaterialTypeId` (`0` = nieokreślony), `TransferRequest` / `TransferStatus` / `TransferRecord`, wspólnej przestrzeni `CommandId` z `reserve`, par zdarzeń `TransferOut` / `TransferIn`, chronologicznej walidacji bilansu w `validate_material_flow()` (odpornej na wielokrotny obieg `UINT64_MAX` i dopuszczającej `lost > initial` po wpływie) oraz formatu `GENESIS_WORK_WORLD 3` (dodatkowe pole `material` w `RESOURCES` oraz sekcja `TRANSFERS`).
2. **`docs/traversal-autonomy.md`:**
   - Dodać sekcję **„Rzeczywisty ładunek i pierwsza dostawa (wersja 4)”**: opis `actor.cargo` jako rzeczywistej partii `WorkWorld` (limit 4 jednostek), `TraversalTask::Deliver`, faz `DeliveryPhase` (`ToPickup`, `ToDestination`, `Returning`, `Completed`, `Cancelled`), ochrony ładunku podczas przerwania na pilną potrzebę (`Eat`/`Rest`), interakcji ręcznej `F4` (`interact_cargo`), zakazu recyklingu zasobu u odbiorcy, wymogu bezpośredniego dostępu bez ściany oraz formatu `GENESIS_TRAVERSAL 4` (`CARGO` i `DELIVERY`).
3. **`docs/subsystem-contracts.md`:**
   - W `INV-06` i na końcu dokumentu dopisać zwięzłą notę normatywną o wdrożonym kontrakcie transferu domenowego (`WorkWorld` v3) i fizycznego przenoszenia partii ładunku w `TraversalSession` v4.
4. **`docs/testing-strategy.md`:**
   - W tabeli `SC-04` przenieść `Transfer` z kolumny „Nadal niewykonane” do „Wdrożony zakres” (pozostawiając recepty i produkcję jako niewykonane).
   - Dopisać akapit o runnerach `reality_transfer_tests` (10 przypadków) i `reality_delivery_tests` (16 przypadków).
5. **`docs/development.md` i `README.md`:**
   - Dopisać komendę uruchomienia `reality_traversal.exe --delivery`, skrót `F4` oraz testy `ctest -R "^(transfer|delivery)[.]"`.
6. **Nowy raport `docs/verification/material-delivery.md` + rejestracja w `docs/documentation-map.json`:**
   - Zarejestrować `docs/verification/material-delivery.md` z rolą `"verification"` i unikalnym `topic: "material-delivery-verification"`.

---

# SPECYFIKACJA B: Projekt Następnego Podsystemu M04 — „Lokalna Wiedza (`Belief`), Ciągłość Infrastruktury i Pełny Łańcuch Konsekwencji (`SC-05`, `SC-10`, `SC-14`)”

Po domknięciu Specyfikacji A masz już:
- Ruch, poziomy, kładki, prom i lokalne ustępowanie (`Clearance`),
- Potrzeby (`Eat`, `Rest`, `Maintain`) i wielorazową pojemność stanowisk,
- Prawdziwy transfer zasobów i fizyczną dostawę ładunku przez kuriera i gracza (`Deliver`).

Czego **dokładnie** brakuje, aby zaliczyć kamień milowy **M04** zgodnie z `docs/prototype-plan.md` i `docs/project-charter.md`?
Brakuje **spięcia przyczynowego trzech ogniw**:
1. **Infrastruktura ma realny skutek dla tras i dostaw (`INV-10`, `INV-14`, `SC-10`):** Zużycie urządzenia/przeprawy (`wear`) lub ingerencja gracza faktycznie zamyka/otwiera przejście, a naprawa wymaga materiału konserwacyjnego (`MaterialTypeId = 2`).
2. **Lokalny obieg informacji (`INV-08`, `SC-05`):** Postacie nie dowiadują się o zamkniętym przejściu ani o nowej dostawie telepatycznie, lecz przez **własną obserwację na miejscu** albo **przekaz informacji od innej osoby**.
3. **Społeczność działa w obu trybach (`SC-14`):** Spokojna samoorganizacja (dostawy + konserwacja + posiłki bez katastrofy) oraz adaptacja po zaburzeniu (zamknięcie przejścia $\rightarrow$ poznanie problemu $\rightarrow$ objazd lub naprawa $\rightarrow$ przywrócenie dostaw).

Poniżej znajduje się kompletny projekt architektoniczny tego podsystemu, zaprojektowany tak, aby **nie tworzyć pustych frameworków (`ENG-03`)** i maksymalnie wykorzystać istniejące struktury `WorkWorld` i `TraversalSession`.

---

## B.1. Granice własności i nowe struktury danych

### Zasada architektoniczna:
- `WorkWorld` pozostaje właścicielem partii zasobów (żywność `MaterialTypeId{1}`, materiał konserwacyjny `MaterialTypeId{2}`, pojemność odpoczynku `ResourceKind::Capacity`), rezerwacji, zużycia i transferów.
- `TraversalSession` posiada fizyczny stan przejść/urządzeń oraz **lokalną bazę przekonań (`BeliefState`) każdego aktora**.
- Planer nawigacji i wyboru zadań (`decide`, `plan`, `choose_approach`) dla autonomicznego aktora **nie czyta globalnej flagi dostępności połączenia ani globalnego zapasu**, lecz korzysta z widoku przefiltrowanego przez przekonania danego aktora!

### 1. Stan infrastruktury powiązany z przejściem (`Passage / Service Condition`)

W `TraversalStation` istnieje już pole `std::uint64_t wear = 500;` oraz zadanie `TraversalTask::Maintain`, ale dziś `wear` jest tylko licznikiem bez skutku przestrzennego. Rozszerzamy je w minimalny, ścisły sposób:

```cpp
struct TraversalStation {
    SurfaceLocation location;
    TraversalTask kind = TraversalTask::Eat;
    std::uint64_t initial_stock = 50;
    EntityId lot;
    std::uint64_t wear = 500;
    std::uint64_t completed = 0;
    MaterialTypeId initial_material;
    // NOWE: Powiązanie stacji konserwacji z konkretnym przejściem (passage) lub usługą (service).
    // 0 oznacza brak powiązanej konstrukcji.
    std::uint32_t linked_passage = 0;
    std::uint32_t linked_service = 0;
};
```

Oraz w `PassageState`:
```cpp
struct PassageState {
    std::uint32_t id = 0;
    std::uint32_t surface = 0;
    EntityId owner;
    std::vector<EntityId> queue;
    bool open = true; // Faktyczny stan fizyczny przeprawy w świecie
};
```

#### Reguły fizyczne konstrukcji (`INV-14`):
1. **Awaria z zużycia:** Gdy dla stacji `Maintain` powiązanej z `linked_passage` wartość `wear` osiągnie `1000` **oraz** żadna kapsuła nie znajduje się aktualnie na powierzchni tego przejścia (`!occupies_passage`), `passage.open` przechodzi w `false`, a `geometry_.version` (lub `topology_version`) inkrementuje się.
2. **Naprawa (`Maintain`):** Ukończenie czynności `Maintain` (zużywającej 1 jednostkę materiału konserwacyjnego `MaterialTypeId{2}` w `WorkWorld`) zmniejsza `wear` o `250` (tak jak dotychczas) i jeśli `wear < 1000`, przywraca `passage.open = true`.
3. **Ingerencja gracza / testu (`set_passage_open` / klawisz interakcji przy przyczółku):**
   - W zasięgu $\le 1.1\text{ m}$ od wejścia na przejście (przy braku osób na kładce i braku trwającego `Step` na tę powierzchnię) można zamknąć/uszkodzić przejście (`wear = 1000, open = false`) albo — mając materiał konserwacyjny — wykonać naprawę.
   - Próba zamknięcia przejścia, na którym stoi postać, jest **odrzucana jawnie** (zgodnie z niezmiennikiem z M03 i `INV-09`).

---

### 2. Model Przekonań i Kanałów Informacji (`ActorBelief` — `INV-08`)

Zamiast budować abstrakcyjny graf semantyczny lub angażować LLM, definiujemy zwartą, deterministyczną strukturę przekonań dla każdego `TraversalActor`:

```cpp
enum class FactSubjectKind : std::uint8_t {
    StationStock, // Czy stanowisko ma dostępny materiał (lub czy jest puste)
    PassageOpen,  // Czy przejście `passage_id` jest otwarte czy zamknięte
    StationWear   // Znany poziom zużycia stanowiska Maintain
};

struct BeliefRecord {
    FactSubjectKind kind = FactSubjectKind::StationStock;
    std::uint32_t subject_id = 0; // indeks stacji lub id przejścia
    std::uint64_t value = 0;      // np. 0 = zamknięte/puste, 1 = otwarte/dostępne, lub wartość wear
    std::uint64_t observed_tick = 0; // kiedy powstała pierwotna obserwacja prawdy
    std::uint64_t learned_tick = 0;  // kiedy ten aktor się o tym dowiedział
    EntityId source_actor;           // kto zaobserwował (actor.id = własna obserwacja, inny ID = przekaz)
};
```

W `TraversalActor` przechowujemy ograniczoną tablicę (lub `std::vector<BeliefRecord>` z limitem np. 16 faktów, po jednym najnowszym na parę `(kind, subject_id)`).

#### Trzy jedyne dozwolone kanały aktualizacji wiedzy (`ENG-09`, `INV-08`):

1. **Kanał 1 — Bezpośrednia obserwacja wzrokowa (`observe_environment`):**
   - Gdy aktor znajduje się w odległości $\le 3.5\text{ m}$ od przyczółka przejścia (`connection.from` / `connection.to`) i odcinek wzroku nie jest zasłonięty ścianą (`direct_access_clear`), aktor aktualizuje swój `BeliefRecord{PassageOpen, passage.id, passage.open ? 1 : 0, now, now, actor.id}`.
   - Gdy aktor podchodzi do stanowiska ($\le 1.1\text{ m}$), aktualizuje `BeliefRecord{StationStock, station_idx, available > 0 ? 1 : 0, now, now, actor.id}` oraz `StationWear`.
2. **Kanał 2 — Przekaz bezpośredni osoba $\rightarrow$ osoba (`share_knowledge`):**
   - Gdy dwóch aktorów znajduje się w odległości $\le 2.0\text{ m}$ na wspólnym podparciu (np. mijają się lub gracz naciska klawisz rozmowy/wymiany informacji `F`), porównują swoje `BeliefRecord` dla tych samych `(kind, subject_id)`.
   - Jeżeli aktor A posiada przekonanie o nowszym `observed_tick` niż aktor B, aktor B przyjmuje ten fakt z `observed_tick = A.observed_tick`, `learned_tick = now`, `source_actor = A.id`!
   - **Kluczowa własność:** Informacja rozchodzi się z prędkością ruchu ludzi (np. kurier wracający spod zamkniętego mostu mija innego mieszkańca i ostrzega go, że most jest zamknięty, albo informuje głodnego mieszkańca, że właśnie dostarczył jedzenie do `stations[1]`!).
3. **Kanał 3 — Wygaśnięcie niepewności (Staleness / Curiosity Horizon):**
   - Przekonanie `StationStock == 0` (brak towaru) lub `PassageOpen == 0` bez nowego potwierdzenia może być ponownie zweryfikowane zwiadem dopiero po upływie określonego horyzontu czasu (np. `400` kroków), chyba że wcześniej inny aktor przekaże nowszą obserwację (`observed_tick` po dostawie lub po naprawie!).

---

## B.2. Jak `Belief` steruje nawigacją i wyborem zadań (bez łamania `INV-09`)

To najważniejszy punkt integracji między wiedzą (`INV-08`) a fizyką (`INV-09`):

1. **Planowanie trasy (`plan` / `find_surface_route`) używa *znanych aktorowi* otwartych połączeń:**
   - Gdy aktor planuje trasę z głębi lądu (daleko od mostu), a w jego `BeliefRecord` most widnieje jako `PassageOpen = 1` (choć 5 sekund temu most uległ awarii i fizycznie `passage.open == false`), **planer wyznacza trasę przez most**, bo aktor jeszcze nie wie o awarii!
2. **Fizyka i wykonanie kroku (`target` / `move`) używają *rzeczywistego stanu świata* (`INV-09`):**
   - Postać nigdy nie może przejść przez fizycznie zamknięty most (`!passage.open`).
   - Gdy postać zbliży się do zamkniętego mostu na odległość obserwacji ($3.5\text{ m}$) — albo spotka wracającego świadka — jej `BeliefRecord` zmienia się na `PassageOpen = 0` (`source_actor = self` lub `witness`).
   - Zmiana przekonania o połączeniu należącym do aktualnej trasy `actor.route` natychmiast unieważnia plan (`INV-10`) i uruchamia przeplanowanie z wykluczeniem zamkniętego połączenia $\rightarrow$ postać wybiera drogę zastępczą (prom)!
3. **Natychmiastowe przebudzenie po dostawie (Rozwiązanie problemu głodujących w osadzie docelowej):**
   - Gdy w osadzie B mieszkańcy odbili się wcześniej od pustego `stations[1]`, mają `Belief{StationStock, 1, value = 0}`.
   - Kiedy kurier dociera do `stations[1]` i wykonuje `interact_cargo` (oddając 4 jednostki żywności), kurier nabywa obserwację `Belief{StationStock, 1, value = 1, observed_tick = now}` (a mieszkańcy stojący w zasięgu wzroku $3.5\text{ m}$ widzą dostawę bezpośrednio; dalszym kurier przekazuje fakt mijając ich).
   - Otrzymanie nowszego przekonania `StationStock = 1` natychmiast kasuje `station_retry[1]` u odbiorcy wiedzy i pozwala mu ruszyć na posiłek!

---

## B.3. Mierzalne Kryteria Akceptacji dla tego Podsystemu (Dowody `SC-05`, `SC-10`, `SC-14`)

Aby uznać ten etap za ukończony zgodnie z `engineering-rules.md`, specyfikacja definiuje 4 jednoznaczne scenariusze testowe C++:

1. **`consequence.bridge_closure_and_detour` (`SC-10`):**
   - Kurier ma dostarczyć żywność ze stacji 0 (brzeg A) do stacji 1 (brzeg B). Najkrótsza droga prowadzi przez kładkę (`passage 1`), dłuższa przez prom (`service 1`).
   - Zamykamy `passage 1`, gdy kurier jest daleko (poza zasięgiem wzroku).
   - **Asercja 1 (`INV-08`):** Kurier początkowo nadal idzie w stronę kładki (brak wszechwiedzy).
   - **Asercja 2 (`INV-10`):** Po wejściu w zasięg obserwacji przyczółka (lub spotkaniu świadka) zapisuje `Belief{PassageOpen, 1, 0}`, zmienia trasę na prom i dowozi ładunek objazdem.
2. **`consequence.witness_knowledge_transfer` (`SC-05`, `INV-08`):**
   - Aktor A widzi zamknięty most (`observed_tick = T`). Wracając, mija aktora B idącego w stronę mostu.
   - **Asercja:** Aktor B otrzymuje `BeliefRecord` z `source_actor == A.id` i `observed_tick == T`, i zawraca na objazd **zanim** sam dojdzie do mostu. W wariancie kontrolnym (gdy A i B idą innymi drogami i się nie mijają), B musi dojść do samego mostu, by się o tym dowiedzieć.
3. **`consequence.maintenance_restores_logistics` (`SC-10`, `INV-14`):**
   - Kładka jest zamknięta z powodu `wear == 1000`. Konserwator pobiera materiał `MaterialTypeId{2}`, wykonuje `Maintain`, `wear` spada do `750`, kładka się otwiera (`passage.open = true`), a kolejne kursy dostawy żywności znów korzystają z szybszej kładki zamiast czekać na prom.
4. **`consequence.autonomous_settlement_peaceful` (`SC-14`):**
   - 8 autonomicznych mieszkańców przez 1000 kroków bez ingerencji gracza: kurier dowozi żywność z osady A do osady B, konserwator utrzymuje sprawność przeprawy, mieszkańcy osady B jedzą dostarczoną żywność i odpoczywają na stanowisku `Capacity`, a bilans `WorkWorld` oraz kontynuacja po zapisie/odczycie są w 100% zachowane.

---

## Rekomendacja Wykonawcza (Kolejność działań)

1. **Najpierw zamknij Specyfikację A** (3 poprawki w `traversal_session.cpp` i `traversal_delivery.cpp`: unikalność `station_lots` w `validate()` oraz `direct_access_clear` w `choose_approach` i `interact_cargo`, plus aktualizacja dokumentacji i raport `docs/verification/material-delivery.md`).
2. **Gdy CTest (`Debug`, `RelWithDebInfo`, `ASan`) i `check_docs.py` będą w 100% zielone**, przejdź bezpośrednio do **Specyfikacji B** — to najkrótsza, najczystsza architektonicznie droga do pełnego zaliczenia kamienia milowego **M04**.