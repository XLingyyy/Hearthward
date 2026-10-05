# Missing Nature target and escort precondition review

QA-only, not applied or run. Actual CPU A09/A10 fields are preserved in missing-target-cpu-actual-evidence.json. No Root/Source/Content/frozen-data/oracle/model changes. Root's npc_line-first diagnostic and rollback are independent; this proposal must not be mixed into the single-variable model experiment.

## Actual A09 and precise source meaning

CPU A09 `帮忙照顾一下作物` actually returned nature_care/harvest1/known_target with no unresolved fields. Stage produced TARGET_REQUIRED, refuse, no candidate, E2E=false, no unconfirmed world effect. The model raw remains incorrect; its own npc_line asks for crop/location/action clarification, which does not correct its structured harvest choice.

- Contract's typed care without Station remains intentionally valid (NPCAgentTests.cpp:55–58); model schema has no Station field. World target resolution belongs to Stage/Preview.
- StageCandidate:146–183 binds an actual nearby unique target. Care candidates are filtered by unwatered/unfertilized/ready state; the resource path checks type/material/proximity. No selected or multiple eligible targets leaves Station invalid.
- PreviewGoal nature_care:1343 and nature_collect ResourceTargetReason:1221 return TARGET_REQUIRED exactly when Station is invalid. The known-target precondition is enforced; there is no observed execution safety defect.
- A valid GUID has separate TARGET_UNAVAILABLE/TARGET_INVALID/ALREADY_DONE/NOT_READY and source/route/safety checks. Those represent invalidated or unmet concrete targets and remain unchanged.
- Stage:297 already stores the entire Goal/Original, but :298–310 has no missing-target question branch and falls through to a generic workplace/facility refusal. It does not record a clarification turn. That is the independently confirmed missing-information UI defect.

The auto-bind filter can also leave no Station when nearby plots exist but none qualify for the guessed action. This proposal asks the player to explicitly identify the target/action; it does not claim the world has no crop, alter eligibility, bypass maturity/completion, or auto-select a different target.

## Minimal scoped proposal

missing-nature-target-clarification-minimal-proposal.patch adds six lines to AgentInteraction after WorkingGoal is stored. Only `ReasonCode == TARGET_REQUIRED` and intent nature_care/nature_collect use the existing waiting/clarify/AddClarification path. The machine reason and full Original remain. No new unresolved slot is inserted: current arbitrary unresolved conditions would persist into later turns, so inserting a target slot without a matching resolution contract would create a new permanent refusal.

Other errors, escorts, raw JSON, all capability/schema/value definitions, gameplay preconditions, registry and oracle are untouched. A09 raw stays a failure. A later E2E change would represent the corrected UI safety/question branch only.

## Actual public RED entry

missing-nature-target-public-stage-red.patch adds sixteen lines inside the already used real world/companion/LocalAI fixture in CraftingPresentationTests.cpp, before its existing source-crafting loop. Root filter: `Hearthward.Crafting057.WarehouseMaterialsPresentation`.

It calls public `AI->SetStructuredGoal(Player,Brother,MissingTarget)`. There is no StageManual API, UI ExecuteAction call, private StageCandidate invocation, private property write, or reflection bypass. The existing public entry really calls StageCandidate. Contract validity, world unpaused, companion Preview TARGET_REQUIRED, and staged reason TARGET_REQUIRED are hard prerequisites. Expected business RED assertions on current Source are LastAppliedIntent clarify and ClarificationTurns1. Candidate remains absent, working Original equals canonical GoalText, and brother inventory remains unchanged. No new world/helper/header/dependency or fake world-state initialization is added. NPCMemory.h includes AgentContract declarations through the existing LocalAI include; AddClarification stores one Player/Question turn (Memory.cpp:95–100).

SetStructuredGoal rewrites Input to canonical GoalText and clears old clarification state. This Native test covers the real missing-target UI branch and canonical draft retention; it does not replay the A09 free-text sentence or earn model raw credit. Root's actual A09 already supplies raw-pipeline evidence. Later free-text validation uses public SubmitPlayerText with the actual model, after the separate experiment is closed.

The plain missing-nature-target-public-stage-red-insert.txt is supplied for Root's known mixed-CRLF file, so only the new block needs insertion. The next existing SetStructuredGoal in the crafting loop clears the manual draft through the normal public API.

## A10 stays unchanged

CPU A10 `带一个人回营地` actually returned escort/rescued_01, then PERSON_NOT_CONTACTED/refuse, no candidate, no unconfirmed world effect. Frozen A10 requires clarify and its raw/E2E both fail. That score discrepancy alone is not evidence that all instances of this reason should become clarification.

PreviewGoal:1356–1360 merges four concrete cases: person absent, uncontacted, arrived, actor unavailable. Current CPU authority records inventory/source/command facts, not Campaign.People/Actor presence, so the actual triggering case is unrecorded. The general player sentence does not authorize the model's choice of rescued_01; adding a universal PERSON_NOT_CONTACTED→clarify mapping would also reclassify an explicitly identified unavailable or already-arrived person.

Campaign Describe/Interact are existing public reflected APIs; public C++ Actor/AssignEscort exist without UFUNCTION. Normal Interact at a real nearby actor establishes following/waiting, and AssignEscort validates contact, actor, distances and epoch. No new NPC dictionary, identity parser or guessed canonical ID is proposed. A10's guard and rejection remain; future focused investigation can record actual Campaign.Describe and Actor presence before using the normal contact/escort path, with no filled snapshot or Stage private seam.

Native RED, production patch and any actual post-fix A09 pipeline result are NOT_RUN/NOT_APPLIED by this subagent.
