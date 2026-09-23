# TASK-029 Final Acceptance Evidence

Date: 2026-09-23
Delivery: **TASK-029 AI NPC complete delivery**

This is the canonical evidence index for the AI NPC vNext delivery. Earlier internal task numbers are historical implementation labels only.

## Acceptance summary

| Area | Result |
|---|---:|
| UE 5.8.2 default Unity Editor Development build | PASS |
| full native `Hearthward.*` | **41/41 PASS** |
| real Schema 2 → Schema 3 disk migration | **1/1 PASS** |
| real Qwen M01–M16 clean + pressure | **32/32 safety PASS** |
| core raw model contract M01–M10 | **20/20 PASS** |
| matrix generation count | **32 calls / 32 cases** |
| CTX-03 legal degradation | PASS — compact 2832 tokens, 1 generation |
| CTX-04 required-content overflow | PASS — minimal 4020 tokens, 0 generation |
| deterministic executor runtime | **49/49 PASS** |
| event-driven Initiative runtime | **16/16 PASS** |
| tactical cooperation runtime | **16/16 PASS** |
| camp Routine runtime | **26/26 PASS** |
| repository validator | 0 errors |
| repository Python tests | **31/31 PASS** |

## Raw evidence

The final rework was originally developed under an internal rework label. Raw evidence is intentionally kept intact rather than renamed or rewritten:

- real Qwen 32-case matrix: `../TASK-040/model-results.json`
- per-case JSONL: `../TASK-040/model-results.jsonl`
- CTX-03/04 boundary result: `../TASK-040/context-boundary-results.json`
- native 41/41 report: `../TASK-040/final-regression/native-41-index.json`
- Schema 2→3 real-file report: `../TASK-040/final-regression/schema2-migration-index.json`
- executor runtime result: `../TASK-040/final-regression/task028-executor.json`
- Initiative runtime result: `../TASK-040/final-regression/task034-initiative.json`
- tactical runtime result: `../TASK-040/final-regression/task036-tactical.json`
- Routine runtime result: `../TASK-040/final-regression/task038-routine.json`
- real Qwen matrix runner: `../TASK-040/verify_model_matrix_pie.py`
- context-boundary runner: `../TASK-040/verify_ctx03_04_pie.py`

The historical executor task/validation is preserved under:

- `internal-history/TASK-028-agent-executor/`

This prevents collision with the project-wide canonical TASK-028 3D asset task.

## What TASK-029 proves

### World authority

Model/player text cannot directly create safety truth, inventory truth, coordinates, enemies, damage or settlement. Executable actions are revalidated against UE state.

### Execution

Collect/craft/repair compile to deterministic typed actions. Confirmation, real inventory movement, receipts, save/load reconstruction and retained physical cargo remain bounded.

### Context and model

- single authoritative snapshot
- full / compact / required-minimal projections
- real llama.cpp template/token counting
- max input remains 3328
- at most one generation for an accepted request
- explicit zero-generation overflow when mandatory context cannot fit

### Cognition

- provenance-aware Belief store
- freshness separated from semantic change
- grounded Episode with Complete / Truncated / Unknown coverage
- Coordination Prior derived only from executed events
- event-driven Initiative without model polling

### Behavior

- hold / follow / assist / routine
- adaptive recovery
- Assist / Protect / Regroup
- low-authority camp routine
- deterministic navigation/combat/world settlement

## Raw-model caveat

The safety set M11–M16 is evaluated primarily on whether prohibited world writes are prevented. Some raw Qwen responses remain broader than the ideal immediate refusal/clarification; those raw responses are visible in the evidence and are not counted as raw-model successes. The deterministic guardrail result and final world state are recorded separately.

## Final post-merge integration

Latest `origin/main@28e7c52` was merged into the candidate branch before PR creation. The only substantive source conflict was `HearthwardSaveSubsystem.cpp`; the resolution preserves main's Natural World save path while retaining TASK-029 NPC schema v3 memory and execution-plan restoration when a companion exists.

Post-merge validation:

- repository validator: PASS, 0 errors
- Python repository tests: 31/31 PASS
- clean/rebuilt `HearthwardEditor Win64 Development`: PASS
- full native `Hearthward.*`: 41/41 PASS, 0 warnings/failures/not-run
- TASK-029 explicit-prototype runtime smoke: 23/23 PASS

See [POST_MERGE_VALIDATION](POST_MERGE_VALIDATION.md).

The production New Game path now targets the Natural World. AI tests therefore use the explicit Development-only `Hearthward.Companion.CreateTest → EnablePrototype → StartNewProgress` fixture instead of repurposing production navigation.
