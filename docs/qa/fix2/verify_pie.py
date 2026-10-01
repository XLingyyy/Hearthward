"""Real-rendering loading/save round trip and scene checks; isolated pool via UEClient.
Run from L_Bootstrap. Captures gameplay evidence, not a performance benchmark.
"""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Fix2'
out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
report={'ok':False,'checks':{},'loading_samples':[]}
move_input=None

def check(name,value):
    report['checks'][name]=bool(value)
    if not value: raise AssertionError(name)

def delay(seconds):
    end=time.monotonic()+seconds
    return lambda: time.monotonic()>=end

def objects():
    world=editor.get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0)
    ui=pc.get_hud().get_editor_property('screen')
    loading=next(s for s in unreal.ObjectIterator(unreal.HearthwardLoadingSubsystem) if s.get_outer()==unreal.GameplayStatics.get_game_instance(world))
    return world,pc,ui,loading

def natural():
    world=editor.get_game_world()
    return world and 'L_HearthwardWilds' in world.get_name() and unreal.GameplayStatics.get_player_controller(world,0)

def shot(world,pc,name):
    unreal.SystemLibrary.execute_console_command(world,f'Shot SHOWUI filename="{(out/name).as_posix()}.png" -nosuffix',pc)

def run():
    global move_input
    levels.editor_request_begin_play()
    yield levels.is_in_play_in_editor
    yield delay(3)
    world,pc,ui,loading=objects()
    check('bootstrap title',str(ui.get_page())=='title')
    check('new from title',ui.execute_action('new'))
    check('overlay before travel',loading.is_loading())
    check('commands blocked while loading',not ui.execute_action('new'))
    yield natural
    world,pc,ui,loading=objects()
    check('overlay after map load',loading.is_loading())
    shot(world,pc,'loading')
    yield lambda: not loading.is_loading()
    campaign=next(s for s in unreal.ObjectIterator(unreal.HearthwardCampaignSubsystem) if s.get_outer()==world)
    check('intro ready before reveal',not campaign.busy())
    check('new game HUD',str(ui.get_page())=='hud')
    check('save available after loading',ui.execute_action('save'))
    save=next(s for s in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if s.get_outer()==world)
    points=save.get_points()
    point=points[-1]
    saved_position=unreal.GameplayStatics.get_player_pawn(world,0).get_actor_location()
    check('same map read',ui.execute_action('load:'+point.save_id.to_string()))
    check('same map loading visible',loading.is_loading())
    yield lambda: not loading.is_loading()
    pawn=unreal.GameplayStatics.get_player_pawn(world,0)
    check('saved position restored',(pawn.get_actor_location()-saved_position).length()<20)
    ui.open_page('settings')
    check('Epic draft defaults',ui.execute_action('settings.defaults'))
    check('Epic applied',ui.execute_action('settings.apply'))
    ui.open_page('hud')
    settings=unreal.GameUserSettings.get_game_user_settings()
    settings.set_resolution_scale_value_ex(100)
    settings.apply_non_resolution_settings()
    check('Epic quality',settings.get_overall_scalability_level()==3)
    report['quality']=settings.get_overall_scalability_level()
    report['resolution_scale']=settings.get_resolution_scale_normalized()
    report['renderer']={
        'anti_aliasing_method':unreal.SystemLibrary.get_console_variable_int_value('r.AntiAliasingMethod'),
        'virtual_shadows':unreal.SystemLibrary.get_console_variable_int_value('r.Shadow.Virtual.Enable'),
        'view_distance_scale':unreal.SystemLibrary.get_console_variable_float_value('r.ViewDistanceScale'),
        'grass_density_scale':unreal.SystemLibrary.get_console_variable_float_value('grass.DensityScale')}
    fills=[a.get_component_by_class(unreal.DirectionalLightComponent) for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.DirectionalLight) if 'HearthwardNightFill' in a.tags]
    check('night fill retained after campaign lighting',len(fills)==1 and fills[0].intensity>.5)
    volumes=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PostProcessVolume)
    check('scene grade active',any(v.priority==20 and v.unbound for v in volumes))
    fog=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.ExponentialHeightFog)[0].component
    check('fog layer at terrain height',abs(fog.get_world_location().z-19500)<1)
    check('volumetric fog enabled',fog.get_editor_property('enable_volumetric_fog'))
    sky_meshes=[c for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Actor) for c in a.get_components_by_class(unreal.StaticMeshComponent) if any('Sky' in m.get_path_name() for m in c.get_materials() if m)]
    report['sky_meshes']=[{'component':c.get_name(),'mesh':c.static_mesh.get_name(),'visible':c.is_visible()} for c in sky_meshes]
    check('template sky disabled',bool(sky_meshes) and all(not c.is_visible() for c in sky_meshes))
    pc.set_control_rotation(unreal.Rotator(pitch=-10,yaw=0))
    yield delay(2)
    shot(world,pc,'night-interior')
    # Use a collision-free exterior point to inspect the same scene lighting and water.
    house=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HearthwardTask028CampHouse) if 'CampaignPrologueHouse' in [str(t) for t in a.tags])
    pawn.set_actor_location(house.get_actor_location()+unreal.Vector(650,-700,150),False,True)
    pc.set_control_rotation(unreal.Rotator(pitch=-12,yaw=175))
    yield delay(3)
    shot(world,pc,'night-exterior')
    start=pawn.get_actor_location()
    subsystem=next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if isinstance(s.get_outer(),unreal.LocalPlayer))
    actions=[a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_outer()==pawn]
    action=next(a for a in actions if any(str(unreal.InputLibrary.key_get_display_name(k))=='W' for k in subsystem.query_keys_mapped_to_action(a)))
    move_input=(subsystem,action)
    yield delay(2)
    move_input=None
    check('movement after loading',(pawn.get_actor_location()-start).length()>100)
    pawn.jump()
    yield delay(.2)
    check('jump after loading',pawn.get_movement_component().is_falling())
    yield delay(1)
    pawn.stop_jumping()
    check('inventory after loading',ui.execute_action('page:inventory'))
    yield delay(1)
    ui.open_page('hud')
    yield delay(1)
    check('return to title',ui.execute_action('title'))
    yield lambda: editor.get_game_world() and 'L_Bootstrap' in editor.get_game_world().get_name()
    world,pc,ui,loading=objects()
    yield lambda: not loading.is_loading()
    check('continue from title',ui.execute_action('continue'))
    yield natural
    world,pc,ui,loading=objects()
    check('cross map read overlay',loading.is_loading())
    yield lambda: not loading.is_loading()
    check('cross map read HUD',str(ui.get_page())=='hud')
    pawn=unreal.GameplayStatics.get_player_pawn(world,0)
    save=next(s for s in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if s.get_outer()==world)
    check('cross map saved location',(pawn.get_actor_location()-saved_position).length()<30)
    shot(world,pc,'continued')
    yield delay(.5)
    campaign=next(s for s in unreal.ObjectIterator(unreal.HearthwardCampaignSubsystem) if s.get_outer()==world)
    check('prologue relic interaction',campaign.interact())
    gameplay=pawn.get_component_by_class(unreal.HearthwardGameplayComponent)
    check('brother follow order',gameplay.order_companion('follow'))
    positions=json.loads(campaign.describe())['positions']
    report['campaign_positions']=positions
    exit_xy=next(p['xy'] for p in json.loads((Path(unreal.Paths.project_dir())/'Resources/Data/gameplay.json').read_text(encoding='utf-8'))['campaign']['locations'] if p['id']=='prologue_exit')
    hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(exit_xy[0]*100,exit_xy[1]*100,80000),unreal.Vector(exit_xy[0]*100,exit_xy[1]*100,-30000),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True).to_tuple()
    check('exit ground ready',hit[0])
    landing=hit[4]+unreal.Vector(0,0,100)
    pawn.set_actor_location(landing,False,True)
    brother=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HearthwardCompanionFixture)[0]
    brother.set_actor_location(landing+unreal.Vector(0,180,0),False,True)
    check('escape interaction',campaign.interact())
    yield lambda: not campaign.busy() and not loading.is_loading()
    check('day camp phase',json.loads(campaign.describe())['phase']=='occupied')
    camp_actors=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Actor)
    for tag,expected,name in [('Hearthward.NaturalCamp','R','camp world label has supported glyphs'),('Hearthward.Quartermaster','Tab','quartermaster world label has supported glyphs')]:
        labels=[c for a in camp_actors if a.actor_has_tag(tag) for c in a.get_components_by_class(unreal.TextRenderComponent)]
        check(name,bool(labels) and all(str(c.get_editor_property('text'))==expected for c in labels))
    pc.set_control_rotation(unreal.Rotator(pitch=-10,yaw=40))
    yield delay(3)
    shot(world,pc,'day-camp')
    check('day save',ui.execute_action('save'))
    report['day_save_id']=save.get_points()[-1].save_id.to_string()
    report['ok']=True

runner=run()
pending=None
deadline=time.monotonic()+240
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline: raise TimeoutError('fix2 verification')
        if move_input:
            subsystem,action=move_input
            subsystem.inject_input_vector_for_action(action,unreal.Vector(0,1,0),[],[])
        if pending and not pending(): return
        pending=next(runner)
        return
    except StopIteration: pass
    except Exception: report['error']=traceback.format_exc()
    (out/'verify.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    levels.editor_request_end_play()
handle=unreal.register_slate_post_tick_callback(tick)
