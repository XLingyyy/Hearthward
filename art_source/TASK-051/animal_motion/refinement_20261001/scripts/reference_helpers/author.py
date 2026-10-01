"""Author, bake and export the complete per-species approved clip library.

Uses contact-controlled IK for feet, anatomical body/neck/ear/tail motion,
quintic easing, travelling body waves, and fixed game roots. All constraints
are baked into deformation bones; the game needs no Blender runtime.
"""
import bpy,json,sys,math,re,hashlib,traceback
from pathlib import Path
from mathutils import Vector,Matrix,Quaternion

HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
from rigs import normalize,make_land,make_fish,rotate_bone

ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
TAU=2*math.pi

def smooth(x):
    x=max(0,min(1,x));return x*x*x*(10+x*(-15+6*x))
def pulse(t):return math.sin(math.pi*max(0,min(1,t)))**2
def duration(value):
    ns=re.findall(r'\d+(?:\.\d+)?',value)
    if not ns:return 1.0
    return sum(map(float,ns))/len(ns)

def plan(job):
    result=[]
    for row in job['action_rows']:
        names=re.findall(r'`(AN_[A-Za-z0-9_]+)`',row[0])
        segments=row[2].split('/')
        for i,name in enumerate(names):
            suffix=name[len('AN_'+job['slug']+'_'):]
            looping=(row[3]=='循环' or ('进入/循环/退出' in row[3] and i==1))
            hold=row[3]=='保持' or suffix.endswith('_Hold')
            t=duration(segments[i] if len(segments)==len(names) else row[2])
            fps=60 if any(s in suffix for s in ['Bound','Gallop','Run','Flutter','Hit','Strike','Bite','Ram','Swipe','Burst','Turn','Start','Stop','Lope','Pounce','Takeoff','Flight','Fast','Land','Collapse']) else 30
            n=max(2,round(t*fps));t=n/fps
            result.append({'name':name,'suffix':suffix,'duration':t,'fps':fps,'frames':n+1,'loop':looping,'hold':hold,'priority':row[4],'description':row[1],'direction':'left' if suffix.endswith('_L') else 'right' if suffix.endswith('_R') else None})
    return result

def translate(arm,name,delta):
    p=arm.pose.bones[name];p.location=p.bone.matrix_local.to_3x3().inverted()@Vector(delta)

def reset(arm):
    for p in arm.pose.bones:
        p.rotation_mode='QUATERNION';p.location=(0,0,0);p.rotation_quaternion=(1,0,0,0);p.scale=(1,1,1)

def classify(s,fish):
    if fish:return 'fish'
    for token,kind in [ ('CorpseHold','corpse'),('Collapse','collapse'),('Hit_','hit'),('DustBath','dust'),('Nest','nest'),('RearSniff','rear'),('Swip','swipe'),('Bite','attack'),('Strike','attack'),('RamShort','attack'),('RamFull','attack'),('LieDown','lie'),('CurlDown','lie'),('GetUp','up'),('Rest','rest'),('Graze','forage'),('Nibble','forage'),('Eat_','forage'),('Peck','peck'),('Scratch','scratch'),('Preen','preen'),('Groom','groom'),('Shake','shake'),('Chew','chew'),('Howl','howl'),('Dig','dig'),('Pounce','pounce'),('Takeoff','takeoff'),('Flight','flight'),('Land','land'),('Slope','slope'),('Turn','turn'),('Start','start'),('Stop','stop'),('Gallop','run'),('BoundFast','run'),('Run','run'),('Flutter','run'),('Lope','run'),('Trot','trot'),('HopSlow','hop'),('Walk','walk'),('Alert','alert'),('Look','look'),('Scan','look'),('Sniff','sniff'),('Idle','idle'),('CareStand','idle') ]:
        if token in s:return kind
    raise ValueError('No anatomical recipe for '+s)

