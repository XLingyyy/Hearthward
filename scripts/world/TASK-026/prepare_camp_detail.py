"""Add bounded natural detail around CAMP_A from a fresh native batch snapshot.

The existing complete HISM batches and stable instance IDs remain intact. The
paired tree/trunk token is reserved for a future harvesting interaction.
"""
from pathlib import Path
import argparse
import copy
import json
import math

import numpy as np
from scipy.spatial import cKDTree

from prepare_dressing import h, slope, wet, sample


ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / 'art_source/TASK-026/Rebuild/ReworkV2'
CAMP = (-980.0, -750.0)
TREE_BOUNDS = (-14.661538, 1946.88749)
ROCK_BOUNDS = (0.035519, 57.101704, 80.149841, 34.187942)
SHRUB_BOUNDS = (-0.875256, 39.597428)
# dx/dy are metres from CAMP_A. The north-east gap keeps the lake visible.
TREES = [
    (-37, 12, 16, 31), (-46, 23, 13, 142), (-34, -10, 15, 218),
    (-48, -18, 11, 86), (36, 8, 16, 285), (45, 17, 13, 57),
    (54, -2, 12, 184), (-18, -38, 14, 113), (9, -42, 12, 260),
    (25, -33, 11, 8),
]


def position(actor, instance):
    return tuple((actor['actor_transform'][0][axis] + instance['transform'][0][axis]) / 100
                 for axis in (0, 1))


def transform(actor, x, y, z, low_cm, scale, yaw):
    half = math.radians(yaw) / 2
    return [[round(x * 100 - actor['actor_transform'][0][0], 6),
             round(y * 100 - actor['actor_transform'][0][1], 6),
             round(z * 100 - low_cm * scale - 3, 6)],
            [0, 0, round(math.sin(half), 8), round(math.cos(half), 8)],
            [round(scale, 6)] * 3]


def main(snapshot):
    inventory = json.loads(snapshot.read_text(encoding='utf-8'))
    actors = inventory['actors']
    batches = {(a['kind'], *a['cell']): a for a in actors
               if a['label'] == f"{a['kind']}_{a['cell'][0]}_{a['cell'][1]}"}
    route = json.loads((SOURCE / 's1-route.json').read_text(encoding='utf-8'))['loop']
    route_tree = cKDTree([p[:2] for p in route])
    existing_trees = [position(a, i) for a in actors if a['kind'] in ('tree', 'smalltree')
                      for i in a['instances']]
    existing_rocks = [position(a, i) for a in actors if a['kind'] == 'rock'
                      for i in a['instances']]
    changes = {}
    manifest = []

    def batch(kind, x, y):
        cell = (math.floor(x / 252), math.floor(y / 252))
        a = batches.get((kind, *cell))
        if not a or a['ownership'] != 'Generated':
            raise ValueError(f'Missing generated {kind} batch at {cell}')
        return a

    def add(actor, token, x, y, z, low, scale, yaw):
        instances = changes.setdefault(actor['id'], copy.deepcopy(actor['instances']))
        instance_id = actor['id'] + ':CAMP_A_DETAIL:' + token
        if any(i['id'] == instance_id for i in instances):
            raise ValueError('Camp detail has already been applied')
        instances.append({'id': instance_id,
                          'transform': transform(actor, x, y, z, low, scale, yaw)})
        return instance_id

    for index, (dx, dy, height, yaw) in enumerate(TREES):
        x, y = CAMP[0] + dx, CAMP[1] + dy
        z = sample(h, x, y)
        if wet(x, y, z) or sample(slope, x, y) > 0.3 or route_tree.query([x, y])[0] < 3.2:
            raise ValueError(f'Tree location unsafe: {index}')
        if cKDTree(existing_trees).query([x, y])[0] < 8:
            raise ValueError(f'Tree overlaps existing grove: {index}')
        existing_trees.append((x, y))
        tree = batch('tree', x, y)
        trunk = batch('trunk', x, y)
        grounded = min(sample(h, x + ox, y + oy) for ox, oy in
                       [(0, 0), (.5, 0), (-.5, 0), (0, .5), (0, -.5)])
        tree_id = add(tree, f'tree-{index}', x, y, grounded, TREE_BOUNDS[0],
                      height * 100 / TREE_BOUNDS[1], yaw)
        trunk_id = add(trunk, f'tree-{index}', x, y, z, 0, height * .32, yaw)
        manifest.append({'kind': 'tree', 'resource': 'wood', 'position_m': [x, y],
                         'visual_instance_id': tree_id, 'collision_instance_id': trunk_id})

    rng = np.random.default_rng(2602)
    # Small uneven stones and low undergrowth give the clearing detail without
    # occupying its central build space or either route exit.
    for kind, target, centres in [
        ('rock', 17, [(-31, -23), (28, -22), (-32, 29), (41, 27), (15, 31)]),
        ('shrub', 28, [(-39, 18), (-38, -18), (39, 12), (25, -35), (-12, -37)]),
    ]:
        made = 0
        for attempt in range(600):
            if made >= target:
                break
            cx, cy = centres[attempt % len(centres)]
            x = CAMP[0] + cx + float(rng.normal(0, 5.5))
            y = CAMP[1] + cy + float(rng.normal(0, 4.0))
            radius_from_camp = math.hypot(x - CAMP[0], y - CAMP[1])
            if radius_from_camp < (18 if kind == 'rock' else 25) or radius_from_camp > 62:
                continue
            z = sample(h, x, y)
            if wet(x, y, z) or sample(slope, x, y) > .35:
                continue
            height = float(rng.uniform(.22, .95) if kind == 'rock' else rng.uniform(.28, .65))
            if kind == 'rock':
                low, native_height, rx, ry = ROCK_BOUNDS
                footprint = rx * height / native_height
                if existing_rocks and cKDTree(existing_rocks).query([x, y])[0] < footprint + 1.5:
                    continue
            else:
                low, native_height = SHRUB_BOUNDS
                footprint = 1.1
            if route_tree.query([x, y])[0] < 3 + footprint:
                continue
            yaw = float(rng.uniform(0, 360))
            if kind == 'rock':
                angle = math.radians(yaw)
                support = [sample(h, x + u * math.cos(angle) - v * math.sin(angle),
                                  y + u * math.sin(angle) + v * math.cos(angle))
                           for u in (-rx * height / native_height, 0, rx * height / native_height)
                           for v in (-ry * height / native_height, 0, ry * height / native_height)]
                z = min(support) - height * .1
            actor = batch(kind, x, y)
            instance_id = add(actor, f'{kind}-{made}', x, y, z, low,
                              height * 100 / native_height, yaw)
            if kind == 'rock':
                existing_rocks.append((x, y))
                manifest.append({'kind': 'rock', 'resource': 'stone', 'position_m': [round(x, 3), round(y, 3)],
                                 'collision_instance_id': instance_id})
            made += 1
        if made != target:
            raise ValueError(f'Only placed {made} of {target} {kind} instances')

    (SOURCE / 'camp-detail-replacements.json').write_text(
        json.dumps(changes, ensure_ascii=False, separators=(',', ':')), encoding='utf-8')
    (SOURCE / 'camp-detail-manifest.json').write_text(
        json.dumps({'camp_m': CAMP, 'instances': manifest, 'interaction': 'reserved; no harvesting gameplay yet'},
                   ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'batches': len(changes), 'trees': len(TREES), 'rocks': 17,
                      'shrubs': 28, 'minimum_core_radius_m': 18}, ensure_ascii=False))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('snapshot', type=Path)
    main(parser.parse_args().snapshot)
