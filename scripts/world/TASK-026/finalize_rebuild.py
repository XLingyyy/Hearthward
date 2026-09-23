"""Persist SM6 material usage and validate the selected natural mesh settings."""
from pathlib import Path
import json
import unreal
root=Path(unreal.Paths.project_dir())
asset='/Game/Hearthward/Assets/NaturalWorld/Rebuild'
ml=unreal.MaterialEditingLibrary
mesh_editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
report={'meshes':{},'materials':[]}
for path in unreal.EditorAssetLibrary.list_assets(asset+'/Materials',True,False):
    mat=unreal.load_asset(path)
    if not isinstance(mat,unreal.Material):continue
    mat.set_editor_properties(dict(used_with_nanite=True,used_with_instanced_static_meshes=True))
    ml.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    report['materials'].append(mat.get_path_name())
for name in ['Tree','Shrub','Rock','Stump']:
    mesh=unreal.load_asset(asset+'/Meshes/SM_'+name)
    report['meshes'][name]=str(mesh_editor.get_nanite_settings(mesh))
unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(-160000,-145000,80000),unreal.Rotator(pitch=-23,yaw=43,roll=0))
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
(root/'Saved/Task026/Rebuild/sm6-finalization.json').write_text(json.dumps(report,indent=2))
