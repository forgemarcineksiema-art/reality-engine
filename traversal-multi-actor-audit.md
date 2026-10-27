# Audyt testów i specyfikacja odbioru ruchu oraz nawigacji wielu postaci

**Status: specyfikacja architektoniczna i plan naprawczy (`PLAN` / `PROPOZYCJA` do wdrożenia w ramach [rozszerzenia ruchu](docs/work-items/advanced-traversal.md), DEC-014).** Dokument oddziela fakty stwierdzone w kodzie i testach od specyfikacji zmian wymaganych do praktycznego odbioru ruchu wielu postaci. Nie zastępuje normatywnych [reguł inżynierskich](engineering-rules.md), [kontraktów subsystemów](docs/subsystem-contracts.md) ani [strategii testów](docs/testing-strategy.md).

## 1. Stan faktyczny obecnych zestawów testowych

Projekt utrzymuje dwa tory ruchu i nawigacji:

1. **Tor bazowy M03 (2,5D, `MotionSession` + `kinematic` + `grid_navigation`)**:
   - Płaszczyzna $y = 0$, przeszkody AABB, stała prędkość $3\text{ m/s}$ bez przyspieszenia, hamowania i orientacji kąta `yaw`, siatka A* z przyłączami $3\times 3$ oraz skracaniem łamanej (`find_motion_route`), stała populacja 2 wykonawców.
   - Zestawy testów: [testy ruchu](tests/motion_tests.cpp) (19 przypadków), [testy adwersarialne](tests/motion_adversarial_tests.cpp) (13 przypadków) oraz [integracja GPU](tests/motion_gpu_integration.py).
2. **Tor przestrzenny DEC-014 (`TraversalSession` + `spatial_geometry` + `locomotion` + `platform_motion` + `surface_navigation`)**:
   - Wypukłe graniastosłupy z obrotem `yaw` i nachyleniem (`SpatialPrism`), kapsuły o różnych wymiarach (`CapsuleSize`), profil prędkości z ograniczeniem zrywu (`jerk`), przyspieszenia, hamowania i prędkości kątowej, jawne połączenia `Walk` / `Step` / `Transport`, ruchome i obracające się podpory, wąskie przejścia FIFO (`PassageState`), promy (`TransportService`), do 32 postaci (domyślna scena `make_traversal_course()` zawiera 8 postaci) oraz ograniczona autonomia potrzeb (`Eat`, `Rest`, `Maintain`).
   - Zestawy testów: [testy prymitywów przestrzennych](tests/spatial_tests.cpp) (16 przypadków) oraz [testy sesji traversal](tests/traversal_tests.cpp) (18 przypadków).

### Pokrycie scenariuszy praktycznych w torze przestrzennym

| Scenariusz praktyczny | Obecny stan w `spatial.*` / `traversal.*` | Brakujące pokrycie do odbioru praktycznego |
| --- | --- | --- |
| Szybkie zmiany celu | `supersede_pending_goal`, `supersede_pending_stop`, `command_roundtrip` (wyłącznie odroczenie podczas `Step` i `Riding`) | Brak serii szybkich zmian celu podczas rozpędzania, hamowania (`acceleration < 0`), zakrętu i zmiany poziomów; brak testu idempotentności `approach()` oraz `set_autonomous()` podczas kroku/transferu. |
| Hamowanie przed przeszkodą | `locomotion_envelopes`, `locomotion_arrival` (1D w próżni), `walk` (dojście na otwartej płycie) | Brak testu hamowania przed ścianą, krawędzią płyty i nagłym blokerem (w ruchu po trasie, w ruchu ręcznym `WASD` oraz po `stop()`). |
| Stoki (`ramp`) | `spatial.ramp` i `traversal.ramp` (wyłącznie ruch pojedynczej postaci w górę stoku) | Brak testu schodzenia w dół stoku, zatrzymania i nawrotu $180^\circ$ na stoku, przejścia `płyta -> rampa -> podest -> rampa -> płyta`, mijania 2 postaci na stoku oraz zapisu/odczytu na stoku. |
| Obracające się podpory | `spatial.platform_volume` (test bryły), `traversal.rider` (1 nieruchomy pasażer na podnośniku z obrotem $\pi/2$) | Brak testu wejścia (`Step`) i zejścia z obracającej się podpory (`body 8`), chodzenia po obracającej się podporze, stania przy narożniku podczas obrotu oraz blokady obrotu przez postać obok. |
| Wsiadanie i wysiadanie | `traversal.step` (1 postać), `traversal.transport` (1 postać w jedną stronę, zapis tylko w `Riding`) | Brak testu wymiany pasażerów na tym samym brzegu (jeden wysiada, kolejni czekają na wejście), zapisu w fazach `Boarding` i `Exiting` oraz przerwania w `Waiting`. |
| Mijanie kilku postaci | `spatial.pair_paths`, `spatial.unequal_capsules` (wyłącznie prymityw `capsule_paths_clear`) | Brak testu mijania $\ge 2$ postaci (czołowego i krzyżowego) oraz obejścia stojącej postaci w `TraversalSession`. |
| Przeciwny ruch na kładce | `release_passage_ownership` (2 postacie z tej samej strony nakładających się płyt) | Brak testu 2+ postaci idących naprzeciw siebie przez wąską kładkę (`body 3`). |
| Kolejki | `last_meal`, `reusable_rest` (2 postacie z symetrycznych pozycji $z = \pm 1$) | Brak testu kolejki $\ge 3$ postaci do kładki i promu oraz 2 postaci podchodzących do stanowiska współliniowo z tego samego kierunku. |
| Zajęty cel | `occupied_goal` (brak fałszywego `Arrived` dla dokładnego celu, potem ręczne `approach`) | Brak testu stanu `Waiting` przy zajętym celu, wznowienia po odejściu blokującego oraz wyboru innego punktu na pierścieniu, gdy pierwszy punkt zostanie zajęty. |
| Wyjście właściciela | `release_passage_ownership` (ręczne wyjście z nakładających się płyt) | Brak testu wyjścia właściciela przez `Step`, przez zmianę celu oraz zwolnienia promu przy oczekujących po obu stronach. |
| Wznowienie po zapisie | Wybrane punkty w `walk`, `rider`, `transport` (`Riding`), `command_roundtrip`, `need_fallback`, `last_meal`, `reusable_rest` | Brak zapisu/odczytu w `Boarding`, `Exiting`, w połowie `Step`, na stoku, podczas hamowania/obrotu, podczas mijania oraz brak testów uszkodzonych i sprzecznych zapisów `GENESIS_TRAVERSAL 1` (SC-08). |
| Pełna scena 8 postaci i czas | `traversal.course` uruchamia domyślną scenę wyłącznie na **5 ticków ($0{,}25\text{ s}$)** | Brak wielotickowego przebiegu domyślnej sceny 8 postaci, testu podziału czasu `SimulationPacer`, testu kolizji idącego pasażera na ruchomej platformie oraz pełnej diagnostyki oczekiwania w `inspect()`. |

