import unreal,json,traceback
from pathlib import Path
out=Path('G:/GameFactory/Hearthward/.agent-local/qa/TASK-096/nearfield');out.mkdir(parents=True,exist_ok=True)
source=Path('G:/GameFactory/test_data/outputs/Hearthward/nearfield-20261008/assets/3d_object/TASK-096-nearfield')
dest='/Game/Hearthward/Assets/TASK-096/Nearfield';report={'passed':False,'assets':[]}
lib=unreal.MaterialEditingLibrary
def expression(mat,cls):return lib.create_material_expression(mat,cls)
def connect(a,pin,b,target):assert lib.connect_material_expressions(a,pin,b,target),(a.get_name(),pin,b.get_name(),target)
def task(name,extension,options=None):
 t=unreal.AssetImportTask();t.filename=str(source/(name+extension));t.destination_path=dest;t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True
 if options:t.options=options
 unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t]);assert t.imported_object_paths,name
 a=unreal.load_asset(dest+'/'+name);assert a,name;report['assets'].append({'name':name,'paths':list(t.imported_object_paths)});return a
try:
 for label in ['RoughStone','OldTimber']:
  diffuse=task('T_'+label+'_D','.png');normal=task('T_'+label+'_N','.png')
  normal.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP);normal.set_editor_property('srgb',False);normal.set_editor_property('flip_green_channel',True)
  unreal.EditorAssetLibrary.save_loaded_asset(normal,only_if_is_dirty=False)
  name='M_'+label;mat=unreal.load_asset(dest+'/'+name)
  if mat is None:mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,dest,unreal.Material,unreal.MaterialFactoryNew())
  assert lib.get_num_material_expressions(mat)==0,'Do not replace an existing graph implicitly'
  mat.set_editor_property('tangent_space_normal',False)
  scale=expression(mat,unreal.MaterialExpressionConstant3Vector);scale.constant=unreal.LinearColor(80,80,80)
  for texture,function,prop in [(diffuse,'WorldAlignedTexture',unreal.MaterialProperty.MP_BASE_COLOR),(normal,'WorldAlignedNormal',unreal.MaterialProperty.MP_NORMAL)]:
   obj=expression(mat,unreal.MaterialExpressionTextureObject);obj.texture=texture
   if function=='WorldAlignedNormal':obj.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
   fn=expression(mat,unreal.MaterialExpressionMaterialFunctionCall);fn.set_material_function(unreal.load_asset('/Engine/Functions/Engine_MaterialFunctions01/Texturing/'+function))
   inputs=list(lib.get_material_expression_input_names(fn));outputs=list(lib.get_material_expression_output_names(fn));report.setdefault('functions',[]).append({'function':function,'inputs':inputs,'outputs':outputs})
   texturepin=next(n for n in inputs if n.startswith('TextureObject'));sizepin=next(n for n in inputs if n.startswith('TextureSize'))
   connect(obj,'',fn,texturepin);connect(scale,'',fn,sizepin)
   if function=='WorldAlignedNormal':
    world=expression(mat,unreal.MaterialExpressionStaticBool);world.set_editor_property('value',True);connect(world,'',fn,'WorldSpace')
   output=next(n for n in outputs if n.startswith('XYZ'))
   assert lib.connect_material_property(fn,output,prop)
  rough=expression(mat,unreal.MaterialExpressionConstant);rough.r=.91;assert lib.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
  lib.recompile_material(mat);lib.layout_material_expressions(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
  report['assets'].append({'name':name,'textures':[t.get_path_name() for t in lib.get_material_used_textures(mat)]})
 opt=unreal.FbxImportUI();opt.automated_import_should_detect_type=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;opt.import_materials=False;opt.import_textures=False;opt.import_animations=False
 opt.static_mesh_import_data.set_editor_property('combine_meshes',True);opt.static_mesh_import_data.set_editor_property('auto_generate_collision',False)
 mesh=task('SM_BedroomDoorframe','.fbx',opt);mesh.set_material(0,unreal.load_asset(dest+'/M_RoughStone'));unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False)
 report['bounds_cm']=str(mesh.get_bounds());report['passed']=True
except Exception:report['error']=traceback.format_exc()
finally:(out/'import.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
