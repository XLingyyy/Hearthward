from pathlib import Path
import sys,time,json,uuid
sys.path.insert(0,'G:/GameFactory')
from engine_adapters.ue5 import UEClient
out=Path(__file__).parent/'run-05/result.json'
assert not out.exists()
u=UEClient(project_path='G:/GameFactory/Hearthward/Hearthward.uproject',ue_root='G:/UnrealEngine/UE_5.8')
r=u.runtime.launch_editor(map_path='/Game/Hearthward/Tests/Graybox/L_GrayboxValidation',extra_args=['-ExecutePythonScript=G:/GameFactory/Hearthward/.agent-local/qa/TASK-098/closure/capture05.py','-DDC=InstalledNoZenLocalFallback','-CoreLimit=4','-UserDir=E:/HearthwardQA/task098-closure-05','-HearthwardSaveTestPool='+str(uuid.uuid4())])
assert r['ok'],r
try:
 for _ in range(360):
  if out.exists():break
  time.sleep(1)
 d=json.loads(out.read_text(encoding='utf-8'));print('passed',d.get('passed'),'checks',len(d.get('checks',{})),'error',d.get('error'),flush=True)
finally:print(u.runtime.stop_editor(r['payload']['process_id']),flush=True)
