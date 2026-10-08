"""Build an independent internal candidate through the public UEClient."""
import json
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, 'G:/GameFactory')
from engine_adapters.ue5 import UEClient

project = Path('G:/GameFactory/Hearthward')
out = project / '.agent-local/qa/TASK-103/package-20261008-6'
archive = Path('F:/HearthwardDemo/iteration-084-103-20261008-6')
stage = Path('E:/HearthwardQA/TASK-103/package-20261008-6/Staged')
if out.exists() or archive.exists() or stage.exists():
    raise RuntimeError('Preserve previous candidate and evidence')
out.mkdir(parents=True)
for args, name in [(['rev-parse', 'HEAD'], 'head.txt'),
                   (['diff', '--binary', 'HEAD'], 'tracked.patch'),
                   (['ls-files', '--others', '--exclude-standard'], 'untracked-files.txt')]:
    result = subprocess.run(['git', *args], cwd=project, capture_output=True, check=True)
    (out / name).write_bytes(result.stdout)
ue = UEClient(project_path=str(project / 'Hearthward.uproject'),
              ue_root='G:/UnrealEngine/UE_5.8')
result = ue.build.package(
    archive_dir=str(archive), configuration='Shipping',
    maps=('/Game/Hearthward/Bootstrap/L_Bootstrap',
          '/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'),
    extra_args=('-stagingdirectory=' + str(stage), '-nodebuginfo'),
    log_path=str(out / 'package.log'), timeout=10800)
(out / 'result.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps({k: v for k, v in result.items() if k != 'payload'}, ensure_ascii=False), flush=True)
sys.exit(0 if result['ok'] else 1)
