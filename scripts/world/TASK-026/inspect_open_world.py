"""Inspect the UE 5.8 Open World template without saving changes."""

import json
from pathlib import Path

import unreal


root = Path(unreal.Paths.project_dir())
output = root / "Saved" / "Task026" / "open-world-template.json"
output.parent.mkdir(parents=True, exist_ok=True)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level("/Engine/Maps/Templates/OpenWorld")

items = []
for actor in actors.get_all_level_actors():
    origin, extent = actor.get_actor_bounds(False)
    items.append(
        {
            "label": actor.get_actor_label(),
            "class": actor.get_class().get_name(),
            "location_cm": [actor.get_actor_location().x, actor.get_actor_location().y, actor.get_actor_location().z],
            "scale": [actor.get_actor_scale3d().x, actor.get_actor_scale3d().y, actor.get_actor_scale3d().z],
            "bounds_origin_cm": [origin.x, origin.y, origin.z],
            "bounds_extent_cm": [extent.x, extent.y, extent.z],
            "spatially_loaded": bool(actor.get_editor_property("is_spatially_loaded")),
        }
    )

payload = {"actor_count": len(items), "actors": items}
output.write_text(json.dumps(payload, indent=2), encoding="utf-8")
unreal.log(f"TASK026_TEMPLATE_ACTORS={len(items)}")
