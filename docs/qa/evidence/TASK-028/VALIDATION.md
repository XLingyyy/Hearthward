# TASK-028 validation snapshot

Date: 2026-09-22
Base: TASK-027 local commit `25e8f13`
Branch: `codex/TASK-028-agent-executor`

## Implemented

- Added deterministic `HearthwardAgentPlan` compiler for existing write capabilities:
  - collect → MoveTo(Source) → Gather → MoveTo(Camp) → Deposit
  - craft/bag → MoveTo(Workshop) → CommitWorkshop → MoveTo(Camp) → Deposit
  - craft/camp → MoveTo(Camp) → TakeMaterials → MoveTo(Workshop) → CommitWorkshop → MoveTo(Camp) → Deposit
  - repair/bag → MoveTo(Workshop) → CommitWorkshop
  - repair/camp → MoveTo(Camp) → TakeMaterials → MoveTo(Workshop) → CommitWorkshop
- Companion execution is now driven by a runtime typed-action cursor. `EHearthwardCompanionPhase` remains as HUD/save compatibility projection.
- Save restore rebuilds the runtime plan cursor from the persisted canonical goal + legacy phase/progress fields; no save schema change was introduced.
- TASK-027 perception/safety is rechecked before side-effecting source/workshop actions. Camp return/deposit remains a recovery path.
- `GetExecutionAction()` and the filtered LLM observation expose the current typed action for diagnostics.
- Additional-acquired semantics remain canonical for ordinary pre-existing cargo. A special compatibility rule handles **physical cargo retained from a cancelled/superseded command**: a replacement collect goal may explicitly adopt up to its requested amount, while any surplus remains in the NPC bag. This preserves real inventory conservation without fabricating acquisition.

## Verification

| Check | Result |
|---|---|
| UE 5.8.2 HearthwardEditor build | PASS |
| `git diff --check` | PASS |
| `python scripts/validate_repo.py` | PASS, 0 errors |
| Python repository tooling | PASS, 31/31 |
| Full native `Hearthward` automation | PASS, 31/31 on TASK-028 commit; 32/32 after TASK-029 suggestion test was added |
| TASK-027 perception/safety PIE after executor refactor | PASS, 27/27 |
| TASK-028 typed executor PIE | PASS, 49/49 after retained-cargo compatibility coverage |
| TASK-025 deterministic workshop regression | PASS, 129/129 on current executor branch |
| Historical TASK-012 companion PIE | PASS, 45/45 checks after retained-cargo compatibility and development-console paused-bootstrap fix |

## Executor PIE coverage

`verify_executor_pie.py` validates:

- collect plan transitions including pre-existing cargo cleanup;
- multi-trip real acquisition and delivery;
- active-command save + reload with plan-cursor reconstruction;
- no duplicate acquisition/deposit after restore;
- finite source shortage: requested 8, only 6 physically available → exactly 6 acquired/delivered, then same plan waits blocked;
- cancelled-command retained cargo: replacement `collect 1` delivers exactly one retained physical unit and leaves the remaining three in the bag;
- bag craft: real output exists in companion bag before deposit, then exactly delivered to camp;
- bag repair: completes without a deposit action and applies the real material/durability transaction.

## Historical TASK-012 compatibility note

TASK-012 exposed two useful compatibility details during the executor refactor. First, a replacement `collect 1` historically reuses one unit of **real cargo retained from a cancelled command**. TASK-028 now preserves that behavior explicitly and conservatively: only up to the new requested amount is adopted; surplus remains physical cargo in the NPC bag. Second, the bootstrap can reopen a PIE world paused; the development-only `Hearthward.Companion.Collect` console command now resumes the simulation before issuing its structured test goal, restoring the old manual-console fixture behavior without weakening normal player/LLM safety gates.

## Remote / workflow

GitHub Issue, reviewer, push and PR are not claimed here. The current conversation's GitHub connector remains unavailable; the user explicitly authorized continued local staged development followed by one reviewable PR with separate task commits.
