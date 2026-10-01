import bpy,json,sys,math
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'corrections_20261001_r2'
sys.path.insert(0,str(REV/'scripts/reference_helpers'));sys.path.insert(0,str(ROOT/'refinement_20261001/scripts'))
from preview import setup
from refine_motion import reset,rotate
for slug,driver in [('stag_a','bone_15'),('ram','Head_0'),('red_fox','bone_29')]:
 out=ROOT/'Hearthward/animal_motion_20260930/assets/motion'/slug;m=json.loads((out/'animation_manifest.json').read_text('utf-8'));rig=m['rig'];L=1.6
 bpy.ops.wm.open_mainfile(filepath=str(out/f'AS_{slug}.blend'));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');arm.animation_data.action=None;scene=bpy.context.scene
 camera,_=setup(arm,L);scene.render.resolution_x=440;scene.render.resolution_y=460;scene.render.image_settings.file_format='PNG'
 target=Vector((.46,0,1.25 if slug in ('stag_a','ram') else .96))
 for angle in [-60,-45,-30,0,30,45,60]:
  reset(arm);rotate(arm,driver,(0,0,1),angle);bpy.context.view_layer.update()
  camera.location=target+Vector((L*.88,0,L*.06));camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler()
  p=REV/'inspection/head_trials'/slug/f'{angle}.png';p.parent.mkdir(parents=True,exist_ok=True);scene.render.filepath=str(p);bpy.ops.render.render(write_still=True)
 print('HEAD_TRIALS',slug,flush=True)

