"""Native snapshot/plan/apply for explicitly owned, complete HISM batches.

Run from the editor console. Request is Saved/Task026/ReworkV2/request.json.
Only apply saves packages; snapshots/plans do not alter map objects.
"""
from pathlib import Path
import hashlib
import importlib
import json
import shutil
import stat
import sys
import time
import unreal

sys.path.insert(0, str(Path(__file__).parent))
import rework_contract
importlib.reload(rework_contract)
from rework_contract import digest, make_plan, validate_plan, instances_equivalent

ROOT = Path(unreal.Paths.project_dir())
DOC = ROOT / 'docs/world/TASK-026/rework-v2'
LOCAL = ROOT / 'Saved/Task026/ReworkV2'
LOCAL.mkdir(parents=True, exist_ok=True)
MAP = '/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
assert editor.get_editor_world().get_path_name().split('.')[0] == MAP


def vector(v):
    return [round(v.x, 6), round(v.y, 6), round(v.z, 6)]


def encode(t):
    return [vector(t.translation), [round(getattr(t.rotation, k), 8) for k in 'xyzw'], vector(t.scale3d)]


def decode(t):
    result = unreal.Transform()
    result.translation = unreal.Vector(*t[0])
    result.rotation = unreal.Quat(*t[1])
    result.scale3d = unreal.Vector(*t[2])
    return result


def snapshot(ownership):
    descs = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
    unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in descs])
    packages = {d.guid.to_string(): str(d.actor_package) for d in descs}
    live = {a.actor_guid.to_string(): a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()}
    result = []
    for actor_id, owner in sorted(ownership.items()):
        if actor_id not in live:
            raise ValueError('Registered actor missing: '+actor_id)
        actor = live[actor_id]
        comps = actor.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
        assert len(comps) == 1, actor_id
        comp = comps[0]
        ids = owner.get('instance_ids')
        if ids is not None and len(ids) != comp.get_instance_count():
            raise ValueError('Stored instance identity count differs: '+actor_id)
        instances = [{'id': ids[i] if ids is not None else actor_id+':'+str(i),
                      'transform': encode(comp.get_instance_transform(i, False))}
                     for i in range(comp.get_instance_count())]
        # Unsaved external actors have a native package before their WP descriptor exists.
        package = packages.get(actor_id) or actor.get_package().get_path_name()
        result.append({'id': actor_id, 'package': package, 'label': actor.get_actor_label(),
                       'ownership': owner['ownership'], 'cell': owner['cell'], 'kind': owner['kind'],
                       'actor_transform': encode(actor.get_actor_transform()),
                       'mesh': comp.static_mesh.get_path_name(),
                       'materials': [m.get_path_name() if m else None for m in comp.get_materials()],
                       'collision': str(comp.get_collision_profile_name()),
                       'tags': sorted(str(t) for t in actor.tags),
                       'instances': instances})
    invariant = []
    for actor_id, actor in sorted(live.items()):
        if actor_id in ownership:
            continue
        item = {'id':actor_id, 'class':actor.get_class().get_name(),
                'label':actor.get_actor_label(), 'transform':encode(actor.get_actor_transform())}
        if isinstance(actor, unreal.LandscapeProxy):
            mat = actor.get_editor_property('landscape_material')
            item['landscape_material'] = mat.get_path_name() if mat else None
        invariant.append(item)
    return {'actors': result, 'non_batch_actors':invariant}, live


def input_hashes(paths):
    return {p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}


def package_file(package):
    assert package.startswith('/Game/')
    return ROOT / ('Content/'+package[len('/Game/'):]+'.uasset')


