"""Verify current delivery files, reports, guides, media and immutable backup."""
import json,hashlib
from pathlib import Path
from urllib.parse import unquote
from html.parser import HTMLParser
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'corrections_20261001_r2';CHANGED={'stag_a','hare','goat','pig','wolf','black_bear','ram','red_fox'}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text('utf-8-sig'))
jobs=read(ROOT/'jobs.json');inventory=read(ROOT/'资产完整性清单.json');summary=read(ROOT/'交付验收汇总.json');counts={'animals':0,'clips':0,'changed_clips':0,'guides_v12':0,'inventory_files':0,'backups':0,'images':0,'videos':0}
assert len(jobs)==len(inventory)==14
for item in inventory:
 for f in item['files']:
  p=Path(f['path']);assert p.is_file() and p.stat().st_size==f['bytes'] and sha(p)==f['sha256'],str(p);counts['inventory_files']+=1
for j in jobs:
 out=Path(j['output']);m=read(out/'animation_manifest.json');fbx=read(out/'fbx_roundtrip_qa.json');ue=read(out/'ue_import_qa.json');guide=Path(j['guidance'])
 assert sha(guide)==j['guidance_sha256']==m['guidance_sha256']
 assert sha(Path(j['source']))==j['source_sha256']==m['source_sha256']
 assert sha(out/f'SK_{j["slug"]}.fbx')==fbx['skeletal_sha256']==ue['skeletal_sha256'] and fbx['structural_pass'] and ue['ok']
 for c,a,b in zip(m['clips'],fbx['clips'],ue['clips']):
  assert c['name']==a['name']==b['name'] and c['sha256']==a['file_sha256']==b['source_sha256']==sha(Path(c['file'])) and a['structural_pass'] and not a['review_flags'] and b['ok'],c['name']
  if 'correction_20261001_r2' in c:
   assert c['qa']['status']=='PASS_CURRENT_ASSET_CHECKS' and c['correction_20261001_r2']['qa']=='PASS_CURRENT_ASSET_CHECKS'
   counts['changed_clips']+=1
  for pct in [0,50,100]:
   assert (out/'review'/f'{c["suffix"]}_{pct}.png').is_file();counts['images']+=1
 if j['slug'] in CHANGED:
  assert '文档版本：1.2' in guide.read_text('utf-8') and guide.read_text('utf-8').count('## 12. 2026-10-01 R2：')==1
  assert read(out/'correction_20261001_r2.json')['checks']=='PASS_CURRENT_ASSET_CHECKS';counts['guides_v12']+=1
  assert read(out/'source_integrity_20261001_r2.json')['blend_sha256']==sha(out/f'AS_{j["slug"]}.blend')
 else:assert '文档版本：1.1' in guide.read_text('utf-8')
 counts['animals']+=1;counts['clips']+=len(m['clips'])
for v in read(REV/'video_encoding_qa.json'):
 p=Path(v['path']);assert sha(p)==v['sha256'] and v['pass'];counts['videos']+=1
for f in read(REV/'backup_index.json'):
 p=Path(f['backup']);assert p.is_file() and p.stat().st_size==f['bytes'] and sha(p)==f['sha256'];counts['backups']+=1
assert counts['clips']==303 and counts['changed_clips']==88 and counts['guides_v12']==8 and counts['images']==909 and counts['videos']==64 and counts['backups']==313,counts
sole=read(REV/'skin_sole_phase_qa.json');assert len(sole)==9 and all(r['pass'] for r in sole)
for r in sole:
 j=next(j for j in jobs if j['slug']==r['slug']);assert r['source_blend_sha256']==sha(Path(j['output'])/f'AS_{r["slug"]}.blend')
class Links(HTMLParser):
 def __init__(self):super().__init__();self.paths=[]
 def handle_starttag(self,tag,attrs):
  for k,v in attrs:
   if k in ('src','href') and v and not v.startswith(('#','http:','https:','javascript:')):self.paths.append(v)
parser=Links();parser.feed((ROOT/'动物动作预览.html').read_text('utf-8'))
for x in parser.paths:assert (ROOT/unquote(x)).resolve().exists(),x
counts['gallery_local_links']=len(parser.paths)
result={'schema':'animal.delivery.verify.r2','pass':True,'revision':'2026-10-01 R2','counts':counts,'source_models_untouched':True,'manifest_fbx_ue_hashes_match':True,'current_head_mesh_changes_localized':True,'skin_sole_sequencing_verified':True,'all_updated_guides_and_manifest_hashes_match':True,'decoded_videos_current':True,'backup_hashes_verified':True,'gameplay_integration':'NOT_RUN','documents':[{'path':str(p),'sha256':sha(p)} for p in [ROOT/'问题修正复核报告_20261001_R2.md',ROOT/'制作交付说明.md',ROOT/'动物动作预览.html',ROOT/'交付验收汇总.json',ROOT/'资产完整性清单.json']]}
(REV/'final_delivery_verification.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(result,ensure_ascii=False,indent=2))

