# Wykonanie discrimination-v1

**Spec:** [protokół](discriminating-experiment.md). **Cel:** audyt, uczciwa próba A/R i jedna konserwacja przejmowana przez drugiego mieszkańca. Wykonanie w obecnym checkout na gałęzi `codex/discriminating-experiments`; bez nowej fizyki, zależności i zmiany starych protokołów. Zlecenie użytkownika obejmuje wykonanie; parametry są decyzjami autora.

## Global constraints

Zachować stare API i bajty formatów. Prawda tylko w aparacie/eksperymentatorze. Modele tylko z programów i pomiarów. Ocena bez uczenia. Równe sufity A/R. Fizyczne części i czynność w WorkWorld przez TraversalSession. Wszystkie kryteria w spec przed próbą. Zachować porażki i koszty. Red→green, pełne Debug/Release/ASan, Python, docs i format przed zamrożeniem.

### Task 1: Audyt i kontrakt

**Files:** `tools/audit_apparatus.py`, `tools/tests/test_apparatus_audit.py`, niniejszy plan/protokół, mapa dokumentacji.
**Interfaces:** produces `first_separator(a,b,inputs)`, `decision_tree(tables,inputs)`, `run()` oraz JSON audytu.

- [ ] Testy: para nierozstrzygalna do64, separator66, drzewo 4-modelowe głębokość2, nierozstrzygnięte grupy przy jednym wejściu. RED brak audytu, GREEN te asercje.
- [ ] Uruchomić pełne wyliczenie audytu, zachować JSON i docs-check. Expected: 2370 modeli, 1397 klas ograniczonych, głębokość7, nierozstrzygnięte0.
- [ ] Commit kontraktu przed implementacją i próbą.

### Task 2: Konkurencyjne modele i fizyczna sesja

**Files:** `src/research/contrast_agent.hpp/.cpp`, `contrast_session.hpp/.cpp`, `tests/contrast_tests.cpp`, CMake.
**Interfaces:** consumes shared grammar/world/traversal; produces `ContrastAgent(actor,active)`, `observe(evidence,shared)`, `choose(permutation)`, `predict(chain,input)`, `choose_chain(input,target)`, state/costs; `ContrastSession(law,active,seed)` with probe/advance/choice/contact/work/freeze and serialize/restore.

- [ ] RED: novel law compatible with F still has competitors from full grammar; witness split above64; selected minimax among all128, random shared permutation. Exhaustion rejects partial declarations. GREEN full native focused suite.
- [ ] RED: remote probe/share denied; two-stage probe consumes once; work only from measured exact output, one physical part consumed/80 ticks; contact provenance; transactional corrupt restore and deterministic pending continuation. GREEN same suite.
- [ ] Commit verified agent/session and API document.

### Task 3: Runner i niezależny sędzia

**Files:** `src/headless/contrast_main.cpp`, `tools/run_contrast_experiment.py`, `tools/contrast_judge.py`, `tools/tests/test_contrast_experiment.py`, `tests/contrast_integration.py`.
**Interfaces:** consumes session; produces case files with private truth and separate plans, native raw JSON, independent replay, `prepare/run/analyze` immutable directories.

- [ ] RED: changed measurement/choice/declaration/goal/manifest rejected; old protocol unchanged; repeat run denied; wrong freeze/provenance denied. GREEN stdlib Python and native fresh-process continuations.
- [ ] Execute 8 development worlds plus outside controls; preserve all results regardless of scientific outcome. No confirmatory generation yet.
- [ ] Commit validated infrastructure and development proof.

### Task 4: Review, próba i raport

**Files:** verification report/evidence, API, research agenda/development/README links and documentation map.
**Interfaces:** consumes frozen reviewed implementation; produces original raw confirmatory outcomes, signed/hash manifests, reproducible independent report.

- [ ] Independent whole-branch review, fix Important/Critical with failing regressions. Full suites, format, docs.
- [ ] Freeze clean source, native binary/build provenance, predeclared seeds and separate plans. Single 64-world confirmation plus16 outside controls; analyze independently, no retries to improve result.
- [ ] Archive portable frozen source/binary/data and checkpoints, SHA manifest; verify extraction/reanalysis, physical work and all costs. Commit honest report including unsupported hypotheses.

## Review Focus

Check partial inference after exhaustion, same domain and budgets for A/R, no task feedback entering knowledge, teaching provenance/contact and counterfactual costs, duplicate work/consumption after restore, journal capacity at pending phases, fresh-process continuation including work, and dependence of semantic identity on finite domain. Each is tested under its owning task; review also inspects untested boundary combinations.
