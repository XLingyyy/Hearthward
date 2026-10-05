# TASK-071 physical hold-open proposal

Status: proposal and static checks only. Neither script has been imported or executed by this agent. No UE, build, Git, Root/Source/Content modification occurred. Root owns physical input, execution, inspection and acceptance.

## Files and Root invocation

- `physical_hold_open_monitor_proposal.py`: 201 lines; dormant prototype setup followed by passive public-state sampling.
- `run_physical_hold_open_proposal.py`: 76 lines; explicit project, unique UUID pool, public UEClient launch, default 900-second deadline, same-PID stop in `finally`.
- `physical-hold-open-static-check.json`: AST and compile-without-execution evidence; ten static checks passed.

Root may execute the host directly from this QA directory; it launches its sibling monitor against the explicitly specified project. No copy into Root is required.

```powershell
python -X utf8 G:/GameFactory/Hearthward/.agent-local/task062/docs/qa/TASK-071/run_physical_hold_open_proposal.py --project G:/GameFactory/Hearthward/.agent-local/task051/Hearthward.uproject --label root-os-keyboard-mouse
```

The host prints the launched PID, UUID and absolute `Saved/Task071/physical-<UUID>` output/finish-marker paths. Wait for `progress.json.stage == "hold_ready_for_root_os_input"` before physical input. Root uses real OS keyboard/mouse for dialogue first Paint, Esc, the ordinary load menu, Enter/Esc and mouse confirmation. The sampler performs none of these inputs.

Root finishes recording by writing a complete UTF-8 JSON value to that output directory's `finish.json`. A JSON object such as the following retains Root's notes; for an external write, prepare a temporary file and rename it to `finish.json` after the write completes.

```json
{"operator_note":"Physical sequence finished; acceptance requires review of journal, screenshots and public state."}
```

The marker never supplies acceptance. Monitor `acceptance` remains `NOT_EVALUATED`, including if the marker contains a claimed PASS. `recording_complete` means the sampler read the marker without error; host exit 0 means recording was captured and its Editor PID stopped. Neither field awards product PASS or Owner credit.

## Fixture and observation evidence

The existing Root `docs/qa/TASK-071/verify_ui_loadpoint_pie.py:47-56` already demonstrates importing the dormant TASK068 flow and unregistering its original Slate callback before advancing it. The proposal preserves that method, redirects output, sets `selected_cases=[]` and `case_limit=1`, and replaces only the fixture's assertion sink. The imported flow's language-case loop receives no cases; its boundary loop at `verify_model_matrix_pie.py:399` requires `case_limit == 60`, so neither loop runs. The original callback cannot run the model matrix after being unregistered. This is a static control-flow finding; the new scripts have no actual PIE sampling result yet.

TASK068 public `SavePoint(True)` at `verify_model_matrix_pie.py:330-334` creates A and stores its actual GUID. The proposal records that node and public baseline state, grants only player wood 1 with public `try_add`, and checks the exact inventory snapshot delta. It checks that future state retains the same save nodes and epoch, records `saved:false`, then enters hold. The ordinary menu can subsequently load A; Root can assess removal of that future wood and advancement of the public epoch from the recorded differences. No scripted load occurs.

The fixture is explicitly disclosed in both `setup.json` and the final report:

- Added Cube floor with BlockAll collision and NatureGround tag (`verify_model_matrix_pie.py:286-290`).
- `CreateTest` companion fixture; every actor with `CombatTargetComponent` moved away, including qualifying animals (`:295-300`).
- Public prototype campaign start, participant/source/camp positioning, `source_safe=True`, source wood 80, companion axe 1; gameplay component tick disabled (`:300-314`).
- Public workbench placement and `CreateTestAccess` warehouse fixture with wood/stone/herb 20 each (`:315-328`).

These conditions do not establish normal new-game behavior or human Owner acceptance. The proposal adds no private reflection setter; it inherits the base fixture's explicit public prototype-field write before hold.

The public read APIs are declared in `Source/Hearthward/UI/HearthwardScreenWidget.h:31,40` (`GetPage`, `DescribeLayout`), `Source/Hearthward/Save/HearthwardSaveSubsystem.h:33-35` (`GetPoints`, campaign ID and status), and `Source/Hearthward/AI/HearthwardLocalAISubsystem.h:49,66` (server PID and generation calls). Save point IDs/manual/locked/location/stage are public read-only properties (`Source/Hearthward/Save/HearthwardSaveGame.h:110-116`). Inventory `DescribeInventory` serializes the snapshot (`Source/Hearthward/Inventory/HearthwardInventoryComponent.cpp:129-130`); its stacks/instances/equipped/backpack schema is defined at `HearthwardInventoryState.h:35-43`. `DescribeLayout` reports page, component bounds, action strings and text (`Source/Hearthward/UI/HearthwardScreenLayout.cpp:172-185`); those strings are recorded without invoking them.

Hold refreshes the current world, actors, subsystems, HUD and Screen on every sample through the existing public-object discovery helper (`verify_model_matrix_pie.py:61-72`), then reads public state. No old actor references are carried through a load for the next observation. The tick only advances setup until it is exhausted; subsequent hold ticks sample public state and inspect the filesystem marker. There is no hold call to `ExecuteAction`, `OpenPage`, `ActionAt`, `LoadPoint`, model submission, inventory grant/removal or a reflection setter.

## Output and limits

- `setup.json`: complete baseline A and future public states, exact future delta, fixture checks and disclosures.
- `events.jsonl`: initial public state and subsequent changed top-level public fields with before/after values; unchanged inventories are not dumped each tick.
- `layouts.jsonl`: first layout and changed layouts only, with sample IDs/timestamps.
- `progress.json`: updated approximately every 0.5 seconds while Slate ticks; compact page/epoch/save-node/task/wood/AI counters and layout/event sequence. No full inventory snapshot in progress.
- `results.json`: Root marker, complete baseline/future/final public states and final differences from each, observed model/server flags, sampling counts, fixture conditions, acceptance unevaluated.
- `launch.json`, `host-progress.json`, `host-result.json`, `stop.json`: public host lifecycle and timeout/stop outcome.

The sampler itself makes zero model requests by static control flow. Actual sampled generation calls/server PID must be inspected after Root execution; nonzero observations remain sticky even if counters later reset on load. A 0.5-second observer can miss transient state changes, and snapshots do not certify OS input provenance or successful physical rendering. An Editor crash or blocked Slate loop leaves the last progress/event/layout evidence and the host reaches its deadline before stopping its launched PID. No unattended timeout is converted to recording complete or PASS.

Official callback lifecycle reference: [Unreal Python module API: register/unregister Slate callback](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/module/unreal?application_version=5.7). This source supports use of callback handles; it provides no runtime validation of these UE 5.8 proposals.
