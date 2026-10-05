"""Root-executed PIE QA: public UI actions across actual LoadPoint; no model request."""
import json
import re
import runpy
import time
import traceback
import uuid
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
command = unreal.SystemLibrary.get_command_line()
pool = str(uuid.UUID(re.search(r'HearthwardSaveTestPool=([0-9A-Fa-f-]+)', command).group(1)))
label = re.search(r'Task071UILabel=([A-Za-z0-9_-]+)', command).group(1)
out = root/'Saved/Task071'/('ui-'+label+'-'+pool)
out.mkdir(parents=True, exist_ok=True)
report = {'ok': False, 'method': 'Public UI ActionAt / ExecuteAction with real SavePoint and LoadPoint',
          'pool': pool, 'checks': [], 'stage': 'fixture', 'setup': {}, 'actions': [],
          'script_revision': 'ui-progress-slate-separated-v3',
          'slate_event_replay': 'NOTRUN', 'physical_input': 'NOTRUN',
          'model_http': 'NOTRUN: normal manual card only',
          'npc_receipt_operation_checks': 'NOTRUN: no public Python API'}
def progress():
    (out/'progress.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')


def set_stage(name):
    report['stage'] = name
    progress()


def operation(name, invoke):
    report['operation'] = {'name': name, 'state': 'before', 'monotonic': time.monotonic()}
    progress()
    value = invoke()
    report['operation'] = {'name': name, 'state': 'returned', 'monotonic': time.monotonic()}
    progress()
    return value


def ui_action(name, action):
    return operation(name, lambda: st['ui'].execute_action(action))


progress()

# Redirect before advancing the dormant fixture flow or allowing a Slate tick.
base = runpy.run_path(str(root/'docs/qa/TASK-068/verify_model_matrix_pie.py'))
g = base['run'].__globals__
g['out'] = out
unreal.unregister_slate_post_tick_callback(base['handle'])
g['selected_cases'] = []
g['case_limit'] = 1
st, levels = g['st'], g['levels']
key, wait, delay, refresh = g['key'], g['wait'], g['delay'], g['refresh_objects']
authority, count, same_world = g['authority'], g['count'], g['same_world']


def check(name, value):
    report['checks'].append({'name': name, 'pass': bool(value)})
    progress()
    if not value:
        raise AssertionError(name)


# Preserve fixture behavior, adding the same immediate check journal before UI routes start.
g['require'] = lambda name, value: check('fixture: '+name, value)


def guid_digits(value):
    return ''.join(f'{part & 0xffffffff:08X}' for part in key(value))


def action_guid(action):
    return action.rsplit(':', 1)[-1].replace('-', '').upper()


def public_task():
    c = st['brother']
    goal = c.get_goal()
    return {'requested': c.get_requested(), 'delivered': c.get_delivered(),
            'acquired': c.get_acquired(), 'carried': c.get_carried(),
            'goal': {'intent': str(goal.intent), 'item': str(goal.item), 'quantity': goal.quantity,
                     'mode': goal.quantity_mode, 'source': goal.source_ref}}


def action_at(name, predicate):
    """Use the actual public element rectangle and verify its hit action."""
    layout = json.loads(operation(name+' DescribeLayout', lambda: st['ui'].describe_layout()))
    rows = [row for row in layout['components'] if row.get('visible')
            and predicate(row.get('action', ''))]
    check(name+' has one visible actual element', len(rows) == 1)
    row = rows[0]
    x, y, width, height = row['rect']
    point = unreal.Vector2D(x+width/2, y+height/2)
    action = operation(name+' ActionAt', lambda: st['ui'].action_at(point))
    check(name+' actual ActionAt matches rendered element', action == row['action'])
    report['actions'].append({'name': name, 'page': str(st['ui'].get_page()),
                              'action': action, 'layout_id': row['id'], 'rect': row['rect']})
    return action


def open_page(name):
    check('normal UI page '+name, ui_action('OpenPage '+name, 'page:'+name)
          and str(st['ui'].get_page()) == name)
    yield wait(lambda: True, 5)  # Resume on a later Slate post-tick; no blocking sleep.