---

## 2. Odtworzone analitycznie problemy w kodzie produkcyjnym

### Problem 1 (KRYTYCZNY — INV-09, INV-10): Zakleszczenie (deadlock) przy przeciwnym ruchu na kładce (`PassageState`)
- **Miejsce**: [src/lab/traversal_session.cpp](src/lab/traversal_session.cpp) (`TraversalSession::target`, okolice linii 663–681).
- **Mechanizm**:
  Trasa przez kładkę (`body 3`, `passage = 1`) składa się z odcinków: `Leg 0 (Walk do progu wejścia)` $\to$ `Leg 1 (Step na kładkę, passage=1)` $\to$ `Leg 2 (Walk po kładce)` $\to$ `Leg 3 (Step z kładki na próg wyjścia, passage=1)`.
  Gdy Aktor A (idący z płyty 1 na płytę 2) wchodzi na kładkę, zajmuje `found->owner = A`. W tym samym czasie Aktor B (idący z płyty 2 na płytę 1) kończy swój `Leg 0` (`passage = 0`) dokładnie w punkcie `{2, {-3.8, -2.8}}` (`world = {2.2, 0, -2.8}`), który jest **punktem lądowania `Leg 3` Aktora A schodzącego z kładki**.
  W `target(B)` przy `found->owner != B` funkcja ustawia `B.state = Waiting` i zwraca `return actor.feet;` (zamiast wyznaczyć odsunięty punkt kolejki). Aktor B staje nieruchomo $0{,}2\text{ m}$ od krawędzi kładki (przy sumie promieni kapsuł $0{,}6\text{ m}$), przez co Aktor A na kładce zatrzymuje się na kolizji w $x = 1{,}6\text{ m}$ przed końcem `Leg 2` ($x = 1{,}8\text{ m}$). Aktor A nigdy nie opuszcza kładki, a Aktor B nigdy nie odsłania jej wyjścia.
- **Dodatkowa luka**: Jeśli postać znajduje się fizycznie na powierzchni kładki (`occupies_passage == true`) przy `found->owner == 0` (np. po spawnie lub ręcznym wejściu), `target()` przydziela `found->owner` innej postaci wchodzącej z trasy.

