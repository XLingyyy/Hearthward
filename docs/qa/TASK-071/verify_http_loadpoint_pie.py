"""Root-executed PIE QA: actual model candidate and pending HTTP across LoadPoint."""
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
label = re.search(r'Task071HTTPLabel=([A-Za-z0-9_-]+)', command).group(1)
out = root/'Saved/Task071'/('http-'+label+'-'+pool)
out.mkdir(parents=True, exist_ok=True)
report = {'ok': False, 'method': 'Real Qwen HTTP, SavePoint and LoadPoint; prototype PIE positioning',
          'pool': pool, 'checks': [], 'stage': 'fixture', 'human_menu_physical_input': 'NOTRUN',
          'forced_late_successful_response': 'NOTRUN',
          'npc_receipt_operation_checks': 'NOTRUN: no public Python API', 'setup': {}}
# runpy creates a dormant generator. No Slate tick runs inside this synchronous Python call.
base = runpy.run_path(str(root/'docs/qa/TASK-068/verify_model_matrix_pie.py'))
g = base['run'].__globals__
g['out'] = out  # Before advancing its flow or writing either cases.jsonl or boundaries.jsonl.
unreal.unregister_slate_post_tick_callback(base['handle'])
g['selected_cases'] = []
g['case_limit'] = 1  # Reuse world/baseline setup only; neither frozen matrix nor boundaries execute.
st = g['st']
levels = g['levels']
key, wait, delay, refresh = g['key'], g['wait'], g['delay'], g['refresh_objects']
authority, count, same_world = g['authority'], g['count'], g['same_world']
texts = {row['id']: row['text'] for row in g['dataset']['cases']}


def check(name, value):
    report['checks'].append({'name': name, 'pass': bool(value)})
    if not value:
        raise AssertionError(name)


def save_now(name):
    known = {key(p.save_id) for p in st['save'].get_points()}
    check(name+' actual SavePoint', st['save'].save_point(True))
    nodes = [p for p in st['save'].get_points() if key(p.save_id) not in known]
    check(name+' single new node', len(nodes) == 1)
    c = st['brother']
    return {'save_id': list(key(nodes[0].save_id)), 'phase': str(c.get_phase()),
            'requested': c.get_requested(), 'delivered': c.get_delivered(),
            'acquired': c.get_acquired(), 'carried': c.get_carried()}


def load_baseline():
    before = key(st['store'].get_timeline_epoch())
    g['restore']()  # Real LoadPoint, refreshed public participants; no synthetic response.
    check('LoadPoint changes epoch', key(st['store'].get_timeline_epoch()) != before)
    check('LoadPoint cancels busy', not st['ai'].is_busy())
    check('LoadPoint clears candidate', not st['ai'].has_candidate())
    check('LoadPoint clears raw response', not st['ai'].get_last_structured_result())


def check_collect_candidate():
    ai = st['ai']
    parsed = json.loads(ai.get_last_structured_result())
    expected = {'intent': 'collect', 'item': 'wood', 'quantity': 1,
                'mode': 'additional_acquired', 'source': 'S1', 'limits': [], 'unresolved': []}
    check('real C01 generation stays within contract', ai.get_generation_calls() == 1
          and 0 < ai.get_input_tokens() <= 3328 and 0 < ai.get_output_tokens() <= 256)
    check('real C01 raw and candidate ready', ai.has_candidate()
          and all(parsed.get(k) == v for k, v in expected.items()))
    return ai.get_candidate_id()


def watch(expected, deadline, cleared):
    while time.monotonic() < deadline:
        actual = authority()
        check('no extra inventory or task effect', same_world(expected, actual))
        check('no stale candidate', not st['ai'].has_candidate())
        check('no old pending request', not st['ai'].is_busy())
        if cleared:
            check('cancelled request cannot publish raw', not st['ai'].get_last_structured_result())
        yield delay(.5)


