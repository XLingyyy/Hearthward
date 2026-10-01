"""Continuous, native-time previews of the corrected multi-part actions."""
import bpy,json,sys,hashlib
from pathlib import Path
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
REV=ROOT/'corrections_20261001_r2'
sys.path.insert(0,str(Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果\corrections_20261001_r2/scripts/reference_helpers')))
from preview import setup
SEQUENCES={
 'goat':{'rest_sequence':['LieDown','Rest','GetUp']},
 'black_bear':{'rear_sequence':['RearSniff_In','RearSniff_Hold','RearSniff_Out'],
               'swipe_left_recovery':['SwipeShort_L'],'swipe_right_recovery':['SwipeShort_R']},
 'wolf':{'howl_posture':['Howl'],'rest_sequence':['LieDown','Rest','GetUp']},
 'hen':{'scratch_then_peck':['ScratchPeck'],'preen':['Preen']},
 'pheasant':{'preen':['Preen'],'folded_wing_transition_candidate':['Takeoff','FlightShort','Land']},
 'red_fox':{'curl_sequence':['CurlDown','CurlRest','GetUp'],'pounce_visual':['PounceVisual'],'ears_alert':['AlertListen']},
 'carp':{'fishing_pose_sequence':['Hooked_In','Struggle','LiftOut','LandFlop','Settle','DisplayStill']},
 'crucian_carp':{'fishing_pose_sequence':['Hooked_In','Struggle','LiftOut','LandFlop','Settle','DisplayStill']},
 'catfish':{'fishing_pose_sequence':['Hooked_In','Struggle','LiftOut','LandFlop','Settle','DisplayStill']},
 'eel':{'fishing_pose_sequence':['Hooked_In','StruggleS','LiftSupported','LandWriggle','SettleArc','DisplayStill'],
        'wave_start_stop':['HoverWave','StartWave','UndulateCruise','StopWave','HoverWave']},
}

def encode(folder,n,fps,path):
 scene=bpy.data.scenes.new('RefinementSequence');scene.render.resolution_x=400;scene.render.resolution_y=300;scene.render.resolution_percentage=100
 scene.render.fps=fps;scene.render.image_settings.file_format='FFMPEG';scene.render.ffmpeg.format='MPEG4';scene.render.ffmpeg.codec='H264';scene.render.ffmpeg.constant_rate_factor='MEDIUM';scene.render.use_sequencer=True
 seq=scene.sequence_editor_create();strips=seq.strips if hasattr(seq,'strips') else seq.sequences
 strip=strips.new_image('continuous_source_frames',filepath=str(folder/'00001.png'),channel=1,frame_start=1)
 for i in range(2,n+1):strip.elements.append(f'{i:05}.png')
 scene.frame_start=1;scene.frame_end=n;scene.render.filepath=str(path);bpy.ops.render.render(animation=True,scene=scene.name);bpy.data.scenes.remove(scene)

args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
jobs=json.loads((ROOT/'jobs.json').read_text('utf-8'))
for job in jobs:
 slug=job['slug']
 if slug not in SEQUENCES or args and slug not in args:continue
 out=Path(job['output']);man=json.loads((out/'animation_manifest.json').read_text('utf-8'));source=out/f'AS_{slug}.blend'
 bpy.ops.wm.open_mainfile(filepath=str(source));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');setup(arm,man['rig']['axis']['target_length_m']);scene=bpy.context.scene;scene.render.resolution_x=400;scene.render.resolution_y=300
 records=[]
 for name,suffixes in SEQUENCES[slug].items():
  clips=[next(c for c in man['clips'] if c['suffix']==s) for s in suffixes]
  fps=max(c['fps'] for c in clips);folder=out/'refinement_preview'/name;folder.mkdir(parents=True,exist_ok=True)
  n=0;segments=[]
  for c in clips:
   start=n/fps;arm.animation_data.action=bpy.data.actions[c['name']]
   # Omit the duplicated shared endpoint, preserving the clip's native time.
   count=round(c['duration']*fps)
   for i in range(count):
    value=1+i/fps*c['fps'];frame=int(value);scene.frame_set(frame,subframe=value-frame)
    n+=1;scene.render.filepath=str(folder/f'{n:05}.png');bpy.ops.render.render(write_still=True)
   segments.append({'action':c['name'],'start_s':start,'end_s':n/fps,'source_fps':c['fps']})
  output=out/'refinement_preview'/(name+'.mp4');encode(folder,n,fps,output)
  records.append({'name':name,'path':str(output),'fps':fps,'duration_s':n/fps,'segments':segments,'source_blend_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'view':'three-quarter; in-place source animation, not gameplay footage'})
  print('SEQUENCE',slug,name,flush=True)
 (out/'refinement_preview'/'index.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
