"""Check final published files, pointers, and current QA fingerprints."""
from pathlib import Path
from urllib.parse import unquote,urlsplit
from html.parser import HTMLParser
import json,hashlib
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'refinement_20261001'
def read(p):return json.loads(p.read_text('utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
class Parser(HTMLParser):
 def __init__(self):super().__init__();self.assets=[];self.sections=0;self.videos=0;self.images=0;self.clips=0
 def handle_starttag(self,tag,attrs):
  a=dict(attrs)
  if tag=='section':self.sections+=1
  if tag=='video':self.videos+=1
  if tag=='img':self.images+=1
  if tag=='details' and a.get('class')=='clip':self.clips+=1
  for k in ('src','href'):
   if a.get(k):self.assets.append(a[k])
jobs=read(ROOT/'jobs.json');assert len(jobs)==14
clips=0;guides=[];numeric=[];native=[];integrity=read(REV/'source_integrity.json')
for j in jobs:
 out=Path(j['output']);m=read(out/'animation_manifest.json');q=read(out/'fbx_roundtrip_qa.json');u=read(out/'ue_import_qa.json');a=read(REV/'final'/f"{j['slug']}_audit.json")
 assert m['ue_import']=='PASS' and q['structural_pass'] and u['ok'] and u['physical_scale_verified']
 assert a['blend_sha256']==sha(out/f"AS_{j['slug']}.blend")
 assert q['skeletal_sha256']==u['skeletal_sha256']==sha(out/f"SK_{j['slug']}.fbx")
 assert [x['name'] for x in m['clips']]==[x['name'] for x in q['clips']]==[x['name'] for x in u['clips']]
 for c,x,y in zip(m['clips'],q['clips'],u['clips']):
  assert c['sha256']==x['file_sha256']==y['source_sha256']==sha(Path(c['file']))
  assert x['structural_pass'] and not x['review_flags'] and y['ok']
  assert x.get('fixed_hold_max_position_drift_m',0)<.001
 for c in a['clips']:
  assert c['max_skin_p99']<=1.8 and (c['ground_min_cm'] is None or c['ground_min_cm']>=-.5)
  assert c['max_stance_sliding_cm'] is None or c['max_stance_sliding_cm']<=2
 assert all(t['position_cm']<.1 and t['rotation_deg']<.1 for t in a['transitions'])
 g=Path(j['guidance']);text=g.read_text('utf-8')
 assert '文档版本：1.1' in text and '## 11. 2026-10-01' in text
 assert not any(s in text for s in ('仅指导文档，动作制作与接入均未执行','没有制作动作','没有生成动画、导入测试','均未创建'))
 assert j['guidance_sha256']==m['guidance_sha256']==sha(g)
 assert j['source_sha256']==sha(Path(j['source']))
 ir=next(x for x in integrity if x['slug']==j['slug']);assert ir['pass'] and ir['blend_sha256']==a['blend_sha256']
 clips+=len(m['clips']);guides.append(str(g))
assert clips==303
inventory=read(ROOT/'资产完整性清单.json')
count=0
for animal in inventory:
 for f in animal['files']:
  p=Path(f['path']);assert p.is_file() and p.stat().st_size==f['bytes'] and sha(p)==f['sha256'],str(p);count+=1
p=Parser();p.feed((ROOT/'动物动作预览.html').read_text('utf-8'))
missing=[]
for url in p.assets:
 part=urlsplit(url)
 if part.scheme or url.startswith('#'):continue
 file=(ROOT/unquote(part.path)).resolve()
 if not file.is_file():missing.append(url)
assert not missing,missing
assert (p.sections,p.videos,p.images,p.clips)==(14,46,909,303)
video=read(REV/'video_encoding_qa.json');assert len(video)==46
for v in video:assert v['pass'] and sha(Path(v['path']))==v['sha256']
history=read(REV/'historical_previews/index.json');assert len(history)==3
for v in history:assert sha(Path(v['archived']))==v['sha256'] and not Path(v['original']).is_file()
for s in read(REV/'script_provenance.json')['revision_scripts']:assert sha(Path(s['path']))==s['sha256']
backup=read(REV/'backup_index.json')
# Support the original backup index object's file list.
files=backup if isinstance(backup,list) else backup.get('files',[])
assert len(files)==474
for f in files:
 b=Path(f.get('backup',f.get('backup_path','')))
 assert b.is_file() and sha(b)==f['sha256'],str(b)
s=read(ROOT/'交付验收汇总.json');assert s['clips']==303 and s['native_current_verified_clips']==303 and s['numeric_review_flags_clips']==0
result={'pass':True,'animals':14,'clips':clips,'updated_guides':14,'current_inventory_files':count,'gallery_images':p.images,'gallery_videos':p.videos,'broken_gallery_links':missing,'verified_backups':len(files),'curve_refined_actions':s['curve_refined_actions'],'affected_actions_including_skin':s['affected_actions_including_skin'],'gameplay_integration':'NOT_RUN'}
(REV/'final_delivery_verification.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(result,ensure_ascii=False,indent=2))