def gait_settings(job,kind,clip,rig):
    slug=job['slug'];bird=job['rig_type']=='avian'
    if kind=='walk':duty=.68;phases={'BL':0,'FL':.25,'BR':.5,'FR':.75};speed=1.0;lift=.038
    elif kind=='trot':duty=.50;phases={'FL':0,'BR':0,'FR':.5,'BL':.5};speed=3.0 if 'Follow' in clip['suffix'] else 1.6;lift=.065
    elif kind=='hop' or slug=='hare':
        duty=.26 if kind=='hop' else .10;phases={'BL':0,'BR':.035,'FL':.51,'FR':.555};speed=1.0 if kind=='hop' else 6.0;lift=.065 if kind=='hop' else .095
    elif bird:
        duty=.10;phases={'FL':0,'FR':.5};speed=5.0 if slug=='pheasant' else 4.0 if 'Escape' in clip['suffix'] else 3.0;lift=.048
    else:
        duty=.18 if slug=='black_bear' else .12;phases={'BL':0,'BR':.10,'FL':.53,'FR':.64};speed={'stag_a':7,'goat':4,'pig':4 if 'Domestic' in clip['suffix'] else 7,'wolf':8,'black_bear':6,'ram':6,'red_fox':7}.get(slug,4);lift=.11 if slug=='black_bear' else .08
    if bird and kind=='walk':phases={'FL':0,'FR':.5};duty=.65
    length=rig['axis']['target_length_m'];lift*=length/(1.6 if not bird and slug!='hare' else .65)
    desired=speed*clip['duration']*duty
    # Maximum excursion is bounded by each animal's measured chain length.
    lengths=[]
    for k,ns in rig['chains'].items():
        a=Vector(rig['bone_heads'][ns[0]]);b=Vector(rig['restfeet'][k]);lengths.append((a-b).length)
    safe_stride=min(lengths)*.85
    cycles=max(1,math.ceil(desired/safe_stride)) if kind in ('walk','trot') else 1
    stride=min(desired/cycles,safe_stride)
    effective_speed=stride*cycles/(clip['duration']*duty)
    return duty,phases,stride,lift,effective_speed,cycles

def foot_x(phase,duty,stride):
    """C1 stance/swing trajectory, including continuous backwards contact speed."""
    if phase<duty:return stride*(.5-phase/duty),0.0,True
    u=(phase-duty)/(1-duty)
    # A quintic return plus compact endpoint tangents is C2 at lift/landing and
    # avoids the large overshoot of a single Hermite curve at high running speeds.
    m=-stride*(1-duty)/duty
    r=.055
    x=-stride/2+stride*smooth(u)+m*(u*math.exp(-(u/r)**2)-(1-u)*math.exp(-((1-u)/r)**2))
    return x,math.sin(math.pi*u)**2,False

