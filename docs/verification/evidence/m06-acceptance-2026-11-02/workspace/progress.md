# SDD ledger — plan: docs/work-items/m06-technical-acceptance.md
Base: 63ae6fb. Scope: user's 02.11.2026 technical M06 acceptance; no participant gate.
Ruling: continue existing clean codex/verified-capabilities-plan branch and build caches — continuation of existing M06 work, no main mutation — costs shared preparatory history, final diff reviewed from 63ae6fb.
Ruling: user replaced the human-trial gate; retain historical reports and update current owners — explicit latest instruction — future product appeal remains untested, no claim of gameplay validation.
Pre-flight: task 1 provides profile CSV/counters consumed by task 2; preserve old trace schema, frame IDs join GPU asynchronously. Task 3 records both technical outcomes and user acceptance separately.

Task 1 RED native profile: old executable exit 1, log red-native.log. Nested observer RED: profile_costs failed isolation, fixed with saved depth stack; GREEN. Initial-world RED: unsupported option exit 1; GREEN file byte identity in native test.
Ruling: add --initial-world and station progress CSV to task 1 — needed for frozen provisioned worlds and evidence of actual activity — slightly wider CLI/observer scope, alias rejection and byte-preservation regression required.

Task 2 analyzer RED: missing module; GREEN 6 tests. Extreme-need regression RED (incorrect pass), GREEN 8 tests, adds per-actor CSV to expose persistent starvation/exhaustion rather than only aggregate progress.
First full Release: 432/434, failures contrast.native_case (binary source fingerprint stale after editing tools) and tooling.tests (new analyzer test discovered before module created). Retain log; freeze code and reconfigure all profiles before final suites. No test weakened.
Pilot 1, baseline and repeat each 60 s, 0 warmup, same prepared save, MX330, viewport 844x781. First profiled run passed, repeat failed frame_p95_ms=22.3983 and p99_ms=35.2771; no thresholds changed; profile instrumentation subsequently gained actor history, requiring final-candidate pilot before soak. Not a general causal overhead benchmark.

Independent read-only reviewer gpt-6-astra REQUEST CHANGES: 4 Important (missing actors export, hard-link input corruption, incomplete/duplicate history acceptance, reanalysis identity bypass); 1 Minor (alias tests stopped at missing mandatory args). Regraded alias coverage as Important because it is the required regression for input-corruption protection. All findings enter single TDD fix pass.
RED hardlink demo: input shrank 290432 to 34 bytes, preserved=False (copied test fixture only). RED actor export: native_runtime_profile failed in full candidate Release 433/434. RED analyzer: five new regressions failed. Shared evaluator now checks process/protocol/world identity, complete unique histories with <=1500ms gaps, and measurement-only scope; 13/13 unit tests GREEN. Native fixes await rebuilt test.
Final: Ruling: long-run performance/thermal behavior/memory/progress/overhead and final suites deferred by reviewer — require final binary pilot, soak and suites before completion — failure leaves affected gate open.
Final: Ruling: forced delayed/disjoint GPU device behavior not executed by reviewer — retain explicit statuses and no-wait design, report normal native coverage only — unusual device paths remain less directly tested.
Final: Ruling: final continuation pending in review — require separate fresh-process helper on actual final save plus existing native F9 — acceptance would otherwise miss save errors.
Final: Ruling: human trials excluded by user — no participant gate or product-appeal claim — commercial/playability questions remain unmeasured.
Ruling: add diagnostic --forecast to test helper (38400 world ticks, not CTest or FPS benchmark) before long run — existing record_limit=4096 and pilot150 events/min suggest capacity risk; exercise same world before spending32 real minutes — new diagnostic path not independently re-reviewed, final suites still required.

Unexpected domain defect: forecast with per-tick validation RED at915 (route or transfer without goal). Reproduction retained before-failure save and log. m06.completed_target RED with 914+1 ticks, GREEN after needs.cpp zero-wear Maintain branch releases navigation and clears route/leg/step/requested goal. No format/balance change. Diagnostic forecast rerun through38400 and checkpoint comparison pending. This is necessary technical acceptance repair, beyond reviewer snapshot; covered by retained regression and full final profiles.

Task 1: complete on final source: Debug435/435 (237.03s), Release435/435 (87.90s), ASan437/437 (332.16s), unchanged limits. Native hardlink/aliases and actor export GREEN. Analyzer13/13 GREEN, all five review regressions retained; review fixes validated with full suites. No deferred minor (alias test issue regraded Important, fixed).
Forecast38400 world ticks validated after every step, checkpoint19200 continuation byte-identical, final3879 events below unchanged4096 cap. Not an FPS/thermal benchmark.

Final code commit3b95471. All profiles435/435,435/435,437/437; Python71/71. Final baseline65s exit0 p99=17.0657ms. Final profile65s complete identities/progress, all functional gates passed, frame_p99 failed34.557ms. Slow frames concentrated5–15s, all later5s buckets after20s p99<=17.12ms. Cause remains unproven.
Ruling: execute the independently pre-frozen120s+1800s protocol despite failed5s-warmup pilot — long protocol's120s warmup was committed in intent before pilot and all warmup data remains retained; no threshold/binary/sample filtering change — cold-start smoothness is not established, recurring steady-phase spikes must fail the long gate.

Task 2: complete. Frozen long protocol3b95471:120+1800s, MX330844x781,115111 samples (7198 warmup,107913 measured),36000 measured ticks. p95=16.9433ms p99=19.9069ms max51.8887ms, all fixed gates PASS. Activity windows154/155/149/148/151/155, total976. Extreme autonomous need9083ms recovered, no gate>60s. World3879 events under4096 limit.
Post-trial helper200/100 continuation exact; verified Python-launched native community resume exit0, community_resume=pass,TRAVERSAL SMOKE PASS, source SHA e7d2010bdb2198b4911552694db3ea19c333b64a7a958876a46b6f99ffa72444 unchanged. Initial PowerShell GUI launch was non-waited; its printed exit wasn't authoritative, verified rerun is retained.
Final source hashes still identical to source-sha256 manifest. Image inspected, hardware-rendered world visible, no claim of full3D physics. Task3: final documentation/evidence archive and commit pending.

Final evidence report records all six execution rulings with costs and review limitations. Source code unchanged after3b95471; only current documentation owners updated after measured result. Final archive preserves failures and final data; no push/merge. Task3 final docs/byte checks underway.

Ruling: preserve the raw review package as .txt in evidence, bytes identical to original .md — its heading starts M06 and the normative-doc checker treated it as a competing milestone definition — raw prompt remains inspectable, but not registered/rendered as a current Markdown specification. The initial docs-check failure is retained. Checker rules unchanged.

Task 3: complete. Final documentation PASS; Python71/71 (21.844s) after archive registration. All engine checks and native continuation already complete. Archive is synchronized byte-for-byte below, manifest excludes its own recursive hash; local final commit follows. No deferred minors, no push or merge.
