"""Save the inspected S1 batch state after quaternion-sign validation repair."""
import importlib
import json
import sys
import time
from pathlib import Path
import unreal

sys.path.insert(0, str(Path(__file__).parent))
import rework_batches as b
importlib.reload(b)
out = b.ROOT/'docs/qa/evidence/TASK-026/rework-v2/S1-scatter/apply.json'
assert not out.exists()
plan = json.loads((out.parent/'plan.json').read_text(encoding='utf-8'))
backup = b.LOCAL/'backups'/plan['plan_hash']
before = json.loads((backup/'before.json').read_text(encoding='utf-8'))
b.validate_plan(plan, before, b.input_hashes(plan['source_hashes']))
ownership = json.loads((b.DOC/'ownership.json').read_text(encoding='utf-8'))
for op in plan['operations']:
    ownership[op['id']]['instance_ids'] = [i['id'] for i in op['instances']]
    ownership[op['id']]['ownership'] = op['ownership']
after, _ = b.snapshot(ownership)
assert before['non_batch_actors'] == after['non_batch_actors']
original = {a['id']: a for a in before['actors']}
ops = {o['id']: o for o in plan['operations']}
for a in after['actors']:
    if a['id'] not in ops:
        assert b.digest(a) == b.digest(original[a['id']]), a['id']
    else:
        op = ops[a['id']]
        expected = dict(original[a['id']], instances=a['instances'], tags=op['tags'], ownership=op['ownership'])
        assert a == expected, a['id']
        assert b.instances_equivalent(a['instances'], op['instances']), a['id']
lock_file = b.ROOT/'Saved/Task026/rework-v2-locks.json'
assert time.time()-lock_file.stat().st_mtime < 1800
locks = json.loads(lock_file.read_text(encoding='utf-8'))
locks = locks if isinstance(locks, list) else locks['locks']
owned = {x['path'] for x in locks if x['owner']['name'] == 'XLingyyy'}
for package in plan['update_packages']:
    assert b.package_file(package).relative_to(b.ROOT).as_posix() in owned
save = unreal.EditorLoadingAndSavingUtils
dirty = [*save.get_dirty_content_packages(), *save.get_dirty_map_packages()]
assert {p.get_path_name() for p in dirty} == set(plan['update_packages'])
assert save.save_packages(dirty, True)
(b.DOC/'ownership.json').write_text(json.dumps(ownership, indent=2), encoding='utf-8')
out.write_text(json.dumps({'status':'PASS', 'plan_hash':plan['plan_hash'],
    'scope':'bounded S1 batch application; quaternion-sign-equivalent readback verified',
    'saved_packages':plan['update_packages'], 'unaffected_semantics_preserved':True,
    'protected_ids':list(plan['protected']), 'backup':backup.relative_to(b.ROOT).as_posix(),
    'after':after}), encoding='utf-8')
unreal.log('S1_SCATTER_SAVED '+str(len(dirty)))
