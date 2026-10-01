"""Measure torso damping choices with original skin and exact limb lengths."""
import bpy,sys,math,numpy as np
from pathlib import Path
from mathutils import Matrix
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import REV,GAITS,jobs,read,save,backup_folder
from author import reset
from refinement_helpers_r1 import apply,move
from rebind_legs import recenter
from limb_solver import solve_limb
from audit_refinement import points

j=jobs(['hare'])[0];m=read(backup_folder(j)/'animation_manifest.json');rig=m['rig']
bpy.ops.wm.open_mainfile(filepath=str(backup_folder(j)/'AS_hare.blend'))
arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];scene=bpy.context.scene
original_rest={b.name:b.matrix_local.copy() for b in arm.data.bones};sources={}
for c in m['clips']:
    if c['kind'] not in ('walk','hop','trot','run'):continue
    arm.animation_data.action=bpy.data.actions[c['name']];sources[c['name']]=[]
    for f in range(1,c['frames']+1):
        scene.frame_set(f);sources[c['name']].append({p.name:p.matrix.copy() for p in arm.pose.bones})
arm.animation_data.action=bpy.data.actions[m['clips'][0]['name']];scene.frame_set(1);standing={p.name:p.matrix.copy() for p in arm.pose.bones}
arm.animation_data.action=None;reset(arm);recenter(arm,meshes,rig,'hare')
new_rest={b.name:b.matrix_local.copy() for b in arm.data.bones};conv={n:original_rest[n].inverted()@new_rest[n] for n in original_rest}
def retarget(mats):
    rebaked={n:mats[n]@conv[n] for n in mats};values={}
    for p in arm.pose.bones:
        basis=p.bone.convert_local_to_pose(rebaked[p.name],new_rest[p.name],parent_matrix=rebaked[p.parent.name] if p.parent else Matrix.Identity(4),parent_matrix_local=new_rest[p.parent.name] if p.parent else Matrix.Identity(4),invert=True)
        loc,q,_=basis.decompose();values[p.name]=(loc,q)
    return rebaked,values
stand=retarget(standing)[1]
for name,frames in sources.items():sources[name]=[retarget(x) for x in frames]
lengths={k:[(arm.data.bones[ns[i+1]].head_local-arm.data.bones[ns[i]].head_local).length for i in range(3)] for k,ns in rig['chains'].items()}
centre=sum(arm.data.bones[ns[0]].head_local.y for ns in rig['chains'].values())/4
reset(arm);bpy.context.view_layer.update();base=points(meshes);edges=[];off=0
for mesh in meshes:
    a=np.empty(len(mesh.data.edges)*2,np.int32);mesh.data.edges.foreach_get('vertices',a);edges.append(a.reshape(-1,2)+off);off+=len(mesh.data.vertices)
edges=np.concatenate(edges);edge_length=np.linalg.norm(base[edges[:,0]]-base[edges[:,1]],axis=1);valid=edge_length>.0003;edges=edges[valid];edge_length=edge_length[valid]
results=[]
for stride in (.55,.65,.75,.85,.95):
  for qdamp in (.45,1.):
    damp=.5
    r={'stride':stride,'damping':damp,'rotation_damping':qdamp,'max_skin_p99':0.,'min_z_cm':1e9}
    for c in m['clips']:
      if c['name'] not in sources:continue
      continuity={}
      for idx,(source_world,values) in enumerate(sources[c['name']]):
        world={n:x.copy() for n,x in source_world.items()}
        for ns in rig['chains'].values():
          foot=world[ns[3]];anchor=new_rest[ns[3]].translation.x;foot.translation.x=anchor+(foot.translation.x-anchor)*stride
        apply(arm,values);body=arm.pose.bones[rig['body']];body.location=stand[rig['body']][0].lerp(values[rig['body']][0],damp);body.rotation_quaternion=stand[rig['body']][1].slerp(values[rig['body']][1],qdamp);bpy.context.view_layer.update()
        drop=0.
        for key,ns in rig['chains'].items():
          hip=arm.pose.bones[ns[0]].matrix.translation;foot=world[ns[3]].translation;span=sum(lengths[key])-.003;horizontal=(hip.x-foot.x)**2+(hip.y-foot.y)**2;allowed=foot.z+math.sqrt(max(.00001,span*span-horizontal));drop=max(drop,hip.z-allowed)
        if drop>0:move(arm,rig['body'],(0,0,-drop));bpy.context.view_layer.update()
        for key,ns in rig['chains'].items():
          old=[world[n].translation.copy() for n in ns[:4]];old[0]=arm.pose.bones[ns[0]].matrix.translation.copy()
          solve_limb(arm,ns,key,old,lengths[key],centre,1.,continuity,world[ns[3]],'hare')
        xyz=points(meshes);p99=float(np.percentile(np.linalg.norm(xyz[edges[:,0]]-xyz[edges[:,1]],axis=1)/edge_length,99));low=float(xyz[:,2].min())*100
        if p99>r['max_skin_p99']:r['max_skin_p99']=p99;r['worst']=[c['suffix'],idx+1]
        r['min_z_cm']=min(r['min_z_cm'],low)
    print('HARE_STRIDE',r,flush=True);results.append(r);save(REV/'hare_stride_trials.json',results)
