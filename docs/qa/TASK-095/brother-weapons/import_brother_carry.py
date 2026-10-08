import unreal,json,traceback
from pathlib import Path
out=Path('G:/GameFactory/Hearthward/.agent-local/qa/TASK-095/brother-weapons');out.mkdir(exist_ok=True)
root=Path('G:/GameFactory/test_data/outputs/Hearthward/brother-carry-20261008/assets')
dest='/Game/Hearthward/Assets/TASK-095/Weapons';report=[]
def import_asset(name,source,skeleton=None,mesh=False):
 task=unreal.AssetImportTask();task.filename=str(source/(name+'.fbx'));task.destination_path=dest;task.destination_name=name
 task.automated=True;task.replace_existing=True;task.save=True
 opt=unreal.FbxImportUI();opt.automated_import_should_detect_type=False
 opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH if mesh else unreal.FBXImportType.FBXIT_ANIMATION
 opt.original_import_type=opt.mesh_type_to_import
 opt.import_mesh=mesh;opt.import_as_skeletal=True;opt.import_animations=not mesh;opt.import_materials=False;opt.import_textures=False;opt.create_physics_asset=False
 if skeleton:opt.skeleton=skeleton
 if not mesh:
  opt.override_animation_name=name;opt.anim_sequence_import_data.set_editor_property('use_default_sample_rate',False);opt.anim_sequence_import_data.set_editor_property('custom_sample_rate',60)
 task.options=opt;unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);assert task.imported_object_paths,name
 asset=unreal.load_asset(dest+'/'+name);assert asset
 report.append({'name':name,'paths':list(task.imported_object_paths)})
 if not mesh:report[-1].update(seconds=asset.get_play_length(),skeleton=asset.get_editor_property('skeleton').get_path_name())
 return asset
try:
 skeleton=unreal.load_asset('/Game/Characters/Brother/Animation/A_Brother_Idle').get_editor_property('skeleton')
 import_asset('A_Brother_WeaponCarry',root/'motion/TASK-095-brother-carry',skeleton)
except Exception:report.append({'error':traceback.format_exc()})
finally:(out/'import.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
