"""Snapshot the current incremental journal change and persistent test profile."""
import hashlib
import json
from pathlib import Path
import subprocess

QA = Path(__file__).resolve().parent
GAME = QA.parents[3]
before = QA / 'before'
before.mkdir(exist_ok=True)
names = ['Source/Hearthward/UI/HearthwardScreenContent.cpp',
         'Source/Hearthward/UI/HearthwardScreenJournalTest.inl',
         'README.md', 'TestClient/README.md', 'docs/tasks/TASK-053.json',
         'docs/handoffs/TASK-053.md']
for name in names:
    target = before / (name.replace('/', '__') + '.snapshot')
    if target.exists():
        raise RuntimeError('Snapshot already exists: ' + str(target))
    target.write_bytes((GAME / name).read_bytes())

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

profile = {p.relative_to(GAME).as_posix(): sha(p)
           for p in (GAME / 'TestClient/Profile').rglob('*') if p.is_file()}
(QA / 'profile-before.json').write_text(json.dumps(profile, indent=2), encoding='utf-8')
(QA / 'client-before.json').write_bytes((GAME / 'TestClient/client.json').read_bytes())
(QA / 'source-before.json').write_bytes((GAME / 'TestClient/Logs/latest-source.json').read_bytes())
context = dict(head=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=GAME, text=True).strip(),
               branch=subprocess.check_output(['git', 'branch', '--show-current'], cwd=GAME, text=True).strip(),
               source='用户：收集要素去掉物品之间的空行，使已记录物品上下相邻。',
               scope='Collection display, pagination and selection only; integrate into the existing root TestClient.',
               before_snapshots=len(names), persistent_profile_files=len(profile))
(QA / 'context.json').write_text(json.dumps(context, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps(context, ensure_ascii=False, indent=2))
