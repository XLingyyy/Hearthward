"""Raise both ford surfaces 30 cm above the adjacent riverbed."""

import json
import math
import traceback
from pathlib import Path

import unreal


root = Path(unreal.Paths.project_dir())
out = root / "Saved/Task026/raise-fords.json"
out.parent.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def river_x(y):
    return 11000.0 * math.sin(y / 42000.0) + 4500.0 * math.sin(y / 18000.0)


try:
    assert levels.load_level("/Game/Hearthward/World/Natural/L_NaturalWorld")
    matches = [actor for actor in actors.get_all_level_actors() if actor.get_actor_label() == "Fords"]
    assert len(matches) == 1
    component = matches[0].get_component_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
    assert component and component.get_instance_count() == 2
    positions = []
    for index, y in enumerate((-50000.0, 50000.0)):
        x = river_x(y)
        changed = component.update_instance_transform(
            index,
            unreal.Transform(
                location=unreal.Vector(x, y, 4830),
                scale=unreal.Vector(360, 180, 4),
            ),
            False,
            True,
            True,
        )
        assert changed
        positions.append([x, y, 4830])
    assert levels.save_current_level()
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    result = {"ok": True, "centers_cm": positions, "surface_z_cm": 5030}
except Exception:
    result = {"ok": False, "error": traceback.format_exc()}

out.write_text(json.dumps(result, indent=2), encoding="utf-8")
if not result["ok"]:
    raise RuntimeError(result["error"])
