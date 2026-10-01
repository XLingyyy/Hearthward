"""Create reproducible animal jobs from the approved Markdown guidance."""
from pathlib import Path
import os, sys, json, re, hashlib

REPO = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(REPO))
RESOURCE = Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物')
os.environ['AAAGF_OUTPUT_ROOT'] = str(RESOURCE / '动作' / '制作成果')
from pipeline.common.paths import task_output_dir

def main():
    rig = json.loads((RESOURCE/'动物骨骼生成清单.json').read_text(encoding='utf-8-sig'))
    records = {r['slug']:r for r in rig['assets']}
    jobs=[]
    for doc in sorted((RESOURCE/'动作').glob('*.md')):
        content=doc.read_text(encoding='utf-8-sig')
        slug=re.search(r'资产标识 `([^`]+)`',content)[1]
        r=records[slug]
        dest=task_output_dir('Hearthward','motion',slug,run_id='animal_motion_20260930')
        dest.mkdir(parents=True,exist_ok=True)
        actions=[]
        for line in content.splitlines():
            if line.startswith('|') and re.search(r'`(?:Idle|Hover|[A-Z][a-z]+)',line) and 'P0' in line or (line.startswith('|') and ('| P1 |' in line or '| G |' in line)):
                cells=[s.strip() for s in line.strip('|').split('|')]
                if len(cells)!=7: continue
                actions.append({'cells':cells})
        # Keep complete source table rows so authoring never silently drops G/P1.
        section=content.split('## 4.')[1].split('## 5.')[0]
        rows=[]
        for line in section.splitlines():
            if not line.startswith('|'):continue
            cells=[s.strip() for s in line.strip('|').split('|')]
            if 'AN_' in cells[0]: rows.append(cells)
        job={'schema':'hearthward.animal.motion.job.v1','slug':slug,'name':r['name'],
             'game_id':'Hearthward','run_id':'animal_motion_20260930','task_id':slug,
             'source':str(RESOURCE/Path(r['rigged_model']).relative_to(RESOURCE)) if Path(r['rigged_model']).is_absolute() else str(RESOURCE/r['rigged_model']),
             'rig_task_id':r['rig_task_id'],'source_task_id':r['source_task_id'],
             'rig_type':r['recommended_rig_type'],'guidance':str(doc),
             'guidance_sha256':hashlib.sha256(doc.read_bytes()).hexdigest(),
             'output':str(dest),'action_rows':rows}
        # Resolve stale/moved manifest paths by the exact task directory.
        if not Path(job['source']).is_file():
            job['source']=str(RESOURCE/'rigged_outputs'/r['rig_task_id']/f'{slug}_rigged.fbx')
        job['source_sha256']=hashlib.sha256(Path(job['source']).read_bytes()).hexdigest()
        (dest/'job.json').write_text(json.dumps(job,ensure_ascii=False,indent=2),encoding='utf-8')
        jobs.append(job)
    root=RESOURCE/'动作'/'制作成果'
    (root/'jobs.json').write_text(json.dumps(jobs,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps([{'slug':j['slug'],'actions':[(r[0],r[2],r[3],r[4]) for r in j['action_rows']]} for j in jobs],ensure_ascii=False))

if __name__=='__main__':main()
