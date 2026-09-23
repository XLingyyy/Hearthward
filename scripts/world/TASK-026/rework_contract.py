"""Pure-data contract for bounded, reviewable TASK-026 batch updates."""
import hashlib
import json
import math


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':'),
                                     ensure_ascii=False, allow_nan=False).encode('utf-8')).hexdigest()


def stable_seed(region, cell, kind, version):
    return int(digest([region, cell, kind, version])[:16], 16)


def instances_equivalent(a, b):
    """Compare native roundoff and the equivalent quaternion pair q / -q."""
    return len(a) == len(b) and all(
        x['id'] == y['id'] and all(
            abs(u-v) <= tolerance
            for index, tolerance in [(0, 1e-5), (2, 4e-6)]
            for u, v in zip(x['transform'][index], y['transform'][index]))
        and min(max(abs(u-sign*v) for u, v in zip(x['transform'][1], y['transform'][1]))
                for sign in (1, -1)) <= 1e-6 for x, y in zip(a, b))


def make_plan(inventory, replacements, source_hashes, protected_ids, promote_authored=()):
    """Plan complete existing batches; authored/unknown objects cannot be targeted."""
    actors = {a['id']: a for a in inventory['actors']}
    missing = set(protected_ids) - actors.keys()
    if missing:
        raise ValueError('Missing protected IDs: '+', '.join(sorted(missing)))
    operations = []
    if set(promote_authored)-replacements.keys():
        raise ValueError('Ownership promotion must name an explicit complete batch')
    for actor_id, instances in sorted(replacements.items()):
        actor = actors[actor_id]
        if actor_id in protected_ids or actor['ownership'] != 'Generated':
            raise ValueError('Protected or unowned actor: '+actor_id)
        if not actor['package'].startswith('/Game/__ExternalActors__/Hearthward/World/Natural/Rebuild/'):
            raise ValueError('Package outside the rebuild scope: '+actor['package'])
        if len({i['id'] for i in instances}) != len(instances):
            raise ValueError('Duplicate instance IDs: '+actor_id)
        for item in instances:
            t = item['transform']
            if len(t) != 3 or [len(x) for x in t] != [3, 4, 3]:
                raise ValueError('Invalid transform shape')
            if not all(math.isfinite(v) for part in t for v in part):
                raise ValueError('Non-finite transform')
        ownership = 'Authored' if actor_id in promote_authored else 'Generated'
        tags = sorted(set(t for t in actor.get('tags', []) if not t.startswith('T026.Generation=')) |
                      {'T026.Generation='+ownership})
        operations.append({'id': actor_id, 'package': actor['package'], 'cell': actor['cell'],
                           'kind': actor['kind'], 'before_hash': digest(actor),
                           'before_count': len(actor['instances']), 'after_count': len(instances),
                           'ownership': ownership, 'tags': tags,
                           'instances': instances})
    if not operations:
        raise ValueError('Empty update plan')
    protected = {k: digest(actors[k]) for k in sorted(protected_ids)}
    plan = {'schema': 1, 'task': 'TASK-026', 'scope': 'existing_complete_batches',
            'source_hashes': source_hashes, 'protected': protected,
            'operations': operations, 'add_packages': [], 'delete_packages': [],
            'update_packages': sorted({o['package'] for o in operations})}
    plan['plan_hash'] = digest(plan)
    return plan


def validate_plan(plan, inventory, source_hashes):
    payload = {k: v for k, v in plan.items() if k != 'plan_hash'}
    if plan['plan_hash'] != digest(payload):
        raise ValueError('Plan content changed after review')
    if source_hashes != plan['source_hashes']:
        raise ValueError('Inputs changed after plan')
    actors = {a['id']: a for a in inventory['actors']}
    for actor_id, expected in plan['protected'].items():
        if actor_id not in actors or digest(actors[actor_id]) != expected:
            raise ValueError('Protected object changed or missing: '+actor_id)
    for op in plan['operations']:
        if op['id'] not in actors or digest(actors[op['id']]) != op['before_hash']:
            raise ValueError('Batch changed after plan: '+op['id'])
    return True
