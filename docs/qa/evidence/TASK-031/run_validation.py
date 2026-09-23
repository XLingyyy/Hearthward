"""Run from GameFactory with its Python environment; closes only its own editor."""
import argparse
import json
import sys
import time
import uuid
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[5]))
from engine_adapters.ue5 import UEClient

parser = argparse.ArgumentParser()
parser.add_argument('mode', choices=['build', 'native', 'hero', 'brother', 'inplace'])
parser.add_argument('--engine', default='G:/UnrealEngine/UE_5.8')
parser.add_argument('--ai-bundle', default='G:/GameFactory/Hearthward/Runtime/LocalAI')
args = parser.parse_args()
root = Path(__file__).resolve().parents[4]
ue = UEClient(project_path=root/'Hearthward.uproject', ue_root=args.engine,
              port=30129, runtime_port=30130)
if args.mode == 'build':
    result = ue.build.project(target='HearthwardEditor', configuration='Development', timeout=1200)
elif args.mode == 'native':
    result = ue.testing.run_automation_tests('Hearthward.Companion.',
        report_dir=str(root/'Saved/Task031/native'), extra_args=['-NullRHI'], timeout=240)
else:
    script, report = {
        'hero': ('verify_hero_pie.py', 'Saved/HeroValidation/playtest5/verification.json'),
        'brother': ('brother/verify_brother_pie.py', 'Saved/BrotherValidation/playtest4/result.json'),
        'inplace': ('verify_in_place.py', 'Saved/HeroValidation/inplace-after-final/result.json'),
    }[args.mode]
    report = root/report
    assert not report.exists(), f'Archive the previous run first: {report}'
    launched = ue.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap', extra_args=[
        '-ExecutePythonScript='+str(Path(__file__).parent/script),
        '-HearthwardSaveTestPool='+str(uuid.uuid4()), '-HearthwardAIBundlePath='+args.ai_bundle,
        '-RenderOffscreen', '-Unattended', '-NoSound', '-NoLiveCoding'])
    assert launched['ok'], launched
    try:
        end = time.monotonic()+240
        while not report.exists() and time.monotonic()<end:
            time.sleep(2)
        result = json.loads(report.read_text(encoding='utf-8')) if report.exists() else {'ok':False,'error':'timeout'}
    finally:
        time.sleep(3)
        ue.runtime.stop_editor(launched['payload']['process_id'])
print(json.dumps(result, ensure_ascii=False, indent=2))
raise SystemExit(0 if result.get('ok') else 1)
