import bpy,json,sys
from pathlib import Path
import numpy as np
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
sys.path.insert(0,str(ROOT/'refinement_20261001'/'scripts'))
from audit_refinement import points
result={}
for slug,suffix in [('wolf','Gallop'),('red_fox','RunFlee')]:
    job=next(j for j in json.loads((ROOT/'jobs.json').read_text('utf-8')) if j['slug']==slug);out=Path(job['output']);m=json.loads((out/'animation_manifest.json').read_text('utf-8'))
    bpy.ops.wm.open_mainfile(filepath=str(out/f'AS_{slug}.blend'));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    arm.animation_data.action=None
    for p in arm.pose.bones:p.location=(0,0,0);p.rotation_quaternion=(1,0,0,0)
    bpy.context.view_layer.update();base=points(meshes)
    groups=[[(o.vertex_groups[g.group].name,g.weight) for g in v.groups if g.weight>.03] for o in meshes for v in o.data.vertices]
    clip=next(c for c in m['clips'] if c['suffix']==suffix);arm.animation_data.action=bpy.data.actions[clip['name']]
    samples=[]
    for f in range(1,clip['frames']+1):
        bpy.context.scene.frame_set(f);ps=points(meshes);i=int(ps[:,2].argmin())
        if ps[i,2]<-.005:samples.append({'frame':f,'minz':float(ps[i,2]),'reference_xyz':base[i].tolist(),'groups':groups[i]})
    result[slug]=samples
(ROOT/'refinement_20261001'/'ground_probe.json').write_text(json.dumps(result,indent=2),'utf-8')
