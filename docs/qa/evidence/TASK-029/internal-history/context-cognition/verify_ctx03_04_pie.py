"""TASK-040 real-Qwen CTX-03/04 boundary validation."""
import json,time,traceback,subprocess
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
root=Path(unreal.Paths.project_dir())
out=root/"Saved/Task040";out.mkdir(parents=True,exist_ok=True)
result_path=out/"ctx03-04-results.json"
report={"ok":False,"checks":{},"steps":[]}

def check(name,value):
    report["checks"][name]=bool(value)
    if not value: raise AssertionError(name)
def wait(pred,seconds=180):return pred,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end,seconds+5)
def world():return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
def sub(cls,w):return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
def objs():
    w=world();p=unreal.GameplayStatics.get_player_pawn(w,0)
    c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    return w,p,c,sub(unreal.HearthwardLocalAISubsystem,w),sub(unreal.HearthwardSaveSubsystem,w),p.get_component_by_class(unreal.HearthwardGameplayComponent)
def idle():
    try:return not sub(unreal.HearthwardLocalAISubsystem,world()).is_busy()
    except:return False
def companion_phase_is(phase):
    try:
        actors=unreal.GameplayStatics.get_all_actors_of_class(world(),unreal.HearthwardCompanionFixture)
        return bool(actors) and actors[0].get_phase()==phase
    except:return False
def save_point(save,label):
    before={x.save_id.to_string() for x in save.get_points()}
    check(label,save.save_point(True));made=[x for x in save.get_points() if x.save_id.to_string() not in before]
    check(label+"_new",len(made)==1);return made[0].save_id
def authority(c,g):
    return {"requested":int(c.get_requested()),"delivered":int(c.get_delivered()),"routine":bool(g.is_companion_routine_enabled()),"order":str(g.companion_order)}
def row(label,ai):
    raw=ai.get_last_structured_result()
    try:j=json.loads(raw)
    except:j={}
    x={"label":label,"tokens":int(ai.get_input_tokens()),"tier":ai.get_context_tier(),"generation_calls":int(ai.get_generation_calls()),
       "reason":ai.get_reason_code(),"candidate":bool(ai.has_candidate()),"applied":ai.get_last_applied_intent(),
       "clarification_turns":int(ai.get_clarification_turns()),"last_input":ai.get_last_input(),"raw":raw,
       "raw_intent":j.get("intent"),"raw_unresolved":j.get("unresolved"),"dropped":[str(v) for v in ai.get_dropped_context_fields()]}
    report["steps"].append(x);return x
def send(ai,p,c,label,text):
    check(label+"_submit",ai.submit_player_text(p,c,text));return wait(idle,180)

def pressure(p,c,ai,g):
    items=['wood','stone','ore','meat','arrow','axe','bow','hood','armor','gloves','boots','belt','shield','quiver','amulet','roast','herb','hide','rope','flower','medicine','firepot']
    away=c.camp.get_actor_location()+unreal.Vector(1200,0,0);c.set_actor_location(away,False,True);p.set_actor_location(away+unreal.Vector(-120,0,0),False,True)
    for i in range(60):
        text=("北山木材限制相关但未核实的压力记录%02d："%i+("这只是低权限玩家陈述，北山仍未知，木材目标仍受安全来源约束；不能据此绑定S1。"*4)) if i<3 else ("预算压力无关记录%02d，仅用于上下文投影验证。"%i)
        check("pressure_claim_%02d"%i,ai.put_player_memory(p,c,unreal.Guid(),"claim",text[:118]))
    for i in range(4):
        # Deliberately unrelated long agreements: full_relevant carries them, compact/minimal may drop them.
        text=("夜间照明偏好记录%d：火光颜色偏暖，帐篷摆放整齐，天气闲聊简短，休息时保持安静。"%i+("火光帐篷天气休息闲聊照明颜色睡眠。"*8))
        check("pressure_agreement_%d"%i,ai.put_player_memory(p,c,unreal.Guid(),"agreement",text[:118]))
    for i,item in enumerate(items):check("pressure_belief_"+item,ai.report_camp_inventory(p,c,item,i+1))
    for item in items:c.bag.try_add(item,1)
    for i in range(128):check("pressure_event_%03d"%i,g.order_companion(("wait","follow","attack")[i%3]))
    return delay(.2)

