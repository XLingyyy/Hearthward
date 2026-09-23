"""Apply reviewed S1 grass-island masks and a grass-only distance-fade material."""
from pathlib import Path
import json
import shutil
import stat
import time
import unreal

root=Path(unreal.Paths.project_dir()).resolve()
local=root/'Saved/Task026/ReworkV2'
config=json.loads((local/'s1-detail-assets.json').read_text(encoding='utf-8'))
out=root/config['output'];out.mkdir(parents=True,exist_ok=False)
base='/Game/Hearthward/Assets/NaturalWorld/Rebuild'
mesh_source=base+'/Meshes/GrassCards/StaticMeshes/SM_GrassCards'
mesh_dest=base+'/Meshes/SM_S1_GrassCards'
material_dest=base+'/Materials/M_S1_GrassCards'
gt_path=base+'/Foliage/GT_S1_Meadow'
textures={base+'/Textures/T_S1_GrassDensity':'S1_GrassDensity_Detail.png',
          base+'/Textures/T_S1_Surface':'S1_Surface_Detail.png'}
proxies=json.loads((root/'docs/qa/evidence/TASK-026/rework-v2/S1-grass-maps-plan-02/plan.json').read_text(encoding='utf-8'))['update_packages']
source=unreal.load_asset(mesh_source)
material_source=source.get_material(0)
assert isinstance(material_source,unreal.Material)
plan={'add_packages':[material_dest,mesh_dest],
      'update_packages':[gt_path,*textures,*proxies], 'delete_packages':[],
      'source_mesh':mesh_source,'source_material':material_source.get_path_name(),
      'source_masks':textures,'scope':'S1 grass type only; duplicate old mesh/material; masks bounded to S1; persist 4 grass-map proxies'}
save=unreal.EditorLoadingAndSavingUtils
if config['action']=='plan':
    (out/'plan.json').write_text(json.dumps(plan,indent=2),encoding='utf-8')
else:
    assert config['action']=='apply'
    assert plan==json.loads((root/config['plan']).read_text(encoding='utf-8'))
    resume=config.get('resume_after_readonly_variety',False)
    dirty_before={p.get_path_name() for p in [*save.get_dirty_content_packages(),*save.get_dirty_map_packages()]}
    assert dirty_before==set(plan['add_packages']) if resume else not dirty_before
    lockfile=root/'Saved/Task026/rework-v2-locks.json'
    assert time.time()-lockfile.stat().st_mtime<1800
    locks=json.loads(lockfile.read_text(encoding='utf-8'))
    locks=locks if isinstance(locks,list) else locks['locks']
    own={x['path'] for x in locks if x['owner']['name']=='XLingyyy'}
    packages=plan['add_packages']+plan['update_packages']
    assert all('Content/'+p[6:]+'.uasset' in own for p in packages)
    backup=local/'backups/s1-detail-assets'
    assert backup.exists() if resume else not backup.exists()
    for p in plan['update_packages']:
        path=root/('Content/'+p[6:]+'.uasset');dest=backup/path.relative_to(root)
        if not resume:
            dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,dest)
        path.chmod(path.stat().st_mode|stat.S_IWRITE)
    if not resume:
        for p in plan['add_packages']:assert not unreal.EditorAssetLibrary.does_asset_exist(p)
    mat=unreal.load_asset(material_dest) if resume else unreal.EditorAssetLibrary.duplicate_asset(material_source.get_path_name(),material_dest)
    mesh=unreal.load_asset(mesh_dest) if resume else unreal.EditorAssetLibrary.duplicate_asset(mesh_source,mesh_dest)
    assert mat and mesh
    ml=unreal.MaterialEditingLibrary
    opacity=unreal.MaterialProperty.MP_OPACITY_MASK
    old=ml.get_material_property_input_node(mat,opacity)
    pin=ml.get_material_property_input_node_output_name(mat,opacity)
    assert old
    if resume:
        assert isinstance(old,unreal.MaterialExpressionMultiply)
        assert any(isinstance(n,unreal.MaterialExpressionPerInstanceFadeAmount) for n in ml.get_inputs_for_material_expression(mat,old))
        assert mesh.get_material(0)==mat
    else:
        fade=ml.create_material_expression(mat,unreal.MaterialExpressionPerInstanceFadeAmount)
        multiply=ml.create_material_expression(mat,unreal.MaterialExpressionMultiply)
        assert ml.connect_material_expressions(old,pin,multiply,'A')
        assert ml.connect_material_expressions(fade,'',multiply,'B')
        assert ml.connect_material_property(multiply,'',opacity)
        mat.set_editor_property('used_with_instanced_static_meshes',True)
        ml.recompile_material(mat)
        mesh.set_material(0,mat)
    gt=unreal.load_asset(gt_path)
    varieties=list(gt.grass_varieties)
    assert len(varieties)==1
    varieties[0].set_editor_property('grass_mesh',mesh)
    gt.set_editor_property('grass_varieties',varieties)
    for p,filename in textures.items():
        task=unreal.AssetImportTask()
        task.set_editor_properties(dict(filename=str(root/'art_source/TASK-026/Rebuild/ReworkV2'/filename),
            destination_path=p.rsplit('/',1)[0],destination_name=p.rsplit('/',1)[1],
            automated=True,save=False,replace_existing=True,replace_existing_settings=False))
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        texture=unreal.load_asset(p)
        texture.set_editor_properties(dict(srgb=False,compression_settings=unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP,
            max_texture_size=0,mip_gen_settings=unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS))
    dirty=[*save.get_dirty_content_packages(),*save.get_dirty_map_packages()]
    assert {p.get_path_name() for p in dirty}<=set(packages)
    assert save.save_packages(dirty,True)
    unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
    landscape_packages={a.get_package().get_path_name():a.get_package()
        for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
        if isinstance(a,unreal.LandscapeProxy)}
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    # Invalidate generated weights after the mask reimport; the four selected
    # proxies rebuild their maps through native PreSave before serialization.
    unreal.SystemLibrary.execute_console_command(world,'grass.FlushCache')
    assert save.save_packages([landscape_packages[p] for p in proxies],False)
    (out/'apply.json').write_text(json.dumps({'status':'SAVED_PENDING_NATIVE_REVIEW','plan':plan,
        'remaining_dirty':[p.get_path_name() for p in [*save.get_dirty_content_packages(),*save.get_dirty_map_packages()]]},indent=2),encoding='utf-8')
