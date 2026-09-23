"""Read back pending S1 instance transforms without saving packages."""
import json
import sys
from pathlib import Path
import unreal

sys.path.insert(0, str(Path(__file__).parent))
import rework_batches as batches

plan = json.loads((batches.ROOT/'docs/qa/evidence/TASK-026/rework-v2/S1-scatter/plan.json').read_text(encoding='utf-8'))
ownership = json.loads((batches.DOC/'ownership.json').read_text(encoding='utf-8'))
for op in plan['operations']:
    ownership[op['id']]['instance_ids'] = [i['id'] for i in op['instances']]
after, _ = batches.snapshot(ownership)
actual = {a['id']: a for a in after['actors']}
differences = []
for op in plan['operations']:
    for a, e in zip(actual[op['id']]['instances'], op['instances']):
        if not batches.instances_equivalent([a], [e]):
            differences.append({'actor':op['id'], 'actual':a, 'expected':e})
out = batches.ROOT/'docs/qa/evidence/TASK-026/rework-v2/S1-scatter/transform-differences.json'
out.write_text(json.dumps(differences, indent=2), encoding='utf-8')
unreal.log('S1_TRANSFORM_DIFFERENCES '+str(len(differences)))
