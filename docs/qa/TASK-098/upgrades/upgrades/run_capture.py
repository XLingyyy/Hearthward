import sys,time,json,uuid
from pathlib import Path
sys.path.insert(0,'G:/GameFactory')
from engine_adapters.ue5 import UEClient
out=Path(__file__).parent/'visual/result.json';assert not out.exists()
u=UEClient(project_path='G:/GameFactory/Hearthward/Hearthward.uproject',ue_root='G:/UnrealEngine/UE_5.8')
r=u.runtime.launch_editor(map_path='/Game/Hearthward/Tests/Graybox/L_GrayboxValidation',extra_args=['-ExecutePythonScript='+str(Path(__file__).parent/'capture.py'),'-DDC=InstalledNoZenLocalFallback','-CoreLimit=4','-HearthwardSaveTestPool='+str(uuid.uuid4()),'-UserDir=E:/HearthwardQA/TASK-098/upgrades-visual'])
assert r['ok'],r
try:
    for _ in range(420):
        if out.exists():break
        time.sleep(1)
    d=json.loads(out.read_text(encoding='utf-8'));print(json.dumps(d,ensure_ascii=False),flush=True)
finally:print(u.runtime.stop_editor(r['payload']['process_id']),flush=True)
