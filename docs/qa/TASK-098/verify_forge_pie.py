import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / '.agent-local/qa/TASK-098/forge-20261008'
assert not (out / "results.json").exists(), "Preserve earlier evidence"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "PROTOTYPE_ONLY isolated flat-ground fixture; fixture supplies and recorded rescue precondition; real tier gate, paid camp upgrade, workbench/forge placement, crafting and SavePoint/LoadPoint; no gameplay balance or formal save changes"}
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
    items = ('wood', 'stone', 'metal_ingot')
    def counts():
        return {item: store.get_item_count(item) for item in items}
    def debit(label, before, cost):
        after = counts()
        report.setdefault('transactions', []).append({'step': label, 'before': before, 'after': after, 'cost': cost})
        check(label+' exact debit', all(before[k]-after[k]==cost.get(k,0) for k in items))
    before=counts()
    check('tier one rejects forge', not builder.select_building('forge'))
    check('rejected selection consumes nothing', counts()==before and builder.building_count()==0)
    for item, total in (('wood',290),('stone',164),('metal_ingot',35)):
        remaining=total
        while remaining:
            count=min(remaining,4)
            assert bag.try_add(item,count)==unreal.HearthwardInventoryResult.SUCCESS, item+' fixture supply'
            moved=access.transfer(bag,True,item,count,unreal.GuidLibrary.new_guid(),store.get_timeline_epoch())
            assert moved.moved_count==count, item+' fixture deposit'
            remaining-=count
    report['supplied']={'wood':290,'stone':164,'metal_ingot':35}
    for index,(kind,role,cost) in enumerate([
        ('workbench','Workbench',{'wood':72}),
        ('forge','Forge',{'wood':160,'stone':140,'metal_ingot':20})]):
        if kind=='forge':
            check('fixture rescue precondition',camp.record_rescue('civilian_initial_01'))
            before=counts()
            check('actual camp upgrade',camp.upgrade_camp(store.get_timeline_epoch()))
            debit('camp tier two',before,{'wood':48,'stone':24})
        player.set_actor_location(unreal.Vector(site['x']-400,site['y']-450+index*550,100),False,True)
        pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0))
        yield delay(1)
        check(kind+' selected',builder.select_building(kind))
        yield delay(.5)
        before=counts()
        check(kind+' paid placement starts',builder.confirm_placement())
        yield wait(lambda: builder.building_count()==index+1,20)
        debit(kind,before,cost)
        built=builder.get_buildings()[-1]
        meshes=built.get_components_by_class(unreal.StaticMeshComponent)
        expected=f'/Game/Hearthward/Assets/TASK-098/{role}/SM_{role}_Practical.SM_{role}_Practical'
        check(kind+' actual new mesh',len(meshes)==1 and meshes[0].static_mesh.get_path_name()==expected)
        check(kind+' bottom offset cm',abs(meshes[0].relative_location.z+36)<.001)
    facility=next(f for f in json.loads(camp.describe())['facilities'] if f['kind']=='forge')
    parsed=unreal.GuidLibrary.parse_string_to_guid(facility['id'])
    forge_id=parsed[0] if isinstance(parsed,tuple) else parsed
    player.set_actor_location(built.get_actor_location()+unreal.Vector(-180,0,100),False,True)
    yield delay(1)
    report['craft_status']=builder.crafting_status(forge_id,'craft_shortblade_2',1,store.get_timeline_epoch())
    before=counts(); old_output=bag.get_item_count('shortblade_2')
    check('actual forge recipe',camp.craft(forge_id,'craft_shortblade_2',1,store.get_timeline_epoch()))
    debit('craft shortblade two',before,{'wood':10,'metal_ingot':15})
    check('craft output added once',bag.get_item_count('shortblade_2')==old_output+1)
    report['forge_before_load']=facility
    report['materials_before_load']=counts()
    check('manual save',save.save_point(True))
    point=save.get_points()[-1].save_id
    check('actual load',save.load_point(point))
    yield delay(3)
    player=unreal.GameplayStatics.get_player_pawn(world,0)
    builder=player.get_component_by_class(unreal.HearthwardBuildingComponent)
    bag=player.get_component_by_class(unreal.HearthwardInventoryComponent)
    restored=next(f for f in json.loads(camp.describe())['facilities'] if f['id']==facility['id'])
    report['forge_after_load']=restored
    check('forge registry including paid materials restored',restored==facility)
    check('shared materials restored exactly',counts()==report['materials_before_load'])
    check('crafted output restored once',bag.get_item_count('shortblade_2')==old_output+1)
    expected='/Game/Hearthward/Assets/TASK-098/Forge/SM_Forge_Practical.SM_Forge_Practical'
    forges=[a for a in builder.get_buildings() if any(c.static_mesh and c.static_mesh.get_path_name()==expected for c in a.get_components_by_class(unreal.StaticMeshComponent))]
    check('one forge restored with approved mesh',len(forges)==1)
    check('both paid buildings restored',builder.building_count()==2)
    report['remaining']=['DPI/inventory icon visual review','Owner visual acceptance','Cook and older saved-world migration']
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
