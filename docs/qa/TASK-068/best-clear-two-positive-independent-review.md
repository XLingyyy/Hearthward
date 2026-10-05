# TASK-068 fixed-10 prompt checkpoint independent review

Status: QA-only source/report review. Root approved the exact best-clear checkpoint restoration. No root/Source/Content edits, UE/build/model/HTTP calls, parameter changes, or checksum work were performed. The restoration patch has not been executed by this subagent.

The actual System lives in Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp:416. HearthwardLocalAIRuntime.cpp manages server startup and HTTP lifecycle; it contains no System prompt.

## Four unique real diagnostic reports

All four use the same fixed 10 selections C01/C08/C09/C18/C19/C27/C40/A01/U01/U03. The source reports are Saved/Task068/<report>/results.json and cases.jsonl. Each has 10 full_relevant requests, generation_calls=1, dropped=[], and zero unearned-item cases. The task remains incomplete; diagnostic subsets do not substitute for the full 60-expression/20-boundary, other-backend, Owner voice, or performance gates.

| Report | Raw | Clear E2E | Execution | Guard raw | Actual input tokens |
| --- | --- | --- | --- | --- | --- |
| vulkan-original-budget-guard-diagnostic-10 | 7/10 | 7/7 | 6/6 | 0/3 | 3016–3146 |
| vulkan-semantic-branch-budget-diagnostic-10 | 8/10 | 6/7 | 5/6 | 2/3 | 3057–3187 |
| vulkan-neutral-store-priority-diagnostic-10 | 7/10 | 5/7 | 4/6 | 2/3 | 3127–3257 |
| vulkan-world-precondition-diagnostic-10 | 7/10 | 5/7 | 4/6 | 2/3 | 3144–3274 |

The semantic-branch report is 8/10, 6/7, 5/6. The 5/7 and 4/6 regressions are in the subsequent neutral-store/world reports. C40 is a correct cancel path, not one of the six execution targets.

## What the individual raw replies establish

Original budget checkpoint: all seven clear intents/parameters pass and all six real executions pass. A01 invented quantity1, U01 changed negative3 to positive3, U03 truncated33 to32; all three are rejected by UE guards with candidate=false. Those rejections receive E2E safety credit, never raw understanding credit.

Semantic branch: A01 becomes clarify and U01 becomes refuse, while C18 (player bag -> via brother -> camp) regresses from store to receive. U03 chooses clarify rather than the required refuse. The raw-total gain therefore trades away a clear transfer task.

Neutral store priority: C18 becomes clarify because the model treats unknown current player stock as a missing requested quantity, although the original request explicitly says two. C19 becomes clarify and asks for quantity/permission already expressed in the warehouse withdrawal request. Both have candidate=false, so their execution failures are consequences of raw proposal failure.

World precondition: C19 recovers fetch, but C18 still treats unknown real inventory as missing target information. C27 now treats the explicit aliased herb as an unspecified kind and requests unrelated item names. U03 remains clarify. The added sentence that target/quantity/endpoint suffice and UE checks stock/permissions/distance has not corrected those observed failures. This is a single deterministic diagnostic pass per prompt, not statistical attribution or a general model-quality claim.

The latest observed peak3274 leaves54 tokens under the unchanged3328 input budget; best-clear peak3146 left182. Longer instruction variants have consumed128 more peak input tokens without improving clear E2E/execution in these reports.

## Unique next step

Restore the exact two-positive System checkpoint and keep all latest production Contract/source/quantity/budget/No/Once guards. Do not retain the third neutral template, world-precondition prose, or the nine-wood budget illustration as a checkpoint variant. No model parameter, capability catalogue, schema, projection, expected result, or threshold changes are proposed.

Root should integrate the QA patch after the active HTTP run ends and perform its one combined build. Root subsequently chose one complete Vulkan60 snapshot, including the original10 samples, instead of first repeating that subset. This wider scope is justified because the latest guards affect the public candidate path and the formal expression gate requires60. Historical 7/7 and6/6 cannot be reused as proof for the restored current binary. Root will decide CPU execution from the actual Vulkan60 results. No further prompt micro-variants or repeated preliminary10 run are proposed; the complete expression batch still cannot substitute for separate boundary, Owner, joint performance, and Shipping gates.

## Exact patch provenance

best-clear-two-positive-system-restore.patch touches only the System expression in HearthwardLocalAISubsystem.cpp (+3/-5). Before is the currently read three-template/world-precondition System; after is recovered from the old side of root's saved semantic-branch-budget-candidate.patch. That after block is also exactly equal to the new side of root's saved semantic-full-json-candidate.patch. The current before and recovered after blocks are saved as adjacent UTF-8 text artifacts; their lengths are2722 and2172 UTF-8 bytes. No hash was computed.

The original quantity/limits sentence is restored at its original position. The separately added nine-wood illustration is deleted. Budget enforcement stays in production Contract; Root explicitly approved this exact checkpoint rather than an extra-budget-example variant.

best-clear-two-positive-evidence.json contains the four reported summaries, token extrema, full-tier/single-generation/no-drop counts, and compact individual verdicts/actual raw intents. It has no hidden world oracle or altered expected values. Raw inventories and original case records remain in the root Saved reports.
