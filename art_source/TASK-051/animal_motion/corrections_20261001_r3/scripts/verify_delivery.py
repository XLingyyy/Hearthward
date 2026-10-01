"""Current file/metadata consistency, untouched sources and immutable backup."""
import sys,os,re,json,shutil
from pathlib import Path
from urllib.parse import unquote
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import ROOT,REV,SLUGS,read,save,sha
from PIL import Image,ImageStat

jobs=read(ROOT/'jobs.json');inventory=[];rows=[];clips_total=0
video_qa=read(REV/'video_encoding_qa.json');sampled=[]
for v in video_qa:
    assert sha(v['path'])==v['sha256'] and v['pass']
    assert abs(v['duration_s']-v['expected_duration_s'])<1.1/v['fps']
    images=[Image.open(s['path']).convert('RGB') for s in v['samples']]
    assert all(max(ImageStat.Stat(i).stddev)>10 for i in images),v['path']+' blank decoded image'
    assert len({i.tobytes() for i in images})>1,v['path']+' static decoded video'
    sampled.append({'path':v['path'],'decoded_phase_images_nonblank_and_changed':True})

for j in jobs:
    slug=j['slug'];out=Path(j['output']);m=read(out/'animation_manifest.json');rig=read(out/'rig_update.json');fr=read(out/'fbx_roundtrip_qa.json');ur=read(out/'ue_import_qa.json');sk=out/f'SK_{slug}.fbx';source=out/f'AS_{slug}.blend';clip_rows=[]
    assert sha(j['source'])==j['source_sha256']
    assert sha(j['guidance'])==j['guidance_sha256']==m['guidance_sha256']
    assert rig['skeleton_signature']==m['rig']['skeleton_signature']
    assert rig['skin_qa']['unweighted_vertices']==0 and rig['skin_qa']['max_influences']<=8 and rig['skin_qa']['max_sum_error']<.0001
    assert fr['structural_pass'] and ur['ok'] and ur['physical_scale_verified']
    assert fr['skeletal_sha256']==ur['skeletal_sha256']==sha(sk)
    assert len(fr['clips'])==len(ur['clips'])==len(m['clips'])
    meta=read(out/'meta.json');assert meta['skeletal_fbx_path']==str(sk)
    for c in m['clips']:
        f=next(r for r in fr['clips'] if r['name']==c['name']);u=next(r for r in ur['clips'] if r['name']==c['name'])
        assert sha(c['file'])==c['sha256']==f['file_sha256']==u['source_sha256']
        assert c['skeleton_signature']==rig['skeleton_signature'] and not f['review_flags'] and u['ok']
        assert meta[c['name']+'_path']==c['file']
        for k in (0,50,100):
            p=out/'review'/f'{c["suffix"]}_{k}.png';im=Image.open(p);assert im.width>=400 and im.height>=300
        clip_rows.append({'name':c['name'],'sha256':c['sha256'],'pass':True})
    index=out/('gait_preview_r3' if slug in SLUGS else 'loop_preview')/'index.json'
    for v in read(index):assert v['source_blend_sha256']==sha(source) and any(q['path']==v['path'] and q['sha256']==sha(v['path']) for q in video_qa)
    if slug in SLUGS:
        target=read(REV/'final'/f'{slug}_targeted.json');native_match=read(REV/'final'/f'{slug}_native_source_match.json');pose=read(out/'review/source_record_20261001_r3.json')
        assert target['pass'] and native_match['core_pass'] and m['correction_20261001_r3']['qa']=='PASS'
        assert target['source_blend_sha256']==native_match['source_blend_sha256']==pose['source_blend_sha256']==sha(source)
        assert target['skeletal_fbx_sha256']==sha(sk) and target['geometry_topology_uv_preserved']
        assert sha(j['guidance'])==read(out/'job.json')['guidance_sha256']
        # Keep Blender's interim autosave beside the historical diagnostics.
        p=out/f'AS_{slug}.blend1'
        if p.is_file():
            dest=REV/'intermediate_blend_backups'/slug/p.name;dest.parent.mkdir(parents=True,exist_ok=True)
            assert p.resolve().is_relative_to(ROOT.resolve()) and dest.resolve().is_relative_to(REV.resolve())
            shutil.move(str(p),str(dest))
    selected=[source,sk,Path(j['guidance']),out/'animation_manifest.json',out/'rig_update.json',out/'rig_audit.json',out/'fbx_roundtrip_qa.json',out/'ue_import_qa.json',out/'meta.json',out/'交付与接入说明.md',index]+[Path(c['file']) for c in m['clips']]
    if slug in SLUGS:selected += [out/'source_integrity_20261001_r3.json',out/'correction_20261001_r3.json',out/'review/source_record_20261001_r3.json']
    selected += [Path(v['path']) for v in read(index)]
    inventory.append({'slug':slug,'revision':'2026-10-01 R3' if slug in SLUGS else 'unchanged verified prior delivery','files':[{'path':str(p),'bytes':p.stat().st_size,'sha256':sha(p)} for p in selected]})
    rows.append({'slug':slug,'clips':len(m['clips']),'files_current':True,'paired_skeleton_verified':True,'source_preserved':True,'guidance_current':True,'clip_checks':clip_rows});clips_total+=len(m['clips'])

html=(ROOT/'动物动作预览.html').read_text('utf-8');data=json.loads(html.split('const DATA=',1)[1].split(';\nconst el=',1)[0])
assert len(data)==14 and sum(len(a['clips']) for a in data)==303
paths=[]
for a in data:
    paths += [a[k] for k in ('blend','sk','guide','notes')]
    for c in a['clips']:paths += [c['file']]+c['images']
    for v in a['videos']:paths += [v['url'],v['poster']]
for p in paths:assert (ROOT/unquote(p)).resolve().is_file(),p
backup=read(REV/'backup_index.json')
for f in backup['files']:assert sha(f['backup'])==f['sha256']
assert clips_total==303 and len(video_qa)==58 and len([v for v in video_qa if v['slug'] in SLUGS])==46
save(ROOT/'资产完整性清单.json',inventory)
result={'revision':'2026-10-01 R3','animals':14,'clips':clips_total,'affected_paired_clips':189,'gait_actions':39,'gait_videos':58,'new_gait_videos':46,'backup_files_verified':len(backup['files']),'original_tripo_sources_preserved':True,'current_metadata_files_match':True,'preview_file_links_verified':len(paths),'decoded_videos_nonblank_and_animated':sampled,'rows':rows,'gameplay_integration':'NOT_RUN','pass':True}
save(REV/'final_delivery_verification.json',result)
print('FINAL_DELIVERY_VERIFIED',len(rows),clips_total,len(video_qa),len(backup['files']),flush=True)
