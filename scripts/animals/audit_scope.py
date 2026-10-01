"""Check current request scope separately from preserved pre-existing dirty work."""
import json,hashlib,subprocess
from pathlib import Path
GAME=Path(__file__).resolve().parents[2];OUT=GAME/'docs/qa/TASK-051'
def git(*args):return subprocess.check_output(['git',*args],cwd=GAME,encoding='utf-8').strip()
def match(path,scope):return path.startswith(scope) if scope.endswith('/') else path==scope
def main():
    task=json.loads((GAME/'docs/tasks/TASK-051.json').read_text('utf-8'));baseline=json.loads((OUT/'baseline.json').read_text('utf-8'))
    previous=[line[3:].replace('\\','/') for line in baseline['status'].splitlines()]
    files=set(filter(None,git('diff','--name-only','-z',baseline['head']).split('\0')))|set(filter(None,git('ls-files','--others','--exclude-standard','-z').split('\0')))
    rows=[]
    for path in sorted(files):
        owned=any(match(path,s) for s in task['allowed_paths']) and not any(match(path,s) for s in task['forbidden_paths'])
        inherited=any(match(path,s) for s in previous)
        rows.append({'path':path,'classification':'TASK-051' if owned else 'inherited' if inherited else 'unexpected'})
    unchanged=[]
    for entry in baseline['files']:
        if any(match(entry['path'],s) for s in task['allowed_paths']):continue
        p=GAME/entry['path'];unchanged.append({'path':entry['path'],'unchanged':p.is_file() and hashlib.sha256(p.read_bytes()).hexdigest()==entry['sha256']})
    result={'scope_source':'2026-10-01 user request, current local TASK-051 allowed_paths; baseline task snapshot is not committed','head':git('rev-parse','HEAD'),'branch':git('branch','--show-current'),'files':rows,'prior_fingerprints':unchanged,'ok':all(r['classification']!='unexpected' for r in rows) and all(r['unchanged'] for r in unchanged)}
    (OUT/'scope_audit.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8');print('SCOPE_AUDIT',len(rows),result['ok'])
    if not result['ok']:raise SystemExit(1)
if __name__=='__main__':main()
