import bpy,json,sys,math,numpy as np
from pathlib import Path
from mathutils import Vector
from mathutils.kdtree import KDTree
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'corrections_20261001_r2'
sys.path.insert(0,str(REV/'scripts/reference_helpers'));sys.path.insert(0,str(ROOT/'refinement_20261001/scripts'))
from preview import setup
from refine_motion import reset,rotate
from audit_refinement import points
r={}
for slug,driver,cut,zlo,zhi in [('stag_a','bone_15',.42,1.10,1.36),('ram','Head_0',.5,1.30,1.56),('red_fox','bone_29',.46,.99,1.23)]:
 out=ROOT/'Hearthward/animal_motion_20260930/assets/motion'/slug;bpy.ops.wm.open_mainfile(filepath=str(out/f'AS_{slug}.blend'));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');arm.animation_data.action=None;scene=bpy.context.scene;meshes=[o for o in scene.objects if o.type=='MESH']
 reset(arm);rotate(arm,driver,(0,0,1),-60);bpy.context.view_layer.update()
 p=points(meshes);cloud=p[(p[:,0]>cut)&(p[:,2]>zlo)&(p[:,2]<zhi)]
 rng=np.random.default_rng(4);test=cloud[rng.choice(len(cloud),min(600,len(cloud)),replace=False)]
 tree=KDTree(len(cloud))
 for i,pt in enumerate(cloud):tree.insert(pt,i)
 tree.balance();scores=[]
 for deg in range(-14,15):
  theta=math.radians(deg);normal=np.array([-math.sin(theta),math.cos(theta),0]);center=cloud.mean(axis=0)
  for offset in np.arange(-.03,.0301,.005):
   plane=center+normal*offset;reflected=test-2*np.sum((test-plane)*normal,axis=1)[:,None]*normal
   d=np.array([tree.find(pt)[2] for pt in reflected]);scores.append((float(np.mean(np.sort(d*d)[:int(.9*len(d))])),deg,offset))
 best=min(scores);correction=-60-best[1] # applying negative residual heading
 r[slug]={'driver':driver,'correction_yaw_deg':float(correction),'face_symmetry_residual_before_deg':int(best[1]),'mirror_rms_mm':math.sqrt(best[0])*1000,'skull_samples':len(cloud),'region':{'x_min':cut,'z_min':zlo,'z_max':zhi}}
 reset(arm);rotate(arm,driver,(0,0,1),correction);bpy.context.view_layer.update()
 cam,_=setup(arm,1.6);scene.render.resolution_x=640;scene.render.resolution_y=640
 target=Vector((.55,0,1.35 if slug=='ram' else 1.30 if slug=='stag_a' else 1.10))
 for name,offset in [('front',(.9,0,.02)),('top',(0,0,1.2))]:
  cam.location=target+Vector(offset);cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(REV/'inspection'/f'{slug}_head_fit_{name}.png');bpy.ops.render.render(write_still=True)
 print('HEAD_FIT',slug,r[slug],flush=True)
(REV/'head_fit.json').write_text(json.dumps(r,indent=2),encoding='utf-8')

