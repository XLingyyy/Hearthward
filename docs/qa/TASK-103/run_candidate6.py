"""Own the isolated packaged-game process while normal UI checks run."""
import json
from pathlib import Path
import sys
import time

sys.path.insert(0, 'G:/GameFactory')
from engine_adapters.ue5 import UEClient

root=Path('G:/GameFactory/Hearthward')
out=root/'.agent-local/qa/TASK-103/os-candidate6'
out.mkdir(parents=True,exist_ok=True)
profile=Path('F:/HearthwardQA/Profiles/candidate6-20261008')
ue=UEClient(project_path=str(root/'Hearthward.uproject'),ue_root='G:/UnrealEngine/UE_5.8')
result=ue.runtime.launch_packaged(
    'F:/HearthwardDemo/iteration-084-103-20261008-6/Windows/Hearthward/Binaries/Win64/Hearthward-Win64-Shipping.exe',
    extra_args=('-UserDir='+str(profile),'-HearthwardAIBackend=cpu','-Silent'))
(out/'launch.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
assert result['ok'],result
print('owned launch',result['payload'],flush=True)
try:
    while not (out/'stop-request').exists():
        time.sleep(1)
finally:
    result=ue.runtime.stop_editor(result['payload']['process_id'])
    (out/'stop.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print(result,flush=True)
