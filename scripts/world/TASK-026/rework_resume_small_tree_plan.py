"""Recover the unsaved small-tree staging plan after descriptor lookup failure."""
from pathlib import Path
from collections import defaultdict
import hashlib
import importlib
import json
import sys
import unreal

root=Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(Path(__file__).parent))
import rework_batches as batches
importlib.reload(batches)
from rework_contract import digest
out=root/'docs/qa/evidence/TASK-026/rework-v2/S1-small-trees-plan-03'
out.mkdir(parents=True,exist_ok=False)
ownership=json.loads((root/'docs/world/TASK-026/rework-v2/ownership.json').read_text(encoding='utf-8'))
source=root/'art_source/TASK-026/Rebuild/ReworkV2/s1-small-trees.json'
groups=defaultdict(list)
for p in json.loads(source.read_text(encoding='utf-8'))['points']:groups[tuple(p['cell'])].append(p)
live=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
added={}
for cell,items in sorted(groups.items()):
    for kind in ['smalltree','trunk']:
        label=f'S1_{kind}_{cell[0]}_{cell[1]}'
        matches=[a for a in live if a.get_actor_label()==label]
        assert len(matches)==1
        actor=matches[0];key=actor.actor_guid.to_string()
        assert key not in ownership
        package=actor.get_package().get_path_name()
        assert package.startswith('/Game/__ExternalActors__/Hearthward/World/Natural/Rebuild/')
        assert not (root/('Content/'+package[6:]+'.uasset')).exists()
        comp=actor.get_component_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
        assert comp.get_instance_count()==len(items)
        for i,p in enumerate(items):
            loc=comp.get_instance_transform(i,True).translation
            assert abs(loc.x-p['xyz_m'][0]*100)<.02 and abs(loc.y-p['xyz_m'][1]*100)<.02
        added[key]={'ownership':'Generated','cell':list(cell),'kind':kind,
            'instance_ids':[key+':'+p['id'] for p in items]}
staged,_=batches.snapshot({**ownership,**added})
original={'actors':[a for a in staged['actors'] if a['id'] in ownership],
          'non_batch_actors':staged['non_batch_actors']}
previous=json.loads((root/'docs/qa/evidence/TASK-026/rework-v2/S1-scatter/apply.json').read_text(encoding='utf-8'))['after']
assert digest(original)==digest(previous),'Existing actor semantics changed'
new=[a for a in staged['actors'] if a['id'] in added]
script=root/'scripts/world/TASK-026/rework_s1_small_trees.py'
inputs={p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in [source,script]}
folder='/Game/__ExternalObjects__/Hearthward/World/Natural/Rebuild/L_HearthwardWilds/C/4O/Q194CFVS9VCFAPJWCLVF5E'
assert not (root/('Content/'+folder[6:]+'.uasset')).exists()
materials=['/Game/Hearthward/Assets/NaturalWorld/Rebuild/Materials/M_S1_Island_'+p for p in ['trunk','branches','leaves']]
assert all(unreal.load_asset(p).get_editor_property('used_with_instanced_static_meshes') for p in materials)
plan={'scope':'new S1 small-tree/trunk batches, new folder metadata and their automatic material instancing flags','add_packages':[a['package'] for a in new]+[folder],
    'update_packages':materials,'delete_packages':[],'source_hashes':inputs,'old_semantics':digest(original),
    'added_ownership':added,'staged_actors':new,
    'staging_note':'Recovered unsaved native actors after WP descriptor lookup failure; old semantics match the saved S1-scatter baseline.'}
plan['plan_hash']=digest(plan)
save=unreal.EditorLoadingAndSavingUtils
dirty={p.get_path_name() for p in [*save.get_dirty_content_packages(),*save.get_dirty_map_packages()]}
assert dirty==set(plan['add_packages']+plan['update_packages']),str(dirty)
(out/'plan.json').write_text(json.dumps(plan,indent=2),encoding='utf-8')