### Problem 2 (KRYTYCZNY — INV-09, INV-10): Zakleszczenie przy wymianie pasażerów promu (`TransportService`) i brak sterowania w fazie dojścia do brzegu
- **Miejsce**: [src/lab/traversal_session.cpp](src/lab/traversal_session.cpp) (`queue_point`, `target`, `progress`, `move`).
- **Mechanizm**:
  1. Warunek przejścia `Waiting -> Boarding` (`s.owner.value == 0 && !s.queue.empty() && s.queue.front() == actor.id && platform_dock(...) == actor.boarding_side && actor.motion.speed < .03`) przełącza oczekującego w `Boarding` natychmiast, gdy stoi on na `queue_point` ($0{,}9\text{–}2{,}4\text{ m}$ od brzegu) albo gdy zatrzymał się daleko od brzegu przez inną postać.
  2. Dla całego `TransferStage::Boarding` flaga `stepping` w `move()` ma wartość `true`, co całkowicie wyłącza sterowanie horyzontowe (`if (wants_movement && !stepping)`) oraz projekcję podparcia już podczas marszu po brzegu z `queue_point` do `points[0] = bank` (`transfer_point == 0`).
  3. Gdy Aktor 0 kończy `Exiting` na Brzegu 1 (stojąc dokładnie w `bank[1]`), zwalnia `s.owner = {}`. W tym samym kroku oczekujący na Brzegu 1 Aktor 1 przechodzi w `Boarding` (`stepping = true`, brak omijania) i idzie po linii prostej z `queue_point` wprost na Aktora 0 stojącego na `bank[1]`.
  4. Ponadto `queue_point` ustawia oczekujących dokładnie na osi wyjścia z promu, a gdy kolejka przekroczy wymiar płyty, zwraca `bank`, stawiając oczekujących w porcie wyjścia.

### Problem 3 (KRYTYCZNY — INV-09): Utrata blokady ruchomej/obracającej się platformy w trakcie wykonywania kroku `Step` oraz wykluczenie mniejszych kapsuł na `body 8`
- **Miejsce**: [src/lab/traversal_session.cpp](src/lab/traversal_session.cpp) (`tick`, okolice linii 1047–1061) oraz [src/lab/traversal_course.cpp](src/lab/traversal_course.cpp) (linia 24).
- **Mechanizm**:
  1. W `tick()` ruchoma platforma jest wstrzymywana dla odcinka `SurfaceLegKind::Step` tylko wtedy, gdy `length(*from_point - actor.feet) < .5 && step_ready(geometry_, leg, actor)`. Ponieważ `allows_step` dopuszcza kroki do $2r + 0{,}05 = 0{,}65\text{ m}$ (a połączenie z `body 8` ma $0{,}60\text{ m}$), w końcowej fazie kroku (`actor.step_point >= 2`, dystans $0{,}51\text{–}0{,}60\text{ m}$ od `from_point`) warunek `< .5` przestaje zachodzić. Jednocześnie `prism_motion_clear` dla kapsuły uniesionej o `step_clearance_m` nad płaską płytą zwraca `true`, więc platforma zaczyna się obracać/przesuwać pod stopami aktora przed zakończeniem kroku.
  2. W `make_traversal_course()` połączenie `{{1, {-1.8, -4}}, {8, {-.7, 0}}}` ma długość $0{,}60\text{ m}$, co dla mniejszych postaci w scenie (`radius = 0.25`, postacie 2 i 5) przekracza `allows_step` ($2 \cdot 0{,}25 + 0{,}05 = 0{,}55\text{ m}$), uniemożliwiając im wejście na `body 8`.

### Problem 4 (WYSOKI — INV-02, INV-09): Niespójność `actor.support` przy kontakcie częściowym, pomijanie `continuous_surface_feet` w `move()` oraz pionowy `padding` obrotu
- **Miejsce**: [src/lab/traversal_session.cpp](src/lab/traversal_session.cpp) (`move`, `tick`) oraz [src/physics/platform_motion.cpp](src/physics/platform_motion.cpp) (`platform_anchor`).
- **Mechanizm**:
  1. Gdy `sweep_capsule` zatrzymuje ruch na przeszkodzie (`MoveStatus::Contact`), `legal` jest `true`, więc `actor.feet` przyjmuje `accepted`, lecz przez `length(actor.feet - destination) > 2 * contact_margin_m` pomijane jest `actor.support = support;`. Po przekroczeniu styku płyt przed kontaktem powoduje to wyjątek `"actor lost declared support"`, a na wznoszącej się platformie (`carrier != 0`, `old_feet.y < carried.y`) zatapia pasażera w platformie na wysokości pośredniej `accepted.y`.
  2. W `move()` wybór podparcia dla `destination` sprawdza `std::abs(*height - carried.y) <= limits(actor).maximum_step + length(own_delta)` zamiast `continuous_surface_feet`, co podczas wyhamowywania (`motion.distance > 0`) pozwala zeskoczyć/wejść na uskok do $0{,}25\text{ m}$ lub przeskoczyć szczelinę $< 3\text{ cm}$ bez połączenia `Step`.
  3. W `move()` i `tick()` zapytania kolizji na obracającej się podporze odejmują `SpatialPoint{0, padding, 0}` od `feet` przy jednoczesnym powiększeniu `radius` o `padding`, co opuszcza dolny biegun kapsuły do `feet.y - padding` i powoduje fałszywą penetrację z sąsiednią płytą na tej samej wysokości.
  4. W `platform_anchor` podwójna transformacja trygonometryczna `to.local(to.world(from.local(feet)))` dla pasażera stojącego na granicy `half_x + epsilon` może przekroczyć `epsilon` o kilka ULP i rzucić `"passenger lost platform support"`.

