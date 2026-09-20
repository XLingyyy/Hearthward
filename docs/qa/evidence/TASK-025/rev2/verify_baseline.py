"""Run unchanged original-025 code in a detached worktree, on the same development inputs/fixture."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
r=Path(unreal.Paths.project_dir()).resolve();ev=r.parents[1]/'docs/qa/evidence/TASK-025/rev2';out=r/'Saved/Task025Rev2';out.mkdir(parents=True,exist_ok=True)
report={'passed':False,'code_sha':'d02b5fef111e783f80b99c3e96a1bc2d2cb97d3b','cases':[],'checks':{},'scope':'common pre-existing capabilities only; B and typed rule proposals excluded; original auto-accept policy retained'}
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);st={}
def check(n,v):
    report['checks'][n]=bool(v)
    if not v:raise AssertionError(n)
def wait(p,seconds=150):return p,time.monotonic()+seconds
def delay(seconds):
    t=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>t,seconds+5)
def sub(cls,w):return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();pc=unreal.GameplayStatics.get_player_controller(w,0);ui=pc.get_hud().screen
    check('new',ui.execute_action('new'));yield delay(.5)
    p=unreal.GameplayStatics.get_player_pawn(w,0);c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    a=sub(unreal.HearthwardLocalAISubsystem,w);s=sub(unreal.HearthwardSaveSubsystem,w);store=sub(unreal.HearthwardStorageSubsystem,w);st['ai']=a
    b=p.get_component_by_class(unreal.HearthwardBuildingComponent);bag=p.get_component_by_class(unreal.HearthwardInventoryComponent)
    c.set_actor_location(unreal.Vector(0,400,100),False,True);c.camp.set_actor_location(unreal.Vector(0,400,100),False,True);c.source.get_owner().set_actor_location(unreal.Vector(600,400,100),False,True)
    p.set_actor_location(unreal.Vector(-200,-200,100),False,True);pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0));yield delay(.6)
    bag.try_add('wood',8);check('select_station',b.select_building('workbench'));yield delay(.3);check('place_station',b.confirm_placement());yield wait(lambda:b.building_count()==1,12)
    p.set_actor_location(unreal.Vector(-110,-200,100),False,True);c.set_actor_location(unreal.Vector(-200,100,100),False,True);c.camp.set_actor_location(unreal.Vector(-200,100,100),False,True);c.source.get_owner().set_actor_location(unreal.Vector(400,100,100),False,True)
    c.bag.try_add('wood',12);c.bag.try_add('rope',4);c.bag.try_add('axe',1);c.source.try_add('wood',40)
    a.put_player_memory(p,c,unreal.Guid(),'preference','我喜欢清晨吃清淡的烤肉。');a.put_player_memory(p,c,unreal.Guid(),'claim','故乡门口有一棵银杏树。')
    yield delay(.5);check('snapshot',s.save_point(True));base=s.get_points()[-1].save_id
    previous=json.loads((ev/'baseline-partial-results.json').read_text(encoding='utf-8'))
    report['cases']=previous['cases'];report['continuation_of']='baseline-partial-manifest.json'
    for case in json.loads((ev/'dev.json').read_text(encoding='utf-8'))[len(report['cases']):42]:
        check(case['id']+'_reset',s.load_point(base));yield delay(.1);source=c.source.get_item_count('wood');rows=[]
        for text in case['turns']:
            accepted=a.submit_player_text(p,c,text);yield wait(lambda:not a.is_busy())
            rows.append({'input':text,'accepted':accepted,'raw':a.get_last_structured_result(),'intent':a.get_last_applied_intent(),'line':a.get_npc_line(),'status':a.get_status(),'seconds':a.get_last_latency_seconds(),'context':a.get_last_filtered_context()})
        last=rows[-1];raw=json.loads(last['raw']) if last['raw'] else {};expected=case['expected']
        semantic=raw.get('intent') in ['clarify','refuse','dialogue'] if expected=='safe' else raw.get('intent')==expected
        if expected=='collect':semantic=semantic and raw.get('quantity')==case['quantity'] and raw.get('item')=='wood'
        if last['intent']=='collect':
            deadline=time.monotonic()+100
            yield wait(lambda:c.get_phase() in [unreal.HearthwardCompanionPhase.COMPLETED,unreal.HearthwardCompanionPhase.WAITING_AT_CAMP] or time.monotonic()>deadline,105)
        actual={'phase':str(c.get_phase()),'requested':c.get_requested(),'delivered':c.get_delivered(),'source_debit':source-c.source.get_item_count('wood'),'warehouse_wood':store.get_item_count('wood')}
        correct=semantic and (expected!='collect' or (actual['delivered']==case['quantity'] and actual['source_debit']==case['quantity']))
        report['cases'].append({**case,'turn_results':rows,'model_understanding_pass':semantic,'actual':actual,'pass':correct})
        (out/'baseline-progress.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    report['completed_success']=sum(c['pass'] for c in report['cases'])/len(report['cases']);report['passed']=True
def finish(error=None):
    if error:report['error']=error;report['passed']=False
    (out/'baseline.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
flow=run();pending=None
def tick(dt):
    global pending
    try:
        if pending:
            p,d=pending
            if not p():
                if time.monotonic()>d:raise TimeoutError(st['ai'].get_status())
                return
        pending=next(flow)
    except StopIteration:finish()
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
