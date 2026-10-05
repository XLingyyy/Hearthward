"""Root-run fixture and passive public PIE sampler; acceptance remains unevaluated."""
import copy
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
label = re.search(r'Task071PhysicalLabel=([A-Za-z0-9_-]+)', command).group(1)
out = root/'Saved/Task071'/('physical-'+pool)
out.mkdir(parents=True, exist_ok=True)
report = {'revision': 'physical-passive-proposal-v1', 'pool': pool, 'label': label,
          'stage': 'fixture', 'recording_complete': False, 'acceptance': 'NOT_EVALUATED',
          'physical_input': 'Root OS keyboard/mouse; sampler cannot certify input provenance',
          'owner_acceptance': 'NOT_EVALUATED', 'normal_new_game': 'NOT_TESTED',
          'fixture_checks': [], 'sample_count': 0, 'event_count': 0, 'layout_count': 0,
          'model_calls_observed_nonzero': False, 'server_process_observed_nonzero': False,
          'model_requests_by_sampler': 0, 'sampling_interval_seconds': .5,
          'sampling_limit': 'Public snapshots at Slate post-tick; transient changes between samples may be missed',
          'setup_disclosure': {
              'source': 'Dormant TASK068 live prototype fixture, not normal new game',
              'floor': 'StaticMesh Cube at (0,0,-100), scale (1600,1600,1), BlockAll, Hearthward.NatureGround tag',
              'combat_targets': 'All actors with CombatTargetComponent moved to (150000+i*1000,150000,100), including any qualifying animals',
              'source_safe': 'Existing base fixture public source_safe field set True; source wood granted 80',
              'companion': 'CreateTest console fixture, axe 1, participant/source/camp positioning; gameplay component tick disabled',
              'storage': 'CreateTestAccess console fixture; public transfers wood/stone/herb 20 each',
              'building': 'Public workbench placement from granted materials',
              'save': 'Public EnablePrototype / StartNewProgress / SavePoint(True) in unique UUID test pool',
              'selected_cases': [], 'case_limit': 1, 'private_setters': False}}
last = None
last_layout = None
pending = None
ready = False
finished = False
next_sample = 0.0
next_progress = 0.0
handle = None


def write(name, value):
    (out/name).write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')


def append(name, value):
    with (out/(name+'.jsonl')).open('a', encoding='utf-8') as stream:
        stream.write(json.dumps(value, ensure_ascii=False)+'\n')


def progress():
    fields = ('revision', 'pool', 'label', 'stage', 'recording_complete', 'acceptance',
              'sample_count', 'event_count', 'layout_count', 'model_calls_observed_nonzero',
              'server_process_observed_nonzero', 'error')
    value = {k: report[k] for k in fields if k in report}
    value.update(updated_unix=time.time(), updated_monotonic=time.monotonic(),
                 fixture_checks=len(report['fixture_checks']), status=report.get('status'),
                 finish_marker=str(out/'finish.json'))
    write('progress.json', value)


def check(name, value):
    report['fixture_checks'].append({'name': name, 'passed': bool(value)})
    if not value:
        raise AssertionError(name)


progress()
base = runpy.run_path(str(root/'docs/qa/TASK-068/verify_model_matrix_pie.py'))
g = base['run'].__globals__
unreal.unregister_slate_post_tick_callback(base['handle'])
g['out'], g['selected_cases'], g['case_limit'] = out, [], 1
g['require'] = check
st = g['st']
key, refresh, authority, count = g['key'], g['refresh_objects'], g['authority'], g['count']


def observation():
    refresh()
    points = [{'save_id': list(key(p.save_id)), 'campaign_id': list(key(p.campaign_id)),
               'manual': bool(p.manual), 'locked': bool(p.locked),
               'location': str(p.location), 'stage': str(p.stage)} for p in st['save'].get_points()]
    return {'page': str(st['ui'].get_page()), 'authority': authority(),
            'epoch': list(key(st['store'].get_timeline_epoch())),
            'campaign_id': list(key(st['save'].get_campaign_id())), 'savepoints': points,
            'save_status': str(st['save'].get_status()),
            'generation_calls': st['ai'].get_generation_calls(),
            'server_process_id': st['ai'].get_server_process_id()}, json.loads(st['ui'].describe_layout())


