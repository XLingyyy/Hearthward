"""PIE: a fed goat produces milk, and player/companion use the Nature receipt."""
import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_saved_dir()) / "Task050/penproducts"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "PIE, real capture/feeding/collection, flat floor and explicit supplies"}
st = {}


def check(name, value):
    report["checks"][name] = bool(value)
    if not value:
        raise AssertionError(name)


def wait(predicate, seconds=25):
    return predicate, time.monotonic() + seconds


def delay(seconds):
    end = time.monotonic() + seconds
    return wait(lambda: time.monotonic() >= end, seconds + 5)


def guid(text):
    result = unreal.GuidLibrary.parse_string_to_guid(text)
    return result[0] if isinstance(result, tuple) else result


def key(value):
    return tuple(value.get_editor_property(part) for part in ("a", "b", "c", "d"))


def subsystem(cls, world):
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer() == world)


def state():
    return json.loads(st["nature"].describe())


def position(row):
    p = row["position"]
    return unreal.Vector(p["x"], p["y"], p["z"])


def go(place):
    st["ui"].open_page("hud")
    st["player"].set_actor_location(place + unreal.Vector(-150, 0, 120), False, True)
    st["pc"].set_control_rotation(unreal.Rotator(pitch=-15, yaw=0))


def action(command, target, option=""):
    st["ui"].open_nature(guid(target) if target else unreal.Guid())
    check("start " + command, st["ui"].execute_action("nature." + command + ":" + option))
    yield wait(lambda: not st["nature"].busy(), 10)
    check("settle " + command, str(st["nature"].feedback) == "操作完成")


def grant_shared(item, amount):
    while amount:
        part = min(amount, 8)
        check("supply " + item, st["bag"].try_add(item, part) == unreal.HearthwardInventoryResult.SUCCESS)
        receipt = st["access"].transfer(st["bag"], True, item, part,
                                         unreal.GuidLibrary.new_guid(), st["store"].get_timeline_epoch())
        check("deposit supply " + item, receipt.moved_count == part)
        amount -= part


def save_point(label):
    old = {key(point.save_id) for point in st["save"].get_points()}
    check(label, st["save"].save_point(True))
    created = [point.save_id for point in st["save"].get_points() if key(point.save_id) not in old]
    check(label + " identity", len(created) == 1)
    return created[0]


