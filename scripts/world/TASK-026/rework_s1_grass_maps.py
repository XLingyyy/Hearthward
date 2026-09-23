"""Persist grass maps for the landscape proxies intersecting the S1 mask.

ALandscapeProxy::PreSave builds and serializes grass maps in an editor save.
No landscape height, material or placement properties are edited here.
"""
from pathlib import Path
import json
import shutil
import stat
import time
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
local = root / 'Saved/Task026/ReworkV2'
config = json.loads((local / 's1-grass-maps.json').read_text(encoding='utf-8'))
out = root / config['output']
out.mkdir(parents=True, exist_ok=False)
save = unreal.EditorLoadingAndSavingUtils
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
descs = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in descs])
targets = {}
details = []
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if not isinstance(actor, unreal.LandscapeProxy):
        continue
    matching = []
    for comp in actor.get_components_by_class(unreal.LandscapeComponent):
        p, extent, radius = unreal.SystemLibrary.get_component_bounds(comp)
        bounds = [p.x - extent.x, p.y - extent.y, p.x + extent.x, p.y + extent.y]
        if bounds[0] < -73000 and bounds[2] > -123000 and bounds[1] < -50000 and bounds[3] > -100000:
            matching.append(bounds)
    if matching:
        package = actor.get_package()
        targets[package.get_path_name()] = package
        details.append({'guid': actor.actor_guid.to_string(), 'package': package.get_path_name(),
                        'component_bounds_cm': matching})
plan = {'update_packages': sorted(targets), 'add_packages': [], 'delete_packages': [],
        'proxies': sorted(details, key=lambda d: d['package']),
        'scope': 'S1 intersecting landscape proxies: editor PreSave grass-map persistence only'}
assert targets and len(targets) <= 9, plan
if config['action'] == 'plan':
    (out / 'plan.json').write_text(json.dumps(plan, indent=2), encoding='utf-8')
else:
    assert config['action'] == 'apply'
    assert plan == json.loads((root / config['plan']).read_text(encoding='utf-8'))
    assert not save.get_dirty_content_packages() and not save.get_dirty_map_packages()
    lockfile = root / 'Saved/Task026/rework-v2-locks.json'
    assert time.time() - lockfile.stat().st_mtime < 1800
    locks = json.loads(lockfile.read_text(encoding='utf-8'))
    locks = locks if isinstance(locks, list) else locks['locks']
    owned = {x['path'] for x in locks if x['owner']['name'] == 'XLingyyy'}
    backup = local / 'backups/s1-grass-maps'
    assert not backup.exists()
    for name in targets:
        relative = 'Content/' + name[6:] + '.uasset'
        assert relative in owned, relative
        source = root / relative
        dest = backup / relative
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, dest)
        source.chmod(source.stat().st_mode | stat.S_IWRITE)
    assert save.save_packages(list(targets.values()), only_dirty=False)
    dirty = [p.get_path_name() for p in [*save.get_dirty_content_packages(), *save.get_dirty_map_packages()]]
    (out / 'apply.json').write_text(json.dumps({'status': 'SAVED_PENDING_FRESH_STANDALONE',
        'plan': plan, 'remaining_dirty': dirty}, indent=2), encoding='utf-8')
    unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),
                                                'grass.DumpGrassData -summary -bygrasstype')
