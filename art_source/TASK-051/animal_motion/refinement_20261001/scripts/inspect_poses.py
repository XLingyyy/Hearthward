"""Focused, labelled multi-time and side-view render for refinement review."""
import bpy,json,sys
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
REV=ROOT/'refinement_20261001'
sys.path.insert(0,r'E:\AiAgent\XLingGame\GameFactory-3A\operators\gen_motion\funcs\animal_motion')
from preview import setup
SELECT={
 'goat':['Rest','RunFlee'],
 'black_bear':['Rest','RearSniff_Hold','SwipeShort_L','Lope'],
 'wolf':['Rest','Gallop','Howl'],
 'red_fox':['CurlRest','RunFlee','AlertListen','PounceVisual'],
 'hen':['ScratchPeck','Preen'],
 'pheasant':['Scratch','Preen'],
 'stag_a':['Rest','LookAround'], 'pig':['Rest'], 'ram':['Rest'],
 'hare':['Scan','Collapse_L'],
 'carp':['Turn_L','LiftOut'], 'crucian_carp':['Turn_L','LiftOut'],
 'catfish':['Turn_L','LiftOut'], 'eel':['Turn_L','LiftSupported'],
}
args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
phase=args.pop(0) if args else 'pilot'
jobs=json.loads((ROOT/'jobs.json').read_text('utf-8'))
for job in jobs:
 if args and job['slug'] not in args:continue
 slug=job['slug'];out=Path(job['output']);man=json.loads((out/'animation_manifest.json').read_text('utf-8'))
 source=REV/'backup'/out.relative_to(ROOT.parent)/f'AS_{slug}.blend' if phase=='before' else out/f'AS_{slug}.blend'
 bpy.ops.wm.open_mainfile(filepath=str(source));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
 length=man['rig']['axis']['target_length_m'];cam,target=setup(arm,length);scene=bpy.context.scene
 scene.render.resolution_x=360;scene.render.resolution_y=270
 folder=REV/'inspection'/phase/slug;folder.mkdir(parents=True,exist_ok=True)
 for suffix in SELECT[slug]:
  c=next((c for c in man['clips'] if c['suffix']==suffix),None)
  if c is None:continue
  arm.animation_data.action=bpy.data.actions[c['name']];scene.render.fps=c['fps']
  for frac in (0,.25,.5,.75,1):
   scene.frame_set(1+round((c['frames']-1)*frac))
   scene.render.filepath=str(folder/(suffix+'_'+str(round(frac*100))+'.png'));bpy.ops.render.render(write_still=True)
  cam.location=Vector((0,-length*2.0,length*.45));cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler()
  scene.frame_set(1+round((c['frames']-1)*.5));scene.render.filepath=str(folder/(suffix+'_side.png'));bpy.ops.render.render(write_still=True)
  cam.location=Vector((0,length*2.0,length*.6));cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler()
  scene.render.filepath=str(folder/(suffix+'_opposite.png'));bpy.ops.render.render(write_still=True)
  cam.location=Vector((length*.75,-length*1.65,length*.9));cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler()
  print('INSPECT',slug,suffix,flush=True)
