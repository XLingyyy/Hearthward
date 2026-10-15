"""UE Python animation-only import; native MCP currently exposes mesh import only."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir())
report=[]
for role in ['Hero','Brother']:
    skeleton=unreal.load_asset(f'/Game/Characters/{role}/UE5/SK_{role}_Skeleton')
    assert skeleton
    source=json.loads((root/f'art_source/TASK-126/{role}-contact.json').read_text(encoding='utf-8'))
    for clip in source['clips']:
        name=clip['name'];path='/Game/Hearthward/Assets/TASK-126/'+name
        options=unreal.FbxImportUI()
        options.automated_import_should_detect_type=False
        options.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION
        options.original_import_type=unreal.FBXImportType.FBXIT_ANIMATION
        options.import_mesh=False;options.import_as_skeletal=False;options.import_animations=True
        options.import_materials=False;options.import_textures=False;options.skeleton=skeleton
        options.override_animation_name=name
        options.anim_sequence_import_data.set_editor_property('custom_sample_rate',30)
        options.anim_sequence_import_data.set_editor_property('use_default_sample_rate',False)
        task=unreal.AssetImportTask();task.filename=str(root/f'art_source/TASK-126/{name}.fbx')
        task.destination_path='/Game/Hearthward/Assets/TASK-126';task.destination_name=name
        task.options=options;task.factory=unreal.FbxFactory();task.automated=True;task.save=False;task.replace_existing=True
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        animation=unreal.load_asset(path);assert isinstance(animation,unreal.AnimSequence),(name,list(task.imported_object_paths))
        assert animation.get_editor_property('skeleton')==skeleton
        length=animation.get_play_length();assert abs(length-clip['seconds'])<.04,(name,length,clip['seconds'])
        unreal.EditorAssetLibrary.save_loaded_asset(animation)
        report.append({'asset':path,'skeleton':skeleton.get_path_name(),'seconds':length,'ok':True})
        (root/'.agent-local/qa/TASK-126/import.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('CONTACT_IMPORT',len(report))
