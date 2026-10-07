"""Real runtime-model matrix plus separately labelled deterministic boundaries. No private setters."""
import json
import math
import re
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root = Path(unreal.Paths.project_dir())
qa = root/'docs/qa/TASK-087'
dataset = json.loads((qa/'cases.json').read_text(encoding='utf-8-sig'))
command_line = unreal.SystemLibrary.get_command_line()
backend_match = re.search(r'HearthwardAIBackend=(cpu|vulkan)', command_line)
backend = backend_match.group(1) if backend_match else 'cpu'
limit_match = re.search(r'Task087Cases=(\d+)', command_line)
case_limit = int(limit_match.group(1)) if limit_match else 60
ids_match = re.search(r'Task087CaseIds=([A-Z0-9,]+)', command_line)
selected_ids = ids_match.group(1).split(',') if ids_match else []
if selected_ids:
    assert len(selected_ids) == len(set(selected_ids)) and set(selected_ids) <= {x['id'] for x in dataset['cases']}
selected_cases = ([x for x in dataset['cases'] if x['id'] in selected_ids]
                  if selected_ids else dataset['cases'][:case_limit])
run_match = re.search(r'Task087Run=([A-Za-z0-9_.-]+)', command_line)
assert run_match, 'A unique Task087Run is required; existing results must not be overwritten'
run_id = run_match.group(1)
assert run_id not in ('.', '..'), 'Run id must name a task-local directory'
out = root/'.agent-local/qa/TASK-087'/run_id/backend
assert not out.exists(), 'Run id already exists; use a new id to retain partial failures'
source_dataset = json.loads((root/'docs/qa/TASK-068/cases.json').read_text(encoding='utf-8-sig'))
assert dataset == source_dataset, 'Original 60 expressions, expected fields and categories must remain unchanged'
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
items = [x['id'] for x in json.loads((root/'Resources/Data/gameplay.json').read_text(encoding='utf-8-sig'))['items']]
lock = json.loads((root/'config/local-ai.lock.json').read_text(encoding='utf-8-sig'))
model = root/'Runtime/LocalAI/models'/lock['model']['filename']
candidate_match = re.search(r'Task087CandidateRecord=([^\s]+)', command_line)
if candidate_match:
    candidate_record = json.loads(Path(candidate_match.group(1)).read_text(encoding='utf-8'))
    model = Path(candidate_record['actual_source_path'])
    lock = candidate_record
    bundle_match = re.search(r'HearthwardAIBundlePath=([^\s]+)', command_line)
    assert bundle_match and Path(bundle_match.group(1)).resolve() == Path(candidate_record['bundle_path']).resolve()
    assert model.samefile(candidate_record['runtime_alias_path']), 'Candidate alias must resolve to the recorded model'
revision_match = re.search(r'Task087Revision=([A-Za-z0-9_.-]+)', command_line)
layers_match = re.search(r'HearthwardAIGpuLayers=(\d+)', command_line)
report = {'ok': False, 'task': 'TASK-087', 'run_id': run_id,
          'source_revision': revision_match.group(1) if revision_match else 'UNRECORDED_DIRTY_WORKTREE',
          'original_source': 'docs/qa/TASK-068/cases.json; task068-real-model-v1',
          'model_lock': lock, 'model_size': model.stat().st_size,
          'model_modified_at': model.stat().st_mtime, 'model_hash_recomputed': False,
          'parameters': {'temperature': 0, 'max_input_tokens': 3328, 'max_output_tokens': 256,
                         'parallel': 1, 'context_size': 4096, 'gpu_layers': 0 if backend == 'cpu' else int(layers_match.group(1)) if layers_match else 16,
                         'http_timeout_seconds': 120, 'stream': False, 'thinking': False},
          'original_denominators': {'all': 60, 'clear': 40, 'guarded': 20, 'execution': 30, 'boundaries': 20},
          'backend': backend, 'dataset': dataset['version'], 'case_limit': case_limit,
          'selection': [x['id'] for x in selected_cases], 'diagnostic_subset': bool(selected_ids),
          'method': 'Real PIE SubmitPlayerText / explicitly recorded model / actual raw JSON; public world APIs and public save-point restore. Explicit prototype setup and positioning. C01-C30 confirm real candidates and observe real side effects. Boundary section uses deterministic structured input and receives no raw-model credit.',
           'cases': [], 'boundaries': [], 'performance_auxiliary': [], 'setup': {}, 'task_complete': False,
          'outstanding_gates': ['Advanced Nature/hunt/fish/capture/camp_batch/escort real-model execution coverage',
                                'TASK-087 real OS IME and edited-input preservation', 'Owner role-voice judgement',
                                'Other actual backend', 'TASK-072 joint game/AI performance and second-machine Shipping']}