def finish(error=None):
    if error:
        report["error"] = error
    report["ok"] = not error and bool(report["checks"]) and all(report["checks"].values())
    try:
        report["nature"] = state()
        report["phase"] = str(st["brother"].get_phase())
        report["block_reason"] = str(st["brother"].block_reason)
        report["brother_position"] = str(st["brother"].get_actor_location())
        report["camp_position"] = str(st["brother"].camp.get_actor_location())
        report["events"] = [{"kind": str(e.kind), "item": str(e.item), "reason": str(e.reason)} for e in st["ai"].get_events()]
    except Exception:
        report["observer_error"] = traceback.format_exc()
    (out / "results.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()


def run():
    editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    floor = editor.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -100))
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    floor.set_actor_scale3d(unreal.Vector(1600, 1600, 1))
    floor.static_mesh_component.set_collision_profile_name("BlockAll")
    floor.tags = ["Hearthward.NatureGround"]
    levels.editor_request_begin_play()
    yield wait(levels.is_in_play_in_editor)
    yield delay(1)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    player = unreal.GameplayStatics.get_player_pawn(world, 0)
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    st.update(world=world, player=player, pc=pc, ui=pc.get_hud().get_editor_property("screen"),
              bag=player.get_component_by_class(unreal.HearthwardInventoryComponent),
              game=player.get_component_by_class(unreal.HearthwardGameplayComponent))
    for name, cls in (("nature", unreal.HearthwardNatureSubsystem), ("camp", unreal.HearthwardCampSubsystem),
                      ("store", unreal.HearthwardStorageSubsystem), ("clock", unreal.HearthwardWorldClockSubsystem),
                      ("save", unreal.HearthwardSaveSubsystem), ("ai", unreal.HearthwardLocalAISubsystem)):
        st[name] = subsystem(cls, world)
    unreal.GameplayStatics.set_game_paused(world, False)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.Companion.CreateTest", pc)
    yield delay(.5)
    st["brother"] = unreal.GameplayStatics.get_actor_of_class(world, unreal.HearthwardCompanionFixture)
    st["game"].enable_adventure()
    st["game"].grant_initial_equipment()
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if actor.get_component_by_class(unreal.HearthwardCombatTargetComponent) and not isinstance(actor, unreal.HearthwardNatureActor):
            actor.set_actor_location(unreal.Vector(150000, 150000, 500), False, True)
    check("prototype saves", st["save"].enable_prototype())
    check("new progress", st["save"].start_new_progress())
    go(unreal.Vector(-200, -200, 0))
    yield delay(1)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.Storage.CreateTestAccess", pc)
    st["access"] = next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageAccessComponent) if x.get_world() == world)
    for item, amount in (("wood", 240), ("stone", 90), ("rope", 20)):
        grant_shared(item, amount)
    builder = player.get_component_by_class(unreal.HearthwardBuildingComponent)
    check("workbench choice", builder.select_building("workbench"))
    yield delay(.4)
    check("workbench built", builder.confirm_placement())
    yield wait(lambda: not builder.is_building(), 10)
    check("rescue prerequisite", st["camp"].record_rescue("task050_product_fixture"))
    check("camp tier two", st["camp"].upgrade_camp(st["store"].get_timeline_epoch()))
    go(unreal.Vector(-2300, -2200, 0))
    yield delay(1)
    yield from action("build_pen", "", "goat")
    pen = state()["pens"][0]
    check("goat pen begins empty", pen["products"] == 0)
    check("capture feed", st["bag"].try_add("feed", 12) == unreal.HearthwardInventoryResult.SUCCESS)
    check("capture rope", st["bag"].try_add("rope", 2) == unreal.HearthwardInventoryResult.SUCCESS)
    goat = next(a for a in state()["animals"] if a["domestic"] and a["definition"] == "goat")
    actor = next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HearthwardNatureActor)
                 if str(a.get_component_by_class(unreal.HearthwardCombatTargetComponent).get_editor_property("id")) == goat["id"])
    actor.set_actor_tick_enabled(False)
    actor.set_actor_location(position(pen) + unreal.Vector(-650, 0, 100), False, True)
    go(actor.get_actor_location())
    yield delay(1)
    yield from action("capture", goat["id"])
    actor.set_actor_tick_enabled(True)
    go(position(pen) + unreal.Vector(-350, 0, 0))
    yield wait(lambda: next(a for a in state()["animals"] if a["id"] == goat["id"])["pen"] == pen["id"], 15)
    go(position(pen))
    yield delay(1)
    yield from action("deposit_feed", pen["id"])
    check("ten feed paid into pen", state()["pens"][0]["feed"] <= 10)
    check("survival food", st["bag"].try_add("wild_food", 30) == unreal.HearthwardInventoryResult.SUCCESS)
    for i in range(3):
        check("first day W " + str(i), st["clock"].advance_calendar(480) > 479)
        st["game"].use_item("wild_food")
    check("one fed goat yields milk", state()["pens"][0]["products"] == 1)
    player_before = st["bag"].get_item_count("milk")
    yield from action("collect_product", pen["id"])
    check("player receives real milk", st["bag"].get_item_count("milk") == player_before + 1
          and state()["pens"][0]["products"] == 0)
    for i in range(3):
        check("second day W " + str(i), st["clock"].advance_calendar(480) > 479)
        st["game"].use_item("wild_food")
    check("next paid day yields one more milk", state()["pens"][0]["products"] == 1)
    st["ui"].open_page("hud")
    saved = save_point("save pending milk")
    check("load pending milk", st["save"].load_point(saved))
    check("load retains exactly one milk", state()["pens"][0]["products"] == 1)
    brother = st["brother"]
    brother.set_actor_location(position(pen) + unreal.Vector(-180, 0, 100), False, True)
    brother.camp.set_actor_location(brother.get_actor_location(), False, True)
    go(position(pen))
    yield delay(.4)
    stored_before = st["store"].get_item_count("milk")
    st["ui"].open_nature(guid(pen["id"]))
    check("brother product card", st["ui"].execute_action("nature.brother_collect_product:1"))
    check("brother product confirmed", st["ai"].confirm_candidate(st["ai"].get_candidate_id()))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 35)
    check("brother milk physically transferred once", state()["pens"][0]["products"] == 0
          and st["store"].get_item_count("milk") == stored_before + 1
          and brother.bag.get_item_count("milk") == 0)
    check("brother receipt counts", brother.get_acquired() == 1 and brother.get_delivered() == 1)
    st["ui"].open_page("hud")
    saved = save_point("save delivered milk")
    check("reload delivered milk", st["save"].load_point(saved))
    check("reload cannot duplicate product", state()["pens"][0]["products"] == 0
          and st["store"].get_item_count("milk") == stored_before + 1)
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
                                       + " block=" + str(st.get("brother").block_reason))
                return
        pending = next(flow)
    except StopIteration:
        pass
    except Exception:
        finish(traceback.format_exc())


handle = unreal.register_slate_post_tick_callback(tick)