### Problem 5 (WYSOKI — INV-09, INV-10): Paraliż mijania poniżej $0{,}657\text{ m}$ i brak przejścia w `TraversalState::Waiting`
- **Miejsce**: [src/lab/traversal_session.cpp](src/lab/traversal_session.cpp) (`move`, linie 858–904 i 993–1005).
- **Mechanizm**:
  1. Pętla sterowania sprawdza wyłącznie 7 odchyleń `{0.0, ±0.35, ±0.7, ±1.15}` ($\le 65{,}9^\circ$) na odcinku `horizon >= 0.7 m`. Dla dwóch kapsuł o promieniu $0{,}3\text{ m}$ ($r_1 + r_2 = 0{,}60\text{ m}$), gdy odległość środków spadnie poniżej $\frac{0{,}60}{\sin(1{,}15)} = 0{,}6573\text{ m}$ (odstęp powierzchni $< 5{,}7\text{ cm}$), wszystkie 7 odcinków o długości $\ge 0{,}7\text{ m}$ przecina obrys sąsiada (`wanted_speed = 0`), mimo że kierunek styczny ($\pm 1{,}45\text{–}1{,}85\text{ rad}$) jest wolny.
  2. Gdy `wanted_speed` zostaje wyzerowane przez brak wolnego kąta (`!std::isfinite(best)`), po wyhamowaniu `motion.distance == 0`, `destination == carried` i `legal == true`, więc warunek `if (!legal || length(actor.feet - destination) > 2 * contact_margin_m)` nie wykonuje się: postać stoi z `speed = 0`, `blocked_ticks = 0` i w nieskończoność pozostaje w stanie `TraversalState::Moving` zamiast `Waiting`.
  3. Brak replanowania trasy powierzchniowej z uwzględnieniem stacjonarnego blokera, gdy lokalne odchylenie jest zablokowane przez geometrię.

### Problem 6 (WYSOKI — INV-09): Blokada obrotu przy krawędzi podpory/ścianie oraz odbijanie w bok przy ręcznym ruchu `WASD`
- **Miejsce**: [src/lab/traversal_session.cpp](src/lab/traversal_session.cpp) (`move`, linie 838, 858–928).
- **Mechanizm**:
  1. Gdy postać stoi przy krawędzi podparcia (np. `local.z ≈ half_z`, `heading = 0`) i otrzymuje cel równoległy do krawędzi (`command.heading = pi / 2`), `advance_locomotion` obraca `state.heading` asymptotycznie od $0$ do $\pi/2$ od dołu (`mid < pi / 2`). Ponieważ dla `mid < pi / 2` składowa $\cos(\text{mid}) > 0$, wektor `own_delta` wypycha `destination.z` poza `half_z + epsilon` (`unsupported == true`), zerując prędkość w każdym podkroku.
  2. W ruchu ręcznym (`desired = carried + input * 3`, `distance = 3.0 m`), przy zbliżeniu się na $< 0{,}7\text{ m}$ prostopadle do ściany lub krawędzi płyty, pętla sterowania odrzuca `offset = 0` i samowolnie skręca postać o $\pm 20^\circ\text{–}66^\circ$ w bok lub zatrzymuje ją $0{,}7\text{ m}$ przed narożnikiem.

### Problem 7 (ŚREDNI/WYSOKI — INV-03, INV-10): Szybkie zmiany celu, `approach()`, `set_autonomous()` i konkurencja o punkt podejścia do stanowiska
- **Miejsce**: [src/lab/traversal_session.cpp](src/lab/traversal_session.cpp) (`set_autonomous`, `choose_approach`, `approach`, `decide`).
- **Mechanizm**:
  1. Powtórne wywołanie `approach(actor, dest, reach)` z tym samym celem i zasięgiem nie jest idempotentne (wywołuje `stop(index)`, resetuje trasę i zwiększa `plans`), a w gałęzi odroczonej wywołuje globalne `clear_input()` dla wszystkich aktorów zamiast wyczyścić tylko wejście aktora `index`.
  2. Wywołanie `set_autonomous(index, true)` podczas `committed_step` lub `transfer != None` wywołuje `stop(index)`, ustawiając `stop_after_transfer = true`, po czym nadpisuje `autonomous = true`; po zakończeniu kroku/transferu `progress()` zatrzymuje postać w `Idle`.
  3. `choose_approach` sprawdza wyłącznie bieżące pozycje `actors_[other].feet`, ignorując cele `actors_[other].goal`. Dwie postacie idące współliniowo z tego samego kierunku do jednego stanowiska `Rest` wybierają ten sam punkt na pierścieniu; pierwsza po zakończeniu odpoczynku przechodzi w `Idle` na tym punkcie, a druga czeka przed nią bez końca, bo `decide()` nie pozwala zmienić punktu podejścia przed osiągnięciem `Arrived`.

