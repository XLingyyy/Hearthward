"""Root-only serial runner. Separate actual backend processes; no model copying or downloads."""
import argparse
import json
import sys
import time
import uuid
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[6]))
from engine_adapters.ue5 import UEClient

parser = argparse.ArgumentParser()
parser.add_argument('--backend', choices=('cpu', 'vulkan'), required=True)
parser.add_argument('--bundle', type=Path, default=Path('G:/GameFactory/Hearthward/Runtime/LocalAI'))
parser.add_argument('--cases', type=int, choices=(1, 60), default=60)
parser.add_argument('--timeout', type=int, default=5400)
parser.add_argument('--case-ids', help='Diagnostic subset of frozen expression IDs, comma separated; full acceptance gates remain unchanged')
args = parser.parse_args()
root = Path(__file__).resolve().parents[3]
qa = root/'docs/qa/TASK-068'
selected_ids = []
if args.case_ids:
    selected_ids = args.case_ids.split(',')
    known_ids = {row['id'] for row in json.loads((qa/'cases.json').read_text(encoding='utf-8-sig'))['cases']}
    if len(selected_ids) != len(set(selected_ids)) or any(x not in known_ids for x in selected_ids):
        parser.error('--case-ids must contain unique IDs from the frozen dataset')
ue = UEClient(project_path=str(root/'Hearthward.uproject'), ue_root='G:/UnrealEngine/UE_5.8')
started = time.time()
launch = ue.runtime.launch_editor(map_path='/Game/Hearthward/Tests/Graybox/L_GrayboxValidation', extra_args=[
    '-ExecutePythonScript='+str(qa/'verify_model_matrix_pie.py'), '-HearthwardSaveTestPool='+str(uuid.uuid4()),
    '-HearthwardAIBundlePath='+str(args.bundle.resolve()), '-HearthwardAIBackend='+args.backend,
    '-HearthwardAIGpuLayers=16', '-Task068Cases='+str(args.cases), '-NoSound', '-NoSplash', '-culture=en']
    + (['-Task068CaseIds='+','.join(selected_ids)] if selected_ids else []))
(qa/(args.backend+'-launch.json')).write_text(json.dumps(launch, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps({'backend': args.backend, 'launch_ok': launch['ok']}), flush=True)
if not launch['ok']: raise SystemExit(1)
result = root/'Saved/Task068'/args.backend/'results.json'
passed = False
try:
    deadline = time.monotonic()+args.timeout
    while time.monotonic() < deadline:
        if result.exists() and result.stat().st_mtime >= started:
            report = json.loads(result.read_text(encoding='utf-8-sig'))
            passed = report.get('ok', False)
            (qa/(args.backend+'-results.json')).write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
            print(json.dumps({k: report.get(k) for k in ('ok', 'backend', 'summary', 'error', 'outstanding_gates')}, ensure_ascii=False), flush=True)
            break
        time.sleep(1)
finally:
    closed = ue.runtime.stop_editor(launch['payload']['process_id'])
    (qa/(args.backend+'-stop.json')).write_text(json.dumps(closed, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'closed': closed['ok']}), flush=True)
raise SystemExit(0 if passed and closed['ok'] else 1)
