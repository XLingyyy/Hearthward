"""Measure actual limb planes, reach, shaft shape and weights before editing."""
import bpy
import sys
import numpy as np
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import ROOT, REV, jobs, read, save, sha
from author import reset
from audit_refinement import points

def inspect(job):
    out = Path(job['output'])
    if '--candidate' in sys.argv:out=REV/'candidate'/job['slug']
    man = read(out/'animation_manifest.json')
    bpy.ops.wm.open_mainfile(filepath=str(out/f'AS_{job["slug"]}.blend'))
    arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    arm.animation_data.action = None
    reset(arm)
    bpy.context.view_layer.update()
    base = points(meshes)
    chains = man['rig']['chains']
    rest = {n: np.array(arm.data.bones[n].head_local) for ns in chains.values() for n in ns}
    masks = {}
    offset = 0
    for key, ns in chains.items():
        for s in (1,2):
            a, b = rest[ns[s]], rest[ns[s+1]]
            axis = b-a
            u = (base-a)@axis/(axis@axis)
            member=[]
            for mesh in meshes:
                names={g.index:g.name for g in mesh.vertex_groups}
                member.extend(sum(g.weight for g in v.groups if names[g.group] in ns) for v in mesh.data.vertices)
            mask=(u>.32)&(u<.68)&(np.array(member)>.8)&(np.linalg.norm(base-a-u[:,None]*axis,axis=1)<np.linalg.norm(axis)*.65)
            masks[key+'_'+str(s)] = np.where(mask)[0]
    records=[]
    for c in man['clips']:
        if c['kind'] not in ('walk','trot','run','hop','start','stop'):continue
        arm.animation_data.action=bpy.data.actions[c['name']]
        rec={'name':c['name'],'kind':c['kind'],'max_outward_cm':0,'max_medial_cm':0,'max_reach_over_total_cm':0,'min_shaft_area_ratio':1.,'limbs':{},'frames':[]}
        for key in chains:rec['limbs'][key]={'outward_cm':0.,'medial_cm':0.}
        for f in range(1,c['frames']+1):
            bpy.context.scene.frame_set(f)
            frame={}
            for key,ns in chains.items():
                p=[np.array(arm.pose.bones[n].matrix.translation) for n in ns[:4]]
                lengths=[np.linalg.norm(rest[ns[i+1]]-rest[ns[i]]) for i in range(3)]
                rec['max_reach_over_total_cm']=max(rec['max_reach_over_total_cm'],(np.linalg.norm(p[3]-p[0])-sum(lengths))*100)
                sign=1 if key.endswith('L') else -1
                for joint in (1,2):
                    u=(p[joint][2]-p[0][2])/(p[3][2]-p[0][2])
                    y=p[0][1]+(p[3][1]-p[0][1])*u
                    lateral=sign*(p[joint][1]-y)*100
                    rec['max_outward_cm']=max(rec['max_outward_cm'],lateral)
                    rec['max_medial_cm']=max(rec['max_medial_cm'],-lateral)
                    rec['limbs'][key]['outward_cm']=max(rec['limbs'][key]['outward_cm'],lateral)
                    rec['limbs'][key]['medial_cm']=max(rec['limbs'][key]['medial_cm'],-lateral)
                frame[key]=[q.tolist() for q in p]
            if f in (1,1+round((c['frames']-1)*.25),1+round((c['frames']-1)*.5),1+round((c['frames']-1)*.75),c['frames']):
                xyz=points(meshes)
                for key,ids in masks.items():
                    if len(ids)<12:continue
                    e0=np.linalg.eigvalsh(np.cov(base[ids].T));e1=np.linalg.eigvalsh(np.cov(xyz[ids].T))
                    ratio=float(np.sqrt(max(0,e1[0]*e1[1])/max(1e-12,e0[0]*e0[1])))
                    rec['min_shaft_area_ratio']=min(rec['min_shaft_area_ratio'],ratio)
                rec['frames'].append({'frame':f,'joints':frame})
        records.append(rec)
        print('GAIT_BASELINE',job['slug'],c['suffix'],round(rec['max_outward_cm'],2),round(rec['min_shaft_area_ratio'],3),flush=True)
    return {'slug':job['slug'],'source_blend_sha256':sha(out/f'AS_{job["slug"]}.blend'),'clips':records,'shaft_masks':{k:len(v) for k,v in masks.items()}}

if __name__=='__main__':
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    phase='candidate' if '--candidate' in args else 'final' if '--final' in args else 'baseline'
    for j in jobs(args):save(REV/phase/f'{j["slug"]}_gait.json',inspect(j))
