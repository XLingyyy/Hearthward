import sys,json,time
from pathlib import Path
sys.path.insert(0,'G:/GameFactory')
from engine_adapters.ue5 import UEClient
out=Path(__file__).parent/'import';out.mkdir(exist_ok=True)
u=UEClient(project_path='G:/GameFactory/Hearthward/Hearthward.uproject',ue_root='G:/UnrealEngine/UE_5.8')
r=u.runtime.launch_editor(extra_args=['-DDC=InstalledNoZenLocalFallback','-CoreLimit=4','-UserDir=E:/HearthwardQA/TASK-100/import-profile'])
assert r['ok'],r
try:
    for _ in range(120):
        if u.observe.check_status()['ok']:break
        time.sleep(2)
    for name in ['SM_RiverGate','SM_WorkshopShelter','SM_DwellingPorch','SM_AssemblyColonnade']:
        result=u.assets.import_prop({'game_id':'Hearthward','run_id':'20261009','task_kind':'3d_object','task_id':'TASK-100-zones','artifact_key':name+'_path'},destination='/Game/Hearthward/Assets/TASK-100/Zones',options={'generate_collision':True,'combine_meshes':True})
        (out/(name+'.json')).write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
        print(name,result['ok'],result.get('errors'),flush=True)
        if not result['ok']:break
finally:print(u.runtime.stop_editor(r['payload']['process_id']),flush=True)
