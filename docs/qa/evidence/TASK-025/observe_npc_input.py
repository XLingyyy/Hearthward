"""Prepare an isolated dialogue; all note entry and conversation use physical user input."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
r=Path(unreal.Paths.project_dir());out=r/'Saved/Task025/physical';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);start=time.monotonic();phase=0;st={};frames=0;last=0;trace=[]
def tick(dt):
    global phase,frames,last
    try:
        if phase==0:levels.editor_request_begin_play();phase=1;return
        if phase==1:
            if not levels.is_in_play_in_editor() or time.monotonic()-start<3:return
            w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();pc=unreal.GameplayStatics.get_player_controller(w,0)
            if not pc or not pc.get_hud() or not pc.get_hud().screen:return
            ui=pc.get_hud().screen;ui.execute_action('new')
            p=unreal.GameplayStatics.get_player_pawn(w,0);c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
            p.set_actor_location(c.get_actor_location()+unreal.Vector(-200,0,0),False,True)
            ui.execute_action('page:dialogue');ai=next(x for x in unreal.ObjectIterator(unreal.HearthwardLocalAISubsystem) if x.get_outer()==w)
            st.update(ui=ui,ai=ai);phase=2
            (out/'ready.json').write_text(json.dumps({'ready':True}),encoding='utf-8')
        if phase==2 and time.monotonic()-last>.5:
            last=time.monotonic();ui=st['ui'];ai=st['ai']
            row={'seconds':round(last-start,2),'page':str(ui.get_page()),'records':[{'text':m.text,'kind':str(m.kind),'revoked':m.revoked} for m in ai.get_player_memories()],'line':ai.get_npc_line(),'status':ai.get_status()}
            if not trace or {k:v for k,v in trace[-1].items() if k!='seconds'}!={k:v for k,v in row.items() if k!='seconds'}:trace.append(row)
            (out/'input-trace.json').write_text(json.dumps(trace,ensure_ascii=False,indent=2),encoding='utf-8')
            if (r/'.agent-local/record025').exists():
                ui.capture_ui(f'task025-physical-{frames:04d}',960,540);frames+=1
    except Exception:
        (out/'error.txt').write_text(traceback.format_exc(),encoding='utf-8');unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
