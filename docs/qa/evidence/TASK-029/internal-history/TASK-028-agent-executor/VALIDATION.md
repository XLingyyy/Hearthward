# Historical AI executor validation

> Historical internal task label: TASK-028. The project-wide canonical TASK-028 is now the 3D asset integration task. For AI NPC delivery, this evidence belongs to **TASK-029 AI NPC complete delivery**.

Date: 2026-09-22
Base: TASK-027 local commit `25e8f13`
Historical branch: `codex/TASK-028-agent-executor`

## Implemented

- Added deterministic `HearthwardAgentPlan` compiler for existing write capabilities:
  - collect → MoveTo(Source) → Gather → MoveTo(Camp) → Deposit
  - craft/bag → MoveTo(Workshop) → CommitWorkshop → MoveTo(Camp) → Deposit
  - craft/camp → MoveTo(Camp) → TakeMaterials → MoveTo(Workshop) → CommitWorkshop → MoveTo(Camp) → Deposit
  - repair/bag → MoveTo(Workshop) → CommitWorkshop
  - repair/camp → MoveTo(Camp) → TakeMaterials → MoveTo(Workshop) → CommitWorkshop
- Companion execution is driven by a runtime typed-action cursor; phase remains a compatibility projection.
- Save restore rebuilds the runtime plan cursor from persisted goal + phase/progress fields.
- Perception/safety is rechecked before side-effecting source/workshop actions.
- Retained cargo compatibility preserves physical inventory without fabricating acquisition.

## Historical verification

| Check | Result |
|---|---|
| UE 5.8.2 HearthwardEditor build | PASS |
| repository validation | PASS |
| Python repository tooling | 31/31 PASS |
| native Hearthward automation | 31/31, later 32/32 |
| perception/safety PIE | 27/27 PASS |
| typed executor PIE | 49/49 PASS |
| workshop deterministic regression | 129/129 PASS |
| historical companion PIE | 45/45 PASS |

The same executor was revalidated during the final TASK-029 rework and again passed 49/49 runtime checks.