def land_pose(arm,rig,targets,job,clip,t):
    s=clip['suffix'];kind=clip['kind'];length=rig['axis']['target_length_m'];side=1 if clip['direction']=='left' else -1
    env=pulse(t);body_pitch=0;body_roll=0;body_yaw=0;body_drop=.045*length if job['slug']=='hare' else 0;head_pitch=0;head_yaw=0;neck_pitch=0;tail_amp=2
    gait=kind in ('walk','trot','run','hop','turn','start','stop','slope','takeoff','land','pounce')
    periodic=clip['loop'];breath=math.sin(TAU*t) if periodic else math.sin(TAU*t)*env
    body_z=.0035*length*breath
    if kind in ('alert','look','sniff','chew','howl','groom','preen','peck','forage'):
        head_yaw=(5*math.sin(TAU*t) if kind=='alert' else 32*math.sin(TAU*t)*env if kind=='look' else 0)
    if kind=='alert':head_pitch=-8;neck_pitch=-5;tail_amp=.5
    if kind=='alert' and job['slug']=='ram':head_pitch=12;neck_pitch=24
    if kind=='flight':body_pitch=-10;neck_pitch=8;body_z=.12*length+.012*length*math.sin(TAU*t*2)
    if kind=='forage':
        a=1 if s.endswith('_Loop') else smooth(t) if s.endswith('_In') else 1-smooth(t)
        neck_pitch=({'stag_a':118,'goat':65,'ram':104,'hare':52,'pig':52}.get(job['slug'],55)) *a
        head_pitch=8*a+2*a*breath;body_drop=(.16 if job['slug']=='hare' else .055)*length*a;body_pitch=(22 if job['slug']=='hare' else 7)*a
    if kind=='sniff':neck_pitch=48*env;head_pitch=18*env;head_yaw=8*math.sin(TAU*t)*env;body_drop=.025*length*env
    if kind=='chew':head_pitch=.9*math.sin(3*TAU*t);neck_pitch=2
    if kind=='howl':neck_pitch=-26*env;head_pitch=-14*env;body_pitch=-2*env
    if kind=='peck':
        neck_pitch=(104 if job['slug']=='hen' else 114)*env
        head_pitch=24*env;body_pitch=18*env;body_drop=.065*length*env
    if kind in ('groom','preen'):neck_pitch=32*env;head_yaw=65*env;head_pitch=12*env;body_drop=.025*length*env
    if kind=='scratch':head_pitch=18*env;body_pitch=5*env
    if kind=='shake':body_roll=4*math.sin(4*TAU*t)*env;head_yaw=-5*math.sin(4*TAU*t)*env;tail_amp=5
    if kind in ('lie','up','rest','nest'):
        a=smooth(t) if kind=='lie' or s.endswith('Settle') else 1-smooth(t) if kind=='up' or s.endswith('Exit') else 1
        if kind=='nest' and s.endswith('Rest'):a=1
        body_drop=.285*length*a if job['slug'] not in ('hare','hen','pheasant') else (.18 if job['slug']=='hare' else .16)*length*a
        body_pitch=4*a;neck_pitch=12*a
        if job['slug']=='red_fox':body_yaw=12*a;head_yaw=45*a;tail_amp=0
    if kind=='dust':
        a=1 if s.endswith('_Loop') else smooth(t) if s.endswith('_In') else 1-smooth(t) if s.endswith('_Out') else env
        body_drop=.13*length*a;body_roll=(24+8*math.sin(2*TAU*t))*a;neck_pitch=18*a
    if kind=='rear':
        a=smooth(t) if s.endswith('_In') else 1-smooth(t) if s.endswith('_Out') else 1
        body_pitch=-34*a;body_drop=-.04*length*a;neck_pitch=-12*a
    if kind in ('attack','swipe','dig'):
        if 'Short' in s:
            strike=1-smooth(t);windup=0
        else:
            windup=smooth(t/.28)*(1-smooth((t-.28)/.18));strike=smooth((t-.30)/.12)*(1-smooth((t-.45)/.55))
        neck_pitch=(20 if job['slug']=='ram' else 8)*strike-12*windup
        head_pitch=(10 if job['slug']=='ram' else 12)*strike;body_pitch=5*strike-3*windup
        body_drop=.03*length*strike
    if kind=='hit':
        recoil=math.sin(math.pi*t)*math.exp(-3*t);body_roll=side*7*recoil;body_yaw=side*4*recoil;head_yaw=-side*8*recoil;body_drop=.025*length*recoil
    collapse=kind in ('collapse','corpse')
    if collapse:
        a=1 if kind=='corpse' else smooth(t)
        body_roll=-side*92*smooth((a-.12)/.88);body_drop=.42*length*a;body_pitch=5*a;neck_pitch=8*a;tail_amp=0;body_z=0
    if kind=='turn':body_yaw=side*9*env;head_yaw=side*12*env;tail_amp=4
    if gait:
        gkind='run' if kind in ('run','start','stop','takeoff','land','pounce') else 'walk' if kind in ('turn','slope') else kind
        duty,phases,stride,lift,speed,cycles=gait_settings(job,gkind,clip,rig)
        a=smooth(t) if kind in ('start','takeoff','pounce') else 1-smooth(t) if kind in ('stop','land') else env if kind in ('turn','slope') else 1
        body_z+=(.008 if gkind=='walk' else .015 if gkind=='trot' else .022)*length*math.sin(TAU*t*cycles*2)*a
        body_drop+=(.025 if gkind=='walk' else .045 if gkind=='trot' else .09)*length*a
        body_pitch+=(-2 if gkind=='walk' else 4)*math.sin(TAU*t*cycles)*a
        for k,(ctl,con,eff) in targets.items():
            phase=(t*cycles+phases[k])%1;x,z,contact=foot_x(phase,duty,stride)
            if kind=='turn':x*=.72 if (k.endswith('L') and side==1) or (k.endswith('R') and side==-1) else 1.15
            ctl.location=Vector(rig['restfeet'][k])+Vector((x*a,0,z*lift*a))
            con.influence=1.0
            p=arm.pose.bones[eff]
            # Counter-rotate foot against the solved shank to keep it flat during stance.
            p.rotation_mode='QUATERNION'
        clip['reference_speed_cm_s']=round(speed*100,3);clip['contact_duty']=duty;clip['contact_offsets']=phases;clip['gait_cycles']=cycles
    else:
        for k,(ctl,con,eff) in targets.items():
            ctl.location=Vector(rig['restfeet'][k]);con.influence=1
            if collapse or kind=='flight':con.influence=0
            elif kind in ('rear','swipe','groom','scratch','dig') and k.startswith('F'):
                a=pulse(t) if kind not in ('rear','swipe') else (1 if s.endswith('_Hold') else smooth(t) if s.endswith('_In') else 1-smooth(t) if s.endswith('_Out') else env)
                if kind=='rear':con.influence=1-a
                elif k==('FL' if side==1 or kind in ('groom','scratch','dig') else 'FR'):
                    ctl.location+=Vector((.09*length*a,.04*length*side*a if kind=='swipe' else 0,.13*length*a))
    rotate_bone(arm,rig['body'],(0,1,0),body_pitch)
    # Compose roll/yaw in the body's rest coordinate frame.
    p=arm.pose.bones[rig['body']];q=p.bone.matrix_local.to_quaternion()
    ws=Quaternion((0,0,1),math.radians(body_yaw))@Quaternion((1,0,0),math.radians(body_roll))@Quaternion((0,1,0),math.radians(body_pitch))
    p.rotation_quaternion=q.inverted()@ws@q
    translate(arm,rig['body'],(0,0,body_z-body_drop))
    for n in rig['neck']:rotate_bone(arm,n,(0,1,0),neck_pitch/max(1,len(rig['neck'])))
    if kind in ('run','hop') and job['slug'] in ('hare','wolf','red_fox'):
        for i,n in enumerate(rig['spine'][:3]):rotate_bone(arm,n,(0,1,0),3*math.sin(TAU*t-.45*i))
    h=arm.pose.bones[rig['head']];hq=h.bone.matrix_local.to_quaternion();h.rotation_quaternion@=hq.inverted()@Quaternion((0,0,1),math.radians(head_yaw))@hq
    h=arm.pose.bones[rig['head']];hq=h.bone.matrix_local.to_quaternion();h.rotation_quaternion@=hq.inverted()@Quaternion((0,1,0),math.radians(head_pitch))@hq
    if collapse and job['slug'] in ('stag_a','goat','ram'):
        h.rotation_quaternion@=hq.inverted()@Quaternion((1,0,0),math.radians(side*35*a))@hq
    for i,n in enumerate(rig['tail']):
        rotate_bone(arm,n,(0,0,1),tail_amp*math.sin(TAU*t-.6*i)/max(1,len(rig['tail'])))
        if job['slug']=='red_fox' and i==0:
            q=arm.pose.bones[n].bone.matrix_local.to_quaternion()
            lift=(15 if kind in ('run','start','stop','pounce') else 5 if kind=='trot' else 0)
            arm.pose.bones[n].rotation_quaternion@=q.inverted()@Quaternion((0,1,0),math.radians(lift))@q
    for i,n in enumerate(rig.get('ears',[])):rotate_bone(arm,n,(0,0,1),(4*breath if kind not in ('collapse','corpse') else 0)*(1 if i==0 else -.65))
    for sign,ns in zip((1,-1),rig.get('wings',[])):
        flutter=kind in ('flight','takeoff','land') or 'Flutter' in s
        angle=(22+15*math.sin(TAU*t*(4 if kind=='flight' else 1))) if flutter else 5*math.sin(3*TAU*t)*env if kind=='shake' else 12*env if kind=='dust' else 0
        for i,n in enumerate(ns):rotate_bone(arm,n,(1,0,0),sign*angle/(i+1))
    if collapse:
        a=1 if kind=='corpse' else smooth(t)
        for k,ns in rig['chains'].items():
            gentle=job['slug'] in ('pig','red_fox')
            folds=((28,-36,8) if gentle else (40,-50,10)) if k.startswith('F') else ((-18,40,-15) if gentle else (-25,55,-20))
            for i,n in enumerate(ns[:3]):rotate_bone(arm,n,(0,1,0),folds[i]*a)
    if kind=='flight':
        for ns in rig['chains'].values():
            for i,n in enumerate(ns[:3]):rotate_bone(arm,n,(0,1,0),(20,-38,12)[i])
    return kind

