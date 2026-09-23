# TASK-029 AI NPC internal history

This directory preserves the implementation lineage that was developed under temporary AI task labels before the Owner unified the external delivery as **TASK-029**.

These labels are historical only. They do **not** claim the project-wide canonical task numbers. In particular:

- canonical `TASK-027` on current main is **character assets and locomotion/action animation**;
- canonical `TASK-028` on current main is **3D asset import/gameplay integration**;
- the complete AI NPC vNext delivery is canonical `TASK-029`.

## Historical implementation map

| Historical AI label | Capability |
|---|---|
| AI-027 | authoritative perception and safety |
| AI-028 | typed Goal → Plan → Action executor |
| AI-030 | deterministic companion combat policy |
| AI-031 | user retest fixes |
| AI-032 | adaptive replanning/recovery |
| AI-033 | provenance-aware Belief state |
| AI-034 | event-driven Initiative |
| AI-035 | grounded Episode projection |
| AI-036 | tactical Assist / Protect / Regroup |
| AI-037 | bounded Coordination Prior |
| AI-038 | low-authority camp Routine |
| AI-039 | runtime/behavior/navigation componentization |
| AI-040 | bounded context, cognition freshness, episode coverage, Schema 3 and final rework |

## Layout

- `tasks/` — archived historical task specs, renamed with an `AI-TASK-` prefix.
- `handoffs/` — archived historical handoffs, renamed with an `AI-TASK-` prefix.
- `executor/` — original historical executor task and runner.
- `context-cognition/` — final real-Qwen matrix, context boundary evidence, Schema migration and final regression evidence.
- `../regression/` — current canonical TASK-029 runners adapted to the latest main's explicit Development-only fixture flow.

Historical evidence is retained for auditability; current acceptance claims must be read from `../FINAL_ACCEPTANCE.md`.