def difference(before, after):
    return {k: {'before': before.get(k), 'after': after.get(k)}
            for k in sorted(before.keys() | after.keys()) if before.get(k) != after.get(k)}


def sample():
    global last, last_layout
    current, layout = observation()
    stamp = {'sample': report['sample_count']+1, 'unix': time.time(), 'monotonic': time.monotonic()}
    report['sample_count'] += 1
    report['model_calls_observed_nonzero'] |= current['generation_calls'] != 0
    report['server_process_observed_nonzero'] |= current['server_process_id'] != 0
    changed = difference(last, current) if last is not None else {'initial': current}
    if changed:
        append('events', dict(stamp, changes=changed))
        report['event_count'] += 1
    if layout != last_layout:
        report['layout_count'] += 1
        append('layouts', dict(stamp, layout_sequence=report['layout_count'], layout=layout))
    state = current['authority']
    report['status'] = dict(stamp, page=current['page'], epoch=current['epoch'],
                            campaign_id=current['campaign_id'], savepoints=current['savepoints'],
                            generation_calls=current['generation_calls'], server_process_id=current['server_process_id'],
                            wood={k: count(state, k, 'wood') for k in ('player', 'brother', 'source', 'camp')},
                            requested=state['requested'], delivered=state['delivered'], phase=state['phase'],
                            layout_sequence=report['layout_count'])
    last, last_layout = current, layout


def setup():
    global next_sample
    yield from base['flow']
    baseline, _layout = observation()
    report['baseline_a'] = {'save_id': list(key(st['baseline'])), 'public_state': baseline}
    check('baseline A exists', any(p['save_id'] == list(key(st['baseline'])) for p in baseline['savepoints']))
    check('zero model calls and no model server before hold', baseline['generation_calls'] == 0 and baseline['server_process_id'] == 0)
    check('future player wood grant succeeds', st['bag'].try_add('wood', 1) == unreal.HearthwardInventoryResult.SUCCESS)
    future, _layout = observation()
    expected = copy.deepcopy(baseline['authority'])
    expected['player']['stacks']['wood'] = count(baseline['authority'], 'player', 'wood')+1
    check('only inventory change is player wood +1', future['authority'] == expected)
    check('future is not a new save or epoch', future['epoch'] == baseline['epoch'] and future['savepoints'] == baseline['savepoints'])
    report['future'] = {'public_state': future, 'difference_from_baseline': difference(baseline, future), 'saved': False}
    write('setup.json', {k: report[k] for k in ('setup_disclosure', 'fixture_checks', 'baseline_a', 'future')})
    report['stage'] = 'hold_ready_for_root_os_input'
    sample()
    next_sample = time.monotonic()+.5
    progress()


def finish(marker=None, error=None):
    global finished
    if finished:
        return
    finished = True
    report['ended_unix'] = time.time()
    report['root_finish_marker'] = marker
    report['recording_complete'] = marker is not None and error is None
    if error:
        report['error'] = error
    report['stage'] = 'recording_finished' if report['recording_complete'] else 'recording_error'
    report['final_public_state'] = last
    if last is not None and 'baseline_a' in report:
        report['final_difference_from_baseline'] = difference(report['baseline_a']['public_state'], last)
        report['final_difference_from_future'] = difference(report['future']['public_state'], last)
    write('results.json', report)
    progress()
    if handle is not None:
        unreal.unregister_slate_post_tick_callback(handle)
    # Host stops its launched Editor PID after consuming results; no UI action or load here.


flow = setup()


def tick(_delta):
    global pending, ready, next_sample, next_progress
    if finished:
        return
    try:
        now = time.monotonic()
        if not ready and now >= next_progress:
            next_progress = now+.5
            progress()
        if not ready:
            if pending and not pending[0]():
                if now > pending[1]:
                    raise TimeoutError('fixture public runtime wait expired')
                return
            try:
                pending = next(flow)
                return
            except StopIteration:
                ready = True
        if now < next_sample:
            return
        next_sample = now+.5
        sample()
        progress()
        marker = out/'finish.json'
        if marker.exists():
            finish(json.loads(marker.read_text(encoding='utf-8-sig')))
    except Exception:
        finish(error=traceback.format_exc())


handle = unreal.register_slate_post_tick_callback(tick)
