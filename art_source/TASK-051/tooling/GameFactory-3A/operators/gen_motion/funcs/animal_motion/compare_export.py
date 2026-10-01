"""Compare delivered FBX skin and poses against the editable source."""
import bpy,json,sys,math
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE))
from validate import points
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
def scene():
    return next(o for o in bpy.context.scene.objects if o.type=='ARMATURE'),[o for o in bpy.context.scene.objects if o.type=='MESH']
def pose(arm):return {p.name:(arm.matrix_world@p.matrix).copy() for p in arm.pose.bones}
def main():
    for slug in ('hare','goat','wolf'):
        j=next(j for j in json.loads((ROOT/'jobs.json').read_text(encoding='utf-8')) if j['slug']==slug);out=Path(j['output']);m=json.loads((out/'animation_manifest.json').read_text(encoding='utf-8'));c=m['clips'][0]
        bpy.ops.wm.open_mainfile(filepath=str(out/f'AS_{slug}.blend'));arm,meshes=scene();arm.animation_data.action=bpy.data.actions[c['name']];bpy.context.scene.frame_set(1);bpy.context.view_layer.update()
        expected=pose(arm);source=points(meshes);source_base=[o.matrix_world@v.co for o in meshes for v in o.data.vertices]
        bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(out/f'SK_{slug}.fbx'));arm,meshes=scene();before=points(meshes);raw=[o.matrix_world@v.co for o in meshes for v in o.data.vertices]
        sk_base_error=max((a-b).length for a,b in zip(source_base,raw));sk_pose_error=max((a-b).length for a,b in zip(raw,before))
        old=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=c['file']);added=set(bpy.data.objects)-old;motion=next(o for o in added if o.type=='ARMATURE');action=motion.animation_data.action
        arm.animation_data_create();arm.animation_data.action=action
        if motion.animation_data.action_slot:arm.animation_data.action_slot=motion.animation_data.action_slot
        for o in added:bpy.data.objects.remove(o,do_unlink=True)
        bpy.context.scene.frame_set(round(action.frame_range[0]));bpy.context.view_layer.update();actual=pose(arm);actual_points=points(meshes)
        errors=sorted([((expected[n].translation-actual[n].translation).length,math.degrees(expected[n].to_quaternion().rotation_difference(actual[n].to_quaternion()).angle),n) for n in expected],reverse=True)
        diffs=sorted((a-b).length for a,b in zip(source,actual_points))
        print(slug,'raw_vertex',sk_base_error,'sk_rest_deform',sk_pose_error,'mesh_actual',diffs[-1],diffs[int(len(diffs)*.99)],'bones',errors[:3],flush=True)
if __name__=='__main__':main()
