"""Create task-owned in-place clips and keyed jump/dig poses on the existing rig.

Existing Tripo packages remain read-only. New actions are authored locally;
they are not mocap or cloud-generated animation.
"""
import json
import math
from pathlib import Path
import unreal

DEST = '/Game/Characters/Hero/Animation'
SOURCE = '/Game/Characters/Hero/Tripo'
tools = unreal.AssetToolsHelpers.get_asset_tools()
unreal.EditorAssetLibrary.make_directory(DEST)
idle = unreal.load_asset(SOURCE + '/A_Hero_Tripo_Actionsidle')
skeleton = idle.get_editor_property('skeleton')
mesh = unreal.load_asset(SOURCE + '/SK_Hero_Tripo')
actor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector())
component = actor.skeletal_mesh_component
component.set_skeletal_mesh_asset(mesh)
bones = [str(component.get_bone_name(i)) for i in range(component.get_num_bones())]
parents = {b: str(component.get_parent_bone(b)) for b in bones}
base = {b: unreal.AnimationLibrary.get_bone_pose_for_time(idle, b, 0., False) for b in bones}

def mul(a, b):
    x,y,z,w=a; X,Y,Z,W=b
    return (w*X+x*W+y*Z-z*Y, w*Y-x*Z+y*W+z*X, w*Z+x*Y-y*X+z*W, w*W-x*X-y*Y-z*Z)

def quat(q): return (q.x,q.y,q.z,q.w)
def inv(q): return (-q[0],-q[1],-q[2],q[3])
def rotation_y(degrees):
    a=math.radians(degrees)*.5
    return (0.,math.sin(a),0.,math.cos(a))

world_rot = {}
for b in bones:
    p=parents[b]
    world_rot[b]=mul(world_rot.get(p,(0,0,0,1)),quat(base[b].rotation))

def create(name, duration, pose_at, fps=30):
    path=DEST+'/A_Hero_'+name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        seq=unreal.load_asset(path)
    else:
        factory=unreal.AnimSequenceFactory()
        factory.set_editor_property('target_skeleton', skeleton)
        seq=tools.create_asset('A_Hero_'+name, DEST, unreal.AnimSequence, factory)
    controller=seq.get_editor_property('controller')
    controller.open_bracket('TASK-027 '+name, False)
    controller.remove_all_bone_tracks(False)
    controller.set_frame_rate(unreal.FrameRate(fps,1),False)
    frames=round(duration*fps)
    controller.set_number_of_frames(unreal.FrameNumber(frames),False)
    poses=[pose_at(i/frames) for i in range(frames+1)]
    if name not in ('JumpStart','Fall'):
        for pose in poses:
            points=positions(pose)
            delta=min(points[b][2] for b in ankles)-standing_ankle_height
            root=pose['Root']; p=root.translation
            root.translation=unreal.Vector(p.x,p.y,p.z-delta)
            pose['Root']=root
    for b in bones:
        controller.add_bone_curve(b,False)
        values=[p[b] for p in poses]
        controller.set_bone_track_keys(b,[v.translation for v in values],[v.rotation for v in values],[v.scale3d for v in values],False)
    controller.close_bracket(False)
    seq.set_editor_property('force_root_lock',False)
    unreal.EditorAssetLibrary.save_loaded_asset(seq)
    return {'path':path,'duration':seq.get_play_length(),'frames':frames,'bones':len(bones)}

def sample(source, t):
    result={b:unreal.AnimationLibrary.get_bone_pose_for_time(source,b,t,False) for b in bones}
    root=result['Root']
    origin=base['Root'].translation
    root.translation=unreal.Vector(origin.x,origin.y,root.translation.z)
    result['Root']=root
    return result

def keyed_pose(angles, drop=0.):
    result={}
    for b in bones:
        value=base[b]
        position=value.translation
        if b=='Root': position=unreal.Vector(position.x,position.y,position.z-drop)
        q=quat(value.rotation)
        if b in angles:
            parent=world_rot.get(parents[b],(0,0,0,1))
            q=mul(mul(mul(inv(parent),rotation_y(angles[b])),parent),q)
        transform=unreal.Transform()
        transform.translation=position
        transform.rotation=unreal.Quat(*q)
        transform.scale3d=value.scale3d
        result[b]=transform
    return result

def interpolate(keys,t):
    for i in range(1,len(keys)):
        if t<=keys[i][0]:
            a,ap,ad=keys[i-1]; b,bp,bd=keys[i]
            u=(t-a)/(b-a); u=u*u*(3.-2.*u)
            return keyed_pose({bone:ap.get(bone,0)*(1-u)+bp.get(bone,0)*u for bone in set(ap)|set(bp)},ad*(1-u)+bd*u)
    return keyed_pose(keys[-1][1],keys[-1][2])

