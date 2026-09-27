"""Import the approved provisional TASK-004 animal meshes in the isolated editor."""
import json,shutil
from pathlib import Path
import unreal

root=Path(unreal.Paths.project_dir()).resolve()
records=[]
tools=unreal.AssetToolsHelpers.get_asset_tools()
meshes=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
for source in sorted((root/'art_source/TASK-004/Tripo/动物/outputs').glob('*/*_model.fbx')):
    stem=source.stem.removesuffix('_model')
    if stem=='stag_b':continue
    name='SM_'+stem
    asset='/Game/Hearthward/Nature/'+name
    if unreal.EditorAssetLibrary.does_asset_exist(asset):
        records.append({'source':str(source.relative_to(root)),'asset':asset,'existing':True});continue
    task=unreal.AssetImportTask()
    temporary=root/'Saved/Task048/import-input'/source.name;temporary.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,temporary)
    task.filename=str(temporary);task.destination_path='/Game/Hearthward/Nature';task.destination_name=name
    task.automated=True;task.save=True;task.replace_existing=False
    options=unreal.FbxImportUI()
    options.import_mesh=True;options.import_as_skeletal=False;options.import_animations=False
    options.import_materials=False;options.import_textures=False
    options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    options.automated_import_should_detect_type=False
    options.static_mesh_import_data.combine_meshes=True
    options.static_mesh_import_data.auto_generate_collision=True
    task.options=options
    tools.import_asset_tasks([task])
    mesh=unreal.load_asset(asset)
    if not isinstance(mesh,unreal.StaticMesh):raise RuntimeError('Missing mesh: '+asset)
    meshes.add_simple_collisions(mesh,unreal.ScriptingCollisionShapeType.BOX)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    records.append({'source':str(source.relative_to(root)),'asset':asset,'bounds':str(mesh.get_bounds()),'provisional':'static mesh; locomotion and body motion driven by actor; no skeletal animation'})
(root/'docs/assets/TASK-048/import-results.json').write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8')
