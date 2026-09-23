"""Explicit plan/apply for the S1 grass material branch, preserving old nodes.

Native editor console entry. Configuration: Saved/Task026/ReworkV2/s1-grass.json.
The new mask is zero outside S1. No Landscape height, actors or layers are edited.
"""
from pathlib import Path
import hashlib
import json
import shutil
import stat
import time
import traceback
import unreal

ROOT=Path(unreal.Paths.project_dir())
LOCAL=ROOT/'Saved/Task026/ReworkV2'
ASSET='/Game/Hearthward/Assets/NaturalWorld/Rebuild'
MAT=ASSET+'/Materials/M_Landscape'
GT=ASSET+'/Foliage/GT_S1_Meadow'
TEXTURES={name:ASSET+'/Textures/T_'+name for name in ['S1_Blend','S1_GrassDensity']}
SOURCES=['art_source/TASK-026/Rebuild/ReworkV2/'+name+'.png' for name in TEXTURES]
SOURCES+=['scripts/world/TASK-026/rework_s1_grass.py','docs/world/TASK-026/rework-v2/height-authority.json']
ml=unreal.MaterialEditingLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
save=unreal.EditorLoadingAndSavingUtils
config=json.loads((LOCAL/'s1-grass.json').read_text(encoding='utf-8'))
OUT=ROOT/config['output']
OUT.mkdir(parents=True,exist_ok=False)


def digest(value):
    return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()


def file(package):
    assert package.startswith(ASSET+'/')
    return ROOT/('Content/'+package[6:]+'.uasset')


def nodes(mat):
    return {n.get_name():n for n in unreal.ObjectIterator(unreal.MaterialExpression) if n.get_outer()==mat}


def snapshot(mat):
    return {'nodes':sorted([n.get_name(),n.get_class().get_name()] for n in nodes(mat).values()),
            'grass_outputs':[[str(g.get_editor_property('name')),g.get_editor_property('grass_type').get_path_name()]
                for n in nodes(mat).values() if isinstance(n,unreal.MaterialExpressionLandscapeGrassOutput)
                for g in n.get_editor_property('grass_types')]}


def hashes():
    return {p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in SOURCES}


def expression(mat,kind,tag,**props):
    n=ml.create_material_expression(mat,getattr(unreal,'MaterialExpression'+kind))
    n.set_editor_property('desc','TASK026.S1.'+tag)
    if props:n.set_editor_properties(props)
    return n


def connect(a,b,pin,out=''):
    assert ml.connect_material_expressions(a,out,b,pin),pin


