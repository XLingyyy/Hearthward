"""Set up a fixture and observe; typing, sending, editing and confirming are physical UI inputs."""
import json,time,traceback,shutil
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
r=Path(unreal.Paths.project_dir());out=r/'Saved/Task025Rev2';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);started=time.monotonic();phase=0;last=0;trace=[];st={};saw_card=False
def tick(dt):
    global phase,last,saw_card
    try:
        if phase==0:levels.editor_request_begin_play();phase=1;return
        if phase==1:
            if not levels.is_in_play_in_editor() or time.monotonic()-started<3:return
            w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();pc=unreal.GameplayStatics.get_player_controller(w,0)
            if not pc or not pc.get_hud() or not pc.get_hud().screen:return
            ui=pc.get_hud().screen;ui.execute_action('new')
            p=unreal.GameplayStatics.get_player_pawn(w,0);c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
            c.set_actor_location(unreal.Vector(0,400,100),False,True);c.camp.set_actor_location(unreal.Vector(0,400,100),False,True);c.source.get_owner().set_actor_location(unreal.Vector(600,400,100),False,True)
            p.set_actor_location(unreal.Vector(-200,400,100),False,True);ui.execute_action('page:dialogue')
            a=next(x for x in unreal.ObjectIterator(unreal.HearthwardLocalAISubsystem) if x.get_outer()==w)
            s=next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if x.get_outer()==w)
            st.update(ui=ui,ai=a,comp=c,store=s);phase=2
            (out/'physical-ready.json').write_text(json.dumps({'ready':True}),encoding='utf-8')
        if phase==2 and time.monotonic()-last>.25:
            last=time.monotonic();a=st['ai'];c=st['comp'];ui=st['ui']
            row={'seconds':round(last-started,2),'page':str(ui.get_page()),'status':a.get_status(),'line':a.get_npc_line(),'raw':a.get_last_structured_result(),'candidate':a.has_candidate(),'card':a.get_candidate_text(),'requested':c.get_requested(),'acquired':c.get_acquired(),'carried':c.get_carried(),'delivered':c.get_delivered(),'storage_wood':st['store'].get_item_count('wood')}
            if not trace or {k:v for k,v in trace[-1].items() if k!='seconds'}!={k:v for k,v in row.items() if k!='seconds'}:trace.append(row)
            (out/'physical-trace.json').write_text(json.dumps(trace,ensure_ascii=False,indent=2),encoding='utf-8')
            if a.has_candidate() and not saw_card:
                saw_card=c.get_requested()==0 and st['store'].get_item_count('wood')==0
                ui.capture_ui('task025-rev2-candidate',1280,720)
            if c.get_phase()==unreal.HearthwardCompanionPhase.COMPLETED:
                ui.capture_ui('task025-rev2-delivered',1280,720)
                report={'passed':saw_card and c.get_delivered()==3 and st['store'].get_item_count('wood')==3,'physical_input':'computer-use SendInput; no scripted submit or confirm','trace':trace}
                for name in ['candidate','delivered']:
                    shutil.copyfile(r/'Saved/Task020'/f'task025-rev2-{name}.png',r/'docs/qa/evidence/TASK-025/rev2'/f'physical-{name}.png')
                (out/'physical.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        (out/'physical.json').write_text(json.dumps({'passed':False,'error':traceback.format_exc()}),encoding='utf-8');unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
