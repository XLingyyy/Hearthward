"""Root-only read-only asset probe through the public editor lifecycle API."""
import argparse
import json
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[6]))
from engine_adapters.ue5 import UEClient

root = Path(__file__).resolve().parents[3]
qa = root / 'docs/qa/TASK-070'
parser = argparse.ArgumentParser()
parser.add_argument('--action', choices=('probe', 'render', 'pbr'), default='probe')
parser.add_argument('--label', default='import-red')
args = parser.parse_args()
if args.action == 'render':
    script = qa / 'render_imported_weapons.py'
    output = root / 'Saved/Task070' / ('weapon-render-' + args.label) / 'render-facts.json'
elif args.action == 'pbr':
    script = qa / 'apply_sample_weapon_pbr.py'
    output = root / 'Saved/Task070/equipment-sample-pbr-fix.json'
else:
    script = qa / 'probe_imported_weapons.py'
    output = root / 'Saved/Task070/equipment-import-probe.json'
record = args.action + ('-' + args.label if args.action == 'render' else '')
ue = UEClient(project_path=str(root / 'Hearthward.uproject'), ue_root='G:/UnrealEngine/UE_5.8')
started = time.time()
launch = ue.runtime.launch_editor(
    map_path='/Game/Hearthward/Tests/Graybox/L_GrayboxValidation',
    extra_args=['-ExecutePythonScript=' + str(script), '-Task070RenderLabel=' + args.label,
                '-NoSound', '-NoSplash', '-culture=en', '-Unattended', '-RenderOffscreen'])
(qa / (record + '-editor-launch.json')).write_text(json.dumps(launch, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps({'launch_ok': launch['ok']}), flush=True)
if not launch['ok']:
    raise SystemExit(1)
complete = False
try:
    deadline = time.monotonic() + 300
    while time.monotonic() < deadline:
        if output.exists() and output.stat().st_mtime >= started:
            report = json.loads(output.read_text(encoding='utf-8-sig'))
            (qa / (record + '-results.json')).write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
            if args.action == 'probe':
                (qa / 'equipment-import-probe.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
                complete = all(not piece['read_errors'] and all(
                    asset.get('status') == 'read_complete' for asset in piece['assets'].values())
                    for piece in report['pieces'])
            elif args.action == 'render':
                complete = report.get('capture_complete', False) and not report.get('error')
            else:
                complete = report.get('ok', False)
            print(json.dumps({'action': args.action, 'complete': complete, 'error': report.get('error')}, ensure_ascii=False), flush=True)
            break
        time.sleep(1)
    else:
        print(json.dumps({'read_complete': False, 'error': 'Probe output timed out'}), flush=True)
finally:
    closed = ue.runtime.stop_editor(launch['payload']['process_id'])
    (qa / (record + '-editor-stop.json')).write_text(json.dumps(closed, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'closed': closed['ok']}), flush=True)
raise SystemExit(0 if complete and closed['ok'] else 1)
