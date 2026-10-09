import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / '.agent-local/qa/TASK-098/production-fire/pie'
assert not (out / "results.json").exists(), "Preserve earlier evidence"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "PROTOTYPE_ONLY isolated flat-ground fixture with supplied construction materials; real paid campfire placement, GPU Niagara activation, mesh/collision/interaction checks and SavePoint/LoadPoint; no gameplay balance or formal save changes"}
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
    unreal.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground',False)
    editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    floor = editor.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -100))
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    floor.set_actor_scale3d(unreal.Vector(1600, 1600, 1))
    floor.static_mesh_component.set_collision_profile_name("BlockAll")
    for index in range(6):
        c=editor.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(0,0,500))
        c.tags=['CampArtCamera:'+str(index)]
    sun=editor.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,1200),unreal.Rotator(pitch=-50,yaw=-40))
    sun.light_component.set_intensity(4)
    fill=editor.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,1200),unreal.Rotator(pitch=-35,yaw=140))
    fill.light_component.set_intensity(1.5)
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
    items=('wood','stone','rope')
    def counts():return {item:store.get_item_count(item) for item in items}
    for item,total in [('wood', 4), ('stone', 4)]:
        remaining=total
        while remaining:
            count=min(remaining,4)
            assert bag.try_add(item,count)==unreal.HearthwardInventoryResult.SUCCESS
            assert access.transfer(bag,True,item,count,unreal.GuidLibrary.new_guid(),store.get_timeline_epoch()).moved_count==count
            remaining-=count
    specs=[('campfire', 'Campfire', {'wood': 4, 'stone': 4})]
    report['models']=[]
    for index,(kind,label,cost) in enumerate(specs):
        if kind=='smelter':
            check('fixture rescue precondition',camp.record_rescue('civilian_initial_01'))
            check('paid camp tier two',camp.upgrade_camp(store.get_timeline_epoch()))
        player.set_actor_location(unreal.Vector(site['x']-500,site['y']-1600+index*550,100),False,True)
        pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0))
        yield delay(1)
        check(kind+' selected',builder.select_building(kind));yield delay(.5)
        before=counts()
        check(kind+' paid placement begins',builder.confirm_placement())
        yield wait(lambda:builder.building_count()==index+1,20)
        check(kind+' exact cost',all(before[k]-counts()[k]==cost.get(k,0) for k in items))
        built=builder.get_buildings()[-1]
        mesh=built.get_components_by_class(unreal.StaticMeshComponent)[0]
        expected='/Game/Hearthward/Assets/TASK-098/Workbench/SM_Workbench_Practical.SM_Workbench_Practical' if kind=='workbench' else '/Game/Hearthward/Assets/TASK-098/CampSet/SM_'+label+'.SM_'+label
        check(kind+' correct mesh',mesh.static_mesh.get_path_name()==expected)
        root=built.get_component_by_class(unreal.BoxComponent);extent=root.get_unscaled_box_extent()
        check(kind+' bottom offset',abs(mesh.relative_location.z+extent.z)<.01)
        check(kind+' visual mesh no collision',mesh.get_collision_enabled()==unreal.CollisionEnabled.NO_COLLISION)
        check(kind+' root collision retained',root.get_collision_enabled()==unreal.CollisionEnabled.QUERY_AND_PHYSICS)
        if kind in ['bed','campfire','medical_area']:
            check(kind+' interaction retained',bool(built.get_component_by_class(unreal.HearthwardFurnitureInteractionComponent)))
        report['models'].append({'kind':kind,'mesh':expected,'location':str(built.get_actor_location()),'root_extent_cm':str(extent)})
        yield delay(2)
        check(kind+' flame and smoke activate on GPU',sum(e.is_active() for e in built.get_components_by_class(unreal.NiagaraComponent))==2)
        camera=unreal.GameplayStatics.get_all_actors_with_tag(world,'CampArtCamera:'+str(index))[0]
        target=built.get_actor_location()+unreal.Vector(0,0,15)
        eye=target+unreal.Vector(195,-240,170)
        camera.set_actor_location(eye,False,True)
        camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,target),True)
        pc.set_view_target_with_blend(camera,0);yield delay(2)
        check(kind+' correct camera',(pc.player_camera_manager.get_camera_location()-eye).length()<5)
        unreal.SystemLibrary.execute_console_command(world,'Shot SHOWUI filename='+str(out/(kind+'.png'))+' -nosuffix')
        yield delay(1)
    registry=json.loads(camp.describe())['facilities'];balance=counts()
    check('save built facilities',save.save_point(True))
    check('load facilities',save.load_point(save.get_points()[-1].save_id));yield delay(3)
    check('registry and paid materials restored',json.loads(camp.describe())['facilities']==registry)
    check('inventory balance restored',counts()==balance)
    builder=unreal.GameplayStatics.get_player_pawn(world,0).get_component_by_class(unreal.HearthwardBuildingComponent)
    check('one unique campfire restored',builder.building_count()==1)
    actual=[m.static_mesh.get_path_name() for a in builder.get_buildings() for m in a.get_components_by_class(unreal.StaticMeshComponent)]
    check('all meshes restored',sorted(actual)==sorted(m['mesh'] for m in report['models']))
    report['remaining']=['Full campaign deferred by owner','Shipping cook','Owner visual acceptance']
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