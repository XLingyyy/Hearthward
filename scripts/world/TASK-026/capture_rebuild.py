"""Capture unretouched SM6 editor views; all cells loaded for visual inspection."""
from pathlib import Path
import time,json,unreal
root=Path(unreal.Paths.project_dir());out=root/'docs/qa/evidence/TASK-026/rebuild/views';out.mkdir(parents=True,exist_ok=True)
unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
views=[('overview',-1500,-1900,1150,-25,52),('forest_lake',-1300,-920,240,-10,55),('river_plateau',350,-1250,260,-12,50),('coastal_cliffs',1420,700,320,-15,-40)]
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
state={'index':0,'phase':'camera','until':0,'task':None}
def tick(delta):
    if time.monotonic()<state['until']:return
    if state['index']>=len(views):
        unreal.unregister_slate_post_tick_callback(handle)
        (out/'views.json').write_text(json.dumps({'scope':'SM6 editor, all cells loaded, no image retouching; not streaming/performance evidence','views':views},indent=2))
        unreal.EditorPythonScripting.set_keep_python_script_alive(False);return
    name,x,y,z,pitch,yaw=views[state['index']]
    if state['phase']=='camera':
        unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(x*100,y*100,z*100),unreal.Rotator(pitch=pitch,yaw=yaw,roll=0))
        state.update(phase='capture',until=time.monotonic()+8)
    elif state['phase']=='capture':
        state['task']=unreal.AutomationLibrary.take_high_res_screenshot(1600,900,str(out/(name+'.png')),force_game_view=True)
        state.update(phase='wait',until=time.monotonic()+2)
    elif state['task'].is_task_done():state.update(index=state['index']+1,phase='camera')
handle=unreal.register_slate_post_tick_callback(tick)
