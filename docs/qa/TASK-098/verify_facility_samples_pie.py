import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / '.agent-local/qa/TASK-098/facility-samples-20261007-01'
assert not (out / "results.json").exists(), "Preserve earlier evidence"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "PROTOTYPE_ONLY isolated flat-ground fixture; real paid placement of approved workbench and warehouse mesh; no gameplay balance or save changes"}
st = {}


def check(name, value):
    report["checks"][name] = bool(value)
    if not value:
        raise AssertionError(name)


def wait(predicate, seconds=30):
    return predicate, time.monotonic() + seconds


def delay(seconds):
    end = time.monotonic() + seconds
    return wait(lambda: time.monotonic() >= end, seconds + 5)


def subsystem(cls, world):
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer() == world)


def finish(error=None):
    if error:
        report["error"] = error
    report["ok"] = not error and bool(report["checks"]) and all(report["checks"].values())
    try:
        report["phase"] = str(st["brother"].get_phase())
        report["reason"] = str(st["brother"].block_reason)
        report["facilities"] = json.loads(st["camp"].describe())["facilities"]
        report["camp_roast"] = st["store"].get_item_count("roast")
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
    levels.editor_request_begin_play()
    yield wait(levels.is_in_play_in_editor)
    yield delay(1)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    player = unreal.GameplayStatics.get_player_pawn(world, 0)
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    unreal.GameplayStatics.set_game_paused(world, False)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.Companion.CreateTest", pc)
    yield delay(.5)
    brother = unreal.GameplayStatics.get_actor_of_class(world, unreal.HearthwardCompanionFixture)
    player.get_component_by_class(unreal.HearthwardGameplayComponent).enable_adventure()
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if actor.get_component_by_class(unreal.HearthwardCombatTargetComponent) and not isinstance(actor, unreal.HearthwardNatureActor):
            actor.set_actor_location(unreal.Vector(150000, 150000, 500), False, True)
    save = subsystem(unreal.HearthwardSaveSubsystem, world)
    check("prototype enabled", save.enable_prototype())
    check("new progress", save.start_new_progress())
    camp = subsystem(unreal.HearthwardCampSubsystem, world)
    store = subsystem(unreal.HearthwardStorageSubsystem, world)
    ai = subsystem(unreal.HearthwardLocalAISubsystem, world)
    bag = player.get_component_by_class(unreal.HearthwardInventoryComponent)
    builder = player.get_component_by_class(unreal.HearthwardBuildingComponent)
    st.update(player=player, brother=brother, camp=camp, store=store)
    site = json.loads(camp.describe())["camps"][0]["position"]
    brother.camp.set_actor_location(unreal.Vector(site["x"] + 100, site["y"], 50), False, True)
    player.set_actor_location(unreal.Vector(site["x"] - 200, site["y"] - 200, 100), False, True)
    brother.set_actor_location(unreal.Vector(site["x"] - 100, site["y"] + 200, 100), False, True)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.Storage.CreateTestAccess", pc)
    access = next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageAccessComponent) if x.get_world() == world)
    for item, total in (("wood", 84), ("stone", 4)):
        remaining=total
        while remaining:
            count=min(remaining,4)
            check("supply "+item+str(remaining), bag.try_add(item,count)==unreal.HearthwardInventoryResult.SUCCESS)
            moved=access.transfer(bag,True,item,count,unreal.GuidLibrary.new_guid(),store.get_timeline_epoch())
            check("deposit "+item+str(remaining), moved.moved_count==count)
            remaining-=count
    st['samples']=[]
    for index,(kind,role) in enumerate([('workbench','Workbench'),('warehouse_access','Warehouse')]):
        player.set_actor_location(unreal.Vector(site['x']-400,site['y']-450+index*550,100),False,True)
        pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0))
        yield delay(1)
        check(kind+' selected',builder.select_building(kind))
        yield delay(.5)
        check(kind+' paid placement starts',builder.confirm_placement())
        yield wait(lambda: builder.building_count()==index+1,15)
        built=builder.get_buildings()[-1]
        meshes=built.get_components_by_class(unreal.StaticMeshComponent)
        expected=f'/Game/Hearthward/Assets/TASK-098/{role}/SM_{role}_Practical.SM_{role}_Practical'
        check(kind+' actual new mesh',len(meshes)==1 and meshes[0].static_mesh.get_path_name()==expected)
        check(kind+' bottom offset cm',abs(meshes[0].relative_location.z+36)<.001)
        check(kind+' facility registry',any(f['kind']==kind for f in json.loads(camp.describe())['facilities']))
        if kind=='warehouse_access':check('storage interaction component',bool(built.get_component_by_class(unreal.HearthwardStorageAccessComponent)))
        st['samples'].append({'kind':kind,'mesh':meshes[0].static_mesh.get_path_name(),'relative_z_cm':meshes[0].relative_location.z,'location':str(built.get_actor_location())})
    report['samples']=st['samples']
    report['remaining']=['Forge paid construction at required camp tier','DPI/inventory icon visual review','Owner visual acceptance','Cook and saved-world migration']
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
