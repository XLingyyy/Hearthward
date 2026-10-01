import bpy,json,sys,numpy as np
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'refinement_20261001'
sys.path.insert(0,str(REV/'scripts'))
from refine_motion import apply,snapshot,rotate,move,evaluated_min_z
from audit_refinement import points
from preview import setup
out=ROOT/'Hearthward/animal_motion_20260930/assets/motion/red_fox'
m=json.loads((out/'animation_manifest.json').read_text('utf-8'));rig=m['rig']
bpy.ops.wm.open_mainfile(filepath=str(out/'AS_red_fox.blend'));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];scene=bpy.context.scene
arm.animation_data.action=bpy.data.actions['AN_red_fox_Idle'];scene.frame_set(1);base=points(meshes)
edges=[];off=0
for obj in meshes:edges += [(a+off,b+off) for a,b in (e.vertices[:] for e in obj.data.edges)];off+=len(obj.data.vertices)
edges=np.array(edges);le=np.linalg.norm(base[edges[:,0]]-base[edges[:,1]],axis=1);v=le>.0003;edges=edges[v];le=le[v]
arm.animation_data.action=bpy.data.actions['AN_red_fox_CurlRest'];scene.frame_set(1);rest=snapshot(arm);arm.animation_data.action=None
folder=REV/'rest_curl_trials';folder.mkdir(exist_ok=True);records=[]
setup(arm,rig['axis']['target_length_m']);scene.render.resolution_x=500;scene.render.resolution_y=375
for i,angles in enumerate(((0,0,0),(20,35,40),(35,55,45),(-35,-55,-45),(45,40,20))):
 apply(arm,rest)
 for n,degree in zip(rig['tail'],angles):rotate(arm,n,(0,0,1),degree)
 bpy.context.view_layer.update();move(arm,rig['body'],(0,0,max(0,.0003-evaluated_min_z(meshes))));bpy.context.view_layer.update()
 p=points(meshes);ratio=np.linalg.norm(p[edges[:,0]]-p[edges[:,1]],axis=1)/le
 records.append({'trial':i,'yaw':angles,'min_ground_cm':float(p[:,2].min())*100,'skin_p99':float(np.percentile(ratio,99))})
 scene.render.filepath=str(folder/f'{i}.png');bpy.ops.render.render(write_still=True)
(folder/'results.json').write_text(json.dumps(records,indent=2),encoding='utf-8')