def fish_pose(arm,rig,job,clip,t):
    s=clip['suffix'];eel=job['slug']=='eel';ns=rig['spine'];amp=1.2 if eel else 1.8;env=1.0;roll=0;pitch=0;yaw=0
    if 'Hover' in s:amp*=.30
    elif 'Slow' in s:amp*=.55
    elif 'Fast' in s or 'Burst' in s:amp*=1.7
    elif 'Struggle' in s:amp*=1.6;yaw=6*math.sin(TAU*t)
    elif 'Coast' in s or 'Glide' in s:amp*=.30
    elif 'Brake' in s or 'StopWave' in s:env=1-smooth(t);pitch=7*pulse(t)
    elif 'StartWave' in s:env=smooth(t)
    elif 'Bite' in s or 'Whisker' in s:amp*=.35;pitch=-5*pulse(t)
    elif 'Hooked' in s:yaw=10*pulse(t);pitch=-8*pulse(t)
    elif 'Escape' in s:amp*=1.5;env=1-smooth((t-.5)/.5)
    if 'Turn_' in s:yaw=(1 if s.endswith('_L') else -1)*12*pulse(t)
    if 'Lift' in s:pitch=-22*smooth(t);amp*=.6
    land='Land' in s or 'Settle' in s or 'Display' in s
    if land:
        roll=(0 if eel else 78)+ (5 if eel else 9)*math.sin(TAU*t)
        amp*=1.3
        if 'Settle' in s:env=1-smooth(t);roll=(0 if eel else 78)+9*math.sin(TAU*t)*env
        if 'Display' in s:env=0;roll=0 if eel else 78
    p=arm.pose.bones[rig['body']];q=p.bone.matrix_local.to_quaternion()
    ws=Quaternion((0,0,1),math.radians(yaw))@Quaternion((1,0,0),math.radians(roll))@Quaternion((0,1,0),math.radians(pitch))
    p.rotation_quaternion=q.inverted()@ws@q
    for i,n in enumerate(ns):
        u=(i+.5)/len(ns);a=amp*env*(.35+.95*u*u)*math.sin(TAU*(t-(1.5 if eel else .8)*u))
        rotate_bone(arm,n,(0,0,1),a)
    for i,n in enumerate(rig.get('fins',[])):rotate_bone(arm,n,(1,0,0),(1 if i==0 else -1)*4*math.sin(TAU*t+.45*i)*env)
    for side,chain in enumerate(rig.get('barbels',[])):
        for i,n in enumerate(chain):rotate_bone(arm,n,(0,0,1),(3 if 'Whisker' in s else .8)*math.sin(TAU*t-.4*i+.8*side)*env)
    clip['wave_cycles']=1;clip['phase_progression_cycles']=1.5 if eel else .8
    return land

