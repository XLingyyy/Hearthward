# TASK-038 validation

- Editor Development build: PASS.
- Repository Python tests: 31/31 PASS.
- Repository validator: 0 errors.
- Native `Hearthward.NPCAgent`: 14/14 PASS.
- Full native `Hearthward.`: 39/39 PASS.
- Runtime PIE `verify_camp_routine_pie.py`: 26/26 PASS.
- TASK-028 executor regression PIE: 49/49 PASS.

Verified arbitration:

```text
routine authorized + idle -> real camp movement
explicit wait/follow     -> routine disabled
confirmed routine        -> routine enabled
typed task               -> routine suspended, executor owns navigation
task cancel              -> routine resumes
save/load                -> routine authorization restored
```
