"""Add only the reviewed CC0 small-tree assets; no map or existing asset writes."""
from pathlib import Path
import hashlib
import json
import time
import traceback
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
local = root/'Saved/Task026/ReworkV2'
config = json.loads((local/'s1-tree-assets.json').read_text(encoding='utf-8'))
out = root/config['output']
out.mkdir(parents=True, exist_ok=False)
base = '/Game/Hearthward/Assets/NaturalWorld/Rebuild'
source = root/'art_source/TASK-004/polyhaven/树木/岛树/textures'
fbx = root/'art_source/TASK-026/Rebuild/ReworkV2/island_tree/island_tree_02.fbx'
tools = unreal.AssetToolsHelpers.get_asset_tools()
ml = unreal.MaterialEditingLibrary
save = unreal.EditorLoadingAndSavingUtils

def connect(a, output, b, pin):
    assert ml.connect_material_expressions(a, output, b, pin)

def expression(mat, cls, **props):
    result = ml.create_material_expression(mat, getattr(unreal, 'MaterialExpression'+cls))
    result.set_editor_properties(props)
    return result

def main():
    textures = {}
    for part in ['trunk', 'branches', 'leaves']:
        prefix = 'island_tree_02'+('' if part == 'trunk' else '_'+part)
        for channel, suffix in [('D','diff'), ('N','nor_gl'), ('R','rough')]:
            files = list(source.glob(prefix+'_'+suffix+'_4k.*'))
            assert len(files) == 1
            textures[f'T_S1_Island_{part}_{channel}'] = (files[0], channel)
    textures['T_S1_Island_leaves_A'] = (source/'island_tree_02_leaves_alpha_4k.png', 'A')
    packages = [base+'/Textures/'+name for name in textures]
    packages += [base+'/Materials/M_S1_Island_'+p for p in ['trunk','branches','leaves']]
    packages += [base+'/Meshes/SM_S1_IslandTree']
    inputs = [fbx, Path(__file__), *[p for p,c in textures.values()]]
    plan = {'add_packages': packages, 'update_packages': [], 'delete_packages': [],
            'source_hashes': {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
            'scope': 'S1 small-tree asset adaptation only; source TASK-004 stays read-only',
            'texture_max_size': 2048, 'collision': 'No generated canopy collision; placement requires separate trunk proxy'}
    plan['plan_hash'] = hashlib.sha256(json.dumps(plan,sort_keys=True).encode()).hexdigest()
    assert all(not unreal.EditorAssetLibrary.does_asset_exist(p) for p in packages)
    if config['action'] == 'plan':
        (out/'plan.json').write_text(json.dumps(plan,indent=2),encoding='utf-8')
        return
    assert config['action'] == 'apply'
    assert plan == json.loads((root/config['plan']).read_text(encoding='utf-8'))
    assert not save.get_dirty_content_packages() and not save.get_dirty_map_packages()
    lockfile = root/'Saved/Task026/rework-v2-locks.json'
    assert time.time()-lockfile.stat().st_mtime < 1800
    locks = json.loads(lockfile.read_text(encoding='utf-8'))
    locks = locks if isinstance(locks,list) else locks['locks']
    owned = {x['path'] for x in locks if x['owner']['name']=='XLingyyy'}
    assert all('Content/'+p[6:]+'.uasset' in owned for p in packages)
    imported = {}
    for name,(path,channel) in textures.items():
        task = unreal.AssetImportTask()
        task.set_editor_properties(dict(filename=str(path),destination_path=base+'/Textures',destination_name=name,
                                       automated=True,save=False,replace_existing=False))
        tools.import_asset_tasks([task])
        tex = unreal.load_asset(base+'/Textures/'+name)
        props = dict(max_texture_size=2048,srgb=channel=='D')
        if channel=='N':
            props.update(compression_settings=unreal.TextureCompressionSettings.TC_NORMALMAP,flip_green_channel=True)
        elif channel in ['A','R']:
            props.update(compression_settings=unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_properties(props)
        imported[name] = tex
    mats = {}
    for part in ['trunk','branches','leaves']:
        mat = tools.create_asset('M_S1_Island_'+part,base+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
        for channel,prop in [('D',unreal.MaterialProperty.MP_BASE_COLOR),('N',unreal.MaterialProperty.MP_NORMAL),('R',unreal.MaterialProperty.MP_ROUGHNESS)]:
            sampler = unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if channel=='N' else (unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if channel=='D' else unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
            node = expression(mat,'TextureSample',texture=imported[f'T_S1_Island_{part}_{channel}'],sampler_type=sampler)
            assert ml.connect_material_property(node,'R' if channel=='R' else 'RGB',prop)
        if part=='leaves':
            mat.set_editor_properties(dict(blend_mode=unreal.BlendMode.BLEND_MASKED,two_sided=True,
                shading_model=unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE,opacity_mask_clip_value=.35))
            alpha = expression(mat,'TextureSample',texture=imported['T_S1_Island_leaves_A'],sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
            assert ml.connect_material_property(alpha,'R',unreal.MaterialProperty.MP_OPACITY_MASK)
            tint = expression(mat,'Constant3Vector',constant=unreal.LinearColor(.045,.08,.018,1))
            assert ml.connect_material_property(tint,'',unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
        ml.recompile_material(mat)
        mats[part]=mat
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.SystemLibrary.execute_console_command(world,'Interchange.FeatureFlags.Import.FBX 0')
    options = unreal.FbxImportUI()
    options.set_editor_properties(dict(import_mesh=True,import_as_skeletal=False,import_materials=False,import_textures=False,
        automated_import_should_detect_type=False,mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH))
    options.static_mesh_import_data.set_editor_properties(dict(combine_meshes=True,auto_generate_collision=False,
        import_mesh_lo_ds=False,generate_lightmap_u_vs=False))
    task = unreal.AssetImportTask()
    task.set_editor_properties(dict(filename=str(fbx),destination_path=base+'/Meshes',destination_name='SM_S1_IslandTree',
        automated=True,save=False,replace_existing=False,options=options))
    tools.import_asset_tasks([task])
    mesh = unreal.load_asset(base+'/Meshes/SM_S1_IslandTree')
    assert mesh
    slots=[]
    for index,slot in enumerate(mesh.static_materials):
        name=str(slot.material_slot_name).lower()
        part = 'leaves' if 'leaves' in name else ('branches' if 'branches' in name else 'trunk')
        mesh.set_material(index,mats[part]);slots.append([name,part])
    assert len(slots)==3 and {p for n,p in slots}==set(mats)
    subsystem=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    settings=subsystem.get_nanite_settings(mesh)
    settings.set_editor_properties(dict(enabled=True,shape_preservation=unreal.NaniteShapePreservation.PRESERVE_AREA))
    subsystem.set_nanite_settings(mesh,settings)
    bounds=mesh.get_bounds()
    assert 300 < bounds.box_extent.z*2 < 400, str(bounds)
    dirty={p.get_path_name() for p in [*save.get_dirty_content_packages(),*save.get_dirty_map_packages()]}
    assert dirty==set(packages),str(dirty)
    for package in packages:
        assert unreal.EditorAssetLibrary.save_asset(package,only_if_is_dirty=True)
    (out/'apply.json').write_text(json.dumps({'status':'SAVED_PENDING_NATIVE_VISUAL_REVIEW','plan':plan,
        'slots':slots,'bounds':str(bounds),'simple_collision_count':subsystem.get_simple_collision_count(mesh)},indent=2),encoding='utf-8')

try:
    main()
except Exception:
    (out/'failure.json').write_text(json.dumps({'error':traceback.format_exc()}),encoding='utf-8')
    raise
