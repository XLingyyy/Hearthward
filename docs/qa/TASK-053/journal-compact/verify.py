"""Launch and collect real journal UI verification without touching the manual client."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import time
import uuid

QA = Path(__file__).resolve().parent
GAME = QA.parents[3]
sys.path.insert(0, str(GAME / 'scripts/animals'))
from paths import configure_factory, ue_root
configure_factory()
from engine_adapters.ue5 import UEClient

run_id = 'verify_' + uuid.uuid4().hex[:8]
out = QA / run_id
out.mkdir()
profile = GAME / 'TestClient/Runs' / run_id
profile.mkdir(parents=True)
os.environ['HEARTHWARD_TITLE_RUN'] = run_id
os.environ['HEARTHWARD_JOURNAL_QA'] = str(out)
preferences = GAME / 'Saved/Config'
before_preferences = {p: p.read_bytes() for p in preferences.glob('*/GameUserSettings.ini')}
compiled = json.loads((QA / 'compiled-source.json').read_text(encoding='utf-8'))
if not all(hashlib.sha256((GAME / n).read_bytes()).hexdigest() == d for n, d in compiled.items()):
    raise RuntimeError('The current source must match the completed Development build')
inputs = set(compiled) | {'Resources/Data/gameplay.json', 'Hearthward.uproject'}
inputs.update(p.relative_to(GAME).as_posix() for p in (GAME / 'Resources/UI').rglob('*') if p.is_file())
inputs.update(p.relative_to(GAME).as_posix() for p in QA.glob('*.py'))
(out / 'fingerprints.json').write_text(json.dumps({n: hashlib.sha256((GAME / n).read_bytes()).hexdigest() for n in sorted(inputs)}, indent=2), encoding='utf-8')
client = UEClient(project_path=GAME / 'Hearthward.uproject', ue_root=ue_root(), port=30171, runtime_port=30172)
launch = client.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap', extra_args=[
    '-NewConsole', '-CoreLimit=4', '-RenderOffscreen', '-unattended', '-nosound',
    '-UserDir=' + str(profile), '-HearthwardSaveTestPool=' + str(uuid.uuid4()),
    '-abslog=' + str(out / 'runtime.log'), '-ExecutePythonScript=' + str(QA / 'verify-pie.py')])
(out / 'launch.json').write_text(json.dumps(launch, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps(dict(run_id=run_id, ok=launch['ok'], evidence=str(out)), ensure_ascii=False), flush=True)
if not launch['ok']:
    raise SystemExit(1)
try:
    report_path = out / 'report.json'
    deadline = time.monotonic() + 300
    while not report_path.exists() and time.monotonic() < deadline:
        time.sleep(1)
    if not report_path.exists():
        raise TimeoutError('Journal verification did not return a report: ' + str(out))
    report = json.loads(report_path.read_text(encoding='utf-8'))
    native = Path(report.get('native_report_path', 'missing-journal-preview.json'))
    if native.is_file():
        shutil.copy2(native, out / native.name)
    for filename in report.get('captures', []):
        shutil.copy2(Path(report['capture_directory']) / filename, out / filename)
    (QA / 'latest-verification.json').write_text(json.dumps(dict(run_id=run_id, passed=report['passed'], checks=len(report['checks']), captures=len(report.get('captures', []))), indent=2), encoding='utf-8')
    print(json.dumps(dict(passed=report['passed'], checks=len(report['checks']), failed=[n for n, v in report['checks'].items() if not v], error=report.get('error'), captures=len(report.get('captures', []))), ensure_ascii=False), flush=True)
    raise SystemExit(0 if report['passed'] else 1)
finally:
    (out / 'stop.json').write_text(json.dumps(client.runtime.stop_editor(launch['payload']['process_id']), indent=2), encoding='utf-8')
    for path in set(before_preferences) | set(preferences.glob('*/GameUserSettings.ini')):
        if path in before_preferences:
            path.write_bytes(before_preferences[path])
        else:
            path.unlink()
