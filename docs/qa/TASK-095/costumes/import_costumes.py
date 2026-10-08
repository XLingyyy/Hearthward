import unreal,json,traceback
from pathlib import Path
ROOT=Path(unreal.Paths.project_dir()).resolve()
src=ROOT.parent/'test_data/outputs/Hearthward/cloth-20261008/assets/3d_object/TASK-095-costumes'
out=ROOT/'.agent-local/qa/TASK-095/costumes';out.mkdir(parents=True,exist_ok=True)
dest='/Game/Hearthward/Assets/TASK-095/Costumes'
report=[];lib=unreal.MaterialEditingLibrary;assets=unreal.AssetToolsHelpers.get_asset_tools()
try:
 for role in ('Hero','Brother'):
  name='T_'+role+'_ClothMask';task=unreal.AssetImportTask()
  task.filename=str(src/(name+'.png'));task.destination_path=dest;task.destination_name=name
  task.automated=True;task.replace_existing=True;task.save=False
  assets.import_asset_tasks([task]);mask=unreal.load_asset(dest+'/'+name);assert mask
  mask.set_editor_property('srgb',False)
  mask.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_MASKS)
  assert unreal.EditorAssetLibrary.save_loaded_asset(mask,only_if_is_dirty=False)
 for role,source,tint in [('Hero','Hero',(.23,.15,.075)),('Brother','Brother',(.13,.18,.14)),('Civilian','Brother',(.28,.17,.13))]:
  name='M_'+role+'_CoarseCloth';assert not unreal.EditorAssetLibrary.does_asset_exist(dest+'/'+name),name
  original=unreal.load_asset('/Game/Characters/'+source+'/UE5/M_'+source)
  material=assets.duplicate_asset(name,dest,original);assert material
  existing=lib.get_material_property_input_node(material,unreal.MaterialProperty.MP_BASE_COLOR);assert existing
  output=lib.get_material_property_input_node_output_name(material,unreal.MaterialProperty.MP_BASE_COLOR)
  mask=lib.create_material_expression(material,unreal.MaterialExpressionTextureSample,-600,400)
  mask.texture=unreal.load_asset(dest+'/T_'+source+'_ClothMask');mask.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS
  color=lib.create_material_expression(material,unreal.MaterialExpressionConstant3Vector,-600,200)
  color.constant=unreal.LinearColor(*tint,1)
  blend=lib.create_material_expression(material,unreal.MaterialExpressionLinearInterpolate,-200,0)
  assert lib.connect_material_expressions(existing,output,blend,'A')
  assert lib.connect_material_expressions(color,'',blend,'B')
  assert lib.connect_material_expressions(mask,'R',blend,'Alpha')
  assert lib.connect_material_property(blend,'',unreal.MaterialProperty.MP_BASE_COLOR)
  lib.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
  lib.recompile_material(material)
  assert unreal.EditorAssetLibrary.save_loaded_asset(material,only_if_is_dirty=False)
  report.append({'role':role,'material':material.get_path_name(),'source_material':original.get_path_name(),
                 'base_color_source':existing.get_class().get_name(),'tint':tint,'preserved_normal_roughness':True})
except Exception:report.append({'error':traceback.format_exc()})
finally:(out/'import.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
