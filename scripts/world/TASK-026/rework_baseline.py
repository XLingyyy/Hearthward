"""Read-only native-editor inventory and fixed ground-level before views."""
from pathlib import Path
import hashlib
import json
import time
import unreal

ROOT = Path(unreal.Paths.project_dir())
DOC = ROOT / 'docs/world/TASK-026/rework-v2'
OUT = ROOT / 'docs/qa/evidence/TASK-026/rework-v2/R0-editor-before'
OUT.mkdir(parents=True, exist_ok=True)
WORLD = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert WORLD.get_path_name().split('.')[0] == '/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'
descs = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in descs])
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()


def vec(v):
    return [v.x, v.y, v.z]


def prop(obj, name):
    try:
        return str(obj.get_editor_property(name))
    except Exception:
        return None


def transform(t):
    return [vec(t.translation), [t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w], vec(t.scale3d)]


records = []
for actor in actors:
    item = {'label': actor.get_actor_label(), 'path': actor.get_path_name(),
            'guid': str(actor.actor_guid), 'class': actor.get_class().get_name(),
            'transform': transform(actor.get_actor_transform()),
            'tags': [str(t) for t in actor.tags], 'spatial': actor.get_editor_property('is_spatially_loaded')}
    components = actor.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
    item['batches'] = []
    for comp in components:
        values = [transform(comp.get_instance_transform(i, False)) for i in range(comp.get_instance_count())]
        item['batches'].append({'mesh': comp.static_mesh.get_path_name() if comp.static_mesh else None,
                               'count': len(values), 'semantic_sha256': hashlib.sha256(json.dumps(values).encode()).hexdigest(),
                               'materials': [m.get_path_name() if m else None for m in comp.get_materials()],
                               'hidden_in_game': comp.get_editor_property('hidden_in_game'),
                               'visible': comp.get_editor_property('visible'),
                               'cull_start': comp.instance_start_cull_distance,
                               'cull_end': comp.instance_end_cull_distance})
    records.append(item)
landscapes = [a for a in actors if isinstance(a, unreal.LandscapeProxy)]
land = next(a for a in landscapes if isinstance(a, unreal.Landscape))
report = {'scope': 'read-only editor inventory; no package save', 'engine': unreal.SystemLibrary.get_engine_version(),
          'landscape_actors': len(landscapes),
          'landscape_components': sum(len(a.get_components_by_class(unreal.LandscapeComponent)) for a in landscapes),
          'landscape_materials': sorted({prop(a, 'landscape_material') for a in landscapes}),
          'layers': str(land.get_edit_layers_bp()), 'actors': records,
          'grass_components': [{'owner': c.get_owner().get_path_name() if c.get_owner() else None,
                                'count': c.get_instance_count()} for c in unreal.ObjectIterator(unreal.GrassInstancedStaticMeshComponent)],
          'descriptor_fields': [x for x in dir(descs[0]) if not x.startswith('_')],
          'descriptor_example': str(descs[0]),
          'console': {n: unreal.SystemLibrary.get_console_variable_float_value(n) for n in
                      ['r.ScreenPercentage', 'r.DynamicRes.OperationMode', 'r.VSync', 't.MaxFPS',
                       'sg.FoliageQuality', 'grass.Enable', 'grass.densityScale', 'grass.GrassMap.UseRuntimeGeneration']}}
(OUT / 'inventory.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')

# Camera heights come from current collision, not the source array.
positions = [('S1-V01', -976, -752, 90), ('S1-V02', -1035, -695, -45),
             ('S1-V03', -1110, -695, 48), ('S1-V04', -1080, -650, 50),
             ('S1-V05', -1010, -680, 0), ('S1-V06', -1150, -775, 50)]
views = []
for name, x, y, yaw in positions:
    hit = unreal.SystemLibrary.line_trace_single(WORLD, unreal.Vector(x*100, y*100, 150000),
        unreal.Vector(x*100, y*100, -100000), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
        True, [], unreal.DrawDebugTrace.NONE, True).to_tuple()
    assert hit[0], name
    views.append({'id': name, 'ground_cm': vec(hit[4]), 'position_cm': [x*100, y*100, hit[4].z+180],
                  'rotation': [-8, yaw, 0], 'camera': 'editor ground-level, 180cm above current trace',
                  'fov': 'editor viewport default; runtime player FOV to be recorded separately',
                  'configuration': 'DX12 SM6 Medium, 100% screen percentage, fixed existing daytime',
                  'image_size': [1920, 1080]})
assert not (DOC / 'views.json').exists(), 'Never overwrite established before camera coordinates'
(DOC / 'views.json').write_text(json.dumps(views, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
state = {'index': 0, 'phase': 'camera', 'until': 0}


def tick(delta):
    if time.monotonic() < state['until']:
        return
    if state['index'] == len(views):
        unreal.unregister_slate_post_tick_callback(handle)
        (OUT / 'summary.json').write_text(json.dumps({'status': 'CAPTURED', 'scope': 'editor baseline', 'views': len(views)}))
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        return
    view = views[state['index']]
    if state['phase'] == 'camera':
        pitch, yaw, roll = view['rotation']
        unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(*view['position_cm']),
            unreal.Rotator(pitch=pitch, yaw=yaw, roll=roll))
        state.update(phase='capture', until=time.monotonic()+8)
    elif state['phase'] == 'capture':
        state['task'] = unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080,
            str(OUT / (view['id']+'.png')), force_game_view=True)
        state.update(phase='wait', until=time.monotonic()+2)
    elif state['task'].is_task_done():
        state.update(index=state['index']+1, phase='camera')


handle = unreal.register_slate_post_tick_callback(tick)