def rotate(q, v):
    return mul(mul(q,(v[0],v[1],v[2],0.)),inv(q))[:3]

def positions(pose):
    points={}; rotations={}; scales={}
    for bone in bones:
        parent=parents[bone]; value=pose[bone]
        pq=rotations.get(parent,(0.,0.,0.,1.)); pp=points.get(parent,(0.,0.,0.))
        ps=scales.get(parent,(1.,1.,1.))
        offset=rotate(pq,(value.translation.x*ps[0],value.translation.y*ps[1],value.translation.z*ps[2]))
        points[bone]=tuple(pp[i]+offset[i] for i in range(3))
        rotations[bone]=mul(pq,quat(value.rotation))
        scales[bone]=(ps[0]*value.scale3d.x,ps[1]*value.scale3d.y,ps[2]*value.scale3d.z)
    return points

reference_positions=positions(base)
ankles=['0_Right_Limb_1','1_Left_Limb_1']
standing_ankle_height=min(component.get_socket_location(b).z for b in ankles)
report=[]
# Idle, Walk and Sprint are published by retarget_template.py.

# Anatomical mapping is based on the measured joint hierarchy, not Tripo labels.
# bone_17/bone_21 are thighs; 0_Right_Limb_0/1_Left_Limb_0 are knees.
squat={'bone_17':-32,'bone_21':-32,'0_Right_Limb_0':65,'1_Left_Limb_0':65,
       '0_Right_Limb_1':-33,'1_Left_Limb_1':-33,'Spine_0':15,
       'bone_6':20,'0_Left_Limb_2':20}
air={'bone_17':-24,'bone_21':-12,'0_Right_Limb_0':48,'1_Left_Limb_0':32,
     '0_Right_Limb_1':-12,'1_Left_Limb_1':-12,'Spine_0':5,
     'bone_6':-48,'0_Left_Limb_2':-48,'bone_7':-28,'0_Left_Limb_3':-28}
fall={'bone_17':-8,'bone_21':-8,'0_Right_Limb_0':18,'1_Left_Limb_0':18,
      'bone_6':-30,'0_Left_Limb_2':-30,'bone_7':-20,'0_Left_Limb_3':-20}
dig_ready={'Spine_0':12,'Spine_1':5,'bone_6':-65,'0_Left_Limb_2':-65,
           'bone_7':-60,'0_Left_Limb_3':-60,'bone_17':-12,'bone_21':-12,
           '0_Right_Limb_0':24,'1_Left_Limb_0':24}
dig_down={'Spine_0':30,'Spine_1':12,'bone_6':-28,'0_Left_Limb_2':-28,
          'bone_7':-10,'0_Left_Limb_3':-10,'bone_17':-18,'bone_21':-18,
          '0_Right_Limb_0':36,'1_Left_Limb_0':36}
attack_ready={'Spine_0':-6,'Spine_1':-5,'bone_6':-115,'bone_7':-50,
              '0_Left_Limb_2':-20,'0_Left_Limb_3':-35,'bone_17':-6,'0_Right_Limb_0':12}
attack_strike={'Spine_0':18,'Spine_1':8,'bone_6':-32,'bone_7':-8,
               '0_Left_Limb_2':-12,'0_Left_Limb_3':-25,'bone_17':-12,'0_Right_Limb_0':24}
for name,duration,keys in [
    ('JumpStart',.28,[(0.,squat,5.),(.5,{},0.),(1.,air,0.)]),
    ('Fall',.6,[(0.,fall,0.),(1.,fall,0.)]),
    ('Land',.24,[(0.,fall,0.),(.35,squat,5.),(1.,{},0.)]),
    ('Attack',.7,[(0.,{},0.),(.3,attack_ready,0.),(.48,attack_strike,1.),(.64,attack_strike,1.),(1.,{},0.)]),
    ('Dig',1.25,[(0.,dig_ready,1.),(.3,dig_ready,1.),(.48,dig_down,3.),(.62,dig_down,3.),(1.,dig_ready,1.)])]:
    report.append(create(name,duration,lambda t,k=keys:interpolate(k,t)))
out=Path(unreal.Paths.project_saved_dir())/'Task027/authored-motion.json'
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(report,indent=2),encoding='utf-8')
(out.parent/'foot-alignment.json').write_text(json.dumps({'reference_ankle_height':standing_ankle_height,'source_idle_ankle_height':min(reference_positions[b][2] for b in ankles),'source_root_scale':str(base['Root'].scale3d)},indent=2),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
