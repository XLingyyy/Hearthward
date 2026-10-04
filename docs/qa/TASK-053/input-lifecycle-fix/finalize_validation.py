"""Record current repository, source, backup and project-test-profile checks."""
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
passed &= run('tools', [sys.executable, '-X', 'utf8', '-m', 'unittest', 'discover', '-s', 'scripts/tests', '-v'])
passed &= run('diff', ['git', 'diff', '--check'])
run('baseline_scope', [sys.executable, '-X', 'utf8', 'scripts/validate_repo.py', '--task', 'TASK-053',
                      '--base', '4db5789184fe38e041d62a1e68c8517338ea0b01'])
checks['baseline_scope']['note'] = 'Historical missing approved TASK-053 snapshot at base; this is not reported as PASS.'

task = json.loads((GAME / 'docs/tasks/TASK-053.json').read_text(encoding='utf-8'))
modified = subprocess.check_output(['git', 'diff', '--name-only'], cwd=GAME, text=True, encoding='utf-8').splitlines()
untracked = subprocess.check_output(['git', '-c', 'core.quotepath=false', 'ls-files', '--others', '--exclude-standard'],
                                    cwd=GAME, text=True, encoding='utf-8').splitlines()
outside = [p for p in modified + untracked if not path_allowed(p, task['allowed_paths'], task['forbidden_paths'])]
checks['local_user_authorized_scope'] = dict(passed=not outside, checked=len(modified + untracked), outside=outside,
    authority='Latest explicit user request and earlier TASK-053 UI authorizations; preserves all existing work')
passed &= not outside

manifest = json.loads((OUT / 'final_compat_f3fd083d/source-manifest.json').read_text(encoding='utf-8'))
stale = [p for p, sha in manifest['fingerprints'].items() if hashlib.sha256((GAME / p).read_bytes()).hexdigest() != sha]
checks['source_and_dll_match_final_run'] = dict(passed=not stale, checked=len(manifest['fingerprints']), stale=stale)
passed &= not stale

removal = json.loads((OUT / 'desktop-removal.json').read_text(encoding='utf-8'))
bad_backups = [p['backup'] for p in removal['preserved']
               if hashlib.sha256(Path(p['backup']).read_bytes()).hexdigest().upper() != p['sha256']]
remaining_targets = [p['root'] for p in removal['inventory'] if Path(p['root']).exists()]
checks['desktop_removed_and_player_backup_intact'] = dict(passed=not bad_backups and not remaining_targets,
    checked=len(removal['preserved']), bad_backups=bad_backups, remaining_c_targets=remaining_targets)
passed &= not bad_backups and not remaining_targets

profile = GAME / 'TestClient/Runs/final_compat_f3fd083d/Saved'
profile_ok = bool(list((profile / 'Config').glob('*/GameUserSettings.ini'))) and bool(list((profile / 'SaveGames').rglob('*.hws')))
checks['test_profile_in_project'] = dict(passed=profile_ok, root=str(profile))
passed &= profile_ok
checks['passed_except_known_baseline_snapshot_limitation'] = passed
report_path.write_text(json.dumps(checks, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print(json.dumps(dict(passed=passed, source_files=len(manifest['fingerprints']), backups=len(removal['preserved']),
                      outside_scope=outside, stale_sources=stale, evidence=str(report_path)), ensure_ascii=False), flush=True)
raise SystemExit(0 if passed else 1)
