"""Finish saving the four S1 packages after the diagnostic game released its files."""
from pathlib import Path
import json
import time
import unreal

root=Path(unreal.Paths.project_dir())
plan=json.loads((root/'docs/qa/evidence/TASK-026/rework-v2/S1-grass-plan-03/plan.json').read_text(encoding='utf-8'))
out=root/'docs/qa/evidence/TASK-026/rework-v2/S1-grass-save-retry'
out.mkdir(exist_ok=False)
lockfile=root/'Saved/Task026/rework-v2-locks.json'
assert time.time()-lockfile.stat().st_mtime<1800
locks=json.loads(lockfile.read_text(encoding='utf-8'))
locks=locks if isinstance(locks,list) else locks['locks']
own={x['path'] for x in locks if x['owner']['name']=='XLingyyy'}
packages=plan['add_packages']+plan['update_packages']
assert all('Content/'+p[6:]+'.uasset' in own for p in packages)
save=unreal.EditorLoadingAndSavingUtils
dirty={p.get_path_name() for p in [*save.get_dirty_content_packages(),*save.get_dirty_map_packages()]}
assert dirty==set(packages),sorted(dirty)
mat=unreal.load_asset(plan['update_packages'][0])
nodes=[n for n in unreal.ObjectIterator(unreal.MaterialExpression) if n.get_outer()==mat]
assert set(map(tuple,plan['before']['nodes'])).issubset({(n.get_name(),n.get_class().get_name()) for n in nodes})
assert len(nodes)==len(plan['before']['nodes'])+4
output=next(n for n in nodes if isinstance(n,unreal.MaterialExpressionLandscapeGrassOutput))
assert [str(g.get_editor_property('name')) for g in output.get_editor_property('grass_types')]==['Meadow','S1_Meadow']
for p in packages:
    assert unreal.EditorAssetLibrary.save_asset(p,only_if_is_dirty=True),p
(out/'apply.json').write_text(json.dumps({'status':'SAVED_PENDING_VISUAL_REVIEW','plan_hash':plan['plan_hash'],
    'saved_packages':packages,'recovered_from':'S1-grass-apply-02/failure.json',
    'old_material_nodes_preserved':True,'height_changes':False,'actor_changes':False},indent=2),encoding='utf-8')