st = {}


def require(name, value):
    if not value:
        raise AssertionError(name)


def wait(predicate, seconds=30):
    return predicate, time.monotonic()+seconds


def delay(seconds):
    end = time.monotonic()+seconds
    return wait(lambda: time.monotonic() >= end, seconds+5)


def subsystem(cls, world):
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer() == world)


def key(value):
    return tuple(value.get_editor_property(x) for x in ('a', 'b', 'c', 'd'))


def refresh_objects():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    player = unreal.GameplayStatics.get_player_pawn(world, 0)
    brother = unreal.GameplayStatics.get_actor_of_class(world, unreal.HearthwardCompanionFixture)
    st.update(world=world, player=player, brother=brother,
              pc=unreal.GameplayStatics.get_player_controller(world, 0),
              bag=player.get_component_by_class(unreal.HearthwardInventoryComponent),
              game=player.get_component_by_class(unreal.HearthwardGameplayComponent))
    for name, cls in [('ai', unreal.HearthwardLocalAISubsystem), ('save', unreal.HearthwardSaveSubsystem),
                      ('store', unreal.HearthwardStorageSubsystem), ('camp', unreal.HearthwardCampSubsystem)]:
        st[name] = subsystem(cls, world)
    st['ui'] = st['pc'].get_hud().get_editor_property('screen')


def add(bag, item, count):
    require('actual inventory grant '+item, bag.try_add(item, count) == unreal.HearthwardInventoryResult.SUCCESS)


def clear(bag):
    for item in items:
        count = bag.get_item_count(item)
        if count:
            require('actual inventory removal '+item, bag.try_remove(item, count) == unreal.HearthwardInventoryResult.SUCCESS)


def authority():
    c = st['brother']
    return {'camp': {x: st['store'].get_item_count(x) for x in items},
            'brother': json.loads(c.bag.describe_inventory()), 'player': json.loads(st['bag'].describe_inventory()),
            'source': json.loads(c.source.describe_inventory()), 'requested': c.get_requested(),
            'delivered': c.get_delivered(), 'phase': str(c.get_phase()),
            'order': str(st['game'].companion_order), 'routine': st['game'].is_companion_routine_enabled()}


def count(state, container, item):
    if container == 'camp':
        return state['camp'].get(item, 0)
    value = state[container]
    return value['stacks'].get(item, 0)+sum(x['definition'] == item for x in value['instances'])


def conserved(before, after):
    return all(sum(count(before, c, item) for c in ('camp', 'brother', 'player', 'source')) ==
               sum(count(after, c, item) for c in ('camp', 'brother', 'player', 'source')) for item in items)


def unearned_items(case, before, after):
    allowed = {}
    if case.get('intent') == 'craft':
        allowed['arrow' if case['item'] == 'arrows' else 'rope'] = case['quantity']*(4 if case['item'] == 'arrows' else 1)
    extra = {}
    for item in items:
        delta = sum(count(after, c, item)-count(before, c, item) for c in ('camp', 'brother', 'player', 'source'))
        if delta > allowed.get(item, 0): extra[item] = delta-allowed.get(item, 0)
    return extra


def same_world(before, after):
    return all(before[x] == after[x] for x in ('camp', 'brother', 'player', 'source', 'requested', 'delivered', 'order', 'routine'))


def goal(intent='collect', item='wood', quantity=4, mode='additional_acquired', source='S1'):
    value = unreal.HearthwardAgentGoal()
    for name, field in {'intent': intent, 'item': item, 'quantity': quantity,
                        'quantity_mode': mode, 'source_ref': source}.items():
        value.set_editor_property(name, field)
    return value


