"""Import corrected fir UV meshes to new packages without replacing loaded assets."""
from pathlib import Path
import json
import time
import traceback

import unreal


root = Path(unreal.Paths.project_dir()).resolve()
local = root / 'Saved/Task026/ReworkV2'
request = json.loads((local / 'camp-fir-uv-new-request.json').read_text(encoding='utf-8'))
output = root / request['output']
assert not output.exists()
output.parent.mkdir(parents=True, exist_ok=True)
base = '/Game/Hearthward/Assets/NaturalWorld/Rebuild'
source = root / 'art_source/TASK-026/Rebuild/ReworkV2/conifers'
names = {'a': 'SM_CampFirA_CampUV', 'c': 'SM_CampFirC_CampUV'}
packages = [base + '/Meshes/' + name for name in names.values()]
plan = {
    'source': 'TASK-004 fir_tree_01_4k.blend generic UVMap corner vector',
    'fix': 'Blender CampUV layer exported through FBX',
    'add_packages': packages, 'update_packages': [], 'delete_packages': [],
    'mesh_sources': [str((source / f'fir_tree_01_{variant}.fbx').relative_to(root).as_posix())
                     for variant in names],
}
save = unreal.EditorLoadingAndSavingUtils


def import_mesh(variant, name):
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
    task = unreal.AssetImportTask()
    task.set_editor_properties(dict(
        filename=str(source / f'fir_tree_01_{variant}.fbx'),
        destination_path=base + '/Meshes', destination_name=name,
        automated=True, save=False, replace_existing=False, options=options,
    ))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.load_asset(base + '/Meshes/' + name)
    assert mesh, name
    slots = []
    for index, slot in enumerate(mesh.static_materials):
        imported = str(slot.material_slot_name).lower()
        part = ('Twig' if 'twig' in imported else
                'TrunkA' if 'trunk_a' in imported else
                'TrunkC' if 'trunk_c' in imported else
                'Bark' if 'bark' in imported or 'dead_branch' in imported else None)
        assert part, imported
        material = unreal.load_asset(base + f'/Materials/M_CampFir_{part}')
        assert material
        mesh.set_material(index, material)
        slots.append([imported, part])
    assert {part for _, part in slots} >= {'Bark', 'Twig', 'Trunk' + variant.upper()}
    editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    nanite = editor.get_nanite_settings(mesh)
    nanite.set_editor_properties(dict(
        enabled=True, shape_preservation=unreal.NaniteShapePreservation.PRESERVE_AREA,
    ))
    editor.set_nanite_settings(mesh, nanite)
    uv_channels = editor.get_num_uv_channels(mesh, 0)
    assert uv_channels >= 1, (name, uv_channels)
    assert editor.get_simple_collision_count(mesh) == 0
    height = round(mesh.get_bounds().box_extent.z * 2, 2)
    assert 1200 < height < 2200, (name, height)
    return {'mesh': mesh.get_path_name(), 'uv_channels': uv_channels,
            'height_cm': height, 'slots': slots, 'simple_collision': 0}


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
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        unreal.SystemLibrary.execute_console_command(world, 'Interchange.FeatureFlags.Import.FBX 0')
        results = [import_mesh(variant, name) for variant, name in names.items()]
        dirty = [*save.get_dirty_content_packages(), *save.get_dirty_map_packages()]
        assert {p.get_path_name() for p in dirty} == set(packages)
        assert save.save_packages(dirty, True)
        output.write_text(json.dumps({'status': 'PASS', 'plan': plan, 'results': results},
                                     ensure_ascii=False, indent=2), encoding='utf-8')
except Exception:
    output.write_text(json.dumps({'error': traceback.format_exc()},
                                 ensure_ascii=False, indent=2), encoding='utf-8')
    raise
