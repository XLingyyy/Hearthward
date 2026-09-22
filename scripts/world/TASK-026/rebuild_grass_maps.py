"""Refresh Landscape material state and persist grass-map build data."""
from pathlib import Path
import time,json,unreal
root=Path(unreal.Paths.project_dir())
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
mat=unreal.load_asset('/Game/Hearthward/Assets/NaturalWorld/Rebuild/Materials/M_Landscape')
proxies=[a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors() if isinstance(a,unreal.LandscapeProxy)]
for actor in proxies:
    actor.modify()
    actor.set_editor_property('landscape_material',None)
    actor.set_editor_property('landscape_material',mat)
unreal.SystemLibrary.execute_console_command(world,'grass.PrerenderGrassmaps 1')
unreal.SystemLibrary.execute_console_command(world,'grass.FlushCache')
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(-97600,-75200,16300),unreal.Rotator(pitch=-8,yaw=90,roll=0))
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
until=time.monotonic()+40
def tick(dt):
    if time.monotonic()<until:return
    unreal.unregister_slate_post_tick_callback(handle)
    for actor in proxies:actor.modify()
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
    unreal.SystemLibrary.execute_console_command(world,'grass.DumpGrassData')
    (root/'Saved/Task026/Rebuild/grass-map-rebuild.json').write_text(json.dumps({'landscape_actors':len(proxies),'material_state_refreshed':True,'saved':True}))
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
handle=unreal.register_slate_post_tick_callback(tick)
