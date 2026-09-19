"""Create isolated damaged equipment and workbench, then only observe real UI input."""
from pathlib import Path
import time,json,unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task024';out.mkdir(exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
state={};last=None;start=time.monotonic()
seen={'physical_E_open':False,'mouse_repair_page':False,'physical_F_repair':False,'return_to_hud':False}
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
        bag=player.get_component_by_class(unreal.HearthwardInventoryComponent);bag.try_add('wood',10);bag.try_add('rope',1)
        game=player.get_component_by_class(unreal.HearthwardGameplayComponent);game.equip('axe')
        durability=dict(game.durability);durability['axe']=0;game.set_editor_property('durability',durability)
        companion=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HearthwardCompanionFixture)[0]
        companion.set_actor_location(unreal.Vector(-700,-700,90),False,True)
        player.set_actor_location(unreal.Vector(-200,-200,100),False,True);pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0))
        state.update(player=player,b=player.get_component_by_class(unreal.HearthwardBuildingComponent),bag=bag,game=game,ready_at=time.monotonic()+1)
    b=state['b'];bag=state['bag'];game=state['game']
    if time.monotonic()<state['ready_at']:return
    if 'selected' not in state:b.select_building('workbench');state['selected']=True;return
    if b.building_count()==0:
        if b.valid_placement and not b.is_building():b.confirm_placement()
        return
    if 'positioned' not in state:
        state['player'].set_actor_location(unreal.Vector(-110,-200,100),False,True);state['positioned']=True
    page=str(ui.get_page())
    seen['physical_E_open']|=page=='crafting'
    seen['mouse_repair_page']|=page=='repairing'
    seen['physical_F_repair']|=bag.get_item_count('wood')==0 and bag.get_item_count('rope')==0 and game.durability.get('axe')==100 and game.events.get('repair:axe',0)==1
    seen['return_to_hud']|=seen['physical_F_repair'] and page=='hud'
    record={'page':page,'wood':bag.get_item_count('wood'),'rope':bag.get_item_count('rope'),'durability':game.durability.get('axe'),'feedback':b.feedback,'checks':seen,'passed':all(seen.values())}
    text=json.dumps(record,ensure_ascii=False,indent=2)
    if text!=last:(out/'physical-input.json').write_text(text,encoding='utf-8');last=text
    if all(seen.values()):unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
