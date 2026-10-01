import bpy, json, sys
from pathlib import Path
import numpy as np
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
sys.path.insert(0,str(ROOT/'refinement_20261001'/'scripts'))
from audit_refinement import points
jobs=json.loads((ROOT/'jobs.json').read_text('utf-8'))
result={}
for slug,suffix in [('red_fox','CurlRest')]:
    out=Path(next(j['output'] for j in jobs if j['slug']==slug))
    m=json.loads((out/'animation_manifest.json').read_text('utf-8'))
    bpy.ops.wm.open_mainfile(filepath=str(out/f'AS_{slug}.blend'))
    arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    arm.animation_data.action=None
    for p in arm.pose.bones:p.location=(0,0,0);p.rotation_quaternion=(1,0,0,0);p.scale=(1,1,1)
    bpy.context.view_layer.update();base=points(meshes)
    edges=[];offset=0
    groups=[]
    for obj in meshes:
        edges += [(a+offset,b+offset) for a,b in (e.vertices[:] for e in obj.data.edges)]
        groups += [[(obj.vertex_groups[g.group].name,round(g.weight,3)) for g in v.groups if g.weight>.02] for v in obj.data.vertices]
        offset+=len(obj.data.vertices)
    edges=np.array(edges);lengths=np.linalg.norm(base[edges[:,0]]-base[edges[:,1]],axis=1)
    c=next(c for c in m['clips'] if c['suffix']==suffix)
    arm.animation_data.action=bpy.data.actions[c['name']];bpy.context.scene.frame_set(1+round((c['frames']-1)*.25))
    ps=points(meshes);ratios=np.linalg.norm(ps[edges[:,0]]-ps[edges[:,1]],axis=1)/np.maximum(lengths,1e-8)
    valid=(lengths>.0003)&(ratios>1.8);indices=np.where(valid)[0]
    dist={}
    for idx in indices:
        mid=(base[edges[idx,0]]+base[edges[idx,1]])/2
        cell=tuple(int(v/.15) for v in mid);dist[str(cell)]=dist.get(str(cell),0)+1
    top=np.argsort(np.where(lengths>.0003,ratios,0))[-20:][::-1]
    result[slug]={'bone_heads':{b.name:list(b.head_local) for b in arm.data.bones},'bounds':[base.min(axis=0).tolist(),base.max(axis=0).tolist()],
        'strained_edges':len(indices),'clusters':sorted(dist.items(),key=lambda v:-v[1])[:15],
        'worst':[{'ratio':float(ratios[i]),'start':base[edges[i,0]].tolist(),'end':base[edges[i,1]].tolist(),'weights_a':groups[edges[i,0]],'weights_b':groups[edges[i,1]]} for i in top]}
(ROOT/'refinement_20261001'/'fox_rest_probe.json').write_text(json.dumps(result,indent=2),'utf-8')
