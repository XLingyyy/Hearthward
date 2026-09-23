"""Preserve distant foliage coverage and refresh native Landscape grass."""
from pathlib import Path
import json
import unreal
root=Path(unreal.Paths.project_dir())
asset='/Game/Hearthward/Assets/NaturalWorld/Rebuild'
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
task=unreal.AssetImportTask()
task.set_editor_properties(dict(filename=str(root/'art_source/TASK-026/Rebuild/water/GrassCards.glb'),
    destination_path=asset+'/Meshes',destination_name='SM_GrassCards',automated=True,save=True,replace_existing=True))
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
mesh=unreal.load_asset(asset+'/Meshes/GrassCards/StaticMeshes/SM_GrassCards')
mesh.set_material(0,unreal.load_asset(asset+'/Materials/M_GrassCards'))
unreal.EditorAssetLibrary.save_loaded_asset(mesh)
report={'console':{name:unreal.SystemLibrary.get_console_variable_int_value(name) for name in ['grass.Enable','grass.GrassMap.UseRuntimeGeneration']}}
report['density_scale']=unreal.SystemLibrary.get_console_variable_float_value('grass.DensityScale')
for name in ['Tree_leaves_Alpha','Shrub_Alpha','GrassCard_A']:
    texture=unreal.load_asset(asset+'/Textures/T_'+name)
    texture.set_editor_properties(dict(do_scale_mips_for_alpha_coverage=True,alpha_coverage_thresholds=unreal.Vector4(.35,0,0,0)))
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
for actor in actors.get_all_level_actors():
    if actor.get_actor_label()=='Sea':
        actor.modify();actor.set_actor_location(unreal.Vector(-4000000,0,0),False,False)
    if isinstance(actor,unreal.LandscapeProxy):
        report.setdefault('landscape_materials',set()).add(str(actor.get_editor_property('landscape_material')))
        report.setdefault('grass_components',[]).extend([{'class':c.get_class().get_name(),'instances':c.get_instance_count()} for c in actor.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent)])
report['landscape_materials']=sorted(report['landscape_materials'])
grass=unreal.load_asset(asset+'/Foliage/GT_Meadow')
report['grass_type']=str(grass.get_editor_property('grass_varieties'))
unreal.SystemLibrary.execute_console_command(world,'grass.FlushCache')
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(-100000,-95000,22000),unreal.Rotator(pitch=-10,yaw=55,roll=0))
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
(root/'Saved/Task026/Rebuild/surface-diagnostic.json').write_text(json.dumps(report,indent=2))
