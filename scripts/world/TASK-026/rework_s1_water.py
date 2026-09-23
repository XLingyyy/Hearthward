"""Bounded S1 water-normal refinement; retain the original output outside S1."""
from pathlib import Path
import hashlib
import json
import shutil
import stat
import time
import traceback
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
local = root / 'Saved/Task026/ReworkV2'
config = json.loads((local / 's1-water.json').read_text(encoding='utf-8'))
out = root / config['output']
out.mkdir(parents=True, exist_ok=False)
base = '/Game/Hearthward/Assets/NaturalWorld/Rebuild'
package = base + '/Materials/M_RiverWater'
ml = unreal.MaterialEditingLibrary
save = unreal.EditorLoadingAndSavingUtils


def expr(mat, kind, name, **properties):
    node = ml.create_material_expression(mat, getattr(unreal, 'MaterialExpression' + kind))
    node.set_editor_property('desc', 'TASK026.S1.Water.' + name)
    if properties:
        node.set_editor_properties(properties)
    return node


def link(source, target, pin, output=''):
    assert ml.connect_material_expressions(source, output, target, pin), pin


def custom(mat, name, code, inputs, output_type):
    pins = []
    for key in inputs:
        item = unreal.CustomInput()
        item.set_editor_property('input_name', key)
        pins.append(item)
    return expr(mat, 'Custom', name, code=code, inputs=pins, output_type=output_type)


def main():
    mat = unreal.load_asset(package)
    graph = [n for n in unreal.ObjectIterator(unreal.MaterialExpression) if n.get_outer() == mat]
    assert not any(n.get_editor_property('desc').startswith('TASK026.S1.Water.') for n in graph)
    properties = {'normal': unreal.MaterialProperty.MP_NORMAL,
                  'roughness': unreal.MaterialProperty.MP_ROUGHNESS}
    original = {k: ml.get_material_property_input_node(mat, p) for k, p in properties.items()}
    outputs = {k: ml.get_material_property_input_node_output_name(mat, p) for k, p in properties.items()}
    assert all(original.values())
    plan = {'scope': 'S1_Blend only; preserve old water output outside sample',
            'update_packages': [package], 'add_packages': [], 'delete_packages': [],
            'before_nodes': sorted(n.get_name() for n in graph),
            'before_inputs': {k: [n.get_name(), outputs[k]] for k, n in original.items()},
            'script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'normal_scales_cm': [700, 1370], 'normal_strengths': [.24, .18],
            'roughness': .24}
    if config['action'] == 'plan':
        (out / 'plan.json').write_text(json.dumps(plan, indent=2), encoding='utf-8')
        return
    assert config['action'] == 'apply'
    assert plan == json.loads((root / config['plan']).read_text(encoding='utf-8'))
    assert not save.get_dirty_content_packages() and not save.get_dirty_map_packages()
    lockfile = root / 'Saved/Task026/rework-v2-locks.json'
    assert time.time() - lockfile.stat().st_mtime < 1800
    locks = json.loads(lockfile.read_text(encoding='utf-8'))
    locks = locks if isinstance(locks, list) else locks['locks']
    asset_file = root / ('Content/' + package[6:] + '.uasset')
    assert asset_file.relative_to(root).as_posix() in {
        x['path'] for x in locks if x['owner']['name'] == 'XLingyyy'}
    backup = local / 'backups/s1-water/M_RiverWater.uasset'
    assert not backup.exists()
    backup.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(asset_file, backup)
    asset_file.chmod(asset_file.stat().st_mode | stat.S_IWRITE)
    mat.modify()
    pos = expr(mat, 'WorldPosition', 'WorldPosition')
    uv = custom(mat, 'SampleUV', 'return (P.xy+201600.0)/403200.0;', ['P'],
                unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    link(pos, uv, 'P')
    mask = expr(mat, 'TextureSample', 'BoundedMask',
                texture=unreal.load_asset(base + '/Textures/T_S1_Blend'),
                sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    link(uv, mask, 'UVs')
    samples = []
    for index, (code, speed) in enumerate([
        ('return P.xy/700.0;', (.007, -.004)),
        ('return float2(P.x*.61-P.y*.7924,P.x*.7924+P.y*.61)/1370.0;', (-.003, .006)),
    ]):
        coords = custom(mat, 'WaveUV' + str(index), code, ['P'],
                        unreal.CustomMaterialOutputType.CMOT_FLOAT2)
        link(pos, coords, 'P')
        pan = expr(mat, 'Panner', 'Flow' + str(index), speed_x=speed[0], speed_y=speed[1])
        link(coords, pan, 'Coordinate')
        sample = expr(mat, 'TextureSample', 'Normal' + str(index),
                      texture=unreal.load_asset(base + '/Textures/T_Water_N'),
                      sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        link(pan, sample, 'UVs')
        samples.append(sample)
    normal = custom(mat, 'FineWaves', 'return normalize(float3(A.xy*.24+B.xy*.18,1));',
                    ['A', 'B'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    for pin, sample in zip(['A', 'B'], samples):
        link(sample, normal, pin, 'RGB')
    roughness = expr(mat, 'Constant', 'Roughness', r=.24)
    for name, refined in [('normal', normal), ('roughness', roughness)]:
        blend = expr(mat, 'LinearInterpolate', 'Bounded' + name)
        link(original[name], blend, 'A', outputs[name])
        link(refined, blend, 'B')
        link(mask, blend, 'Alpha', 'R')
        assert ml.connect_material_property(blend, '', properties[name])
    ml.recompile_material(mat)
    dirty = [*save.get_dirty_content_packages(), *save.get_dirty_map_packages()]
    assert {p.get_path_name() for p in dirty} == {package}
    assert save.save_packages(dirty, True)
    (out / 'apply.json').write_text(json.dumps({
        'status': 'SAVED_PENDING_NATIVE_REVIEW', 'plan': plan,
        'remaining_dirty': [p.get_path_name() for p in [
            *save.get_dirty_content_packages(), *save.get_dirty_map_packages()]]}, indent=2), encoding='utf-8')


try:
    main()
except Exception:
    (out / 'failure.json').write_text(json.dumps({'error': traceback.format_exc()}), encoding='utf-8')
    raise