def run():
    yield from base['flow']  # Existing live Ground/Companion/camp/workbench/inventory/baseline SavePoint.
    refresh()
    report['setup'] = {'save_id': list(key(st['baseline'])), 'source': 'TASK068 live public fixture',
                       'private_setters': False, 'prototype_source_safe': True,
                       'opponents_positioned_outside_camp': True}
    baseline = authority()
    baseline_node = next(p for p in st['save'].get_points() if key(p.save_id) == key(st['baseline']))
    check('baseline actual saved ID retained', key(baseline_node.save_id) == key(st['baseline']))
    c = st['brother']
    goal = c.get_goal()
    report['setup']['baseline_public_task'] = {
        'phase': str(c.get_phase()), 'command_id': list(key(c.get_command_id())),
        'requested': c.get_requested(), 'delivered': c.get_delivered(),
        'acquired': c.get_acquired(), 'carried': c.get_carried(),
        'goal': {'intent': str(goal.intent), 'item': str(goal.item), 'quantity': goal.quantity,
                 'mode': goal.quantity_mode, 'source': goal.source_ref}}
    check('baseline has no requested or carried material task', c.get_requested() == 0
          and c.get_delivered() == 0 and c.get_acquired() == 0 and c.get_carried() == 0
          and goal.quantity == 0 and str(goal.intent).lower() in ('', 'none')
          and str(goal.item).lower() in ('', 'none'))

    report['stage'] = 'real_candidate_across_load'
    ai = st['ai']
    check('first actual model submit', ai.submit_player_text(st['player'], st['brother'], texts['C01']))
    yield wait(lambda: not ai.is_busy(), 250)
    old_id = check_collect_candidate()
    report['old_candidate_id'] = list(key(old_id))
    check('candidate before confirmation has no world effect', same_world(baseline, authority()))
    load_baseline()
    check('old real confirmation rejected after Load', not ai.confirm_candidate(old_id)
          and ai.get_reason_code() == 'STALE_CONFIRMATION')
    check('old confirmation has no effect', same_world(baseline, authority()))

    report['stage'] = 'real_pending_http_across_load'
    check('second actual model submit', ai.submit_player_text(st['player'], st['brother'], texts['C02']))
    yield wait(lambda: ai.is_busy() and ai.get_generation_calls() == 1
               and ai.get_status() == '弟弟正在思考' and not ai.get_last_structured_result(), 250)
    dispatch_at = time.monotonic()
    report['pending_http'] = {'generation_calls': ai.get_generation_calls(),
                              'input_tokens': ai.get_input_tokens(), 'tier': ai.get_context_tier(),
                              'dropped': [str(x) for x in ai.get_dropped_context_fields()],
                              'status': ai.get_status(), 'raw_empty': not ai.get_last_structured_result()}
    check('pending dispatch stayed within original budget', 0 < ai.get_input_tokens() <= 3328)
    load_baseline()
    yield from watch(baseline, time.monotonic()+3, True)
    checkpoint = save_now('cancelled HTTP checkpoint')
    check('cancelled HTTP leaves inventory and public task unchanged', same_world(baseline, authority())
          and checkpoint['requested'] == 0
          and checkpoint['delivered'] == 0 and checkpoint['acquired'] == 0 and checkpoint['carried'] == 0)
    report['cancelled_checkpoint'] = checkpoint

    report['stage'] = 'fresh_actual_model_and_execution'
    check('fresh actual model submit', ai.submit_player_text(st['player'], st['brother'], texts['C01']))
    yield wait(lambda: not ai.is_busy(), 250)
    fresh_id = check_collect_candidate()
    check('fresh candidate ID differs', key(fresh_id) != key(old_id))
    check('fresh candidate has no unconfirmed effect', same_world(baseline, authority()))
    check('old confirmation still rejected beside fresh candidate', not ai.confirm_candidate(old_id))
    check('fresh confirmation accepted', ai.confirm_candidate(fresh_id))
    yield wait(lambda: st['brother'].get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 90)
    after = authority()
    check('fresh command delivers exactly once', st['brother'].get_delivered() == 1
          and count(after, 'camp', 'wood') == count(baseline, 'camp', 'wood')+1
          and count(after, 'source', 'wood') == count(baseline, 'source', 'wood')-1
          and count(after, 'brother', 'wood') == count(baseline, 'brother', 'wood')
          and after['player'] == baseline['player'])
    settled = save_now('fresh completed checkpoint')
    check('fresh saved public task completed once', settled['phase'] == str(unreal.HearthwardCompanionPhase.COMPLETED)
          and settled['requested'] == 1 and settled['delivered'] == 1)
    report['fresh_checkpoint'] = settled
    report['stage'] = 'old_request_timeout_horizon'
    # Existing Generate timeout/activity timeout is 120s; no blocked sleep or synthetic callback.
    yield from watch(after, max(time.monotonic()+3, dispatch_at+125), False)
    final = save_now('final quiet checkpoint')
    check('no extra public completion after old request horizon', same_world(after, authority())
          and all(final[k] == settled[k] for k in ('phase', 'requested', 'delivered', 'acquired', 'carried')))
    report['final_checkpoint'] = final
    report['observed_seconds_after_old_dispatch'] = time.monotonic()-dispatch_at
    report['stage'] = 'done'


def finish(error=None):
    if error:
        report['error'] = error
    report['ok'] = not error and all(c['pass'] for c in report['checks']) and report['stage'] == 'done'
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
