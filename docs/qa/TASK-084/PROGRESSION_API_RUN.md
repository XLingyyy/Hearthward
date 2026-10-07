# TASK-084 fresh prologue API probe

Root executes this runner. The current navigation mode is **STATIC_CHECKED / runtime NOT_RUN**. Previous HTTP-pulse and remote-Python attempts remain failed evidence below. No QA preparation starts UE, builds Source, submits a model request, or modifies the game.

```powershell
G:/GameFactory/.venv/Scripts/python.exe -X utf8 G:/GameFactory/Hearthward/docs/qa/TASK-084/run_prologue_progression.py --run fresh-prologue-navigation-api-20261007-03 --input-mode navigation --build-evidence <latest actual successful Editor build JSON> --startup-timeout 180 --route-timeout 240
```

Use the actual build13 Editor evidence when Root runs it after the package process ends. The argument accepts any JSON basename, checks actual `build.project`, HearthwardEditor/Win64/Development, dry_run=false and returncode=0, and records the DLL size/mtime without hashing. A later failed build does not inherit an earlier binary binding. Root keeps that DLL unchanged during the owned run; no auto-build occurs. Every run name must be new.

## Current public navigation path

The runner launches one owned Development Editor `-game` process through public UEClient, with a fresh UUID save pool, an isolated profile and an exact RC function allowlist. It starts at the actual Bootstrap title, invokes the real menu new-game action, waits for real loading/intro completion, and reads current Campaign goals. The existing complete `FindPathToLocationSynchronously` result is validated. `AIBlueprintHelperLibrary.SimpleMoveToLocation` then receives the actual local PlayerController and the actual Campaign goal. The controller's real `GetCurrentPath` is read back and must be valid, nonpartial and contain usable points.

This public helper explicitly supports PlayerController in installed UE5.8 source: `AIBlueprintHelperLibrary.cpp`439–459 finds or registers a normal UPathFollowingComponent, and 527–594 requests actual synchronous navigation. `PathFollowingComponent.cpp`675–688 advances the route on normal engine ticks. `FollowPathSegment` calls `RequestPathMove` or `RequestDirectMove` according to the existing Character navigation settings. The QA driver does not alter those settings or directly write location, velocity, movement mode, clock or Tick. This is **Development/API navigation evidence**; navigation mode receives no AddMovementInput or normal OS keyboard credit.

The host continues reading the actual pause/HUD/input-ignore/life/combat/timed-action gates while moving. It logs actual position and goal distance at least once per second of successful polling. Eight seconds without 20 cm accumulated progress, an invalid/partial path, a safety gate, death, timeout or API failure stops the route. Arrival, error and timeout use the public Controller.StopMovement operation; no movement resumes after a failed gate. A stop failure is recorded without replacing the original route error. Cleanup finally stops the owned UE PID and verifies its exit.

Ordinary Campaign.Interact must earn the relic fact, the existing direct follow order must earn prologue_order, and the ordinary escape interaction must complete the game's own travel transaction and earn prologue_complete/occupied. Only then does the real menu save action invoke the normal safe SavePoint transaction. Campaign goal positions are observed; the QA script supplies no coordinates, gifts, quest facts, forced ticks, time changes or raw save state.

Startup is bounded to 180 seconds; the route defaults to 240 and cannot exceed 300. Every HTTP operation checks the own PID and fatal/critical log markers. A short cleanup allowance is restricted to stopping movement/screenshot requests after a deadline. Normal private Enhanced Input Move cancellation logic is outside public navigation; the QA host stops on busy gates and claims no full input-path equivalence.

## Security and error evidence

Current RC settings allow the exact console function needed for Shot SHOWUI but do not enable remote Python. Installed `WebRemoteControlInternalUtils.cpp`583–605 explicitly rejects PY/python through ExecuteConsoleCommand when bEnableRemotePythonExecution is false. Current `--input-mode tick` therefore fails argument validation before launching or creating a run; the host contains no PY command or EnablePython flag. No security flag, broader function wildcard or alternate command bypass is introduced. The unused prepared tick helper was archived in the second failed attempt and removed from the active QA folder.

HTTP errors now retain only the own loopback RC status and up to 16 KiB of UTF8 response body, with truncation and secret-value redaction flags. Headers are not read or recorded. Invalid UTF8 is NOT_READ; absent historic response bodies remain unknown. Future capture does not rewrite old raw errors.

## Outputs and limits

Outputs in `.agent-local/qa/TASK-084/<run>/` include results.json, http-events.jsonl, launch.json, stop.json, the exact per-process RC policy, runtime.log and the isolated profile. Screenshot requests alone receive no image credit. Complete actual route plus successful safe save reports PROLOGUE_EARNED_CHECKPOINT_API_VERIFIED; the first blocker reports FAILED_FIRST_BLOCKER and its stage. Exit zero additionally requires verified owned-process exit.

First rescue remains NOT_RUN: main quest02 requires actual wood/stone receipts, workbench/worker assignment, camp order and a real quest01 claim; rescue then requires the encounter, enemies, contact/escort and safe return. This short route does not create those prerequisites. TASK-102's other five stable scenes remain NOT_RUN: reached safe camp and usable facilities with real paid sleep/wait for day/night, a genuine sustained three-person combat encounter, an earned streaming route with actual cell loading, and real facilities/workers/production for camp management. The genuine old checkpoint tested in TASK-101 restored an outdoor nighttime camp-preparation objective with the camp 1,243 m away, which does not establish these conditions. Shipping, OS input, full route, audio audition and performance receive no credit.

## Preserved attempts

- `fresh-prologue-api-20261007-01`: HTTP pulse mode returned a complete NavPath, sent 79 one-shot inputs (median 45.195 ms, max 517.436 ms), moved 13.38 cm, then failed its eight-second progress guard. Collision/navigation failure and input starvation remain unproven causes. Original report and qa-source-as-run.py are preserved; owned stop/exit verified, actual screenshot output absent.
- `fresh-prologue-tick-api-20261007-01`: Bootstrap initial RC connection refusal exposed a main-scope urllib.error shadowing bug. The cleanup exception variable was renamed. Actual old handler AST reproduced UnboundLocalError; the revised pure-Python handler probe entered RETRY_CONNECTION without network/UE. Original failure/as-run source retained. No tick callback or movement reached.
- `fresh-prologue-tick-api-20261007-02`: real new-game loading and safety passed; PY console call returned 400 while the same function Shot SHOWUI returned 200. No helper/state/callback or movement was created; owned stop/exit verified. Original body was not retained. The source's expected security error text is recorded as source evidence, never as the missing actual HTTP body. The -EnablePython attempt also produced engine ToolsetDefinition/PythonTestRunner initialization errors, which do not prove a project navigation/collision defect.

Public references: [GetCurrentPath](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/AIModule/UAIBlueprintHelperLibrary/GetCurrentPath), [Find Path to Location Synchronously](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/AI/Navigation/FindPathtoLocationSynchronously). Installed UE5.8 headers confirm SimpleMoveToLocation/GetCurrentPath and Controller.StopMovement are reflected public APIs. Current navigation-mode runtime exposure and earned gameplay remain to be tested.
