# Independent M06 review

Reviewer: gpt-6-astra, fresh context, read-only. Base 63ae6fb, working tree candidate before final fixes. Verdict: REQUEST CHANGES. No native processes, builds or repository mutations by reviewer; adversarial analyzer cases used temporary directories.

## Findings

1. Important: actors_csv_ populated but missing from export_trace. Fresh runs declare complete export without actors.csv; native regression and evaluator fail.
2. Important: initial-world path comparison misses hard links. A report linked to seed.save is truncated before loading the seed. Reject equivalent existing files before opening outputs.
3. Important: incomplete/duplicated histories yield false acceptance. A single initial starving-actor sample passes; duplicating a final station row invents progress. Require known identities, uniqueness, chronology, consistent ticks and coverage with bounded gaps.
4. Important: evaluate/--analyze-dir bypasses initial-world hash and process evidence checks performed only in run. Empty final file passes existence. Centralize integrity validation and distinguish measurement gates from separately proven restoration/continuation.
5. Minor: negative alias tests omit mandatory save/report options and fail before alias validation. Use otherwise valid commands and an actual hard-link test.

No additional concrete defect found in lexical nested binding/exception handling or GPU ring S_FALSE/pending/skipped/disjoint handling. Optional profiling does not change serialized domain state.

## Declined to judge

- Final long-run performance, thermal stability, memory growth, actual progress and overhead: pilot/soak pending.
- Final Debug/Release/ASan results: executing independently.
- Actual forced delayed/disjoint GPU paths: no native run by reviewer; ordinary native success does not force those branches.
- Final-world continuation: final trial pending; helper checkpoints within one process, requiring retained separate post-trial invocation.
- Human trials/gameplay interest/playability: excluded by user's instruction.

Review does not establish completion of M06. Executor must retain failing evidence, fix findings with regressions and measure the final candidate.
