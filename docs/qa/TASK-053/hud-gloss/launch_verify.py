"""Development-only HUD evidence; restore developer settings and stop only our own editor."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import time
import uuid

GAME = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(GAME / 'scripts/animals'))
from paths import configure_factory, ue_root
configure_factory()
from engine_adapters.ue5 import UEClient

token = uuid.uuid4().hex[:12]
run_id = 'hud_gloss_' + token
os.environ['HEARTHWARD_HUD_RUN'] = run_id
os.environ['HEARTHWARD_TITLE_RUN'] = 'verify_gloss_' + token
out = Path(__file__).parent / run_id
out.mkdir(parents=True)
preferences = {p: p.read_bytes() for p in (GAME / 'Saved/Config').glob('*/GameUserSettings.ini')}
client = UEClient(project_path=GAME / 'Hearthward.uproject', ue_root=ue_root(), port=30093, runtime_port=30094)
launch = client.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap', extra_args=[
    '-HearthwardSaveTestPool=' + str(uuid.uuid4()), '-abslog=' + str(out / 'runtime.log'), '-CoreLimit=4',
    '-RenderOffscreen', '-unattended', '-nosound', '-windowed', '-ResX=1600', '-ResY=1000',
    '-ExecutePythonScript=' + str(Path(__file__).with_name('verify.py'))])
(out / 'launch.json').write_text(json.dumps(launch, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps({'run_id': run_id, 'launch': launch}, ensure_ascii=False, indent=2), flush=True)
if not launch['ok']:
    raise SystemExit(1)
sources = ['Source/Hearthward/UI/HearthwardScreenContent.cpp', 'Source/Hearthward/UI/HearthwardScreenPaint.cpp',
           'Source/Hearthward/UI/HearthwardScreenHUDTest.inl', 'Source/Hearthward/UI/HearthwardScreenWidget.h',
           'Resources/UI/interface.json', 'Resources/UI/layout.json', 'Resources/Data/gameplay.json',
           'Source/Hearthward/Gameplay/HearthwardGameplayComponent.cpp', 'scripts/ui/launch_hud_test.py',
           'Binaries/Win64/UnrealEditor-Hearthward.dll']
(out / 'fingerprints.json').write_text(json.dumps({p: hashlib.sha256((GAME / p).read_bytes()).hexdigest()
                                                 for p in sources}, indent=2), encoding='utf-8')
saved = GAME / 'Saved/HUDPreview' / run_id
try:
    deadline = time.monotonic() + 300
    while not (saved / 'report.json').is_file() and time.monotonic() < deadline:
        time.sleep(1)
    if not (saved / 'report.json').is_file():
        raise TimeoutError('HUD gloss verification did not return')
    report = json.loads((saved / 'report.json').read_text(encoding='utf-8'))
    for p in saved.glob('*'):
        if p.is_file():
            shutil.copy2(p, out / p.name)
    for name in report['ui_captures']:
        shutil.copy2(GAME / 'Saved/Task020' / name, out / name)
    print(json.dumps({'run_id': run_id, 'passed': report['passed'], 'checks': len(report['checks']),
                      'captures': len(report['captures']) + len(report['ui_captures']),
                      'error': report.get('error'), 'evidence': str(out)}, ensure_ascii=False, indent=2), flush=True)
    if not report['passed']:
        raise SystemExit(1)
finally:
    stop = client.runtime.stop_editor(launch['payload']['process_id'])
    (out / 'stop.json').write_text(json.dumps(stop, indent=2), encoding='utf-8')
    for p in set(preferences) | set((GAME / 'Saved/Config').glob('*/GameUserSettings.ini')):
        if p in preferences:
            p.write_bytes(preferences[p])
        else:
            p.unlink()
    (out / 'preference-restore.json').write_text(json.dumps({'restored': True}, indent=2), encoding='utf-8')
