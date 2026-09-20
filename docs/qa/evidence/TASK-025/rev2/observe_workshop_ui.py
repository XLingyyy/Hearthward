"""Physical input only for NPC craft/repair; script prepares and observes the explicit fixture."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
r=Path(unreal.Paths.project_dir());out=r/'Saved/Task025Rev2';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);st={};trace=[];checks={}
def wait(p,seconds=300):return p,time.monotonic()+seconds
def delay(s):
    t=time.monotonic()+s;return wait(lambda:time.monotonic()>t,s+5)
def check(k,v):
    checks[k]=bool(v)
    if not v:raise AssertionError(k)
def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();pc=unreal.GameplayStatics.get_player_controller(w,0);ui=pc.get_hud().screen
    check('new',ui.execute_action('new'));yield delay(.5)
    p=unreal.GameplayStatics.get_player_pawn(w,0);b=p.get_component_by_class(unreal.HearthwardBuildingComponent);bag=p.get_component_by_class(unreal.HearthwardInventoryComponent)
    c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    p.set_actor_location(unreal.Vector(-200,-200,100),False,True);pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0));yield delay(.5)
    bag.try_add('wood',8);b.select_building('workbench');yield delay(.3);check('build_station',b.confirm_placement());yield wait(lambda:b.building_count()==1,15)
    p.set_actor_location(unreal.Vector(-110,-200,100),False,True);c.set_actor_location(unreal.Vector(-200,100,100),False,True);c.camp.set_actor_location(unreal.Vector(-200,100,100),False,True)
    c.bag.try_add('wood',6);c.bag.try_add('rope',2);c.bag.try_add('axe',1);c.set_editor_property('owned_durability',{'axe':20.0})
    a=next(x for x in unreal.ObjectIterator(unreal.HearthwardLocalAISubsystem) if x.get_outer()==w);store=next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if x.get_outer()==w)
    st.update(ai=a,comp=c,store=store);ui.execute_action('page:dialogue')
    (out/'physicalb-ready.json').write_text('{"ready":true}',encoding='utf-8')
    yield wait(a.has_candidate);check('craft_unconfirmed',c.get_requested()==0 and c.bag.get_item_count('wood')==6)
    yield wait(lambda:c.get_phase()==unreal.HearthwardCompanionPhase.COMPLETED)
    check('craft_effect',c.get_delivered()==8 and c.bag.get_item_count('wood')==4 and store.get_item_count('arrow')==8)
    yield wait(a.has_candidate);check('repair_unconfirmed',c.owned_durability.get('axe')==20 and c.bag.get_item_count('wood')==4)
    yield wait(lambda:c.get_phase()==unreal.HearthwardCompanionPhase.COMPLETED and c.owned_durability.get('axe')==100)
    check('repair_effect',c.bag.get_item_count('wood')==2 and c.bag.get_item_count('rope')==1 and c.bag.get_item_count('axe')==1 and store.get_item_count('arrow')==8)
    yield wait(a.has_candidate);check('rule_unconfirmed',len(a.get_player_memories())==0)
    yield wait(lambda:len(a.get_player_memories())==1)
    rule=a.get_player_memories()[0]
    check('rule_confirmed_by_ui',str(rule.kind)=='typed_constraint' and rule.constraint=='ban:wood')
def finish(error=None):
    (out/'physicalb.json').write_text(json.dumps({'passed':not error,'checks':checks,'trace':trace,'error':error,'physical_input':'computer-use; script never submits or confirms NPC goals'},ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
flow=run();pending=None;last=0
def tick(dt):
    global pending,last
    try:
        if st and time.monotonic()-last>.2:
            last=time.monotonic();a=st['ai'];c=st['comp']
            row={'input':a.get_last_input(),'raw':a.get_last_structured_result(),'status':a.get_status(),'card':a.get_candidate_text(),'candidate':a.has_candidate(),'requested':c.get_requested(),'delivered':c.get_delivered(),'wood':c.bag.get_item_count('wood'),'rope':c.bag.get_item_count('rope'),'durability':c.owned_durability.get('axe'),'warehouse_arrows':st['store'].get_item_count('arrow'),'player_memory_count':len(a.get_player_memories())}
            if not trace or row!=trace[-1]:trace.append(row)
            (out/'physicalb-trace.json').write_text(json.dumps(trace,ensure_ascii=False,indent=2),encoding='utf-8')
        if pending:
            p,d=pending
            if not p():
                if time.monotonic()>d:raise TimeoutError('physical action wait')
                return
        pending=next(flow)
    except StopIteration:finish()
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
