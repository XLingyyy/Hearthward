# TASK-104 axe handle and hand pose

## Status

Source geometry, shared attachment code, per-rig finger poses and offline checks are prepared. The production `.uasset` has **not** been changed here. UE build, asset application/reload, native tests and in-game visual acceptance are **NOT_RUN**. Do not ship only the C++ changes with the old oversized mesh: the new native test deliberately rejects that combination.

## Source and scope

- Original project-owned Tripo source: `art_source/TASK-004/Tripo/妙妙道具/outputs/5ce74ca3-7123-488b-babd-d69a75aec742/stone_bone_axe_model.fbx`; provenance remains in that input's `SOURCE.md` under `妙妙道具`. No replacement model or new third-party asset was introduced.
- Original FBX SHA-256: `35cdf73c81a92870b63a694e6e7377c4322522735d0b66e0e56c706b7c0b1909`.
- Original production axe SHA-256: `a54c68a246c86497664c48aad3f2714fca384588318ba6245eed0a63409de5bf`.
- Hero/Brother source rigs: `art_source/TASK-095/source-cache/{Hero,Brother}/SK_{Hero,Brother}.fbx`. Exact hashes are checked by the extraction script. Hero's full reference positions and rotation matrices agree with archived production measurements; the Brother is fitted separately because its proportions differ.
- `stone_bone_axe_handle_fit.obj` preserves all 47,631 vertices, original polygon/UV/material assignments, head coordinates and axial handle coordinates. Only 10,416 handle vertices move radially. The 14 cm physical grasp region has a maximum 1.8 cm diameter at the unchanged `.7` display scale; transitions are smooth. This is not a whole-axe scale change.
- OBJ coordinates are Blender world metres, +Z up. `handle_fit_delta.json` contains the corresponding exact UE LOD0 vertex-ID changes in centimetres. The verified conversion is `(x,y,z) * (100,-100,100)`.
- `hand_pose_profiles.json` contains the two rigs' measured palm anchors and 15 right-finger local rotations. No wrist, arm, finger translation or finger scale is changed. The final animation node is enabled only for the durable, equipped, visible axe and is off for other weapons, ranged selection, death/rescue, and any work state that hides the axe. Visible axe harvesting remains enabled.

## Reproduce offline source checks

From the repository root, after fetching the exact source FBX files with Git LFS:

```sh
blender -b --python scripts/equipment/task104_build_axe_source.py -- --source "art_source/TASK-004/Tripo/妙妙道具/outputs/5ce74ca3-7123-488b-babd-d69a75aec742/stone_bone_axe_model.fbx"
blender -b --python scripts/equipment/task104_extract_grip_rigs.py
python scripts/equipment/task104_validate_grip_pose.py
python scripts/equipment/task104_grip_wrap_metrics.py
python scripts/equipment/task104_render_axe_geometry.py
blender -b --python scripts/equipment/task104_render_grip_pose.py
python -m unittest scripts.tests.test_axe_handle_grip scripts.tests.test_task104_axe_import -v
```

The build script requires actual FBX bytes, not a pointer; the rig extractor also accepts a previously fetched object in the normal `.git/lfs/objects` cache. Validation/rendering needs NumPy/SciPy/Matplotlib; Blender 4.3.2 was used here. No tool changes the original FBX or a production UE package during these offline steps.

## Apply to UE safely

1. Use the project's configured UE/GameFactory environment and public `UEClient` lifecycle from `docs/qa/BUILD_AND_TEST.md`. Close or save unrelated editor work. Fetch required LFS assets and obtain your own lock on exactly `Content/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe.uasset`.
2. Build `HearthwardEditor` using that environment's verified project path.
3. Launch a fresh editor through `UEClient.runtime.launch_editor`, supplying these supported `extra_args` (replace `PROJECT` and `LOCK_ID` with your verified values):
   - `-ExecutePythonScript=PROJECT/scripts/equipment/task104_apply_axe_grip.py`
   - `-Task104AxeApply`
   - `-Task104AxeLockId=LOCK_ID`
4. Inspect `Saved/Task104/axe-grip-apply.json`. Require `ok: true`, the exact axe as the sole saved package, matching UV/topology signatures, sockets, materials, collision counts and exact corrected positions. The immutable preservation receipt is `Saved/Task104/axe-grip-accepted.json`; retain it with the QA evidence.
5. Close that editor using its owning UEClient session. Launch a fresh editor with only the same `ExecutePythonScript` argument. Require `Saved/Task104/axe-grip-probe.json` to have `ok: true`. This verifies persisted whole-mesh geometry and preservation properties against the original success receipt. Repeating Apply on the corrected asset is read-only/idempotent and cannot establish a new baseline.
6. Run `Hearthward.Iteration.Task104.Grip.` plus existing axe combat/sweep tests. The four native tests cover mathematical attachment, actual mesh diameter/both actor attachments, finger-only transform scope and real Hero/Brother animation-graph equip/switch/broken/ranged behavior.
7. Inspect both characters at idle/walk/attack/visible axe work, including camera close-ups, transitions, rescue/death and other weapons. Verify axe blade sweeps after the attachment orientation change. Owner visual acceptance remains required.

The importer refuses active PIE/simulation, unsaved Content/maps, missing/wrong ownership locks, source hash/vertex mismatches, changed UV/topology/material/socket/collision data, and modified head/body geometry on later probes. It rebuilds only the exact existing mesh through the same public mesh-description APIs already used in TASK-055; it does not depend on ambiguous OBJ import axis conversion. Do not use the original FBX's ordinary Reimport button afterward: that would restore the old thick handle, which the new test is intended to catch.

## Evidence and limits

See `docs/qa/TASK-104/grip/` for whole-source before/after and per-rig Blender renders, reference-matrix checks, core dimensions and fingertip metrics. Full source-rig hand triangles have minimum axis clearance 1.124 cm (Hero, 227 triangles) and 0.997 cm (Brother, 387 triangles) around the 0.9 cm-radius core. Fingertip centres cover approximately 243°/227° around the shaft. These are offline geometric checks, not animation/runtime PASS.

The pose fit excludes the previously documented remote vertex influenced by thigh bones from the hand-contact set; no weights were changed. Hands are original low-poly source geometry and the diagnostic renders use neutral materials. Hero pinky has a larger remaining fingertip gap (closest tip-to-axis ~2.40 cm); a tight contact claim would be inaccurate. Blend transitions, full materials, compressed animations, LODs, hand self-intersection, blade collisions and visual quality still require UE validation.

API references: [StaticMesh](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/StaticMesh?application_version=5.4), [MeshDescriptionBase](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MeshDescriptionBase?application_version=5.6), [StaticMeshDescription](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/StaticMeshDescription?application_version=5.6), [StaticMeshEditorSubsystem](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/StaticMeshEditorSubsystem?application_version=5.6). These references establish exposed API names, not execution against the project's unverified engine installation.
