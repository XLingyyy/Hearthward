# Advanced three-case independent runner review

QA-only source review. Root files, Source, Content, frozen60/20 and all production model parameters are unchanged. No UE/build/Git/model run. Root current Original9/9 and UI96/96 are separate reported results, not evidence that these three model cases pass.

## Verified entry and effect paths

- Nature Act/Describe/Busy, Inventory TryAdd/TryRemove/FirstInstance/EquipInstance, LocalAI SubmitPlayerText/GetCandidate/ConfirmCandidate and Companion phase/acquired/delivered/carried getters are reflected current public APIs. NatureState itself has no Python wrapper; the runner reads its actual Describe JSON. GuidLibrary returns (out_guid, success). The previous 062 reflection failure is avoided.
- Plant: public Act starts a real five-second plant and consumes the actual seed before creating the crop GUID. Water changes Watered, with no material outputs. Target selection asserts the same unique300cm predicate as Stage.
- Stone: actual initial loose_stones resource is tied to the Camp source by point.key. Companion ActionActor makes native Commit use the requested1, despite ordinary hand gathering yielding2. Final source remaining−1, warehouse stone+1 and settled empty cargo are required.
- Fish: actual fish point/Actor and within30m, public equipped rod GUID and bait1 are native Preview conditions. Real CatchCompanion consumes bait, simulates fishing, wears the same rod, decreases stock and increases Successes; public acquired/delivered1 and carried0 are required. The four-species gain totals exactly1. These are completed counters; private operation receipts are not reflected and are not counted.

## Two concrete defects and minimal revisions

1. Frozen run_pie.py hardcodes the matrix script and Saved/Task068/<backend>/results.json. Merely replacing ExecutePythonScript would wait on the wrong output. The independent47-line run_advanced_pie_proposal.py reuses launch/poll/owned-process stop, keeps bundle/backend/Vulkan16 arguments, and creates a unique UUID save pool plus a matching unique advanced output prefix. It does not edit or invoke the frozen matrix runner. Only its --help was executed.
2. Existing fishing oracle allowed reward1 and compensation together, or a newly recorded reward without any bonus cargo; it also rejected valid duplicate compensation when the reward marker already existed. Native NatureFishing.cpp:97–109 emits either reward1 or rope2+herb2. Revised oracle uses actual public Feedback plus Rewards and requires an exact branch delta, unchanged pending in this empty-capacity fixture, no extra gains, same rod identity/non-durability fields, stable inventory metadata and acquired/delivered1/carried0. Marker-only, combined gifts, missing compensation, changed rod/metadata, extra rod and unsettled cargo are rejected. The nature Feedback UPROPERTY is publicly reflected.

Incremental runner changes are advanced-runner-oracle-output-proposal.patch (+35/−11); revised runner is also available as a complete review artifact. advanced-execution-public-api-review-doc.patch corrects the old launch-only advice and receipt wording. advanced-oracle-sourceprobe.json reports twelve source-only checks (all match expected), no Unreal import or model credit. Both Python artifacts parse; standalone host --help exits0.

## Runtime limits and acceptance

Root should apply the runner patch, copy the host, and use the reviewed Vulkan command in advanced-execution-public-api-proposal.md only after CPU60 ends. Exactly three real SubmitPlayerText calls are intended, one per prepared sample; setup failure gets no raw-model credit. No deterministic proposal fallback exists. Backend budget3328, output budget256 and production single-flight/model are untouched.

Actual plant clearance, unique stationary targets, terrain/navigation for returning stone, prototype safety/epoch, Python property exposure during PIE, model raw understanding and the live rare/no-rare branch have not been executed. Failed setup remains a fixture failure; incorrect model JSON remains raw failure; successful native execution requires real confirmation and COMPLETED. No hunt/capture/camp_batch/escort, full-bag pending recovery, rare probability, normal menu/keyboard, Owner voice, performance or Shipping credit is claimed. task_complete=false refers to TASK068's overall outstanding gates; host success follows results.ok for this three-case supplemental runner.
