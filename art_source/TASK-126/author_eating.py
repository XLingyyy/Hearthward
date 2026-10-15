"""Original hand-to-mouth contact pass on the unchanged contact rig, via Blender MCP."""
import bpy,math,json,sys
from pathlib import Path
from mathutils import Matrix,Vector
root=Path('G:/GameFactory/Hearthward');sys.path.append(str(root.parent))
from operators.gen_motion.funcs.retarget_utils import world_delta as w
target=bpy.data.objects['Armature'];role=Path(bpy.data.filepath).stem.split('-')[0];scene=bpy.context.scene
target.animation_data.action=bpy.data.actions['A_'+role+'_Operate'];scene.frame_set(1)
base={p.name:(p.location.copy(),p.rotation_quaternion.copy(),p.scale.copy()) for p in target.pose.bones}
def aim(bone,child,point):
    p=target.pose.bones[bone];direction=target.pose.bones[child].head-p.head
    rotation=direction.normalized().rotation_difference((point-p.head).normalized());mat=p.matrix.copy()
    p.matrix=Matrix.Translation(mat.translation)@rotation.to_matrix().to_4x4()@Matrix.Translation(-mat.translation)@mat;bpy.context.view_layer.update()
def hand(point):
    shoulder=target.pose.bones['upperarm_r'].head.copy();elbow=target.pose.bones['lowerarm_r'].head.copy();wrist=target.pose.bones['hand_r'].head.copy()
    a=(elbow-shoulder).length;b=(wrist-elbow).length;d=point-shoulder;length=min(a+b-.003,max(abs(a-b)+.003,d.length));axis=d.normalized()
    pole=Vector((-.3,-.08,-.1));pole=(pole-axis*pole.dot(axis)).normalized();along=(a*a+length*length-b*b)/(2*length)
    aim('upperarm_r','lowerarm_r',shoulder+axis*along+pole*math.sqrt(max(0,a*a-along*along)))
    aim('lowerarm_r','hand_r',shoulder+axis*length)
rows=[]
for frame in range(91):
    for name,(loc,q,scale) in base.items():p=target.pose.bones[name];p.location=loc;p.rotation_quaternion=q;p.scale=scale
    bpy.context.view_layer.update();t=frame/30;f=min(1,t/.65,(3-t)/.7);f=max(0,f);f=f*f*(3-2*f)
    wrist=target.pose.bones['hand_r'].head.copy();head=target.pose.bones['head'].head.copy();point=head+Vector((-.025,-.085,-.055+math.sin(t*7)*.005))
    hand(wrist.lerp(point,f));rows.append({p.name:(p.location.copy(),p.rotation_quaternion.copy(),p.scale.copy()) for p in target.pose.bones})
name='A_'+role+'_Eat'
if bpy.data.actions.get(name):bpy.data.actions.remove(bpy.data.actions[name])
action=bpy.data.actions.new(name);action.use_fake_user=True;target.animation_data.action=action;previous={}
for frame,row in enumerate(rows,1):
    for name,(loc,q,scale) in row.items():
        p=target.pose.bones[name];p.location=loc;p.rotation_quaternion=w.stabilize(q,previous.get(name));previous[name]=p.rotation_quaternion.copy();p.scale=scale
        for channel in ('location','rotation_quaternion','scale'):p.keyframe_insert(channel,frame=frame)
scene.frame_start=1;scene.frame_end=91;scene.render.fps=30;scene.frame_set(40);scene.unit_settings.scale_length=1
bpy.ops.object.select_all(action='DESELECT');target.select_set(True);bpy.context.view_layer.objects.active=target
bpy.ops.export_scene.fbx(filepath=str(root/'art_source/TASK-126'/(action.name+'.fbx')),use_selection=True,object_types={'ARMATURE'},add_leaf_bones=False,use_armature_deform_only=False,axis_forward='X',axis_up='Z',apply_scale_options='FBX_SCALE_NONE',bake_anim=True,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_force_startend_keying=True,bake_anim_step=1,bake_anim_simplify_factor=0)
p=root/'art_source/TASK-126'/(role+'-contact.json');d=json.loads(p.read_text(encoding='utf-8'));d['clips']=[c for c in d['clips'] if c['name']!=action.name];d['clips'].append({'name':action.name,'frames':91,'seconds':3,'source':'Original hand-to-mouth contact performance'});p.write_text(json.dumps(d,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=bpy.data.filepath);print('EAT_AUTHORED',role)
