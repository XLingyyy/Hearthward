"""Bind the latest water-colour delivery to fresh runtime and pixel evidence."""
import hashlib
import argparse
import json
from pathlib import Path
import subprocess

QA = Path(__file__).resolve().parent
GAME = QA.parents[3]
RUN = QA / 'verify_47a9ea03'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--scope-only',action='store_true')
args = parser.parse_args()

def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def write(name,value):
    (QA/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

task = read(GAME/'docs/tasks/TASK-053.json')
old_fingerprints = read(QA/'baseline-source.json')['fingerprints']
changed_render_files = {'scripts/ui/realistic_local_map.py','scripts/ui/draw_local_map.py',
    'scripts/ui/launch_map_test.py','Resources/UI/Art/map-local-terrain.png'}
preserved_checks = {name:sha(GAME/name)==digest for name,digest in old_fingerprints.items()
    if name not in changed_render_files}
old_scene = read(QA.parent/'local-map-angular/verify_639ec35e/scene.json')
scene = read(RUN/'scene.json')
scene_keys = ['samples','spacingM','originM','heightsM','spawnCm','trees','rocks','water','houses']
scene_checks = {name:scene[name]==old_scene[name] for name in scene_keys}
write('scene-comparison.json',dict(passed=all(scene_checks.values()),checks=scene_checks))
preserved_checks['current_scene_unchanged'] = all(scene_checks.values())
write('preserved-data.json',dict(passed=all(preserved_checks.values()),checks=preserved_checks,
    water_only_expected_changes=sorted(changed_render_files),preserved_fingerprints=len(preserved_checks)-1))

def matches(name,pattern):
    return name.startswith(pattern) if pattern.endswith('/') else name==pattern

changes = []
for snapshot in sorted((QA/'before').iterdir()):
    name = snapshot.name.removesuffix('.snapshot').replace('__','/')
    external = name.startswith('parent/')
    path = GAME.parent/name.removeprefix('parent/') if external else GAME/name
    if sha(path)!=sha(snapshot):
        allowed = name in ['parent/地图测试版_预览.png','parent/地图测试版_移动验证.png'] if external else any(matches(name,p) for p in task['allowed_paths']) and not any(matches(name,p) for p in task['forbidden_paths'])
        changes.append(dict(path=name,allowed=allowed,before_sha256=sha(snapshot),after_sha256=sha(path)))
qa_paths = [p.relative_to(GAME).as_posix() for p in QA.rglob('*') if p.is_file()]
qa_allowed = all(any(matches(name,p) for p in task['allowed_paths']) and not any(matches(name,p) for p in task['forbidden_paths']) for name in qa_paths)
scope_ok = all(row['allowed'] for row in changes) and qa_allowed
write('incremental-scope.json',dict(passed=scope_ok,changes=changes,snapshot_count=len(list((QA/'before').iterdir())),
    qa_file_count=len(qa_paths),new_qa_folder_allowed=qa_allowed,
    scope='Latest user-authorized water-colour changes against before snapshots; parent preview files explicitly requested. Previous uncommitted work excluded. This does not replace the failed formal baseline approval check.'))
if args.scope_only:
    print(json.dumps(dict(scope_passed=scope_ok,preserved=all(preserved_checks.values()),scene_checks=len(scene_checks)),ensure_ascii=False))
    raise SystemExit(0 if scope_ok and all(preserved_checks.values()) else 1)
manifest = read(RUN/'source-manifest.json')
mismatches = [name for name,digest in manifest['fingerprints'].items() if sha(GAME/name)!=digest]
profile_before = read(QA/'manual-profile-before.json')
profile_after = {p.relative_to(GAME).as_posix():sha(p) for p in (GAME/'TestClient/Profile').rglob('*') if p.is_file()}
copies = {GAME/'地图测试版_预览.png':RUN/'map-1600x1000.png',
    GAME.parent/'地图测试版_预览.png':RUN/'map-1600x1000.png',
    GAME.parent/'地图测试版_移动验证.png':RUN/'map-separated-flames.png'}
root = read(QA/'root-launch-smoke.json')
native = read(RUN/'report.json')
image = read(QA/'final-audit.json')
water = read(QA/'water-audit.json')
reused_build = read(QA.parent/'local-map-angular/build_3fb31c6c/build.json')
head = subprocess.check_output(['git','rev-parse','HEAD'],cwd=GAME,text=True).strip()
branch = subprocess.check_output(['git','branch','--show-current'],cwd=GAME,text=True).strip()
checks = dict(successful_existing_build_and_current_cpp_dll_unchanged=reused_build['ok'] and not reused_build['diagnostics'] and water['checks']['compiled_source_and_dll_unchanged'],
    native_56_pass=native['passed'] and len(native['checks'])==56 and all(native['checks'].values()),
    image_29_pass=image['passed'] and len(image['checks'])==29 and all(image['checks'].values()),
    water_12_pass=water['passed'] and len(water['checks'])==12 and all(water['checks'].values()),
    current_241_fingerprints_match=len(manifest['fingerprints'])==241 and not mismatches,
    other_source_assets_config_and_scene_preserved=all(preserved_checks.values()),
    manual_profile_59_unchanged=profile_before==profile_after and len(profile_before)==59,
    root_pngs_match_current_native_captures=all(sha(dest)==sha(src) for dest,src in copies.items()),
    root_launcher_smoke_pass=root['passed'] and all(root['checks'].values()),
    repository_zero_errors='PASS: repository checks only; 0 error(s)' in (QA/'repo-validation.txt').read_text(encoding='utf-8'),
    diff_format_pass='Exit code: 0' in (QA/'diff-check.txt').read_text(encoding='utf-8'),
    tool_tests_33_pass='Ran 33 tests' in (QA/'tool-tests.txt').read_text(encoding='utf-8') and '\nOK' in (QA/'tool-tests.txt').read_text(encoding='utf-8'),
    current_incremental_scope_allowed=scope_ok,
    head_matches_manifest=head==manifest['head'])
delivery = [GAME/'README.md',GAME/'TestClient/README.md',GAME/'docs/handoffs/TASK-053.md',
    GAME/'docs/tasks/TASK-053.json',QA/'REPORT.md',QA/'water-audit.py',QA/'finish-evidence.py',QA/'root-launch-audit.py']+list(copies)
report = dict(passed=all(checks.values()),checks=checks,head=head,branch=branch,
    reused_build='local-map-angular/build_3fb31c6c',new_cpp_build='NOT_RUN: no compiled source or DLL changes',
    native_run=RUN.name,root_run=root['run'],root_process_id=root['process_id'],root_responsiveness=root['responsiveness_observation'],
    native_checks=len(native['checks']),image_checks=len(image['checks']),water_checks=len(water['checks']),
    fingerprints=len(manifest['fingerprints']),mismatches=mismatches,manual_profile_files=len(profile_after),
    delivery_sha256={str(p):sha(p) for p in delivery},
    formal_baseline_scope='FAIL: no usable approved TASK-053 snapshot at base; separately recorded, not counted as PASS',
    not_committed_or_pushed=True)
write('delivery-integrity.json',report)
print(json.dumps({k:v for k,v in report.items() if k!='delivery_sha256'},ensure_ascii=False,indent=2))
raise SystemExit(0 if report['passed'] else 1)
