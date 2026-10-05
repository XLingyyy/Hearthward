"""TASK-062 rendered UI. Explicit isolated initial-save fixtures; no natural-map/input acceptance."""
import copy
import json
import struct
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task062/presentation'
out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'ok':False,'checks':{},'screens':{},'method':'Actual PIE HUD ScreenWidget OpenNature/DescribeLayout/Shot SHOWUI; temporary reflected InitialWorld fixtures restored through StartNewProgress in an isolated GUID save pool. No physical-input or natural-map acceptance; map is not saved.'}
st={}

def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)

def wait(predicate,seconds=20):
    return predicate,time.monotonic()+seconds

def delay(seconds):
    deadline=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=deadline,seconds+5)

def guid(text):
    value=unreal.GuidLibrary.parse_string_to_guid(text)
    return value[0] if isinstance(value,tuple) else value

def id_text():
    return unreal.GuidLibrary.conv_guid_to_string(unreal.GuidLibrary.new_guid())

def position(x,y,z=0):
    return {'x':x,'y':y,'z':z}

def crop(calendar):
    value=copy.deepcopy(st['base_nature'])
    value.update(calendar=calendar,points=[],animals=[],slots=[],pens=[],crops=[{'id':id_text(),'definition':'greens','position':position(300,200),'planted':100,'watered':True,'fertilized':True}])
    return value,value['crops'][0]['id']

def pen(species,products=0):
    return {'id':id_text(),'definition':species,'position':position(300,200),'level':1,'feed':0,'products':products,'paid':{},'pairs':{}}

def animal(home,juvenile=False,growth=0,product_minutes=0,fed=1440):
    health={'hen':20,'goat':60,'pig':80}[home['definition']]
    return {'id':id_text(),'definition':home['definition'],'pen':home['id'],'position':home['position'],'health':health,'domestic':True,'captured':True,'juvenile':juvenile,'growth':growth,'fedRemaining':fed,'productMinutes':product_minutes}

def livestock(species,young=False):
    value=copy.deepcopy(st['base_nature']);home=pen(species,6 if species=='goat' else 0)
    animals=[]
    if young:animals=[animal(home,juvenile=True,growth=720)]
    elif species=='goat':animals=[animal(home,product_minutes=1200,fed=0),animal(home,product_minutes=240)]
    value.update(calendar=1540,points=[],crops=[],slots=[],pens=[home],animals=animals)
    return value,(animals[0]['id'] if young else home['id'])

def install(value,wood=0):
    # These are existing UPROPERTY fields, not Nature.State (which is unreflected).
    initial=st['save'].get_editor_property('initial_world')
    initial.set_editor_property('calendar_minutes',value['calendar'])
    initial.set_editor_property('nature',json.dumps(value,ensure_ascii=False))
    camp=copy.deepcopy(st['base_camp']);camp['calendar']=value['calendar']
    initial.set_editor_property('camp_economy',json.dumps(camp,ensure_ascii=False))
    bag=initial.get_editor_property('player_items')
    bag.set_editor_property('backpack_rank',1)
    bag.set_editor_property('stacks',{'wood':wood} if wood else {})
    bag.set_editor_property('instances',[]);bag.set_editor_property('equipped',{})
    initial.set_editor_property('player_items',bag)
    st['save'].set_editor_property('initial_world',initial)
    st['ui'].open_page('hud')
    check('restore fixture '+str(len(report['screens'])),st['save'].start_new_progress())
    actual=json.loads(st['nature'].describe())
    check('fixture calendar '+str(len(report['screens'])),abs(actual['calendar']-value['calendar'])<.01)
    return actual

def intersects(a,b):
    return min(a[0]+a[2],b[0]+b[2])>max(a[0],b[0]) and min(a[1]+a[3],b[1]+b[3])>max(a[1],b[1])

def observe(label,marker):
    layout=json.loads(st['ui'].describe_layout())
    rows=[r for r in layout['components'] if r['visible']]
    text='\n'.join(r.get('text','') for r in rows)
    details=[r for r in rows if marker in r.get('text','') and not r.get('action','')]
    check(label+' one real detail text',len(details)==1)
    info=details[0];right=[r for r in rows if r.get('action','').startswith('nature.') and r['rect'][0]>=info['rect'][0] and r['rect'][1]>=info['rect'][1]]
    check(label+' detail at most three lines',len(info['text'].splitlines())<=3)
    check(label+' buttons avoid detail',all(not intersects(info['rect'],r['rect']) for r in right))
    check(label+' buttons avoid each other',all(not intersects(a['rect'],b['rect']) for i,a in enumerate(right) for b in right[i+1:]))
    check(label+' buttons above feedback',all(r['rect'][1]+r['rect'][3]<=815 for r in right))
    check(label+' controls inside design canvas',all(r['rect'][0]>=0 and r['rect'][0]+r['rect'][2]<=1672 and r['rect'][1]>=0 and r['rect'][1]+r['rect'][3]<=941 for r in [info]+right))
    report['screens'][label]={'text':text,'info':info,'detail_buttons':right,'png':str(out/(label+'.png'))}
    (out/(label+'-layout.json')).write_text(json.dumps(layout,ensure_ascii=False,indent=2),encoding='utf-8')
    st.setdefault('shot_started',{})[label]=time.time()
    unreal.SystemLibrary.execute_console_command(st['world'],f'Shot SHOWUI filename="{(out/label).as_posix()}.png" -nosuffix',st['pc'])
    return text

