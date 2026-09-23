"""Bounded material overlay; preserve every old expression and outside-S1 output."""
from pathlib import Path
import json
import hashlib
import shutil
import stat
import time
import traceback
import unreal

root=Path(unreal.Paths.project_dir())
local=root/'Saved/Task026/ReworkV2'
config=json.loads((local/'s1-surface.json').read_text(encoding='utf-8'))
out=root/config['output'];out.mkdir(parents=True,exist_ok=False)
asset='/Game/Hearthward/Assets/NaturalWorld/Rebuild'
package=asset+'/Materials/M_Landscape'
texture_package=asset+'/Textures/T_S1_Surface'
ml=unreal.MaterialEditingLibrary
save=unreal.EditorLoadingAndSavingUtils

def nodes(mat):
    return {n.get_name():n for n in unreal.ObjectIterator(unreal.MaterialExpression) if n.get_outer()==mat}

def file(p):return root/('Content/'+p[6:]+'.uasset')

def connect(a,b,pin,output=''):
    assert ml.connect_material_expressions(a,output,b,pin),pin

def expr(mat,kind,tag,**properties):
    n=ml.create_material_expression(mat,getattr(unreal,'MaterialExpression'+kind))
    n.set_editor_property('desc','TASK026.S1.Surface.'+tag)
    if properties:n.set_editor_properties(properties)
    return n

def scalar(mat,name,value):
    return expr(mat,'ScalarParameter',name,parameter_name='S1_'+name,default_value=value)

