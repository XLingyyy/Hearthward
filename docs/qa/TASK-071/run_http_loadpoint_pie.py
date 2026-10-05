"""Root-only serial launch of the actual TASK071 HTTP/LoadPoint PIE QA."""
import argparse
import json
import re
import sys
import time
import uuid
from pathlib import Path

sys.path.insert(0, 'G:/GameFactory')
from engine_adapters.ue5 import UEClient

p = argparse.ArgumentParser()
p.add_argument('--project', type=Path, default=Path('G:/GameFactory/Hearthward/.agent-local/task051/Hearthward.uproject'))
p.add_argument('--bundle', type=Path, default=Path('G:/GameFactory/Hearthward/Runtime/LocalAI'))
p.add_argument('--label', default='actual-http-loadpoint')
p.add_argument('--pool', type=uuid.UUID, default=uuid.uuid4())
p.add_argument('--timeout', type=int, default=900)
a = p.parse_args()
if not re.fullmatch(r'[A-Za-z0-9_-]+', a.label):
    p.error('label permits only letters, digits, underscore and hyphen')
root = a.project.resolve().parent
out = root/'Saved/Task071'/('http-'+a.label+'-'+str(a.pool))
out.mkdir(parents=True, exist_ok=True)
ue = UEClient(project_path=str(a.project.resolve()), ue_root='G:/UnrealEngine/UE_5.8')
started = time.time()
launch = ue.runtime.launch_editor(map_path='/Game/Hearthward/Tests/Graybox/L_GrayboxValidation', extra_args=[
    '-ExecutePythonScript='+str(Path(__file__).with_name('verify_http_loadpoint_pie.py').resolve()),
    '-HearthwardSaveTestPool='+str(a.pool), '-Task071HTTPLabel='+a.label,
    '-HearthwardAIBundlePath='+str(a.bundle.resolve()), '-HearthwardAIBackend=vulkan',
    '-HearthwardAIGpuLayers=16', '-NoSound', '-NoSplash', '-culture=en'])
(out/'launch.json').write_text(json.dumps(launch, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps({'launch_ok': launch['ok'], 'pool': str(a.pool)}), flush=True)
if not launch['ok']:
    raise SystemExit(1)
passed = False
try:
    deadline = time.monotonic()+a.timeout
    while time.monotonic() < deadline:
        result = out/'results.json'
        if result.exists() and result.stat().st_mtime >= started:
            report = json.loads(result.read_text(encoding='utf-8-sig'))
            passed = report.get('ok', False)
            print(json.dumps({k: report.get(k) for k in ('ok', 'stage', 'error', 'observed_seconds_after_old_dispatch')}, ensure_ascii=False), flush=True)
            break
        time.sleep(1)
finally:
    stopped = ue.runtime.stop_editor(launch['payload']['process_id'])
    (out/'stop.json').write_text(json.dumps(stopped, ensure_ascii=False, indent=2), encoding='utf-8')
raise SystemExit(0 if passed and stopped['ok'] else 1)
