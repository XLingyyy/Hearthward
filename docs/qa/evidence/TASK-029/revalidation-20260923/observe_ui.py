"""Explicit isolated fixture; conversation/confirmation are driven by physical input."""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root = Path(unreal.Paths.project_dir())
out = root / 'Saved/NPCValidation/ui'
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
started = time.monotonic()
phase = 0
last = 0
trace = []
state = {}

def tick(_dt):
    global phase, last
    try:
        if phase == 0:
            levels.editor_request_begin_play()
            phase = 1
            return
        if phase == 1:
            if not levels.is_in_play_in_editor() or time.monotonic() - started < 3:
                return
            w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
            pc = unreal.GameplayStatics.get_player_controller(w, 0)
            if not pc or not pc.get_hud() or not pc.get_hud().screen:
                return
            unreal.SystemLibrary.execute_console_command(w, 'Hearthward.Companion.CreateTest', pc)
            p = unreal.GameplayStatics.get_player_pawn(w, 0)
            c = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.HearthwardCompanionFixture)[0]
            gameplay = p.get_component_by_class(unreal.HearthwardGameplayComponent)
            gameplay.enable_adventure()
            save = next(x for x in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if x.get_outer() == w)
            assert save.enable_prototype() and save.start_new_progress()
            gameplay.order_companion('wait')
            p.set_actor_location(c.get_actor_location() + unreal.Vector(-150, 0, 0), False, True)
            ui = pc.get_hud().screen
            ui.open_page('hud')
            ai = next(x for x in unreal.ObjectIterator(unreal.HearthwardLocalAISubsystem) if x.get_outer() == w)
            store = next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if x.get_outer() == w)
            state.update(ui=ui, ai=ai, c=c, store=store)
            (out / 'ready.json').write_text(json.dumps({'ready': True}), encoding='utf-8')
            phase = 2
        if phase == 2 and time.monotonic() - last > .5:
            last = time.monotonic()
            ui, ai, c, store = (state[k] for k in ('ui', 'ai', 'c', 'store'))
            row = dict(page=str(ui.get_page()), input=ai.get_last_input(), line=ai.get_npc_line(),
                       status=ai.get_status(), raw=ai.get_last_structured_result(), candidate=ai.has_candidate(),
                       requested=c.get_requested(), acquired=c.get_acquired(), delivered=c.get_delivered(),
                       carried=c.get_carried(), source=c.source.get_item_count('wood'), storage=store.get_item_count('wood'))
            if not trace or {k: v for k, v in trace[-1].items() if k != 'seconds'} != row:
                row['seconds'] = round(last - started, 2)
                trace.append(row)
                ui.capture_ui('npc-revalidation-%03d' % len(trace), 1280, 720)
                (out / 'trace.json').write_text(json.dumps(trace, ensure_ascii=False, indent=2), encoding='utf-8')
        if (out / 'stop').exists():
            unreal.unregister_slate_post_tick_callback(handle)
            levels.editor_request_end_play()
            unreal.SystemLibrary.quit_editor()
    except Exception:
        (out / 'error.txt').write_text(traceback.format_exc(), encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(handle)

handle = unreal.register_slate_post_tick_callback(tick)
