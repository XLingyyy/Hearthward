import sys,json,time
from pathlib import Path
sys.path.insert(0,'G:/GameFactory')
from engine_adapters.ue5 import UEClient
out=Path(__file__).parent/'import';out.mkdir(exist_ok=True)
u=UEClient(project_path='G:/GameFactory/Hearthward/Hearthward.uproject',ue_root='G:/UnrealEngine/UE_5.8')
r=u.runtime.launch_editor(extra_args=['-DDC=InstalledNoZenLocalFallback','-CoreLimit=4','-UserDir=E:/HearthwardQA/TASK-098/upgrades-import'])
assert r['ok'],r
try:
    for _ in range(120):
        if u.observe.check_status()['ok']:break
        time.sleep(2)
    for name in ['SM_Workbench_L2', 'SM_Workbench_L3', 'SM_Smelter_L2', 'SM_Smelter_L3', 'SM_Forge_L2', 'SM_Forge_L3', 'SM_Cooking_L2', 'SM_Cooking_L3', 'SM_ForgeHearth']:
        result=u.assets.import_prop({'game_id':'Hearthward','run_id':'20261009','task_kind':'3d_object','task_id':'TASK-098-upgrades','artifact_key':name+'_path'},destination='/Game/Hearthward/Assets/TASK-098/Upgrades',options={'generate_collision':True,'combine_meshes':True})
        (out/(name+'.json')).write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
        print(name,result['ok'],result.get('errors'),flush=True)
        if not result['ok']:break
finally:print(u.runtime.stop_editor(r['payload']['process_id']),flush=True)
