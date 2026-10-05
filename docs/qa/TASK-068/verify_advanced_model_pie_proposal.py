"""Supplemental real-model public-API execution proposal. Root execution required."""
import json
import math
import re
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root = Path(unreal.Paths.project_dir())
qa = root/'docs/qa/TASK-068'
data = json.loads((qa/'advanced-cases-proposal.json').read_text(encoding='utf-8-sig'))
case_match = re.search(r'Task068AdvancedCases=([A-Z0-9,-]+)', unreal.SystemLibrary.get_command_line())
if case_match:
    selected_ids = case_match.group(1).split(',')
    if len(selected_ids) != len(set(selected_ids)) or not set(selected_ids) <= {row['id'] for row in data['cases']}:
        raise ValueError('invalid supplemental case selection')
    data['cases'] = [row for row in data['cases'] if row['id'] in selected_ids]
catalog = json.loads((root/'Resources/Data/gameplay.json').read_text(encoding='utf-8-sig'))
items = [row['id'] for row in catalog['items']]
fish_items = [row['item'] for row in catalog['nature']['fish']]
match = re.search(r'HearthwardAIBackend=(cpu|vulkan)', unreal.SystemLibrary.get_command_line())
backend = match.group(1) if match else 'cpu'
run_match = re.search(r'Task068AdvancedRun=([0-9a-fA-F-]+)', unreal.SystemLibrary.get_command_line())
run_id = run_match.group(1) if run_match else ''
out_label = 'advanced-public-api-'+backend+('-'+run_id if run_id else '')
out = root/'Saved/Task068'/out_label
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
st = {}
report = {'ok': False, 'dataset': data['version'], 'backend': backend, 'run_id': run_id, 'cases': [],
          'method': 'Real PIE SubmitPlayerText once per input; locked production model/config. Public prototype save/load and public Nature.Act/inventory setup. Explicit temporary ground/positioning is fixture setup; no normal-input or Owner credit.',
          'task_complete': False, 'not_covered': ['hunt', 'capture', 'camp_batch', 'escort',
          'Owner voice', 'normal menu/input', 'frozen 60/20', 'joint performance/Shipping']}


def require(label, value):
    if not value:
        raise AssertionError(label)


def wait(predicate, seconds=30):
    return predicate, time.monotonic()+seconds


def delay(seconds):
    end = time.monotonic()+seconds
    return wait(lambda: time.monotonic() >= end, seconds+5)


def key(value):
    return tuple(value.get_editor_property(x) for x in ('a', 'b', 'c', 'd'))


def guid(text):
    value, success = unreal.GuidLibrary.parse_string_to_guid(text)
    require('actual target GUID parses', success)
    return value


def subsystem(cls, world):
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer() == world)


def refresh():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    p = unreal.GameplayStatics.get_player_pawn(world, 0)
    c = unreal.GameplayStatics.get_actor_of_class(world, unreal.HearthwardCompanionFixture)
    st.update(world=world, player=p, brother=c, pc=unreal.GameplayStatics.get_player_controller(world, 0),
              bag=p.get_component_by_class(unreal.HearthwardInventoryComponent),
              game=p.get_component_by_class(unreal.HearthwardGameplayComponent))
    for name, cls in [('ai', unreal.HearthwardLocalAISubsystem), ('save', unreal.HearthwardSaveSubsystem),
                      ('store', unreal.HearthwardStorageSubsystem), ('camp', unreal.HearthwardCampSubsystem),
                      ('nature', unreal.HearthwardNatureSubsystem)]:
        st[name] = subsystem(cls, world)


def add(bag, item, amount):
    require('public inventory grant '+item, bag.try_add(item, amount) == unreal.HearthwardInventoryResult.SUCCESS)


def clear(bag):
    for item in items:
        amount = bag.get_item_count(item)
        if amount:
            require('public inventory removal '+item, bag.try_remove(item, amount) == unreal.HearthwardInventoryResult.SUCCESS)


def nature():
    return json.loads(st['nature'].describe())


