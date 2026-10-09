import unreal,json,time,traceback
from pathlib import Path
out=Path(unreal.Paths.project_dir()).resolve()/'.agent-local/qa/TASK-095/windup/visual';out.mkdir(parents=True,exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'fixture':'PROTOTYPE_ONLY unsaved scene; shared production melee authority and animation','samples':[]}
pending=None;busy=False;active_world=None
def delay(s):
    end=time.monotonic()+s
    return lambda:time.monotonic()>=end
def run():
    global active_world
    unreal.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground',False)
    levels.load_level('/Game/Hearthward/Tests/Graybox/L_GrayboxValidation')
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    camera=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(220,230,160));camera.tags=['MeleeCapture'];camera.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    light=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(150,180,320));light.tags=['MeleeLight']
    levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(2)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();unreal.GameplayStatics.set_game_paused(world,False)
    unreal.SystemLibrary.execute_console_command(world,'Hearthward.Test095.CastPreview')
    hero=unreal.GameplayStatics.get_player_pawn(world,0)
    hero.get_component_by_class(unreal.load_class(None,'/Script/Hearthward.HearthwardGameplayComponent')).set_component_tick_enabled(False)
    brother=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.load_class(None,'/Script/Hearthward.HearthwardCompanionFixture')) if a.actor_has_tag('Task095Cast.Brother'))
    brother.set_actor_location(unreal.Vector(0,0,82.4),False,True)
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Character):
        if actor!=brother:actor.set_actor_location(actor.get_actor_location()+unreal.Vector(-2000,0,0),False,True)
    camera=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SceneCapture2D) if a.actor_has_tag('MeleeCapture'))
    light=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PointLight) if a.actor_has_tag('MeleeLight'))
    light.point_light_component.set_intensity(220);light.point_light_component.set_attenuation_radius(1500)
    target=unreal.RenderingLibrary.create_render_target2d(world,960,720,unreal.TextureRenderTargetFormat.RTF_RGBA8)
    c=camera.capture_component2d;c.texture_target=target;c.capture_source=unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR
    c.set_editor_property('capture_every_frame',False);c.set_editor_property('capture_on_movement',False);c.fov_angle=52
    look=unreal.Vector(50,0,90);position=look+unreal.Vector(220,300,100)
    camera.set_actor_location(position,False,True);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(position,look),False)
    for item in ['spear','longblade_2','axe']:
        folder=out/item;folder.mkdir(exist_ok=True)
        unreal.SystemLibrary.execute_console_command(world,'Hearthward.Test095.MeleePreview setup '+item);yield delay(1.3)
        victim=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Actor) if a.actor_has_tag('Task095MeleeTarget'))
        health=victim.get_component_by_class(unreal.load_class(None,'/Script/Hearthward.HearthwardCombatTargetComponent'))
        active_world=world
        for i in range(36):
            yield delay(.04)
            anim=brother.mesh.get_anim_instance()
            report['samples'].append({'item':item,'frame':i,'world_time':unreal.GameplayStatics.get_time_seconds(world),'health':health.health,'motion':str(anim.get_editor_property('motion_state'))})
            c.capture_scene();yield delay(.01);unreal.RenderingLibrary.export_render_target(world,target,str(folder),f'{i:03}.png')
        active_world=None
    report['passed']=True
iterator=run();deadline=time.monotonic()+240
def finish():
    (out/'result.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(delta):
    global pending,busy
    if busy:return
    busy=True
    try:
        if active_world:unreal.SystemLibrary.execute_console_command(active_world,'Hearthward.Test095.MeleePreview tick')
        if time.monotonic()>deadline:raise TimeoutError('melee capture')
        if pending and not pending():return
        pending=next(iterator)
    except StopIteration:finish()
    except Exception:report['error']=traceback.format_exc();finish()
    finally:busy=False
handle=unreal.register_slate_post_tick_callback(tick)
