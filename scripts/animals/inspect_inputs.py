"""Read-only asset catalog and pre-existing workspace fingerprints."""
import json,hashlib,subprocess
from pathlib import Path
GAME=Path(__file__).resolve().parents[2]
from paths import SOURCE,load_jobs
ROOT=SOURCE
OUT=GAME/'docs/qa/TASK-051';OUT.mkdir(parents=True,exist_ok=True)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
status=subprocess.check_output(['git','status','--porcelain'],cwd=GAME,text=True)
prior=[]
for row in status.splitlines():
    p=GAME/row[3:]
    if p.is_file() and not p.as_posix().startswith((GAME/'docs/tasks/TASK-051').as_posix()):prior.append({'path':row[3:],'sha256':sha(p)})
baseline=OUT/'baseline.json'
if not baseline.exists():baseline.write_text(json.dumps({'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=GAME,text=True).strip(),'status':status,'files':prior},ensure_ascii=False,indent=2),encoding='utf-8')
rows=[]
for j in load_jobs():
    folder=Path(j['output']);m=json.loads((folder/'animation_manifest.json').read_text('utf-8'))
    clips=[{k:c[k] for k in ('suffix','kind','duration','fps','loop','hold')}|{'speed':c.get('reference_speed_mps',c.get('speed_mps'))} for c in m['clips']]
    row={'slug':j['slug'],'rig_axis':m['rig']['axis'],'source':j['source'],'folder':str(folder),'clips':clips};rows.append(row)
    print(j['slug'],m['rig']['axis'],[(c['suffix'],c['kind'],c['speed']) for c in clips])
(OUT/'source_catalog.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
