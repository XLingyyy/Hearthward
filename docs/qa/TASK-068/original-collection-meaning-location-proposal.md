# TASK-068 original collection meaning/location safety proposal

Status: QA-only incremental drafts. Root actual public model/pipeline defects are confirmed; these new Native tests and production proposal have not been executed or integrated by this subagent. No Source/root/Content, model calls, frozen dataset, or global thresholds were changed.

## Actual first-source evidence

Current root Saved/Task068/vulkan/cases.jsonl has the two actual one-generation/full_relevant records, copied narrowly into original-collection-meaning-location-pipeline-evidence.json. A04 original 拿二份木材过来 produced collect/wood2/additional_acquired/S1 with empty limits/unresolved; candidate=true, Reason empty. A08 original 新采四份木材，但别去那里 produced collect/wood4/S1 with empty limits/unresolved; its raw npc_line says it avoids the old point while the actual candidate still uses S1. Neither ambiguous candidate was confirmed by the QA runner, and both recorded no unconfirmed effects/zero unearned items. The defect proven is an incorrect executable proposal, not an observed improper harvest.

StageCandidate131 binds the actual Input to Goal.Original, appends earlier clarification original185, then checks selected unknown-place phrases191–193 and quantity/item consistency. It checks unknown words 尚未发现/未发现/未知地点/没去过 but not an excluded-place expression. Its transfer-source guards cover store/give/fetch/receive/retrieve, while collect has no original collection-action/source check. The fields and quantities are legal, so Contract Validate277 and real PreviewGoal294 pass, creating Candidate306. The actual pipeline therefore confirms the missing gate without invoking private Stage or a mock callback.

## Smallest meaningful Native RED increment

original-collection-meaning-location-red.patch adds35 lines to existing NPCAgentTests, two independent tests:

- OriginalCollectionMeaning parses the actual model-shaped raw proposal, binds the actual original, proves original quantity matches, then expects Validate to reject it. Positive controls retain explicit new collection, the existing 木头 alias, plain 采, and canonical GoalText.
- OriginalCollectionLocationRestriction similarly proves the actual target quantity remains valid, then expects the omitted negative place to block default S1. It preserves the existing positive source:S1 limit and canonical GoalText controls.

Each has one intended RED assertion. They exercise the public Contract entry called by Stage/Confirm/Plan. They do not invoke a substitute model or count Native deterministic tests as model understanding. Root owns the real RED run before production integration.

## Production proposal, only two existing cpp hunks

original-collection-meaning-location-minimal-proposal.patch adds17 lines, no helpers/header/dependency/Schema/registry changes.

Contract: only nonempty original collect/nature_collect requests are checked. Explicit negative go/enter/approach/exclude-location wording returns UNRESOLVED_COLLECTION_LOCATION because the current typed schema has positive source:S1 and material ban constraints, no negative-location identity/route enforcement. The raw named/pronominal place cannot be replaced by the sole default S1.

Only legacy collect additionally needs an affirmative collection verb in an original sentence/field. The bounded regex supports complete 新采/采集/收集 and a one-character 采 followed by a quantity or the current registered ItemText/id, with common request/location prefixes. Material aliases are normalized through the existing table. The prefix cannot cross an explicit negative cue. Sentence/field colon support allows a preserved source clarification in 补充：. An unspecified 拿/搬/交付 operation has no acquisition verb, so it cannot choose S1 on the player's behalf. Nature_collect is not subjected to this legacy-verb requirement; its existing actual-target binding remains, and only the omitted negative-location guard is shared.

Empty Original remains valid for the existing typed Contract/Plan fixtures. Parse itself still preserves semantic failures in a model-shaped goal; Stage supplies the original before Validate. StructuredGoal goes through canonical GoalText, whose existing 新采集 prefix remains valid. No actual raw JSON is rewritten.

Stage: Memory.WorkingGoal=Goal is already performed297, so the entire actual original remains saved. Only the two new specific reason codes take the existing waiting-for-clarification state and Memory.AddClarification path. Unknown acquisition asks source and destination; excluded-place asks which place/condition must be retained. This avoids the generic AMBIGUOUS_TARGET equipment-only message. It changes no global refuse handling, numeric guard, or other capability outcome. The original excluded-place text continues through subsequent Goal.Original history; no condition is silently dropped or replaced with a fabricated typed source.

These are deliberately finite safety checks around the two observed first-source gaps, not a general natural-language intent classifier. More complex phrasing that cannot express a trusted acquisition/source/location remains unresolved. Raw-model understanding for A04/A08 still fails if the model still emits collect; guard-generated clarify is UE pipeline safety/UI behavior and receives no raw-correct credit. It does not satisfy the full TASK068 understanding threshold.

## Root narrow validation after actual RED

Apply only after the two intended Native errors and positive prerequisites are confirmed. Run the two original meaning/location tests plus existing OriginalQuantityBoundary (shared original/clarification compatibility), preserving current rules/No/Once tests as needed in the combined already-planned build. Stage branch remains source-path evidence until Root runs actual public SubmitPlayerText for the two originals and a legitimate acquisition control. Such a replay must retain raw/result/LastAppliedIntent/candidate/status/original and prove no executable candidate; it does not retroactively alter the frozen Vulkan60 report or raw score. Full historical model results retain their own snapshot identity.