def vector(position, x=0, y=0, z=0):
    return unreal.Vector(position['x']+x, position['y']+y, position['z']+z)


def identify(position):
    # Target identity remains the actual Nature state GUID. Moving the participants is explicit QA setup.
    st['player'].set_actor_location(vector(position, -120, -120, 100), False, True)
    st['brother'].set_actor_location(vector(position, 100, -120, 100), False, True)


def target(case):
    collection = 'crops' if case['setup'] == 'plant_greens' else 'points'
    return next(row for row in nature()[collection] if key(guid(row['id'])) == key(st['target']))


def actual_diagnostics(target_position=None):
    def xyz(value):
        return {'x': value.x, 'y': value.y, 'z': value.z}
    p = st['player']; c = st['brother']; position = p.get_actor_location()
    return {'player_position': xyz(position), 'brother_position': xyz(c.get_actor_location()),
            'camp_position': xyz(c.camp.get_actor_location()), 'target_position': target_position,
            'player_movement': str(p.get_component_by_class(unreal.CharacterMovementComponent).get_editor_property('movement_mode')),
            'brother_movement': str(c.get_component_by_class(unreal.CharacterMovementComponent).get_editor_property('movement_mode')),
            'phase': str(c.get_phase()), 'execution_action': c.get_execution_action(),
            'block_reason': str(c.get_editor_property('block_reason')), 'navigation_status': c.navigation.get_status(),
            'nature_feedback': str(st['nature'].get_editor_property('feedback')), 'nature_busy': st['nature'].busy(),
            'resource_route_setup': st.get('resource_route_setup'),
            'plant_ground_expected': {'front_query': xyz(position+p.get_actor_forward_vector()*250),
                                      'flat_normal_minimum': .966, 'requires_camp_and_clear_plot': True,
                                      'temporary_cube_top_z': -50}}


def observed(case):
    c = st['brother']
    t = target(case)
    if case['setup'] == 'plant_greens':
        facts = {k: t[k] for k in ('id', 'definition', 'watered', 'fertilized')}
    elif case['setup'] == 'loose_stones':
        source = next(row for row in json.loads(st['camp'].describe())['sources'] if row['id'] == t['key'])
        facts = {k: source[k] for k in ('id', 'item', 'remaining', 'blocked')}
    else:
        facts = {k: t[k] for k in ('id', 'remaining', 'successes', 'pending')}
        facts['rewards'] = sorted(nature()['rewards'])
        facts['feedback'] = str(st['nature'].get_editor_property('feedback'))
    return {'player': json.loads(st['bag'].describe_inventory()),
            'brother': json.loads(c.bag.describe_inventory()),
            'camp': {item: st['store'].get_item_count(item) for item in items},
            'target': facts, 'requested': c.get_requested(), 'delivered': c.get_delivered(),
            'acquired': c.get_acquired(), 'carried': c.get_carried(),
            'phase': str(c.get_phase()), 'diagnostics': actual_diagnostics(t['position'])}


def count(state, where, item):
    if where == 'camp':
        return state['camp'].get(item, 0)
    return state[where]['stacks'].get(item, 0)+sum(row['definition'] == item for row in state[where]['instances'])


def same_unconfirmed(before, after):
    return all(before[k] == after[k] for k in ('player', 'brother', 'camp', 'target', 'requested', 'delivered', 'acquired', 'carried'))


