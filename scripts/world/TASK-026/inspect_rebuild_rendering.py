"""Read active foliage rendering settings without changing project configuration."""
from pathlib import Path
import json
import unreal
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
report={}
for name in ['r.Nanite','r.Nanite.ProjectEnabled','r.Nanite.MaxPixelsPerEdge','r.Nanite.Streaming.StreamingPoolSize','r.Nanite.AllowMaskedMaterials','foliage.ForceLOD','foliage.LODDistanceScale','r.ViewDistanceScale','r.MaterialQualityLevel']:
    report[name]=unreal.SystemLibrary.get_console_variable_float_value(name)
mesh=unreal.load_asset('/Game/Hearthward/Assets/NaturalWorld/Rebuild/Meshes/SM_Tree')
report['tree_bounds']=str(mesh.get_bounds())
gm=unreal.load_asset('/Game/Hearthward/Assets/NaturalWorld/Rebuild/Meshes/GrassCards/StaticMeshes/SM_GrassCards')
report['grass_nanite']=str(unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).get_nanite_settings(gm))
report['grass_materials']=[str(x.material_interface) for x in gm.static_materials]
report['grass_components']=[{'count':c.get_instance_count(),'mesh':str(c.static_mesh),'owner':str(c.get_owner())} for c in unreal.ObjectIterator(unreal.GrassInstancedStaticMeshComponent)]
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if 'tree' not in a.tags:continue
    comp=a.get_component_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
    report['component']={}
    for name in ['disallow_nanite','force_nanite_for_masked','forced_lod_model','min_lod','override_min_lod']:
        try:report['component'][name]=str(comp.get_editor_property(name))
        except Exception as e:report['component'][name]=str(e)
    break
(Path(unreal.Paths.project_dir())/'Saved/Task026/Rebuild/rendering.json').write_text(json.dumps(report,indent=2))
