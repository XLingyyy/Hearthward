"""Native editor placement of the reviewed natural meshes, river and grass.

252 m instance batches preserve World Partition locality; placement uses the
same sampled ground as Landscape and explicit upright rotations.
"""
from pathlib import Path
from collections import defaultdict
import sys
import math
import json
import traceback
import importlib
import unreal

sys.path.insert(0,str(Path(__file__).parent))
import rebuild_terrain
importlib.reload(rebuild_terrain)
from rebuild_terrain import ROOT,SRC,OUT,ASSET,MAP,tools,ml,texture,expr,sample,constant,connect,finish,ground_material
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
mesh_editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
subobjects=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
existing={}


def color(mat,rgb):return expr(mat,'Constant3Vector',constant=unreal.LinearColor(*rgb,1))


def make_water(name,foam=False):
    mat=unreal.load_asset(ASSET+'/Materials/'+name)
    if mat:return mat
    else:mat=tools.create_asset(name,ASSET+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    mat.set_editor_property('two_sided',True)
    mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
    ml.connect_material_property(color(mat,(.008,.045,.07) if not foam else (.20,.26,.24)),'',unreal.MaterialProperty.MP_BASE_COLOR)
    ml.connect_material_property(constant(mat,.75 if not foam else .9),'',unreal.MaterialProperty.MP_OPACITY)
    ml.connect_material_property(constant(mat,.14 if not foam else .3),'',unreal.MaterialProperty.MP_ROUGHNESS)
    ml.connect_material_property(constant(mat,.4),'',unreal.MaterialProperty.MP_SPECULAR)
    output=expr(mat,'SingleLayerWaterMaterialOutput')
    connect(color(mat,(.001,.004,.006)),output,'ScatteringCoefficients')
    connect(color(mat,(.008,.003,.0015)),output,'AbsorptionCoefficients')
    pos=expr(mat,'WorldPosition'); xy=expr(mat,'ComponentMask',r=True,g=True);connect(pos,xy,'')
    uv=expr(mat,'Divide',const_b=900.0);connect(xy,uv,'A')
    pan=expr(mat,'Panner',speed_x=.014,speed_y=-.023 if not foam else -.20);connect(uv,pan,'Coordinate')
    normal=sample(mat,texture(SRC/'water_normal.png','T_Water_N','normal'),pan,True)
    ml.connect_material_property(normal,'RGB',unreal.MaterialProperty.MP_NORMAL)
    return finish(mat)


def grass_material():
    mat=unreal.load_asset(ASSET+'/Materials/M_GrassCards')
    if mat:return mat
    base=ROOT/'art_source/TASK-004/polyhaven/草/草2/textures'
    mat=tools.create_asset('M_GrassCards',ASSET+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property('two_sided',True)
    d=sample(mat,texture(base/'grass_medium_01_diff_4k.jpg','T_GrassCard_D'))
    a=sample(mat,texture(base/'grass_medium_01_alpha_4k.png','T_GrassCard_A','data'))
    ml.connect_material_property(d,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    ml.connect_material_property(a,'R',unreal.MaterialProperty.MP_OPACITY_MASK)
    ml.connect_material_property(constant(mat,.94),'',unreal.MaterialProperty.MP_ROUGHNESS)
    return finish(mat)


def glb(path,name):
    target=ASSET+'/Meshes/'+path.stem+'/StaticMeshes/'+name
    m=unreal.load_asset(target)
    if m:
        settings=mesh_editor.get_nanite_settings(m)
        if settings.enabled:
            settings.set_editor_property('enabled',False);mesh_editor.set_nanite_settings(m,settings)
            unreal.EditorAssetLibrary.save_loaded_asset(m)
        return m
    task=unreal.AssetImportTask()
    task.set_editor_properties(dict(filename=str(path),destination_path=ASSET+'/Meshes',destination_name=name,
                                     automated=True,save=True,replace_existing=False))
    tools.import_asset_tasks([task])
    meshpaths=[p for p in task.imported_object_paths if isinstance(unreal.load_asset(p),unreal.StaticMesh)]
    assert meshpaths, str(task.imported_object_paths)
    m=unreal.load_asset(meshpaths[0])
    settings=mesh_editor.get_nanite_settings(m);settings.set_editor_property('enabled',False)
    mesh_editor.set_nanite_settings(m,settings);unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m


def batch(label,mesh,items,kind,cell):
    if label in existing:
        actor=existing[label]
        actor.modify()
        comp=actor.get_component_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
        comp.modify()
        comp.clear_instances()
    else:
        actor=actors.spawn_actor_from_class(unreal.Actor,unreal.Vector(cell[0]*25200,cell[1]*25200,0))
        actor.set_actor_label(label);actor.tags=['TASK026.REBUILD',kind]
        actor.set_folder_path('Natural/'+kind)
        actor.set_editor_property('is_spatially_loaded',True)
        handles=subobjects.k2_gather_subobject_data_for_instance(actor)
        handle,reason=subobjects.add_new_subobject(unreal.AddNewSubobjectParams(
            parent_handle=handles[0],new_class=unreal.HierarchicalInstancedStaticMeshComponent))
        assert not str(reason),str(reason)
        comp=unreal.SubobjectDataBlueprintFunctionLibrary.get_object(
            unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle))
    assert isinstance(comp,unreal.HierarchicalInstancedStaticMeshComponent)
    comp.set_static_mesh(mesh)
    comp.set_editor_property('mobility',unreal.ComponentMobility.STATIC)
    comp.set_collision_profile_name('BlockAll' if kind in ['rock','stump','trunk'] else 'NoCollision')
    comp.set_editor_property('cast_shadow',kind not in ['grass','shrub','trunk'])
    if kind=='trunk':comp.set_visibility(False)
    cull={'tree':400000,'shrub':20000,'rock':300000,'stump':35000,'grass':11000,'trunk':150000}[kind]
    comp.set_cull_distances(int(cull*.8),cull)
    actor.set_actor_location(unreal.Vector(cell[0]*25200,cell[1]*25200,0),False,False)
    bounds=mesh.get_bounds(); low=bounds.origin.z-bounds.box_extent.z
    height=bounds.box_extent.z*2
    assert height>0
    transforms=[]
    for x,y,z,yaw,desired in items:
        scale=desired*100/height
        # Ground contact includes the model's authored pivot offset.
        position=unreal.Vector(x*100-cell[0]*25200,y*100-cell[1]*25200,z*100-low*scale-(0 if kind=='rock' else 3))
        transforms.append(unreal.Transform(location=position,rotation=unreal.Rotator(pitch=0,yaw=yaw,roll=0),scale=unreal.Vector(scale,scale,scale)))
    comp.add_instances(transforms,False,False)
    assert comp.get_instance_count()==len(transforms)
    return actor


try:
    assert world.get_path_name().split('.')[0]==MAP,'Open the rebuild map first'
    if any('TASK026.REBUILD' in a.tags for a in actors.get_all_level_actors()):
        raise RuntimeError('Initial dressing refused: existing batches require a reviewed ReworkV2 plan.')
    unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
    existing={a.get_actor_label():a for a in actors.get_all_level_actors() if 'TASK026.REBUILD' in a.tags}
    routes=json.loads((SRC/'routes.json').read_text(encoding='utf-8'))
    terrain_checks=[]
    for x,y,z in routes['loop'][::max(1,len(routes['loop'])//8)]:
        hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(x*100,y*100,150000),unreal.Vector(x*100,y*100,-100000),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,[],unreal.DrawDebugTrace.NONE,True)
        data=hit.to_tuple()
        terrain_checks.append({'xy_m':[x,y],'expected_z_m':z,'hit':bool(data[0]),'actual_z_m':data[4].z/100,
                               'actor':str(data[9]),'scale':str(data[9].get_actor_scale3d()) if data[9] else None})
    (OUT/'height-checks.json').write_text(json.dumps(terrain_checks,indent=2),encoding='utf-8')
    assert all(p['hit'] and abs(p['actual_z_m']-p['expected_z_m'])<.25 for p in terrain_checks),'Height import differs from placement source'
    meshes={k:unreal.load_asset(ASSET+'/Meshes/SM_'+n) for k,n in [('tree','Tree'),('shrub','Shrub'),('rock','Rock'),('stump','Stump')]}
    assert all(meshes.values()), 'Natural mesh adaptation must finish first'
    grass=glb(SRC/'water/GrassCards.glb','SM_GrassCards')
    grass.set_material(0,grass_material());unreal.EditorAssetLibrary.save_loaded_asset(grass)
    meshes['grass']=grass
    trunk=glb(SRC/'water/TrunkCollision.glb','SM_TrunkCollision')
    if mesh_editor.get_simple_collision_count(trunk)==0:
        mesh_editor.add_simple_collisions(trunk,unreal.ScriptingCollisionShapeType.NDOP26)
        unreal.EditorAssetLibrary.save_loaded_asset(trunk)
    meshes['trunk']=trunk
    result={'batches':{},'water':[],'mesh_bounds':{k:str(v.get_bounds()) for k,v in meshes.items()}}
    groups=json.loads((SRC/'scatter.json').read_text(encoding='utf-8'))
    for item,contact in zip(groups['rock'],json.loads((SRC/'rock_contact_heights.json').read_text())):item[2]=contact
    groups['trunk']=[item[:4]+[item[4]*.32] for item in groups['tree']]
    for kind,items in groups.items():
        cells=defaultdict(list)
        for item in items:cells[(math.floor(item[0]/252),math.floor(item[1]/252))].append(item)
        wanted={f'{kind}_{cell[0]}_{cell[1]}' for cell in cells}
        for label,a in existing.items():
            if kind in a.tags and label not in wanted:actors.destroy_actor(a)
        for cell,items in cells.items():batch(f'{kind}_{cell[0]}_{cell[1]}',meshes[kind],items,kind,cell)
        result['batches'][kind]={'actors':len(cells),'instances':len(groups[kind])}
    water=make_water('M_RiverWater');falls=make_water('M_RiverFoam',True)
    for entry in json.loads((SRC/'water.json').read_text(encoding='utf-8')):
        if entry['name'] in existing:
            a=existing[entry['name']]
            m=a.static_mesh_component.static_mesh
            m.set_material(0,falls if entry['kind']=='falls' else water)
            unreal.EditorAssetLibrary.save_loaded_asset(m)
            if entry['name']=='Sea':
                a.modify();a.set_actor_scale3d(unreal.Vector(20,20,1))
                a.set_actor_location(unreal.Vector(-4000000,0,0),False,False)
            result['water'].append({'label':entry['name'],'asset':a.static_mesh_component.static_mesh.get_path_name()})
            continue
        m=glb(SRC/'water'/(entry['name']+'.glb'),'SM_'+entry['name'])
        m.set_material(0,falls if entry['kind']=='falls' else water)
        unreal.EditorAssetLibrary.save_loaded_asset(m)
        x,y,z=entry['position']
        a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x*100,y*100,z*100))
        a.static_mesh_component.set_static_mesh(m)
        a.static_mesh_component.set_collision_profile_name('NoCollision')
        a.static_mesh_component.set_editor_property('cast_shadow',False)
        a.set_actor_label(entry['name']);a.tags=['TASK026.REBUILD','water'];a.set_folder_path('Natural/Water')
        # The sea is a single far-distance visual; river segments stream locally.
        a.set_editor_property('is_spatially_loaded',entry['name']!='Sea')
        result['water'].append({'label':entry['name'],'asset':m.get_path_name()})
    routes=json.loads((SRC/'routes.json').read_text(encoding='utf-8'))
    x,y,z=routes['loop'][0]
    starts=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.PlayerStart)]
    if not starts:starts=[actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(x*100,y*100,z*100+120))]
    for a in starts:
        a.modify()
        a.set_actor_location(unreal.Vector(x*100,y*100,z*100+120),False,False)
        a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=90,roll=0),False)
        a.set_editor_property('is_spatially_loaded',False)
    result['player_starts']=[str(a.get_actor_location()) for a in starts]
    ground_material()
    unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(-135000,-125000,50000),unreal.Rotator(pitch=-15,yaw=46,roll=0))
    assert levels.save_current_level()
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
    result['ok']=True
except Exception:
    result={'ok':False,'error':traceback.format_exc()}
(OUT/'dressing.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
unreal.log('TASK026_DRESSING '+str(result))