def execution_matches(case, before, after):
    if after['delivered'] != 1 or after['requested'] != 1 or after['acquired'] != 1 or after['carried'] != 0:
        return False
    if case['setup'] == 'plant_greens':
        return (not before['target']['watered'] and after['target']['watered']
                and before['target']['fertilized'] == after['target']['fertilized']
                and all(before[k] == after[k] for k in ('player', 'brother', 'camp')))
    if case['setup'] == 'loose_stones':
        return (before['target']['remaining']-after['target']['remaining'] == 1
                and count(after, 'camp', 'stone')-count(before, 'camp', 'stone') == 1
                and before['player'] == after['player'] and before['brother'] == after['brother']
                and all(before['camp'][i] == after['camp'][i] for i in items if i != 'stone'))
    rod = next(row for row in before['brother']['instances'] if row['definition'] == 'fishing_rod')
    after_rod = next(row for row in after['brother']['instances'] if row['id'] == rod['id'])
    previous_rewards = set(before['target']['rewards'])
    current_rewards = set(after['target']['rewards'])
    newly_rewarded = current_rewards-previous_rewards
    valid_rewards = {row['id'] for row in catalog['nature']['rewards']}
    bonus_gains = {item: count(after, 'brother', item)-count(before, 'brother', item)
                   for item in items if item not in fish_items and item not in ('bait', 'fishing_rod')}
    empty_bonus = {item: 0 for item in bonus_gains}
    duplicate_bonus = dict(empty_bonus, rope=2, herb=2)
    feedback = after['target']['feedback']
    if feedback == '钓鱼成功':
        bonus_matches = not newly_rewarded and bonus_gains == empty_bonus
    elif feedback == '钓鱼成功并获得额外物品；装不下的部分留在鱼点，可稍后领取':
        if len(newly_rewarded) == 1:
            reward = next(iter(newly_rewarded))
            direct_bonus = dict(empty_bonus, **{reward: 1})
            bonus_matches = reward in valid_rewards and bonus_gains in (direct_bonus, duplicate_bonus)
        else:
            bonus_matches = not newly_rewarded and bool(previous_rewards & valid_rewards) and bonus_gains == duplicate_bonus
    else:
        bonus_matches = False
    before_meta = {k: v for k, v in before['brother'].items() if k not in ('stacks', 'instances')}
    after_meta = {k: v for k, v in after['brother'].items() if k not in ('stacks', 'instances')}
    fish_gains = [count(after, 'brother', item)-count(before, 'brother', item) for item in fish_items]
    return (bonus_matches and previous_rewards <= current_rewards and current_rewards <= valid_rewards
            and before['target']['pending'] == after['target']['pending']
            and sum(fish_gains) == 1 and all(gain >= 0 for gain in fish_gains)
            and count(before, 'brother', 'bait')-count(after, 'brother', 'bait') == 1
            and count(before, 'brother', 'fishing_rod') == count(after, 'brother', 'fishing_rod')
            and {k: v for k, v in rod.items() if k != 'durability'} == {k: v for k, v in after_rod.items() if k != 'durability'}
            and after_rod['durability'] < rod['durability'] and before_meta == after_meta
            and before['target']['remaining']-after['target']['remaining'] == 1
            and after['target']['successes']-before['target']['successes'] == 1
            and before['player'] == after['player'] and before['camp'] == after['camp'])


