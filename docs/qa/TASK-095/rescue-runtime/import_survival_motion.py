import unreal,json,traceback
from pathlib import Path
out=Path('G:/GameFactory/Hearthward/.agent-local/qa/TASK-095/rescue-runtime')
src=Path('G:/GameFactory/test_data/outputs/Hearthward/survival-motion-20261008/assets/motion/TASK-095-survival')
dest='/Game/Hearthward/Assets/TASK-095/Survival';report=[]
try:
 for role in ['Hero','Brother']:
  old=unreal.load_asset('/Game/Characters/'+role+('/AnimationV2/' if role=='Hero' else '/Animation/')+'A_'+role+'_Idle')
  skeleton=old.get_editor_property('skeleton')
  for kind in ['Down','GetUp','Rescue']:
   name='A_'+role+'_'+kind
   task=unreal.AssetImportTask();task.filename=str(src/(name+'.fbx'));task.destination_path=dest;task.destination_name=name
   task.automated=True;task.replace_existing=True;task.save=True
   opt=unreal.FbxImportUI();opt.automated_import_should_detect_type=False
   opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION;opt.original_import_type=opt.mesh_type_to_import
   opt.import_mesh=False;opt.import_as_skeletal=True;opt.import_animations=True;opt.import_materials=False;opt.import_textures=False;opt.create_physics_asset=False
   opt.skeleton=skeleton;opt.override_animation_name=name
   opt.anim_sequence_import_data.set_editor_property('use_default_sample_rate',False)
   opt.anim_sequence_import_data.set_editor_property('custom_sample_rate',30)
   task.options=opt;unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
   assert task.imported_object_paths,name
   clip=unreal.load_asset(dest+'/'+name);assert clip
   report.append({'name':name,'paths':list(task.imported_object_paths),'seconds':clip.get_play_length(),'skeleton':clip.get_editor_property('skeleton').get_path_name()})
except Exception:report.append({'error':traceback.format_exc()})
finally:(out/'import.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
