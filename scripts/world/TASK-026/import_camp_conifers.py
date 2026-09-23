"""Import two CC0 conifers as isolated TASK-026 UE assets for CAMP_A."""
from pathlib import Path
import json
import time
import traceback

import unreal


root = Path(unreal.Paths.project_dir()).resolve()
local = root / 'Saved/Task026/ReworkV2'
request = json.loads((local / 'conifer-assets-request.json').read_text(encoding='utf-8'))
output = root / request['output']
assert not output.exists()
output.parent.mkdir(parents=True, exist_ok=True)
base = '/Game/Hearthward/Assets/NaturalWorld/Rebuild'
source = root / 'art_source/TASK-004/polyhaven/树木'
derived = root / 'art_source/TASK-026/Rebuild/ReworkV2/conifers'
species = {'Fir': ('fir_tree_01', '冷杉树'), 'Pine': ('pine_tree_01', '松树')}
parts = {'Bark': 'bark', 'TrunkA': 'trunk_a', 'TrunkC': 'trunk_c', 'Twig': 'twig'}
channels = {'D': 'diff', 'N': 'nor_gl', 'R': 'rough', 'A': 'alpha'}
textures = {}
for label, (slug, folder) in species.items():
    for part, prefix in parts.items():
        for channel in ('D', 'N', 'R', 'A') if part == 'Twig' else ('D', 'N', 'R'):
            name = f'T_Camp{label}_{part}_{channel}'
            path = source / folder / 'textures' / f'{slug}_{prefix}_{channels[channel]}_4k.png'
            assert path.is_file(), path
            textures[name] = (path, channel)

packages = [base + '/Textures/' + name for name in textures]
packages += [base + f'/Materials/M_Camp{label}_{part}'
             for label in species for part in parts]
packages += [base + f'/Meshes/SM_Camp{label}{variant.upper()}' +
             ('_CampUV' if label == 'Fir' else '')
             for label in species for variant in ('a', 'c')]
plan = {
    'source_commit': '73bb10ec4c19260cb72112c7e282a2c29f6c2432',
    'add_packages': packages, 'update_packages': [], 'delete_packages': [],
    'mesh_sources': [str(p.relative_to(root).as_posix())
                     for p in sorted(derived.glob('*.fbx'))],
    'texture_count': len(textures), 'texture_max_size': 2048,
    'collision': 'No canopy collision; existing paired HISM trunk proxies remain',
}
assert len(packages) == 38 and len(plan['mesh_sources']) == 4
tools = unreal.AssetToolsHelpers.get_asset_tools()
ml = unreal.MaterialEditingLibrary
save = unreal.EditorLoadingAndSavingUtils


def texture_import(name, path, channel):
    task = unreal.AssetImportTask()
    task.set_editor_properties(dict(
        filename=str(path), destination_path=base + '/Textures',
        destination_name=name, automated=True, save=False, replace_existing=False,
    ))
    tools.import_asset_tasks([task])
    asset = unreal.load_asset(base + '/Textures/' + name)
    assert asset, name
    props = dict(max_texture_size=2048, srgb=channel == 'D')
    if channel == 'N':
        props.update(compression_settings=unreal.TextureCompressionSettings.TC_NORMALMAP,
                     flip_green_channel=True)
    elif channel in ('R', 'A'):
        props['compression_settings'] = unreal.TextureCompressionSettings.TC_MASKS
    if channel == 'A':
        props.update(do_scale_mips_for_alpha_coverage=True,
                     alpha_coverage_thresholds=unreal.Vector4(.35, 0, 0, 0))
    asset.set_editor_properties(props)
    return asset


