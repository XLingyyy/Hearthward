"""Render current delivery thumbnails and explicit front/side correction videos."""
import bpy,json,sys,hashlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'corrections_20261001_r2'
sys.path.insert(0,str(REV/'scripts/reference_helpers'))
from preview import setup
COLORS={'FL':(.08,.45,1,1),'FR':(1,.12,.16,1),'BL':(.08,.85,.25,1),'BR':(1,.6,.05,1)}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def encode(folder,frames,fps,path,cycles=5):
 scene=bpy.data.scenes.new('CorrectionVideo');scene.render.resolution_x=480;scene.render.resolution_y=360;scene.render.resolution_percentage=100;scene.render.fps=fps
 scene.render.image_settings.file_format='FFMPEG';scene.render.ffmpeg.format='MPEG4';scene.render.ffmpeg.codec='H264';scene.render.ffmpeg.constant_rate_factor='MEDIUM';scene.render.use_sequencer=True
 editor=scene.sequence_editor_create();strips=editor.strips if hasattr(editor,'strips') else editor.sequences
 for i in range(cycles):
  strip=strips.new_image('repeat_'+str(i),filepath=str(folder/frames[0]),channel=1,frame_start=1+i*len(frames))
  for f in frames[1:]:strip.elements.append(f)
 scene.frame_start=1;scene.frame_end=cycles*len(frames);scene.render.filepath=str(path);bpy.ops.render.render(animation=True,scene=scene.name);bpy.data.scenes.remove(scene)
args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
jobs=json.loads((ROOT/'jobs.json').read_text('utf-8'))
for j in jobs:
 slug=j['slug']
 if slug not in args:continue
 out=Path(j['output']);m=json.loads((out/'animation_manifest.json').read_text('utf-8'));rig=m['rig'];L=rig['axis'].get('body_reference_length_m',rig['axis']['target_length_m']);source=out/f'AS_{slug}.blend';digest=sha(source)
 bpy.ops.wm.open_mainfile(filepath=str(source));scene=bpy.context.scene;arm=next(o for o in scene.objects if o.type=='ARMATURE');height=max(v.co.z for o in scene.objects if o.type=='MESH' for v in o.data.vertices);cam,target=setup(arm,L);scene.render.resolution_x=480;scene.render.resolution_y=360
 # Update every pose thumbnail for these native scenes, including all head-retargeted actions.
 folder=out/'review';folder.mkdir(exist_ok=True);images=[]
 for c in m['clips'] if '--videos-only' not in args else []:
  arm.animation_data.action=bpy.data.actions[c['name']];scene.render.fps=c['fps']
  for frac in [0,.5,1]:
   f=1+round((c['frames']-1)*frac);scene.frame_set(f);p=folder/f'{c["suffix"]}_{round(frac*100)}.png';scene.render.filepath=str(p);bpy.ops.render.render(write_still=True);images.append({'action':c['name'],'frame':f,'path':str(p)})
 if images:(folder/'source_record_20261001_r2.json').write_text(json.dumps({'source_blend_sha256':digest,'images':images,'view':'actual native baked pose; three-quarter'},indent=2),encoding='utf-8')
 if '--thumbnails-only' in args:continue
 # Coloured foot markers are confined to review scenes and never saved in SK/AS.
 markers={}
 for k,color in COLORS.items():
  bpy.ops.mesh.primitive_uv_sphere_add(segments=12,ring_count=8,radius=L*.02);o=bpy.context.object;o.name='ReviewFoot_'+k;mat=bpy.data.materials.new(o.name);mat.diffuse_color=color;o.data.materials.append(mat);markers[k]=o
 records=[];run=next(c for c in m['clips'] if c['kind']=='run')
 for view,location in [('side',(0,-L*2,L*.43)),('front',(L*2,0,L*.43))]:
  target=Vector((0,0,height*.46));cam.location=location;cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=max(L*1.7,height*1.75)
  name=f'four_feet_{run["suffix"]}_{view}';folder=out/'correction_preview'/name;folder.mkdir(parents=True,exist_ok=True);frames=[];arm.animation_data.action=bpy.data.actions[run['name']]
  for f in range(1,run['frames']):
   scene.frame_set(f)
   for k,o in markers.items():
    ns=rig['chains'][k];p=arm.pose.bones[ns[3]].matrix.translation;o.location=p+Vector((0,0,-rig['restfeet'][k][2]+L*.012))
   file=f'{f:05}.png';scene.render.filepath=str(folder/file);bpy.ops.render.render(write_still=True);frames.append(file)
  path=out/'correction_preview'/f'{name}.mp4';encode(folder,frames,run['fps'],path)
  records.append({'name':name,'path':str(path),'fps':run['fps'],'duration_s':run['duration']*5,'cycles':5,'segments':[{'action':run['name'],'start_s':0,'end_s':run['duration']*5,'source_fps':run['fps']}],'source_blend_sha256':digest,'view':view,'review_overlays_only':{'FL':'blue','FR':'red','BL':'green','BR':'orange'}})
 for o in markers.values():bpy.data.objects.remove(o,do_unlink=True)
 if slug=='goat':
  for view,location in [('side',(0,-L*2,L*.43)),('front',(L*2,0,L*.43))]:
   target=Vector((0,0,height*.46));cam.location=location;cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=max(L*1.6,height*1.75)
   clips=[next(c for c in m['clips'] if c['suffix']==s) for s in ['LieDown','Rest','GetUp']];fps=max(c['fps'] for c in clips);name='rest_sequence_'+view;folder=out/'correction_preview'/name;folder.mkdir(exist_ok=True);frames=[];segments=[];n=0
   for c in clips:
    arm.animation_data.action=bpy.data.actions[c['name']];start=n/fps
    for i in range(round(c['duration']*fps)):
     v=1+i/fps*c['fps'];scene.frame_set(int(v),subframe=v-int(v));n+=1;file=f'{n:05}.png';scene.render.filepath=str(folder/file);bpy.ops.render.render(write_still=True);frames.append(file)
    segments.append({'action':c['name'],'start_s':start,'end_s':n/fps,'source_fps':c['fps']})
   path=out/'correction_preview'/f'{name}.mp4';encode(folder,frames,fps,path,1)
   records.append({'name':name,'path':str(path),'fps':fps,'duration_s':n/fps,'cycles':1,'segments':segments,'source_blend_sha256':digest,'view':view,'review_overlays_only':False})
 (out/'correction_preview/index.json').write_text(json.dumps(records,indent=2),encoding='utf-8');print('CORRECTION_REVIEW',slug,len(images),len(records),flush=True)

