"""Targeted melee regression in the explicit prototype encounter world."""
import json, math, re, time, traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task029Integration/equipped-attack.json'
out.parent.mkdir(parents=True,exist_ok=True)
report={'passed':False,'checks':{},'scope':'explicit development encounter fixture; natural map currently has no enemy actors or physical mouse verification'}
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
def check(name,value): report['checks'][name]=bool(value)
def wait(fn,seconds=45): return fn,time.monotonic()+seconds
def delay(seconds):
    deadline=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=deadline)
def game(): return editor.get_game_world()
def screen():
    world=game();pc=unreal.GameplayStatics.get_player_controller(world,0) if world else None
    return pc.get_hud().screen if pc and pc.get_hud() else None
def run():
    check('isolated_pool',bool(re.search(r'-HearthwardSaveTestPool=[0-9a-fA-F-]+',unreal.SystemLibrary.get_command_line())))
    levels.editor_request_begin_play()
    yield wait(levels.is_in_play_in_editor)
    yield delay(3)
    pawn=unreal.GameplayStatics.get_player_pawn(game(),0)
    gameplay=pawn.get_component_by_class(unreal.HearthwardGameplayComponent)
    bag=pawn.get_component_by_class(unreal.HearthwardInventoryComponent)
    pc=unreal.GameplayStatics.get_player_controller(game(),0)
    unreal.SystemLibrary.execute_console_command(game(),'Hearthward.Companion.CreateTest',pc)
    yield delay(.5)
    companion=unreal.GameplayStatics.get_all_actors_of_class(game(),unreal.HearthwardCompanionFixture)[0]
    save=next(x for x in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if x.get_outer()==game())
    check('prototype_session',save.enable_prototype() and save.start_new_progress())
    report['level']=str(unreal.GameplayStatics.get_current_level_name(game(),True))
    gameplay.enable_adventure()
    report['adventure_enabled']=bool(gameplay.enabled)
    report['opponents']={str(k):v for k,v in gameplay.opponents.items()}
    bag.try_add('axe',1)
    check('starter_axe',bag.get_item_count('axe')>=1)
    check('equip',gameplay.equip('axe'))
    durability_before=gameplay.durability.get(unreal.Name('axe'),0)
    report['miss_result']=gameplay.attack()
    mesh=pawn.get_component_by_class(unreal.SkeletalMeshComponent)
    yield delay(.05)
    anim=mesh.get_anim_instance() if mesh else None
    check('miss_animates',bool(anim and str(anim.motion_state)=='Attack'))
    check('miss_does_not_enter_combat',not gameplay.in_combat())
    check('miss_does_not_wear_axe',gameplay.durability.get(unreal.Name('axe'),0)==durability_before)
    catalog=json.loads((Path(unreal.Paths.project_dir())/'Resources/Data/gameplay.json').read_text(encoding='utf-8'))
    offset=catalog['encounters'][0]['position'];camp=companion.camp.get_actor_location()
    target=unreal.Vector(camp.x+offset[0],camp.y+offset[1],camp.z-100+offset[2])
    candidates=[]
    for actor in unreal.GameplayStatics.get_all_actors_of_class(game(),unreal.Actor):
        scale=actor.get_actor_scale3d()
        if abs(scale.x-.7)<.01 and abs(scale.y-.7)<.01 and abs(scale.z-1.7)<.01:
            pos=actor.get_actor_location();candidates.append([pos.x,pos.y,pos.z])
    report['encounter_actor_positions']=candidates
    if candidates:target=unreal.Vector(*candidates[0])
    # The graybox arena blocks a direct walking line near x=916. This explicit
    # development fixture places the pawn within melee range to isolate damage.
    report['enemy_target']=[target.x,target.y,target.z]
    pawn.set_actor_location(unreal.Vector(target.x-160,target.y,target.z),False,False)
    yield delay(1.0)  # wait past the real 0.65 s attack cooldown
    placed=pawn.get_actor_location()
    report['player_at_hit']=[placed.x,placed.y,placed.z]
    report['distance_to_enemy']=math.sqrt((placed.x-target.x)**2+(placed.y-target.y)**2+(placed.z-target.z)**2)
    check('fixture_within_melee_range',report['distance_to_enemy']<200)
    report['hit_result']=gameplay.attack()
    report['hit_feedback']=str(gameplay.feedback)
    check('real_hit',report['hit_result'])
    check('weapon_wear_on_hit',gameplay.durability.get(unreal.Name('axe'),0)<durability_before)
    report['passed']=all(report['checks'].values())
    out.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    levels.editor_request_end_play();unreal.SystemLibrary.quit_editor()

it=run();pending=None
def tick(_):
    global pending
    try:
        if pending:
            fn,deadline=pending
            if not fn():
                if time.monotonic()>deadline:raise TimeoutError('equipped attack wait')
                return
        pending=next(it)
    except StopIteration:unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['error']=traceback.format_exc()
        out.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        unreal.log_error(report['error'])
        unreal.unregister_slate_post_tick_callback(handle)
        levels.editor_request_end_play();unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
