"""Bake exact-rig Guard locomotion and original hand-constrained bow actions."""
from pathlib import Path
import bpy,json,runpy,sys,math
from mathutils import Matrix,Vector
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT.parent))
from pipeline.common.paths import task_output_dir
a=bpy.app.driver_namespace['archer'];rig=a['rig'];rest=a['REST'];original=a['original']
rig.animation_data.action=None
for action in list(bpy.data.actions):
 if action.name.startswith('A_Archer_'):bpy.data.actions.remove(action)
scene=bpy.context.scene
out=task_output_dir('Hearthward','motion','TASK-095-archer','archer-motion-20261008');out.mkdir(parents=True,exist_ok=True)
report={'source':'Existing Guard clips exported by UE; original upper body and articulated bow authoring','clips':[]}
samples={}
for kind in ['Idle','Walk','Run']:
 before=set(bpy.data.objects)
 bpy.ops.import_scene.fbx(filepath=str(ROOT/'.agent-local/qa/TASK-095/guard-motion'/(kind+'.fbx')))
 imported=list(set(bpy.data.objects)-before);source=next(o for o in imported if o.type=='ARMATURE')
 source.hide_render=True
 frames=tuple(int(v) for v in source.animation_data.action.frame_range)
 root_rest=Matrix.LocRotScale(source.matrix_world.translation,source.matrix_world.to_quaternion(),Vector((1,1,1)))
 bone_rest={b.name:source.matrix_world@b.matrix_local for b in source.data.bones}
 rows=[]
 for frame in range(frames[0],frames[1]+1):
  scene.frame_set(frame);row={}
  root_pose=Matrix.LocRotScale(source.matrix_world.translation,source.matrix_world.to_quaternion(),Vector((1,1,1)))
  row['tripo::Root']=root_pose@root_rest.inverted()@rest['tripo::Root']
  for name in original:
   if name=='tripo::Root':continue
   n=name.replace('tripo::','');row[name]=source.matrix_world@source.pose.bones[n].matrix@bone_rest[n].inverted()@rest[name]
  rows.append(row)
 samples[kind]={'poses':rows,'fps':scene.render.fps}
 for obj in imported:bpy.data.objects.remove(obj,do_unlink=True)

export=runpy.run_path(str(ROOT/'art_source/TASK-051/tooling/GameFactory-3A/operators/gen_motion/funcs/animal_motion/author.py'))['export_fbx']
def smooth(t):
 t=max(0,min(1,t));return t*t*(3-2*t)
def set_source(row):
 a['reset']()
 for name in original:
  if name in ('tripo::Root','tripo::Spine_0') or name.startswith(('tripo::0_Left_Limb','tripo::1_Right_Limb')):
   rig.pose.bones[name].matrix=row[name]
   bpy.context.view_layer.update()
def key(frame):
 for p in rig.pose.bones:
  p.keyframe_insert('location',frame=frame);p.keyframe_insert('rotation_quaternion',frame=frame);p.keyframe_insert('scale',frame=frame)

for kind in ['Idle','Walk','Run','Shoot']:
 data=samples['Idle' if kind=='Shoot' else kind]
 # Keep the release visibility transition below one rendered 60 Hz frame.
 fps=120 if kind=='Shoot' else data['fps'];frames=265 if kind=='Shoot' else len(data['poses'])
 scene.render.fps=fps;scene.frame_start=1;scene.frame_end=frames
 rig.animation_data_create();action=bpy.data.actions.new('A_Archer_'+kind);action.use_fake_user=True;rig.animation_data.action=action
 checks=[]
 for f in range(1,frames+1):
  scene.frame_set(f)
  row=original if kind=='Shoot' else data['poses'][f-1];set_source(row)
  if kind=='Shoot':
   seconds=(f-1)/fps
   if seconds<=.6:
    lift=smooth(seconds/.32);draw=smooth((seconds-.16)/.44);recoil=0;loaded=int(seconds<.6)
   elif seconds<.78:
    lift=1;draw=1-smooth((seconds-.6)/.075);recoil=math.sin((seconds-.6)/.18*math.pi);loaded=0
   else:
    lift=1-smooth((seconds-.90)/.75);draw=0;recoil=0;loaded=0
   qa=a['pose'](draw,lift,recoil)
   rig.pose.bones['NockedArrow'].scale=Vector((1,1,1)) if loaded else Vector((.001,)*3)
  else:
   qa=a['pose'](0,0,0);rig.pose.bones['NockedArrow'].scale=Vector((.001,)*3)
  bpy.context.view_layer.update();key(f);checks.append(qa)
 scene.frame_set(1);export(rig,[],out/(action.name+'.fbx'),geometry=False,animation=True)
 report['clips'].append({'name':action.name,'fps':fps,'frames':frames,'duration':(frames-1)/fps,
  'max_grip_error_m':max(q['bow_grip_error'] for q in checks),'max_draw_hand_error_m':max(q['draw_hand_error'] for q in checks)})

rig.animation_data.action=None;a['reset']()
meshes=[o for o in scene.objects if o.type=='MESH']
export(rig,meshes,out/'SK_Archer_Combat.fbx',geometry=True,animation=False)
rig.animation_data.action=bpy.data.actions['A_Archer_Shoot'];scene.frame_start=1;scene.frame_end=265;scene.render.fps=120;scene.frame_set(73)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'art_source/TASK-095/Archer-Animated.blend'))
report['output_dir']=str(out);report['bone_count']=len(rig.data.bones);report['release_time_seconds']=.6
(out/'author-results.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('ARCHER_MOTION_BAKED',json.dumps(report))
