import json
import sys
import time
import uuid
from pathlib import Path

sys.path.insert(0, 'G:/GameFactory')
from engine_adapters.ue5 import UEClient

ROOT = Path('G:/GameFactory/Hearthward-ai-npc-fix')
OUT = ROOT / 'Saved/NPCValidation'
ue = UEClient(project_path=ROOT / 'Hearthward.uproject', ue_root='G:/UnrealEngine/UE_5.8', port=30129, runtime_port=30130)
mode = sys.argv[1]
if mode == 'build':
    result = ue.build.project(target='HearthwardEditor', configuration='Development', timeout=1200)
elif mode == 'native':
    result = ue.testing.run_automation_tests('Hearthward.', report_dir=str(OUT / 'native'), extra_args=['-NullRHI'], timeout=300)
else:
    script = ROOT / sys.argv[2]
    report = ROOT / sys.argv[3]
    limit = int(sys.argv[4]) if len(sys.argv) > 4 else 300
    assert not report.exists(), f'Preserve previous result: {report}'
    result = ue.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap', extra_args=[
        '-ExecutePythonScript=' + str(script), '-HearthwardSaveTestPool=' + str(uuid.uuid4()),
        '-HearthwardAIBackend=vulkan', '-HearthwardAIGpuLayers=32',
        '-HearthwardAIBundlePath=G:/GameFactory/Hearthward/Runtime/LocalAI',
        *(['-windowed', '-ResX=1280', '-ResY=720'] if mode == 'ui' else ['-RenderOffscreen']),
        '-Unattended', '-NoSound', '-NoLiveCoding',
        '-abslog=' + str(OUT / (mode + '.log')),
    ])
    (OUT / (mode + '-launch.json')).write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    if result['ok']:
        deadline = time.monotonic() + limit
        try:
            while not report.exists() and time.monotonic() < deadline:
                time.sleep(2)
            result = json.loads(report.read_text(encoding='utf-8')) if report.exists() else {'ok': False, 'error': 'runner timeout; inspect editor log'}
        finally:
            time.sleep(3)
            ue.runtime.stop_editor(json.loads((OUT / (mode + '-launch.json')).read_text(encoding='utf-8'))['payload']['process_id'])
(OUT / (mode + '-result.json')).write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
if 'payload' in result:
    result['payload'].pop('stdout', None)
    result['payload'].pop('stderr', None)
print(json.dumps({'ok': result.get('ok'), 'summary': result.get('summary'),
                  'checks': len(result.get('checks', {})),
                  'failed': [k for k, v in result.get('checks', {}).items() if not v],
                  'error': result.get('error'), 'errors': result.get('errors')}, ensure_ascii=False, indent=2))
