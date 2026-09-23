# TASK-029 Latest-main Finalization

Date: 2026-09-23

## Integration inputs

- latest main baseline: `ba547c0ee5a1d8dae41e747a5d1d0674d7702899` (PR #35 hero/animation integration)
- PR #34 natural-camp head merged into final candidate: `25d53d80ab76472d71c2888b4c010ed8159da3c7`
- latest-main AI core checkpoint before natural-camp merge: `2d1106d7a4eb482943bd5e0e16b708221943cb36`

The final candidate keeps the natural-camp AI implementation and latest-main TASK-027 character/animation state in one tree. The merge resolved the actual overlaps without restoring historical AI TASK-027/028 documents over the canonical task namespace.

## Latest-main AI core verification

Before the natural-camp branch was merged into the final tree, the isolated `main@ba547c0` AI candidate produced:

| Check | Result |
|---|---:|
| UE 5.8.2 Editor Development build | PASS |
| repository Python tests | **31/31 PASS** |
| full native `Hearthward.*` | **41/41 PASS** |
| Schema 2 → 3 real-file migration | **1/1 PASS** |
| deterministic Executor | **49/49 PASS** |
| event-driven Initiative | **16/16 PASS** |
| tactical cooperation | **16/16 PASS** |
| camp Routine | **26/26 PASS** |
| explicit Development runtime smoke | **23/23 PASS** |

The runtime smoke covers explicit fixture creation, prototype/new-progress start, suggestion non-mutation, typed collect confirmation, mid-task save/load reconstruction, exact 2/2 completion and exact +2 shared-storage settlement.

## Natural-camp integration evidence

The PR #34 natural-camp head independently recorded:

- Editor Development build: PASS
- native automation: 42/42
- Python: 31/31
- natural-map collection/save: 22/22
- workbench/cross-map save-load harness: 23 checks
- legacy natural-save upgrade: 12/12
- real keyboard/mouse dialogue → candidate confirmation → 2 wood delivery

See `natural-camp-integration/REPORT.md` and `revalidation-20260923/REPORT.md`.

## TASK-036 regression correction

The first latest-main tactical rerun exposed a **test-script assumption**, not a production tactical regression.

The old runner derived `guard_1` / `guard_2` from camp coordinates. Current gameplay creates encounter actors relative to the player `Origin` captured when adventure is enabled. The regression runner was updated to derive encounter probes from that player origin. Production tactical code was not changed; the rerun returned **16/16 PASS**.

The corrected runner is kept in the TASK-029 regression evidence and mirrored into the current tactical validation scripts.

## Repository validator

`python -X utf8 scripts/validate_repo.py` is **not** recorded as a project-wide PASS.

It reports 9 errors, all in canonical TASK-026/027/028 workflow metadata:

- TASK-026: reviewer missing;
- TASK-026: real Issue URL missing;
- TASK-027: reviewer missing;
- TASK-027: real Issue URL missing;
- TASK-026: one unknown required-test reference;
- TASK-028: four descriptive strings used as unknown required-test references.

A clean worktree at `main@ba547c0` reproduces the identical 9 errors. Therefore these are main-baseline workflow issues, not TASK-029 regressions.

The task-scoped command also records two process constraints in the isolated verification worktree: the worktree is detached, and TASK-029 does not exist as an approved snapshot at the `ba547c0` baseline. These are recorded rather than bypassed.

## Final-tree gate

After the natural-camp merge, the final tree must rerun build/native/Python/runtime smoke before push. Previous results remain correctly attributed to their source snapshots until that rerun completes.

PR #34 remains open for independent Reviewer / Owner acceptance. **Agent does not merge main.**
