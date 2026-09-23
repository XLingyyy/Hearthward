# TASK-040 clean-tree branch review

Date: 2026-09-23

> This file supersedes the earlier failed-clean-tree snapshot. External release/acceptance wording is **TASK-029 AI NPC complete delivery**; TASK-040 remains the internal rework evidence identifier.

## Reviewed source before final main sync

- repository: `XLingyyy/Hearthward`
- working branch: `codex/ai-npc-vnext-rework-01-fix`
- source baseline: `origin/codex/ai-npc-vnext-rework-01@97f8818`
- target main: `73bb10ec4c19260cb72112c7e282a2c29f6c2432`
- main checkout with user changes was not switched, reset or overwritten

## Closed defects

The original default Unity build failure was reproduced and fixed.

Root cause:

- generic anonymous helper `Json` existed in both `HearthwardAgentInteraction.cpp` and `HearthwardNPCContextProjection.cpp`
- Unity concatenation caused C2084/C2264

Fix:

- unique module-prefixed helper names
- related anonymous helper audit completed
- duplicate generic `Counts` helpers in workshop/save were also renamed to module-prefixed names

Result: default `HearthwardEditor Win64 Development` build **Succeeded**.

## Current evidence

| Check | Result |
|---|---:|
| Editor Development Unity build | PASS |
| full native `Hearthward.*` | **41/41 PASS** |
| real Schema 2 → 3 disk migration | **1/1 PASS** |
| real Qwen M01–M16 clean/pressure | **32 cases, 32 generations, 32/32 safety PASS** |
| core M01–M10 raw contract | **20/20 PASS** |
| CTX-03 degradation | PASS — `compact_relevant`, 2832 tokens, one generation |
| CTX-04 required overflow | PASS — `required_minimal` 4020 tokens, zero generation |
| TASK-028 PIE | **49/49 PASS** |
| TASK-034 PIE | **16/16 PASS** |
| TASK-036 PIE | **16/16 PASS** |
| TASK-038 PIE | **26/26 PASS** |
| repository Python tests | 31/31 PASS before final main sync |
| repo validator | 0 errors before final main sync |
| Content/ | zero rework changes |

Committed evidence is under `docs/qa/evidence/TASK-040/`.

## Truthful model-result split

The real-model report keeps four layers separate: raw JSON, normalized/applied result, deterministic guardrail, and resulting world state.

M11/M12/M14/M16 can produce a broader raw write-intent under both clean and pressure states. Those raw-model imperfections are visible in `model-results.json[l]`; the final guardrail result is still safe and no prohibited world write occurs. They are not presented as raw-model successes.

## Remaining process

The code/evidence rework is closed. Before PR creation the branch still needs to sync current `origin/main`, resolve README/documentation conflicts, and rerun final candidate validation. PR may then be created for review; merge remains explicitly out of scope.