def check_collect_card(name):
    ai = st['ai']
    check(name+' normal manual card', ui_action(name+' agentCollectCard', 'agentCollectCard') and ai.has_candidate())
    candidate = ai.get_candidate()
    check(name+' actual collect wood one S1', str(candidate.intent) == 'collect'
          and str(candidate.item) == 'wood' and candidate.quantity == 1
          and candidate.quantity_mode == 'additional_acquired' and candidate.source_ref == 'S1'
          and not candidate.limits and not candidate.unresolved)
    action = action_at(name+' confirm', lambda a: a.startswith('agentConfirm:'))
    check(name+' action carries actual candidate GUID', action_guid(action) == guid_digits(ai.get_candidate_id()))
    return action, key(ai.get_candidate_id())


def direct_load(save_id, name):
    before = key(st['store'].get_timeline_epoch())
    check(name+' real LoadPoint', operation(name+' LoadPoint', lambda: st['save'].load_point(save_id)))
    yield wait(lambda: True, 5)
    refresh()
    check(name+' actual epoch changes', key(st['store'].get_timeline_epoch()) != before)
    check(name+' actual HUD restore event', str(st['ui'].get_page()) == 'hud')
    check(name+' candidate and busy cleared', not st['ai'].has_candidate() and not st['ai'].is_busy())


def unchanged(name, state, task):
    check(name+' inventory and authority unchanged', same_world(state, authority()))
    check(name+' public task unchanged', public_task() == task)


