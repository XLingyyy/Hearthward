"""Shared paths and helpers for the rigid-limb gait correction."""
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
REV = ROOT / 'corrections_20261001_r3'
R2 = ROOT / 'corrections_20261001_r2'
sys.path.insert(0, str(R2 / 'scripts/reference_helpers'))
sys.path.insert(0, str(R2 / 'scripts'))
SLUGS = ('stag_a', 'hare', 'goat', 'pig', 'wolf', 'black_bear', 'ram', 'red_fox')
GAITS = ('walk', 'trot', 'run', 'hop', 'start', 'stop')

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8'))

def save(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')

def jobs(args=()):
    return [j for j in read(ROOT / 'jobs.json') if j['slug'] in SLUGS and (not args or j['slug'] in args)]

def backup_folder(job):
    return REV / 'backup' / Path(job['output']).relative_to(ROOT.parent)
