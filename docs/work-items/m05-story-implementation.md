# Historia SC-13 — plan wykonania

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax for tracking.

**Goal:** jedna rzeczywista historia dostawy, która reaguje na usunięcie rozmówcy/miejsca i przeniesienie zapasu.

**Architecture:** opcjonalny StoryState w TraversalSession. Historia posiada role po ID i postęp; WorkWorld zachowuje materiał/komendy, geometria zachowuje miejsca, istniejące kontrolery wykonują ruch i potrzeby. Jeden pion integracyjny obejmuje domenę, kodek i aplikację, ponieważ ich kontrakty muszą zostać odebrane razem.

**Tech Stack:** C++20, MSVC/Win32/D3D11, CMake/CTest, Python stdlib, własna fizyka/nawigacja.

**Spec:** [zatwierdzony projekt](m05-story-design.md), akceptacja użytkownika 29.10.2026: „Tak, wdrażaj projekt”. Plan rozwija tę akceptację technicznie; nie wymaga kolejnego wyboru zakresu.

## Global Constraints

- Cztery jednostki żywności, cztery początkowe osoby, 600 kroków świata po 50 ms.
- Usunięcia tylko przy ticku 0, przed rozpoczęciem historii i na bezpiecznej granicy. Odmowa nie mutuje stanu.
- Nie zmieniać limitów procesów 8/18 s, CTest 20 s, starych kroków/asercji i formatów 1–9.
- Role wskazują trwałe ID. Brak zdalnego odczytu zapasu do sterowania NPC. Brak tworzenia zastępczej osoby, miejsca lub żywności.
- Zachować porażki, historię dokumentów i rzeczywiste bajty dowodów. Odbiór użytkownika oraz całego M05 pozostaje osobny.

## Review Focus

1. Żądanie z daleka przy brakującym rozmówcy: OutOfReach, bez ujawnienia stanu/zmiany historii (test contact).
2. Usunięcie środkowej osoby: role pozostałych nie zmieniają ID, panel/renderer wybierają żywe osoby (removed_actor, native).
3. Zajęty wykonawca/załadowana torba: Busy bez porzucenia zamiaru lub materiału (busy).
4. Anulowanie/termin w trakcie ruchu i z ładunkiem: zachowanie chronionej operacji i bilansu (cancel, deadline).
5. Zwrot materiału po odmowie i F9: historyczne zakończenie pozostaje trwałe (sticky, corruption, fresh_process).

## Task 1: Kompletny kurs i kontynuacja SC-13

**Files:** nowe src/lab/traversal_story.hpp/.cpp, tests/story_tests.cpp, tests/story_integration.py, tools/test_story_native.py, tools/start_story_playtest.py; zmiany w traversal_session.hpp/.cpp, traversal_course.cpp, traversal_codec.cpp, traversal_delivery.cpp, platform/traversal_main.cpp, CMakeLists.txt i odpowiedzialnych dokumentach.

**Interfaces:**
- StoryStage: Dormant, Accepted, Completed, MissingSpeaker, MissingSite, MissingSupply, Failed.
- StoryReply: Accepted, Ended, OutOfReach, Busy, Unavailable.
- StorySeed: indeksy początkowych wykonawcy/rozmówcy/odbiorcy, źródła/celu, ID elementu miejsca, SurfaceLocation znanego punktu spotkania; ilość 4.
- StoryState: role po EntityId, partie po EntityId, miejsce i punkt spotkania; etap/czasy/granice zdarzeń; komenda i torba zlecenia; dowód lokalnego kontaktu/odczytu oraz rzeczywistego posiłku.
- TraversalSession::story() const noexcept -> const optional<StoryState>&; begin_story(size_t) -> StoryReply; remove_actor(EntityId) -> bool; remove_static_body(uint32_t) -> bool.
- make_story_course() -> TraversalScene. Prywatne init_story(), update_story(), validate_story() oraz historyczny widok dostępności z istniejących zdarzeń domeny.

- [x] Napisać test normal: Dormant bez automatycznej decyzji; Accepted po kontakcie; ponowienie nie tworzy zlecenia; rzeczywiste 4/4 i posiłek, bilans po każdym kroku. Uruchomić i zachować RED brakującego API, potem RED wykonania przed implementacją.
- [x] Dodać typy, scenę i inicjalizację ról. Najpierw minimalna kompilowalna odmowa ma wykazać brak przyjęcia; potem wdrożyć bezpośrednio wspólny kontrakt dostawy i obserwatora.
- [x] Testy removed_actor, removed_site, moved_supply i kombinacje: rzeczywiste usunięcie/transfer, brak odtwarzania, stałe pierwszeństwo, role po ID; wdrożyć transakcyjne operacje przygotowania.
- [x] Testy contact, wall, busy, cancel, deadline, sticky: przyjęcie i odczyt tylko po wspólnych bramkach fizycznych, brak przejęcia sterowania, termin 600, zachowany ładunek i historyczne odmowy. Każdy wykazany błąd RED→GREEN.
- [x] Rozszerzyć wyłącznie opcjonalną otoczkę 10: wspólne sekcje 9 (dopuszczone zero dalekich połączeń), marker praktyk i ścisła sekcja STORY. Reset story przy odczycie starszego świata. Sprawdzić byte-roundtrip starego 9, historyczne 1–8 i format10 z opcjonalnymi praktykami.
- [x] Nowe procesy: checkpoint przed startem po trzech zaburzeniach, podczas rzeczywistego ładunku i po zakończeniach. Porównać pełne końcowe bajty i nienaruszenie checkpointu. Korupcja z poprawnym checksumem: brak domenowych skutków sukcesu, ID, czasy, obserwacje i etapy; odmowa transakcyjna.
- [x] Wpiąć --story/--story-case, T i panel. Factory/launcher stosują te same publiczne operacje domeny. N/F2/F4/F5/F9 i bezpieczny obcy świat; żaden wynik nie jest wpisywany przez UI.
- [x] Native smoke czterech początków: prawdziwe wejście, wynik kontrolki, GPU, F5/F9 i obcy świat, brak usuniętej kapsuły/elementu. Zachować obrazy i raporty; osobno pozostawić ocenę człowieka.
- [x] Przeczytać diff i zlecić jeden niezależny przegląd całego pionu. Ważne znalezione błędy utrwalić RED→GREEN; bez wymyślania ponownej niezależnej akceptacji.
- [x] Zamrozić źródła, pełne szeregowe build/test Debug/Release/ASan, format, Python/dokumentacja. Każda komenda z kodem wyjścia i logiem; porażki nie znikają po zielonym wyniku.
- [x] Manifest/archiwum źródeł i dowodów, raport, aktualizacja jednego planu i zatwierdzonego briefu. Lokalne commity spójnego przyrostu, bez push/merge lub usuwania wcześniejszych dowodów.

Rekord dostawy przypisanej do tej pojedynczej historii pozostaje związany z wykonawcą także po zakończeniu. assign_delivery odmawia jego zastąpienia; ręczny transport, ruch, potrzeby i zwykły zwrot nadal działają. Zapis sprawdza trwały rekord wraz z przedziałem komend i fizycznymi transferami. To ograniczenie jednej sceny, nie historia wszystkich zleceń świata.

Wynik końcowy: Debug 420/420, Release 420/420, ASan 422/422; 38 przypadków story w każdym profilu, 58 testów Pythona, format i dokumentacja PASS. Szczegóły i archiwum należą do raportu; odbiór człowieka pozostaje osobny.
