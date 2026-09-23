# TASK-029 Post-merge Validation

Date: 2026-09-23
Candidate branch: `codex/ai-npc-vnext-rework-01-fix`
Integrated main: `origin/main@ba547c0ee5a1d8dae41e747a5d1d0674d7702899`

## Result

The TASK-029 AI core was re-integrated on the latest main baseline and then combined with the PR #34 natural-camp line. The integration preserves the relevant save/runtime boundaries:

- main natural-world save support can capture/restore without a companion fixture;
- TASK-029 keeps NPC schema v3 memory, command state, receipts and execution-plan restoration when a companion is present.

## Final integrated checks

| Check | Result |
|---|---:|
| repository validator | **Baseline-blocked** — identical 9 pre-existing main workflow-metadata errors |
| repository Python tests | PASS, 31/31 |
| clean/rebuilt UE 5.8.2 HearthwardEditor Development | PASS |
| full native `Hearthward.*` | **41/41 PASS**, 0 warnings/failures/not-run |
| TASK-029 post-merge runtime smoke | **23/23 PASS** |

## Post-merge runtime smoke

The smoke intentionally uses the explicit Development-only prototype fixture:

```text
Hearthward.Companion.CreateTest
→ EnablePrototype
→ StartNewProgress
```

It does **not** reuse the production “New Game” action, because main now correctly routes New Game into the Natural World.

Verified:

- prototype requires an explicit companion fixture;
- companion fixture can be created explicitly;
- prototype save campaign starts successfully;
- TASK-029 suggestions begin empty and only refresh explicitly;
- refresh produces exactly three suggestions;
- refresh does not mutate memory, last input, filtered context or candidate state;
- a typed collect goal still requires stage + confirmation;
- a collect can be saved while carrying real acquired cargo;
- loading the in-flight save preserves acquired/carried/delivered/source/storage state;
- execution plan is rebuilt after load;
- the resumed task completes exactly 2/2;
- final shared storage increases by exactly two units.

Raw local result: `Saved/Task029/post-merge-runtime-smoke.json`.
Latest-main core runner: `docs/qa/evidence/TASK-029/regression/verify_runtime_smoke.py`.
Latest-main core rerun: **23/23 PASS**.

## Repository validator note

`python -X utf8 scripts/validate_repo.py` reports 9 errors, and a clean `main@ba547c0` worktree reproduces the identical set. They are limited to canonical TASK-026/027/028 workflow metadata, so TASK-029 does not claim a project-wide validator PASS.

The task-scoped validator additionally records the detached verification worktree and missing TASK-029 baseline snapshot rather than bypassing those process constraints.

See [LATEST_MAIN_FINALIZATION](LATEST_MAIN_FINALIZATION.md) for the latest-main and natural-camp merge attribution.

## Important integration note

During main sync, Natural World LFS assets were intentionally not smudged into this isolated AI worktree. That does not affect source/native validation. AI runtime testing therefore uses the existing explicit prototype fixture rather than attempting to launch the production Natural World from LFS pointer files.

The candidate does not modify or replace those main assets; it inherits them from main.
