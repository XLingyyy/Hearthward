"""Preserve the current test profile and launcher before root-entry integration."""
import hashlib
import json
from pathlib import Path
import subprocess

QA = Path(__file__).resolve().parent
GAME = QA.parents[3]
before = QA/'before'
before.mkdir(exist_ok=True)
names = ['README.md','TestClient/README.md','TestClient/启动测试端.cmd','TestClient/.gitignore',
    'scripts/ui/launch_test_client.py','docs/tasks/TASK-053.json','docs/handoffs/TASK-053.md']
for name in names:
    target = before/(name.replace('/','__')+'.snapshot')
    if target.exists():
        raise RuntimeError('Before snapshot already exists: '+str(target))
    target.write_bytes((GAME/name).read_bytes())

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

profile = {p.relative_to(GAME).as_posix():sha(p) for p in (GAME/'TestClient/Profile').rglob('*') if p.is_file()}
(QA/'manual-profile-before.json').write_text(json.dumps(profile,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(QA/'client-state-before.json').write_bytes((GAME/'TestClient/client.json').read_bytes())
for p in (GAME/'TestClient/Profile/Saved').rglob('*'):
    if p.is_file() and any(part in ['Config','SaveGames'] for part in p.parts):
        target = GAME/'TestClient/Backups/20261004-root-client-sync'/p.relative_to(GAME/'TestClient/Profile')
        target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes(p.read_bytes())
(QA/'baseline-source.json').write_bytes((QA.parent/'local-map-water/verify_47a9ea03/source-manifest.json').read_bytes())
context = dict(head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=GAME,text=True).strip(),
    branch=subprocess.check_output(['git','branch','--show-current'],cwd=GAME,text=True).strip(),
    source='用户2026-10-04：将最新版更新导入项目根目录处的测试版游戏。',
    target='Current project Development client, TestClient; root CMD forwards to the existing persistent client.',
    new_root_entries=['E:/AiAgent/XLingGame/启动测试版游戏.cmd',str(GAME/'启动测试版游戏.cmd')],
    manual_profile_files=len(profile),snapshot_count=len(names))
(QA/'context.json').write_text(json.dumps(context,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(context,ensure_ascii=False,indent=2))
