"""Non-saving line trace probe against the Open World landscape."""

import json
import traceback
from pathlib import Path

import unreal


root = Path(unreal.Paths.project_dir())
output = root / "Saved" / "Task026" / "landscape-trace-probe.json"
output.parent.mkdir(parents=True, exist_ok=True)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

try:
    assert levels.load_level("/Engine/Maps/Templates/OpenWorld")
    world = levels.get_world()
    doc = str(unreal.SystemLibrary.line_trace_single.__doc__)
    samples = []
    for x, y in [(0, 0), (-50000, -50000), (50000, 50000), (80000, -80000)]:
        trace_result = unreal.SystemLibrary.line_trace_single(
            world,
            unreal.Vector(x, y, 100000),
            unreal.Vector(x, y, -100000),
            unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
            True,
            [],
            unreal.DrawDebugTrace.NONE,
            True,
        )
        samples.append({"x": x, "y": y, "result": repr(trace_result)})
    payload = {"ok": True, "doc": doc, "samples": samples}
except Exception:
    payload = {"ok": False, "error": traceback.format_exc()}

output.write_text(json.dumps(payload, indent=2), encoding="utf-8")
if not payload["ok"]:
    raise RuntimeError(payload.get("error", payload))
