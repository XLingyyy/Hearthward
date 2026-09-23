"""Place paired conifer/trunk instances on the CAMP_A forest edge."""
from pathlib import Path
import argparse
import copy
import json
import math

import numpy as np
from scipy.spatial import cKDTree

from prepare_camp_detail import ROOT, SOURCE, CAMP, position, transform
from prepare_dressing import h, slope, wet, sample


NATIVE_HEIGHT_CM = {
    'SM_CampFirA_CampUV': 1895.46,
    'SM_CampFirC_CampUV': 1454.58,
    'SM_CampPineA': 2039.68,
    'SM_CampPineC': 1755.07,
}
# Relative to CAMP_A. The lakeward middle stays open while the two shoulders
# and the inland edge gain a visibly clustered tree line.
CLUSTERS = [
    (-52, 43, 8), (56, 44, 8),
    (-76, 8, 7), (78, 2, 7),
    (-67, -48, 5), (65, -50, 5),
    (-24, -74, 4), (30, -76, 4),
]


def main(snapshot_path):
    actors = json.loads(snapshot_path.read_text(encoding='utf-8'))['actors']
    batches = {(a['kind'], *a['cell']): a for a in actors
               if a['label'] == f"{a['kind']}_{a['cell'][0]}_{a['cell'][1]}"}
    existing_trees = [position(a, i) for a in actors if a['kind'] in ('tree', 'smalltree')
                      for i in a['instances']]
    existing_rocks = [position(a, i) for a in actors if a['kind'] == 'rock'
                      for i in a['instances']]
    route = json.loads((SOURCE / 's1-route.json').read_text(encoding='utf-8'))['loop']
    route_tree = cKDTree([p[:2] for p in route])
    rock_tree = cKDTree(existing_rocks)
    replacements = {}
    manifest = []
    rng = np.random.default_rng(2626)

    for cluster_index, (cx, cy, count) in enumerate(CLUSTERS):
        placed = 0
        for _ in range(1600):
            if placed == count:
                break
            dx = cx + float(rng.normal(0, 13))
            dy = cy + float(rng.normal(0, 12))
            radius = math.hypot(dx, dy)
            if radius < 34 or radius > 112 or (dy > 15 and abs(dx) < 19):
                continue
            x, y = CAMP[0] + dx, CAMP[1] + dy
            z = sample(h, x, y)
            if wet(x, y, z) or sample(slope, x, y) > .3:
                continue
            if route_tree.query([x, y])[0] < 4.5:
                continue
            if cKDTree(existing_trees).query([x, y])[0] < 7:
                continue
            if rock_tree.query([x, y])[0] < 3:
                continue
            cell = (math.floor(x / 252), math.floor(y / 252))
            visual = batches.get(('tree', *cell))
            collision = batches.get(('trunk', *cell))
            if not visual or not collision or visual['ownership'] != 'Generated' or collision['ownership'] != 'Generated':
                continue
            name = visual['mesh'].split('.')[-1]
            native_height = NATIVE_HEIGHT_CM.get(name)
            if native_height is None or visual['collision'] != 'NoCollision' or collision['collision'] != 'BlockAll':
                raise ValueError(f'Unexpected conifer batch at {cell}: {name}')

            height_m = float(rng.uniform(11.5, 18.5))
            yaw = float(rng.uniform(0, 360))
            ground = min(sample(h, x + ox, y + oy) for ox, oy in
                         [(0, 0), (1.5, 0), (-1.5, 0), (0, 1.5), (0, -1.5)])
            token = f'grove-{cluster_index}-{placed}'
            ids = []
            for actor, elevation, scale in [
                (visual, ground, height_m * 100 / native_height),
                (collision, z, height_m * .32),
            ]:
                instance_id = actor['id'] + ':CAMP_A_CONIFER:' + token
                instances = replacements.setdefault(actor['id'], copy.deepcopy(actor['instances']))
                assert all(i['id'] != instance_id for i in instances)
                instances.append({'id': instance_id,
                                  'transform': transform(actor, x, y, elevation, 0, scale, yaw)})
                ids.append(instance_id)
            existing_trees.append((x, y))
            manifest.append({
                'kind': 'tree', 'resource': 'wood', 'species': 'fir' if 'Fir' in name else 'pine',
                'height_m': round(height_m, 2), 'position_m': [round(x, 3), round(y, 3)],
                'visual_instance_id': ids[0], 'collision_instance_id': ids[1],
            })
            placed += 1
        if placed != count:
            raise ValueError(f'Only placed {placed} of {count} in cluster {cluster_index}')

    (SOURCE / 'camp-conifer-grove-replacements.json').write_text(
        json.dumps(replacements, ensure_ascii=False, separators=(',', ':')), encoding='utf-8')
    (SOURCE / 'camp-conifer-grove-manifest.json').write_text(
        json.dumps({'camp_m': CAMP, 'open_core_radius_m': 34,
                    'lakeward_open_half_width_m': 19,
                    'instances': manifest,
                    'interaction': 'reserved; no harvesting gameplay yet'},
                   ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'trees': len(manifest), 'fir': sum(i['species'] == 'fir' for i in manifest),
                      'pine': sum(i['species'] == 'pine' for i in manifest),
                      'batches': len(replacements)}, ensure_ascii=False))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('snapshot', type=Path)
    main(parser.parse_args().snapshot)
