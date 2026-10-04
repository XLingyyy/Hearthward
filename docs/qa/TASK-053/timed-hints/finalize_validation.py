"""Bind the two-second hint result and lifecycle regression to the current source."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

GAME = Path(__file__).resolve().parents[4]
OUT = Path(__file__).parent
sys.path.insert(0, str(GAME / 'scripts'))
from validate_repo import path_allowed

report_path = OUT / 'repository-checks.json'
report_path.write_text('{}\n', encoding='utf-8')
checks = {}


def run(name, command):
    result = subprocess.run(command, cwd=GAME, capture_output=True, text=True, encoding='utf-8', errors='replace')
    checks[name] = dict(command=command, returncode=result.returncode, stdout=result.stdout, stderr=result.stderr)
    return result.returncode == 0


passed = run('repository', [sys.executable, '-X', 'utf8', 'scripts/validate_repo.py'])
passed &= run('diff', ['git', 'diff', '--check'])
run('baseline_scope', [sys.executable, '-X', 'utf8', 'scripts/validate_repo.py', '--task', 'TASK-053',
                      '--base', '4db5789184fe38e041d62a1e68c8517338ea0b01'])
checks['baseline_scope']['note'] = 'Historical missing approved TASK-053 snapshot at base; not reported as PASS.'
checks['tools'] = json.loads((OUT / 'tool-tests.json').read_text(encoding='utf-8'))
passed &= checks['tools']['returncode'] == 0

task = json.loads((GAME / 'docs/tasks/TASK-053.json').read_text(encoding='utf-8'))
modified = subprocess.check_output(['git', '-c', 'core.quotepath=false', 'diff', '--name-only'], cwd=GAME, text=True, encoding='utf-8').splitlines()
untracked = subprocess.check_output(['git', '-c', 'core.quotepath=false', 'ls-files', '--others', '--exclude-standard'],
                                    cwd=GAME, text=True, encoding='utf-8').splitlines()
outside = [p for p in modified + untracked if not path_allowed(p, task['allowed_paths'], task['forbidden_paths'])]
checks['local_user_authorized_scope'] = dict(passed=not outside, checked=len(modified + untracked), outside=outside)
passed &= not outside

for label, directory in [('hints', OUT / 'verify_7669336d'),
                         ('lifecycle', OUT.parent / 'input-lifecycle-fix/timed_hints_lifecycle_2853df39')]:
    manifest = json.loads((directory / 'source-manifest.json').read_text(encoding='utf-8'))
    stale = [p for p, sha in manifest['fingerprints'].items() if hashlib.sha256((GAME / p).read_bytes()).hexdigest() != sha]
    result = json.loads((directory / 'report.json').read_text(encoding='utf-8'))
    checks[label] = dict(passed=result['passed'] and not stale, checked_files=len(manifest['fingerprints']),
                         passed_checks=sum(result['checks'].values()), stale=stale)
    passed &= checks[label]['passed']

native = json.loads((OUT / 'native_1369910b/summary.json').read_text(encoding='utf-8'))
checks['native_rules'] = native
passed &= all(r['ok'] and r['failed'] == 0 for r in native.values())
checks['passed_except_known_baseline_snapshot_limitation'] = passed
report_path.write_text(json.dumps(checks, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print(json.dumps(dict(passed=passed, outside_scope=outside, hints=checks['hints'], lifecycle=checks['lifecycle'],
                      evidence=str(report_path)), ensure_ascii=False), flush=True)
raise SystemExit(0 if passed else 1)
