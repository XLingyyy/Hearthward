"""Choose shaft correction strength against the entire actual action library.

Linear skinning is linear in weights. Two complete geometry passes give exact
intermediate-weight geometry without repeated IK or repeated export trials.
"""
import bpy,sys,numpy as np
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import ROOT,REV,jobs,read,save,backup_folder
from author import reset
from audit_refinement import points
from correct_gait import rigid_skin
from refinement_helpers_r1 import repair_torso
from body_skin import repair_hare_torso

def weights(mesh):
    w=np.zeros((len(mesh.data.vertices),len(mesh.vertex_groups)),np.float32)
    for v in mesh.data.vertices:
        for g in v.groups:w[v.index,g.group]=g.weight
    return w

def put(mesh,w):
    for v in mesh.data.vertices:
        for g in list(v.groups):mesh.vertex_groups[g.group].remove([v.index])
        for i in np.where(w[v.index]>1e-7)[0]:mesh.vertex_groups[int(i)].add([v.index],float(w[v.index,i]),'REPLACE')

def calibrate(job):
    slug=job['slug'];out=REV/'candidate'/slug;man=read(out/'animation_manifest.json')
    bpy.ops.wm.open_mainfile(filepath=str(backup_folder(job)/f'AS_{slug}.blend'))
    original={o.name:weights(o) for o in bpy.context.scene.objects if o.type=='MESH'}
    bpy.ops.wm.open_mainfile(filepath=str(out/f'AS_{slug}.blend'))
    arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];scene=bpy.context.scene
    for mesh in meshes:put(mesh,original[mesh.name])
    if slug=='hare':repair_hare_torso(arm,meshes,man['rig'])
    arm.animation_data.action=None;reset(arm);bpy.context.view_layer.update();base=points(meshes)
    edges=[];offset=0
    for mesh in meshes:
        a=np.empty(len(mesh.data.edges)*2,np.int32);mesh.data.edges.foreach_get('vertices',a);edges.append(a.reshape(-1,2)+offset);offset+=len(mesh.data.vertices)
    edges=np.concatenate(edges);lengths=np.linalg.norm(base[edges[:,0]]-base[edges[:,1]],axis=1);valid=lengths>.0003;edges=edges[valid];lengths=lengths[valid]
    sources={};samples=[]
    for c in man['clips']:
        arm.animation_data.action=bpy.data.actions[c['name']]
        critical=c['kind'] in ('walk','hop','trot','run','start','stop','lie','rest','up','collapse','rear','swipe','pounce')
        frames=range(1,c['frames']+1) if critical else sorted({1+round((c['frames']-1)*v) for v in (0,.25,.5,.75,1)})
        for f in frames:
            scene.frame_set(f);sources[(c['name'],f)]=points(meshes).astype(np.float32);samples.append((c,f))
    arm.animation_data.action=None;reset(arm);rigid_skin(arm,meshes,man['rig'],chain_gains={'BL':0.,'BR':0.} if slug=='hare' else None)
    gains=[0.,.20,.40,.60,.80,1.]
    results={g:{'gain':g,'max_skin_p99':0.,'min_z_cm':1e9,'max_clip':None,'min_clip':None} for g in gains}
    for c,f in samples:
        arm.animation_data.action=bpy.data.actions[c['name']];scene.frame_set(f);target=points(meshes).astype(np.float32);start=sources.pop((c['name'],f));delta=target-start
        for g in gains:
            xyz=start+delta*g;ratios=np.linalg.norm(xyz[edges[:,0]]-xyz[edges[:,1]],axis=1)/lengths;p99=float(np.percentile(ratios,99));low=float(xyz[:,2].min())*100
            r=results[g]
            if p99>r['max_skin_p99']:r['max_skin_p99']=p99;r['max_clip']=[c['suffix'],f]
            if low<r['min_z_cm']:r['min_z_cm']=low;r['min_clip']=[c['suffix'],f]
    passed=[g for g,r in results.items() if r['max_skin_p99']<=1.8 and r['min_z_cm']>=-.5]
    result={'slug':slug,'revision':'recentered_r3','sample_count':len(samples),'method':'two exact linear-skinning endpoint geometry passes; all locomotion/rest/collapse frames and five samples otherwise','candidates':list(results.values()),'selected_gain':max(passed) if passed else None}
    if slug=='hare' and passed:
        result['chain_gains']={'FL':max(passed),'FR':max(passed),'BL':0.,'BR':0.}
        result['selected_gain']=0.
        result['method']+='; hare fore-shaft-only gain and 30 percent torso repair'
    save(REV/'skin_calibration'/f'{slug}.json',result)
    print('SKIN_CALIBRATION',slug,result['selected_gain'],[(g,round(r['max_skin_p99'],3),round(r['min_z_cm'],3)) for g,r in results.items()],flush=True)

if __name__=='__main__':
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    for j in jobs(args):calibrate(j)