def main():
    assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
    mat=unreal.load_asset(package);graph=nodes(mat)
    assert not any(n.get_editor_property('desc').startswith('TASK026.S1.Surface.') for n in graph.values())
    blend=next(n for n in graph.values() if isinstance(n,unreal.MaterialExpressionTextureSample) and n.texture.get_path_name().endswith('/T_S1_Blend.T_S1_Blend'))
    mapuv=next(n for n in ml.get_inputs_for_material_expression(mat,blend) if n)
    soil=next(n for n in graph.values() if isinstance(n,unreal.MaterialExpressionTextureSample) and n.texture.get_path_name().endswith('/T_Soil_D.T_Soil_D'))
    soilnormal=next(n for n in graph.values() if isinstance(n,unreal.MaterialExpressionTextureSample) and n.texture.get_path_name().endswith('/T_Soil_N.T_Soil_N'))
    properties={'color':unreal.MaterialProperty.MP_BASE_COLOR,'normal':unreal.MaterialProperty.MP_NORMAL,'roughness':unreal.MaterialProperty.MP_ROUGHNESS}
    inputs={k:ml.get_material_property_input_node(mat,p) for k,p in properties.items()}
    pins={k:ml.get_material_property_input_node_output_name(mat,p) for k,p in properties.items()}
    sources=['scripts/world/TASK-026/rework_s1_surface.py','art_source/TASK-026/Rebuild/ReworkV2/S1_Surface.png','art_source/TASK-026/Rebuild/ReworkV2/s1-surface.json']
    plan={'update_packages':[package],'add_packages':[texture_package],'delete_packages':[],
        'before_nodes':sorted(graph),'before_inputs':{k:[n.get_name(),pins[k]] for k,n in inputs.items()},
        'source_hashes':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in sources},
        'scope':'S1_Blend only; no landscape height or actor edits','old_nodes_deleted':False,
        'parameters':{'SoilStrength':1.0,'MacroContrast':.12,'ForestDesaturation':.25,'WetDarkening':.32,'WetRoughness':.58}}
    plan['plan_hash']=hashlib.sha256(json.dumps(plan,sort_keys=True).encode()).hexdigest()
    if config['action']=='plan':
        (out/'plan.json').write_text(json.dumps(plan,indent=2),encoding='utf-8');return
    assert config['action']=='apply'
    assert plan==json.loads((root/config['plan']).read_text(encoding='utf-8'))
    assert not unreal.EditorAssetLibrary.does_asset_exist(texture_package)
    assert not save.get_dirty_content_packages() and not save.get_dirty_map_packages()
    lockfile=root/'Saved/Task026/rework-v2-locks.json'
    assert time.time()-lockfile.stat().st_mtime<1800
    locks=json.loads(lockfile.read_text(encoding='utf-8'));locks=locks if isinstance(locks,list) else locks['locks']
    owned={x['path'] for x in locks if x['owner']['name']=='XLingyyy'}
    for p in [package,texture_package]:assert file(p).relative_to(root).as_posix() in owned,p
    backup=local/'backups'/plan['plan_hash']/'M_Landscape.uasset';backup.parent.mkdir(parents=True,exist_ok=True)
    assert not backup.exists();shutil.copy2(file(package),backup)
    file(package).chmod(file(package).stat().st_mode|stat.S_IWRITE)
    task=unreal.AssetImportTask()
    task.set_editor_properties(dict(filename=str(root/sources[1]),destination_path=asset+'/Textures',destination_name='T_S1_Surface',automated=True,save=False,replace_existing=False))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture=unreal.load_asset(texture_package)
    texture.set_editor_properties(dict(srgb=False,compression_settings=unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP,
        max_texture_size=0,mip_gen_settings=unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS))
    mat.modify()
    mask=expr(mat,'TextureSample','Masks',texture=texture,sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    connect(mapuv,mask,'UVs')
    params={k:scalar(mat,k,v) for k,v in plan['parameters'].items()}
    custom_inputs=[]
    for name in ['Original','Soil','Mask',*params]:
        i=unreal.CustomInput();i.set_editor_property('input_name',name);custom_inputs.append(i)
    color=expr(mat,'Custom','Color',output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3,inputs=custom_inputs,
        code='float3 c=lerp(Original,Soil,saturate(Mask.r*SoilStrength)); float grey=dot(c,float3(.299,.587,.114)); c=lerp(c,grey.xxx,Mask.b*ForestDesaturation); c*=1+(Mask.a*2-1)*MacroContrast; return c*(1-Mask.g*WetDarkening);')
    connect(inputs['color'],color,'Original',pins['color']);connect(soil,color,'Soil','RGB');connect(mask,color,'Mask','RGBA')
    for name,n in params.items():connect(n,color,name)
    normal=expr(mat,'LinearInterpolate','SoilNormal');connect(inputs['normal'],normal,'A',pins['normal']);connect(soilnormal,normal,'B','RGB');connect(mask,normal,'Alpha','R')
    rough=expr(mat,'LinearInterpolate','WetRoughness');connect(inputs['roughness'],rough,'A',pins['roughness']);connect(params['WetRoughness'],rough,'B');connect(mask,rough,'Alpha','G')
    for key,node in [('color',color),('normal',normal),('roughness',rough)]:
        final=expr(mat,'LinearInterpolate','Bounded'+key)
        connect(inputs[key],final,'A',pins[key]);connect(node,final,'B');connect(blend,final,'Alpha','R')
        assert ml.connect_material_property(final,'',properties[key])
    ml.recompile_material(mat)
    assert set(graph).issubset(nodes(mat))
    dirty={p.get_path_name() for p in [*save.get_dirty_content_packages(),*save.get_dirty_map_packages()]}
    assert dirty=={package,texture_package},str(dirty)
    assert unreal.EditorAssetLibrary.save_asset(texture_package,only_if_is_dirty=True)
    assert unreal.EditorAssetLibrary.save_asset(package,only_if_is_dirty=True)
    (out/'apply.json').write_text(json.dumps({'status':'SAVED_PENDING_VISUAL_REVIEW','plan':plan,'nodes_after':sorted(nodes(mat))},indent=2),encoding='utf-8')

try:main()
except Exception:
    (out/'failure.json').write_text(json.dumps({'error':traceback.format_exc()}),encoding='utf-8')
    raise
