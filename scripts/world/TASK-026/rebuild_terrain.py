"""Native editor authoring. Execute with UE's Python menu/console.

Creates a separate map; never edits the previous developer's locked assets.
"""
from pathlib import Path
import json
import traceback
import unreal

ROOT=Path(unreal.Paths.project_dir())
SRC=ROOT/'art_source/TASK-026/Rebuild'
OUT=ROOT/'Saved/Task026/Rebuild'
OUT.mkdir(parents=True,exist_ok=True)
MAP='/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'
ASSET='/Game/Hearthward/Assets/NaturalWorld/Rebuild'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
tools=unreal.AssetToolsHelpers.get_asset_tools()
ml=unreal.MaterialEditingLibrary


def texture(path,name,kind='color',replace=False):
    target=ASSET+'/Textures/'+name
    asset=unreal.load_asset(target)
    if asset and not replace: return asset
    task=unreal.AssetImportTask()
    task.set_editor_properties(dict(filename=str(path),destination_path=ASSET+'/Textures',destination_name=name,automated=True,save=True,replace_existing=replace))
    tools.import_asset_tasks([task])
    asset=unreal.load_asset(target)
    assert asset, str(path)
    asset.set_editor_property('max_texture_size',2048)
    if kind!='color': asset.set_editor_property('srgb',False)
    if kind=='normal':
        asset.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
        asset.set_editor_property('flip_green_channel',True)
    elif kind=='data': asset.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


def expr(mat,cls,**props):
    node=ml.create_material_expression(mat,getattr(unreal,'MaterialExpression'+cls))
    if props: node.set_editor_properties(props)
    return node


def connect(a,b,pin,out=''):
    assert ml.connect_material_expressions(a,out,b,pin), (str(a),str(b),pin)


def constant(mat,value): return expr(mat,'Constant',r=float(value))


def struct(cls,**props):
    value=cls();value.set_editor_properties(props);return value


def sample(mat,tex,uv=None,normal=False):
    node=expr(mat,'TextureSample',texture=tex)
    if normal: node.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    elif not tex.get_editor_property('srgb'): node.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    if uv: connect(uv,node,'UVs')
    return node


def finish(mat):
    mat.set_editor_properties(dict(used_with_nanite=True,used_with_instanced_static_meshes=True))
    ml.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


