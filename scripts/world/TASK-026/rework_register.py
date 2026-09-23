"""Read-only migration of the draft ownership manifest to persistent UE GUIDs.

The old str(Guid) values were Python object addresses. Match the original
inventory by exact unique actor label, mesh, count and transform before
registering a real GUID. No map package is changed here.
"""
from pathlib import Path
import json
import unreal

ROOT = Path(unreal.Paths.project_dir())
DOC = ROOT/'docs/world/TASK-026/rework-v2'
OUT = ROOT/'docs/qa/evidence/TASK-026/rework-v2/R1-registration'
assert not OUT.exists(), 'Registration evidence already exists'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().split('.')[0] == '/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'
descs = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in descs])
packages = {d.guid.to_string(): str(d.actor_package) for d in descs}
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
old = json.loads((DOC/'ownership.json').read_text(encoding='utf-8'))
baseline = json.loads((ROOT/'docs/qa/evidence/TASK-026/rework-v2/R0-editor-before/inventory.json').read_text(encoding='utf-8'))
records = {a['label']: a for a in baseline['actors'] if a['batches']}
new = {}
for value in old.values():
    label = value['baseline_label']
    matches = [a for a in actors if a.get_actor_label() == label]
    assert len(matches) == 1, label
    a = matches[0]
    components = a.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
    assert len(components) == 1, label
    c = components[0]
    b = records[label]
    assert c.get_instance_count() == b['batches'][0]['count'], label
    assert c.static_mesh.get_path_name() == b['batches'][0]['mesh'], label
    assert [a.get_actor_location().x, a.get_actor_location().y, a.get_actor_location().z] == b['transform'][0], label
    guid = a.actor_guid.to_string()
    assert len(guid) >= 32 and guid in packages and guid not in new, guid
    new[guid] = dict(value, package=packages[guid])
OUT.mkdir(parents=True)
(OUT/'draft-ownership-invalid-ids.json').write_text(json.dumps(old, indent=2), encoding='utf-8')
(DOC/'ownership.json').write_text(json.dumps(new, indent=2), encoding='utf-8')
(OUT/'summary.json').write_text(json.dumps({'status':'PASS', 'actors':len(new),
    'map_packages_saved':[], 'identity':'FGuid.ToString, verified against actor descriptors',
    'old_guid_fields_invalid':True}), encoding='utf-8')
unreal.log('TASK026_REGISTERED '+str(len(new)))
