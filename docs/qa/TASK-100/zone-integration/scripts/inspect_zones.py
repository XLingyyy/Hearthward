import unreal,json,time,traceback
from pathlib import Path
out=Path('G:/GameFactory/Hearthward/.agent-local/qa/TASK-100/world-04');out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'method':'Production natural map/new-game scenery; diagnostic repositioning and camera. Not normal campaign completion.','houses':[]}
pending=None;busy=False
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
def delay(s):
    until=time.monotonic()+s
    return lambda:time.monotonic()>=until
def xyz(v):return [round(v.x,2),round(v.y,2),round(v.z,2)]
def ground(world,point):
    hits=unreal.SystemLibrary.line_trace_multi_for_objects(world,unreal.Vector(point.x,point.y,80000),unreal.Vector(point.x,point.y,-30000),[unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1],False,[],unreal.DrawDebugTrace.NONE,True)
    for hit in hits:
        b=hit.to_tuple()
        if b[9] and (b[9].actor_has_tag('Hearthward.NatureGround') or 'Landscape' in b[9].get_class().get_name()) and b[7].z>.65:return b[5]
    raise ValueError('No terrain at '+str(point))
def run():
    unreal.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground',False)
    levels.load_level('/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds')
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/Hearthward.HearthwardGameMode'))
    preview=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.CameraActor,unreal.Vector(0,0,200))
    preview.tags=['Task100PreviewCamera']
    levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(3)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0);ui=pc.get_hud().get_editor_property('screen')
    assert ui.execute_action('new');yield delay(15)
    unreal.GameplayStatics.set_game_paused(world,False)
    player=unreal.GameplayStatics.get_player_character(world,0)
    rows=json.loads(Path('G:/GameFactory/Hearthward/Resources/Data/gameplay.json').read_text(encoding='utf-8'))['campaign']['zones']
    camera=unreal.GameplayStatics.get_all_actors_with_tag(world,'Task100PreviewCamera')[0]
    for zone in rows:
        player.set_actor_location(ground(world,unreal.Vector(zone['center'][0]*100,zone['center'][1]*100,0))+unreal.Vector(0,0,95),False,True)
        yield delay(3)
        for i in range(4):
            tag='CampaignHouse:'+zone['id']+':'+str(i)
            actor=next(a for a in unreal.GameplayStatics.get_all_actors_with_tag(world,tag))
            p=actor.get_actor_location();direction=actor.get_actor_forward_vector()
            samples=[ground(world,p+unreal.Vector(dx,dy,0)).z for dx,dy in [(0,0),(-350,-350),(-350,350),(350,-350),(350,350)]]
            meshes=actor.get_components_by_class(unreal.StaticMeshComponent)
            row={'id':tag,'position':xyz(p),'terrain_span_cm':round(max(samples)-min(samples),2),'terrain_z':samples,'mesh':meshes[0].static_mesh.get_path_name(),'bounds':str(meshes[0].static_mesh.get_bounding_box())}
            start=ground(world,p-direction*430)+unreal.Vector(0,0,95)
            player.set_actor_location(start,False,True);player.character_movement.stop_movement_immediately()
            pc.set_control_rotation(unreal.MathLibrary.make_rot_from_x(direction))
            deadline=time.monotonic()+5
            def walked():
                player.add_movement_input(direction,1.0,False)
                return (player.get_actor_location()-start).dot(direction)>850 or time.monotonic()>deadline
            yield walked
            distance=(player.get_actor_location()-start).dot(direction)
            row['walk_distance_cm']=round(distance,1);row['passage_walked']=distance>850
            report['houses'].append(row)
            if i==0:
                target=p+unreal.Vector(0,0,180);eye=p+unreal.Vector(1050,-1250,750)
                camera.set_actor_location(eye,False,True);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,target),False)
                pc.set_view_target_with_blend(camera,0)
                yield delay(1)
                unreal.SystemLibrary.execute_console_command(world,'Shot SHOWUI filename='+str(out/(zone['id']+'.png'))+' -nosuffix')
                yield delay(1)
    report['passed']=len(report['houses'])==16 and all(r['passage_walked'] for r in report['houses'])
iterator=run();deadline=time.monotonic()+300
def finish():
    (out/'result.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(delta):
    global pending,busy
    if busy:return
    busy=True
    try:
        if time.monotonic()>deadline:raise TimeoutError('zone inspection')
        if pending and not pending():return
        pending=next(iterator)
    except StopIteration:finish()
    except Exception:report['error']=traceback.format_exc();finish()
    finally:busy=False
handle=unreal.register_slate_post_tick_callback(tick)
