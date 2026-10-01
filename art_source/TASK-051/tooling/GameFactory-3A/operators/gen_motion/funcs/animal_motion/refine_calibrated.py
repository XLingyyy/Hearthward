"""Refine browsing and gentle collapse folds on calibrated bind skeletons."""
import bpy,json,sys
from pathlib import Path
from mathutils import Vector,Quaternion
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE))
from author import rotate_bone,smooth,evaluated_min_z,curves
from rigs import descendants
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
def main():
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else ['goat','hare','wolf']
    for j in json.loads((ROOT/'jobs.json').read_text(encoding='utf-8')):
        if j['slug'] not in args:continue
        out=Path(j['output']);m=json.loads((out/'animation_manifest.json').read_text(encoding='utf-8'));rig=m['rig']
        bpy.ops.wm.open_mainfile(filepath=str(out/f"AS_{j['slug']}.blend"));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];scene=bpy.context.scene
        changed=[]
        for c in m['clips']:
            forage=j['slug']=='goat' and c['kind']=='forage';dead=j['slug'] in ('hare','wolf','red_fox') and c['kind'] in ('collapse','corpse')
            if not (forage or dead):continue
            arm.animation_data.action=bpy.data.actions[c['name']];scene.render.fps=c['fps']
            for f in range(1,c['frames']+1):
                scene.frame_set(f);t=(f-1)/(c['frames']-1)
                if forage:
                    a=1 if c['suffix'].endswith('_Loop') else smooth(t) if c['suffix'].endswith('_In') else 1-smooth(t)
                    for n in rig['neck']:
                        rotate_bone(arm,n,(0,1,0),32*a/len(rig['neck']))
                        p=arm.pose.bones[n];p.keyframe_insert('rotation_quaternion',frame=f,group=n)
                    p=arm.pose.bones[rig['head']];q=p.bone.matrix_local.to_quaternion();p.rotation_quaternion@=q.inverted()@Quaternion((0,1,0),.10472*a)@q;p.keyframe_insert('rotation_quaternion',frame=f,group=p.name)
                if dead:
                    a=1 if c['kind']=='corpse' else smooth(t)
                    for k,ns in rig['chains'].items():
                        for b in descendants(arm.data.bones[ns[0]]):
                            p=arm.pose.bones[b.name];p.location=(0,0,0);p.rotation_quaternion=(1,0,0,0);p.keyframe_insert('location',frame=f,group=p.name);p.keyframe_insert('rotation_quaternion',frame=f,group=p.name)
                        for n,degrees in zip(ns[:3],(24,-32,8) if k.startswith('F') else (-15,32,-12)):
                            rotate_bone(arm,n,(0,1,0),degrees*a);arm.pose.bones[n].keyframe_insert('rotation_quaternion',frame=f,group=n)
                    bpy.context.view_layer.update();correction=.0003-evaluated_min_z(meshes)
                    p=arm.pose.bones[rig['body']];p.location+=p.bone.matrix_local.to_3x3().inverted()@Vector((0,0,correction));p.keyframe_insert('location',frame=f,group=p.name)
            for fc in curves(arm.animation_data.action):
                for k in fc.keyframe_points:k.interpolation='LINEAR'
            changed.append(c['name']);print('REFINED '+c['name'],flush=True)
        m['reference_pose_calibration']['later_pose_refinements']=changed;m['ue_import']='NOT_RUN'
        (out/'animation_manifest.json').write_text(json.dumps(m,ensure_ascii=False,indent=2),encoding='utf-8')
        first=m['clips'][0];arm.animation_data.action=bpy.data.actions[first['name']];scene.render.fps=first['fps'];scene.frame_start=1;scene.frame_end=first['frames'];scene.frame_set(1);bpy.ops.wm.save_as_mainfile(filepath=str(out/f"AS_{j['slug']}.blend"))
if __name__=='__main__':main()
