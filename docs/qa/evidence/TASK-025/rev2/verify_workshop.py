"""PIE evaluation; uses the shipped single-model path and real task-card confirmation."""
import json, time, traceback, os, sys
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
mode=os.environ.get('HEARTHWARD_REV2_MODE','smoke')
out=Path(unreal.Paths.project_saved_dir())/'Task025Rev2';out.mkdir(parents=True,exist_ok=True)
report={'passed':False,'mode':mode,'provider':'deterministic-PIE' if mode=='faults' else 'real-model','cases':[],'checks':{}}
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);st={}
frame_samples={'before':[],'during':[],'after':[]};model_seen=False
def check(n,v):
    report['checks'][n]=bool(v)
    if not v:raise AssertionError(n)
def wait(p,seconds=150):return p,time.monotonic()+seconds
def delay(s):
    t=time.monotonic()+s
    return wait(lambda:time.monotonic()>=t,s+5)
def subsystem(cls,w):return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();pc=unreal.GameplayStatics.get_player_controller(w,0);ui=pc.get_hud().screen
    check('new_campaign',ui.execute_action('new'));yield delay(.5)
    p=unreal.GameplayStatics.get_player_pawn(w,0);c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    a=subsystem(unreal.HearthwardLocalAISubsystem,w);s=subsystem(unreal.HearthwardSaveSubsystem,w);store=subsystem(unreal.HearthwardStorageSubsystem,w)
    st.update(ai=a,player=p,comp=c)
    c.set_actor_location(unreal.Vector(0,400,100),False,True);c.camp.set_actor_location(unreal.Vector(0,400,100),False,True)
    c.source.get_owner().set_actor_location(unreal.Vector(600,400,100),False,True);p.set_actor_location(unreal.Vector(-200,400,100),False,True)
    yield delay(1)
    if mode in ['faults','lifecycle']:
        sys.path.insert(0,str(Path(unreal.Paths.project_dir())/'docs/qa/evidence/TASK-025/rev2'))
        if mode=='faults':from fault_scenarios import scenarios
        else:from lifecycle_scenarios import scenarios
        yield from scenarios(check,wait,delay,w,pc,ui,p,c,a,s,store)
        return
    if mode in ['dev','heldout','cpu','b','workshop']:
        yield from evaluate(w,pc,ui,p,c,a,s,store)
        return
    samples=[('explicit','替我采集两份木材带回营地。',['proposal']),('quantity','帮我采些木材。',['clarify']),('followup','三份就够了。',['proposal']),('negative','不要采集木材，我只想聊聊天。',['dialogue','refuse']),('unknown','去尚未发现的北山采集四份木材。',['clarify','refuse']),('query','仓库现在有几份木材？',['inventory'])]
    for name,text,expected in samples:
        if name!='followup':a.clear_clarification()
        before=store.get_item_count('wood');requested=c.get_requested()
        check(name+'_sent',a.submit_player_text(p,c,text));yield wait(lambda:not a.is_busy())
        row={'name':name,'input':text,'intent':a.get_last_applied_intent(),'raw':a.get_last_structured_result(),'status':a.get_status(),'line':a.get_npc_line(),'reason':a.get_reason_code(),'input_tokens':a.get_input_tokens(),'output_tokens':a.get_output_tokens(),'latency':a.get_last_latency_seconds(),'expected':expected,'world_unchanged':store.get_item_count('wood')==before and c.get_requested()==requested}
        row['pass']=row['intent'] in expected and row['world_unchanged']
        if name=='followup':row['pass']=row['pass'] and json.loads(row['raw']).get('intent')=='collect' and json.loads(row['raw']).get('quantity')==3
        report['cases'].append(row)
        (out/(mode+'-progress.json')).write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        if name=='explicit' and a.has_candidate():
            ident=a.get_candidate_id();check('confirm_once',a.confirm_candidate(ident));check('duplicate_confirmation_rejected',not a.confirm_candidate(ident))
            yield wait(lambda:c.get_phase()==unreal.HearthwardCompanionPhase.COMPLETED,50)
            check('actual_delivery',store.get_item_count('wood')==before+2 and c.get_acquired()==2 and c.get_carried()==0 and c.get_delivered()==2)
            check('actual_events',len(a.get_events())>=3)
            check('save_completed',s.save_point(True));saved=s.get_points()[-1].save_id
            check('load_completed',s.load_point(saved));yield delay(.3)
        else:a.cancel_pending()
    check('all_samples',all(x['pass'] for x in report['cases']))
    levels.editor_request_end_play();yield wait(lambda:not levels.is_in_play_in_editor())

