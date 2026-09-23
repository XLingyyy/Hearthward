"""Place four paired visual/collision trees at the CAMP_A lake-facing edge."""
from pathlib import Path
import copy
import json
import math

from scipy.spatial import cKDTree

from prepare_camp_detail import ROOT, SOURCE, CAMP, TREE_BOUNDS, position, transform
from prepare_dressing import h, slope, wet, sample


# Two loose groups frame the water view; the centre and 30 m camp core stay open.
FRAME = [(-26, 29, 15, 22), (-17, 31, 12, 174),
         (21, 29, 15, 299), (29, 31, 11, 105)]


def main(snapshot):
    actors = json.loads(snapshot.read_text(encoding='utf-8'))['actors']
    batches = {(a['kind'], *a['cell']): a for a in actors
               if a['label'] == f"{a['kind']}_{a['cell'][0]}_{a['cell'][1]}"}
    existing = [position(a, i) for a in actors if a['kind'] in ('tree', 'smalltree')
                for i in a['instances']]
    route = json.loads((SOURCE / 's1-route.json').read_text(encoding='utf-8'))['loop']
    route_tree = cKDTree([p[:2] for p in route])
    replacements = {}
    manifest = []
    for index, (dx, dy, height, yaw) in enumerate(FRAME):
        x, y = CAMP[0] + dx, CAMP[1] + dy
        z = sample(h, x, y)
        if wet(x, y, z) or sample(slope, x, y) > .3 or route_tree.query([x, y])[0] < 3.2:
            raise ValueError(f'Unsafe frame tree {index}')
        if cKDTree(existing).query([x, y])[0] < 8:
            raise ValueError(f'Frame tree overlaps grove {index}')
        existing.append((x, y))
        cell = (math.floor(x / 252), math.floor(y / 252))
        visual = batches[('tree', *cell)]
        collision = batches[('trunk', *cell)]
        if visual['ownership'] != 'Generated' or collision['ownership'] != 'Generated':
            raise ValueError('Frame batch is protected')
        ids = []
        for actor, ground, low, scale in [
            (visual, min(sample(h, x + ox, y + oy) for ox, oy in
                         [(0, 0), (.5, 0), (-.5, 0), (0, .5), (0, -.5)]),
             TREE_BOUNDS[0], height * 100 / TREE_BOUNDS[1]),
            (collision, z, 0, height * .32),
        ]:
            instances = replacements.setdefault(actor['id'], copy.deepcopy(actor['instances']))
            instance_id = actor['id'] + f':CAMP_A_FRAME:tree-{index}'
            if any(i['id'] == instance_id for i in instances):
                raise ValueError('Frame tree already placed')
            instances.append({'id': instance_id,
                              'transform': transform(actor, x, y, ground, low, scale, yaw)})
            ids.append(instance_id)
        manifest.append({'kind': 'tree', 'resource': 'wood', 'position_m': [x, y],
                         'visual_instance_id': ids[0], 'collision_instance_id': ids[1]})
    (SOURCE / 'camp-frame-replacements.json').write_text(
        json.dumps(replacements, separators=(',', ':')), encoding='utf-8')
    (SOURCE / 'camp-frame-manifest.json').write_text(
        json.dumps({'camp_m': CAMP, 'instances': manifest,
                    'interaction': 'reserved; no harvesting gameplay yet'}, indent=2), encoding='utf-8')
    print(json.dumps({'trees': len(FRAME), 'batches': len(replacements),
                      'open_centre_half_width_m': 17}))


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('snapshot', type=Path)
    main(parser.parse_args().snapshot)
