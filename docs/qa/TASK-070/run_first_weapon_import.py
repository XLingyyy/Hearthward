"""Root-only public import of two registered existing-source equipment samples."""
import argparse
import json
import os
import sys
import time
from pathlib import Path

parser=argparse.ArgumentParser()
parser.add_argument('--piece',choices=('spear','shortblade'),required=True)
args=parser.parse_args()
root=Path(__file__).resolve().parents[3]
qa=root/'docs/qa/TASK-070'
manifest=json.loads((qa/'first-weapon-import-manifest.json').read_text(encoding='utf-8-sig'))
os.environ['AAAGF_OUTPUT_ROOT']=manifest['output_root']
sys.path.insert(0,str(Path(__file__).resolve().parents[6]))
from engine_adapters.ue5 import UEClient
row=next(x for x in manifest['imports'] if x['piece']==args.piece)
ue=UEClient(project_path=str(root/'Hearthward.uproject'),ue_root='G:/UnrealEngine/UE_5.8')
launch=ue.runtime.launch_editor(map_path='/Game/Hearthward/Tests/Graybox/L_GrayboxValidation',extra_args=['-NoSound','-culture=en','-Unattended','-RenderOffscreen'])
(qa/(args.piece+'-editor-launch.json')).write_text(json.dumps(launch,ensure_ascii=False,indent=2),encoding='utf-8')
if not launch['ok']:raise SystemExit(1)
result={'ok':False,'errors':['Editor readiness timed out']}
try:
    deadline=time.monotonic()+240
    while time.monotonic()<deadline:
        status=ue.observe.check_status(timeout=5)
        if status['payload']['python_execution']['ok']:
            break
        time.sleep(1)
    else:
        raise RuntimeError('Public Python editor endpoint did not become ready')
    result=ue.assets.import_weapon(row['source'],destination=row['destination'],options=row['options'])
    (qa/(args.piece+'-public-import.json')).write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({'piece':args.piece,'ok':result['ok'],'errors':result.get('errors'),'warnings':result.get('warnings'),'payload':result.get('payload')},ensure_ascii=False),flush=True)
finally:
    closed=ue.runtime.stop_editor(launch['payload']['process_id'])
    (qa/(args.piece+'-editor-stop.json')).write_text(json.dumps(closed,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({'closed':closed['ok']}),flush=True)
raise SystemExit(0 if result['ok'] and closed['ok'] else 1)
