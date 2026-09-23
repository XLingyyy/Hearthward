"""Stage unsaved new S1 batches, review their exact packages, then apply with locks."""
from pathlib import Path
from collections import defaultdict
import hashlib
import importlib
import json
import math
import shutil
import stat
import sys
import time
import traceback
import unreal

root=Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(Path(__file__).parent))
import rework_batches as batches
importlib.reload(batches)
from rework_contract import digest
local=root/'Saved/Task026/ReworkV2'
config=json.loads((local/'s1-small-trees.json').read_text(encoding='utf-8'))
out=root/config['output'];out.mkdir(parents=True,exist_ok=False)
ownership_path=root/'docs/world/TASK-026/rework-v2/ownership.json'
save=unreal.EditorLoadingAndSavingUtils
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
base='/Game/Hearthward/Assets/NaturalWorld/Rebuild'

def main():
    ownership=json.loads(ownership_path.read_text(encoding='utf-8'))
    before,live=batches.snapshot(ownership)
    source=root/'art_source/TASK-026/Rebuild/ReworkV2/s1-small-trees.json'
    inputs={p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in [source,Path(__file__)]}
    if config['action']=='plan':
        assert not save.get_dirty_content_packages() and not save.get_dirty_map_packages()
        groups=defaultdict(list)
        for point in json.loads(source.read_text(encoding='utf-8'))['points']:
            groups[tuple(point['cell'])].append(point)
        subobjects=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
        meshes={'smalltree':unreal.load_asset(base+'/Meshes/SM_S1_IslandTree')}
        # Use the exact existing proxy dependency rather than guessing an import path.
        proxy=next(a['mesh'] for a in before['actors'] if a['kind']=='trunk')
        meshes['trunk']=unreal.load_asset(proxy)
        assert all(meshes.values())
        added={}
        for cell,items in sorted(groups.items()):
            for kind,mesh in meshes.items():
                label=f'S1_{kind}_{cell[0]}_{cell[1]}'
                assert not any(a.get_actor_label()==label for a in live.values())
                actor=actors.spawn_actor_from_class(unreal.Actor,unreal.Vector(cell[0]*25200,cell[1]*25200,0))
                actor.set_actor_label(label)
                actor.tags=['TASK026.REBUILD',kind,'T026.Generation=Generated','T026.Region=S1_CAMP_LAKE']
                actor.set_folder_path('Natural/'+kind)
                actor.set_editor_property('is_spatially_loaded',True)
                handles=subobjects.k2_gather_subobject_data_for_instance(actor)
                handle,reason=subobjects.add_new_subobject(unreal.AddNewSubobjectParams(
                    parent_handle=handles[0],new_class=unreal.HierarchicalInstancedStaticMeshComponent))
                assert not str(reason),str(reason)
                comp=unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle))
                comp.set_static_mesh(mesh)
                comp.set_editor_property('mobility',unreal.ComponentMobility.STATIC)
                comp.set_collision_profile_name('BlockAll' if kind=='trunk' else 'NoCollision')
                comp.set_editor_property('cast_shadow',kind!='trunk')
                if kind=='trunk':comp.set_visibility(False)
                comp.set_cull_distances(80000,100000)
                actor.set_actor_location(unreal.Vector(cell[0]*25200,cell[1]*25200,0),False,False)
                bounds=mesh.get_bounds();height=bounds.box_extent.z*2;low=bounds.origin.z-bounds.box_extent.z
                transforms=[]
                for item in items:
                    x,y,z=item['xyz_m']
                    desired=item['height_m'] if kind=='smalltree' else 1.6
                    scale=desired*100/height
                    scale_xyz=unreal.Vector(scale,scale,scale) if kind=='smalltree' else unreal.Vector(2.2,2.2,scale)
                    transforms.append(unreal.Transform(location=unreal.Vector(x*100-cell[0]*25200,y*100-cell[1]*25200,z*100-low*scale-3),
                        rotation=unreal.Rotator(pitch=0,yaw=item['yaw'],roll=0),scale=scale_xyz))
                comp.add_instances(transforms,False,False)
                key=actor.actor_guid.to_string()
                added[key]={'ownership':'Generated','cell':list(cell),'kind':kind,
                    'instance_ids':[key+':'+p['id'] for p in items]}
        staged,_=batches.snapshot({**ownership,**added})
        existing=[a for a in staged['actors'] if a['id'] in ownership]
        assert digest(existing)==digest(before['actors'])
        assert before['non_batch_actors']==staged['non_batch_actors']
        new=[a for a in staged['actors'] if a['id'] in added]
        dirty={p.get_path_name() for p in [*save.get_dirty_content_packages(),*save.get_dirty_map_packages()]}
        actor_packages={a['package'] for a in new}
        folder_prefix='/Game/__ExternalObjects__/Hearthward/World/Natural/Rebuild/L_HearthwardWilds/'
        folders={p for p in dirty-actor_packages if p.startswith(folder_prefix)}
        assert len(folders)<=1
        assert all(not (root/('Content/'+p[6:]+'.uasset')).exists() for p in folders)
        materials={base+'/Materials/M_S1_Island_'+part for part in ['trunk','branches','leaves']}
        updated=dirty-actor_packages-folders
        assert updated<=materials,sorted(updated)
        assert all(unreal.load_asset(p).get_editor_property('used_with_instanced_static_meshes') for p in updated)
        plan={'scope':'new S1 small-tree/trunk batches, new folder metadata and automatic material instancing flags',
            'add_packages':sorted(actor_packages|folders),
            'update_packages':sorted(updated),'delete_packages':[],'source_hashes':inputs,'old_semantics':digest(before),
            'added_ownership':added,'staged_actors':new,
            'staging_note':'New actors are staged unsaved to allocate native GUID/package names; plan saves no assets.'}
        plan['plan_hash']=digest(plan)
        assert dirty==set(plan['add_packages']+plan['update_packages']),str(dirty)
        (out/'plan.json').write_text(json.dumps(plan,indent=2),encoding='utf-8')
        return
    assert config['action']=='apply'
    plan=json.loads((root/config['plan']).read_text(encoding='utf-8'))
    assert plan['plan_hash']==digest({k:v for k,v in plan.items() if k!='plan_hash'})
    assert plan['source_hashes']==inputs
    combined={**ownership,**plan['added_ownership']}
    staged,_=batches.snapshot(combined)
    original={'actors':[a for a in staged['actors'] if a['id'] in ownership],
              'non_batch_actors':staged['non_batch_actors']}
    assert digest(original)==plan['old_semantics'],'Existing actors changed since staging'
    assert [a for a in staged['actors'] if a['id'] in plan['added_ownership']]==plan['staged_actors']
    lockfile=root/'Saved/Task026/rework-v2-locks.json'
    assert time.time()-lockfile.stat().st_mtime<1800
    locks=json.loads(lockfile.read_text(encoding='utf-8'));locks=locks if isinstance(locks,list) else locks['locks']
    own={x['path'] for x in locks if x['owner']['name']=='XLingyyy'}
    packages=plan['add_packages']+plan['update_packages']
    assert all('Content/'+p[6:]+'.uasset' in own for p in packages)
    backup=local/'backups'/plan['plan_hash']
    for p in plan['update_packages']:
        path=root/('Content/'+p[6:]+'.uasset')
        dest=backup/path.relative_to(root)
        dest.parent.mkdir(parents=True,exist_ok=True)
        assert not dest.exists()
        shutil.copy2(path,dest)
        path.chmod(path.stat().st_mode|stat.S_IWRITE)
    dirty=[*save.get_dirty_content_packages(),*save.get_dirty_map_packages()]
    assert {p.get_path_name() for p in dirty}==set(packages)
    assert save.save_packages(dirty,True)
    ownership_path.write_text(json.dumps(combined,indent=2),encoding='utf-8')
    (out/'apply.json').write_text(json.dumps({'status':'SAVED_PENDING_VISUAL_COLLISION_REVIEW','plan_hash':plan['plan_hash'],
        'saved_packages':packages,'old_semantics_preserved':True,'actors':plan['staged_actors']},indent=2),encoding='utf-8')

try:main()
except Exception:
    (out/'failure.json').write_text(json.dumps({'error':traceback.format_exc()}),encoding='utf-8')
    raise
