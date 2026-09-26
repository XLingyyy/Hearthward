"""Isolated graybox PIE fixture. Uses the real Enhanced Input actions; edits no assets."""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / "Saved/Task045/pie"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "PIE native actors and Enhanced Input injection; fixture enemy placements; no physical keyboard claim"}
st = {"phase": "start", "deadline": time.monotonic() + 240}

def check(name, value):
    report["checks"][name] = bool(value)
    if not value:
        raise AssertionError(name)

def phase(name):
    st.update(phase=name, since=time.monotonic())

def active():
    return st["clock"].get_snapshot().active_play_seconds

def shot(name):
    unreal.SystemLibrary.execute_console_command(st["world"], f'Shot SHOWUI filename="{(out/name).as_posix()}.png" -nosuffix', st["pc"])

def inject(name, value=1):
    st["input"].inject_input_vector_for_action(st["actions"][name], unreal.Vector(value, 0, 0), [], [])

def finish(error=None):
    if error:
        report["error"] = error
    report["ok"] = not error and all(report["checks"].values())
    (out / "results.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():
        unreal.GameplayStatics.set_game_paused(st["world"], False)
        levels.editor_request_end_play()

def targets():
    return sorted((a.get_component_by_class(unreal.HearthwardCombatTargetComponent) for a in unreal.GameplayStatics.get_all_actors_of_class(st["world"], unreal.Actor) if a.get_component_by_class(unreal.HearthwardCombatTargetComponent)), key=lambda t: str(t.id))

def stage_target(target):
    target.get_owner().set_actor_location(st["pawn"].get_actor_location() + unreal.Vector(110, 0, 0), False, True)
    target.get_owner().set_actor_rotation(unreal.Rotator(0, 0, 0), True)
    st["target"] = target

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
            w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
            pawn = unreal.GameplayStatics.get_player_pawn(w, 0)
            if not pawn:
                return
            pc = unreal.GameplayStatics.get_player_controller(w, 0)
            st.update(world=w, pawn=pawn, pc=pc,
                      g=pawn.get_component_by_class(unreal.HearthwardGameplayComponent),
                      c=pawn.get_component_by_class(unreal.HearthwardCombatComponent),
                      bag=pawn.get_component_by_class(unreal.HearthwardInventoryComponent),
                      screen=pc.get_hud().get_editor_property("screen"),
                      save=next(x for x in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if x.get_outer() == w),
                      clock=next(x for x in unreal.ObjectIterator(unreal.HearthwardWorldClockSubsystem) if x.get_outer() == w),
                      input=next(x for x in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if isinstance(x.get_outer(), unreal.LocalPlayer)),
                      actions={x.get_name(): x for x in unreal.ObjectIterator(unreal.InputAction) if x.get_outer() == pawn})
            unreal.GameplayStatics.set_game_paused(w, False)
            unreal.SystemLibrary.execute_console_command(w, "Hearthward.Companion.CreateTest", pc)
            brother = unreal.GameplayStatics.get_actor_of_class(w, unreal.HearthwardCompanionFixture)
            check("brother participant exists", brother is not None)
            brother.set_actor_location(pawn.get_actor_location() + unreal.Vector(-150, 0, 0), False, True)
            phase("ground")
        elif p == "ground":
            if now - st["since"] < 1:
                return
            st["g"].enable_adventure()
            captured = st["save"].enable_prototype()
            report["capture_status"] = st["save"].get_status()
            report["player_position"] = str(st["pawn"].get_actor_location())
            check("save participant capture", captured)
            check("new progress", st["save"].start_new_progress())
            st["screen"].open_page("hud")
            all_targets = targets()
            check("encounters loaded", len(all_targets) >= 2)
            for i, t in enumerate(all_targets):
                t.get_owner().set_actor_location(st["pawn"].get_actor_location() + unreal.Vector(20000 + i * 1000, 0, 0), False, True)
            st["all"] = all_targets
            if st["bag"].get_item_count("axe") == 0:
                st["bag"].try_add("axe", 1)
            if str(st["g"].equipment.get("weapon", "None")) != "axe":
                check("equipment switch begins", st["g"].equip("axe"))
            phase("equipped")
        elif p == "equipped":
            if st["c"].busy() or now - st["since"] < .7:
                return
            stage_target(st["all"][0])
            st["g"].set_editor_property("stamina", 100)
            st["g"].set_editor_property("health", 100)
            st["target_hp"] = st["target"].health
            inject("CombatExecute")
            st["f_request"] = active()
            phase("f_started")
        elif p == "f_started":
            if now - st["since"] < .3:
                return
            check("F enters execution", st["c"].executing())
            check("enemy cannot retaliate", not st["target"].can_act())
            check("animation is execution", st["pawn"].mesh.get_anim_instance().get_editor_property("motion_state") == "Execution")
            clip = unreal.load_asset("/Game/Characters/Hero/AnimationV2/A_Hero_Attack")
            check("reused clip has no sound or other notify events", len(unreal.AnimationLibrary.get_animation_notify_events(clip)) == 0)
            check("no early defeat", st["target"].health == st["target_hp"])
            check("no stamina charge", st["g"].stamina == 100)
            shot("execution")
            phase("before_pause")
        elif p == "before_pause":
            if now - st["since"] < .3:
                return
            st["screen"].open_page("pause")
            st["frozen"] = (active(), st["c"].elapsed, st["target"].health)
            phase("paused")
        elif p == "paused":
            if now - st["since"] < 1:
                return
            check("pause freezes execution and A", st["frozen"] == (active(), st["c"].elapsed, st["target"].health))
            st["screen"].open_page("hud")
            phase("f_finish")
        elif p == "f_finish":
            if st["c"].busy():
                check("enemy locked until completion", not st["target"].can_act())
                return
            check("F completion defeats enemy", st["target"].health == 0)
            check("F duration at least three active seconds", active() - st["f_request"] >= 3)
            check("victim never counterattacked", st["g"].health == 100)
            report["f_active_seconds"] = active() - st["f_request"]
            st["dead_id"] = str(st["target"].id)
            shot("corpse")
            st["target"].get_owner().set_actor_location(st["pawn"].get_actor_location() + unreal.Vector(-400, 400, -50), False, True)
            stage_target(st["all"][1])
            inject("CombatContextR")
            st["r_request"] = active()
            phase("r_started")
        elif p == "r_started":
            if now - st["since"] < .3:
                return
            check("R uses identical execution", st["c"].executing() and st["c"].duration == 3)
            check("R also locks target", not st["target"].can_act())
            st["g"].apply_damage(1)
            check("positive external damage cancels", not st["c"].executing() and st["target"].health > 0)
            check("cancel releases target", st["target"].can_act())
            phase("r_retry")
        elif p == "r_retry":
            if st["c"].busy():
                return
            inject("CombatContextR")
            st["r_request"] = active()
            phase("r_finish")
        elif p == "r_finish":
            if active() - st["r_request"] < 3.3:
                return
            check("R completion also defeats", st["target"].health == 0)
            check("R no stamina charge", st["g"].stamina == 100)
            inject("CombatSense")
            phase("sense")
        elif p == "sense":
            if now - st["since"] < .3:
                return
            check("V starts sense", st["c"].state.get_editor_property("sense_remaining") > 0)
            check("safe node saves combat", st["save"].save_point(True))
            st["point"] = st["save"].get_points()[-1].save_id
            phase("load")
        elif p == "load":
            if now - st["since"] < .5:
                return
            check("load combat node", st["save"].load_point(st["point"]))
            check("corpse remains cleared after load", any(str(t.id) == st["dead_id"] and t.health == 0 for t in targets()))
            check("sense cooldown survives load", st["c"].state.get_editor_property("sense_cooldown") > 0)
            shot("restored")
            phase("finish")
        elif p == "finish" and now - st["since"] > 1:
            finish()
    except Exception:
        finish(traceback.format_exc())

handle = unreal.register_slate_post_tick_callback(tick)
