from pathlib import Path
import sys,time,json,uuid
sys.path.insert(0,'G:/GameFactory')
from engine_adapters.ue5 import UEClient
out=Path('G:/GameFactory/Hearthward/.agent-local/qa/TASK-095/weapons/preview.json')
if out.exists():out.rename(out.with_name('preview-'+str(time.time_ns())+'.json'))
u=UEClient(project_path='G:/GameFactory/Hearthward/Hearthward.uproject',ue_root='G:/UnrealEngine/UE_5.8')
r=u.runtime.launch_editor(map_path='/Game/Hearthward/Tests/Graybox/L_GrayboxValidation',extra_args=['-ExecutePythonScript=G:/GameFactory/Hearthward/.agent-local/qa/TASK-095/capture_weapons.py','-CoreLimit=4','-HearthwardSaveTestPool='+str(uuid.uuid4())])
assert r['ok'],r
try:
 for _ in range(240):
  if out.exists():break
  time.sleep(1)
 data=json.loads(out.read_text(encoding='utf-8'));print({k:v for k,v in data.items() if k not in ('samples','reverse_samples')},'samples',len(data.get('samples',[])),flush=True)
finally:print(u.runtime.stop_editor(r['payload']['process_id']),flush=True)
