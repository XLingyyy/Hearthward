import unreal,json,traceback
from pathlib import Path
out=Path('G:/GameFactory/Hearthward/.agent-local/qa/TASK-095/weapons')
report=[]
try:
 for row in json.loads((out/'source.json').read_text(encoding='utf-8')):
  label=row['piece'];dest='/Game/Hearthward/Assets/TASK-095/Weapons/'+label;name='SM_'+label+'_Practical'
  task=unreal.AssetImportTask();task.filename=row['fbx'];task.destination_path=dest;task.destination_name=name
  task.automated=True;task.replace_existing=True;task.save=False
  opt=unreal.FbxImportUI();opt.automated_import_should_detect_type=False
  opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;opt.original_import_type=opt.mesh_type_to_import
  opt.import_mesh=True;opt.import_as_skeletal=False;opt.import_animations=False;opt.import_materials=False;opt.import_textures=False
  opt.static_mesh_import_data.set_editor_property('combine_meshes',True)
  opt.static_mesh_import_data.set_editor_property('auto_generate_collision',False)
  opt.static_mesh_import_data.set_editor_property('vertex_color_import_option',unreal.VertexColorImportOption.REPLACE)
  task.options=opt;unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
  mesh=unreal.load_asset(dest+'/'+name);assert mesh,name
  existing=unreal.EditorAssetLibrary.does_asset_exist(dest+'/M_'+label+'_Practical')
  mat=unreal.load_asset(dest+'/M_'+label+'_Practical') if unreal.EditorAssetLibrary.does_asset_exist(dest+'/M_'+label+'_Practical') else unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_'+label+'_Practical',dest,unreal.Material,unreal.MaterialFactoryNew())
  assert mat
  lib=unreal.MaterialEditingLibrary
  if not existing:
   color=lib.create_material_expression(mat,unreal.MaterialExpressionVertexColor,-400,0)
   grade=lib.create_material_expression(mat,unreal.MaterialExpressionScalarParameter,-700,200);grade.set_editor_property('parameter_name','MetalFinish');grade.set_editor_property('default_value',1)
   metal=lib.create_material_expression(mat,unreal.MaterialExpressionMultiply,-100,200)
   assert lib.connect_material_expressions(color,'A',metal,'A');assert lib.connect_material_expressions(grade,'',metal,'B')
   assert lib.connect_material_property(metal,'',unreal.MaterialProperty.MP_METALLIC)
   stone=lib.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-700,-200);stone.set_editor_property('constant',unreal.LinearColor(.105,.112,.118,1))
   surface=lib.create_material_expression(mat,unreal.MaterialExpressionLinearInterpolate,-100,-200)
   assert lib.connect_material_expressions(stone,'',surface,'A');assert lib.connect_material_expressions(color,'',surface,'B');assert lib.connect_material_expressions(grade,'',surface,'Alpha')
   body=lib.create_material_expression(mat,unreal.MaterialExpressionLinearInterpolate,100,0)
   assert lib.connect_material_expressions(color,'',body,'A');assert lib.connect_material_expressions(surface,'',body,'B');assert lib.connect_material_expressions(color,'A',body,'Alpha')
   assert lib.connect_material_property(body,'',unreal.MaterialProperty.MP_BASE_COLOR)
   rough=lib.create_material_expression(mat,unreal.MaterialExpressionConstant,-400,200);rough.set_editor_property('r',.58)
   assert lib.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
  errors=lib.recompile_material(mat);assert not errors,str(errors)
  mesh.set_material(0,mat)
  assert unreal.EditorAssetLibrary.save_loaded_asset(mat)
  assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
  bounds=mesh.get_bounds();s=bounds.box_extent*2
  report.append(dict(piece=label,mesh=mesh.get_path_name(),dimensions_cm=[s.x,s.y,s.z],triangles=mesh.get_num_triangles(0),material=mat.get_path_name()))
except Exception:report.append({'error':traceback.format_exc()})
finally:(out/'import.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
