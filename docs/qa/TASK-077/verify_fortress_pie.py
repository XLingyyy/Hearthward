"""Actual natural-map PIE: opening, capsule movement, companion follow and rendered views.
Movement is injected through Pawn input; this is not a physical-keyboard acceptance test.
"""
from pathlib import Path
import json,time,traceback
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
visual='-HearthwardFortressVisual' in unreal.SystemLibrary.get_command_line()
inspect_courtyard='-HearthwardCourtyardInspect' in unreal.SystemLibrary.get_command_line()
out=Path(unreal.Paths.project_dir())/'.agent-local/qa/TASK-077'/('courtyard-v2' if inspect_courtyard else 'visual' if visual else 'pie')
out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
report={'ok':False,'checks':{},'method':__doc__,'positions':[]}
moving=None

def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)

def delay(seconds):
    end=time.monotonic()+seconds
    return lambda:time.monotonic()>=end

def subsystem(cls,world):
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==world)

def shot(world,pc,name):
    unreal.SystemLibrary.execute_console_command(world,f'Shot SHOWUI filename="{(out/name).as_posix()}.png" -nosuffix',pc)

def run():
    global moving
    editor.get_editor_world().get_world_settings().set_editor_property('default_game_mode',unreal.HearthwardGameMode)
    if inspect_courtyard:
        actor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(120000,53500,32000))
        actor.tags=['CourtyardV2Inspection']
        mesh=actor.get_component_by_class(unreal.StaticMeshComponent)
        mesh.set_static_mesh(unreal.load_asset('/Game/Hearthward/Assets/TASK-077/SM_StoneholdCourtyard/StaticMeshes/SM_StoneholdCourtyard'))
        mesh.set_collision_profile_name('NoCollision')
        mesh.set_cast_shadow(False)
    levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(4)
    world=editor.get_game_world();pc=unreal.GameplayStatics.get_player_controller(world,0)
    pawn=unreal.GameplayStatics.get_player_pawn(world,0)
    unreal.GameplayStatics.set_game_paused(world,False)
    game=pawn.get_component_by_class(unreal.HearthwardGameplayComponent)
    ui=pc.get_hud().get_editor_property('screen')
    campaign=subsystem(unreal.HearthwardCampaignSubsystem,world)
    check('normal new game',ui.execute_action('new'))
    yield lambda:not campaign.busy()
    yield delay(3)
    brother=unreal.GameplayStatics.get_actor_of_class(world,unreal.HearthwardCompanionFixture)
    homes=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HearthwardHometownFortress)
    check('one independent fortress',len(homes)==1)
    home=homes[0];origin=home.get_actor_location()
    report['origin']=str(origin)
    state=json.loads(campaign.describe());report['opening_state']={k:state[k] for k in ['phase','facts','positions']}
    blocks=[]
    for component in home.get_components_by_class(unreal.StaticMeshComponent):
        if component.static_mesh and component.static_mesh.get_name()=='Cube':
            p=component.get_editor_property('relative_location');s=component.get_editor_property('relative_scale3d')
            blocks.append({'name':component.get_name(),'center_cm':[p.x,p.y,p.z],'size_cm':[s.x*100,s.y*100,s.z*100]})
    (out/'blocks.json').write_text(json.dumps(blocks,indent=2),encoding='utf-8')
    report['player_start']=str(pawn.get_actor_location());report['brother_start']=str(brother.get_actor_location())
    check('player in bedroom',(pawn.get_actor_location()-origin).length2d()<600)
    check('brother in bedroom',(brother.get_actor_location()-origin).length2d()<600)
    pc.set_control_rotation(unreal.Rotator(pitch=-12,yaw=90))
    yield delay(1);shot(world,pc,'bedroom');yield delay(1)
    pc.set_control_rotation(unreal.Rotator(pitch=-15,yaw=-90))
    yield delay(.5);shot(world,pc,'bedroom-beds');yield delay(.5)
    if inspect_courtyard:
        actor=unreal.GameplayStatics.get_all_actors_with_tag(world,'CourtyardV2Inspection')[0]
        arm=pawn.get_component_by_class(unreal.SpringArmComponent)
        arm.set_editor_property('do_collision_test',False)
        arm.set_editor_property('target_arm_length',0)
        pawn.get_component_by_class(unreal.CharacterMovementComponent).disable_movement()
        pawn.set_actor_hidden_in_game(True)
        pawn.set_actor_location(actor.get_actor_location()+unreal.Vector(0,0,180),False,False)
        for yaw in (-90,0,90,180):
            unreal.GameplayStatics.set_game_paused(world,False)
            pc.set_control_rotation(unreal.Rotator(pitch=0,yaw=yaw))
            yield delay(1)
            unreal.GameplayStatics.set_game_paused(world,True)
            for light in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.DirectionalLight):
                night='HearthwardNightFill' in [str(t) for t in light.tags]
                light.get_component_by_class(unreal.DirectionalLightComponent).set_intensity(0 if night else 3)
                if not night:light.set_actor_rotation(unreal.Rotator(pitch=-45,yaw=-40),False)
            yield delay(1);shot(world,pc,'courtyard-'+str(yaw));yield delay(.5)
            if yaw==-90:
                unreal.SystemLibrary.execute_console_command(world,'viewmode unlit',pc)
                yield delay(1);shot(world,pc,'courtyard-unlit');yield delay(.5)
                unreal.SystemLibrary.execute_console_command(world,'viewmode lit',pc)
        unreal.GameplayStatics.set_game_paused(world,False)
        pawn.set_actor_location(actor.get_actor_location()+unreal.Vector(0,7500,3500),False,False)
        pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=-90))
        yield delay(1)
        unreal.GameplayStatics.set_game_paused(world,True)
        for light in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.DirectionalLight):
            night='HearthwardNightFill' in [str(t) for t in light.tags]
            light.get_component_by_class(unreal.DirectionalLightComponent).set_intensity(0 if night else 8)
            if not night:light.set_actor_rotation(unreal.Rotator(pitch=-45,yaw=-40),False)
        yield delay(1);shot(world,pc,'courtyard-exterior');yield delay(.5)
        unreal.SystemLibrary.execute_console_command(world,'viewmode unlit',pc)
        yield delay(1);shot(world,pc,'courtyard-exterior-unlit');yield delay(.5)
        report['ok']=True
        report['inspection_only']='Transient courtyard actor and source-view camera; no map saved and no gameplay acceptance.'
        report['mesh_cast_shadow']=False
        return
    if visual:
        # Inspection camera/daylight only. This does not alter the opening's night setting.
        arm=pawn.get_component_by_class(unreal.SpringArmComponent)
        arm.set_editor_property('do_collision_test',False)
        arm.set_editor_property('target_arm_length',12000)
        pc.set_control_rotation(unreal.Rotator(pitch=-32,yaw=-35))
        yield delay(1)
        shot(world,pc,'overview-opening-night');yield delay(.5)
        unreal.GameplayStatics.set_game_paused(world,True)
        for light in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.DirectionalLight):
            component=light.get_component_by_class(unreal.DirectionalLightComponent)
            component.set_intensity(0 if 'HearthwardNightFill' in [str(t) for t in light.tags] else 8)
            if 'HearthwardNightFill' not in [str(t) for t in light.tags]:
                light.set_actor_rotation(unreal.Rotator(pitch=-45,yaw=-40),False)
        for component in home.get_components_by_class(unreal.StaticMeshComponent):
            if component.get_name()=='CourtyardFacade':
                component.get_material(0).set_vector_parameter_value('Tint',unreal.LinearColor(.85,.85,.85,1))
        yield delay(2);shot(world,pc,'overview-inspection-daylight');yield delay(1)
        report['ok']=True;report['visual_only']='Temporary inspection camera and daylight; not opening lighting.'
        return
    # Actual input-driven movement, never teleporting the player along the route.
    for label,x,y in [('relic',-200,170),('door',0,850),('gallery',1500,850),('stairs',1500,2900),('court',0,5700),('postern',0,7100),('exit',-2000,8500)]:
        goal=origin+unreal.Vector(x,y,0)
        moving=(pawn,goal)
        end=time.monotonic()+40
        yield lambda:(pawn.get_actor_location()-goal).length2d()<65 or time.monotonic()>end
        moving=None
        report['positions'].append({'point':label,'player':str(pawn.get_actor_location()),'brother':str(brother.get_actor_location()),'distance':(pawn.get_actor_location()-goal).length2d()})
        (out/'progress.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        check('walk to '+label,(pawn.get_actor_location()-goal).length2d()<100)
        if label=='relic':
            check('relic interaction',campaign.interact())
            check('follow command',game.order_companion('follow'))
        if label in ('gallery','court','postern'):
            pc.set_control_rotation(unreal.Rotator(pitch=-10,yaw=-35 if label=='court' else 0));yield delay(.5);shot(world,pc,label);yield delay(1)
    yield delay(8)
    check('brother reaches escape',(brother.get_actor_location()-pawn.get_actor_location()).length2d()<1000)
    check('exit interaction',campaign.interact())
    yield lambda:not campaign.busy()
    state=json.loads(campaign.describe());report['after_escape']={k:state[k] for k in ['phase','facts']}
    check('campaign leaves prologue',report['after_escape']['phase']=='occupied')
    report['ok']=True

runner=run();pending=None;deadline=time.monotonic()+420
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('Fortress PIE')
        if moving:
            pawn,goal=moving
            direction=goal-pawn.get_actor_location();direction.z=0
            pawn.add_movement_input(direction.normal(),1,False)
        if pending and not pending():return
        pending=next(runner);return
    except StopIteration:pass
    except Exception:report['error']=traceback.format_exc()
    (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    levels.editor_request_end_play()

handle=unreal.register_slate_post_tick_callback(tick)