def apply(plan, ownership, before, live):
    validate_plan(plan, before, input_hashes(plan['source_hashes']))
    lock_file = ROOT / 'Saved/Task026/rework-v2-locks.json'
    if time.time()-lock_file.stat().st_mtime > 1800:
        raise ValueError('Refresh remote LFS locks before apply')
    locks = json.loads(lock_file.read_text(encoding='utf-8'))
    locks = locks if isinstance(locks, list) else locks['locks']
    locked = {x['path'] for x in locks if x['owner']['name'] == 'XLingyyy'}
    allowed = set(plan['update_packages'])
    save = unreal.EditorLoadingAndSavingUtils
    dirty_before = {p.get_path_name() for p in [*save.get_dirty_content_packages(), *save.get_dirty_map_packages()]}
    if dirty_before & allowed:
        raise ValueError('Target packages already dirty; save/review before planning')
    backup = LOCAL / 'backups' / plan['plan_hash']
    backup.mkdir(parents=True, exist_ok=True)
    for package in allowed:
        source = package_file(package)
        relative = source.relative_to(ROOT).as_posix()
        if relative not in locked:
            raise ValueError('Missing current own lock: '+relative)
        dest = backup / relative
        if not dest.exists():
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, dest)
        # LFS lock ownership was checked above. The local lockable bit may
        # remain read-only after an external client acquired the remote lock.
        source.chmod(source.stat().st_mode | stat.S_IWRITE)
    (backup/'before.json').write_text(json.dumps(before), encoding='utf-8')
    changed = []
    for op in plan['operations']:
        actor = live[op['id']]
        comp = actor.get_component_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
        current = next(a for a in before['actors'] if a['id'] == op['id'])
        ownership[op['id']]['instance_ids'] = [i['id'] for i in op['instances']]
        ownership[op['id']]['ownership'] = op['ownership']
        same_instances = instances_equivalent(current['instances'], op['instances'])
        if same_instances and current['tags'] == op['tags']:
            continue
        actor.modify()
        comp.modify()
        actor.tags = [unreal.Name(t) for t in op['tags']]
        if not same_instances:
            comp.clear_instances()
            comp.add_instances([decode(i['transform']) for i in op['instances']], False, False)
        assert comp.get_instance_count() == len(op['instances'])
        changed.append(op['id'])
    after, _ = snapshot(ownership)
    assert before['non_batch_actors'] == after['non_batch_actors'], 'Non-batch actors changed'
    original = {a['id']: a for a in before['actors']}
    targets = {o['id'] for o in plan['operations']}
    for a in after['actors']:
        if a['id'] not in targets and digest(a) != digest(original[a['id']]):
            raise AssertionError('Unplanned semantic change: '+a['id'])
        if a['id'] in targets:
            op = next(o for o in plan['operations'] if o['id'] == a['id'])
            assert a['tags'] == op['tags'] and a['ownership'] == op['ownership']
            assert [i['id'] for i in a['instances']] == [i['id'] for i in op['instances']]
            assert instances_equivalent(a['instances'], op['instances']), 'Transform differs from plan'
    dirty = [*save.get_dirty_content_packages(), *save.get_dirty_map_packages()]
    unexpected = {p.get_path_name() for p in dirty}-dirty_before-allowed
    if unexpected:
        raise AssertionError('Unplanned dirty packages; not saved: '+str(unexpected))
    selected = [p for p in dirty if p.get_path_name() in allowed]
    assert save.save_packages(selected, True) if selected else True
    (DOC/'ownership.json').write_text(json.dumps(ownership, indent=2), encoding='utf-8')
    return {'status': 'PASS', 'scope': 'bounded batch application', 'plan_hash': plan['plan_hash'],
            'changed_actor_ids': changed, 'saved_packages': [p.get_path_name() for p in selected],
            'unaffected_semantics_preserved': True, 'protected_ids': list(plan['protected']),
            'backup': backup.relative_to(ROOT).as_posix(), 'after': after}


def main():
    request = json.loads((LOCAL/'request.json').read_text(encoding='utf-8'))
    action = request['action']
    if action not in ['snapshot', 'plan', 'apply']:
        raise ValueError('Unknown action: '+action)
    output = ROOT/request['output']
    if output.exists():
        raise ValueError('Evidence path already exists; use a new run/output path')
    ownership = json.loads((DOC/'ownership.json').read_text(encoding='utf-8'))
    inventory, live = snapshot(ownership)
    if action == 'snapshot':
        result = inventory
    elif action == 'plan':
        changes = json.loads((ROOT/request['replacements']).read_text(encoding='utf-8'))
        result = make_plan(inventory, changes, input_hashes(request['inputs']),
                           [k for k, v in ownership.items() if v['ownership'] == 'Authored'],
                           request.get('promote_authored', []))
    else:
        plan = json.loads((ROOT/request['plan']).read_text(encoding='utf-8'))
        result = apply(plan, ownership, inventory, live)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, ensure_ascii=False, separators=(',', ':')), encoding='utf-8')
    unreal.log('TASK026_REWORK_'+action.upper()+' '+str(output))


if __name__ == '__main__':
    main()
