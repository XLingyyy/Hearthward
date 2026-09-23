# TASK-035 validation

- Editor Development build: PASS.
- Python repository tests: 31/31 PASS.
- Native automation `Hearthward.NPCAgent`: 12/12 PASS.
- Runtime PIE `verify_episode_pie.py`: 21/21 PASS.

Verified episode:

```text
wood command
actual acquired = 2
actual delivered = 2
replans = 1
reason = SOURCE_POSITION_CHANGED
completed = true
evidence = 4 real event ids
```

The same command/evidence/projection and grounded recall line are reconstructed after SaveGame restore from persisted events.
