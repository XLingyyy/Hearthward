from pathlib import Path
import json
import unreal
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
out=[]
for a in actors.get_all_level_actors():
    if isinstance(a,unreal.PlayerStart) or a.get_actor_label().startswith('tree_-4_-3'):
        row={'label':a.get_actor_label(),'class':a.get_class().get_name(),'location':str(a.get_actor_location()),'spatial':a.get_editor_property('is_spatially_loaded')}
        c=a.get_component_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
        if c:
            row.update(component=str(c.get_world_transform()),instance=str(c.get_instance_transform(0,True)),materials=[str(c.get_material(i)) for i in range(c.get_num_materials())])
        out.append(row)
Path(unreal.Paths.project_dir(),'Saved/Task026/Rebuild/scene-inspection.json').write_text(json.dumps(out,indent=2))