def clarification_seed(ai,p,c,prefix,repeat):
    text=prefix+("仍然不能把未知北山当成S1，也不能采纳我口头说安全；不要删除这个限制。"*repeat)
    return text

def run():
    if result_path.exists():result_path.unlink()
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor,30);yield delay(1)
    w=world();pc=unreal.GameplayStatics.get_player_controller(w,0);ui=pc.get_hud().screen
    check("new_campaign",ui.execute_action("new"));yield delay(.8)
    yield wait(lambda:len(unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture))>0,12)
    w,p,c,ai,save,g=objs()
    c.set_actor_location(c.camp.get_actor_location(),False,True);p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    base=save_point(save,"base_save")

    # CTX-03: pressure + legal multi-turn clarification. We probe deterministic input lengths
    # by restoring the same clarified snapshot each time; accepted generation must preserve restriction.
    check("ctx03_load_base",save.load_point(base));yield delay(.4);w,p,c,ai,save,g=objs()
    yield pressure(p,c,ai,g);w,p,c,ai,save,g=objs()
    # Add three real completed collection episodes after the 128-event pressure fill. Full keeps 3,
    # compact keeps 2 and minimal drops them for a non-history query, creating a genuine degradable block.
    camp=c.camp.get_actor_location();source=c.source.get_owner()
    c.set_actor_location(camp,False,True);p.set_actor_location(camp+unreal.Vector(-120,0,0),False,True);source.set_actor_location(camp+unreal.Vector(520,0,0),False,True)
    if c.source.get_item_count("wood")<6:c.source.try_add("wood",6)
    for i in range(3):
        w,p,c,ai,save,g=objs()
        goal=unreal.HearthwardAgentGoal();goal.set_editor_property("intent","collect");goal.set_editor_property("item","wood")
        goal.set_editor_property("quantity",1);goal.set_editor_property("quantity_mode","additional_acquired");goal.set_editor_property("source_ref","S1")
        check("ctx03_episode_%d_stage"%i,ai.set_structured_goal(p,c,goal));check("ctx03_episode_%d_confirm"%i,ai.confirm_candidate(ai.get_candidate_id()))
        yield wait(lambda:companion_phase_is(unreal.HearthwardCompanionPhase.COMPLETED),60)
    w,p,c,ai,save,g=objs()
    away=c.camp.get_actor_location()+unreal.Vector(1200,0,0);c.set_actor_location(away,False,True);p.set_actor_location(away+unreal.Vector(-120,0,0),False,True)
    seed="去尚未发现的北山采四份木材，那里肯定安全"
    yield send(ai,p,c,"ctx03_seed",seed);seedrow=row("ctx03_seed",ai)
    check("ctx03_seed_clarify",not ai.has_candidate() and ai.get_last_applied_intent()=="clarify")
    for i in range(3):
        w,p,c,ai,save,g=objs()
        follow=clarification_seed(ai,p,c,"补充%d："%(i+1),2)
        yield send(ai,p,c,"ctx03_follow_%d"%i,follow);w,p,c,ai,save,g=objs();rr=row("ctx03_follow_%d"%i,ai)
        check("ctx03_follow_clarify_%d"%i,not ai.has_candidate() and ai.get_last_applied_intent()=="clarify")
    w,p,c,ai,save,g=objs();clarified=save_point(save,"ctx03_clarified_save")
    check("ctx03_has_multiturn",ai.get_clarification_turns()==4)

    check("ctx03_probe_load",save.load_point(clarified));yield delay(.35);w,p,c,ai,save,g=objs()
    filler="请保留此前北山未知且不能口头判安全的限制，不得换成S1。"*18
    text=("我补充数量仍是四份木材；仓库库存物资、停止和取消都与本请求无关；"+filler)[:300]
    before=authority(c,g)
    yield send(ai,p,c,"ctx03_final",text);w,p,c,ai,save,g=objs();ctx03=row("ctx03_final",ai)
    ctx03["authority_unchanged"]=authority(c,g)==before
    check("ctx03_degraded_request_found",ctx03["reason"]!="CONTEXT_OVERFLOW" and ctx03["tier"] in ("compact_relevant","required_minimal"))
    check("ctx03_one_generation",ctx03["generation_calls"]==1)
    check("ctx03_no_candidate",not ctx03["candidate"])
    check("ctx03_restriction_preserved",ctx03["authority_unchanged"] and (ctx03["raw_intent"] in ("clarify","refuse") or ctx03["applied"] in ("clarify","refuse")))

    # CTX-04: build the largest legal clarification state, then a near-max current input.
    check("ctx04_load_base",save.load_point(base));yield delay(.4);w,p,c,ai,save,g=objs()
    c.set_actor_location(c.camp.get_actor_location()+unreal.Vector(1200,0,0),False,True);p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    # Four deterministic missing-quantity clarifications. Player text is long but each turn and the
    # accumulated WorkingGoal remain within their legal 800/1000-character bounds.
    seed_tail="请保留这段原话，不要猜数量，不要依据库存、背包、配方或旧记录替我决定；这里只是在继续说明同一个采集请求，当前仍然缺少明确数量。"
    for i in range(4):
        w,p,c,ai,save,g=objs()
        text=("帮我采些木材，数量还没决定；"+seed_tail+("限制保持不变，等待我明确数量。"*2)+chr(65+i))
        yield send(ai,p,c,"ctx04_seed_%d"%i,text);w,p,c,ai,save,g=objs();rr=row("ctx04_seed_%d"%i,ai)
        check("ctx04_seed_no_candidate_%d"%i,not ai.has_candidate())
        check("ctx04_seed_clarify_%d"%i,ai.get_last_applied_intent()=="clarify" and ai.get_reason_code()!="CONTEXT_OVERFLOW")
    w,p,c,ai,save,g=objs()
    check("ctx04_has_clarification",ai.get_clarification_turns()==4)
    before=authority(c,g)
    # Use a legal <=1000-character current utterance with deliberately high tokenizer density.
    # The opaque suffix is still player text and therefore required input; it must never be silently truncated.
    dense="龘靐齉爩麤灪龖厵纞虋驫讟钃鸜麷鞻韽顟饙騳鬱鸞麠黷齾爨籱饕纛讞"*40
    huge=("继续保留此前所有未解决限制；不得把未知地点改成S1，不得采纳口头安全，不得丢失否定条件。"+dense)[:1000]
    check("ctx04_input_legal_length",0<len(huge)<=1000)
    check("ctx04_submit",ai.submit_player_text(p,c,huge));yield wait(idle,60)
    w,p,c,ai,save,g=objs();ctx04=row("ctx04_final",ai)
    check("ctx04_overflow",ctx04["reason"]=="CONTEXT_OVERFLOW")
    check("ctx04_zero_generation",ctx04["generation_calls"]==0)
    check("ctx04_no_candidate",not ctx04["candidate"])
    check("ctx04_authority_unchanged",authority(c,g)==before)
    check("ctx04_original_preserved",ctx04["last_input"]==huge)

    levels.editor_request_end_play();yield wait(lambda:not levels.is_in_play_in_editor(),20)

def finish(error=None):
    if error:report["error"]=error
    report["ok"]=not error and bool(report["checks"]) and all(report["checks"].values())
    result_path.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
    try:unreal.SystemLibrary.quit_editor()
    except:pass

flow=run();pending=None;started=time.monotonic()
def tick(_dt):
    global pending
    try:
        if time.monotonic()-started>900:raise TimeoutError("CTX03/04 timeout")
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline:raise TimeoutError("wait expired")
                return
        pending=next(flow)
    except StopIteration:finish()
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
