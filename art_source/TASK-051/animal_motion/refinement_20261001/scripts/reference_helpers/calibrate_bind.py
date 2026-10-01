"""Calibrate anatomical bind orientation to the solved standing pose.

Source limb bone rolls can produce permanent IK twist even at rest. Keep the
original vertices/UVs/weights and every authored world-space joint trajectory;
change the reference skeleton and rebake relative curves to remove that twist.
"""
import bpy,json,sys,shutil,math
from pathlib import Path
from mathutils import Matrix,Vector
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE))
from author import reset,curves,evaluated_min_z,write_rig_report
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')

def main():
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else ['stag_a','hare','goat','pig','wolf','ram','red_fox']
    jobs=json.loads((ROOT/'jobs.json').read_text(encoding='utf-8'))
    for j in jobs:
        if j['slug'] not in args:continue
        out=Path(j['output']);path=out/f"AS_{j['slug']}.blend";m=json.loads((out/'animation_manifest.json').read_text(encoding='utf-8'))
        if m.get('reference_pose_calibration'):print('Already calibrated '+j['slug'],flush=True);continue
        backup=out/'history';backup.mkdir(exist_ok=True);shutil.copyfile(path,backup/'AS_before_bind_calibration.blend')
        bpy.ops.wm.open_mainfile(filepath=str(path));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];rig=m['rig'];scene=bpy.context.scene
        ref_clip=m['clips'][0];sampled={}
        for c in m['clips']:
            arm.animation_data.action=bpy.data.actions[c['name']];poses=[]
            for f in range(1,c['frames']+1):
                scene.frame_set(f);bpy.context.view_layer.update();poses.append({p.name:p.matrix.copy() for p in arm.pose.bones})
            sampled[c['name']]=poses
        reference=sampled[ref_clip['name']][0];lengths={b.name:b.length for b in arm.data.bones}
        arm.animation_data.action=None;reset(arm);bpy.context.view_layer.objects.active=arm;arm.select_set(True)
        bpy.ops.object.mode_set(mode='EDIT')
        for n,mat in reference.items():arm.data.edit_bones[n].matrix=mat;arm.data.edit_bones[n].length=lengths[n]
        bpy.ops.object.mode_set(mode='OBJECT');reset(arm);bpy.context.view_layer.update()
        max_error=0;floor_offsets={}
        for c in m['clips']:
            act=bpy.data.actions[c['name']];arm.animation_data.action=act;scene.render.fps=c['fps'];previous={};offsets=[]
            for f,world in enumerate(sampled[c['name']],1):
                scene.frame_set(f)
                for p in arm.pose.bones:
                    basis=p.bone.convert_local_to_pose(world[p.name],p.bone.matrix_local,parent_matrix=world[p.parent.name] if p.parent else Matrix.Identity(4),parent_matrix_local=p.parent.bone.matrix_local if p.parent else Matrix.Identity(4),invert=True)
                    loc,rot,scale=basis.decompose()
                    if p.name in previous and previous[p.name].dot(rot)<0:rot.negate()
                    p.location=loc;p.rotation_mode='QUATERNION';p.rotation_quaternion=rot;p.scale=(1,1,1);previous[p.name]=rot.copy()
                    p.keyframe_insert('location',frame=f,group=p.name);p.keyframe_insert('rotation_quaternion',frame=f,group=p.name)
                bpy.context.view_layer.update()
                max_error=max(max_error,max((p.matrix.translation-world[p.name].translation).length for p in arm.pose.bones))
                correction=0
                if c['kind'] in ('collapse','corpse','lie','up','rest','nest','dust'):
                    correction=.0003-evaluated_min_z(meshes)
                    p=arm.pose.bones[rig['body']];p.location+=p.bone.matrix_local.to_3x3().inverted()@Vector((0,0,correction));p.keyframe_insert('location',frame=f,group=p.name);bpy.context.view_layer.update()
                offsets.append(correction)
            floor_offsets[c['name']]=max(abs(v) for v in offsets)
            for fc in curves(act):
                for k in fc.keyframe_points:k.interpolation='LINEAR'
            print('CALIBRATED '+j['slug']+' '+c['suffix'],flush=True)
        rig['repairs'].append('Calibrated bind orientations to solved natural stance; rebaked all world-space joint trajectories without IK rest twist')
        for k,ns in rig['chains'].items():rig['restfeet'][k]=list(arm.data.bones[ns[3] if len(ns)>3 else ns[-1]].head_local)
        write_rig_report(arm,meshes,rig,out)
        for c in m['clips']:c['skeleton_signature']=rig['skeleton_signature']
        m['reference_pose_calibration']={'reference_action':ref_clip['name'],'world_joint_preservation_error_m':max_error,'floor_correction_by_clip_m':floor_offsets,'mesh_vertices_uv_weights':'unchanged','bone_rest':'calibrated'}
        m['rig']=rig;m['ue_import']='NOT_RUN';m['visual_qa']='pending after bind calibration'
        (out/'animation_manifest.json').write_text(json.dumps(m,ensure_ascii=False,indent=2),encoding='utf-8')
        arm.animation_data.action=bpy.data.actions[ref_clip['name']];scene.render.fps=ref_clip['fps'];scene.frame_start=1;scene.frame_end=ref_clip['frames'];scene.frame_set(1);bpy.ops.wm.save_as_mainfile(filepath=str(path))
        print('BIND CALIBRATION '+j['slug']+' world error '+str(max_error),flush=True)
if __name__=='__main__':main()
