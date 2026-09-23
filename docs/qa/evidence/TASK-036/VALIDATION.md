# TASK-036 validation

- Editor Development build: PASS.
- Repository Python tests: 31/31 PASS.
- Repository validator: 0 errors.
- Native automation `Hearthward.NPCAgent`: 12/12 PASS.
- Runtime PIE `verify_tactical_cooperation_pie.py`: 16/16 PASS.

Observed:
- healthy assist selects guard_1 and applies real UE damage;
- low-health single-threat state -> protect / PROTECT_LOW_HEALTH;
- low-health multi-threat state -> regroup / LOW_HEALTH_OVERWHELMED;
- regroup closes companion-player distance from 500cm to ~89.5cm;
- player down -> regroup / PLAYER_DOWN_REGROUP;
- downed regroup closes 500cm to ~142cm.
