"""UE Python through UEClient: import the two reviewed CC0 seated loops on the existing skeleton."""
import json
import os
import traceback
from pathlib import Path
import unreal

out = Path(os.environ['HEARTHWARD_TASK109_QA'])
src = Path(os.environ['HEARTHWARD_TASK109_MOTION'])
dest = '/Game/Hearthward/Assets/TASK-109/CampLife'
report = {'ok': False, 'clips': []}
try:
    skeleton = unreal.load_asset('/Game/Characters/Brother/Animation/A_Brother_Idle').get_editor_property('skeleton')
    for kind in ['SeatedIdle', 'SeatedTalk']:
        name = 'A_Brother_' + kind
        task = unreal.AssetImportTask()
        task.filename = str(src / (name + '.fbx'))
        task.destination_path = dest
        task.destination_name = name
        task.automated = True
        task.replace_existing = False
        task.save = True
        opt = unreal.FbxImportUI()
        opt.automated_import_should_detect_type = False
        opt.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
        opt.original_import_type = opt.mesh_type_to_import
        opt.import_mesh = False
        opt.import_as_skeletal = True
        opt.import_animations = True
        opt.import_materials = False
        opt.import_textures = False
        opt.create_physics_asset = False
        opt.skeleton = skeleton
        opt.override_animation_name = name
        opt.anim_sequence_import_data.set_editor_property('use_default_sample_rate', False)
        opt.anim_sequence_import_data.set_editor_property('custom_sample_rate', 30)
        task.options = opt
        assert not unreal.EditorAssetLibrary.does_asset_exist(dest + '/' + name), 'Refuse overwrite; inspect/lock the existing asset first'
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        clip = unreal.load_asset(dest + '/' + name)
        assert clip and clip.get_play_length() > 1
        report['clips'].append({'name': name, 'paths': list(task.imported_object_paths),
            'seconds': clip.get_play_length(), 'skeleton': clip.get_editor_property('skeleton').get_path_name()})
    report['ok'] = True
except Exception:
    report['error'] = traceback.format_exc()
finally:
    (out / 'import-camp-life.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
