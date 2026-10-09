from pathlib import Path
import sys,time,json,uuid
sys.path.insert(0,'G:/GameFactory')
from engine_adapters.ue5 import UEClient
out=Path(__file__).parent/'run-02/result.json'
assert not out.exists()
u=UEClient(project_path='G:/GameFactory/Hearthward/Hearthward.uproject',ue_root='G:/UnrealEngine/UE_5.8')
r=u.runtime.launch_editor(map_path='/Game/Hearthward/Tests/Graybox/L_GrayboxValidation',extra_args=['-ExecutePythonScript=G:/GameFactory/Hearthward/.agent-local/qa/TASK-097/walking/capture02.py','-DDC=InstalledNoZenLocalFallback','-CoreLimit=4','-HearthwardSaveTestPool='+str(uuid.uuid4())])
assert r['ok'],r
try:
 for _ in range(540):
  if out.exists():break
  time.sleep(1)
 d=json.loads(out.read_text(encoding='utf-8'));print({k:v for k,v in d.items() if k!='samples'},'samples',len(d.get('samples',[])),flush=True)
finally:print(u.runtime.stop_editor(r['payload']['process_id']),flush=True)
