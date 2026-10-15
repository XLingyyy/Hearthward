"""Retarget licensed UAL contact actions onto the unchanged Hero/Brother rigs via Blender MCP.

Open art_source/TASK-095/Hero.blend or Brother.blend first. Original bind poses remain unchanged.
"""
import bpy, math, json, sys
from pathlib import Path
from mathutils import Matrix, Vector
ROOT=Path('G:/GameFactory/Hearthward')
sys.path.extend([str(ROOT.parent),str(ROOT.parent/'.venv/Lib/site-packages')])
from operators.gen_motion.funcs.retarget_utils import world_delta as w
role=Path(bpy.data.filepath).stem
assert role in ('Hero','Brother')
scene=bpy.context.scene
target=bpy.data.objects['Armature'];body=next(o for o in target.children if o.type=='MESH')
target.data.pose_position='POSE'
before=set(bpy.data.objects)
bpy.ops.import_scene.gltf(filepath=str(ROOT/'art_source/TASK-109/UAL1_Standard.glb'))
source=next(o for o in bpy.data.objects if o not in before and o.type=='ARMATURE')
imported=set(bpy.data.objects)-before
for obj in imported:obj.hide_render=True
for track in source.animation_data.nla_tracks:track.mute=True
source.animation_data.action=bpy.data.actions['A_TPose'];source.animation_data.action_slot=source.animation_data.action.slots[0]
target.animation_data_clear();target.animation_data_create();w.reset_target_pose(target)
scene.frame_set(0);bpy.context.view_layer.update()
sref={p.name:(source.matrix_world@p.matrix).to_quaternion() for p in source.pose.bones}
local=w.cache_local_rest_quaternions(target)
hip=target.pose.bones['pelvis'].head.copy();source_hip=source.pose.bones['pelvis'].head.copy()
ratio=(hip.z-target.pose.bones['foot_l'].head.z)/(source_hip.z-source.pose.bones['foot_l'].head.z)
for side in ('l','r'):
    for bone,child in [('upperarm_','lowerarm_'),('lowerarm_','hand_')]:
        p=target.pose.bones[bone+side];direction=target.pose.bones[child+side].head-p.head
        desired=source.pose.bones[child+side].head-source.pose.bones[bone+side].head
        rotation=direction.normalized().rotation_difference(desired.normalized());mat=p.matrix.copy()
        p.matrix=Matrix.Translation(mat.translation)@rotation.to_matrix().to_4x4()@Matrix.Translation(-mat.translation)@mat
        bpy.context.view_layer.update()
tref={p.name:(target.matrix_world@p.matrix).to_quaternion() for p in target.pose.bones}
mapping={p.name:p.name.lower() if p.name=='Head' else p.name for p in source.pose.bones if (p.name.lower() if p.name=='Head' else p.name) in target.pose.bones}
ordered=w.order_mapping_parent_first(mapping,target);source_fps=scene.render.fps/scene.render.fps_base
out=ROOT/'art_source/TASK-126';out.mkdir(parents=True,exist_ok=True)
report={'role':role,'source':'Quaternius Universal Animation Library Standard','license':'CC0','clips':[]}
clips=[('Interact','Operate',False),('PickUp_Table','Pickup',False),('PickUp_Table','Handoff',True),
       ('Push_Loop','MoveBeam',False),('Sitting_Enter','SitDown',False),('Sitting_Idle_Loop','SeatedIdle',False),
       ('Sitting_Exit','StandUp',False),('Fixing_Kneeling','Gather',False)]
for original_name,name,reverse in clips:
    original=bpy.data.actions[original_name];source.animation_data.action=original;source.animation_data.action_slot=original.slots[0]
    start,end=original.frame_range;count=round((end-start)/source_fps*30)+1;rows=[];contacts=[]
    for index in range(count):
        fraction=index/max(1,count-1);time=end-fraction*(end-start) if reverse else start+fraction*(end-start)
        scene.frame_set(math.floor(time),subframe=time%1);w.reset_target_pose(target);bpy.context.view_layer.update()
        w.retarget_frame(source,target,ordered,sref,tref,local,0)
        pelvis=target.pose.bones['pelvis'];mat=pelvis.matrix.copy();mat.translation=hip+(source.pose.bones['pelvis'].head-source_hip)*ratio;pelvis.matrix=mat
        bpy.context.view_layer.update()
        obj=body.evaluated_get(bpy.context.evaluated_depsgraph_get());mesh=obj.to_mesh();floor=min((obj.matrix_world@v.co).z for v in mesh.vertices);obj.to_mesh_clear()
        root=target.pose.bones['root'];mat=root.matrix.copy();mat.translation+=target.matrix_world.inverted().to_3x3()@Vector((0,0,-floor));root.matrix=mat
        bpy.context.view_layer.update()
        rows.append({p.name:(p.location.copy(),p.rotation_quaternion.copy(),p.scale.copy()) for p in target.pose.bones})
        if index in (0,count//2,count-1):contacts.append({'frame':index+1,'hand_r':list(target.matrix_world@target.pose.bones['hand_r'].head),'hand_l':list(target.matrix_world@target.pose.bones['hand_l'].head),'foot_r':list(target.matrix_world@target.pose.bones['foot_r'].head)})
    action=bpy.data.actions.new('A_'+role+'_'+name);action.use_fake_user=True;target.animation_data.action=action;previous={}
    for frame,row in enumerate(rows,1):
        for bone,(position,rotation,scale) in row.items():
            p=target.pose.bones[bone];p.location=position;p.rotation_quaternion=w.stabilize(rotation,previous.get(bone));previous[bone]=p.rotation_quaternion.copy();p.scale=scale
            for channel in ('location','rotation_quaternion','scale'):p.keyframe_insert(channel,frame=frame)
    scene.render.fps=30;scene.render.fps_base=1;scene.frame_start=1;scene.frame_end=count;scene.frame_set(1)
    bpy.ops.object.select_all(action='DESELECT');target.select_set(True);bpy.context.view_layer.objects.active=target
    scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
    bpy.ops.export_scene.fbx(filepath=str(out/(action.name+'.fbx')),use_selection=True,object_types={'ARMATURE'},add_leaf_bones=False,use_armature_deform_only=False,
        axis_forward='X',axis_up='Z',apply_scale_options='FBX_SCALE_NONE',bake_anim=True,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,
        bake_anim_use_all_actions=False,bake_anim_force_startend_keying=True,bake_anim_step=1,bake_anim_simplify_factor=0)
    report['clips'].append({'name':action.name,'frames':count,'seconds':(count-1)/30,'contact_samples_m':contacts,'reverse':reverse})
    target.animation_data.action=None
for obj in imported:obj.hide_set(True)
body.hide_set(False)
target.animation_data.action=bpy.data.actions['A_'+role+'_Operate'];scene.frame_start=1;scene.frame_end=61;scene.frame_set(30)
bpy.ops.wm.save_as_mainfile(filepath=str(out/(role+'-Contact.blend')))
(out/(role+'-contact.json')).write_text(json.dumps(report,indent=2),encoding='utf-8')
print('CONTACT_MOTION',role,len(report['clips']))