def run():
    yield from base['flow']
    refresh()
    baseline_id = st['baseline']
    baseline, baseline_task = authority(), public_task()
    c = st['brother']
    report['setup'] = {'source': 'TASK068 live public fixture', 'save_a': list(key(baseline_id)),
                       'prototype_source_safe': True, 'opponents_positioned_outside_camp': True,
                       'baseline_public_task': baseline_task, 'phase': str(c.get_phase()),
                       'command_id': list(key(c.get_command_id())), 'private_setters': False}
    check('actual baseline node exists', any(key(p.save_id) == key(baseline_id) for p in st['save'].get_points()))
    check('baseline has no material task', all(baseline_task[x] == 0 for x in ('requested', 'delivered', 'acquired', 'carried'))
          and baseline_task['goal']['quantity'] == 0
          and baseline_task['goal']['intent'].lower() in ('', 'none')
          and baseline_task['goal']['item'].lower() in ('', 'none'))
    check('no model generation or server before public card', st['ai'].get_generation_calls() == 0
          and st['ai'].get_server_process_id() == 0)

    set_stage('old_manual_card_action_across_load')
    yield from open_page('dialogue')
    old_action, old_id = check_collect_card('old card')
    report['old_candidate_id'] = list(old_id)
    unchanged('unconfirmed old card', baseline, baseline_task)
    check('Load starts with actual dialogue and candidate', str(st['ui'].get_page()) == 'dialogue'
          and st['ai'].has_candidate())
    yield from direct_load(baseline_id, 'dialogue old card')  # Do not navigate away and cancel it before Load.
    unchanged('actual baseline restored', baseline, baseline_task)
    check('old actual action rejected on HUD', not ui_action('replay old candidate', old_action))
    unchanged('HUD old action', baseline, baseline_task)
    yield from open_page('dialogue')
    check('old actual action rejected after dialogue reopens', not ui_action('replay old candidate', old_action))
    unchanged('reopened dialogue old action', baseline, baseline_task)

    set_stage('fresh_manual_card_ui_execution')
    fresh_action, fresh_id = check_collect_card('fresh card')
    check('fresh actual GUID differs', fresh_id != old_id)
    check('old action rejected beside fresh card', not ui_action('replay old candidate', old_action))
    check('old replay leaves fresh candidate valid', st['ai'].has_candidate()
          and key(st['ai'].get_candidate_id()) == fresh_id)
    unchanged('fresh card before confirmation', baseline, baseline_task)
    check('fresh actual UI confirmation accepted', ui_action('confirm fresh candidate', fresh_action))
    yield wait(lambda: st['brother'].get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 90)
    completed = authority()
    report['fresh_public_task'] = public_task()
    check('fresh UI command actually delivers exactly one', st['brother'].get_delivered() == 1
          and st['brother'].get_requested() == 1 and st['brother'].get_acquired() == 1
          and st['brother'].get_carried() == 0
          and count(completed, 'camp', 'wood') == count(baseline, 'camp', 'wood')+1
          and count(completed, 'source', 'wood') == count(baseline, 'source', 'wood')-1
          and count(completed, 'brother', 'wood') == count(baseline, 'brother', 'wood')
          and completed['player'] == baseline['player'] and g['conserved'](baseline, completed))
    settled_task = public_task()
    yield delay(2)
    unchanged('quiet after fresh UI completion', completed, settled_task)

    set_stage('normal_save_menu_old_confirmation_across_load')
    yield from direct_load(baseline_id, 'prepare independent save menu')
    unchanged('save A restored for independent route', baseline, baseline_task)
    check('normal future inventory input', st['bag'].try_add('wood', 1) == unreal.HearthwardInventoryResult.SUCCESS)
    future, future_task = authority(), public_task()
    check('future B is distinct from A by exactly one player wood', count(future, 'player', 'wood') == count(baseline, 'player', 'wood')+1)
    known = {key(p.save_id) for p in st['save'].get_points()}
    yield from open_page('save')
    save_action = action_at('normal manual save button', lambda a: a == 'save')
    check('normal UI writes actual future SavePoint', ui_action('normal save', save_action))
    new_nodes = [p for p in st['save'].get_points() if key(p.save_id) not in known]
    check('normal save creates one actual different SaveId', len(new_nodes) == 1
          and key(new_nodes[0].save_id) != key(baseline_id))
    future_id = new_nodes[0].save_id
    report['setup']['save_b'] = list(key(future_id))
    ask_a = action_at('normal menu ask load A', lambda a: a.startswith('ask:load:')
                      and action_guid(a) == guid_digits(baseline_id))
    check('normal load A asks confirmation', ui_action('old ask load A', ask_a))
    old_confirm = action_at('old menu confirmation', lambda a: a == 'confirm')
    check('old save confirmation is on actual save page', str(st['ui'].get_page()) == 'save')
    yield from direct_load(future_id, 'external load B while old modal open')
    unchanged('actual B restored', future, future_task)
    check('old generic confirm rejected after external Load', not ui_action('replay old menu confirm', old_confirm))
    unchanged('old modal action cannot load A', future, future_task)

    set_stage('fresh_normal_menu_load_positive')
    yield from open_page('save')
    fresh_ask_a = action_at('fresh normal menu ask load A', lambda a: a.startswith('ask:load:')
                            and action_guid(a) == guid_digits(baseline_id))
    check('fresh normal load A asks confirmation', ui_action('fresh ask load A', fresh_ask_a))
    fresh_confirm = action_at('fresh menu confirmation', lambda a: a == 'confirm')
    epoch = key(st['store'].get_timeline_epoch())
    check('fresh normal menu confirm actually loads A', ui_action('fresh menu confirm', fresh_confirm))
    yield wait(lambda: True, 5)
    refresh()
    check('normal menu Load changes epoch and restores HUD', key(st['store'].get_timeline_epoch()) != epoch
          and str(st['ui'].get_page()) == 'hud')
    unchanged('normal OpenSavePoint restores A', baseline, baseline_task)
    yield delay(2)
    unchanged('quiet after normal menu Load', baseline, baseline_task)
    report['model_generation_calls'] = st['ai'].get_generation_calls()
    report['server_process_id'] = st['ai'].get_server_process_id()
    check('entire public UI test has zero model requests and server', report['model_generation_calls'] == 0
          and report['server_process_id'] == 0)
    set_stage('done')


def finish(error=None):
    if error:
        report['error'] = error
    report['ok'] = not error and all(c['pass'] for c in report['checks']) and report['stage'] == 'done'
    progress()
    (out/'results.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()


flow, pending = run(), None


def tick(_delta):
    global pending
    try:
        if pending and not pending[0]():
            if time.monotonic() > pending[1]:
                pending = flow.throw(TimeoutError('actual runtime stage deadline: '+report['stage']))
            return
        pending = next(flow)
    except StopIteration:
        finish()
    except Exception:
        finish(traceback.format_exc())


handle = unreal.register_slate_post_tick_callback(tick)
