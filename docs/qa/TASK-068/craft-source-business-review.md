# TASK068 material-source business regression

State: QA draft only. Source was read, not edited. Native/build/model NOTRUN by this subagent; the parent owns the real RED → minimal fix → GREEN sequence.

## Confirmed runtime evidence

Root `Saved/Task068/vulkan-full-json-diagnostic-10/results.json`, C09: the model produced a valid craft/rope/1/batches/camp card, limits/unresolved empty. Candidate retained camp authorization; confirmation succeeded. Camp wood stayed 20, brother wood fell 12→10, camp rope rose 0→1. `raw_pass=true`, `e2e_pass=false`, `execution_pass=false`.

## First production error

`AI/HearthwardAgentInteraction.cpp` passes the complete candidate into `SubmitGoal` at 341. `Companion/HearthwardCompanionFixture.cpp` accepts the Goal, including `SourceRef`, and calls the existing plan builder. `HearthwardAgentPlan.cpp` correctly adds `TakeMaterials` only for camp. The source remains intact.

`Companion/HearthwardCompanionFixture.cpp` 1563 and 1574 subtract all existing bag material from each authorized camp recipe amount. With a rope cost of wood2 and bag wood12, both `Missing` values become zero. Warehouse transfer is skipped, and the later real `HearthwardWorkshop::Commit` correctly consumes held wood2. This is the source violation; Workshop settlement itself operates as designed.

The separate `craft-source-minimal-fix.patch` replaces only those two calculations with `C.Value`. The camp step then takes the full authorized recipe amount before Commit, preserving the original bag stock after craft. The bag plan has no TakeMaterials step and keeps its existing behavior. No API, plan, protocol, Save fields, receipt identities, Workshop implementation, or model raw text changes.

## Regression-only patch

`craft-source-regression-red.patch` extends `Source/Hearthward/Tests/CraftingPresentationTests.cpp` in the existing `Hearthward.Crafting057.WarehouseMaterialsPresentation` fixture (+66 lines), retaining all existing presentation/transaction/reservation assertions. This avoids a duplicate World, viewport, or navigation harness.

After the original reservation scenario releases both reservations, the new checks use the actual registered workbench and standalone GameInstance/possessed local player. A normal companion is placed at Z80 on the existing box ground top Z0 (the actor's actual capsule half-height is 80), horizontally 100cm from the workbench center. A camp scene actor is at the companion position, matching the existing public companion fixture convention. The source stock component is present. `IsAtCamp`, `CanCommunicate`, and actual `HearthwardWorkshop::Check` must pass before any business assertions; a failure there is a fixture precondition failure.

For each of camp then bag, public `Storage.Adjust`, `Bag.TryRemove`, and `Bag.TryAdd` prepare exact independent wood20/wood12/rope0 stock. Public `SetStructuredGoal` stages craft rope one batch; candidate source, zero unconfirmed consumption, real `ConfirmCandidate`, and accepted source are checked. Up to 16 calls to public `Brother.Tick(.01f)` execute normal Move/Take/Commit/Deposit actions. Both movement targets start inside the existing acceptance checks; no actor motion, Nav success, consumption, delivery, command state, or receipts are simulated.

Expected final quantities:

| source | camp wood | brother wood | camp rope | brother rope | delivered |
|---|---:|---:|---:|---:|---:|
| camp | 18 | 12 | 1 | 0 | 1 |
| bag | 20 | 10 | 1 | 0 | 1 |

The second fixture prepares its own stock through public inventory APIs, so an already recorded camp-source failure does not invalidate the bag positive control. The executor must reach `Completed` for either scenario.

Read-only signature checks: `SetStructuredGoal`, `ConfirmCandidate`, `GetCandidate`, `GetGoal`, `Tick`, `GetPhase`, `GetDelivered`, `Bag`, and `IsAtCamp` are public; `Workshop.Check` has a default EquipmentId argument; storage/inventory mutations are the existing public APIs. No private state or test setter is accessed. Only QA patch/document files were written.

Root runtime label `quantity068-clarification-green-craftsource-red`: OriginalQuantityBoundary 22 assertions PASS with zero warnings. The extended Crafting test has exactly two business RED assertions: camp expected18 actual20 and brother expected12 actual10. Other checks, including the entire bag-source positive control, passed. The first compile exposed SourceRef is FString; the parent corrected the scenario loop to const FString& and rebuilt before this RED. This QA draft now uses that real field type. Parent is applying the separate two-line production fix and owns the next GREEN run; no source/business GREEN is claimed here.
