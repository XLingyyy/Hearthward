import unreal,json,traceback
from pathlib import Path
out=Path('G:/GameFactory/Hearthward/.agent-local/qa/TASK-098/camp-set')
dest='/Game/Hearthward/Assets/TASK-098/CampSet'
lib=unreal.MaterialEditingLibrary
report={'passed':False,'materials':[],'meshes':[]}
def expr(mat,cls):return lib.create_material_expression(mat,cls)
def connect(a,pin,b,target):assert lib.connect_material_expressions(a,pin,b,target)
try:
 for material,label,size in [('M_CampStone','RoughStone',45),('M_CampWood','OldTimber',70)]:
  mat=unreal.load_asset(dest+'/'+material);assert mat
  if isinstance(mat,unreal.MaterialInstanceConstant):
   parent=unreal.load_asset('/Game/Hearthward/Assets/TASK-096/Nearfield/M_'+label)
   lib.set_material_instance_parent(mat,parent)
   lib.update_material_instance(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat,False)
   report['materials'].append({'material':material,'parent':parent.get_path_name()})
   continue
  lib.delete_all_material_expressions(mat)
  mat.set_editor_property('tangent_space_normal',False)
  scale=expr(mat,unreal.MaterialExpressionConstant3Vector);scale.constant=unreal.LinearColor(size,size,size)
  for suffix,function,prop in [('D','WorldAlignedTexture',unreal.MaterialProperty.MP_BASE_COLOR),('N','WorldAlignedNormal',unreal.MaterialProperty.MP_NORMAL)]:
   obj=expr(mat,unreal.MaterialExpressionTextureObject);obj.texture=unreal.load_asset('/Game/Hearthward/Assets/TASK-096/Nearfield/T_'+label+'_'+suffix);assert obj.texture
   if suffix=='N':obj.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
   fn=expr(mat,unreal.MaterialExpressionMaterialFunctionCall);fn.set_material_function(unreal.load_asset('/Engine/Functions/Engine_MaterialFunctions01/Texturing/'+function))
   inputs=list(lib.get_material_expression_input_names(fn));outputs=list(lib.get_material_expression_output_names(fn))
   connect(obj,'',fn,next(n for n in inputs if n.startswith('TextureObject')))
   connect(scale,'',fn,next(n for n in inputs if n.startswith('TextureSize')))
   if suffix=='N':
    world=expr(mat,unreal.MaterialExpressionStaticBool);world.set_editor_property('value',True);connect(world,'',fn,'WorldSpace')
   assert lib.connect_material_property(fn,next(n for n in outputs if n.startswith('XYZ')),prop)
  rough=expr(mat,unreal.MaterialExpressionConstant);rough.r=.91;lib.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
  lib.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat,False)
  report['materials'].append({'material':material,'textures':[t.get_path_name() for t in lib.get_material_used_textures(mat)]})
 for name,color,roughness,metal in [('Iron',(.055,.06,.062),.72,.72),('Linen',(.55,.49,.36),.96,0),('Rope',(.29,.22,.12),.94,0),('Charcoal',(.025,.02,.018),.97,0),('Clay',(.25,.10,.046),.85,0),('Blanket',(.13,.19,.18),.95,0)]:
  mat=unreal.load_asset(dest+'/M_Camp'+name);assert mat
  if isinstance(mat,unreal.MaterialInstanceConstant):
   params=[str(p) for p in lib.get_scalar_parameter_names(mat)]
   for p in params:
    if 'roughness' in p.lower():lib.set_material_instance_scalar_parameter_value(mat,p,roughness)
    if 'metallic' in p.lower():lib.set_material_instance_scalar_parameter_value(mat,p,metal)
   lib.update_material_instance(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat,False)
   report['materials'].append({'material':name,'scalar_parameters':params})
   continue
  lib.delete_all_material_expressions(mat)
  base=expr(mat,unreal.MaterialExpressionConstant3Vector);base.constant=unreal.LinearColor(*color)
  lib.connect_material_property(base,'',unreal.MaterialProperty.MP_BASE_COLOR)
  for val,prop in [(roughness,unreal.MaterialProperty.MP_ROUGHNESS),(metal,unreal.MaterialProperty.MP_METALLIC)]:
   n=expr(mat,unreal.MaterialExpressionConstant);n.r=val;lib.connect_material_property(n,'',prop)
  lib.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat,False)
 for label in ['Campfire','RopeBed','Smelter','Cooking','MedicalBed']:
  mesh=unreal.load_asset(dest+'/SM_'+label);assert mesh
  mats=[str(s.material_interface.get_path_name()) for s in mesh.static_materials]
  b=mesh.get_bounds()
  report['meshes'].append({'mesh':label,'materials':mats,'bounds_cm':str(b)})
 report['passed']=True
except Exception:report['error']=traceback.format_exc()
(out/'materials.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
