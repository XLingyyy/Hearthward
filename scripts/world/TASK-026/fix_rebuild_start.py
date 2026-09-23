import json
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())
routes=json.loads((root/'art_source/TASK-026/Rebuild/routes.json').read_text())
x,y,z=routes['loop'][0]
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
starts=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.PlayerStart)]
assert len(starts)==1
a=starts[0]
a.modify()
a.set_actor_location(unreal.Vector(x*100,y*100,z*100+120),False,False)
a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=90,roll=0),False)
a.set_editor_property('is_spatially_loaded',False)
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
