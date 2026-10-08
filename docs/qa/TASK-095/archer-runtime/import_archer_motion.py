import unreal,json,traceback
from pathlib import Path
out=Path('G:/GameFactory/Hearthward/.agent-local/qa/TASK-095/archer-runtime');out.mkdir(parents=True,exist_ok=True)
src=Path('G:/GameFactory/test_data/outputs/Hearthward/archer-motion-20261008/assets/motion/TASK-095-archer')
dest='/Game/Hearthward/Assets/TASK-095/Archer'
report=[]
try:
 for name in ['SK_Archer_Combat','A_Archer_Idle','A_Archer_Walk','A_Archer_Run','A_Archer_Shoot']:
  mesh=name.startswith('SK_')
  if mesh and unreal.EditorAssetLibrary.does_asset_exist(dest+'/'+name):continue
  task=unreal.AssetImportTask();task.filename=str(src/(name+'.fbx'));task.destination_path=dest;task.destination_name=name
  task.automated=True;task.replace_existing=True;task.save=True
  opt=unreal.FbxImportUI();opt.automated_import_should_detect_type=False
  opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH if mesh else unreal.FBXImportType.FBXIT_ANIMATION
  opt.original_import_type=opt.mesh_type_to_import;opt.import_mesh=mesh;opt.import_as_skeletal=mesh;opt.import_animations=not mesh
  opt.import_materials=False;opt.import_textures=False;opt.create_physics_asset=False
  if not mesh:
   opt.skeleton=unreal.load_asset(dest+'/SK_Archer_Combat_Skeleton');opt.override_animation_name=name
   opt.anim_sequence_import_data.set_editor_property('use_default_sample_rate',False)
   opt.anim_sequence_import_data.set_editor_property('custom_sample_rate',120 if name.endswith('Shoot') else 24)
  task.options=opt
  unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
  assert task.imported_object_paths,name
  report.append({'name':name,'paths':list(task.imported_object_paths)})
 asset=unreal.load_asset(dest+'/SK_Archer_Combat')
 mats={'tripo_mat_6406d71c':'/Game/Hearthward/Campaign/Guard/tripo_mat_6406d71c',
 'M_Archer_ShoulderRepairCloth':dest+'/M_Archer_ShoulderRepairCloth','M_Archer_Sling':dest+'/M_Archer_Sling',
 'M_Longbow_Practical':'/Game/Hearthward/Assets/TASK-095/Archery/Longbow/M_Longbow_Practical',
 'M_Quiver_Practical':'/Game/Hearthward/Assets/TASK-095/Archery/Quiver/M_Quiver_Practical',
 'M_Arrow_Practical':'/Game/Hearthward/Assets/TASK-095/Archery/Arrow/M_Arrow_Practical'}
 materials=asset.get_editor_property('materials')
 for index,m in enumerate(materials):
  key=str(m.material_slot_name);key=key.removesuffix('.001')
  assert key in mats,key
  material=unreal.load_asset(mats[key]);assert material,mats[key]
  if mats[key].startswith('/Game/Hearthward/Assets/TASK-095/'):
   unreal.MaterialEditingLibrary.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
   assert unreal.EditorAssetLibrary.save_loaded_asset(material,only_if_is_dirty=False),mats[key]
  m.set_editor_property('material_interface',material)
  materials[index]=m
 asset.set_editor_property('materials',materials)
 saved=unreal.EditorAssetLibrary.save_loaded_asset(asset,only_if_is_dirty=False)
 report.append({'materials':[{'slot':str(m.material_slot_name),'material':m.material_interface.get_path_name() if m.material_interface else None} for m in asset.get_editor_property('materials')],'saved':saved,'ok':True})
except Exception:report.append({'error':traceback.format_exc()})
finally:(out/'import.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