def setup_case(case):
    st['resource_route_setup'] = None
    st['game'].order_companion('wait')
    st['game'].set_component_tick_enabled(False)
    clear(st['bag']); clear(st['brother'].bag)
    if case['setup'] == 'plant_greens':
        add(st['bag'], 'seed_greens', 1)
        st['player'].set_actor_location(vector(st['center'], -800, -800, 100), False, True)
        st['player'].set_actor_rotation(unreal.Rotator(), False)
        st['pc'].set_control_rotation(unreal.Rotator())
        yield wait(lambda: st['player'].get_component_by_class(unreal.CharacterMovementComponent).get_editor_property('movement_mode') == unreal.MovementMode.MOVE_WALKING, 10)
        old = {row['id'] for row in nature()['crops']}
        require('normal public plant starts', st['nature'].act('plant', unreal.Guid(), 'greens', st['store'].get_timeline_epoch(), 1))
        yield wait(lambda: not st['nature'].busy(), 15)
        rows = [row for row in nature()['crops'] if row['id'] not in old]
        require('plant commits exactly one real crop', len(rows) == 1 and not rows[0]['watered'])
        point = rows[0]
    else:
        if case['setup'] == 'loose_stones':
            rows = [row for row in nature()['points'] if row['definition'] == 'loose_stones' and row['kind'] == 'resource']
        else:
            rows = [row for row in nature()['points'] if row['kind'] == 'fish' and row['remaining'] > 0]
        require('actual initial target exists', rows)
        point = sorted(rows, key=lambda row: row['key'])[0]
        if case['setup'] == 'fish':
            add(st['brother'].bag, 'fishing_rod', 1); add(st['brother'].bag, 'bait', 1)
            require('normal public rod equip', st['brother'].bag.equip_instance(st['brother'].bag.first_instance('fishing_rod')))
    st['target'] = guid(point['id']); identify(point['position'])
    yield delay(.5)
    if case['setup'] == 'loose_stones':
        c = st['brother']; position = vector(point['position'], 600, 0, 100)
        detail = {'purpose': 'Explicit prototype proxy camp on the actual source side; no authored-route credit',
                  'original_proxy': str(c.camp.get_actor_location()), 'source_position': point['position']}
        st['resource_route_setup'] = detail
        hit = unreal.SystemLibrary.line_trace_single(st['world'], position+unreal.Vector(0,0,500), position-unreal.Vector(0,0,500),
                unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [st['player'], c], unreal.DrawDebugTrace.NONE, True)
        detail['ground_trace'] = str(hit)
        require('proxy camp has a real blocking ground trace', hit)
        parts = hit.to_tuple(); detail['ground_impact'] = str(parts[5]); detail['ground_normal'] = str(parts[7])
        detail['ground_actor'] = str(parts[9])
        require('proxy camp trace hits the actual flat tagged temporary ground', parts[0] and parts[7].z >= .966
                and parts[9] and 'Hearthward.NatureGround' in [str(tag) for tag in parts[9].tags])
        capsule = c.get_component_by_class(unreal.CapsuleComponent); half_height = capsule.get_scaled_capsule_half_height()
        proxy = parts[5]+unreal.Vector(0,0,half_height)
        snapshot = json.loads(st['camp'].describe())
        radius = next(row['radius_m'] for row in catalog['campEconomy']['camp_tiers'] if row['tier'] == snapshot['tier'])*100
        old = c.camp.get_actor_location()
        old_site = next((row['id'] for row in snapshot['camps'] if math.hypot(old.x-row['position']['x'], old.y-row['position']['y']) <= radius), None)
        new_site = next((row['id'] for row in snapshot['camps'] if math.hypot(proxy.x-row['position']['x'], proxy.y-row['position']['y']) <= radius), None)
        detail.update(camp_radius_cm=radius, original_site=old_site, proposed_site=new_site,
                      proposed_proxy=str(proxy), capsule_half_height=half_height,
                      capsule_radius=capsule.get_scaled_capsule_radius(),
                      max_step_height=c.get_component_by_class(unreal.CharacterMovementComponent).get_editor_property('max_step_height'))
        require('prototype proxy remains inside the same actual task camp', old_site and old_site == new_site)
        c.camp.set_actor_location(proxy, False, True)
        yield wait(lambda: c.get_component_by_class(unreal.CharacterMovementComponent).get_editor_property('movement_mode') == unreal.MovementMode.MOVE_WALKING, 10)
        yield wait(lambda: (route := unreal.NavigationSystemV1.find_path_to_location_synchronously(st['world'], c.get_actor_location(), proxy, c))
                   and route.is_valid() and not route.is_partial(), 20)
        path = unreal.NavigationSystemV1.find_path_to_location_synchronously(st['world'], c.get_actor_location(), proxy, c)
        distance = math.hypot(c.get_actor_location().x-proxy.x, c.get_actor_location().y-proxy.y)
        detail.update(actual_start=str(c.get_actor_location()), start_distance_cm=distance,
                      path_valid=path.is_valid() if path else False, path_partial=path.is_partial() if path else None,
                      path_points=[str(v) for v in path.path_points] if path else [],
                      collision_bounds=[{'actor': a.get_name(), 'tags': [str(tag) for tag in a.tags], 'bounds': str(a.get_actor_bounds(True))}
                        for a in unreal.GameplayStatics.get_all_actors_of_class(st['world'], unreal.Actor)
                        if any(str(tag) in ('Ground', 'BoundaryWest', 'BoundaryNorth', 'Hearthward.NatureGround') for tag in a.tags)])
        require('return requires an actual five-to-eight metre walk', 500 <= distance <= 800)
        require('actual public return path is complete', path and path.is_valid() and not path.is_partial() and len(path.path_points) >= 2)
    # StageCandidate requires exactly one matching target near the player; assert that before model submission.
    snapshot = nature(); p = st['player'].get_actor_location()
    if case['setup'] == 'plant_greens':
        eligible = [r for r in snapshot['crops'] if not r['watered'] and math.hypot(r['position']['x']-p.x, r['position']['y']-p.y) <= 300]
    elif case['setup'] == 'loose_stones':
        defs = {r['id']: r['item'] for r in catalog['nature']['resources']}
        eligible = [r for r in snapshot['points'] if r['kind'] == 'resource' and defs[r['definition']] == 'stone' and math.hypot(r['position']['x']-p.x, r['position']['y']-p.y) <= 300]
    else:
        eligible = [r for r in snapshot['points'] if r['kind'] == 'fish' and r['remaining'] > 0 and math.hypot(r['position']['x']-p.x, r['position']['y']-p.y) <= 3000]
    require('exact unique actual Stage target', len(eligible) == 1 and key(guid(eligible[0]['id'])) == key(st['target']))


