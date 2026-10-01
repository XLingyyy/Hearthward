"""Search compact shaft and gentle body weight variants across actual poses."""
import bpy,sys,numpy as np
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import REV,jobs,read,save,backup_folder
from calibrate_skin import weights,put
from correct_gait import rigid_skin
from refinement_helpers_r1 import repair_torso
from author import reset
from audit_refinement import points
from mathutils.kdtree import KDTree

j=jobs(['hare'])[0];out=REV/'candidate/hare';m=read(out/'animation_manifest.json')
bpy.ops.wm.open_mainfile(filepath=str(backup_folder(j)/'AS_hare.blend'));original={o.name:weights(o) for o in bpy.context.scene.objects if o.type=='MESH'}
bpy.ops.wm.open_mainfile(filepath=str(out/'AS_hare.blend'));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];scene=bpy.context.scene
for o in meshes:put(o,original[o.name])
arm.animation_data.action=None;reset(arm);bpy.context.view_layer.update();base=points(meshes)
edges=[];offset=0
for mesh in meshes:
    a=np.empty(len(mesh.data.edges)*2,np.int32);mesh.data.edges.foreach_get('vertices',a);edges.append(a.reshape(-1,2)+offset);offset+=len(mesh.data.vertices)
edges=np.concatenate(edges);length=np.linalg.norm(base[edges[:,0]]-base[edges[:,1]],axis=1);valid=length>.0003;edges=edges[valid];length=length[valid]
samples=[]
for c in m['clips']:
    critical=c['kind'] in ('walk','hop','trot','run','start','stop','lie','rest','up','collapse','rear','swipe','pounce')
    frames=range(1,c['frames']+1) if critical else sorted({1+round((c['frames']-1)*v) for v in (0,.25,.5,.75,1)})
    samples.extend((c,f) for f in frames)
def sample():
    result=[]
    for c,f in samples:
        arm.animation_data.action=bpy.data.actions[c['name']];scene.frame_set(f);result.append(points(meshes).astype(np.float32))
    return np.array(result)
xyz0=sample();arm.animation_data.action=None;reset(arm);rigid_skin(arm,meshes,m['rig']);xyzshaft=sample()
for o in meshes:put(o,original[o.name])
arm.animation_data.action=None;reset(arm);repair_torso(arm,meshes,m['rig'],'hare');xyzbody=sample()
for o in meshes:put(o,original[o.name])
# Local body smoothing only above the elbow line; foot and distal shafts keep
# their exact original weights. This is an optional measured candidate.
for mesh in meshes:
    orig=original[mesh.name];w=orig.copy();tree=KDTree(len(mesh.data.vertices))
    for v in mesh.data.vertices:tree.insert(v.co,v.index)
    tree.balance();near=np.array([[i for _,i,d in tree.find_n(v.co,24)] for v in mesh.data.vertices])
    for _ in range(16):w=.3*w+.7*w[near].mean(axis=1)
    xyz=np.array([list(v.co) for v in mesh.data.vertices]);zlo=min(arm.data.bones[ns[1]].head_local.z for ns in m['rig']['chains'].values())
    mask=np.clip((xyz[:,2]-zlo-.01)/.07,0,1);put(mesh,orig+(w-orig)*mask[:,None])
xyzsmooth=sample();results=[]
for method,target in [('body_repair',xyzbody),('body_smooth',xyzsmooth)]:
 for bg in (0.,.1,.2,.3,.4,.6,.8,1.):
  for sg in (0.,.2,.4,.6,.8,1.):
    xyz=xyz0+bg*(target-xyz0)+sg*(xyzshaft-xyz0)
    max_p=0.;min_z=1e9;worst=None
    for idx,(c,f) in enumerate(samples):
        p=float(np.percentile(np.linalg.norm(xyz[idx,edges[:,0]]-xyz[idx,edges[:,1]],axis=1)/length,99));z=float(xyz[idx,:,2].min())*100
        if p>max_p:max_p=p;worst=[c['suffix'],f]
        min_z=min(min_z,z)
    results.append({'method':method,'body_gain':bg,'shaft_gain':sg,'max_skin_p99':max_p,'min_z_cm':min_z,'worst':worst,'pass':max_p<=1.8 and min_z>=-.5})
results.sort(key=lambda x:x['max_skin_p99']);save(REV/'hare_weight_trials.json',results)
for x in results[:8]:print('HARE_WEIGHTS',x,flush=True)
