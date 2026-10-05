# TASK-071 core cross-process disk roundtrip

Root ran write and read in separate actual Unreal automation processes, using isolated save pool d37c272e-ea98-4681-90ae-8f38bdb01889 and the same exact save test. The write process also ran the stone-axe test before the save test; a named /Temp/Task071RestartFixture package makes the map identity stable independently of prior fixture allocation. The production LoadPoint map guard remains intact.

- Write PID19568 saved node E7DCB9CC4FF716457F80399F199B484A through public SavePoint; test passed.
- Read PID21740 loaded the same real disk node twice through public LoadPoint; test passed, no errors or warnings.
- The archive retains the actual axe instance GUID, player/shared/Brother/source stock, active command GUID and counters, A/W and calendar origin, the original NPC agreement, event GUIDs/coverage/conversation state, and exact exchange. Repeated load/resave produced no additional operations or receipts.
- The first write fixture used a new GUID where PutPlayerMemory expects an invalid GUID for creating a record. The existing API generates the real new record GUID; the fixture was corrected. The first read fixture had a different default Untitled package and was correctly rejected by the production map guard. Neither fixture failure is counted as a save business RED.

Raw reports: Saved/Task053/held-axe055-green-restart071-write/index.json and Saved/Task053/restart071-stable-map-read/index.json; summary JSONs in docs/qa/TASK-053. QA manifest in Saved/Task071/D37C272EEA98468190AE8F38BDB01889/manifest.json.

This is an actual development fixture process restart and disk roundtrip. Mature active camp batches, nonempty treasure pending claims, two-camp population/ecology, A/W separation after facility skipping, natural-map menu/exit/continue, and real HTTP/UI late callback coverage remain separate and unverified.
