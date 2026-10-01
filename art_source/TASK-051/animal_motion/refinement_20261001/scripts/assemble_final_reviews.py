"""Assemble current poses and frames decoded from the actual delivered videos."""
from pathlib import Path
import json,hashlib,shutil
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'refinement_20261001'
FONT=r'C:\Windows\Fonts\msyh.ttc'
jobs=json.loads((ROOT/'jobs.json').read_text('utf-8'));by_slug={j['slug']:j for j in jobs}
for r in json.loads((REV/'最终精修检查/index.json').read_text('utf-8')):
 j=by_slug[r['slug']];out=Path(j['output']);m=json.loads((out/'animation_manifest.json').read_text('utf-8'))
 clips=[next(c for c in m['clips'] if c['name']==n) for n in r['actions']]
 sheet=Image.new('RGB',(900,75+220*len(clips)),'#1e242d');d=ImageDraw.Draw(sheet)
 d.text((12,12),j['name']+' / '+r['slug'],font=ImageFont.truetype(FONT,23),fill='white')
 d.text((12,44),'2026-10-01 current source: start / middle / end',font=ImageFont.truetype(FONT,17),fill='#c4ccd8')
 for row,c in enumerate(clips):
  y=75+220*row;d.text((10,y),c['suffix'],font=ImageFont.truetype(FONT,17),fill='white')
  for col,t in enumerate((0,50,100)):
   tile=Image.open(out/'review'/f"{c['suffix']}_{t}.png").convert('RGB').resize((290,190),Image.Resampling.LANCZOS);sheet.paste(tile,(col*300,y+25))
 sheet.save(r['path'],quality=92)
folder=REV/'连续视频检查';folder.mkdir(exist_ok=True);story=[]
for r in json.loads((REV/'video_encoding_qa.json').read_text('utf-8')):
 if not r['samples']:continue
 sheet=Image.new('RGB',(1000,462),'#1e242d');d=ImageDraw.Draw(sheet)
 d.text((12,12),r['slug']+' / '+r['name'],font=ImageFont.truetype(FONT,22),fill='white')
 d.text((12,43),'Decoded delivered MP4 / '+str(r['fps'])+' FPS',font=ImageFont.truetype(FONT,15),fill='#c4ccd8')
 for i,s in enumerate(r['samples']):
  x=(i%4)*250;y=75+(i//4)*190
  tile=Image.open(s['path']).convert('RGB').resize((250,165),Image.Resampling.LANCZOS);sheet.paste(tile,(x,y))
  d.text((x+8,y+165),f"{s['time_s']:.2f}s",font=ImageFont.truetype(FONT,15),fill='white')
 p=folder/(r['slug']+'_'+r['name']+'.jpg');sheet.save(p,quality=92)
 story.append({'slug':r['slug'],'name':r['name'],'path':str(p),'video_sha256':r['sha256']})
(folder/'index.json').write_text(json.dumps(story,ensure_ascii=False,indent=2),encoding='utf-8')
helper=Path(r'E:\AiAgent\XLingGame\GameFactory-3A\operators\gen_motion\funcs\animal_motion');copies=REV/'scripts/reference_helpers';copies.mkdir(exist_ok=True)
references=[]
for p in helper.glob('*.py'):
 dst=copies/p.name;shutil.copy2(p,dst);references.append({'original':str(p),'snapshot':str(dst),'sha256':hashlib.sha256(dst.read_bytes()).hexdigest()})
provenance={'blender_path':r'E:\AiAgent\XLingGame\GameFactory-3A\test_data\tools\blender-4.5.13-windows-x64\blender.exe','blender_version':'4.5.13 LTS','ue_version':'5.8.2','reference_helpers':references,'revision_scripts':[{'path':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in (REV/'scripts').glob('*.py')],'historical_probe_notice':'probe_* and fix_wolf_rest.py are diagnostic history. Final source generation is refine_motion.py; the latter old rest patch now skips the measured support pose.'}
(REV/'script_provenance.json').write_text(json.dumps(provenance,ensure_ascii=False,indent=2),encoding='utf-8')
print('24 current pose sheets; '+str(len(story))+' decoded-video storyboards; helper snapshots saved')

