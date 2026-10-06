"""Import the last prepared courtyard variant for QA (default-lit comparison material).

Run prepare_courtyard_v2.py for vertex colors, or prepare_courtyard_textured.py
then pass --textured. The source textured GLB declares KHR_materials_unlit;
this diagnostic binding must not be accepted as a production material.
"""
from pathlib import Path
import json,sys,time
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT.parent))
from engine_adapters.ue5 import UEClient
ue=UEClient(project_path=ROOT/'Hearthward.uproject',ue_root='G:/UnrealEngine/UE_5.8')
launch=ue.runtime.launch_editor(extra_args=['-Unattended','-NullRHI','-NoSound','-NoLiveCoding','-NoSourceControl','-RCWebControlEnable'])
assert launch['ok'],launch
print('launch',launch['ok'],flush=True)
try:
    for _ in range(60):
        if ue.observe.check_status(timeout=3).get('payload',{}).get('python_execution',{}).get('ok'):break
        time.sleep(2)
    else:raise RuntimeError('Editor Python not ready')
    desc=dict(game_id='Hearthward',task_kind='3d_object',task_id='TASK-077-courtyard-v2',run_id='stonehold-20261006',artifact_key='model_path')
    if '--bind-only' not in sys.argv:
        result=ue.assets.import_prop(desc,destination='/Game/Hearthward/Assets/TASK-077',options={'generate_collision':False})
        (ROOT/'docs/qa/TASK-077/courtyard-v2-import.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
        print('import',json.dumps(result,ensure_ascii=False),flush=True)
        assert result['ok'],result
    desc['artifact_key']='material_path'
    options={'auto':False,'used_with_nanite':True,'use_vertex_color':True,'two_sided':True}
    if '--textured' in sys.argv:
        from pipeline.common.paths import task_output_dir
        folder=task_output_dir(desc['game_id'],desc['task_kind'],desc['task_id'],run_id=desc['run_id'])
        options={'auto':False,'used_with_nanite':True,'two_sided':True,'textures':{'base_color':str(folder/'courtyard_base_color.png')}}
    result=ue.bindings.bind_pbr_material(asset_id='StoneholdCourtyard',source=desc,
        mesh_assets=['/Game/Hearthward/Assets/TASK-077/SM_StoneholdCourtyard/StaticMeshes/SM_StoneholdCourtyard'],
        destination='/Game/Hearthward/Assets/TASK-077',options=options)
    (ROOT/'docs/qa/TASK-077/courtyard-v2-material.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print('material',json.dumps(result,ensure_ascii=False),flush=True)
finally:
    print('stop',ue.runtime.stop_editor(launch['payload']['process_id'])['ok'],flush=True)
