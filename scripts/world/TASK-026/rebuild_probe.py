"""Run from Unreal's File > Execute Python Script, in the current editor."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
out = root / 'Saved/Task026/Rebuild'
out.mkdir(parents=True, exist_ok=True)
classes = ['Landscape', 'LandscapeStreamingProxy', 'LandscapeSubsystem', 'WorldPartitionEditorSubsystem',
           'EditorStaticMeshSubsystem', 'RenderingLibrary', 'MaterialEditingLibrary']
result = {}
for name in classes:
    cls = getattr(unreal, name, None)
    result[name] = [v for v in dir(cls) if any(k in v for k in ['height', 'layer', 'load', 'region', 'render_target', 'lod', 'collision', 'bounds'])] if cls else None
result['landscapes'] = []
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor, unreal.LandscapeProxy):
        result['landscapes'].append({'class':actor.get_class().get_name(), 'location':str(actor.get_actor_location()), 'scale':str(actor.get_actor_scale3d()), 'bounds':str(actor.get_actor_bounds(False))})
result['rt_doc'] = unreal.RenderingLibrary.create_render_target2d.__doc__
result['import_doc'] = unreal.LandscapeProxy.landscape_import_heightmap_from_render_target.__doc__
(out/'probe.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