### Problem 8 (ŚREDNI — INV-12, SC-08): Niepełna walidacja semantyczna `GENESIS_TRAVERSAL 1` i brak testów uszkodzonych zapisów
- **Miejsce**: [src/lab/traversal_session.cpp](src/lab/traversal_session.cpp) (`validate`) oraz [src/lab/traversal_codec.cpp](src/lab/traversal_codec.cpp).
- **Mechanizm**:
  `TraversalSession::validate()` nie weryfikuje dwustronnej zgodności `s.owner == actor.id` $\iff$ aktor jest w fazie `Boarding` / `Riding` / `Exiting` na usłudze `s`, ciągłości łańcucha `actor.route.legs`, zgodności `legs.back().to == *actor.goal` przy `RouteStatus::Found` ani warunku `!actor.goal` $\implies$ pusta trasa i brak transferu. W `tests/traversal_tests.cpp` brakuje testów odrzucenia uszkodzonych/podrobionych zapisów bez mutacji aktywnej sesji.

### Problem 9 (ŚREDNI — Ergonomia UI): Zasłanianie powierzchni kliknięcia PPM przez własną kapsułę wybranej postaci
- **Miejsce**: [src/platform/traversal_main.cpp](src/platform/traversal_main.cpp) (`TraversalWindow::click`, okolice linii 435–440).
- **Mechanizm**:
  Nadpisanie wyniku `pick_traversal_surface` przez `session_.actors()[*object].support` zachodzi również dla `*object == selection_`, przez co kliknięcie PPM na podłogę przez bryłę aktualnie wybranej postaci ustawia cel na jej własną pozycję.

---

## 3. Dodatkowe zagadnienia krytyczne wykraczające poza pierwotną listę

Poniższe sześć zagadnień stanowi bezpośrednią część specyfikacji odbioru i zostaje włączone do zakresu zmian oraz testów:

### 3.1. Długi przebieg pełnej sceny wieloosobowej (`make_traversal_course()`, 8 postaci)
- **Diagnoza**: Test `traversal.course` w [tests/traversal_tests.cpp](tests/traversal_tests.cpp) wykonuje obecnie tylko 5 ticków ($0{,}25\text{ s}$). W domyślnej scenie [src/lab/traversal_course.cpp](src/lab/traversal_course.cpp) na starcie aż 5 z 7 autonomicznych postaci (aktorzy 1–5, z `hunger < 250` i `energy = 1000`) w pierwszym kroku wybiera jedyne stanowisko `Maintain` na górnym wschodnim podeście (`body 6`), dostępne wyłącznie przez pojedynczy prom (`body 7`). Po ukończeniu konserwacji muszą wrócić tym samym promem po jedzenie i odpoczynek, mijając kolejne postacie czekające na podeście `body 5`, rampie `body 4` oraz kładce `body 3`.
- **Wymaganie**: Rozszerzyć weryfikację pełnej sceny 8 postaci o długi przebieg ($\ge 400$ ticków) z zapisem i odczytem w trakcie, potwierdzający brak wyjątków `validate()`, rzeczywiste ukończenia czynności domenowych (`Eat`, `Rest`, `Maintain`) i brak zakleszczenia na promie oraz kładce.

### 3.2. Równoczesny ruch własny pasażera i pionowy/obrotowy ruch platformy z kolizją
- **Diagnoza**: Obecny test `traversal.rider` sprawdza wyłącznie pasażera stojącego nieruchomo (`Idle`) na podnośniku. Gdy pasażer idzie po wznoszącej się lub obracającej się platformie (`carrier != 0`) i uderzy w przeszkodę lub drugiego pasażera na pokładzie (`MoveStatus::Contact` lub kolizja par kapsuł), obecny kod w `move()` przy kontakcie z bryłą przyjmuje pośrednie `accepted.y` pomiędzy starą a nową wysokością platformy, zatapiając pasażera w pokładzie.
- **Wymaganie**: Dodać obsługę bezpiecznego powrotu do `carried` przy niepełnym podkroku na ruchomej podporze (`carrier != 0`) oraz test dwóch pasażerów poruszających się po ruchomej i obracającej się platformie (w tym z kontaktem o krawędź/przeszkodę i mijaniem na pokładzie).

### 3.3. Nierówne wymiary kapsuł w rzeczywistej nawigacji i na stopniach (`unequal_capsules` w sesji)
- **Diagnoza**: Test `spatial.unequal_capsules` weryfikuje wyłącznie funkcję geometryczną `capsule_paths_clear`. W `TraversalSession` mniejsze postacie (`radius = 0.25, height = 1.2`, aktorzy 2 i 5 w `make_traversal_course()`) nie mają testu przechodzenia pod niskim stropem (pod którym wysoka postać $1{,}8\text{ m}$ zostaje odrzucona) ani testu wejścia przez `Step` na obracający się pomost `body 8` (gdzie obecny odstęp $0{,}60\text{ m}$ przekracza ich limit $2r + 0{,}05 = 0{,}55\text{ m}$).
- **Wymaganie**: Skorygować przyłącze `Step` do `body 8` do $0{,}50\text{ m}$ oraz dodać w `traversal_tests.cpp` scenariusz weryfikujący w `TraversalSession` różnicę dostępności tras (niski strop, wąskie przejście, stopnie i mijanie kapsuł o różnych promieniach/wysokościach).

