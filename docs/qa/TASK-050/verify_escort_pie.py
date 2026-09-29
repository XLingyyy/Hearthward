"""Focused PIE: the brother escorts a contacted civilian into a real camp."""
import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_saved_dir()) / "Task050/escort"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "natural-map PIE contact, companion escort, camp arrival, save/load"}
st = {}


def check(name, value):
    report["checks"][name] = bool(value)
    if not value:
        raise AssertionError(name)


def wait(predicate, seconds=45):
    return predicate, time.monotonic() + seconds


def delay(seconds):
    end = time.monotonic() + seconds
    return wait(lambda: time.monotonic() >= end, seconds + 5)


def subsystem(cls, world):
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer() == world)


def campaign_state():
    return json.loads(st["campaign"].describe())


def camp_state():
    return json.loads(st["camp"].describe())


def person_state():
    return next(x for x in campaign_state()["people"] if x["id"] == "rescued_01")


def vec(row):
    return unreal.Vector(row["x"], row["y"], row["z"])


def guid_key(value):
    return tuple(value.get_editor_property(part) for part in ("a", "b", "c", "d"))


def person_actor():
    return next((a for a in unreal.GameplayStatics.get_all_actors_of_class(st["world"], unreal.HearthwardCampaignActor)
                 if str(a.identity) == "rescued_01"), None)


def go_to(location):
    row = next(x for x in st["data"]["campaign"]["locations"] if x["id"] == location)
    movement = st["player"].get_movement_component()
    movement.disable_movement()
    st["player"].set_actor_location(unreal.Vector(row["xy"][0] * 100, row["xy"][1] * 100, 60000), False, True)
    yield wait(lambda: location in campaign_state()["positions"], 80)
    yield delay(3)
    st["player"].set_actor_location(vec(campaign_state()["positions"][location]) + unreal.Vector(0, 0, 60), False, True)
    movement.set_movement_mode(unreal.MovementMode.MOVE_WALKING)
    yield delay(1)