def propose(value=None):
    return st['ai'].set_structured_goal(st['player'], st['brother'], value or goal())


def setup_case(case):
    ai, c = st['ai'], st['brother']
    st['game'].order_companion('wait')
    st['game'].set_component_tick_enabled(False)
    intent = case.get('intent')
    if intent in ('craft', 'repair'):
        add(c.bag, 'wood', 12); add(c.bag, 'stone', 10)
    if intent == 'repair':
        require('wear actual own GUID', c.bag.wear_instance(c.bag.first_instance('axe'), 60))
    if intent in ('store', 'give') and case.get('source') == 'bag':
        add(c.bag, case['item'], case['quantity'])
    if intent == 'receive' or (intent == 'store' and case.get('source') == 'player_bag'):
        add(st['bag'], case['item'], case['quantity'])
    if case.get('two_axes'):
        add(c.bag, 'axe', 1)
        require('wear first own axe', c.bag.wear_instance(c.bag.first_instance('axe'), 40))
    if case.get('wood_ban'):
        require('store existing ban', ai.put_player_memory(st['player'], c, unreal.Guid(),
                'collection_ban', '以后禁止采集木材', 'wood'))
    if case.get('pressure'):
        for i in range(60):
            require('pressure memory', ai.put_player_memory(st['player'], c, unreal.Guid(), 'claim', f'无关原话记录{i}'))
        for i in range(4):
            require('pressure agreement', ai.put_player_memory(st['player'], c, unreal.Guid(), 'agreement', f'回答简短约定{i}'))
        for i in range(128):
            require('pressure events', st['game'].order_companion(('wait', 'follow', 'attack')[i % 3]))
        st['game'].order_companion('wait')
    if case.get('active_order'):
        require('seed actual cancellable task', propose(goal(quantity=32)))
        require('confirm seed task', ai.confirm_candidate(ai.get_candidate_id()))


def raw_matches(case, value):
    if 'intents' in case:
        return value.get('intent') in case['intents'] and value.get('item') == 'none'
    return (all(value.get(x) == case[x] for x in ('intent', 'item', 'quantity', 'mode', 'source'))
            and sorted(value.get('limits', [])) == sorted(case.get('limits', [])) and not value.get('unresolved'))


def candidate_matches(case):
    ai = st['ai']
    if not ai.has_candidate():
        return False
    value = ai.get_candidate()
    return (str(value.intent) == case['intent'] and str(value.item) == case['item']
            and value.quantity == case['quantity'] and str(value.quantity_mode) == case['mode']
            and str(value.source_ref) == case['source']
            and sorted(str(x) for x in value.limits) == sorted(case.get('limits', [])) and not value.unresolved)


def execution_matches(case, before, after):
    intent, item, amount = case['intent'], case['item'], case['quantity']
    if intent == 'companion_order':
        return (after['order'] == {'hold': 'wait', 'follow': 'follow', 'assist': 'attack', 'routine': 'wait'}[item]
                and after['routine'] == (item == 'routine'))
    if intent == 'collect':
        return (after['delivered'] == amount and count(after, 'camp', item)-count(before, 'camp', item) == amount
                and count(before, 'source', item)-count(after, 'source', item) == amount and conserved(before, after))
    if intent in ('store', 'fetch', 'give', 'receive', 'retrieve'):
        source, target = {'store': ('player' if case['source'] == 'player_bag' else 'brother', 'camp'),
                          'fetch': ('camp', 'brother'), 'give': ('brother', 'player'),
                          'receive': ('player', 'brother'), 'retrieve': ('camp', 'player')}[intent]
        return (after['delivered'] == amount and count(before, source, item)-count(after, source, item) == amount
                and count(after, target, item)-count(before, target, item) == amount and conserved(before, after))
    if intent == 'craft':
        output, produced, wood = ('arrow', amount*4, amount) if item == 'arrows' else ('rope', amount, amount*2)
        source = 'camp' if case['source'] == 'camp' else 'brother'
        return (count(after, 'camp', output)-count(before, 'camp', output) == produced
                and count(before, source, 'wood')-count(after, source, 'wood') == wood)
    if intent == 'repair':
        old = next(x for x in before['brother']['instances'] if x['definition'] == 'axe')
        new = next((x for x in after['brother']['instances'] if x['id'] == old['id']), None)
        return bool(new and old['durability'] == 20 and new['durability'] == 80
                    and count(before, 'brother', 'wood')-count(after, 'brother', 'wood') == 2
                    and count(before, 'brother', 'stone')-count(after, 'brother', 'stone') == 3
                    and before['player'] == after['player'])
    return False


