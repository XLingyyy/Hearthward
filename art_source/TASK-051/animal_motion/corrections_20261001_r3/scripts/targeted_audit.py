"""Current-file gait joints, matched shaft samples, foot tracks and source QA."""
import bpy,sys,math,hashlib,numpy as np
from pathlib import Path
from mathutils import Vector
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import ROOT,REV,GAITS,jobs,read,save,sha,backup_folder
from author import reset
from audit_refinement import points,angle,difference,pose

def digest_mesh(mesh):
    xyz=np.empty(len(mesh.vertices)*3,np.float32);mesh.vertices.foreach_get('co',xyz)
    faces=[list(p.vertices) for p in mesh.polygons]
    uv=[]
    for layer in mesh.uv_layers:
        a=np.empty(len(layer.data)*2,np.float32);layer.data.foreach_get('uv',a);uv.append(a.tobytes())
    return {'vertices':hashlib.sha256(xyz.tobytes()).hexdigest(),'faces':hashlib.sha256(str(faces).encode()).hexdigest(),'uv':hashlib.sha256(b''.join(uv)).hexdigest()}

def audit(job):
    slug=job['slug'];out=REV/'candidate'/slug if '--candidate' in sys.argv else Path(job['output']);m=read(out/'animation_manifest.json');base=backup_folder(job)
    source_geometry={};old_feet={};original_members={};old_mats={}
    bpy.ops.wm.open_mainfile(filepath=str(base/f'AS_{slug}.blend'));scene=bpy.context.scene;arm=next(o for o in scene.objects if o.type=='ARMATURE');meshes=[o for o in scene.objects if o.type=='MESH']
    for mesh in meshes:
        source_geometry[mesh.name]=digest_mesh(mesh.data)
        names={g.index:g.name for g in mesh.vertex_groups}
        original_members[mesh.name]={k:np.array([sum(g.weight for g in v.groups if names[g.group] in ns) for v in mesh.data.vertices]) for k,ns in m['rig']['chains'].items()}
    for c in m['clips']:
        if c['kind'] not in GAITS:continue
        arm.animation_data.action=bpy.data.actions[c['name']];old_feet[c['name']]=[]
        for f in range(1,c['frames']+1):
            scene.frame_set(f);old_feet[c['name']].append({k:(arm.pose.bones[ns[3]].matrix.translation.copy(),arm.pose.bones[ns[3]].matrix.to_quaternion()) for k,ns in m['rig']['chains'].items()})
    bpy.ops.wm.open_mainfile(filepath=str(out/f'AS_{slug}.blend'));scene=bpy.context.scene;arm=next(o for o in scene.objects if o.type=='ARMATURE');meshes=[o for o in scene.objects if o.type=='MESH']
    geom={o.name:digest_mesh(o.data) for o in meshes}
    assert geom==source_geometry,'Changed mesh geometry/topology/UV: '+slug
    arm.animation_data.action=None;reset(arm);bpy.context.view_layer.update();xyz=points(meshes)
    masks={};chains=m['rig']['chains'];lengths={}
    for key,ns in chains.items():
        lengths[key]=[(arm.data.bones[ns[i+1]].head_local-arm.data.bones[ns[i]].head_local).length for i in range(3)]
        member=np.concatenate([original_members[mesh.name][key] for mesh in meshes])
        for i in (1,2):
            a=np.array(arm.data.bones[ns[i]].head_local);b=np.array(arm.data.bones[ns[i+1]].head_local);v=b-a;u=(xyz-a)@v/(v@v)
            mask=(u>.34)&(u<.66)&(member>.8)&(np.linalg.norm(xyz-a-u[:,None]*v,axis=1)<np.linalg.norm(v)*.65)
            ids=np.where(mask)[0]
            if len(ids)>=16:masks[key+'_'+str(i)]=(ids,np.linalg.eigvalsh(np.cov(xyz[ids].T)))
    centre=sum(arm.data.bones[ns[0]].head_local.y for ns in chains.values())/4
    reports=[];endpoints={}
    for c in m['clips']:
        arm.animation_data.action=bpy.data.actions[c['name']]
        scene.frame_set(1);p0=pose(arm);scene.frame_set(c['frames']);endpoints[c['suffix']]=(p0,pose(arm))
        if c['kind'] not in GAITS:continue
        r={'name':c['name'],'kind':c['kind'],'segment_length_error_cm':0.,'foot_track_error_cm':0.,'foot_orientation_error_deg':0.,'max_outward_joint_cm_at_full_gait':0.,'min_shaft_area_ratio':1.,'max_adjacent_rotation_deg':0.,'fixed_root_drift_cm':0.,'scale_error':0.,'shaft_samples':{}}
        last=None
        for f in range(1,c['frames']+1):
            scene.frame_set(f);p=pose(arm);t=(f-1)/(c['frames']-1)
            amplitude=(t*t*t*(10+t*(-15+6*t))) if c['kind']=='start' else 1-t*t*t*(10+t*(-15+6*t)) if c['kind']=='stop' else 1
            r['fixed_root_drift_cm']=max(r['fixed_root_drift_cm'],(p['root'][0]-p0['root'][0]).length*100)
            r['scale_error']=max(r['scale_error'],max(abs(v-1) for b in arm.pose.bones for v in b.scale))
            if last:r['max_adjacent_rotation_deg']=max(r['max_adjacent_rotation_deg'],max(angle(last[n][1],p[n][1]) for n in p))
            last=p
            for key,ns in chains.items():
                q=[p[n][0] for n in ns[:4]]
                if c['kind'] not in ('start','stop') or amplitude>.999:
                    r['segment_length_error_cm']=max(r['segment_length_error_cm'],max(abs((q[i+1]-q[i]).length-lengths[key][i])*100 for i in range(3)))
                old_pos,old_q=old_feet[c['name']][f-1][key]
                r['foot_track_error_cm']=max(r['foot_track_error_cm'],(p[ns[3]][0]-old_pos).length*100)
                r['foot_orientation_error_deg']=max(r['foot_orientation_error_deg'],angle(old_q,p[ns[3]][1]))
                if amplitude>.95:
                    side=1 if key.endswith('L') else -1
                    for i in (1,2):
                        u=(q[i].z-q[0].z)/(q[3].z-q[0].z)
                        y=q[0].y+(q[3].y-q[0].y)*u
                        r['max_outward_joint_cm_at_full_gait']=max(r['max_outward_joint_cm_at_full_gait'],side*(q[i].y-y)*100)
            if c['kind'] in ('walk','hop','trot','run'):
                a=points(meshes)
                for name,(ids,e0) in masks.items():
                    e=np.linalg.eigvalsh(np.cov(a[ids].T));ratio=float(np.sqrt(max(0,e[0]*e[1])/max(1e-12,e0[0]*e0[1])))
                    r['min_shaft_area_ratio']=min(r['min_shaft_area_ratio'],ratio)
                    r['shaft_samples'][name]=min(r['shaft_samples'].get(name,1.),ratio)
        r['pass']=bool(r['segment_length_error_cm']<.001 and r['foot_track_error_cm']<.001 and r['foot_orientation_error_deg']<.1 and r['fixed_root_drift_cm']<.001 and r['scale_error']<.0001 and r['max_outward_joint_cm_at_full_gait']<.5 and r['max_adjacent_rotation_deg']<60)
        reports.append(r);print('TARGETED',slug,c['suffix'],r['pass'],round(r['min_shaft_area_ratio'],3),flush=True)
    start=next(c for c in m['clips'] if c['kind']=='start');stop=next(c for c in m['clips'] if c['kind']=='stop');runs=[c for c in m['clips'] if c['kind']=='run'];run=runs[-1] if slug=='pig' else runs[0];idle=m['clips'][0]
    seams=[{'from':a,'to':b,**difference(endpoints[a][1],endpoints[b][0])} for a,b in [(idle['suffix'],start['suffix']),(start['suffix'],run['suffix']),(run['suffix'],stop['suffix']),(stop['suffix'],idle['suffix'])]]
    result={'slug':slug,'source_blend_sha256':sha(out/f'AS_{slug}.blend'),'skeletal_fbx_sha256':sha(out/f'SK_{slug}.fbx'),'source_fbx_unchanged':sha(job['source'])==job['source_sha256'],'geometry_topology_uv_preserved':geom==source_geometry,'geometry_hashes':geom,'gaits':reports,'start_stop_seams':seams,'bone_rest_signature':m['rig']['skeleton_signature'],'gameplay_integration':'NOT_RUN'}
    result['pass']=all(r['pass'] for r in reports) and all(s['position_cm']<.1 and s['rotation_deg']<.1 for s in seams) and result['source_fbx_unchanged']
    # Compare exactly the same original vertices, covariance reference and
    # phase frames, rather than regenerating a different mask for each rig.
    bpy.ops.wm.open_mainfile(filepath=str(base/f'AS_{slug}.blend'));scene=bpy.context.scene;arm=next(o for o in scene.objects if o.type=='ARMATURE');meshes=[o for o in scene.objects if o.type=='MESH']
    for r in reports:
        if r['kind'] not in ('walk','hop','trot','run'):continue
        c=next(c for c in m['clips'] if c['name']==r['name']);arm.animation_data.action=bpy.data.actions[c['name']]
        previous={name:1. for name in masks}
        for f in range(1,c['frames']+1):
            scene.frame_set(f);a=points(meshes)
            for name,(ids,e0) in masks.items():
                e=np.linalg.eigvalsh(np.cov(a[ids].T));ratio=float(np.sqrt(max(0,e[0]*e[1])/max(1e-12,e0[0]*e0[1])))
                previous[name]=min(previous[name],ratio)
        r['baseline_shaft_samples_same_vertices']=previous
        r['baseline_min_shaft_area_ratio']=min(previous.values()) if previous else None
    result['shaft_sampling_note']='PCA area proxy of identical original vertex groups in each shaft; a local visual diagnostic, not a whole-mesh volume guarantee'
    save(out/'source_integrity_20261001_r3.json',{k:v for k,v in result.items() if k not in ('gaits','start_stop_seams')})
    return result

if __name__=='__main__':
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    phase='candidate' if '--candidate' in args else 'final'
    for j in jobs(args):save(REV/phase/f'{j["slug"]}_targeted.json',audit(j))