### 3.4. Ciągłość i bezkolizyjność interpolacji obrazu (`presentation(fraction)` i `presentation_geometry(fraction)`)
- **Diagnoza**: W starym torze M03 test `adversarial.passing` sprawdza dla 5 faz pośrednich kroku `{0, 0.25, 0.5, 0.75, 1.0}`, że interpolowane pozycje nie przenikają przez siebie ani przez bryły. W `TraversalSession` sprawdzany jest tylko jeden punkt `presentation(0.5)` dla pojedynczego pasażera w `traversal.rider`.
- **Wymaganie**: W testach mijania wielu postaci oraz wsiadania/jazdy/wysiadania z platformy sprawdzać `presentation(fraction)` i `presentation_geometry(fraction)` dla `fraction` $\in \{0.0, 0.25, 0.5, 0.75, 1.0\}$ pod kątem braku przenikania par kapsuł (`capsule_paths_clear`), braku penetracji geometrii (`spatial_occupiable`), braku skoków pozycji między podkrokami oraz braku mutacji stanu świata przez zapytania prezentacji.

### 3.5. Niezależność wyniku od podziału kroków czasu (`SimulationPacer` partition)
- **Diagnoza**: W `MotionSession` test `motion.partition` porównuje 500 wywołań `SimulationPacer::update` po $10\text{ ms}$ z 5 wywołaniami po $1\text{ s}$ (INV-11). W `TraversalSession` brak takiego sprawdzenia dla ruchu z przyspieszeniem/zrywem, ruchomych platform i autonomii.
- **Wymaganie**: Dodać test partycjonowania czasu z użyciem `SimulationPacer` na `TraversalSession` (porównanie drobnych aktualizacji np. $10\text{ ms}$ z aktualizacjami zbiorczymi oraz sprawdzenie ochrony stanu przy przepełnieniu czasu).

### 3.6. Przejrzysta diagnostyka przyczyny oczekiwania w `inspect()`
- **Diagnoza**: `TraversalState::Waiting` reprezentuje jednocześnie oczekiwanie na kładkę (`PassageState`), oczekiwanie w kolejce na prom (`TransferStage::Waiting`), jazdę promem (`TransferStage::Riding`), oczekiwanie na `step_ready` ruchomej podpory, blokadę kolizyjną przez inną postać oraz przerwę po odmowie zasobu. Ponadto `TraversalSession::inspect()` wypisuje `service=`, ale całkowicie pomija `passage=`.
- **Wymaganie**: Rozszerzyć `TraversalSession::inspect()` o sekcję `passage=<id> surface=<surface> owner=<owner> queue=<size>` oraz jawną informację o stanie blokady/oczekiwania aktora (np. `blocked=<blocked_ticks>`), bez łamania istniejących parserów testowych.

---

## 4. Specyfikacja naprawcza do wdrożenia

### Zakres plików
- Kod produkcyjny:
  - [src/lab/traversal_session.hpp](src/lab/traversal_session.hpp)
  - [src/lab/traversal_session.cpp](src/lab/traversal_session.cpp)
  - [src/lab/traversal_course.cpp](src/lab/traversal_course.cpp)
  - [src/navigation/surface_navigation.hpp](src/navigation/surface_navigation.hpp)
  - [src/navigation/surface_navigation.cpp](src/navigation/surface_navigation.cpp)
  - [src/physics/platform_motion.cpp](src/physics/platform_motion.cpp)
  - [src/platform/traversal_main.cpp](src/platform/traversal_main.cpp)
- Testy i konfiguracja:
  - [tests/spatial_tests.cpp](tests/spatial_tests.cpp)
  - [tests/traversal_tests.cpp](tests/traversal_tests.cpp)
  - [CMakeLists.txt](CMakeLists.txt)
- Dokumentacja:
  - [docs/testing-strategy.md](docs/testing-strategy.md)
  - [docs/traversal-autonomy.md](docs/traversal-autonomy.md)
  - [docs/work-items/advanced-traversal.md](docs/work-items/advanced-traversal.md)
  - [docs/documentation-map.json](docs/documentation-map.json)

