import bpy,sys,numpy as np,collections
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import REV,jobs,read,save,backup_folder
from calibrate_skin import weights,put
from author import reset
from audit_refinement import points
j=jobs(['hare'])[0];out=REV/'candidate'/'hare';m=read(out/'animation_manifest.json')
bpy.ops.wm.open_mainfile(filepath=str(backup_folder(j)/'AS_hare.blend'));original={o.name:weights(o) for o in bpy.context.scene.objects if o.type=='MESH'}
bpy.ops.wm.open_mainfile(filepath=str(out/'AS_hare.blend'));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
for o in meshes:put(o,original[o.name])
arm.animation_data.action=None;reset(arm);bpy.context.view_layer.update();base=points(meshes)
edges=[];offset=0;w=[];names=[]
for mesh in meshes:
 a=np.empty(len(mesh.data.edges)*2,np.int32);mesh.data.edges.foreach_get('vertices',a);edges.extend(a.reshape(-1,2)+offset);offset+=len(mesh.data.vertices)
 w.extend(original[mesh.name]);names=[g.name for g in mesh.vertex_groups]
edges=np.array(edges);w=np.array(w);length=np.linalg.norm(base[edges[:,0]]-base[edges[:,1]],axis=1);valid=length>.0003;edges=edges[valid];length=length[valid]
cal=read(REV/'skin_calibration/hare.json');suffix,frame=cal['candidates'][0]['max_clip'];c=next(c for c in m['clips'] if c['suffix']==suffix)
arm.animation_data.action=bpy.data.actions[c['name']];bpy.context.scene.frame_set(frame);xyz=points(meshes);ratio=np.linalg.norm(xyz[edges[:,0]]-xyz[edges[:,1]],axis=1)/length
top=edges[ratio>np.percentile(ratio,99)];ids=np.unique(top);groups=np.argmax(w[ids],axis=1)
print('MAX',suffix,frame,'p99',np.percentile(ratio,99),flush=True)
print('DOMINANT',collections.Counter(names[g] for g in groups),flush=True)
print('COORDINATE_RANGE_CM',base[ids].min(axis=0)*100,base[ids].max(axis=0)*100,flush=True)
save(REV/'hare_probe.json',{'suffix':suffix,'frame':frame,'groups':dict(collections.Counter(names[g] for g in groups)),'centroid_cm':(base[ids].mean(axis=0)*100).tolist()})
