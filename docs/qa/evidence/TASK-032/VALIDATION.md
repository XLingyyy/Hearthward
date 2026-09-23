# TASK-032 validation

- Repository Python tests: 31/31 PASS.
- Repository validator: 0 errors.
- Editor Development build: PASS; new recovery module compiled and project DLL relinked.
- Native automation filter `Hearthward.NPCAgent`: 9/9 PASS.
- Runtime PIE `verify_adaptive_recovery_pie.py`: 11/11 PASS.

Runtime scenario: collection starts normally, the authoritative source actor is moved during Gather, executor enters `Recovery:Replan->MoveTo:Source`, reacquires the moved source, gathers two real wood, returns to camp and deposits exactly two. No manual retry is issued and no progress is credited during replanning.
