# Wykonanie odbioru technicznego etapu M06

> Wykonanie: `superpowers:executing-plans`, zadania kolejno, jedna niezależna kontrola kodu na końcu.

**Aktualizacja 02.11.2026:** Debug/Release 435/435, ASan 437/437 po jednej kontroli niezależnej i naprawach. Długi pomiar i końcowa kontynuacja pozostają do wykonania.

**Cel:** wiarygodny fundament: poprawność, ciągłość świata, rzeczywisty postęp i kontrolowane dłuższe działanie aplikacji na laptopie użytkownika.

**Specyfikacja:** [brief M06](m06-acceptance.md), [plan nadrzędny](../prototype-plan.md#m06--odbiór-i-decyzja-o-skali), [budżety](../performance-budget.md). Decyzja użytkownika z 02.11.2026 usuwa uczestników i ocenę zainteresowania grą z bramek M06. Odbiór użytkownika jest zadeklarowany; wyniki techniczne muszą zostać rzeczywiście uzyskane, a porażki zachowane.

**Architektura:** opcjonalny obserwator kosztów wykonania, wiązany wyłącznie na czas wywołań aplikacji, bez zmiany formatu świata i decyzji NPC. Osobne CSV profilu i asynchronicznych znaczników GPU zachowują historyczny schemat `samples.csv`. D3D11, C++20, Win32, Python bez nowych zależności.

## Kontrakty i granice

- Stały krok 50 ms, semantyka gracza/NPC, bilanse, lokalna wiedza i stare zapisy pozostają obowiązujące.
- `--profile-runtime` wymaga `--measurement-trace`; brak opcji nie uruchamia zapytań GPU.
- CPU work to czas ścienny przed Present; osobno czas Present i rzeczywisty czas CPU procesu. Nie sumować CPU/GPU jako budżetu klatki.
- `navigation` obejmuje wyszukiwanie tras powierzchniowych; `learning` obsługę praktyk; `save` serializację i atomowy zapis sesji. Zakresy mogą się zawierać i nie są pełnym podziałem CPU.
- GPU: timestamp/disjoint, osiem slotów, GetData DONOTFLUSH; brak gotowości, pełna kolejka i disjoint są jawne, bez aktywnego czekania i fikcyjnych zer.
- Nowe pliki tylko CREATE_NEW w nowym katalogu; błędy i niepełny przebieg nie dają sukcesu.

## Review Focus

Niezależna kontrola: zagnieżdżony obserwator i wyjątki; opóźnione/disjoint zapytania GPU; zapis przy uszkodzonym raporcie; kolizje nazw plików; granice faz i odtworzenie świata. Testy poniżej obejmują te sytuacje w stosownym zakresie, a próba natywna sprawdza rzeczywiste zapytania GPU.

## Zadania

### Task 1: koszty wykonania i GPU

Pliki: `src/core/runtime_profile.*`, `src/navigation/surface_navigation.cpp`, `src/lab/traversal_practice.cpp`, `src/lab/traversal_session.cpp`, `src/graphics/d3d11_renderer.*`, `src/platform/traversal_main.cpp`, `tests/traversal_diagnostics_tests.cpp`, `tools/test_traversal_diagnostics.py`, `tools/test_runtime_profile.py`, `tests/m06_tests.cpp`, `CMakeLists.txt`, instrukcja development.

- [x] RED: natywna próba `--profile-runtime` wymaga rzeczywistych CSV CPU/GPU, niezmienionego samples.csv, zgodnej kontynuacji i jawnych liczników; obecny program odrzuca opcję.
- [x] RED: testy kosztów sprawdzają izolację wiązań, wyjątek, zagnieżdżone zakresy oraz identyczny wynik symulacji z obserwatorem i bez.
- [x] GREEN: zaimplementować wskazane kontrakty, zachować istniejące regresje eksportu.
- [x] Sprawdzić profil natywnie, Debug/Release/ASan, Python i dokumentację; zachować pełne logi, commit.

### Task 2: protokół, pilotaż i długi przebieg

Pliki: `tools/run_m06_acceptance.py`, testy narzędzia, `docs/verification/m06-acceptance.md`, dowody oraz zamrożony protokół JSON.

- [x] Testy analizatora mają wykrywać utraconą fazę, niepełny zapis, błędy symulacji, brak aktywności i przekroczenia bramek na danych wejściowych.
- [ ] Pilotaż: ta sama scena i ustawienia, profil włączony/wyłączony, ujawnić koszt obserwacji i rzeczywisty viewport.
- [ ] Zamrozić wersję, pliki startowe, adapter, viewport, zasoby, fazy, kryteria oraz czas 120 s + 1800 s przed długą próbą.
- [ ] Wykonać długą próbę Release bez kompilacji i ciężkich zadań w tle. Zachować wszystkie próbki i końcowy świat. Zidentyfikować postęp, niedobory i stagnację.
- [ ] Odtworzyć końcowy świat w nowym procesie, sprawdzić bilanse i kontynuację; osobno istniejące natywne SC-11 i oba warianty konsekwencji M05.

### Task 3: końcowe dowody i status

- [ ] Jedna niezależna kontrola kodu, istotne uwagi RED/GREEN, pełne wymagane kontrole finalnego kodu.
- [ ] Raportować osobno poprawność, wydajność, postęp, granice. Jeśli bramka nie przeszła, naprawić przyczynę i ponowić porównywalną próbę; nie dopasować progu do wyniku.
- [ ] Zarchiwizować logi i hashe, zaktualizować jeden plan na podstawie dowodów i decyzji użytkownika. Commit lokalny; publikacja nie jest częścią tego zlecenia.
