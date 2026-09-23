"""Switch only four owned CAMP_A tree batches to the imported conifer meshes."""
from pathlib import Path
import json
import shutil
import stat
import sys
import time
import traceback

import unreal

sys.path.insert(0, str(Path(__file__).parent))
from rework_batches import snapshot


root = Path(unreal.Paths.project_dir())
local = root / 'Saved/Task026/ReworkV2'
request = json.loads((local / 'camp-conifer-switch-request.json').read_text(encoding='utf-8'))
output = root / request['output']
assert not output.exists()
output.parent.mkdir(parents=True, exist_ok=True)
map_path = '/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().split('.')[0] == map_path
base = '/Game/Hearthward/Assets/NaturalWorld/Rebuild/Meshes/'
phase = request.get('phase', 'initial')
if phase == 'initial':
    assignments = {
        'tree_-4_-3': ('SM_Tree', 'SM_CampFirA'),
        'tree_-5_-3': ('SM_Tree', 'SM_CampPineA'),
        'tree_-4_-4': ('SM_Tree', 'SM_CampFirC'),
        'tree_-5_-4': ('SM_Tree', 'SM_CampPineC'),
    }
elif phase == 'fir_uv':
    assignments = {
        'tree_-4_-3': ('SM_CampFirA', 'SM_CampFirA_CampUV'),
        'tree_-4_-4': ('SM_CampFirC', 'SM_CampFirC_CampUV'),
    }
else:
    raise ValueError('Unknown switch phase: ' + phase)
ownership = json.loads((root / 'docs/world/TASK-026/rework-v2/ownership.json').read_text(encoding='utf-8'))
before, live = snapshot(ownership)
by_label = {a['label']: a for a in before['actors']}
assert all(label in by_label for label in assignments)
plan = {'map': map_path, 'phase': phase, 'targets': []}
for label, (old_name, mesh_name) in assignments.items():
    actor = by_label[label]
    assert actor['kind'] == 'tree' and actor['ownership'] == 'Generated'
    assert actor['mesh'] == base + old_name + '.' + old_name
    comp = live[actor['id']].get_component_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
    overrides = [m.get_path_name() if m else None for m in comp.get_editor_property('override_materials')]
    plan['targets'].append({
        'label': label, 'id': actor['id'], 'package': actor['package'],
        'old_mesh': actor['mesh'], 'new_mesh': base + mesh_name + '.' + mesh_name,
        'instances': len(actor['instances']), 'overrides': overrides,
        'collision': actor['collision'],
    })
plan['targets'].sort(key=lambda item: item['label'])
save = unreal.EditorLoadingAndSavingUtils


try:
    if request['action'] == 'plan':
        output.write_text(json.dumps(plan, ensure_ascii=False, indent=2), encoding='utf-8')
    else:
        assert request['action'] == 'apply'
        assert plan == json.loads((root / request['plan']).read_text(encoding='utf-8'))
        assert not save.get_dirty_content_packages() and not save.get_dirty_map_packages()
        lockfile = root / 'Saved/Task026/rework-v2-locks.json'
        assert time.time() - lockfile.stat().st_mtime < 1800
        locks = json.loads(lockfile.read_text(encoding='utf-8'))
        locks = locks if isinstance(locks, list) else locks['locks']
        owned = {item['path'] for item in locks if item['owner']['name'] == 'XLingyyy'}
        backup = local / ('backups/camp-conifer-meshes' if phase == 'initial'
                          else 'backups/camp-conifer-fir-uv')
        assert not backup.exists()
        packages = set()
        for target in plan['targets']:
            package = target['package']
            path = root / ('Content/' + package[6:] + '.uasset')
            relative = path.relative_to(root).as_posix()
            assert relative in owned, relative
            destination = backup / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, destination)
            path.chmod(path.stat().st_mode | stat.S_IWRITE)
            packages.add(package)
            actor = live[target['id']]
            comp = actor.get_component_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
            mesh = unreal.load_asset(target['new_mesh'])
            assert mesh
            actor.modify()
            comp.modify()
            comp.set_editor_property('override_materials', [])
            comp.set_static_mesh(mesh)
            assert comp.get_instance_count() == target['instances']
            assert str(comp.get_collision_profile_name()) == target['collision']
        after, _ = snapshot(ownership)
        expected = {a['id']: a for a in before['actors']}
        target_ids = {t['id'] for t in plan['targets']}
        for actor in after['actors']:
            original = expected[actor['id']]
            if actor['id'] not in target_ids:
                assert actor == original, actor['label']
            else:
                assert all(actor[key] == original[key] for key in original
                           if key not in ('mesh', 'materials')), actor['label']
                target = next(t for t in plan['targets'] if t['id'] == actor['id'])
                assert actor['mesh'] == target['new_mesh']
        assert after['non_batch_actors'] == before['non_batch_actors']
        dirty = [*save.get_dirty_content_packages(), *save.get_dirty_map_packages()]
        assert {p.get_path_name() for p in dirty} == packages
        assert save.save_packages(dirty, True)
        output.write_text(json.dumps({
            'status': 'PASS', 'targets': plan['targets'],
            'saved_packages': sorted(packages),
            'unplanned_actors_preserved': True,
            'backup': backup.relative_to(root).as_posix(),
        }, ensure_ascii=False, indent=2), encoding='utf-8')
except Exception:
    output.write_text(json.dumps({'error': traceback.format_exc()},
                                 ensure_ascii=False, indent=2), encoding='utf-8')
    raise
