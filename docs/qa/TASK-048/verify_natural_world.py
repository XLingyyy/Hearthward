"""Natural-map spawn/interaction/render verification; no map asset is modified."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task048/natural';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'ok':False,'checks':{},'method':'Actual natural world, new game, resource interaction and rendering; camera/player repositioned for observation'};st={}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def wait(predicate,seconds=45):return predicate,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>end,seconds+8)
def state():return json.loads(st['nature'].describe())
def vector(p):return unreal.Vector(p['x'],p['y'],p['z'])
def shot(name):unreal.SystemLibrary.execute_console_command(st['world'],f'Shot SHOWUI filename="{(out/name).as_posix()}.png" -nosuffix',st['pc'])
def finish(error=None):
    if error:report['error']=error
    report['ok']=not error and all(report['checks'].values())
    if st.get('nature'):report['state']=state()
    (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
def run():
    # Match the production title screen's OpenLevel game= override, without saving the map.
    editor_world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    editor_world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/Hearthward.HearthwardGameMode'))
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(8)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();p=unreal.GameplayStatics.get_player_pawn(w,0);pc=unreal.GameplayStatics.get_player_controller(w,0)
    st.update(world=w,pawn=p,pc=pc,ui=pc.get_hud().get_editor_property('screen'),bag=p.get_component_by_class(unreal.HearthwardInventoryComponent))
    st['nature']=next(x for x in unreal.ObjectIterator(unreal.HearthwardNatureSubsystem) if x.get_outer()==w)
    check('start natural new game',st['ui'].execute_action('new'));yield delay(8)
    s=state();counts={}
    for point in s['points']:counts[point['definition']]=counts.get(point['definition'],0)+1
    report['initial_counts']=counts;report['initial_wildlife']=sorted(set(a['definition'] for a in s['animals'] if not a['domestic']))
    check('eight wildlife species on actual terrain',len(report['initial_wildlife'])==8)
    check('twelve starter livestock on terrain',sum(a['domestic'] for a in s['animals'])==12)
    check('four lake fishing spots',sum(p['kind']=='fish' for p in s['points'])==4)
    for kind,count in {'food_patch':4,'fallen_branches':6,'loose_stones':4,'tree':12,'stone_outcrop':6,'herb_patch':4,'ore_vein':4,'rich_ore_vein':2,'wild_seed_greens':2,'wild_seed_grain':2,'wild_seed_herb':2}.items():check('terrain source '+kind,counts.get(kind,0)>=count)
    shot('camp-nature');yield delay(.6)
    target=next(x for x in s['points'] if x['definition']=='fallen_branches')
    p.set_actor_location(vector(target['position'])+unreal.Vector(-150,0,120),False,True);pc.set_control_rotation(unreal.Rotator(pitch=-15,yaw=0));yield delay(2)
    interaction=p.get_component_by_class(unreal.HearthwardInteractionComponent);nearest=interaction.get_nearest_target()
    check('resource has one native interaction',isinstance(nearest,unreal.HearthwardNatureInteraction) and len(nearest.get_owner().get_components_by_class(unreal.HearthwardInteractionTargetComponent))==1)
    report['selected_target']=nearest.get_owner().get_name() if nearest else None;report['position_before']=str(p.get_actor_location())
    before=st['bag'].get_item_count('wood');check('E interaction on real resource',interaction.interact_nearest());yield delay(6)
    report['nature_feedback']=str(st['nature'].feedback);report['interaction_feedback']=interaction.get_completion_feedback();report['position_after']=str(p.get_actor_location())
    yield wait(lambda:not st['nature'].busy(),12);check('real source harvest gives wood',st['bag'].get_item_count('wood')>before);shot('resource-harvest');yield delay(.6)
    animal=next(x for x in state()['animals'] if x['definition']=='goat')
    # Observation offset crosses a hillside: start above it, then let character movement land.
    p.set_actor_location(vector(animal['position'])+unreal.Vector(-900,0,2000),False,True);pc.set_control_rotation(unreal.Rotator(pitch=-10,yaw=0));yield delay(4)
    check('observation player lands on terrain',not p.get_movement_component().is_falling());shot('provisional-animals');yield delay(.6)
    fish=next(x for x in state()['points'] if x['kind']=='fish');p.set_actor_location(vector(fish['position'])+unreal.Vector(-150,0,120),False,True);yield delay(3)
    result=unreal.GuidLibrary.parse_string_to_guid(fish['id']);st['ui'].open_nature(result[0] if isinstance(result,tuple) else result);shot('lake-fishing');yield delay(.6)
    finish()
runner=run();pending=None
def tick(delta):
    global pending
    try:
        if pending:
            predicate,deadline=pending
            if not predicate():
                if time.monotonic()>deadline:raise TimeoutError('Natural world condition timed out')
                return
        pending=next(runner)
    except StopIteration:pass
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
