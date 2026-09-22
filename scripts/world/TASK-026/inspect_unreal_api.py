"""Dump the small Unreal Python surface needed by TASK-026.

This is an editor-only diagnostic. It does not create or save project assets.
"""

import json
from pathlib import Path

import unreal


root = Path(unreal.Paths.project_dir())
output = root / "Saved" / "Task026" / "unreal-api.json"
output.parent.mkdir(parents=True, exist_ok=True)


def public_names(value):
    return sorted(name for name in dir(value) if not name.startswith("_"))


payload = {
    "level_editor_subsystem": public_names(unreal.LevelEditorSubsystem),
    "editor_actor_subsystem": public_names(unreal.EditorActorSubsystem),
    "actor": public_names(unreal.Actor),
    "hierarchical_instanced_static_mesh_component": public_names(
        unreal.HierarchicalInstancedStaticMeshComponent
    ),
    "landscape": public_names(unreal.Landscape),
    "landscape_proxy": public_names(unreal.LandscapeProxy),
}
output.write_text(json.dumps(payload, indent=2), encoding="utf-8")
unreal.log(f"TASK026_API_DUMP={output}")
