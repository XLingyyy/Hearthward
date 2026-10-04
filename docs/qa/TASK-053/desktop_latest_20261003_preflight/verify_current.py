"""Run the four existing focused UI previews sequentially and bind them to the current DLL."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

GAME = Path(__file__).resolve().parents[4]
OUT = Path(__file__).parent
commands = [
    ('pause', ['scripts/ui/launch_pause_test.py', '--verify'], 'docs/qa/TASK-053/pause-preview', 'hud_pause_'),
    ('building', ['scripts/ui/launch_building_test.py', '--verify'], 'docs/qa/TASK-053/building-preview', 'hud_building_'),
    ('loading', ['scripts/ui/launch_loading_test.py', '--verify'], 'docs/qa/TASK-053/loading-preview', 'hud_loading_'),
    ('hud', ['docs/qa/TASK-053/hud-gloss/launch_verify.py'], 'docs/qa/TASK-053/hud-gloss', 'hud_gloss_'),
]
results = {}
for name, args, parent, prefix in commands:
    root = GAME / parent
    previous = {p.name for p in root.iterdir() if p.is_dir()}
    print('Checking current ' + name + ' UI...', flush=True)
    completed = subprocess.run([sys.executable, '-X', 'utf8', *args], cwd=GAME,
                               capture_output=True, text=True, encoding='utf-8')
    (OUT / (name + '-command-output.txt')).write_text(completed.stdout + completed.stderr, encoding='utf-8')
    current = [p for p in root.iterdir() if p.is_dir() and p.name.startswith(prefix) and p.name not in previous]
    entry = {'command': args, 'returncode': completed.returncode}
    if len(current) == 1 and (current[0] / 'report.json').is_file():
        path = current[0]
        report = json.loads((path / 'report.json').read_text(encoding='utf-8'))
        hashes = json.loads((path / 'fingerprints.json').read_text(encoding='utf-8'))
        matches = {p: hashlib.sha256((GAME / p).read_bytes()).hexdigest() == value for p, value in hashes.items()}
        entry.update({'run_id': path.name, 'evidence': path.relative_to(GAME).as_posix(),
                      'passed': report['passed'], 'checks': len(report['checks']),
                      'captures': len(report.get('captures', [])) + len(report.get('ui_captures', [])),
                      'fingerprints_match': matches,
                      'report_sha256': hashlib.sha256((path / 'report.json').read_bytes()).hexdigest()})
    results[name] = entry
    (OUT / 'development-validation.json').write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding='utf-8')
    if completed.returncode or not entry.get('passed') or not all(entry.get('fingerprints_match', {}).values()):
        print(json.dumps(entry, ensure_ascii=False), flush=True)
        raise SystemExit(1)
    print(name + ': PASS ' + str(entry['checks']) + ' checks; ' + str(entry['captures']) + ' PNGs.', flush=True)
print('All current focused UI checks passed.', flush=True)