### Reguły implementacji (ENG-02, ENG-03, ENG-04, ENG-05)
1. **Kolejki i własność kładki (`PassageState`)**:
   - Jeśli `p.owner.value == 0`, a dowolny aktor fizycznie spełnia `occupies_passage(geometry_, p.surface, other)`, ten aktor utrzymuje `p.owner = other.id` (oraz obecność w `p.queue`). Żaden inny aktor nie może przejąć `p.owner`, dopóki jakakolwiek inna postać fizycznie zajmuje kładkę.
   - Dodać funkcję `passage_queue_point(geometry, passage, entry_location, actor)` wyznaczającą punkt oczekiwania odsunięty w głąb płyty wejściowej (`1.0 + 0.75 * pos` m) oraz przesunięty bocznie prostopadle do osi kładki (próbując `lateral` w `{0.55, -0.55, 0.3, -0.3, 0.0}`), tak aby oczekujący nie blokowali progu wyjścia właścicielowi idącemu z naprzeciwka.
   - Gdy aktor zbliża się na $< 2{,}0\text{ m}$ do wejścia na kładkę na odcinku `Walk` poprzedzającym `Step` (lub jest na odcinku `Step` wejścia na kładkę) i kładka należy do innego aktora, `target()` kieruje go do `passage_queue_point` i ustawia `TraversalState::Waiting`.
2. **Wymiana pasażerów promu (`TransportService`)**:
   - W `queue_point` dodać boczne odsunięcie od osi wejścia/zejścia z promu i bezpieczny dobór punktu wspieranego na powierzchni brzegu (bez fallbacku na `bank`, dopóki na płycie istnieje wolny punkt za progiem).
   - Gdy `s.owner.value == 0 && !s.queue.empty() && s.queue.front() == actor.id && platform_dock(...) == actor.boarding_side`:
     - Dopóki `length(actor.feet - bank) > arrival`, aktor w `TransferStage::Waiting` podchodzi do `bank` zwykłym ruchem powierzchniowym (`stepping == false`, z pełnym omijaniem wychodzących pasażerów i projekcją podparcia), a przycumowany prom wstrzymuje odjazd (`next = moving.phase`), gdy `s.queue.front()` jest na tym brzegu w zasięgu $< 2{,}5\text{ m}$ od `bank`.
     - Przejście w `TransferStage::Boarding` (`s.owner = actor.id`, `actor.transfer_point = 0`) następuje dopiero wtedy, gdy `length(actor.feet - bank) <= arrival && actor.motion.speed < .03` oraz ścieżka wejścia na prom (`step_points(bank, port)` i `center`) jest wolna od innych kapsuł.
3. **Blokada platformy podczas `committed_step` i dostępność `body 8`**:
   - W `TraversalSession::tick()`: jeśli `ahead == actor.leg && actor.step_point > 0` dla odcinka `Step` powiązanego z `moving.definition.body`, bezwarunkowo utrzymać `next = moving.phase` do zakończenia kroku.
   - W `make_traversal_course()`: zmienić przyłącze `Step` do `body 8` z `{1, {-1.8, -4}}` na `{1, {-1.7, -4}}` (dystans poziomy $0{,}50\text{ m} \le 0{,}55\text{ m}$), obsługując również mniejsze kapsuły (`radius = 0.25`).
4. **Spójność `support`, `continuous_surface_feet`, `padding` i `platform_anchor`**:
   - W `move()` dla ruchu bez `Step` weryfikować odcinek `carried -> destination` przez `continuous_surface_feet` i wybierać wyłącznie współpłaszczyznową chodliwą podporę.
   - Przy `MoveStatus::Contact` na ruchomej podporze (`carrier != 0`), gdy nie osiągnięto `destination`, odrzucić krok (`legal = false`, powrót do `carried`). Na nieruchomej podporze (`carrier == 0`) zaktualizować `actor.support` w punkcie `accepted` na podstawie `continuous_surface_feet`.
   - Nie obniżać `feet.y` poniżej powierzchni podparcia przy poziomym `padding` obrotu.
   - W `platform_anchor` zabezpieczyć współrzędne lokalne na brzegu podpory przed błędem zaokrąglenia przy obrocie `yaw`.
5. **Sterowanie lokalne, omijanie z bliska, obrót przy krawędzi, hamowanie ręczne i stan `Waiting`**:
   - Rozszerzyć kąty sterowania w ruchu po trasie do `{0.0, .35, -.35, .7, -.7, 1.15, -1.15, 1.55, -1.55, 1.9, -1.9}` z dodatkowym krótszym horyzontem `std::min(distance, .35)`.
   - W ruchu ręcznym (`manual`) wyznaczać bezpieczny prostoliniowy `stop_distance` przed przeszkodą/krawędzią zamiast odchylać kierunek na boki.
   - Jeśli `std::abs(angle_delta(actor.motion.heading, heading)) > .2`, a krótki krok wzdłuż bieżącego `actor.motion.heading` nie ma ciągłego podparcia lub koliduje, przekazać `wanted_speed = 0`, aby postać obróciła się w miejscu bez uderzania w krawędź/ścianę.
   - Gdy `wants_movement` jest `true`, lecz postać stoi w miejscu (`length(actor.feet - old_feet) <= 1e-5 && actor.motion.speed < .01`), inkrementować `actor.blocked_ticks` i ustawić `actor.state = TraversalState::Waiting`. Po przekroczeniu progu blokady (`blocked_ticks >= 25`) przebudować trasę powierzchniową z kandydatami omijającymi stacjonarnych sąsiadów.
