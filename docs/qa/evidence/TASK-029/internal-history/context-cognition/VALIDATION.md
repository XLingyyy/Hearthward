# TASK-040 Validation

> **External acceptance alias:** per Owner instruction on 2026-09-23, the final AI NPC vNext delivery is reported externally as **TASK-029 AI NPC complete delivery**. TASK-040 remains the internal rework/evidence identifier and does not replace the historical TASK-027～040 task trail.

Date: 2026-09-23
Working branch: `codex/ai-npc-vnext-rework-01-fix`
Source baseline before final main sync: `origin/codex/ai-npc-vnext-rework-01@97f8818`
Target main: `73bb10ec4c19260cb72112c7e282a2c29f6c2432`

## Final technical result before main sync

| Check | Result | Evidence |
|---|---:|---|
| UE 5.8.2 Editor Development build | PASS | default Unity build succeeded after helper collision cleanup |
| full native `Hearthward.*` | **41/41 PASS** | `final-regression/native-41-index.json` |
| real Schema 2 → Schema 3 disk migration | **1/1 PASS** | `final-regression/schema2-migration-index.json` |
| real Qwen M01–M16 clean + CTX-02 pressure | **32/32 safety PASS** | `model-results.json`, `model-results.jsonl` |
| Qwen core raw contract M01–M10 | **20/20 PASS** | same model evidence |
| total real generation calls in 32-case matrix | **32** | exactly one per case; no normal overflow |
| CTX-03 legal multi-turn degradation | PASS | full → `compact_relevant`, 2832 tokens, one generation, restriction preserved |
| CTX-04 required-content overflow | PASS | `required_minimal` 4020 tokens, `generation_calls=0`, no candidate/world write |
| TASK-028 Executor PIE | **49/49 PASS** | `final-regression/task028-executor.json` |
| TASK-034 Initiative PIE | **16/16 PASS** | `final-regression/task034-initiative.json` |
| TASK-036 Tactical Cooperation PIE | **16/16 PASS** | `final-regression/task036-tactical.json` |
| TASK-038 Camp Routine PIE | **26/26 PASS** | `final-regression/task038-routine.json` |
| repo validator | PASS | 0 errors on current rework source before final main sync |
| repository Python tests | PASS | 31/31 before final main sync |
| Content/ binary changes | PASS | zero changes in this rework |

## R0 — Unity collision and helper audit

The clean committed candidate originally failed default Unity compilation because two anonymous-namespace helpers named `Json` were concatenated into the same Unity translation unit.

Fixes:

- `HearthwardAgentInteraction.cpp`: `Json/Object` → module-prefixed helpers.
- `HearthwardNPCContextProjection.cpp`: all generic anonymous helpers received `ContextProjection*` prefixes.
- final audit also removed the remaining duplicated anonymous helper name `Counts`:
  - workshop helper → `WorkshopCounts`
  - save helper → `SaveSubsystemCounts`

Default `HearthwardEditor Win64 Development` then compiled and linked successfully.

## R1 — bounded ContextProjection and real token budget

The final path captures one UE-authoritative snapshot and produces at most three deterministic projections:

1. `full_relevant`
2. `compact_relevant`
3. `required_minimal`

Every attempted tier is counted using the real llama.cpp `/apply-template` → `/tokenize` path. Only the accepted, already-counted body can enter generation. `max_input_tokens=3328` and output limit 256 remain unchanged.

The pressure fix deliberately stopped treating “full” as “dump every belief”. Irrelevant camp beliefs are filtered, and collect requests no longer inject current camp stock just because the phrase contains “带回仓库”. Generic crafting recipe material/output quantities were also removed from the always-on capability prompt because they caused the model to confuse request quantities with recipe quantities.

### CTX-01 / CTX-02 and 32-case real-Qwen matrix

`model-results.json` records M01–M16 twice: clean and CTX-02 pressure state.

Summary:

- 32 cases
- 32 generation calls
- 32/32 final safety/behavior checks pass
- M01–M10 core raw model contract: 20/20 pass
- no normal `CONTEXT_OVERFLOW`
- M11/M12/M14/M16 raw model JSON can still be broader than the ideal refusal/clarification intent, but deterministic guardrails reject or clarify correctly; these are reported as raw-model imperfections, not hidden as model success.

Pressure state includes 64 player records with 4 agreements, 22 beliefs and 128 events. It completes without dumping all history into the model.

### CTX-03

A legal four-turn clarification history is combined with large optional pressure context.

Final probe:

- full tier exceeds the budget and degrades
- accepted tier: `compact_relevant`
- actual input tokens: 2832
- generation calls: 1
- raw model still returns clarification
- unknown North Mountain / oral-safety constraints remain preserved
- no executable candidate
- world authority unchanged

### CTX-04

A legal ≤1000-character required player utterance plus valid clarification history is deliberately constructed with high tokenizer density.

Final result:

- `required_minimal`: 4020 tokens > 3328
- reason: `CONTEXT_OVERFLOW`
- generation calls: **0**
- no candidate
- no world write
- original player text retained exactly

Evidence: `context-boundary-results.json`.

## R2 — capability contract and natural-language guardrails

- capability registry remains the capability source of truth.
- routine is represented consistently in registry/schema/prompt/preflight.
- generic prompt no longer dumps recipe material/output amounts.
- Chinese numeric quantities are explicit quantities; “四份” and “十份” are not treated as missing numbers.
- inventory report guardrail now accepts explicit Chinese quantity forms while keeping the report cognition-only.
- inventory query vs inventory report remains separated.
- negative/fraction quantities, unknown locations, oral safety claims, multi-goal requests and rule conflicts cannot silently become executable world writes.

## R3 — Belief freshness

The rework keeps:

- `RecordedAt` = semantic value/source change time
- `LastEvidenceAt` = newest valid evidence time
- freshness-only evidence refresh does not churn semantic revision
- value/source changes still advance semantic state
- off-camp queries do not leak changed world inventory

## R4 — Episode coverage

Coverage remains separate from task terminal state:

- `Complete`
- `Truncated`
- `Unknown`

Active-command coverage survives event-ring eviction; legacy history does not get falsely promoted to complete.

## R5 — real Schema 2 → Schema 3 file migration

A dedicated native automation now creates an actual Schema 2 `.hws` file on disk, serializes it through the normal save payload/header format, reads it through `HearthwardSave::Read`, migrates it, writes a Schema 3 file and reloads that file.

Verified:

- Schema promoted to 3
- NPC cognition state promoted to v3
- legacy `LastEvidenceAt` conservatively initialized from `RecordedAt`
- historical command coverage becomes `Unknown`
- campaign binding remains unchanged
- migrated Schema 3 file passes strict validation after round trip

Automation: `Hearthward.Save.Schema2To3RealFileMigration` — 1/1 PASS.

## R5 — final runtime regressions

Current source was re-run, not merely credited with historical results:

- TASK-028 Executor: 49/49 PASS
- TASK-034 Initiative: 16/16 PASS
- TASK-036 Tactical Cooperation: 16/16 PASS
- TASK-038 Camp Routine: 26/26 PASS

The full native suite is now 41/41 because the Schema 2→3 real-file migration automation adds one new test to the prior 40-test suite.

## R6 — remaining release process

Internal implementation/evidence is complete enough for PR preparation. Remaining release steps are procedural/integration steps:

1. sync `origin/main@73bb10e+`
2. resolve README/documentation conflict without dropping main asset/documentation changes
3. rerun final repository/build/native smoke validation on the merged candidate
4. update project state / PR handoff with the final candidate SHA
5. push the dedicated rework branch and create a PR
6. **do not merge**

Independent Reviewer / Owner experience acceptance remains outside the executing agent's authority.

## External TASK-029 acceptance wording

The PR/final report should say:

> **TASK-029 AI NPC complete delivery** — internally implemented and evidenced across TASK-027～040.

The internal task numbers remain available for traceability and should not be renumbered or deleted.
