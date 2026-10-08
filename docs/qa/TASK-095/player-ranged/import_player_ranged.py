import unreal,json,traceback
from pathlib import Path
out=Path('G:/GameFactory/Hearthward/.agent-local/qa/TASK-095/player-ranged');out.mkdir(exist_ok=True)
root=Path('G:/GameFactory/test_data/outputs/Hearthward/ranged-20261008/assets')
dest='/Game/Hearthward/Assets/TASK-095/PlayerRanged';report=[]
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
 hero=unreal.load_asset('/Game/Characters/Hero/AnimationV2/A_Hero_Idle').get_editor_property('skeleton')
 for kind in ['BowDraw','BowRelease','CrossbowAim','CrossbowReload']:import_asset('A_Hero_'+kind,root/'motion/TASK-095-player-ranged',hero)
 bow=import_asset('SK_PlayerBow',root/'3d_object/TASK-095-player-bow',mesh=True)
 materials=bow.get_editor_property('materials')
 report[-1]['material_before']=[m.material_interface.get_path_name() if m.material_interface else None for m in materials]
 material=unreal.load_asset('/Game/Hearthward/Assets/TASK-095/Archery/Longbow/M_Longbow_Practical')
 report[-1]['skeletal_usage_before']=material.get_editor_property('used_with_skeletal_mesh')
 if not material.get_editor_property('used_with_skeletal_mesh'):
  material.set_editor_property('used_with_skeletal_mesh',True);unreal.MaterialEditingLibrary.recompile_material(material);unreal.EditorAssetLibrary.save_loaded_asset(material,only_if_is_dirty=False)
 slot=materials[0];slot.set_editor_property('material_interface',material);materials[0]=slot;bow.set_editor_property('materials',materials)
 assert bow.get_editor_property('materials')[0].material_interface==material
 report[-1]['material_after']=bow.get_editor_property('materials')[0].material_interface.get_path_name()
 unreal.EditorAssetLibrary.save_loaded_asset(bow,only_if_is_dirty=False)
 import_asset('A_PlayerBow_Draw',root/'3d_object/TASK-095-player-bow',bow.get_editor_property('skeleton'))
except Exception:report.append({'error':traceback.format_exc()})
finally:(out/'import.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
