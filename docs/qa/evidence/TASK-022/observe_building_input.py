"""Prepare an isolated smoke scene and observe physical UI inputs; does not inject input."""
from pathlib import Path
import time,json,unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task022';out.mkdir(exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
state={};last=None;start=time.monotonic();seen={'catalog':False,'preview':False,'rotation':False,'construction_prompt':False,'built':False}
levels.editor_request_begin_play()
def tick(delta):
    global last
    if not levels.is_in_play_in_editor() or time.monotonic()-start<3:return
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0)
    if not pc or not pc.get_hud():return
    ui=pc.get_hud().screen
    if not state:
        if not ui.execute_action('new'):return
        player=unreal.GameplayStatics.get_player_pawn(world,0)
        bag=player.get_component_by_class(unreal.HearthwardInventoryComponent);bag.try_add('wood',12)
        companion=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HearthwardCompanionFixture)[0]
        companion.set_actor_location(unreal.Vector(-700,-700,90),False,True)
        player.set_actor_location(unreal.Vector(-200,-200,100),False,True)
        pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0))
        state.update(building=player.get_component_by_class(unreal.HearthwardBuildingComponent),bag=bag)
    b=state['building'];bag=state['bag']
    if str(ui.get_page())=='building' and not seen['catalog']:
        pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0))
        seen['catalog']=True
    if b.is_placing() and 'initial_yaw' not in state:state['initial_yaw']=b.yaw
    seen['preview']|=b.is_placing()
    seen['rotation']|=b.is_placing() and abs((b.yaw-state['initial_yaw'])%360-15)<.1
    seen['built']|=b.building_count()==1 and bag.get_item_count('wood')==4
    seen['construction_prompt']|=b.is_building() and b.feedback=='建造中，移动或受伤将中断'
    record={'page':str(ui.get_page()),'placing':b.is_placing(),'valid':b.valid_placement,'yaw':b.yaw,'pending':b.is_building(),'buildings':b.building_count(),'wood':bag.get_item_count('wood'),'feedback':b.feedback,'checks':seen,'passed':all(seen.values())}
    text=json.dumps(record,ensure_ascii=False,indent=2)
    if text!=last:
        (out/'physical-input.json').write_text(text,encoding='utf-8');last=text
    if all(seen.values()):
        ui.capture_ui('task022-physical-build',1280,720)
        unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