def sample(material, texture, channel):
    node = ml.create_material_expression(material, unreal.MaterialExpressionTextureSample)
    node.set_editor_property('texture', texture)
    sampler = (unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if channel == 'N'
               else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if channel == 'D'
               else unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    node.set_editor_property('sampler_type', sampler)
    return node


def material_create(label, part, imported):
    material = tools.create_asset(f'M_Camp{label}_{part}', base + '/Materials',
                                  unreal.Material, unreal.MaterialFactoryNew())
    assert material
    material.set_editor_property('used_with_instanced_static_meshes', True)
    for channel, prop in [('D', unreal.MaterialProperty.MP_BASE_COLOR),
                          ('N', unreal.MaterialProperty.MP_NORMAL),
                          ('R', unreal.MaterialProperty.MP_ROUGHNESS)]:
        node = sample(material, imported[f'T_Camp{label}_{part}_{channel}'], channel)
        assert ml.connect_material_property(node, 'R' if channel == 'R' else 'RGB', prop)
    if part == 'Twig':
        material.set_editor_properties(dict(
            blend_mode=unreal.BlendMode.BLEND_MASKED, two_sided=True,
            shading_model=unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE,
            opacity_mask_clip_value=.2,
        ))
        alpha = sample(material, imported[f'T_Camp{label}_Twig_A'], 'A')
        alpha.set_editor_properties(dict(
            mip_value_mode=unreal.TextureMipValueMode.TMVM_MIP_BIAS,
            const_mip_value=-2,
        ))
        assert ml.connect_material_property(alpha, 'R', unreal.MaterialProperty.MP_OPACITY_MASK)
        tint = ml.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
        tint.set_editor_property('constant', unreal.LinearColor(.04, .085, .025, 1))
        assert ml.connect_material_property(tint, '', unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    ml.recompile_material(material)
    return material


def mesh_import(label, slug, variant, materials):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.SystemLibrary.execute_console_command(world, 'Interchange.FeatureFlags.Import.FBX 0')
    options = unreal.FbxImportUI()
    options.set_editor_properties(dict(
        import_mesh=True, import_as_skeletal=False, import_materials=False,
        import_textures=False, automated_import_should_detect_type=False,
        mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH,
    ))
    options.static_mesh_import_data.set_editor_properties(dict(
        combine_meshes=True, auto_generate_collision=False,
        import_mesh_lo_ds=False, generate_lightmap_u_vs=False,
    ))
    name = f'SM_Camp{label}{variant.upper()}' + ('_CampUV' if label == 'Fir' else '')
    task = unreal.AssetImportTask()
    task.set_editor_properties(dict(
        filename=str(derived / f'{slug}_{variant}.fbx'),
        destination_path=base + '/Meshes', destination_name=name,
        automated=True, save=False, replace_existing=False, options=options,
    ))
    tools.import_asset_tasks([task])
    mesh = unreal.load_asset(base + '/Meshes/' + name)
    assert mesh, name
    slots = []
    for index, slot in enumerate(mesh.static_materials):
        imported_name = str(slot.material_slot_name).lower()
        part = ('Twig' if 'twig' in imported_name else
                'TrunkA' if 'trunk_a' in imported_name else
                'TrunkC' if 'trunk_c' in imported_name else
                'Bark' if 'bark' in imported_name or 'dead_branch' in imported_name else None)
        assert part, imported_name
        mesh.set_material(index, materials[part])
        slots.append([imported_name, part])
    assert {p for _, p in slots} >= {'Bark', 'Twig', 'Trunk' + variant.upper()}
    editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    nanite = editor.get_nanite_settings(mesh)
    nanite.set_editor_properties(dict(
        enabled=True, shape_preservation=unreal.NaniteShapePreservation.PRESERVE_AREA,
    ))
    editor.set_nanite_settings(mesh, nanite)
    height_cm = mesh.get_bounds().box_extent.z * 2
    assert 1200 < height_cm < 2200, (name, height_cm)
    assert editor.get_simple_collision_count(mesh) == 0
    return {'mesh': mesh.get_path_name(), 'height_cm': round(height_cm, 2),
            'slots': slots, 'simple_collision': 0}


try:
    if request['action'] == 'plan':
        assert all(not unreal.EditorAssetLibrary.does_asset_exist(p) for p in packages)
        output.write_text(json.dumps(plan, ensure_ascii=False, indent=2), encoding='utf-8')
    else:
        assert request['action'] == 'apply'
        assert plan == json.loads((root / request['plan']).read_text(encoding='utf-8'))
        assert all(not unreal.EditorAssetLibrary.does_asset_exist(p) for p in packages)
        assert not save.get_dirty_content_packages() and not save.get_dirty_map_packages()
        lockfile = root / 'Saved/Task026/rework-v2-locks.json'
        assert time.time() - lockfile.stat().st_mtime < 1800
        locks = json.loads(lockfile.read_text(encoding='utf-8'))
        locks = locks if isinstance(locks, list) else locks['locks']
        owned = {item['path'] for item in locks if item['owner']['name'] == 'XLingyyy'}
        assert all('Content/' + p[6:] + '.uasset' in owned for p in packages)
        imported = {name: texture_import(name, *spec) for name, spec in textures.items()}
        results = []
        for label, (slug, _) in species.items():
            materials = {part: material_create(label, part, imported) for part in parts}
            for variant in ('a', 'c'):
                results.append(mesh_import(label, slug, variant, materials))
        dirty = {p.get_path_name() for p in
                 [*save.get_dirty_content_packages(), *save.get_dirty_map_packages()]}
        assert dirty == set(packages), sorted(dirty ^ set(packages))
        for package in packages:
            assert unreal.EditorAssetLibrary.save_asset(package, only_if_is_dirty=True)
        output.write_text(json.dumps({'status': 'SAVED_PENDING_VISUAL_REVIEW',
                                      'plan': plan, 'meshes': results},
                                     ensure_ascii=False, indent=2), encoding='utf-8')
except Exception:
    output.write_text(json.dumps({'error': traceback.format_exc()},
                                 ensure_ascii=False, indent=2), encoding='utf-8')
    raise
