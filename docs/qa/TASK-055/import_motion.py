from pathlib import Path
import argparse,json,sys,uuid,time
sys.path.insert(0,str(Path(__file__).resolve().parents[6]))
from engine_adapters.ue5 import UEClient
root=Path(__file__).resolve().parents[3]
ue=UEClient(project_path=str(root/'Hearthward.uproject'),ue_root='G:/UnrealEngine/UE_5.8')
qa=root/'docs/qa/TASK-055'
parser=argparse.ArgumentParser();parser.add_argument('--avatar',choices=['Hero','Brother'],default='Hero');args=parser.parse_args()
avatar=args.avatar
label=avatar.lower()
launch=ue.runtime.launch_editor(map_path='/Game/Hearthward/Tests/Graybox/L_GrayboxValidation',extra_args=['-NoSound','-NoSplash','-HearthwardSaveTestPool='+str(uuid.uuid4())])
(qa/('motion-launch-'+label+'.json')).write_text(json.dumps(launch,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({'launch_ok':launch['ok']}),flush=True)
if not launch['ok']:raise SystemExit(1)
passed=False
try:
 source={'game_id':'hearthward','run_id':'20261003_task055','task_kind':'motion','task_id':'knight61','artifact_key':'motion_fbx_path'}
 result=ue.animation.import_motion(source,skeleton=f'/Game/Characters/{avatar}/UE5/SK_{avatar}_Skeleton',destination=f'/Game/Hearthward/Assets/TASK-055/Motion/{avatar}',avatar_name=avatar)
 (qa/('motion-import-'+label+'.json')).write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
 passed=result['ok'];print(json.dumps({'import_ok':passed,'errors':result.get('errors'),'diagnostics':result.get('diagnostics'),'imported_count':len(result.get('payload',{}).get('imported_paths',[]))},ensure_ascii=False),flush=True)
finally:
 closed=ue.runtime.stop_editor(launch['payload']['process_id'])
 (qa/('motion-stop-'+label+'.json')).write_text(json.dumps(closed,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps({'closed':closed['ok']}),flush=True)
raise SystemExit(0 if passed and closed['ok'] else 1)
