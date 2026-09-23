# TASK-033 validation

- Editor Development build: PASS.
- Python repository tests: 31/31 PASS.
- Native automation `Hearthward.NPCAgent`: 10/10 PASS.
- Runtime PIE `verify_belief_state_pie.py`: 25/25 PASS.

Key invariant verified in PIE:

```text
World truth may change while NPC is away
        ≠
NPC belief changes automatically
```

A player report of 99 wood leaves real storage unchanged. A later real storage transfer while the companion is away does not alter that report belief. Returning to camp replaces it with firsthand evidence. Save/load preserves both world truth and the potentially stale belief independently.
