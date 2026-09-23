"""Read-only S1 after views at the immutable R0 camera coordinates."""
from pathlib import Path
import json
import time
import unreal

root=Path(unreal.Paths.project_dir())
out=root/'docs/qa/evidence/TASK-026/rework-v2/S1-editor-after-05'
out.mkdir(parents=True,exist_ok=False)
views=json.loads((root/'docs/world/TASK-026/rework-v2/views.json').read_text(encoding='utf-8'))
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
state={'index':0,'phase':'camera','until':0,'busy':False}
unreal.EditorPythonScripting.set_keep_python_script_alive(True)

def tick(delta):
    if state['busy'] or time.monotonic()<state['until']:
        return
    state['busy']=True
    try:
        if state['index']==len(views):
            unreal.unregister_slate_post_tick_callback(handle)
            (out/'report.json').write_text(json.dumps({'status':'CAPTURED','scope':'editor fixed after views','views':views}),encoding='utf-8')
            return
        view=views[state['index']]
        if state['phase']=='camera':
            pitch,yaw,roll=view['rotation']
            unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(*view['position_cm']),unreal.Rotator(pitch=pitch,yaw=yaw,roll=roll))
            state.update(phase='capture',until=time.monotonic()+10)
        elif state['phase']=='capture':
            state['task']=unreal.AutomationLibrary.take_high_res_screenshot(1920,1080,str(out/(view['id']+'.png')),force_game_view=True)
            state.update(phase='wait',until=time.monotonic()+2)
        elif state['task'].is_task_done():
            state.update(index=state['index']+1,phase='camera')
    finally:
        state['busy']=False

handle=unreal.register_slate_post_tick_callback(tick)
