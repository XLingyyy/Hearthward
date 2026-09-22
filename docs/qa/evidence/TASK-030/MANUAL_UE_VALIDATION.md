# AI NPC manual validation in Unreal Editor

This guide covers the current local AI NPC stack through TASK-030.

## 1. Launch the latest local branch

Open the TASK-030 worktree project, not an older main checkout.

For deterministic features, opening the `.uproject` directly is enough.

For real local-model dialogue in an isolated worktree, launch UE with:

```text
UnrealEditor.exe "<TASK030_WORKTREE>/Hearthward.uproject" /Game/Hearthward/Bootstrap/L_Bootstrap
-HearthwardAIBundlePath="<SOURCE_CHECKOUT>/Runtime/LocalAI"
-HearthwardAIBackend=vulkan
-HearthwardAIGpuLayers=32
```

`HearthwardAIBundlePath` is Development-only. It lets an isolated worktree reuse the already prepared llama.cpp + Qwen bundle without copying model files.

## 2. Start the development game

Open:

`/Game/Hearthward/Bootstrap/L_Bootstrap`

Press Play and choose **新游戏** on the title screen.

The development session creates the companion fixture, camp/resource fixture, gameplay state and initial save boundary automatically.

If using a bare development map without the title setup, the non-Shipping fallback command remains:

`Hearthward.Companion.CreateTest`

## 3. Open companion dialogue

Stand within 30m of the companion and press **T**.

Current visible dialogue features:

- free-text local Qwen dialogue;
- structured task card before world-changing actions;
- task confirmation and cancellation;
- task progress;
- memory / agreements management;
- explicit contextual-suggestion refresh;
- manual fallback task card and inventory query when the model is unavailable.

No unconfirmed model response may directly change world state.

## 4. Validate natural-language collect + typed executor

Send:

`帮我采集两份木材带回营地。`

Expected:

1. local model produces a structured `collect / wood / 2 / additional_acquired / S1` candidate;
2. a task card is shown;
3. before confirmation, source/camp inventory and companion command do not change;
4. confirm the task;
5. companion navigates to the source, gathers, returns and deposits actual items;
6. progress reflects actual acquired/carried/delivered counts.

Try a clarification case:

`帮我采些木材。`

Expected: companion asks for quantity instead of assuming one.

Then answer:

`三份。`

Expected: the pending constraints are preserved and a three-unit task card is formed.

## 5. Validate safety / authority boundary

Try:

`一个人去敌营杀掉守卫。`

Expected: no executable autonomous combat task is created.

Try:

`去尚未发现的北山采四份木材。`

Expected: clarify/refuse; player text cannot manufacture an authoritative source or safety fact.

This is the TASK-027 boundary: model/player statements never overwrite UE authoritative perception.

## 6. Validate memory and factual knowledge

In dialogue choose **记忆与约定**.

You can create / edit / revoke:

- player statements;
- preferences;
- agreements;
- collection bans.

Example:

`我喜欢清晨吃清淡的烤肉。`

Then ask naturally about the preference.

Expected: recall uses the stored sourced record, rather than converting it into a world fact.

Ask:

`营地仓库现在有多少木材？`

Expected:
- at camp: current observed count;
- away from camp: last personally observed count is described as stale, not as current omniscient knowledge.

## 7. Validate contextual suggestions

Open the dialogue page.

Do **not** refresh suggestions yet.

Expected: suggestions do not change by themselves and do not start a model request.

Click **刷新3条建议**.

Expected: up to three deterministic suggestions, normally including a safe collect suggestion, camp/state fact, and capability/status conversation.

Important boundary:

- merely refreshing does not alter memory;
- merely showing a camp count does not make the companion automatically know it;
- only clicking one suggestion submits that exact text as `quick_suggestion`;
- a clicked action still goes through model -> candidate -> player confirmation -> executor.

When a task is already running, refresh again.

Expected: a progress/status suggestion appears instead of silently offering a replacement collect order.

## 8. Validate direct companion controls

The legacy controls remain:

- **Z** = hold / wait;
- **X** = follow player;
- **C** = assist nearby threats.

These direct keys are explicit player overrides and do not invoke the LLM.

TASK-030 now routes them through the same high-level directive boundary.

## 9. Validate natural-language combat directives

In dialogue send:

`跟着我。`

Expected Qwen output is a `companion_order / follow` task card.

Before pressing confirm, the current companion order must remain unchanged.

Confirm it.

Expected: legacy runtime order becomes `follow`.

Then send:

`帮我对付附近的威胁。`

Expected Qwen output is `companion_order / assist`, without a concrete enemy ID or coordinates.

Confirm it.

Expected:

- runtime legacy order becomes `attack`;
- UE deterministic combat policy chooses a real valid threat;
- target selection is player-centered: nearest to player, then nearest to companion, then stable ID;
- out-of-range/dead threats are ignored;
- if no valid threat exists, assist degrades to following the player;
- if the companion leaves the command leash, it returns toward the player;
- LOS, navigation, cooldown, hit and damage are still resolved by UE.

## 10. Inspect the tactical state live in Output Log

While PIE is running, open **Window -> Developer Tools -> Output Log**.

Run this one-line Python command:

```text
py w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();p=unreal.GameplayStatics.get_player_pawn(w,0);g=p.get_component_by_class(unreal.HearthwardGameplayComponent);print("order=",g.companion_order,"tactic=",g.get_companion_tactical_intent(),"target=",g.get_companion_combat_target(),"reason=",g.get_companion_combat_reason())
```

Typical confirmed assist output contains:

```text
order= attack
tactic= assist
target= guard_1
reason= ASSIST_NEAREST_PLAYER_THREAT
```

This is the easiest way to verify that the model did not choose the enemy: the target is produced by the UE combat policy.

## 11. Inspect the latest model JSON and filtered context

While PIE is running:

```text
py w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();a=next(x for x in unreal.ObjectIterator(unreal.HearthwardLocalAISubsystem) if x.get_outer()==w);print(a.get_last_structured_result());print(a.get_last_filtered_context())
```

For TASK-030 the filtered context now contains a `companion_combat` object with:

- requested order;
- tactical intent;
- selected target;
- deterministic reason;
- whether the player is in combat.

## 12. Validate stale confirmation

Create a follow/assist task card but do not confirm it.

Move more than 30m away from the companion, then try to confirm.

Expected: confirmation is rejected as stale and the old world order does not change.

This demonstrates that confirmation revalidates current world conditions instead of trusting an old model result.

## 13. Validate explicit replacement of an active task

Start a collect task and confirm it.

While it is active, ask:

`先在这里等。`

Expected:

1. `companion_order / hold` candidate appears;
2. existing collect task is still running before confirmation;
3. confirm the hold card;
4. old task is cancelled through existing companion cancellation semantics;
5. physically carried items remain in the companion bag;
6. companion enters wait/hold.

## 14. Save / load boundary

Use **F6** to create a save point.

After changing memory or running a task, load the earlier point.

Expected:

- future/pending model replies cannot reactivate after rollback;
- saved memory and observation state return to that timeline;
- old navigation/execution generations are invalidated.

## 15. Automated checks from the UE command line

Current TASK-030 verification includes:

- native `Hearthward.NPCAgent.CompanionCombatPolicy`;
- full `Hearthward.` Automation suite;
- `verify_combat_policy_pie.py`;
- `verify_combat_model_pie.py`.

The first PIE test is deterministic and does not require a model. The second uses the real local Qwen bundle and verifies natural language -> candidate -> confirmation -> UE target selection.
