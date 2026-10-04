"""Bind the final compact journal delivery to native evidence and the authorized increment."""
import difflib
import hashlib
import json
from pathlib import Path
import subprocess

QA = Path(__file__).resolve().parent
GAME = QA.parents[3]

def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def write(name, value):
    (QA / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')

task = read(GAME / 'docs/tasks/TASK-053.json')
matches = lambda name, pattern: name.startswith(pattern) if pattern.endswith('/') else name == pattern
allowed = lambda name: any(matches(name, p) for p in task['allowed_paths']) and not any(matches(name, p) for p in task['forbidden_paths'])
changes = []
for snapshot in (QA / 'before').iterdir():
    name = snapshot.name.removesuffix('.snapshot').replace('__', '/')
    if sha(GAME / name) == sha(snapshot):
        continue
    changes.append(dict(path=name, allowed=allowed(name), before_sha256=sha(snapshot), after_sha256=sha(GAME / name)))
    if name.startswith('Source/'):
        diff = difflib.unified_diff(snapshot.read_text(encoding='utf-8').splitlines(keepends=True),
            (GAME / name).read_text(encoding='utf-8').splitlines(keepends=True), fromfile='before/' + name, tofile=name)
        (QA / (Path(name).name + '.diff')).write_text(''.join(diff), encoding='utf-8')
scope_ok = all(row['allowed'] for row in changes) and all(allowed(p.relative_to(GAME).as_posix()) for p in QA.rglob('*') if p.is_file())
write('incremental-scope.json', dict(passed=scope_ok, changes=changes,
    scope='Current six-file snapshots and journal-compact QA directory; prior session changes excluded. Explicit latest collection/root-client user authorization. Formal baseline approval remains separate.'))

latest = read(QA / 'latest-verification.json')
native_dir = QA / latest['run_id']
native = read(native_dir / 'report.json')
normal = read(QA / 'verify-normal/report.json')
root = read(QA / 'root-smoke.json')
root_source = read(QA / 'root-source.json')['fingerprints']
normal_source = read(QA / 'verify-normal/source-manifest.json')['fingerprints']
previous = read(QA / 'source-before.json')['fingerprints']
compiled = read(QA / 'compiled-source.json')
paths = lambda n: GAME.parent / n.removeprefix('parent/') if n.startswith('parent/') else GAME / n
changed_inputs = [n for n, d in previous.items() if root_source.get(n) != d]
expected_changes = {'Source/Hearthward/UI/HearthwardScreenContent.cpp',
                    'Source/Hearthward/UI/HearthwardScreenJournalTest.inl', 'Binaries/Win64/UnrealEditor-Hearthward.dll'}
checks = dict(development_build_success=read(QA / 'build.json')['ok'],
    all_230_compiled_inputs_current=len(compiled)==230 and all(sha(GAME / n)==d for n,d in compiled.items()),
    native_journal_124_pass=native['passed'] and len(native['checks'])==124 and all(native['checks'].values()),
    native_inputs_current=all(sha(GAME / n)==d for n,d in read(native_dir / 'fingerprints.json').items()),
    normal_root_input_144_pass=normal['passed'] and len(normal['checks'])==144 and all(normal['checks'].values()),
    normal_282_inputs_current=len(normal_source)==282 and all(sha(paths(n))==d for n,d in normal_source.items()),
    actual_root_startup_11_pass=root['passed'] and len(root['checks'])==11 and all(root['checks'].values()),
    root_282_inputs_current=len(root_source)==282 and all(sha(paths(n))==d for n,d in root_source.items()),
    only_two_ui_files_and_dll_changed=set(changed_inputs)==expected_changes,
    all_279_other_runtime_inputs_same=len(previous)==282 and len(changed_inputs)==3,
    all_map_art_and_ui_data_same=all(root_source.get(n)==d for n,d in previous.items() if n.startswith('Resources/')),
    original_17_profile_files_preserved_at_actual_startup=root['original_protected_profile_files']==17 and root['checks']['original_player_settings_and_save_files_same'],
    original_profile_and_pool_retained=root['checks']['persistent_profile_same'] and root['checks']['persistent_save_pool_same'],
    native_jobs_stopped_own_process=read(native_dir / 'stop.json')['ok'] and read(QA / 'verify-normal/stop.json')['ok'],
    incremental_scope_allowed=scope_ok,
    repository_zero_errors='PASS: repository checks only; 0 error(s)' in (QA / 'repo-validation.txt').read_text(encoding='utf-8'),
    tools_33_pass='Ran 33 tests' in (QA / 'tool-tests.txt').read_text(encoding='utf-8') and '\nOK' in (QA / 'tool-tests.txt').read_text(encoding='utf-8'),
    diff_format_pass='Exit code: 0' in (QA / 'diff-check.txt').read_text(encoding='utf-8'))
captures = {name: sha(native_dir / name) for name in native['captures']}
delivery = ['README.md', 'TestClient/README.md', 'docs/tasks/TASK-053.json', 'docs/handoffs/TASK-053.md']
result = dict(passed=all(checks.values()), checks=checks, branch=task['branch'],
    head=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=GAME, text=True).strip(),
    root_entry=str(GAME.parent / '启动测试版游戏.cmd'), build='build.json', native_run=latest['run_id'],
    changed_runtime_inputs=changed_inputs, unchanged_runtime_inputs=len(previous)-len(changed_inputs),
    captures=captures, delivery_sha256={n: sha(GAME / n) for n in delivery}, report_sha256=sha(QA / 'REPORT.md'),
    formal_baseline_scope='FAIL: no approved TASK-053 snapshot at HEAD; current explicitly authorized increment separately passed',
    no_commit_push_merge_shipping_or_public_release=True)
write('delivery-integrity.json', result)
print(json.dumps(dict(passed=result['passed'], checks=len(checks), failed=[n for n,v in checks.items() if not v],
    native_checks=len(native['checks']), normal_checks=len(normal['checks']), root_checks=len(root['checks']),
    captures=len(captures), changed_inputs=changed_inputs), ensure_ascii=False, indent=2))
raise SystemExit(0 if result['passed'] else 1)
