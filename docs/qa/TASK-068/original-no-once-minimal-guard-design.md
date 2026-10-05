# TASK-068 original material No/Once minimal guard draft

Status: QA-only in-memory incremental patches prepared against current root task051. Root owns integration, build, Native execution, and acceptance. Source/Content were not written. New guard is NOT_RUN in UE.

Root actual RED: Saved/Task053/revision071-blade055-green-no-once068-red/index.json; OriginalMaterialProhibition and OriginalOnceAuthorization each had exactly one expected error and zero warnings. Save13/Combat10 passed. These two existing RED test bodies remain intact.

## Incremental changes

- original-no-once-minimal-guard.patch: HearthwardAgentContract.cpp only, +48/-1. Add a local original-text material permission guard beside the existing budget guard; call both only for craft/repair. Registry, aliases, Schema/Parse, quantities, cost arithmetic, memory rules, and model prompt/parameters are unchanged.
- original-no-once-compatibility-tests.patch: NPCAgentTests.cpp only, +32/-0, added to the two already-integrated tests. Preserve all original assertions.

The guard normalizes with existing Aliases. After an explicit prohibition/permission verb it binds the longest registered complete ItemText or item id at the material phrase start. Id continuation checks prevent a shorter id from matching a larger ASCII token. Known material lists use 和/与/及/、. It does not scan for a known suffix inside an unknown leading phrase. Registered refined_ore is 精矿; 精炼矿石 has no registered alias and remains unresolved, never mapped to the tail 矿石. Unknown/ambiguous explicit prohibition returns UNRESOLVED_CONSTRAINT.

Original explicit No materials must retain their matching no limits, including materials not consumed by this recipe. No recognizes existing common prohibition/use words and the optional current-only phrase in 不允许本次消耗木材. It preserves no even when raw once is also present.

Each raw once material must have explicit current-only permission to use/consume that same registered material. The positive pattern starts at a sentence/field boundary (start, Chinese/ASCII punctuation, GoalText colon, newline, canonical label separator); it cannot start at 允许 inside 不允许本次消耗木材. Generic 这次/本次 or an allowance for another material is insufficient. Complex or unrecognized consent remains refused. Canonical GoalText 仅本次允许消耗木材 is supported. Plain warehouse-source authorization without once is unaffected.

Material captures stop before existing GoalText No/Once/Max label prefixes joined by 、. This keeps individual labels distinct while preserving explicit material lists. A regression asserts that a later canonical No cannot disappear from the limits.

## Narrow compatibility coverage

The incremental assertions cover another-material No and 药草 alias, wrong-material binding, exact 精矿/refined_ore versus ore, unresolved 精炼矿石, multiple canonical No labels including a missing later label, plain warehouse-source authorization, repair No with 木头 alias, wrong-material Once, stone/wood aliases and current-only permission wording, and the exact negated-consent sentence with both no+once plus its correct no-only control.

The negative consent test retains no:wood as well as once:wood; therefore it specifically proves that a raw once cannot pass merely because the No-preservation branch passed. The existing original GoalText No and Once controls remain intact.

No Stage change is proposed: StageCandidate merges applicable rules, then calls the same Contract Validate before creating a valid pending candidate. Confirm and execution planning validate again. An unauthorized raw once is refused by this gate even if Stage temporarily omitted a matching applicable no from the draft limits. This is source-path evidence; Stage/HTTP callback behavior has not been exercised by this subagent.

Root minimum execution: existing OriginalMaterialProhibition and OriginalOnceAuthorization plus the existing original-material budget tests because the shared craft/repair Validate expression is touched. Use the root pending combined build. Do not assign model-understanding, Stage, HTTP, or full acceptance credit to these Contract-only tests.