6. **Komendy `approach()`, `set_autonomous()` i konkurencja do stanowiska**:
   - Uczynić `approach()` idempotentnym dla tego samego `(destination, reach)` przy aktualnej wersji geometrii i wolnym `*actor.goal`; czyścić wyłącznie wejście danego aktora.
   - W `set_autonomous(index, true)` podczas `committed_step` lub `transfer != None` czyścić `stop_after_transfer = false`.
   - W `choose_approach` unikać kandydatów na pierścieniu będących już celem `actors_[other].goal` innej postaci, a przy zablokowaniu na zajętym punkcie pierścienia (`requested_reach > 0 && blocked_ticks >= 25`) wybrać kolejny wolny punkt na pierścieniu.
7. **Walidacja zapisu, diagnostyka `inspect()` i wybór PPM**:
   - Rozszerzyć `TraversalSession::validate()` o pełną weryfikację spójności `s.owner` $\iff$ `actor.transfer`, ciągłości `actor.route.legs` oraz stanu bez celu.
   - Dodać stan `passage=` oraz `blocked=` do `TraversalSession::inspect()`.
   - W `TraversalWindow::click` pominąć nadpisywanie celu kapsułą, gdy `*object == selection_`.

---

## 5. Wymagane nowe przypadki testowe i kryteria odbioru (ENG-13, ENG-14, ENG-15)

Nowe przypadki w `reality_traversal_tests`:
1. `traversal.rapid_retarget` — $\ge 120$ szybkich zmian celu (`set_goal`, `approach`, ponowienie, `stop`, `set_autonomous`) podczas rozpędzania, hamowania, zakrętu, jazdy po stoku i `Step`, z kontrolą zapisu/odczytu i końcowego `Arrived`.
2. `traversal.obstacle_braking` — hamowanie z pełnej prędkości przed ścianą i krawędzią płyty (po `stop()`, bliskim celu i w ruchu ręcznym `WASD`) oraz obrót i odejście z samej krawędzi płyty pod kątem $90^\circ$ i $180^\circ$.
3. `traversal.slope_maneuvers` — wejście przez `Step` na rampę, zatrzymanie i odczyt w połowie stoku, nawrót $180^\circ$, wjazd na górny podest, zejście w dół stoku oraz mijanie 2 postaci na stoku z kontrolą limitu prędkości 3D.
4. `traversal.rotating_support` — wejście i zejście (`Step`) dla kapsuły standardowej ($r = 0{,}3$) i małej ($r = 0{,}25$) na obracający się pomost `body 8`, chodzenie po pomoście (także z kontaktem i drugim pasażerem na ruchomej platformie) i stanie przy narożniku podczas obrotu, blokada obrotu przez postać obok oraz zapis/odczyt w trakcie obrotu.
5. `traversal.ferry_exchange` — wymiana 3 pasażerów na promie (1 wysiada na Brzegu B, 2 czeka w kolejce na Brzegu B do rejsu powrotnego), z zapisem i odczytem w fazach `Boarding`, `Riding` i `Exiting` oraz kontrolą `presentation(fraction)`.
6. `traversal.crowd_passing` — krzyżowe i czołowe mijanie 4 postaci (w tym o nierównych wymiarach kapsuł oraz pod niskim stropem) na wspólnej płycie oraz obejście nieruchomego blokera z kontrolą rozłączności `presentation(fraction)` dla `fraction` $\in \{0, 0.25, 0.5, 0.75, 1.0\}$.
7. `traversal.narrow_bridge_opposite` — 4 postacie (2 z zachodu, 2 ze wschodu) pokonujące wąską kładkę (`body 3`) w przeciwnych kierunkach z kontrolą kolejki FIFO, diagnostyki `passage=` w `inspect()`, zwalniania właściciela i zapisu/odczytu.
8. `traversal.occupied_goal_recovery` — przejście w `Waiting` przed zajętym dokładnym celem i automatyczne wznowienie po odejściu blokującego, oraz współliniowe podejście 2 autonomicznych postaci z tego samego kierunku do jednego stanowiska `Rest` (`completed == 2`).
9. `traversal.snapshot_corruption` — test SC-08 dla `GENESIS_TRAVERSAL 1` (odrzucenie uszkodzonej sumy, złej wersji, sprzecznego właściciela usługi, nieciągłej trasy i nałożonych kapsuł bez zmiany bieżącego świata), wielotickowy przebieg pełnej sceny 8 postaci `make_traversal_course()` ($\ge 400$ ticków) oraz test niezależności wyniku od podziału kroków czasu `SimulationPacer`.
