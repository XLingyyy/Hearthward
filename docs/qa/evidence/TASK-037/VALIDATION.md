# TASK-037 validation

- Editor Development build: PASS.
- Repository Python tests: 31/31 PASS.
- Native automation `Hearthward.NPCAgent`: 13/13 PASS.
- Runtime PIE `verify_coordination_prior_pie.py`: 28/28 PASS.

Observed profile transitions:

```text
2 follow                 -> unstable
3 follow                 -> stable follow (1.00)
3 follow + 1 hold        -> stable follow (.75)
rolling + 6 assist       -> stable assist (.75)
load saved boundary      -> stable follow (.75)
```

A stable prior alters only the refreshed suggestion set. It never overwrites the current companion order.
