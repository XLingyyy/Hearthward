from pathlib import Path
import sys,time,json,uuid
sys.path.insert(0,'G:/GameFactory')
from engine_adapters.ue5 import UEClient
out=Path(__file__).parent/'after/result.json'
if out.exists():out.rename(out.with_name('preview-'+str(time.time_ns())+'.json'))
u=UEClient(project_path='G:/GameFactory/Hearthward/Hearthward.uproject',ue_root='G:/UnrealEngine/UE_5.8')
r=u.runtime.launch_editor(map_path='/Game/Hearthward/Tests/Graybox/L_GrayboxValidation',extra_args=['-ExecutePythonScript=G:/GameFactory/Hearthward/.agent-local/qa/TASK-096/route-review/capture_after.py','-CoreLimit=4','-Silent','-HearthwardSaveTestPool='+str(uuid.uuid4())])
assert r['ok'],r
try:
 for _ in range(360):
  if out.exists():break
  time.sleep(1)
 data=json.loads(out.read_text(encoding='utf-8'));print('inspection saved',out,'error',data.get('error'),flush=True)
finally:print(u.runtime.stop_editor(r['payload']['process_id']),flush=True)
