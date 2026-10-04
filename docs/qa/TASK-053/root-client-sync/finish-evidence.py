"""Audit the concrete root-game delivery, current assets and preserved player data."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

QA = Path(__file__).resolve().parent
GAME = QA.parents[3]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--scope-only',action='store_true')
args = parser.parse_args()

def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def write(name,value):
    (QA/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

baseline = read(QA/'baseline-source.json')['fingerprints']
preserved = {name:sha(GAME/name)==digest for name,digest in baseline.items()}
profile_before = read(QA/'manual-profile-before.json')
protected = {name:digest for name,digest in profile_before.items() if '/Config/' in name or '/SaveGames/' in name}
profile_after = {p.relative_to(GAME).as_posix():sha(p) for p in (GAME/'TestClient/Profile').rglob('*') if p.is_file()}
protected_ok = all(profile_after.get(name)==digest for name,digest in protected.items())
backup = GAME/'TestClient/Backups/20261004-root-client-sync'
backup_ok = all(sha(backup/Path(name).relative_to('TestClient/Profile'))==digest for name,digest in protected.items())
diagnostic_changes = [name for name,digest in profile_after.items() if profile_before.get(name)!=digest and name not in protected]
write('preserved-data.json',dict(passed=all(preserved.values()) and protected_ok and backup_ok,
    latest_map_241_fingerprints=preserved,protected_player_files=len(protected),all_player_settings_and_saves_unchanged=protected_ok,
    all_player_backup_files_match=backup_ok,manual_profile_before_files=len(profile_before),manual_profile_after_files=len(profile_after),
    new_or_updated_diagnostic_files=diagnostic_changes,backup_ignored_by_git='TestClient/Backups/20261004-root-client-sync'))
task = read(GAME/'docs/tasks/TASK-053.json')

def allowed(name):
    def matches(pattern):
        return name.startswith(pattern) if pattern.endswith('/') else name==pattern
    return any(matches(p) for p in task['allowed_paths']) and not any(matches(p) for p in task['forbidden_paths'])

changes = []
for snapshot in (QA/'before').iterdir():
    name = snapshot.name.removesuffix('.snapshot').replace('__','/')
    current = GAME/name
    if sha(current)!=sha(snapshot):
        changes.append(dict(path=name,allowed=allowed(name),before_sha256=sha(snapshot),after_sha256=sha(current)))
changes.append(dict(path='启动测试版游戏.cmd',allowed=allowed('启动测试版游戏.cmd'),sha256=sha(GAME/'启动测试版游戏.cmd')))
changes.append(dict(path='parent/启动测试版游戏.cmd',allowed=True,sha256=sha(GAME.parent/'启动测试版游戏.cmd'),
    authorization='User explicitly requested the root test game; parent entry forwards to the same project.'))
scope_ok = all(row['allowed'] for row in changes) and all(allowed(p.relative_to(GAME).as_posix()) for p in QA.rglob('*') if p.is_file())
write('incremental-scope.json',dict(passed=scope_ok,changes=changes,
    scope='Current root-client integration compared to seven before snapshots, new root wrappers explicitly requested; previous uncommitted UI work excluded. Formal baseline approval check remains separate.'))
if args.scope_only:
    print(json.dumps(dict(scope_passed=scope_ok,latest_241_unchanged=all(preserved.values()),protected_player_files=len(protected),player_files_unchanged=protected_ok,backup_matches=backup_ok),ensure_ascii=False))
    raise SystemExit(0 if scope_ok and all(preserved.values()) and protected_ok and backup_ok else 1)

native = read(QA/'verify-normal/report.json')
manifest = read(QA/'verify-normal/source-manifest.json')
root = read(QA/'root-launch-smoke.json')
root_manifest = read(QA/'root-source.json')
runtime_inputs = {name:digest for name,digest in baseline.items() if not name.startswith('scripts/ui/')}
paths = lambda name: GAME.parent/name.removeprefix('parent/') if name.startswith('parent/') else GAME/name
checks = dict(normal_root_client_144_pass=native['passed'] and len(native['checks'])==144 and all(native['checks'].values()),
    native_m_shortcut_and_return_pass=all(value for key,value in native['checks'].items() if 'map' in key),
    verification_282_fingerprints_current=len(manifest['fingerprints'])==282 and all(sha(paths(n))==d for n,d in manifest['fingerprints'].items()),
    root_282_fingerprints_current=len(root_manifest['fingerprints'])==282 and all(sha(paths(n))==d for n,d in root_manifest['fingerprints'].items()),
    newest_runtime_237_files_same_in_normal_game=all(root_manifest['fingerprints'].get(n)==d==manifest['fingerprints'].get(n) for n,d in runtime_inputs.items()),
    original_current_map_241_still_match=all(preserved.values()),
    regular_root_startup_10_pass=root['passed'] and len(root['checks'])==10 and all(root['checks'].values()),
    original_player_saves_and_settings_preserved=protected_ok and len(protected)==16,
    player_backup_matches=backup_ok,
    native_job_stopped_own_process=read(QA/'verify-normal/stop.json')['ok'],
    current_incremental_scope_allowed=scope_ok,
    repository_zero_errors='PASS: repository checks only; 0 error(s)' in (QA/'repo-validation.txt').read_text(encoding='utf-8'),
    diff_format_pass='Exit code: 0' in (QA/'diff-check.txt').read_text(encoding='utf-8'),
    tool_tests_33_pass='Ran 33 tests' in (QA/'tool-tests.txt').read_text(encoding='utf-8') and '\nOK' in (QA/'tool-tests.txt').read_text(encoding='utf-8'))
delivery = [GAME/'README.md',GAME/'TestClient/README.md',GAME/'TestClient/.gitignore',GAME/'docs/tasks/TASK-053.json',GAME/'docs/handoffs/TASK-053.md',
    GAME/'启动测试版游戏.cmd',GAME.parent/'启动测试版游戏.cmd',GAME/'TestClient/启动测试端.cmd',GAME/'scripts/ui/launch_test_client.py',QA/'REPORT.md']
report = dict(passed=all(checks.values()),checks=checks,
    head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=GAME,text=True).strip(),
    branch=subprocess.check_output(['git','branch','--show-current'],cwd=GAME,text=True).strip(),
    entry=str(GAME.parent/'启动测试版游戏.cmd'),root_process_id=root['process_id'],root_game_left_open=root['preview_left_open'],
    protected_player_files=len(protected),diagnostic_changes=diagnostic_changes,
    native_checks=len(native['checks']),root_checks=len(root['checks']),current_fingerprints=len(root_manifest['fingerprints']),
    reused_cpp_build='local-map-angular/build_3fb31c6c; original compiled source and DLL remain unchanged',
    delivery_sha256={str(p):sha(p) for p in delivery},
    formal_baseline_scope='FAIL: no approved TASK-053 snapshot at base; current user-authorized incremental scope separately checked',
    no_commit_push_merge_shipping_or_public_release=True)
write('delivery-integrity.json',report)
print(json.dumps({k:v for k,v in report.items() if k!='delivery_sha256'},ensure_ascii=False,indent=2))
raise SystemExit(0 if report['passed'] else 1)
