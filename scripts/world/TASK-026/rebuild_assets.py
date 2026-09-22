"""Import and adapt the selected existing natural assets inside Unreal Editor."""
import sys
from pathlib import Path
import json
import traceback
import unreal

sys.path.insert(0,str(Path(__file__).parent))
import rebuild_terrain as common
from rebuild_terrain import ROOT,SRC,OUT,ASSET,tools,ml,texture,expr,sample,constant,connect,finish
meshes=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
report={}


def material(name,base,normal=None,alpha=None):
    path=ASSET+'/Materials/M_'+name
    mat=unreal.load_asset(path)
    if mat:
        ml.delete_all_material_expressions(mat)
    else:mat=tools.create_asset('M_'+name,ASSET+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    color=sample(mat,texture(base,'T_'+name+'_D'))
    tint=expr(mat,'Constant3Vector',constant=unreal.LinearColor(*((.24,.42,.19,1) if alpha else (.65,.65,.65,1))))
    tinted=expr(mat,'Multiply');connect(color,tinted,'A','RGB');connect(tint,tinted,'B')
    ml.connect_material_property(tinted,'',unreal.MaterialProperty.MP_BASE_COLOR)
    ml.connect_material_property(constant(mat,.86),'',unreal.MaterialProperty.MP_ROUGHNESS)
    if normal:
        n=sample(mat,texture(normal,'T_'+name+'_N','normal'),normal=True)
        ml.connect_material_property(n,'RGB',unreal.MaterialProperty.MP_NORMAL)
    if alpha:
        mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
        mat.set_editor_property('two_sided',True)
        mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
        mat.set_editor_property('opacity_mask_clip_value',.35)
        tint=expr(mat,'Constant3Vector',constant=unreal.LinearColor(.04,.08,.015,1))
        ml.connect_material_property(tint,'',unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
        mask=sample(mat,texture(alpha,'T_'+name+'_Alpha','data'))
        ml.connect_material_property(mask,'R',unreal.MaterialProperty.MP_OPACITY_MASK)
    return finish(mat)


def mesh_import(path,name):
    target=ASSET+'/Meshes/'+name
    mesh=unreal.load_asset(target)
    if mesh:return mesh
    task=unreal.AssetImportTask()
    options=unreal.FbxImportUI()
    options.set_editor_properties(dict(import_mesh=True,import_as_skeletal=False,import_materials=False,import_textures=False,
                                       automated_import_should_detect_type=False,mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH))
    options.static_mesh_import_data.set_editor_properties(dict(combine_meshes=True,auto_generate_collision=False,import_mesh_lo_ds=True,
                                                               generate_lightmap_u_vs=False))
    task.set_editor_properties(dict(filename=str(path),destination_path=ASSET+'/Meshes',destination_name=name,automated=True,
                                     save=True,replace_existing=False,options=options))
    tools.import_asset_tasks([task])
    mesh=unreal.load_asset(target)
    assert mesh, f'Failed mesh import {path}: {task.imported_object_paths}'
    return mesh


try:
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.SystemLibrary.execute_console_command(world,'Interchange.FeatureFlags.Import.FBX 0')
    poly=ROOT/'art_source/TASK-004/polyhaven'
    tree_dir=poly/'树木/蓝花楹树/textures'
    tree_mats={}
    for part in ['branches','trunk','leaves']:
        prefix='jacaranda_tree_'+part
        tree_mats[part]=material('Tree_'+part,tree_dir/(prefix+'_diff_4k.png'),tree_dir/(prefix+'_nor_gl_4k.png'),
                                 tree_dir/(prefix+'_alpha_4k.png') if part=='leaves' else None)
    tree=mesh_import(SRC/'polyhaven/jacaranda_tree/jacaranda_tree.fbx','SM_Tree')
    slots=[]
    for index,slot in enumerate(tree.static_materials):
        name=str(slot.material_slot_name).lower(); slots.append(name)
        part=next((p for p in tree_mats if p in name),None)
        assert part, 'Unknown tree material slot '+name
        tree.set_material(index,tree_mats[part])
    # Conventional reduction of the 3.86M-triangle source exceeded 25 minutes
    # and 20 GB commit on the target host. Let Nanite own distance reduction.
    settings=meshes.get_nanite_settings(tree)
    if not settings.enabled:
        if meshes.get_lod_count(tree)>1:assert meshes.remove_lods(tree)
        settings.set_editor_property('enabled',True)
        settings.set_editor_property('shape_preservation',unreal.NaniteShapePreservation.PRESERVE_AREA)
        meshes.set_nanite_settings(tree,settings)
    unreal.EditorAssetLibrary.save_loaded_asset(tree)
    report['tree']={'slots':slots,'bounds':str(tree.get_bounds()),'lods':meshes.get_lod_count(tree)}
    for title,folder,prefix in [('Shrub','灌木','shrub_01'),('Stump','树桩','tree_stump_01')]:
        base=poly/folder/'textures'
        mat=material(title,base/(prefix+'_diff_4k.jpg'),base/(prefix+'_nor_gl_4k.exr'),base/(prefix+'_alpha_4k.png') if title=='Shrub' else None)
        mesh=mesh_import(SRC/'polyhaven'/prefix/(prefix+'.fbx'),'SM_'+title)
        for i in range(len(mesh.static_materials)):mesh.set_material(i,mat)
        settings=meshes.get_nanite_settings(mesh)
        if not settings.enabled:
            settings.set_editor_property('enabled',True)
            if title=='Shrub':settings.set_editor_property('shape_preservation',unreal.NaniteShapePreservation.PRESERVE_AREA)
            meshes.set_nanite_settings(mesh,settings)
        if title=='Stump' and meshes.get_simple_collision_count(mesh)==0:
            meshes.add_simple_collisions(mesh,unreal.ScriptingCollisionShapeType.NDOP26)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        report[title]={'bounds':str(mesh.get_bounds()),'lods':meshes.get_lod_count(mesh)}
    rock=mesh_import(SRC/'sketchfab/中岩石/stone assets.obj','SM_Rock')
    rockdir=ROOT/'art_source/TASK-004/sketchfab/岩石/中岩石/textures'
    rockmat=material('RockScan',rockdir/'d_m.jpg')
    for i in range(len(rock.static_materials)):rock.set_material(i,rockmat)
    nanite=meshes.get_nanite_settings(rock); nanite.set_editor_property('enabled',True)
    meshes.set_nanite_settings(rock,nanite)
    if meshes.get_simple_collision_count(rock)==0:
        meshes.add_simple_collisions(rock,unreal.ScriptingCollisionShapeType.NDOP26)
    unreal.EditorAssetLibrary.save_loaded_asset(rock)
    report['rock']={'bounds':str(rock.get_bounds()),'lods':meshes.get_lod_count(rock)}
    report['ok']=True
except Exception:
    report['ok']=False; report['error']=traceback.format_exc()
(OUT/'assets.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('TASK026_REBUILD_ASSETS '+str(report))
