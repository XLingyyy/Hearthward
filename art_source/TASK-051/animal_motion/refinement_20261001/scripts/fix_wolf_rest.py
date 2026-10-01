import bpy,json,sys,hashlib
from pathlib import Path
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'refinement_20261001'
sys.path.insert(0,str(REV/'scripts'))
from refine_motion import snapshot,apply,rotate,move,bake_frame,smooth,curves,evaluated_min_z,export_fbx,save
out=ROOT/'Hearthward/animal_motion_20260930/assets/motion/wolf'
m=json.loads((out/'animation_manifest.json').read_text('utf-8'));rig=m['rig']
if m.get('refinement_20261001',{}).get('rest_support') or m.get('refinement_20261001',{}).get('wolf_rest_tail_support_degrees')==50:sys.exit(0)
bpy.ops.wm.open_mainfile(filepath=str(out/'AS_wolf.blend'));bpy.context.preferences.filepaths.save_version=0
arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];scene=bpy.context.scene
records=json.loads((out/'refinement_20261001.json').read_text('utf-8'))
for c in m['clips']:
 if c['kind'] not in ('lie','rest','up'):continue
 old=bpy.data.actions[c['name']];arm.animation_data.action=old;poses=[]
 for f in range(1,c['frames']+1):scene.frame_set(f);poses.append(snapshot(arm))
 old.name=c['name']+'_prior_rest';action=bpy.data.actions.new(c['name']);action.use_fake_user=True;arm.animation_data.action=action
 for f,p in enumerate(poses,1):
  t=(f-1)/(c['frames']-1);a=smooth(t) if c['kind']=='lie' else 1-smooth(t) if c['kind']=='up' else 1
  scene.frame_set(f);apply(arm,p);rotate(arm,rig['tail'][0],(0,1,0),50*a);bpy.context.view_layer.update()
  move(arm,rig['body'],(0,0,.0003-evaluated_min_z(meshes)));bake_frame(arm,action,f)
 for fc in curves(action):
  for k in fc.keyframe_points:k.interpolation='LINEAR'
 bpy.data.actions.remove(old);scene.render.fps=c['fps'];scene.frame_start=1;scene.frame_end=c['frames'];scene.frame_set(1)
 export_fbx(arm,meshes,Path(c['file']),False,True);c['sha256']=hashlib.sha256(Path(c['file']).read_bytes()).hexdigest()
 c['refinement_20261001']['changes'].append('Tail rests clear of the belly support plane; body can settle instead of being lifted by the hanging tail tip')
 c['refinement_20261001']['qa']='NOT_RUN'
 for r in records['changed_files']:
  if r['name']==c['name']:r['after_sha256']=c['sha256']
first=m['clips'][0];arm.animation_data.action=bpy.data.actions[first['name']];scene.render.fps=first['fps'];scene.frame_start=1;scene.frame_end=first['frames'];scene.frame_set(1)
bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(out/'AS_wolf.blend'))
m['ue_import']='NOT_RUN';m['visual_qa']='pending final rest review'
m['refinement_20261001']['wolf_rest_tail_support_degrees']=50
save(out/'animation_manifest.json',m);save(out/'refinement_20261001.json',records)

