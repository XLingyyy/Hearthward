from pathlib import Path
import sys,json,uuid
sys.path.insert(0,'G:/GameFactory')
from engine_adapters.ue5 import UEClient
out=Path('G:/GameFactory/Hearthward/.agent-local/qa/TASK-095/weapons');out.mkdir(exist_ok=True)
u=UEClient(project_path='G:/GameFactory/Hearthward/Hearthward.uproject',ue_root='G:/UnrealEngine/UE_5.8')
r=u.testing.run_automation_tests('Hearthward.Iteration.Task095.WeaponInstancePresentation',report_dir=str(out/'native'),extra_args=['-NullRHI','-HearthwardSaveTestPool='+str(uuid.uuid4())],timeout=300)
(out/'native-result.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
print('NATIVE',r['ok'],{k:v for k,v in r['payload'].items() if k.startswith('tests_')},flush=True)