def evaluated_min_z(meshes):
    dg=bpy.context.evaluated_depsgraph_get()
    low=float('inf')
    for o in meshes:
        ev=o.evaluated_get(dg);data=ev.to_mesh()
        low=min(low,min((ev.matrix_world@v.co).z for v in data.vertices));ev.to_mesh_clear()
    return low

def curves(action):
    if hasattr(action,'fcurves'):return list(action.fcurves)
    return [f for layer in action.layers for strip in layer.strips for bag in strip.channelbags for f in bag.fcurves]

def write_rig_report(arm,meshes,rig,out):
    rig['bone_heads']={b.name:list(b.head_local) for b in arm.data.bones}
    rig['bone_count']=len(arm.data.bones)
    rig['skeleton_signature']=hashlib.sha256(json.dumps([{'name':b.name,'parent':b.parent.name if b.parent else None,'matrix':[list(row) for row in b.matrix_local]} for b in arm.data.bones],sort_keys=True).encode()).hexdigest()
    counts={g.name:0 for m in meshes for g in m.vertex_groups}
    error=0;unweighted=0;max_inf=0
    for m in meshes:
        names={g.index:g.name for g in m.vertex_groups}
        for v in m.data.vertices:
            gs=[g for g in v.groups if g.weight>1e-5]
            total=sum(g.weight for g in gs);error=max(error,abs(1-total));unweighted+=int(total==0);max_inf=max(max_inf,len(gs))
            for g in gs:counts[names[g.group]]+=1
    rig['skin_qa']={'unweighted_vertices':unweighted,'max_sum_error':error,'max_influences':max_inf,'weighted_vertices_per_bone':counts}
    (out/'rig_update.json').write_text(json.dumps(rig,ensure_ascii=False,indent=2),encoding='utf-8')