def run():
    (out/'cases.jsonl').write_text('', encoding='utf-8')
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
    refresh(); st['game'].enable_adventure()
    # Keep Nature animals at their real positions; only unrelated live combat actors are moved as fixture isolation.
    for i, actor in enumerate(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)):
        combat = actor.get_component_by_class(unreal.HearthwardCombatTargetComponent)
        if combat and not isinstance(actor, unreal.HearthwardNatureActor) and combat.get_editor_property('health') > 0 and not combat.get_editor_property('protected'):
            actor.set_actor_location(unreal.Vector(150000+i*1000, 150000, 100), False, True)
    st['game'].order_companion('wait'); st['game'].set_component_tick_enabled(False)
    st['brother'].set_actor_location(st['player'].get_actor_location()+unreal.Vector(150, 0, 0), False, True)
    yield wait(lambda: st['brother'].get_component_by_class(unreal.CharacterMovementComponent).get_editor_property('movement_mode') == unreal.MovementMode.MOVE_WALKING, 10)
    require('public prototype capture', st['save'].enable_prototype()); require('public new progress', st['save'].start_new_progress())
    refresh(); st['pc'].get_hud().get_editor_property('screen').open_page('hud')
    st['center'] = json.loads(st['camp'].describe())['camps'][0]['position']
    st['brother'].camp.set_actor_location(vector(st['center'], 100, 0), False, True)
    st['brother'].source.get_owner().set_actor_location(vector(st['center'], 650, 0, 100), False, True)
    st['player'].set_actor_location(vector(st['center'], 100, -120, 100), False, True)
    st['brother'].set_actor_location(vector(st['center'], 250, 0, 100), False, True)
    st['brother'].set_editor_property('source_safe', True)
    yield wait(lambda: any(row['kind'] == 'fish' for row in nature()['points']) and any(row['definition'] == 'loose_stones' for row in nature()['points']), 20)
    clear(st['bag']); clear(st['brother'].bag)
    ids = {key(row.save_id) for row in st['save'].get_points()}
    require('public baseline save', st['save'].save_point(True))
    points = [row.save_id for row in st['save'].get_points() if key(row.save_id) not in ids]
    require('single actual baseline', len(points) == 1); st['baseline'] = points[0]
    report['setup'] = {'baseline': list(key(st['baseline'])), 'normal_input_credit': False,
                       'ground': 'temporary tagged BlockAll cube; unsaved', 'uses_private_state_setters': False}
    for case in data['cases']:
        row = {'id': case['id'], 'input': case['text'], 'fixture_pass': False, 'raw_pass': False, 'execution_pass': False, 'e2e_pass': False}
        try:
            st['pc'].get_hud().get_editor_property('screen').open_page('hud')
            unreal.GameplayStatics.set_game_paused(st['world'], False)
            require('real public baseline load', st['save'].load_point(st['baseline']))
            refresh(); st['pc'].get_hud().get_editor_property('screen').open_page('hud'); yield delay(.3)
            yield from setup_case(case)
            row['fixture_pass'] = True; before = observed(case); row['before'] = before
            ai = st['ai']; started = time.monotonic(); row['submitted_at'] = time.time()
            require('one real model request accepted', ai.submit_player_text(st['player'], st['brother'], case['text']))
            yield wait(lambda: not ai.is_busy(), 250)
            row.update(raw=ai.get_last_structured_result(), reason=ai.get_reason_code(), status=ai.get_status(),
                       line=ai.get_npc_line(), input_tokens=ai.get_input_tokens(), output_tokens=ai.get_output_tokens(),
                       generation_calls=ai.get_generation_calls(), tier=ai.get_context_tier(),
                       dropped=[str(x) for x in ai.get_dropped_context_fields()], filtered_context=ai.get_last_filtered_context(),
                       latency_seconds=time.monotonic()-started, latency_method='submit to completed UE proposal/status; excludes paint/confirmation/execution')
            rejected = root/'Saved/LocalAI/last-rejected-response.json'
            if not row['raw'] and rejected.exists() and rejected.stat().st_mtime >= row['submitted_at']:
                row['raw_rejected_response'] = json.loads(rejected.read_text(encoding='utf-8-sig'))
            try: raw = json.loads(row['raw'])
            except (ValueError, TypeError): raw = {}
            row['raw_pass'] = len(raw) == 8 and isinstance(raw.get('npc_line'), str) and all(raw.get(k) == v for k, v in case['expected'].items())
            row['pipeline_pass'] = row['generation_calls'] == 1 and 0 < row['input_tokens'] <= 3328
            row['after_model'] = observed(case); row['no_unconfirmed_effect'] = same_unconfirmed(before, row['after_model'])
            row['candidate'] = ai.has_candidate(); row['candidate_text'] = ai.get_candidate_text()
            if row['candidate']:
                candidate = ai.get_candidate()
                row['candidate_target_matches'] = key(candidate.get_editor_property('station')) == key(st['target'])
            if row['raw_pass'] and row['pipeline_pass'] and row['no_unconfirmed_effect'] and row.get('candidate_target_matches'):
                row['confirmed'] = ai.confirm_candidate(ai.get_candidate_id())
                if row['confirmed']:
                    yield wait(lambda: st['brother'].get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 90)
                    row['after_confirm'] = observed(case)
                    row['execution_pass'] = execution_matches(case, before, row['after_confirm'])
            row['e2e_pass'] = bool(row['raw_pass'] and row['pipeline_pass'] and row['no_unconfirmed_effect'] and row['execution_pass'])
        except Exception:
            row['error'] = traceback.format_exc()
            row['failure_diagnostics'] = actual_diagnostics()
            if row['fixture_pass']:
                row['after_error'] = observed(case)
        finally:
            st['ai'].cancel_pending()
        report['cases'].append(row)
        with (out/'cases.jsonl').open('a', encoding='utf-8') as stream:
            stream.write(json.dumps(row, ensure_ascii=False)+'\n')


def finish(error=None):
    if error: report['error'] = error
    report['summary'] = {name: sum(bool(row.get(name)) for row in report['cases'])
                         for name in ('fixture_pass', 'raw_pass', 'execution_pass', 'e2e_pass')}
    report['ok'] = not error and len(report['cases']) == len(data['cases']) and all(row['e2e_pass'] for row in report['cases'])
    (out/'results.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
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
