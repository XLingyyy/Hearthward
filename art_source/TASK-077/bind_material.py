"""Bind the exported atlas through the public UE material API."""
from pathlib import Path
import json
import sys
import time
import trimesh

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT.parent))
from pipeline.common.paths import task_output_dir, write_task_meta
from engine_adapters.ue5 import UEClient

descriptor = dict(game_id='Hearthward', task_kind='3d_object', task_id='TASK-077-atlas',
                  run_id='stonehold-20261006', artifact_key='material_path')
folder = task_output_dir(descriptor['game_id'], descriptor['task_kind'], descriptor['task_id'], run_id=descriptor['run_id'])
folder.mkdir(parents=True, exist_ok=True)
mesh = trimesh.load(Path(__file__).with_name('stonehold-textured.glb'), force='scene').to_geometry()
atlas = folder/'stonehold_base_color.png'
mesh.visual.material.baseColorTexture.save(atlas)
write_task_meta(folder, {**descriptor, 'material_path': str(atlas)})
ue = UEClient(project_path=ROOT/'Hearthward.uproject', ue_root='G:/UnrealEngine/UE_5.8')
launch = ue.runtime.launch_editor(extra_args=['-Unattended', '-NullRHI', '-NoSound', '-NoLiveCoding', '-NoSourceControl', '-RCWebControlEnable'])
assert launch['ok'], launch
print('launch', launch['ok'], flush=True)
try:
    for _ in range(60):
        if ue.observe.check_status(timeout=3).get('payload', {}).get('python_execution', {}).get('ok'):
            break
        time.sleep(2)
    else:
        raise RuntimeError('Editor Python not ready')
    result = ue.bindings.bind_pbr_material(asset_id='Stonehold', source=descriptor,
        mesh_assets=['/Game/Hearthward/Assets/TASK-077/SM_StoneholdVista/StaticMeshes/SM_StoneholdVista'],
        destination='/Game/Hearthward/Assets/TASK-077',
        options={'auto': False, 'used_with_nanite': True, 'textures': {'base_color': str(atlas)}})
    (ROOT/'docs/qa/TASK-077/bind-material.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(result, ensure_ascii=False), flush=True)
finally:
    print('stop', ue.runtime.stop_editor(launch['payload']['process_id'])['ok'], flush=True)