def bind_proxy(arm):
    name='BindPoseProxy_'+arm.name
    existing=bpy.data.objects.get(name)
    if existing:return existing
    vertices=[];faces=[]
    for b in arm.data.bones:
        i=len(vertices);p=b.head_local
        vertices.extend([p+Vector((0,0,0)),p+Vector((.0001,0,0)),p+Vector((0,.0001,0))]);faces.append((i,i+1,i+2))
    data=bpy.data.meshes.new(name);data.from_pydata(vertices,[],faces);obj=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(obj)
    for i,b in enumerate(arm.data.bones):obj.vertex_groups.new(name=b.name).add([3*i,3*i+1,3*i+2],1.0,'REPLACE')
    mod=obj.modifiers.new('BindOnly','ARMATURE');mod.object=arm
    obj.hide_render=True
    return obj

def export_fbx(arm,meshes,path,geometry=False,animation=True):
    # UE strips Blender's Armature object, including an object-level unit scale.
    # Export actual centimetre coordinates with identity object/root scales.
    # Work on copies so editable scenes and their actions stay in metres.
    scene=bpy.context.scene;before=set(bpy.data.objects);old_name=arm.name
    unit_system=scene.unit_settings.system;unit_scale=scene.unit_settings.scale_length
    arm.name='_FBXSource_'+old_name;copy=arm.copy();copy.data=arm.data.copy()
    bpy.context.collection.objects.link(copy);copy.name='Armature';copy.data.transform(Matrix.Scale(100,4))
    copied_action=None;copied_meshes=[]
    try:
        scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.01
        if arm.animation_data and arm.animation_data.action:
            copied_action=arm.animation_data.action.copy();copy.animation_data.action=copied_action
            for fc in curves(copied_action):
                if fc.data_path.endswith('location'):
                    for k in fc.keyframe_points:
                        k.co.y*=100;k.handle_left.y*=100;k.handle_right.y*=100
        for p in copy.pose.bones:p.location*=100
        if geometry:
            for obj in meshes:
                m=obj.copy();m.data=obj.data.copy();bpy.context.collection.objects.link(m)
                m.data.transform(Matrix.Scale(100,4));m.parent=None;m.matrix_world=Matrix.Identity(4)
                for modifier in m.modifiers:
                    if modifier.type=='ARMATURE':modifier.object=copy
                copied_meshes.append(m)
        bpy.ops.object.select_all(action='DESELECT');copy.select_set(True)
        for obj in copied_meshes:obj.select_set(True)
        if animation and not geometry:bind_proxy(copy).select_set(True)
        bpy.context.view_layer.objects.active=copy
        scene.frame_set(scene.frame_current);bpy.context.view_layer.update()
        bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'ARMATURE','MESH'} if geometry or animation else {'ARMATURE'},
            use_mesh_modifiers=True,mesh_smooth_type='FACE',add_leaf_bones=False,use_armature_deform_only=False,
            axis_forward='X',axis_up='Z',apply_scale_options='FBX_SCALE_ALL',path_mode='COPY',embed_textures=geometry,
            bake_anim=animation,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,
            bake_anim_force_startend_keying=True,bake_anim_step=1,bake_anim_simplify_factor=0)
    finally:
        for obj in set(bpy.data.objects)-before:
            data=obj.data;bpy.data.objects.remove(obj,do_unlink=True)
            if isinstance(data,bpy.types.Mesh):bpy.data.meshes.remove(data)
            elif isinstance(data,bpy.types.Armature):bpy.data.armatures.remove(data)
        if copied_action:bpy.data.actions.remove(copied_action)
        arm.name=old_name;scene.unit_settings.system=unit_system;scene.unit_settings.scale_length=unit_scale
        bpy.context.view_layer.objects.active=arm

