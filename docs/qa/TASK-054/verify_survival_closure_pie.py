"""TASK-054 real UI entry regression; explicit graybox life/inventory fixture, no asset edits."""
import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / "Saved/Task054/pie"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "PIE public UI actions and real saved medicine; explicit fixture, no physical keyboard or model claim"}
st = {"phase": "start", "deadline": time.monotonic() + 150}


def check(name, value):
    report["checks"][name] = bool(value)
    if not value:
        raise AssertionError(name)


def phase(name):
    st.update(phase=name, since=time.monotonic())


def active():
    return st["clock"].get_snapshot().active_play_seconds


def life():
    return st["survival"].state.life


def shot(name):
    unreal.SystemLibrary.execute_console_command(st["world"], f'Shot SHOWUI filename="{(out / name).as_posix()}.png" -nosuffix', st["pc"])


def finish(error=None):
    if error:
        report["error"] = error
    report["ok"] = not error and all(report["checks"].values())
    (out / "results.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():
        unreal.GameplayStatics.set_game_paused(st["world"], False)
        levels.editor_request_end_play()


def tick(dt):
    try:
        now = time.monotonic()
        if now > st["deadline"]:
            raise TimeoutError(st["phase"])
        p = st["phase"]
        if p == "start":
            levels.editor_request_begin_play()
            phase("possess")
        elif p == "possess":
            if not levels.is_in_play_in_editor():
                return
            world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
            pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
            if not pawn:
                return
            pc = unreal.GameplayStatics.get_player_controller(world, 0)
            unreal.GameplayStatics.set_game_paused(world, False)
            unreal.SystemLibrary.execute_console_command(world, "Hearthward.Companion.CreateTest", pc)
            brother = unreal.GameplayStatics.get_actor_of_class(world, unreal.HearthwardCompanionFixture)
            check("brother fixture exists", brother is not None)
            brother.set_actor_location(pawn.get_actor_location() + unreal.Vector(150, 0, 0), False, True)
            st.update(world=world, pawn=pawn, pc=pc, brother=brother,
                      screen=pc.get_hud().get_editor_property("screen"),
                      g=pawn.get_component_by_class(unreal.HearthwardGameplayComponent),
                      survival=pawn.get_component_by_class(unreal.HearthwardSurvivalComponent),
                      bag=pawn.get_component_by_class(unreal.HearthwardInventoryComponent),
                      save=next(x for x in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if x.get_outer() == world),
                      clock=next(x for x in unreal.ObjectIterator(unreal.HearthwardWorldClockSubsystem) if x.get_outer() == world))
            phase("ground")
        elif p == "ground":
            if now - st["since"] < 1:
                return
            st["g"].set_editor_property("enabled", True)
            check("prototype save participants bind", st["save"].enable_prototype())
            check("isolated fixture progress starts", st["save"].start_new_progress())
            st["screen"].open_page("hud")
            st["g"].set_editor_property("health", 40)
            st["bag"].try_add("medicine", 1)
            check("real medicine action starts", st["g"].use_item("medicine"))
            check("pending medicine can be saved", st["save"].save_point(True))
            st["point"] = st["save"].get_points()[-1].save_id
            st["g"].apply_damage(10000)
            st["screen"].open_page("pause")
            check("damage downs the player", life() == unreal.HearthwardLife.DOWNED)
            st["frozen"] = active()
            check("give-up opens confirmation", st["screen"].execute_action("giveUp"))
            check("opening confirmation does not kill", life() == unreal.HearthwardLife.DOWNED)
            check("confirmation stays on the paused page", str(st["screen"].get_page()) == "pause" and unreal.GameplayStatics.is_game_paused(st["world"]))
            phase("cancel_confirmation")
        elif p == "cancel_confirmation":
            if now - st["since"] < .7:
                return
            shot("give-up-confirmation")
            phase("return_confirmation")
        elif p == "return_confirmation":
            if now - st["since"] < .4:
                return
            check("pause freezes the active survival clock", st["frozen"] == active())
            check("return button cancels confirmation", st["screen"].execute_action("cancel"))
            check("return preserves downed life and the frozen active clock", life() == unreal.HearthwardLife.DOWNED and st["frozen"] == active())
            st["screen"].execute_action("confirm")
            check("a cancelled confirmation cannot give up later", life() == unreal.HearthwardLife.DOWNED)
            check("give-up can be requested again", st["screen"].execute_action("giveUp"))
            check("legal node restores during a pending give-up confirmation", st["save"].load_point(st["point"]))
            check("old give-up confirmation is rejected after restore", not st["screen"].execute_action("confirm") and life() == unreal.HearthwardLife.ALIVE)
            st["g"].apply_damage(10000)
            st["screen"].open_page("pause")
            check("give-up starts with a fresh confirmation", st["screen"].execute_action("giveUp"))
            check("explicit confirmation commits give-up", st["screen"].execute_action("confirm"))
            check("confirmed give-up truly kills", life() == unreal.HearthwardLife.DEAD)
            phase("failed")
        elif p == "failed":
            if now - st["since"] < .5:
                return
            check("give-up enters the paused failure save page", str(st["screen"].get_page()) == "save" and unreal.GameplayStatics.is_game_paused(st["world"]))
            st["screen"].execute_action("back")
            check("closing the failure page cannot resume the world", str(st["screen"].get_page()) == "save" and unreal.GameplayStatics.is_game_paused(st["world"]))
            shot("failure-load")
            phase("restore")
        elif p == "restore":
            if now - st["since"] < .4:
                return
            check("failure can restore a legal node", st["save"].load_point(st["point"]))
            check("saved pending medicine is restored", life() == unreal.HearthwardLife.ALIVE and str(st["survival"].state.get_editor_property("medicine")) == "medicine" and st["bag"].get_item_count("medicine") == 1 and st["bag"].get_item_count("medicine_half") == 0)
            st["screen"].open_page("pause")
            check("alive player cannot request give-up", not st["screen"].execute_action("giveUp"))
            st["screen"].open_page("hud")
            st["started"] = active()
            phase("restored_medicine")
        elif p == "restored_medicine":
            if active() - st["started"] < 3.2:
                return
            check("restored dose completes once", st["bag"].get_item_count("medicine") == 0 and st["g"].health >= 70)
            st["g"].set_editor_property("health", st["g"].max_health())
            check("full-health medicine is refused", not st["g"].use_item("medicine"))
            check("full-health feedback explains the refusal", "生命已满" in st["g"].get_editor_property("feedback"))
            finish()
    except Exception:
        finish(traceback.format_exc())


handle = unreal.register_slate_post_tick_callback(tick)
