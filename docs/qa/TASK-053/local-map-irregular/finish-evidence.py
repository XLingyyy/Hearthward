"""Bind this round's existing checks to current source, assets and delivery files."""
import hashlib
import json
from pathlib import Path
import subprocess

QA = Path(__file__).resolve().parent
PROJECT = QA.parents[3]
PARENT = PROJECT.parent
RUN = QA / 'verify_ba7eadec'

def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def write(name, data):
    (QA / name).write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

task = read(PROJECT / 'docs/tasks/TASK-053.json')
before_config = read(QA / 'before/Resources__UI__interface.json.snapshot')['localMap']
config = read(PROJECT / 'Resources/UI/interface.json')['localMap']
old_fingerprints = read(QA.parent / 'local-map-relief-fog/verify_9fb13427/source-manifest.json')['fingerprints']
assets = {name: sha(PROJECT / name) for name in old_fingerprints if name.startswith('Resources/UI/Art/map-')}
preserved_checks = {name: config[name]==value for name,value in before_config.items()}
preserved_checks.update({name: digest==old_fingerprints[name] for name,digest in assets.items()})
old_scene = read(QA.parent / 'local-map-relief-fog/verify_9fb13427/scene.json')
scene = read(RUN / 'scene.json')
scene_checks = {key: scene[key]==old_scene[key] for key in read(QA / 'scene-comparison.json')['checks']}
preserved_checks['current_scene_unchanged'] = all(scene_checks.values())
write('preserved-data.json',dict(passed=all(preserved_checks.values()),checks=preserved_checks,art_sha256=assets,scene_checks=scene_checks))

def matches(name, pattern):
    return name.startswith(pattern) if pattern.endswith('/') else name==pattern

changes = []
for snapshot in sorted((QA / 'before').iterdir()):
    name = snapshot.name.removesuffix('.snapshot').replace('__','/')
    external = name.startswith('parent/')
    path = PARENT / name.removeprefix('parent/') if external else PROJECT / name
    if sha(path)!=sha(snapshot):
        allowed = name in ['parent/地图测试版_预览.png','parent/地图测试版_移动验证.png'] if external else any(matches(name,p) for p in task['allowed_paths']) and not any(matches(name,p) for p in task['forbidden_paths'])
        changes.append(dict(path=name,allowed=allowed,before_sha256=sha(snapshot),after_sha256=sha(path)))
qa_paths = [p.relative_to(PROJECT).as_posix() for p in QA.rglob('*') if p.is_file()]
qa_allowed = all(any(matches(name,p) for p in task['allowed_paths']) and not any(matches(name,p) for p in task['forbidden_paths']) for name in qa_paths)
scope_ok = all(c['allowed'] for c in changes) and qa_allowed
write('incremental-scope.json',dict(passed=scope_ok,changes=changes,new_qa_folder_allowed=qa_allowed,qa_file_count=len(qa_paths),scope='Latest user-authorized changes against 15 before snapshots. Parent artifacts explicitly requested. Existing earlier uncommitted changes are excluded. Does not replace the formal baseline approval check.'))

manifest = read(RUN / 'source-manifest.json')
mismatches = [name for name,digest in manifest['fingerprints'].items() if sha(PROJECT / name)!=digest]
profile_before = read(QA / 'manual-profile-before.json')
profile_after = {p.relative_to(PROJECT).as_posix():sha(p) for p in (PROJECT / 'TestClient/Profile').rglob('*') if p.is_file()}
copies = {str(PROJECT/'地图测试版_预览.png'):RUN/'map-1600x1000.png',str(PARENT/'地图测试版_预览.png'):RUN/'map-1600x1000.png',str(PARENT/'地图测试版_移动验证.png'):RUN/'map-separated-flames.png'}
root = read(QA / 'root-launch-smoke.json')
native = read(RUN / 'report.json')
image = read(QA / 'final-audit.json')
build = read(QA / 'build_904f2616/build.json')
head = subprocess.check_output(['git','rev-parse','HEAD'],cwd=PROJECT,text=True).strip()
branch = subprocess.check_output(['git','branch','--show-current'],cwd=PROJECT,text=True).strip()
checks = dict(build_success_no_diagnostics=build['ok'] and not build['diagnostics'],
    native_49_pass=native['passed'] and len(native['checks'])==49 and all(native['checks'].values()),
    image_28_pass=image['passed'] and len(image['checks'])==28 and all(image['checks'].values()),
    current_241_fingerprints_match=len(manifest['fingerprints'])==241 and not mismatches,
    original_assets_map_parameters_and_scene_preserved=all(preserved_checks.values()),
    manual_profile_59_unchanged=profile_before==profile_after and len(profile_before)==59,
    root_pngs_match_current_native_captures=all(sha(Path(dest))==sha(src) for dest,src in copies.items()),
    root_launcher_log_7_pass=root['passed'] and len(root['checks'])==7 and all(root['checks'].values()),
    repository_zero_errors='PASS: repository checks only; 0 error(s)' in (QA/'repo-validation.txt').read_text(encoding='utf-8'),
    diff_format_pass='Exit code: 0' in (QA/'diff-check.txt').read_text(encoding='utf-8'),
    tool_tests_33_pass='Ran 33 tests' in (QA/'tool-tests.txt').read_text(encoding='utf-8-sig') and '\nOK' in (QA/'tool-tests.txt').read_text(encoding='utf-8-sig'),
    current_incremental_scope_allowed=scope_ok,
    head_matches_manifest=head==manifest['head'])
delivery = [PROJECT/'README.md',PROJECT/'TestClient/README.md',PROJECT/'docs/handoffs/TASK-053.md',PROJECT/'docs/tasks/TASK-053.json',PROJECT/'scripts/ui/audit_local_map.py',QA/'REPORT.md',QA/'finish-evidence.py',QA/'root-launch-audit.py']+[Path(dest) for dest in copies]
data = dict(passed=all(checks.values()),checks=checks,head=head,branch=branch,build='build_904f2616',native_run=RUN.name,root_run=root['run'],root_process_id=root['process_id'],root_responsiveness=root['responsiveness_observation'],native_checks=len(native['checks']),image_checks=len(image['checks']),fingerprints=len(manifest['fingerprints']),mismatches=mismatches,manual_profile_files=len(profile_after),retained_area_ratio=image['outline']['retained_area_ratio'],delivery_sha256={str(p):sha(p) for p in delivery},formal_baseline_scope='FAIL: no usable approved TASK-053 snapshot at base; recorded separately, not counted as a passed check',not_committed_or_pushed=True)
write('delivery-integrity.json',data)
print(json.dumps({k:v for k,v in data.items() if k!='delivery_sha256'},ensure_ascii=False,indent=2))
raise SystemExit(0 if data['passed'] else 1)
