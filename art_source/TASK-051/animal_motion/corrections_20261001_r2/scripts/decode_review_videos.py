"""Decode the delivered MP4 files through Blender's video sequencer."""
import bpy,json,hashlib
from pathlib import Path
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'corrections_20261001_r2'
records=[]
for j in json.loads((ROOT/'jobs.json').read_text('utf-8')):
 out=Path(j['output'])
 for category in ('loop_preview','refinement_preview','correction_preview'):
  index=out/category/'index.json'
  if not index.is_file():continue
  for v in json.loads(index.read_text('utf-8')):
   fps=v.get('native_fps',v.get('fps'))
   duration=v['cycle_seconds']*v['cycles'] if category=='loop_preview' else v['duration_s']
   name=v.get('name',v.get('action'));path=Path(v['path'])
   previous_scene=bpy.context.window.scene
   scene=bpy.data.scenes.new('DecodeReview');bpy.context.window.scene=scene;scene.render.fps=fps;scene.render.use_sequencer=True
   scene.render.resolution_x=400;scene.render.resolution_y=300;scene.render.resolution_percentage=100
   scene.render.image_settings.file_format='PNG'
   ed=scene.sequence_editor_create();strips=ed.strips if hasattr(ed,'strips') else ed.sequences
   movie=strips.new_movie(name,filepath=str(path),channel=1,frame_start=1)
   n=movie.frame_duration;actual_fps=movie.fps
   assert abs(actual_fps-fps)<.01,(name,actual_fps,fps)
   assert abs(n/fps-duration)<1.1/fps,(name,n/fps,duration)
   frames=[]
   if category in ('loop_preview','refinement_preview','correction_preview'):
    folder=REV/'video_decode'/j['slug']/name;folder.mkdir(parents=True,exist_ok=True)
    for k in range(8):
     limit=round(v['cycle_seconds']*fps) if category=='loop_preview' else round(v['duration_s']/v['cycles']*fps) if category=='correction_preview' and v.get('cycles',1)>1 else n;frame=1+round((limit-1)*k/7);scene.frame_set(frame);target=folder/f'{k}.png';scene.render.filepath=str(target)
     bpy.ops.render.render(write_still=True,scene=scene.name);frames.append({'frame':frame,'time_s':(frame-1)/fps,'path':str(target)})
   records.append({'slug':j['slug'],'name':name,'category':category,'path':str(path),'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'fps':actual_fps,'decoded_frames':n,'duration_s':n/fps,'expected_duration_s':duration,'samples':frames,'pass':True})
   print('VIDEO_QA',j['slug'],name,n,actual_fps,flush=True);bpy.context.window.scene=previous_scene;bpy.data.scenes.remove(scene)
(REV/'video_encoding_qa.json').write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8')
assert len(records)==64

