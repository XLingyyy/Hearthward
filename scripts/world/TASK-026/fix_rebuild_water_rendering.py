"""Single Layer Water cannot use Nanite; keep procedural water meshes conventional."""
from pathlib import Path
import json,unreal
root=Path(unreal.Paths.project_dir());asset='/Game/Hearthward/Assets/NaturalWorld/Rebuild'
meshes=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
report=[]
for item in json.loads((root/'art_source/TASK-026/Rebuild/water.json').read_text()):
    name=item['name'];mesh=unreal.load_asset(asset+f'/Meshes/{name}/StaticMeshes/SM_{name}')
    settings=meshes.get_nanite_settings(mesh);before=settings.enabled
    settings.set_editor_property('enabled',False);meshes.set_nanite_settings(mesh,settings)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    report.append({'mesh':mesh.get_path_name(),'nanite_before':before,'nanite_after':False})
(root/'Saved/Task026/Rebuild/water-rendering.json').write_text(json.dumps(report,indent=2))