def ground_material():
    target=ASSET+'/Materials/M_Landscape'
    mat=unreal.load_asset(target)
    if mat:
        return mat
    else:
        mat=tools.create_asset('M_Landscape',ASSET+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    pos=expr(mat,'WorldPosition')
    xy=expr(mat,'ComponentMask',r=True,g=True,b=False,a=False); connect(pos,xy,'')
    uv=expr(mat,'Divide',const_b=500.0); connect(xy,uv,'A')
    rockuv=expr(mat,'Divide',const_b=420.0); connect(xy,rockuv,'A')
    offset=expr(mat,'Add',const_b=201600.0); connect(xy,offset,'A')
    mapuv=expr(mat,'Divide',const_b=403200.0); connect(offset,mapuv,'A')
    weights=sample(mat,texture(SRC/'biome_weights.png','T_Biomes','data',True),mapuv)
    samples=[]; normals=[]
    for title,folder,prefix,texuv in [('Grass','草地','grass_ground',uv),('Soil','土','dirt',uv),('Rock','岩面','rocky_terrain',rockuv)]:
        base=ROOT/'art_source/TASK-004/polyhaven/地表'/folder/'textures'
        if title=='Rock':
            base=SRC/'polyhaven/rock_3'
            tex=expr(mat,'TextureObject',texture=texture(base/'rock_3_diff_2k.jpg','T_Cliff_D'))
            normal_ws=expr(mat,'VertexNormalWS')
            diffuse=expr(mat,'Custom',output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                inputs=[struct(unreal.CustomInput,input_name=n) for n in ['Rock','Position','Normal']],
                code='float3 w=pow(abs(Normal),4); w/=max(w.x+w.y+w.z,0.001); float3 c=0; const float scales[3]={420,3700,13700}; const float weights[3]={0.45,0.35,0.20}; [unroll] for(int i=0;i<3;i++){float3 p=Position/scales[i]+float3(i*.37,i*.61,i*.23); c+=(Texture2DSample(Rock,RockSampler,p.yz).rgb*w.x+Texture2DSample(Rock,RockSampler,p.xz).rgb*w.y+Texture2DSample(Rock,RockSampler,p.xy).rgb*w.z)*weights[i];} float grey=dot(c,float3(0.299,0.587,0.114)); return lerp(c,grey.xxx,0.85)*float3(0.78,0.82,0.84);')
            connect(tex,diffuse,'Rock');connect(pos,diffuse,'Position');connect(normal_ws,diffuse,'Normal')
        else:diffuse=sample(mat,texture(base/(prefix+'_diff_4k.jpg'),'T_'+title+'_D'),texuv)
        if title=='Grass':
            tint=expr(mat,'Constant3Vector',constant=unreal.LinearColor(.48,.73,.30,1))
            colored=expr(mat,'Multiply');connect(diffuse,colored,'A','RGB');connect(tint,colored,'B')
            samples.append(colored)
        else:samples.append(diffuse)
        if title=='Rock':normals.append(expr(mat,'Constant3Vector',constant=unreal.LinearColor(0,0,1,1)))
        else:
            normal=texture(base/(prefix+'_nor_gl_4k.exr'),'T_'+title+'_N','normal')
            normals.append(sample(mat,normal,texuv,True))
    for nodes,prop in [(samples,unreal.MaterialProperty.MP_BASE_COLOR),(normals,unreal.MaterialProperty.MP_NORMAL)]:
        lerp1=expr(mat,'LinearInterpolate'); connect(nodes[0],lerp1,'A'); connect(nodes[1],lerp1,'B','RGB'); connect(weights,lerp1,'Alpha','G')
        lerp2=expr(mat,'LinearInterpolate'); connect(lerp1,lerp2,'A'); connect(nodes[2],lerp2,'B'); connect(weights,lerp2,'Alpha','B')
        ml.connect_material_property(lerp2,'',prop)
    grass_path=ASSET+'/Foliage/GT_Meadow'
    grass=unreal.load_asset(grass_path)
    if not grass:
        grass=tools.create_asset('GT_Meadow',ASSET+'/Foliage',unreal.LandscapeGrassType,unreal.LandscapeGrassTypeFactory())
    mesh=unreal.load_asset(ASSET+'/Meshes/GrassCards/StaticMeshes/SM_GrassCards')
    if mesh and (SRC/'grass_density.png').exists():
        variety=unreal.GrassVariety()
        variety.set_editor_properties(dict(grass_mesh=mesh,grass_density=unreal.PerPlatformFloat(default=35),
            start_cull_distance=unreal.PerPlatformInt(default=5500),end_cull_distance=unreal.PerPlatformInt(default=9000),
            scale_x=unreal.FloatInterval(min=.35,max=.70),scale_y=unreal.FloatInterval(min=.35,max=.70),
            scale_z=unreal.FloatInterval(min=.35,max=.70),random_rotation=True,align_to_surface=True,
            cast_dynamic_shadow=False))
        grass.set_editor_property('grass_varieties',[variety]);unreal.EditorAssetLibrary.save_loaded_asset(grass)
        output=expr(mat,'LandscapeGrassOutput',grass_types=[struct(unreal.GrassInput,name='Meadow',grass_type=grass)])
        density=sample(mat,texture(SRC/'grass_density.png','T_GrassDensity','data',True),mapuv)
        connect(density,output,'Meadow','R')
    ml.connect_material_property(constant(mat,.92),'',unreal.MaterialProperty.MP_ROUGHNESS)
    return finish(mat)



def main():
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        raise RuntimeError('Initial terrain creation refused: map exists. Use bounded ReworkV2 updates.')
    try:
        if not unreal.EditorAssetLibrary.does_asset_exist(MAP):
            assert levels.new_level_from_template(MAP,'/Engine/Maps/Templates/OpenWorld')
            for actor in actors.get_all_level_actors():
                if isinstance(actor,unreal.LandscapeProxy):
                    location=actor.get_actor_location()
                    actor.set_actor_location(unreal.Vector(location.x*2,location.y*2,0),False,False)
                    actor.set_actor_scale3d(unreal.Vector(200,200,400))
            assert levels.save_current_level()
        elif editor.get_editor_world().get_path_name().split('.')[0] != MAP:
            assert levels.load_level(MAP)
        descs=unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
        unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in descs])
        landscapes=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.LandscapeProxy)]
        land=next(a for a in landscapes if isinstance(a,unreal.Landscape))
        # Keep the imported absolute heightfield free of inherited sculpt layers.
        for layer in land.get_edit_layers_bp()[1:]:layer.set_editor_property('visible',False)
        # OpenWorld's prebuilt flat-terrain HLODs no longer describe this heightfield.
        stale_hlods=[a for a in actors.get_all_level_actors() if a.get_class().get_name()=='WorldPartitionHLOD']
        for a in stale_hlods:actors.destroy_actor(a)
        height=texture(SRC/'height_rg.png','T_HeightRG','data',True)
        height.set_editor_property('filter',unreal.TextureFilter.TF_NEAREST)
        height.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        height.set_editor_property('max_texture_size',0)
        unreal.EditorAssetLibrary.save_loaded_asset(height)
        mat=unreal.load_asset(ASSET+'/Materials/M_HeightImport')
        if not mat:
            mat=tools.create_asset('M_HeightImport',ASSET+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
            mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
            tex=sample(mat,height)
            ml.connect_material_property(tex,'RGB',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
            finish(mat)
        ground=ground_material()
        for actor in landscapes:
            actor.set_editor_property('landscape_material',ground)
            actor.set_editor_property('is_spatially_loaded',False)
            actor.set_folder_path('Natural/Terrain')
        world=editor.get_editor_world()
        rt=unreal.RenderingLibrary.create_render_target2d(world,2017,2017,unreal.TextureRenderTargetFormat.RTF_RGBA8)
        unreal.RenderingLibrary.draw_material_to_render_target(world,rt,mat)
        assert land.landscape_import_heightmap_from_render_target(rt,True,0),'Landscape height import failed'
        land.force_layers_full_update()
        ws=world.get_world_settings()
        gm=unreal.load_class(None,'/Game/Hearthward/World/Natural/BP_NaturalWorldGameMode.BP_NaturalWorldGameMode_C')
        assert gm
        ws.set_editor_property('default_game_mode',gm)
        for a in actors.get_all_level_actors():
            if isinstance(a,unreal.PlayerStart):
                # Candidate forest camp: exact terrain height sampled by host preparation.
                layout=json.loads((SRC/'layout.json').read_text(encoding='utf-8'))
                a.set_actor_location(unreal.Vector(-98000,-75000,22000),False,False)
            elif isinstance(a,unreal.DirectionalLight):
                a.set_actor_rotation(unreal.Rotator(pitch=-38,yaw=-38,roll=0),False)
                a.light_component.set_editor_property('intensity',5.5)
            elif isinstance(a,unreal.ExponentialHeightFog):
                a.component.set_editor_property('fog_density',.004)
        unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(-245000,-255000,230000),unreal.Rotator(pitch=-33,yaw=46,roll=0))
        assert levels.save_current_level()
        assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
        traces=[]
        for x,y in [(-980,-750),(1090,450),(800,680),(0,0),(-1500,1000)]:
            hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(x*100,y*100,150000),unreal.Vector(x*100,y*100,-100000),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,[],unreal.DrawDebugTrace.NONE,True)
            data=hit.to_tuple()
            traces.append({'xy_m':[x,y],'hit':bool(data[0]),'height_m':data[4].z/100})
        result={'ok':True,'landscape_actors':len(landscapes),'removed_template_hlods':len(stale_hlods),'map':MAP,'edit_layers':str(land.get_edit_layers_bp()),'traces':traces}
    except Exception:
        result={'ok':False,'error':traceback.format_exc()}
    (OUT/'terrain.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    unreal.log('TASK026_REBUILD_TERRAIN '+str(result))


if __name__ == '__main__':
    main()