def evaluate(w,pc,ui,p,c,a,s,store):
    b=p.get_component_by_class(unreal.HearthwardBuildingComponent);bag=p.get_component_by_class(unreal.HearthwardInventoryComponent)
    p.set_actor_location(unreal.Vector(-200,-200,100),False,True);pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0));yield delay(.6)
    bag.try_add('wood',8);check('select_workbench',b.select_building('workbench'));yield delay(.3)
    check('build_workbench',b.confirm_placement());yield wait(lambda:b.building_count()==1,12)
    p.set_actor_location(unreal.Vector(-110,-200,100),False,True)
    c.set_actor_location(unreal.Vector(-200,100,100),False,True);c.camp.set_actor_location(unreal.Vector(-200,100,100),False,True)
    c.source.get_owner().set_actor_location(unreal.Vector(400,100,100),False,True)
    c.bag.try_add('wood',12);c.bag.try_add('rope',4);c.bag.try_add('axe',1);c.set_editor_property('owned_durability',{'axe':20.0})
    c.source.try_add('wood',40)
    check('seed_preference',a.put_player_memory(p,c,unreal.Guid(),'preference','我喜欢清晨吃清淡的烤肉。'))
    check('seed_claim',a.put_player_memory(p,c,unreal.Guid(),'claim','故乡门口有一棵银杏树。'))
    yield delay(.5);check('fixture_snapshot',s.save_point(True));base=s.get_points()[-1].save_id
    if mode=='workshop':
        sys.path.insert(0,str(Path(unreal.Paths.project_dir())/'docs/qa/evidence/TASK-025/rev2'))
        from workshop_scenarios import scenarios
        report['provider']='deterministic-PIE'
        yield from scenarios(check,wait,delay,w,pc,ui,p,c,a,s,store,base)
        return
    dataset=json.loads((Path(unreal.Paths.project_dir())/'docs/qa/evidence/TASK-025/rev2'/('dev.json' if mode in ['cpu','b'] else mode+'.json')).read_text(encoding='utf-8'))
    if mode=='cpu':dataset=dataset[:3]
    if mode=='b':dataset=[c for c in dataset if c['id'] in ['dev-043','dev-047','dev-052']]
    for case in dataset:
        check(case['id']+'_reset',s.load_point(base));yield delay(.1)
        rows=[];before={i:store.get_item_count(i) for i in ['wood','arrow','rope']}
        before_bag={i:c.bag.get_item_count(i) for i in ['wood','arrow','rope','axe']};source_before=c.source.get_item_count('wood')
        for text in case['turns']:
            t=time.monotonic();accepted=a.submit_player_text(p,c,text);response=time.monotonic()-t;yield wait(lambda:not a.is_busy())
            rows.append({'input':text,'accepted':accepted,'raw':a.get_last_structured_result(),'intent':a.get_last_applied_intent(),'reason':a.get_reason_code(),'line':a.get_npc_line(),'tokens_in':a.get_input_tokens(),'tokens_out':a.get_output_tokens(),'seconds':a.get_last_latency_seconds(),'submit_response_seconds':response,'context':a.get_last_filtered_context()})
        expected=case['expected'];last=rows[-1];raw=json.loads(last['raw']) if last['raw'] else {};candidate=a.has_candidate()
        unchanged=all(store.get_item_count(i)==n for i,n in before.items()) and c.get_requested()==0
        wanted='craft' if expected=='craft_rope' else expected
        semantic=(last['intent'] in ['clarify','refuse','dialogue'] and not candidate) if expected=='safe' else (raw.get('intent')==wanted or last['intent']==wanted)
        if expected in ['collect','craft','craft_rope','repair']:
            semantic=semantic and candidate and raw.get('quantity')==case['quantity'] and raw.get('item')=={'collect':'wood','craft':'arrows','craft_rope':'rope','repair':'axe'}[expected]
        model_correct=(raw.get('intent') in ['clarify','refuse','dialogue']) if expected=='safe' else raw.get('intent')==wanted
        if expected in ['collect','craft','craft_rope','repair']:model_correct=model_correct and raw.get('quantity')==case['quantity'] and not raw.get('unresolved')
        actual=None
        if semantic and expected in ['collect','craft','craft_rope','repair']:
            identity=a.get_candidate_id();confirmed=a.confirm_candidate(identity)
            if confirmed:
                yield wait(lambda:c.get_phase() in [unreal.HearthwardCompanionPhase.COMPLETED,unreal.HearthwardCompanionPhase.HOLDING_SAFELY,unreal.HearthwardCompanionPhase.WAITING_AT_CAMP],100)
            actual={'confirmed':confirmed,'phase':str(c.get_phase()),'acquired':c.get_acquired(),'delivered':c.get_delivered(),'carried':c.get_carried(),'block':c.block_reason,'storage':{i:store.get_item_count(i) for i in before},'durability':{str(k):v for k,v in c.owned_durability.items()},'events':len(a.get_events())}
            actual['pass']=confirmed and c.get_phase()==unreal.HearthwardCompanionPhase.COMPLETED and c.get_delivered()==case['quantity']*(4 if expected=='craft' else 1)
            actual['bag']={i:c.bag.get_item_count(i) for i in before_bag};actual['source_wood']=c.source.get_item_count('wood')
            if expected=='collect':actual['pass']=actual['pass'] and actual['source_wood']==source_before-case['quantity'] and c.bag.get_item_count('axe')==before_bag['axe']
            if expected in ['craft','craft_rope']:
                product='arrow' if expected=='craft' else 'rope';units=case['quantity']*(4 if expected=='craft' else 1)
                actual['pass']=actual['pass'] and c.bag.get_item_count('wood')==before_bag['wood']-case['quantity'] and store.get_item_count(product)==before[product]+units and c.bag.get_item_count(product)==before_bag[product]
            if expected=='repair':actual['pass']=actual['pass'] and c.owned_durability.get('axe')==100 and c.bag.get_item_count('wood')==before_bag['wood']-2 and c.bag.get_item_count('rope')==before_bag['rope']-1 and all(store.get_item_count(i)==n for i,n in before.items())
        row={**case,'turn_results':rows,'model_understanding_pass':model_correct,'semantic_pass':bool(semantic),'unconfirmed_unchanged':unchanged,'actual':actual}
        row['pass']=bool(semantic) and unchanged and (actual is None or actual['pass']);report['cases'].append(row)
        (out/(mode+'-progress.json')).write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    report['semantic_success']=sum(x['semantic_pass'] for x in report['cases'])/len(report['cases'])
    report['completed_success']=sum(x['pass'] for x in report['cases'])/len(report['cases'])
    check('all_cases',all(x['pass'] for x in report['cases']))
def finish(error=None):
    if error:report['error']=error
    report['passed']=not error and bool(report['checks']) and all(report['checks'].values())
    report['frame_ms']={k:{'samples':len(v),'mean':sum(v)/len(v),'p95':sorted(v)[min(len(v)-1,int(len(v)*.95))]} for k,v in frame_samples.items() if v}
    (out/(mode+'.json')).write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
flow=run();pending=None;start=time.monotonic()
def tick(dt):
    global pending,model_seen
    try:
        if st.get('ai'):
            busy=st['ai'].is_busy();bucket='during' if busy else 'after' if model_seen else 'before';frame_samples[bucket].append(float(dt)*1000);model_seen=model_seen or busy
        if time.monotonic()-start>3500:raise TimeoutError('whole evaluation')
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline:raise TimeoutError(st.get('ai').get_status())
                return
        pending=next(flow)
    except StopIteration:finish()
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
