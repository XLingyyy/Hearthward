"""Import the courtyard facade and preserve its baked-color material through UEClient."""
from pathlib import Path
import json,sys,time
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT.parent))
from engine_adapters.ue5 import UEClient
from pipeline.common.paths import task_output_dir
ue=UEClient(project_path=ROOT/'Hearthward.uproject',ue_root='G:/UnrealEngine/UE_5.8')
launch=ue.runtime.launch_editor(extra_args=['-Unattended','-NullRHI','-NoSound','-NoLiveCoding','-NoSourceControl','-RCWebControlEnable'])
assert launch['ok'],launch
print('launch',launch['ok'],flush=True)
try:
    for _ in range(60):
        if ue.observe.check_status(timeout=3).get('payload',{}).get('python_execution',{}).get('ok'):break
        time.sleep(2)
    else:raise RuntimeError('Editor Python not ready')
    desc=dict(game_id='Hearthward',task_kind='3d_object',task_id='TASK-077-facade',run_id='stonehold-20261006',artifact_key='model_path')
    result=ue.assets.import_prop(desc,destination='/Game/Hearthward/Assets/TASK-077',options={'generate_collision':False})
    (ROOT/'docs/qa/TASK-077/facade-import.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8');assert result['ok'],result
    print('import',result['ok'],flush=True)
    desc['artifact_key']='material_path';folder=task_output_dir(desc['game_id'],desc['task_kind'],desc['task_id'],run_id=desc['run_id'])
    result=ue.bindings.bind_pbr_material(asset_id='StoneholdFacade',source=desc,
        mesh_assets=['/Game/Hearthward/Assets/TASK-077/SM_StoneholdFacade/StaticMeshes/SM_StoneholdFacade'],
        destination='/Game/Hearthward/Assets/TASK-077',options={'auto':False,'used_with_nanite':True,'unlit':True,'two_sided':True,'textures':{'base_color':str(folder/'facade_base_color.png')}})
    (ROOT/'docs/qa/TASK-077/facade-material.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8');assert result['ok'],result
    print('material',result['ok'],flush=True)
finally:print('stop',ue.runtime.stop_editor(launch['payload']['process_id'])['ok'],flush=True)
