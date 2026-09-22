"""Place the natural-world start inside a terrain tile, away from tile seams."""

import json
import traceback
from pathlib import Path

import unreal


root = Path(unreal.Paths.project_dir())
out = root / "Saved/Task026/player-start-update.json"
out.parent.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

try:
    assert levels.load_level("/Game/Hearthward/World/Natural/L_NaturalWorld")
    starts = [actor for actor in actors.get_all_level_actors() if actor.get_class().get_name() == "PlayerStart"]
    assert len(starts) == 1
    starts[0].set_actor_location(unreal.Vector(-62500, -62500, 6200), False, False)
    starts[0].set_actor_rotation(unreal.Rotator(0, 45, 0), False)
    assert levels.save_current_level()
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    result = {"ok": True, "location_cm": [-62500, -62500, 6200], "yaw": 45}
except Exception:
    result = {"ok": False, "error": traceback.format_exc()}

out.write_text(json.dumps(result, indent=2), encoding="utf-8")
if not result["ok"]:
    raise RuntimeError(result["error"])