def finish(error=None):
    if error:
        report["error"] = error
    report["ok"] = not error and bool(report["checks"]) and all(report["checks"].values())
    try:
        report["person"] = person_state()
        report["rescued"] = camp_state()["rescued"]
        report["phase"] = str(st["brother"].get_phase())
        report["block_reason"] = str(st["brother"].block_reason)
        report["action"] = st["brother"].get_execution_action()
        report["player_at"] = str(st["player"].get_actor_location())
        report["brother_at"] = str(st["brother"].get_actor_location())
        report["person_at"] = str(person_actor().get_actor_location()) if person_actor() else None
        report["campaign_feedback"] = str(st["campaign"].feedback)
        report["agent_feedback"] = st["ai"].get_candidate_text()
        report["save_status"] = st["save"].get_status()
    except Exception:
        report["observer_error"] = traceback.format_exc()
    (out / "results.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()


def run():
    editor_world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    editor_world.get_world_settings().set_editor_property(
        "default_game_mode", unreal.load_class(None, "/Script/Hearthward.HearthwardGameMode"))
    levels.editor_request_begin_play()
    yield wait(levels.is_in_play_in_editor)
    yield delay(8)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    player = unreal.GameplayStatics.get_player_pawn(world, 0)
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    ui = pc.get_hud().get_editor_property("screen")
    st.update(world=world, player=player, ui=ui,
              game=player.get_component_by_class(unreal.HearthwardGameplayComponent),
              campaign=subsystem(unreal.HearthwardCampaignSubsystem, world),
              camp=subsystem(unreal.HearthwardCampSubsystem, world),
              save=subsystem(unreal.HearthwardSaveSubsystem, world),
              ai=subsystem(unreal.HearthwardLocalAISubsystem, world))
    st["data"] = json.loads((Path(unreal.Paths.project_dir()) / "Resources/Data/gameplay.json").read_text(encoding="utf-8"))
    check("normal new game", ui.execute_action("new"))
    yield wait(lambda: not st["campaign"].busy(), 120)
    yield delay(3)
    check("prologue relic", st["campaign"].interact())
    check("prologue brother order", st["game"].order_companion("follow"))
    brother = unreal.GameplayStatics.get_actor_of_class(world, unreal.HearthwardCompanionFixture)
    st["brother"] = brother
    exit_at = vec(campaign_state()["positions"]["prologue_exit"])
    player.set_actor_location(exit_at + unreal.Vector(-120, 0, 100), False, True)
    brother.set_actor_location(exit_at + unreal.Vector(-160, 0, 100), False, True)
    yield delay(1)
    check("prologue exit", st["campaign"].interact())
    yield wait(lambda: campaign_state()["phase"] == "occupied" and not st["campaign"].busy(), 120)
    check("population initially unchanged", "rescued_01" not in camp_state()["rescued"])

    yield from go_to("slice_rescue")
    yield wait(lambda: person_actor() is not None, 15)
    check("player contacts named person", st["campaign"].interact())
    check("person contacted, no population yet", person_state()["stage"] == "following"
          and "rescued_01" not in camp_state()["rescued"])
    person = person_actor()
    camp_at = vec(campaign_state()["positions"]["camp"])
    radius = float(st["data"]["campEconomy"]["camp_tiers"][0]["radius_m"]) * 100
    yield from go_to("camp")
    check("rescuer still in loaded world", person_actor() is not None)
    person = person_actor()
    person.set_actor_location(camp_at + unreal.Vector(radius + 500, 0, 80), False, True)
    player.set_actor_location(person.get_actor_location() + unreal.Vector(-180, 0, 80), False, True)
    brother.set_actor_location(person.get_actor_location() + unreal.Vector(-130, 0, 80), False, True)
    brother.camp.set_actor_location(camp_at + unreal.Vector(0, 0, 100), False, True)
    yield delay(.8)
    if person_state()["stage"] == "following":
        check("person waits for brother assignment", st["campaign"].interact())
    check("contacted person waiting", person_state()["stage"] == "waiting")
    goal = unreal.HearthwardAgentGoal()
    for key, value in {"intent": "escort", "item": "rescued_01", "quantity": 1,
                       "quantity_mode": "one_person", "source_ref": "known_person"}.items():
        goal.set_editor_property(key, value)
    check("escort card", st["ai"].set_structured_goal(player, brother, goal))
    check("escort confirmation", st["ai"].confirm_candidate(st["ai"].get_candidate_id()))
    yield wait(lambda: person_state()["escort"] == "brother", 25)
    check("campaign assigned brother after real contact", person_state()["stage"] == "following")
    check("no premature population", "rescued_01" not in camp_state()["rescued"])
    yield wait(lambda: not player.get_movement_component().is_falling()
               and not brother.get_movement_component().is_falling(), 12)
    before_active_save = {guid_key(x.save_id) for x in st["save"].get_points()}
    saved_active = st["save"].save_point(True)
    report["active_save_status"] = st["save"].get_status()
    check("save active escort", saved_active)
    active_point = [x.save_id for x in st["save"].get_points() if guid_key(x.save_id) not in before_active_save]
    check("active escort save point", len(active_point) == 1)
    check("reload active escort", st["save"].load_point(active_point[0]))
    check("active escort preserved", person_state()["stage"] == "following"
          and person_state()["escort"] == "brother" and "rescued_01" not in camp_state()["rescued"])
    player.set_actor_location(camp_at + unreal.Vector(radius - 1000, 0, 100), False, True)
    yield wait(lambda: "rescued_01" in camp_state()["rescued"], 60)
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 15)
    check("actual civilian arrival", person_state()["stage"] == "arrived")
    check("single citizen", camp_state()["rescued"].count("rescued_01") == 1)
    check("escort receipt", brother.get_delivered() == 1)
    existing = {guid_key(x.save_id) for x in st["save"].get_points()}
    check("save completed escort", st["save"].save_point(True))
    created = [x.save_id for x in st["save"].get_points() if guid_key(x.save_id) not in existing]
    check("escort save point", len(created) == 1)
    check("reload escort", st["save"].load_point(created[0]))
    check("no duplicate rescue after reload", camp_state()["rescued"].count("rescued_01") == 1
          and person_state()["stage"] == "arrived")
    finish()


flow = run()
pending = None


def tick(_delta):
    global pending
    try:
        if pending:
            predicate, deadline = pending
            if not predicate():
                if time.monotonic() > deadline:
                    raise TimeoutError("phase=" + str(st.get("brother").get_phase())
                                       + " block=" + str(st.get("brother").block_reason)
                                       + " person=" + str(person_state()))
                return
        pending = next(flow)
    except StopIteration:
        pass
    except Exception:
        finish(traceback.format_exc())


handle = unreal.register_slate_post_tick_callback(tick)
