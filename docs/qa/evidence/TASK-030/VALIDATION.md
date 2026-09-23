# TASK-030 validation

Date: 2026-09-22
Branch: `codex/TASK-030-companion-combat-policy-stack`
Baseline: `79c1147 feat: add contextual NPC suggestions`

## Build

HearthwardEditor / Win64 / Development built successfully with UE 5.8 toolchain.

The final incremental build after adding the development-only local-AI bundle override compiled and linked:

- `HearthwardLocalAISubsystem.cpp`
- `UnrealEditor-Hearthward.lib`
- `UnrealEditor-Hearthward.dll`

Result: **PASS**.

## Repository checks

- `python scripts/validate_repo.py`: PASS, 0 errors.
- `python -m unittest discover -s scripts/tests -v`: 31/31 PASS.
- `git diff --check`: PASS.

## Native Automation

Filter: `Hearthward.`

Result: **33/33 PASS**, 0 failed, 0 not run.

New targeted native test:

- `Hearthward.NPCAgent.CompanionCombatPolicy`: 1/1 PASS.

It verifies:
- wait -> Hold;
- follow -> Follow;
- attack/assist obeys player-centered leash;
- dead and out-of-range threats are excluded;
- target selection is deterministic: player distance, then companion distance, then stable ID;
- no legal threat falls back to Follow;
- down player forces Hold;
- `companion_order` only accepts hold/follow/assist and cannot name an arbitrary enemy.

## Deterministic PIE

Script: `verify_combat_policy_pie.py`

Result: **29/29 checks PASS**.

Observed runtime result:

- confirmed assist maps to legacy `attack` order;
- UE tactical intent: `assist`;
- UE-selected target: `guard_1`;
- reason: `ASSIST_NEAREST_PLAYER_THREAT`;
- legacy follow/wait controls still work;
- candidate staging has no world effect before confirmation;
- leaving the 30m communication range invalidates confirmation;
- an explicitly confirmed directive may replace an active collect task through existing cancel semantics.

## Real-model PIE

Script: `verify_combat_model_pie.py`

The isolated worktree reused the existing local runtime through the development-only command line override:

`-HearthwardAIBundlePath=<existing Runtime/LocalAI>`

with Vulkan / 32 GPU layers.

Result: **14/14 checks PASS**.

Actual Qwen3.5-4B structured outputs:

- player: `跟着我。`
  - `intent=companion_order`
  - `item=follow`
  - `quantity=1`
  - `mode=directive`
  - `source=player`
  - world unchanged until confirmation.
- player: `帮我对付附近的威胁。`
  - `intent=companion_order`
  - `item=assist`
  - `quantity=1`
  - `mode=directive`
  - `source=player`
  - world unchanged until confirmation.

After confirmation, UE—not the model—selected `guard_1` from authoritative encounter state. The filtered context also contained the current `companion_combat` object.

## User-retest regression

The user's UE retest exposed a real navigation arbitration regression and one dialogue-layout collision.

### Dialogue toolbar + deterministic collect

Script: `verify_user_retest_pie.py`

Result: **11/11 PASS**.

Verified:

- the static `弟弟` title region has no dynamic action hitbox;
- `刷新建议`, `记忆与约定`, and manual task type controls are reachable in the toolbar without occupying the title region;
- a two-unit collect candidate confirms successfully;
- executor starts with a typed `MoveTo` action;
- companion physically moves more than 50 cm;
- command completes with 2/2 delivered.

Root cause of the movement failure: `UHearthwardGameplayComponent::TickCompanion` called `StopNavigation()` whenever a collect/craft/repair phase was active. That combat-policy tick ran continuously and cancelled the executor's freshly issued MoveTo request. The active-task branch now yields immediately without touching navigation.

### Full executor regression

Re-ran `docs/qa/evidence/TASK-028/verify_executor_pie.py` after the fix.

Result: **49/49 PASS**.

This covers collect, finite-source shortage, retained cargo, craft, repair, and save-plan reconstruction.

### Exact real-model user phrase

Script: `verify_user_model_collect_pie.py`

Input: `帮我采集两份木材带回营地。`

Result: **11/11 PASS**.

Actual Qwen output:

`{"intent":"collect","item":"wood","quantity":2,"mode":"additional_acquired","source":"S1",...}`

The candidate caused no world change before confirmation. After confirmation, the typed executor started, the companion physically moved, and the task completed with exactly 2 delivered.

The full native `Hearthward.` suite was also re-run after this fix: **33/33 PASS**.

## Boundary confirmed

TASK-030 does **not** let the model choose coordinates, enemy IDs, per-frame movement, attack timing, LOS, cooldown, hit resolution, or damage. Those remain UE-authoritative.

Formal GitHub Issue / independent reviewer requirements remain blocked because the current GitHub integration can read the repository but issue creation returned HTTP 403. This does not change the local validation result.
