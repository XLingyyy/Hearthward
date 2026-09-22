"""Non-saving probe for an actor-owned HISM component."""

import json
import traceback
from pathlib import Path

import unreal


root = Path(unreal.Paths.project_dir())
output = root / "Saved" / "Task026" / "hism-probe.json"
output.parent.mkdir(parents=True, exist_ok=True)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

try:
    assert levels.load_level("/Engine/Maps/Templates/OpenWorld")
    actor = actors.spawn_actor_from_class(unreal.Actor, unreal.Vector())
    actor.set_actor_label("TASK026_HISM_PROBE_DO_NOT_SAVE")
    component = unreal.new_object(
        unreal.HierarchicalInstancedStaticMeshComponent,
        outer=actor,
        name="ProbeInstances",
    )
    component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cone"))
    actor.set_editor_property("root_component", component)
    index = component.add_instance(
        unreal.Transform(
            location=unreal.Vector(100, 200, 300),
            scale=unreal.Vector(2, 2, 2),
        )
    )
    result = {
        "ok": index == 0 and component.get_instance_count() == 1,
        "index": index,
        "count": component.get_instance_count(),
        "component": component.get_path_name(),
    }
except Exception:
    result = {"ok": False, "error": traceback.format_exc()}

output.write_text(json.dumps(result, indent=2), encoding="utf-8")
unreal.log(f"TASK026_HISM_PROBE={result}")
if not result["ok"]:
    raise RuntimeError(result.get("error", result))