def author(job):
    out=Path(job['output']);bpy.ops.wm.open_mainfile(filepath=str(out/'source_import.blend'))
    arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    axis=normalize(arm,meshes,job)
    fish=job['rig_type'] in ('aquatic','serpentine')
    if fish:arm,rig=make_fish(arm,meshes,job);targets={}
    else:rig,targets=make_land(arm,meshes,job)
    rig['axis']=axis;arm.name='Armature';arm.data.name='SKEL_'+job['slug']
    write_rig_report(arm,meshes,rig,out)
    reset(arm);bpy.context.view_layer.update()
    export_fbx(arm,meshes,out/('SK_'+job['slug']+'.fbx'),True,False)
    clips=plan(job);(out/'clips').mkdir(exist_ok=True)
    for clip in clips:
        clip['kind']=classify(clip['suffix'],fish)
        if not fish and clip['kind'] in ('walk','trot','hop','slope'):
            clip['fps']=60;clip['frames']=round(clip['duration']*60)+1;clip['duration']=(clip['frames']-1)/60
        scene=bpy.context.scene;scene.render.fps=clip['fps'];scene.frame_start=1;scene.frame_end=clip['frames']
        arm.animation_data_create();act=bpy.data.actions.new(clip['name']);arm.animation_data.action=act;act.use_fake_user=True
        for k,v in [('fps',clip['fps']),('loop',clip['loop']),('description',clip['description'])]:act[k]=v
        matrices=[];qa={'max_ground_penetration_m':0.0,'root_translation_drift_m':0.0,'root_rotation_drift_deg':0.0,'max_adjacent_rotation_deg':0.0}
        previous=None;floor_offsets=[];samples=[]
        for frame in range(1,clip['frames']+1):
            t=(frame-1)/(clip['frames']-1);scene.frame_set(frame);reset(arm)
            for ctl,con,eff in targets.values():con.mute=False
            if fish:ground=fish_pose(arm,rig,job,clip,t)
            else:ground=land_pose(arm,rig,targets,job,clip,t) in ('collapse','corpse','rest','lie','up','dust','nest')
            bpy.context.view_layer.update()
            if not fish:
                # Maintain a flat planted paw/hoof against the solved shank.
                for k,(ctl,con,eff) in targets.items():
                    p=arm.pose.bones[eff];m=p.matrix.copy()
                    rotation=m.to_quaternion().slerp(arm.data.bones[eff].matrix_local.to_quaternion(),con.influence)
                    desired=rotation.to_matrix().to_4x4();desired.translation=m.translation
                    p.matrix=desired
                bpy.context.view_layer.update()
            floor=0
            if ground:
                minz=evaluated_min_z(meshes)
                floor=max(0,-minz+.0003)
                if floor:translate(arm,rig['body'],Vector((0,0,floor))+arm.pose.bones[rig['body']].bone.matrix_local.to_3x3()@arm.pose.bones[rig['body']].location);bpy.context.view_layer.update()
            floor_offsets.append(floor)
            world={p.name:p.matrix.copy() for p in arm.pose.bones}
            basis={p.name:p.bone.convert_local_to_pose(world[p.name],p.bone.matrix_local,parent_matrix=world[p.parent.name] if p.parent else Matrix.Identity(4),parent_matrix_local=p.parent.bone.matrix_local if p.parent else Matrix.Identity(4),invert=True) for p in arm.pose.bones}
            matrices.append(basis)
            if frame in (1,1+round((clip['frames']-1)/4),1+round((clip['frames']-1)/2),clip['frames']):samples.append({'frame':frame,'feet':{k:list(world[eff].translation) for k,(_,_,eff) in targets.items()},'ground_min_z_m':evaluated_min_z(meshes) if not fish or ground else None})
            # Disable constraints while writing already solved deformation transforms.
            for ctl,con,eff in targets.values():con.mute=True
            for p in arm.pose.bones:
                loc,rot,scale=basis[p.name].decompose()
                if previous and previous[p.name][1].dot(rot)<0:rot.negate()
                if previous:
                    delta=math.degrees(previous[p.name][1].rotation_difference(rot).angle)
                    if delta>qa['max_adjacent_rotation_deg']:qa['max_adjacent_rotation_deg']=delta;qa['largest_rotation_bone']=p.name;qa['largest_rotation_frame']=frame
                p.location=loc;p.rotation_quaternion=rot;p.scale=(1,1,1)
                p.keyframe_insert('location',frame=frame,group=p.name);p.keyframe_insert('rotation_quaternion',frame=frame,group=p.name)
            # Re-evaluate the actual constraint-free deformation. Contact must
            # survive baking rather than only be correct in the IK preview.
            bpy.context.view_layer.update()
            if ground:
                correction=.0003-evaluated_min_z(meshes)
                p=arm.pose.bones[rig['body']]
                p.location+=p.bone.matrix_local.to_3x3().inverted()@Vector((0,0,correction))
                p.keyframe_insert('location',frame=frame,group=p.name)
                bpy.context.view_layer.update()
            previous={p.name:(p.location.copy(),p.rotation_quaternion.copy()) for p in arm.pose.bones}
        for fc in curves(act):
            for k in fc.keyframe_points:k.interpolation='LINEAR'
        # Fixed root, loop seam, and endpoint velocity comparison on baked channels.
        first,last=matrices[0],matrices[-1]
        qa['root_translation_drift_m']=(first[rig['root']].translation-last[rig['root']].translation).length
        qa['root_rotation_drift_deg']=math.degrees(first[rig['root']].to_quaternion().rotation_difference(last[rig['root']].to_quaternion()).angle)
        if clip['loop']:
            qa['loop_pose_error_deg']=max(math.degrees(first[n].to_quaternion().rotation_difference(last[n].to_quaternion()).angle) for n in first)
            qa['loop_position_error_m']=max((first[n].translation-last[n].translation).length for n in first)
        qa['floor_correction_max_m']=max(floor_offsets);qa['sampled_poses']=samples
        zvalues=[x['ground_min_z_m'] for x in samples if x['ground_min_z_m'] is not None]
        if zvalues:qa['max_ground_penetration_m']=max(0,-min(zvalues))
        clip['qa']=qa;clip['skeleton_signature']=rig['skeleton_signature'];clip['events']={'gameplay_notifies':[],'damage_authority':'existing gameplay only'}
        export_fbx(arm,meshes,out/'clips'/(clip['name']+'.fbx'),False,True)
        clip['file']=str(out/'clips'/(clip['name']+'.fbx'));clip['sha256']=hashlib.sha256(Path(clip['file']).read_bytes()).hexdigest()
        print('CLIP '+job['slug']+' '+clip['suffix'],flush=True)
        (out/'animation_manifest.json').write_text(json.dumps({'schema':'hearthward.animal.animations.v1','name':job['name'],'slug':job['slug'],'source_sha256':job['source_sha256'],'guidance_sha256':job['guidance_sha256'],'rig':rig,'clips':clips[:clips.index(clip)+1],'status':'authoring','visual_qa':'pending','ue_import':'NOT_RUN'},ensure_ascii=False,indent=2),encoding='utf-8')
    for ctl,con,eff in targets.values():
        for p in arm.pose.bones:
            if con in list(p.constraints):p.constraints.remove(con)
        bpy.data.objects.remove(ctl,do_unlink=True)
    for o in list(bpy.data.objects):
        if o.name.startswith('IKPole_') or o.name.startswith('BindPoseProxy_'):bpy.data.objects.remove(o,do_unlink=True)
    arm.animation_data.action=bpy.data.actions[clips[0]['name']];bpy.context.scene.render.fps=clips[0]['fps'];bpy.context.scene.frame_start=1;bpy.context.scene.frame_end=clips[0]['frames'];bpy.context.scene.frame_set(1)
    bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(out/('AS_'+job['slug']+'.blend')))
    manifest=json.loads((out/'animation_manifest.json').read_text(encoding='utf-8'));manifest['status']='authored';manifest['provenance']={'mesh':'existing user Tripo source','motion':'species-specific authored curves and contact IK','cloud_motion':'cloud_motion.fbx is retained as an unapproved reference; not substituted for authored action coverage','license':'User-supplied/generated model; original authored motion, no third-party mocap'}
    (out/'animation_manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    print('AUTHORED '+job['slug']+' '+str(len(clips))+' clips',flush=True)

def main():
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    jobs=json.loads((ROOT/'jobs.json').read_text(encoding='utf-8'))
    if args:jobs=[j for j in jobs if j['slug'] in args]
    for j in jobs:
        try:author(j)
        except Exception:
            (Path(j['output'])/'author_error.txt').write_text(traceback.format_exc(),encoding='utf-8');raise

if __name__=='__main__':main()
