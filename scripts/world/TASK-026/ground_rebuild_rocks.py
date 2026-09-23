"""Embed existing rock instances to remove visibly floating downhill edges."""
from pathlib import Path
from collections import defaultdict
import json,math
import unreal
root=Path(unreal.Paths.project_dir())
groups=defaultdict(list)
contacts=json.loads((root/'art_source/TASK-026/Rebuild/rock_contact_heights.json').read_text())
for item,contact in zip(json.loads((root/'art_source/TASK-026/Rebuild/scatter.json').read_text())['rock'],contacts):
    item[2]=contact
    groups[(math.floor(item[0]/252),math.floor(item[1]/252))].append(item)
unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
actors={a.get_actor_label():a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()}
count=0
for cell,items in groups.items():
    actor=actors[f'rock_{cell[0]}_{cell[1]}']; actor.modify()
    comp=actor.get_component_by_class(unreal.HierarchicalInstancedStaticMeshComponent);comp.modify()
    bounds=comp.static_mesh.get_bounds();low=bounds.origin.z-bounds.box_extent.z;height=bounds.box_extent.z*2
    for i,(x,y,z,yaw,desired) in enumerate(items):
        scale=desired*100/height
        transform=unreal.Transform(location=unreal.Vector(x*100-cell[0]*25200,y*100-cell[1]*25200,z*100-low*scale),rotation=unreal.Rotator(pitch=0,yaw=yaw,roll=0),scale=unreal.Vector(scale,scale,scale))
        assert comp.update_instance_transform(i,transform,False,i==len(items)-1,True)
        count+=1
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
(root/'Saved/Task026/Rebuild/grounded-rocks.json').write_text(json.dumps({'instances':count,'placement':'nine footprint samples, minimum ground minus 15% height'}))
