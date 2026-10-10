from pathlib import Path
import sys,time
sys.path.insert(0,'G:/GameFactory')
from engine_adapters.ue5 import UEClient
out=Path(__file__).parent;out.mkdir(exist_ok=True)
(out/'import.json').unlink(missing_ok=True)
u=UEClient(project_path='G:/GameFactory/Hearthward/Hearthward.uproject',ue_root='G:/UnrealEngine/UE_5.8')
r=u.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap',extra_args=['-ExecutePythonScript=G:/GameFactory/Hearthward/.agent-local/qa/TASK-095/windup/import_spear.py','-DDC=InstalledNoZenLocalFallback','-CoreLimit=4','-NullRHI','-unattended'])
assert r['ok'],r
try:
 for _ in range(240):
  if (out/'import.json').exists():break
  time.sleep(1)
 print((out/'import.json').read_text(encoding='utf-8'),flush=True)
finally:print(u.runtime.stop_editor(r['payload']['process_id']),flush=True)
