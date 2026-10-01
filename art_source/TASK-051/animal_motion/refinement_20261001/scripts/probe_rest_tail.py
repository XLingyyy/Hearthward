import bpy,json,sys,math
from pathlib import Path
import numpy as np
from mathutils import Vector
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'refinement_20261001'
sys.path.insert(0,str(REV/'scripts'))
from refine_motion import apply,snapshot,build_ik,bake_frame,rotate,move
from audit_refinement import points
from preview import setup
results={}
for slug in ('wolf','red_fox'):
 out=ROOT/'Hearthward/animal_motion_20260930/assets/motion'/slug;m=json.loads((out/'animation_manifest.json').read_text('utf-8'));rig=m['rig']
 bpy.ops.wm.open_mainfile(filepath=str(out/f'AS_{slug}.blend'));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];scene=bpy.context.scene
 c=m['clips'][0];arm.animation_data.action=bpy.data.actions[c['name']];scene.frame_set(1);stand=snapshot(arm);arm.animation_data.action=None
 apply(arm,stand);bpy.context.view_layer.update();base=points(meshes);length=rig['axis']['target_length_m']
 fore=sum((arm.pose.bones[rig['chains'][k][0]].matrix.translation for k in ('FL','FR')),Vector())/2
 hind=sum((arm.pose.bones[rig['chains'][k][0]].matrix.translation for k in ('BL','BR')),Vector())/2;cy=(fore.y+hind.y)/2;width=np.mean([abs(arm.pose.bones[ns[0]].matrix.translation.y-cy) for ns in rig['chains'].values()])
 mask=(base[:,0]>hind.x+length*.08)&(base[:,0]<fore.x-length*.12)&(abs(base[:,1]-cy)<width*.65)&(base[:,2]>length*.12)
 edges=[];off=0
 for obj in meshes:edges += [(a+off,b+off) for a,b in (e.vertices[:] for e in obj.data.edges)];off+=len(obj.data.vertices)
 edges=np.array(edges);le=np.linalg.norm(base[edges[:,0]]-base[edges[:,1]],axis=1);v=le>.0003;edges=edges[v];le=le[v]
 targets=build_ik(arm,rig,stand);drop=float(base[mask,2].min())-.001;setup(arm,length)
 scene.render.resolution_x=400;scene.render.resolution_y=300;folder=REV/'rest_trials_tail'/slug;folder.mkdir(parents=True,exist_ok=True);records=[]
 for k,(fs,hs) in enumerate([(0,0),(-.12,.1),(.12,.05),(-.06,.16)]):
  apply(arm,stand);move(arm,rig['body'],(0,0,-drop));rotate(arm,rig['tail'][0],(0,1,0),50 if slug=='wolf' else 35)
  if slug=='red_fox':
   rotate(arm,rig['tail'][1],(0,1,0),20);rotate(arm,rig['tail'][2],(0,1,0),10)
  for key,(ctl,con,eff,p,q) in targets.items():ctl.location=p+Vector((length*(fs if key.startswith('F') else hs),0,0));con.mute=False
  for step in range(6):
   bpy.context.view_layer.update()
   for ctl,con,eff,p,q in targets.values():
    pb=arm.pose.bones[eff];mat=q.to_matrix().to_4x4();mat.translation=pb.matrix.translation;pb.matrix=mat
   bpy.context.view_layer.update();ps=points(meshes);move(arm,rig['body'],(0,0,.001-float(ps[mask,2].min())))
  bpy.context.view_layer.update();ps=points(meshes);ratio=np.linalg.norm(ps[edges[:,0]]-ps[edges[:,1]],axis=1)/le
  rec={'trial':k,'front_shift':fs,'hind_shift':hs,'skin_p99':float(np.percentile(ratio,99)),'ground_cm':float(ps[:,2].min())*100,'belly_cm':float(ps[mask,2].min())*100}
  scene.render.filepath=str(folder/f'{k}.png');bpy.ops.render.render(write_still=True);records.append(rec)
 results[slug]=records
(REV/'rest_trials_tail/results.json').write_text(json.dumps(results,indent=2),encoding='utf-8')