def finish(error=None):
    if error:report['error']=error
    try:
        report['nature']=json.loads(st['nature'].describe()) if 'nature' in st else None
        report['save_status']=st['save'].get_status() if 'save' in st else None
    except Exception:report['observer_error']=traceback.format_exc()
    report['ok']=not error and bool(report['checks']) and all(report['checks'].values())
    (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()

def run():
    editor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    floor=editor.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-100))
    floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    floor.set_actor_scale3d(unreal.Vector(1600,1600,1));floor.static_mesh_component.set_collision_profile_name('BlockAll');floor.tags=['Hearthward.NatureGround']
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pawn=unreal.GameplayStatics.get_player_pawn(world,0);pc=unreal.GameplayStatics.get_player_controller(world,0)
    st.update(world=world,pawn=pawn,pc=pc,ui=pc.get_hud().get_editor_property('screen'),game=pawn.get_component_by_class(unreal.HearthwardGameplayComponent))
    for key,cls in [('save',unreal.HearthwardSaveSubsystem),('nature',unreal.HearthwardNatureSubsystem)]:
        st[key]=next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==world)
    unreal.GameplayStatics.set_game_paused(world,False)
    unreal.SystemLibrary.execute_console_command(world,'Hearthward.Companion.CreateTest',pc);yield delay(.5)
    st['game'].enable_adventure()
    for i,a in enumerate(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Actor)):
        if a.get_component_by_class(unreal.HearthwardCombatTargetComponent):a.set_actor_location(unreal.Vector(150000+i*1000,150000,100),False,True)
    st['game'].order_companion('wait')
    brother=unreal.GameplayStatics.get_actor_of_class(world,unreal.HearthwardCompanionFixture)
    brother.set_actor_location(pawn.get_actor_location()+unreal.Vector(150,0,0),False,True)
    yield wait(lambda:brother.get_component_by_class(unreal.CharacterMovementComponent).get_editor_property('movement_mode')==unreal.MovementMode.MOVE_WALKING,8)
    yield delay(.3)
    check('isolated prototype saves',st['save'].enable_prototype())
    # A reflection failure here is a recorded blocker; no substitute setter is invented.
    initial=st['save'].get_editor_property('initial_world')
    st['base_nature']=json.loads(initial.get_editor_property('nature'))
    st['base_camp']=json.loads(initial.get_editor_property('camp_economy'))
    check('actual nature seed exists',st['base_nature']['seed']!=0)
    report['reflection_fields']=['SaveSubsystem.InitialWorld','WorldSave.Nature','WorldSave.CalendarMinutes','WorldSave.CampEconomy','WorldSave.PlayerItems']
    st['game'].set_component_tick_enabled(False)
    for label,fixture,marker in [
        ('crop-half',lambda:crop(1540),'成长'),
        ('crop-mature-full-bag',lambda:crop(2980),'成长'),
        ('juvenile-quarter',lambda:livestock('hen',True),'幼年成长'),
        ('goat-fed-pen',lambda:livestock('goat'),'级栏舍'),
        ('pig-pen',lambda:livestock('pig'),'级栏舍')]:
        value,target=fixture();install(value,100 if label=='crop-mature-full-bag' else 0)
        st['ui'].open_nature(guid(target));yield delay(.3)
        check(label+' normal OpenNature entry',str(st['ui'].get_page())=='nature')
        text=observe(label,marker)
        if label=='crop-half':check(label+' authority displayed','50%' in text and '1440 W分钟' in text)
        elif label=='crop-mature-full-bag':check(label+' authority displayed','100%' in text and '容量不足' in text)
        elif label=='juvenile-quarter':check(label+' authority displayed','25%' in text and '2160 W分钟' in text)
        elif label=='goat-fed-pen':
            check(label+' authority displayed','1200 W分钟' in text)
            check(label+' maximum nine controls',len(report['screens'][label]['detail_buttons'])==9)
        else:check(label+' no invented product','普通产物' not in text and '产出剩余' not in text)
        yield wait(lambda:(out/(label+'.png')).exists() and (out/(label+'.png')).stat().st_mtime>=st['shot_started'][label],10)
        check(label+' rendered PNG exists',(out/(label+'.png')).stat().st_size>0)
        with (out/(label+'.png')).open('rb') as capture:header=capture.read(24)
        report['screens'][label]['capture_dimensions']=list(struct.unpack('>II',header[16:24]))

flow=run();pending=None;deadline=time.monotonic()+120
def tick(dt):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('overall presentation fixture timeout')
        if pending:
            if time.monotonic()>pending[1]:raise TimeoutError('stage '+str(list(report['checks'])[-3:]))
            if not pending[0]():return
        pending=next(flow)
    except StopIteration:finish()
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
