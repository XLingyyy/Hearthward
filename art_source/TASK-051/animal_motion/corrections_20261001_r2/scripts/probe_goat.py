import bpy,json,sys,math,numpy as np
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'corrections_20261001_r2'
sys.path.insert(0,str(REV/'scripts'));sys.path.insert(0,str(REV/'scripts/reference_helpers'));sys.path.insert(0,str(ROOT/'refinement_20261001/scripts'))
from preview import setup
from refine_motion import reset,snapshot,apply,move
from audit_refinement import points
from pose_tools import solve_three
out=ROOT/'Hearthward/animal_motion_20260930/assets/motion/goat';m=json.loads((out/'animation_manifest.json').read_text('utf-8'));rig=m['rig'];L=1.6
bpy.ops.wm.open_mainfile(filepath=str(out/'AS_goat.blend'));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];scene=bpy.context.scene
arm.animation_data.action=None;reset(arm);bpy.context.view_layer.update();stand=snapshot(arm);heads={p.name:p.matrix.translation.copy() for p in arm.pose.bones}
base=points(meshes);edges=np.concatenate([np.array([list(e.vertices) for e in o.data.edges]) for o in meshes]);length=np.linalg.norm(base[edges[:,0]]-base[edges[:,1]],axis=1);ok=length>.0003;edges=edges[ok];length=length[ok]
camera,_=setup(arm,L);scene.render.resolution_x=520;scene.render.resolution_y=400;results=[]
for drop in [.38,.40,.42,.44]:
 for dx in [.14,.20]:
  apply(arm,stand);move(arm,rig['body'],(0,0,-drop));bpy.context.view_layer.update()
  joints={}
  for k,ns in rig['chains'].items():
   hip=arm.pose.bones[ns[0]].matrix.translation.copy();foot=heads[ns[3]].copy();foot.x+=dx if k.startswith('F') else -.07
   angle=math.radians(45 if k.startswith('F') else 55);sign=-1 if k.startswith('F') else 1
   v=Vector((sign*math.sin(angle),0,-math.cos(angle)))
   normal=Vector((0,1 if k.startswith('F') else -1,0))
   joints[k]=[list(x) for x in solve_three(arm,ns,hip,foot,v,normal,heads)]
  xyz=points(meshes);ratios=np.linalg.norm(xyz[edges[:,0]]-xyz[edges[:,1]],axis=1)/length
  values={'drop':drop,'fore_dx':dx,'floor_cm':float(xyz[:,2].min()*100),'skin_p99':float(np.percentile(ratios,99)),'joints':joints};results.append(values)
  folder=REV/'inspection/goat_trials'/f'{drop}_{dx}';folder.mkdir(parents=True,exist_ok=True)
  for name,location in [('side',(0,-2.8,.75)),('front',(2.8,0,.8)),('top',(0,0,3))]:
   target=Vector((0,0,.34));camera.location=location;camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(folder/f'{name}.png');bpy.ops.render.render(write_still=True)
  print('GOAT_TRIAL',drop,dx,values['floor_cm'],values['skin_p99'],flush=True)
(REV/'goat_trials.json').write_text(json.dumps(results,indent=2),encoding='utf-8')