def main():
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    assert world.get_path_name().split('.')[0]=='/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'
    mat=unreal.load_asset(MAT)
    before=snapshot(mat)
    assert before['grass_outputs']==[['Meadow',ASSET+'/Foliage/GT_Meadow.GT_Meadow']], 'S1 branch exists or graph changed; create a new bounded edit plan'
    graph=nodes(mat)
    output=next(n for n in graph.values() if isinstance(n,unreal.MaterialExpressionLandscapeGrassOutput))
    inputs=ml.get_inputs_for_material_expression(mat,output)
    assert len(inputs)==1 and isinstance(inputs[0],unreal.MaterialExpressionTextureSample)
    density=inputs[0]
    assert density.texture.get_path_name()==ASSET+'/Textures/T_GrassDensity.T_GrassDensity'
    uv_inputs=[n for n in ml.get_inputs_for_material_expression(mat,density) if n]
    assert len(uv_inputs)==1
    mapuv=uv_inputs[0]
    adds=[GT,*TEXTURES.values()]
    for package in adds:assert not unreal.EditorAssetLibrary.does_asset_exist(package),package
    plan={'version':'S1-grass-1','scope_m':[-1230,-1000,-730,-500],
          'add_packages':adds,'update_packages':[MAT],'delete_packages':[],
          'before':before,'source_hashes':hashes(),
          'density':500,'xy_scale':[1.15,1.6],'z_scale':[.25,.45],
          'cull_cm':[8000,13000],'existing_density_node':density.get_name(),'map_uv_node':mapuv.get_name(),
          'height_changes':False,'actor_changes':False,'old_nodes_deleted':False}
    plan['plan_hash']=digest(plan)
    if config['action']=='plan':
        (OUT/'plan.json').write_text(json.dumps(plan,indent=2),encoding='utf-8')
        return
    assert config['action']=='apply'
    approved=json.loads((ROOT/config['plan']).read_text(encoding='utf-8'))
    assert plan==approved,'Plan inputs or material changed'
    lockfile=ROOT/'Saved/Task026/rework-v2-locks.json'
    assert time.time()-lockfile.stat().st_mtime<1800,'Refresh own remote locks'
    locks=json.loads(lockfile.read_text(encoding='utf-8'))
    if isinstance(locks,dict):locks=locks['locks']
    own={x['path'] for x in locks if x['owner']['name']=='XLingyyy'}
    packages=[MAT,*adds]
    dirty_before={p.get_path_name() for p in [*save.get_dirty_content_packages(),*save.get_dirty_map_packages()]}
    assert not dirty_before.intersection(packages),'Target package already dirty'
    backup=LOCAL/'backups'/plan['plan_hash']
    for package in packages:
        p=file(package); rel=p.relative_to(ROOT)
        assert rel.as_posix() in own,'Missing own lock: '+rel.as_posix()
        if p.exists():
            target=backup/rel;target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(p,target)
            p.chmod(p.stat().st_mode|stat.S_IWRITE)
    imported={}
    for name,target in TEXTURES.items():
        task=unreal.AssetImportTask()
        task.set_editor_properties(dict(filename=str(ROOT/'art_source/TASK-026/Rebuild/ReworkV2'/(name+'.png')),
            destination_path=ASSET+'/Textures',destination_name=target.rsplit('/',1)[1],
            automated=True,save=False,replace_existing=False))
        tools.import_asset_tasks([task])
        tex=unreal.load_asset(target);assert isinstance(tex,unreal.Texture2D)
        tex.set_editor_properties(dict(srgb=False,compression_settings=unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP,
            max_texture_size=0,mip_gen_settings=unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS))
        imported[name]=tex
    gt=tools.create_asset('GT_S1_Meadow',ASSET+'/Foliage',unreal.LandscapeGrassType,unreal.LandscapeGrassTypeFactory())
    v=unreal.GrassVariety()
    v.set_editor_properties(dict(grass_mesh=unreal.load_asset(ASSET+'/Meshes/GrassCards/StaticMeshes/SM_GrassCards'),
        grass_density=unreal.PerPlatformFloat(default=plan['density']),scaling=unreal.GrassScaling.FREE,
        scale_x=unreal.FloatInterval(min=plan['xy_scale'][0],max=plan['xy_scale'][1]),
        scale_y=unreal.FloatInterval(min=plan['xy_scale'][0],max=plan['xy_scale'][1]),
        scale_z=unreal.FloatInterval(min=plan['z_scale'][0],max=plan['z_scale'][1]),
        start_cull_distance=unreal.PerPlatformInt(default=plan['cull_cm'][0]),
        end_cull_distance=unreal.PerPlatformInt(default=plan['cull_cm'][1]),
        random_rotation=True,align_to_surface=True,cast_dynamic_shadow=False))
    gt.set_editor_property('grass_varieties',[v])
    mat.modify()
    samples={}
    for name,tex in imported.items():
        n=expression(mat,'TextureSample',name,texture=tex,sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        connect(mapuv,n,'UVs');samples[name]=n
    inverse=expression(mat,'OneMinus','OutsideSample');connect(samples['S1_Blend'],inverse,'','R')
    masked=expression(mat,'Multiply','OriginalMeadowOutside');connect(density,masked,'A','R');connect(inverse,masked,'B')
    outputs=list(output.get_editor_property('grass_types'))
    entry=unreal.GrassInput();entry.set_editor_properties(dict(name='S1_Meadow',grass_type=gt))
    outputs.append(entry);output.set_editor_property('grass_types',outputs)
    connect(masked,output,'Meadow');connect(samples['S1_GrassDensity'],output,'S1_Meadow','R')
    ml.recompile_material(mat)
    after=snapshot(mat)
    assert set(map(tuple,before['nodes'])).issubset(set(map(tuple,after['nodes']))),'Existing expression removed'
    dirty={p.get_path_name() for p in [*save.get_dirty_content_packages(),*save.get_dirty_map_packages()]}
    unexpected=dirty-dirty_before-set(packages)
    assert not unexpected,'Unplanned dirty packages: '+str(sorted(unexpected))
    for package in packages:assert unreal.EditorAssetLibrary.save_asset(package,only_if_is_dirty=True),package
    (OUT/'apply.json').write_text(json.dumps({'status':'SAVED_PENDING_VISUAL_REVIEW','plan_hash':plan['plan_hash'],
        'saved_packages':packages,'before':before,'after':after,'backup':str(backup)},indent=2),encoding='utf-8')


try:main()
except Exception:
    (OUT/'failure.json').write_text(json.dumps({'error':traceback.format_exc()},indent=2),encoding='utf-8')
    raise
