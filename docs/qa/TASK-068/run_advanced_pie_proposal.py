"""Root-only serial supplemental three-case runner; frozen matrix runner is unchanged."""
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
parser.add_argument('--timeout', type=int, default=1200)
parser.add_argument('--case-ids', default='', help='Comma-separated supplemental IDs, e.g. ADV-N01,ADV-N02')
args = parser.parse_args()
if args.case_ids:
    known_ids = {row['id'] for row in json.loads((Path(__file__).resolve().parent/'advanced-cases-proposal.json').read_text(encoding='utf-8-sig'))['cases']}
    selected_ids = args.case_ids.split(',')
    if len(selected_ids) != len(set(selected_ids)) or not set(selected_ids) <= known_ids:
        parser.error('--case-ids must contain unique known supplemental IDs')
root = Path(__file__).resolve().parents[3]
qa = root/'docs/qa/TASK-068'
pool = str(uuid.uuid4())
label = 'advanced-public-api-'+args.backend+'-'+pool
ue = UEClient(project_path=str(root/'Hearthward.uproject'), ue_root='G:/UnrealEngine/UE_5.8')
started = time.time()
launch = ue.runtime.launch_editor(map_path='/Game/Hearthward/Tests/Graybox/L_GrayboxValidation', extra_args=[
    '-ExecutePythonScript='+str(qa/'verify_advanced_model_pie_proposal.py'), '-HearthwardSaveTestPool='+pool,
    '-HearthwardAIBundlePath='+str(args.bundle.resolve()), '-HearthwardAIBackend='+args.backend,
    '-HearthwardAIGpuLayers=16', '-Task068AdvancedRun='+pool, '-NoSound', '-NoSplash', '-culture=en']
    + (['-Task068AdvancedCases='+args.case_ids] if args.case_ids else []))
(qa/(label+'-launch.json')).write_text(json.dumps(launch, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps({'backend': args.backend, 'pool': pool, 'output': label, 'launch_ok': launch['ok']}), flush=True)
if not launch['ok']:
    raise SystemExit(1)
result = root/'Saved/Task068'/label/'results.json'
passed = False
try:
    deadline = time.monotonic()+args.timeout
    while time.monotonic() < deadline:
        if result.exists() and result.stat().st_mtime >= started:
            report = json.loads(result.read_text(encoding='utf-8-sig'))
            passed = report.get('ok', False)
            (qa/(label+'-results.json')).write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
            print(json.dumps({k: report.get(k) for k in ('ok', 'backend', 'run_id', 'summary', 'error', 'not_covered')}, ensure_ascii=False), flush=True)
            break
        time.sleep(1)
finally:
    closed = ue.runtime.stop_editor(launch['payload']['process_id'])
    (qa/(label+'-stop.json')).write_text(json.dumps(closed, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'closed': closed['ok']}), flush=True)
raise SystemExit(0 if passed and closed['ok'] else 1)