def append(section, row):
    report[section].append(row)
    with (out/(section+'.jsonl')).open('a', encoding='utf-8') as stream:
        stream.write(json.dumps(row, ensure_ascii=False)+'\n')


def restore():
    st['ui'].open_page('hud')
    unreal.GameplayStatics.set_game_paused(st['world'], False)
    require('restore clean public save point', st['save'].load_point(st['baseline']))
    refresh_objects()


def run_boundary(number):
    ai, p, c, store = st['ai'], st['player'], st['brother'], st['store']
    before = authority()
    if number == 1:
        return propose() and same_world(before, authority())
    if number in (2, 4, 6, 7, 9, 10):
        require('boundary card', propose()); old = ai.get_candidate_id()
        if number == 2: ai.cancel_pending()
        elif number == 4: require('adjust card', ai.adjust_candidate(old, 1))
        elif number == 6: require('replacement card', propose(goal(quantity=2)))
        elif number == 7: require('new memory', ai.put_player_memory(p, c, unreal.Guid(), 'claim', '新的原话'))
        elif number == 9: store.advance_timeline()
        else: restore(); ai = st['ai']
        return not ai.confirm_candidate(old) and same_world(before, authority())
    if number == 3:
        add(c.bag, 'wood', 1); before = authority()
        require('one actual store card', propose(goal('store', 'wood', 1, 'held_to_camp', 'bag')))
        card = ai.get_candidate_id(); require('one actual store confirmation', ai.confirm_candidate(card))
        yield wait(lambda: c.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 30)
        after = authority()
        rejected = not ai.confirm_candidate(card)
        yield delay(.3)
        return rejected and same_world(after, authority()) and conserved(before, after)
    if number == 5:
        require('actual inference submitted for cancellation', ai.submit_player_text(p, c, '新采四份木材带回仓库'))
        yield wait(lambda: ai.get_generation_calls() == 1 or not ai.is_busy(), 150)
        require('actual in-flight generation started', ai.is_busy() and ai.get_generation_calls() == 1)
        ai.cancel_pending(); yield delay(2)
        return not ai.is_busy() and not ai.has_candidate() and same_world(before, authority())
    if number == 8:
        require('real execution to cancel', propose()); require('confirm real execution', ai.confirm_candidate(ai.get_candidate_id()))
        require('cancel real execution', ai.cancel_execution(p, c)); after = authority(); yield delay(2)
        return c.get_phase() == unreal.HearthwardCompanionPhase.CANCELLED and same_world(after, authority()) and conserved(before, authority())
    if number == 11:
        p.set_actor_location(c.get_actor_location()+unreal.Vector(4000, 0, 0), False, True)
        return not ai.submit_player_text(p, c, '新采四份木材带回仓库') and same_world(before, authority())
    if number == 12:
        unreal.GameplayStatics.set_game_paused(st['world'], True)
        return not ai.submit_player_text(p, c, '新采四份木材带回仓库') and same_world(before, authority())
    if number in (13, 14):
        return not ai.submit_player_text(p, c, '' if number == 13 else '木'*1001) and same_world(before, authority())
    if number in (15, 16, 17, 18):
        value = goal()
        if number == 15: value.set_editor_property('intent', 'build')
        elif number == 16: value.set_editor_property('quantity', -3)
        elif number == 17: value.set_editor_property('quantity', 33)
        else: value.set_editor_property('source_ref', 'unknown_north')
        return not propose(value) and not ai.has_candidate() and same_world(before, authority())
    if number == 19:
        require('hard collection ban', ai.put_player_memory(p, c, unreal.Guid(), 'collection_ban', '以后禁止采集木材', 'wood'))
        return not propose() and not ai.has_candidate() and same_world(before, authority())
    if number == 20:
        add(st['bag'], 'wood', 100); add(c.bag, 'wood', 1); before = authority()
        return not propose(goal('give', 'wood', 1, 'bag_to_player', 'bag')) and not ai.has_candidate() and same_world(before, authority())
    raise ValueError(number)


