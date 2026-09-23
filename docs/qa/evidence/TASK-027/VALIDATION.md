# TASK-027 validation snapshot

Date: 2026-09-22
Base: `b1f85525697b79e6017455decab9d79a54977834`
Branch: `codex/TASK-027-npc-perception`
Tested commit: **UNCOMMITTED WORKTREE** — do not treat these results as commit-bound evidence.

## Implemented surface

- Added `HearthwardNPCPerception` as a small deterministic seam:
  - `Capture(companion)` reads UE-authoritative runtime observation.
  - `Evaluate(observation, goal)` is a pure fail-closed safety policy.
- Observation covers world availability, pause, combat-state availability/current combat, camp/source availability, prototype safe-source evidence, navigation rebuild state, camp/source distances and current execution phase.
- Collection requires a known safe source and camp; own-bag craft/repair no longer incorrectly depends on the collection source; camp-authorized material use requires camp availability.
- Candidate preview/confirmation and active collection/workshop phases re-evaluate current observation.
- Local-AI filtered context exposes the same observation and collection safety verdict used by execution.
- Existing `bSourceSafe` remains only a PROTOTYPE_ONLY known-safe-source observation input; it is no longer read throughout execution as the final safety decision.

## Verification

| Check | Result |
|---|---|
| `git diff --check` | PASS |
| `python scripts/validate_repo.py` | PASS, 0 repository errors |
| `python -m unittest discover -s scripts/tests -v` | PASS, 31/31 |
| UE Editor build | PASS on installed UE 5.8.2 using `-NoHotReloadFromIDE` |
| `Hearthward.NPCAgent` native automation | PASS, 5/5 |
| Full `Hearthward` native automation | PASS, 30/30 |
| TASK-027 deterministic PIE safety regression | PASS, 27/27 checks |
| TASK-025 deterministic workshop regression | PASS, 129/129 checks |
| Real-model regression | NOT_RUN in this worktree (GGUF not present) |

### UE build and automation

Detected installed Epic engine: `D:\\UE5.8\\UE_5.8`, manifest version `5.8.2-56702186`. The repository target is now aligned to UE 5.8.2, matching this recorded build.

The first build attempt was blocked because another Unreal Editor instance still held the shared Live Coding mutex. Rather than closing the user's unrelated editor, the build was rerun with UnrealBuildTool's documented command-line switch `-NoHotReloadFromIDE`:

```text
D:/UE5.8/UE_5.8/Engine/Build/BatchFiles/Build.bat HearthwardEditor Win64 Development
-Project=C:/Users/Lenovo/.devspace/worktrees/Hearthward-fec9c075/Hearthward.uproject
-WaitMutex -NoHotReloadFromIDE
```

Result: **Succeeded**. The first compile exposed one real C++ error (`IsNavigationBuildInProgress` called through a const pointer); the pointer was corrected and the second build linked `UnrealEditor-Hearthward.dll` successfully.

Native automation was then executed through `UnrealEditor-Cmd.exe -NullRHI`:

- `Hearthward.NPCAgent`: **5/5 PASS**, including the new `PerceptionSafety` case.
- Full `Hearthward`: **30/30 PASS**, 0 failed, 0 not-run.

Reports were written under `Saved/Task027/Automation*` and are generated evidence rather than tracked source files.

### PIE integration

`docs/qa/evidence/TASK-027/verify_perception_pie.py` ran against the real development fixture and gameplay combat state. Result: **27/27 PASS**. It verifies that a task card is revalidated after safety changes, pause/unpause, and real combat entry/exit; source-safety or combat changes during a five-second gather prevent acquisition/deposit side effects and return the companion through the existing safe recovery path.

The pre-existing deterministic TASK-025 workshop regression was also rerun in `workshop` mode: **129/129 PASS** across three repetitions, covering own-bag/camp material authority, craft/repair settlement, save/load receipts, disappearing materials, rule changes, and replacement-task behavior.

The local Qwen GGUF is intentionally absent from the isolated worktree, so no real-model generation run is claimed for this snapshot.

## Workflow / remote status

The requested GitHub connector was invoked again and returned:

`FORBIDDEN: This conversation is restricted to developer MCPs`.

Consequently no TASK-027 Issue, remote branch, PR, review request, commit, push or merge is claimed in this snapshot.
