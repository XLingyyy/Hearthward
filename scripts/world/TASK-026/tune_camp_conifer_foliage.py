"""Retain fir and pine needle coverage at the CAMP_A viewing distance."""
from pathlib import Path
import json
import shutil
import stat
import time
import traceback

import unreal


root = Path(unreal.Paths.project_dir()).resolve()
local = root / 'Saved/Task026/ReworkV2'
request = json.loads((local / 'camp-conifer-foliage-request.json').read_text(encoding='utf-8'))
output = root / request['output']
assert not output.exists()
output.parent.mkdir(parents=True, exist_ok=True)
base = '/Game/Hearthward/Assets/NaturalWorld/Rebuild'
species = ('Fir', 'Pine')
packages = [base + f'/Textures/T_Camp{label}_Twig_A' for label in species]
packages += [base + f'/Materials/M_Camp{label}_Twig' for label in species]
save = unreal.EditorLoadingAndSavingUtils
ml = unreal.MaterialEditingLibrary


def alpha_node(material, texture):
    nodes = [node for node in unreal.ObjectIterator(unreal.MaterialExpressionTextureSample)
             if node.get_outer() == material and node.get_editor_property('texture') == texture]
    assert len(nodes) == 1, (material.get_name(), len(nodes))
    return nodes[0]


before = []
for label in species:
    texture = unreal.load_asset(base + f'/Textures/T_Camp{label}_Twig_A')
    material = unreal.load_asset(base + f'/Materials/M_Camp{label}_Twig')
    assert texture and material
    node = alpha_node(material, texture)
    before.append({
        'species': label,
        'texture': texture.get_path_name(),
        'scale_alpha_mips': texture.get_editor_property('do_scale_mips_for_alpha_coverage'),
        'alpha_thresholds': [round(getattr(texture.get_editor_property('alpha_coverage_thresholds'),
                                           axis), 6) for axis in 'xyzw'],
        'material': material.get_path_name(),
        'opacity_clip': material.get_editor_property('opacity_mask_clip_value'),
        'mip_mode': str(node.get_editor_property('mip_value_mode')),
        'mip_value': node.get_editor_property('const_mip_value'),
    })
plan = {'update_packages': packages, 'add_packages': [], 'delete_packages': [],
        'before': before,
        'settings': {'scale_alpha_mips': True, 'alpha_threshold': .35,
                     'opacity_clip': .2, 'mask_mip_bias': -2},
        'basis': 'Existing M_Tree_leaves far-distance alpha coverage settings'}


try:
    if request['action'] == 'plan':
        output.write_text(json.dumps(plan, ensure_ascii=False, indent=2), encoding='utf-8')
    else:
        assert request['action'] == 'apply'
        assert plan == json.loads((root / request['plan']).read_text(encoding='utf-8'))
        assert not save.get_dirty_content_packages() and not save.get_dirty_map_packages()
        lockfile = root / 'Saved/Task026/rework-v2-locks.json'
        assert time.time() - lockfile.stat().st_mtime < 1800
        locks = json.loads(lockfile.read_text(encoding='utf-8'))
        locks = locks if isinstance(locks, list) else locks['locks']
        owned = {item['path'] for item in locks if item['owner']['name'] == 'XLingyyy'}
        backup = local / 'backups/camp-conifer-foliage'
        assert not backup.exists()
        for package in packages:
            file = root / ('Content/' + package[6:] + '.uasset')
            relative = file.relative_to(root).as_posix()
            assert relative in owned, relative
            destination = backup / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(file, destination)
            file.chmod(file.stat().st_mode | stat.S_IWRITE)
        for label in species:
            texture = unreal.load_asset(base + f'/Textures/T_Camp{label}_Twig_A')
            material = unreal.load_asset(base + f'/Materials/M_Camp{label}_Twig')
            texture.set_editor_properties(dict(
                do_scale_mips_for_alpha_coverage=True,
                alpha_coverage_thresholds=unreal.Vector4(.35, 0, 0, 0),
            ))
            node = alpha_node(material, texture)
            node.set_editor_properties(dict(
                mip_value_mode=unreal.TextureMipValueMode.TMVM_MIP_BIAS,
                const_mip_value=-2,
            ))
            material.set_editor_property('opacity_mask_clip_value', .2)
            ml.recompile_material(material)
        dirty = [*save.get_dirty_content_packages(), *save.get_dirty_map_packages()]
        assert {p.get_path_name() for p in dirty} == set(packages)
        assert save.save_packages(dirty, True)
        output.write_text(json.dumps({
            'status': 'PASS', 'plan': plan, 'saved_packages': sorted(packages),
            'backup': backup.relative_to(root).as_posix(),
        }, ensure_ascii=False, indent=2), encoding='utf-8')
except Exception:
    output.write_text(json.dumps({'error': traceback.format_exc()},
                                 ensure_ascii=False, indent=2), encoding='utf-8')
    raise