def run():
    for name in ('cases', 'boundaries'):
        (out/(name+'.jsonl')).write_text('', encoding='utf-8')
    editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    floor = editor.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -100))
    floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    floor.set_actor_scale3d(unreal.Vector(1600, 1600, 1)); floor.static_mesh_component.set_collision_profile_name('BlockAll')
    floor.tags = ['Hearthward.NatureGround']
    levels.editor_request_begin_play(); yield wait(levels.is_in_play_in_editor); yield delay(1)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    unreal.GameplayStatics.set_game_paused(world, False)
    unreal.SystemLibrary.execute_console_command(world, 'Hearthward.Companion.CreateTest', pc); yield delay(.5)
    refresh_objects(); st['game'].enable_adventure()
    for i, actor in enumerate(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)):
        if actor.get_component_by_class(unreal.HearthwardCombatTargetComponent):
            actor.set_actor_location(unreal.Vector(150000+i*1000, 150000, 100), False, True)
    st['game'].order_companion('wait'); st['game'].set_component_tick_enabled(False)
    c, p = st['brother'], st['player']
    c.set_actor_location(p.get_actor_location()+unreal.Vector(150, 0, 0), False, True)
    yield wait(lambda: c.get_component_by_class(unreal.CharacterMovementComponent).get_editor_property('movement_mode') == unreal.MovementMode.MOVE_WALKING, 10)
    require('public prototype capture', st['save'].enable_prototype()); require('new campaign', st['save'].start_new_progress())
    refresh_objects(); st['ui'].open_page('hud')
    site = json.loads(st['camp'].describe())['camps'][0]['position']
    center = unreal.Vector(site['x'], site['y'], 100)
    c, p = st['brother'], st['player']
    c.camp.set_actor_location(center+unreal.Vector(100, 0, -50), False, True)
    c.source.get_owner().set_actor_location(center+unreal.Vector(650, 0, 0), False, True)
    c.set_actor_location(center+unreal.Vector(250, 0, 0), False, True)
    p.set_actor_location(center+unreal.Vector(100, -120, 0), False, True)
    c.set_editor_property('source_safe', True)
    clear(st['bag']); clear(c.bag); clear(c.source); add(c.bag, 'axe', 1); add(c.source, 'wood', 80)
    build = p.get_component_by_class(unreal.HearthwardBuildingComponent)
    workbench_recipe = next(row for row in json.loads((root/'Resources/Data/gameplay.json').read_text(encoding='utf-8'))['buildings'] if row['id'] == 'workbench')
    for item, quantity in workbench_recipe['materials'].items():
        add(st['bag'], item, quantity)
    st['pc'].set_control_rotation(unreal.Rotator(pitch=-20, yaw=0))
    require('select actual workbench', build.select_building('workbench')); yield delay(.2)
    require('start actual workbench', build.confirm_placement()); yield wait(lambda: build.building_count() >= 1, 60)
    clear(st['bag'])
    unreal.SystemLibrary.execute_console_command(st['world'], 'Hearthward.Storage.CreateTestAccess', st['pc'])
    access = next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageAccessComponent) if x.get_world() == st['world'])
    for item in ('wood', 'stone', 'herb'):
        add(st['bag'], item, 20)
        transfer = access.transfer(st['bag'], True, item, 20, unreal.GuidLibrary.new_guid(), st['store'].get_timeline_epoch())
        require('real baseline warehouse '+item, transfer.moved_count == 20)
    yield delay(.6)
    ids = {key(x.save_id) for x in st['save'].get_points()}
    require('public baseline save', st['save'].save_point(True))
    points = [x.save_id for x in st['save'].get_points() if key(x.save_id) not in ids]
    require('single baseline node', len(points) == 1); st['baseline'] = points[0]
    report['setup'] = {'baseline': list(key(points[0])), 'authority': authority(), 'uses_private_setters': False}
    for case in selected_cases:
        row = {'id': case['id'], 'group': case['group'], 'input': case['text'], 'expected': case, 'raw_pass': False, 'e2e_pass': False, 'execution_pass': False}
        try:
            restore(); yield delay(.3); setup_case(case); yield delay(.1)
            ai = st['ai']; before = authority(); row['before'] = before; row['submitted_at'] = time.time()
            row['model_ready_before_submit'] = bool(ai.is_model_ready())
            row['latency_kind'] = ('warm_followup' if row['model_ready_before_submit']
                                   else 'cold_first' if not report['cases'] else 'cold_restart')
            submitted_monotonic = time.monotonic()
            require('real input submitted', ai.submit_player_text(st['player'], st['brother'], case['text']))
            yield wait(lambda: not ai.is_busy(), 250)
            raw = ai.get_last_structured_result()
            row.update(raw=raw, reason=ai.get_reason_code(), status=ai.get_status(), line=ai.get_npc_line(),
                       filtered_context=ai.get_last_filtered_context(), input_tokens=ai.get_input_tokens(),
                       output_tokens=ai.get_output_tokens(), generation_calls=ai.get_generation_calls(),
                       tier=ai.get_context_tier(), dropped=[str(x) for x in ai.get_dropped_context_fields()],
                       latency_seconds=ai.get_last_latency_seconds(), applied_intent=ai.get_last_applied_intent(),
                       candidate=ai.has_candidate(), candidate_text=ai.get_candidate_text(),
                       ue_verified_latency_seconds=time.monotonic()-submitted_monotonic,
                       ue_verified_latency_clock='time.monotonic',
                       ue_verified_latency_method='submit to !busy and completed proposal/status reads; excludes confirmation, execution and Slate paint',
                       legacy_latency_method='FPlatformTime submission to successful HTTP callback parse; includes model startup when needed')
            rejected = Path(unreal.Paths.project_saved_dir())/'LocalAI/last-rejected-response.json'
            if not raw and rejected.exists() and rejected.stat().st_mtime >= row['submitted_at']:
                row['raw_rejected_response'] = json.loads(rejected.read_text(encoding='utf-8-sig'))
            try: parsed = json.loads(raw)
            except (ValueError, TypeError): parsed = {}
            row['parsed'] = parsed
            row['epoch'] = list(key(st['store'].get_timeline_epoch()))
            row['raw_pass'] = raw_matches(case, parsed)
            row['pipeline_pass'] = row['generation_calls'] == 1 and 0 < row['input_tokens'] <= 3328
            after = authority(); row['after_model'] = after
            row['no_unconfirmed_world_effect'] = same_world(before, after) if not case.get('active_order') else True
            if case.get('execute'):
                row['candidate_pass'] = candidate_matches(case)
                if row['candidate_pass']:
                    row['confirmed'] = ai.confirm_candidate(ai.get_candidate_id())
                    if row['confirmed']:
                        if case['intent'] != 'companion_order':
                            yield wait(lambda: st['brother'].get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 90)
                        else: yield delay(.2)
                        row['after_confirm'] = authority()
                        row['execution_pass'] = execution_matches(case, before, row['after_confirm'])
                row['e2e_pass'] = row['execution_pass'] and row['no_unconfirmed_world_effect']
            elif case.get('confirm_rule'):
                old_revision = ai.get_memory_revision()
                row['e2e_pass'] = (candidate_matches(case) and ai.confirm_candidate(ai.get_candidate_id())
                                   and ai.get_memory_revision() > old_revision and same_world(before, authority()))
            elif case.get('active_order'):
                row['e2e_pass'] = ai.get_last_applied_intent() == 'cancel' and st['brother'].get_phase() == unreal.HearthwardCompanionPhase.CANCELLED and conserved(before, after)
            else:
                row['e2e_pass'] = (not ai.has_candidate() and row['no_unconfirmed_world_effect']
                                   and ai.get_last_applied_intent() in case.get('intents', [case.get('intent')]))
                if case.get('intent') == 'inventory_report':
                    belief = ai.get_camp_stock_belief(case['item'])
                    row['belief'] = {'known': belief.known, 'value': belief.value, 'source': str(belief.source)}
                    row['e2e_pass'] = row['e2e_pass'] and belief.known and belief.value == case['quantity'] and 'player_report' in str(belief.source).lower()
            row['e2e_pass'] = bool(row['e2e_pass'] and row['pipeline_pass'])
            row['unearned_items'] = unearned_items(case, before, authority())
        except Exception:
            row['error'] = traceback.format_exc(); row['e2e_pass'] = False
            if 'before' in row and 'brother' in st:
                row['after_error'] = authority()
                row['unearned_items'] = unearned_items(case, row['before'], row['after_error'])
            if 'ai' in st: st['ai'].cancel_pending()
        append('cases', row)
    if case_limit == 60 and not selected_ids:
        case = next(x for x in dataset['cases'] if x['id'] == 'C01')
        row = {'id': 'WARM-C01-01', 'source_case_id': 'C01', 'scope': 'performance_only',
               'input': case['text'], 'confirmed': False,
               'method': 'One predeclared C01 after the original 60; clean public baseline restore; no confirmation, retry, language or execution credit'}
        try:
            restore(); yield delay(.3); setup_case(case); yield delay(.1)
            ai = st['ai']; before = authority(); row['before'] = before; row['submitted_at'] = time.time()
            row['model_ready_before_submit'] = bool(ai.is_model_ready())
            row['latency_kind'] = 'warm_followup' if row['model_ready_before_submit'] else 'cold_restart'
            require('same backend ready before auxiliary warm request', row['model_ready_before_submit'])
            submitted_monotonic = time.monotonic()
            require('auxiliary real input submitted', ai.submit_player_text(st['player'], st['brother'], case['text']))
            yield wait(lambda: not ai.is_busy(), 250)
            row.update(raw=ai.get_last_structured_result(), reason=ai.get_reason_code(),
                       status=ai.get_status(), line=ai.get_npc_line(),
                       filtered_context=ai.get_last_filtered_context(), input_tokens=ai.get_input_tokens(),
                       output_tokens=ai.get_output_tokens(), generation_calls=ai.get_generation_calls(),
                       tier=ai.get_context_tier(), dropped=[str(x) for x in ai.get_dropped_context_fields()],
                       latency_seconds=ai.get_last_latency_seconds(), applied_intent=ai.get_last_applied_intent(),
                       candidate=ai.has_candidate(), candidate_text=ai.get_candidate_text(),
                       ue_verified_latency_seconds=time.monotonic()-submitted_monotonic,
                       ue_verified_latency_clock='time.monotonic',
                       ue_verified_latency_method='submit to !busy and completed proposal/status reads; excludes confirmation, execution and Slate paint')
            after = authority(); row['after_model'] = after
            row['no_unconfirmed_world_effect'] = same_world(before, after)
            require('auxiliary inference has no unconfirmed world effect', row['no_unconfirmed_world_effect'])
        except Exception:
            row['error'] = traceback.format_exc()
            if 'before' in row and 'brother' in st: row['after_error'] = authority()
            if 'ai' in st: st['ai'].cancel_pending()
        append('performance_auxiliary', row)
        for number in range(1, 21):
            row = {'id': f'B{number:02d}', 'method': 'deterministic public API boundary; no language-understanding credit', 'passed': False}
            try:
                restore(); yield delay(.3)
                row['passed'] = bool((yield from run_boundary(number)))
            except Exception: row['error'] = traceback.format_exc()
            append('boundaries', row)


def finish(error=None):
    if error: report['error'] = error
    cases = report['cases']; clear_cases = [x for x in cases if x['group'] == 'clear']
    guarded = [x for x in cases if x['group'] in ('ambiguous', 'unsupported')]
    executable = [x for x in cases if int(x['id'][1:]) <= 30 and x['group'] == 'clear']
    summary = {'case_count': len(cases), 'raw_correct': sum(x['raw_pass'] for x in cases),
               'guarded_count': len(guarded), 'guarded_raw_correct': sum(x['raw_pass'] for x in guarded),
               'clear_count': len(clear_cases), 'clear_e2e_correct': sum(x['e2e_pass'] for x in clear_cases),
               'execution_count': len(executable), 'execution_correct': sum(x['execution_pass'] for x in executable),
               'unearned_item_cases': sum(bool(x.get('unearned_items')) for x in cases),
               'boundary_count': len(report['boundaries']), 'boundary_correct': sum(x['passed'] for x in report['boundaries'])}
    report['summary'] = summary
    warm_cases = [x for x in cases if x.get('latency_kind') == 'warm_followup']
    minimum_warm_samples = len(dataset['cases'])
    warm_limit = 30 if backend == 'cpu' else 10
    auxiliary = report['performance_auxiliary']
    warm_auxiliary = [x for x in auxiliary if x.get('latency_kind') == 'warm_followup']
    populations = [('warm_component_latency', warm_cases)]
    if auxiliary:
        populations.append(('warm_component_latency_with_auxiliary', warm_cases+warm_auxiliary))
    for name, population in populations:
        warm_latencies = sorted(x['ue_verified_latency_seconds'] for x in population if 'ue_verified_latency_seconds' in x)
        incomplete_replies = sum('ue_verified_latency_seconds' not in x or not x.get('raw') for x in population)
        auxiliary_failures = (sum(bool(x.get('error')) or x.get('latency_kind') != 'warm_followup'
                                  or 'ue_verified_latency_seconds' not in x or not x.get('raw') for x in auxiliary)
                              if name == 'warm_component_latency_with_auxiliary' else 0)
        warm_p95 = None
        if incomplete_replies or auxiliary_failures:
            warm_status = 'INCOMPLETE_REPLIES'
        elif len(warm_latencies) < minimum_warm_samples:
            warm_status = 'INSUFFICIENT_SAMPLES'
        else:
            warm_p95 = warm_latencies[math.ceil(.95*len(warm_latencies))-1]
            warm_status = 'PASS' if warm_p95 <= warm_limit else 'FAIL'
        report[name] = {
            'sample_count': len(warm_latencies), 'minimum_sample_count': minimum_warm_samples,
            'incomplete_reply_count': incomplete_replies, 'p95_seconds': warm_p95,
            'p95_method': 'nearest rank after at least the full expression dataset of warm samples',
            'limit_seconds': warm_limit, 'status': warm_status,
            'clock': 'time.monotonic', 'endpoint': 'UE proposal/status review completed; no Slate paint',
            'cold_first_cases': [x['id'] for x in cases if x.get('latency_kind') == 'cold_first'],
            'cold_restart_cases': [x['id'] for x in cases if x.get('latency_kind') == 'cold_restart'],
            'ui_paint_status': 'NOT_RUN', 'joint_scenario_status': 'NOT_RUN'}
        if name == 'warm_component_latency_with_auxiliary':
            report[name].update(original_warm_sample_count=report['warm_component_latency']['sample_count'],
                                auxiliary_warm_sample_count=len(warm_auxiliary),
                                auxiliary_failure_count=auxiliary_failures,
                                auxiliary_sample_ids=[x['id'] for x in auxiliary],
                                population='Original warm rows plus the one predeclared performance-only C01; no failed rows replaced')
    report['ok'] = bool(not error and summary['case_count'] == 60 and summary['raw_correct'] >= 54
                        and summary['guarded_raw_correct'] >= 18 and summary['clear_e2e_correct'] >= 36
                        and summary['execution_count'] == 30 and summary['execution_correct'] >= 29
                        and summary['unearned_item_cases'] == 0
                        and summary['boundary_count'] == 20 and summary['boundary_correct'] == 20)
    (out/'results.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    import csv
    fields = ['id', 'group', 'input', 'raw_pass', 'candidate_pass', 'pipeline_pass', 'execution_pass',
              'e2e_pass', 'generation_calls', 'input_tokens', 'output_tokens', 'latency_seconds',
              'ue_verified_latency_seconds', 'applied_intent', 'reason', 'raw', 'error']
    with (out/'cases.csv').open('w', encoding='utf-8-sig', newline='') as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, extrasaction='ignore')
        writer.writeheader()
        writer.writerows(cases)
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor(): levels.editor_request_end_play()


flow = run(); pending = None


def tick(_delta):
    global pending
    try:
        if pending:
            if not pending[0]():
                if time.monotonic() > pending[1]:
                    pending = flow.throw(TimeoutError('real runtime stage deadline'))
                return
        pending = next(flow)
    except StopIteration: finish()
    except Exception: finish(traceback.format_exc())


handle = unreal.register_slate_post_tick_callback(tick)
