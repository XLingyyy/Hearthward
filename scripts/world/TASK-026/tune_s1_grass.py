"""Apply the single-asset S1 density revision after fixed-view review."""
from pathlib import Path
import json
import shutil
import stat
import time
import unreal

root=Path(unreal.Paths.project_dir())
out=root/'docs/qa/evidence/TASK-026/rework-v2/S1-grass-density-02'
plan=json.loads((out/'plan.json').read_text(encoding='utf-8'))
assert not (out/'apply.json').exists()
package=plan['update_packages'][0]
file=root/('Content/'+package[6:]+'.uasset')
locksfile=root/'Saved/Task026/rework-v2-locks.json'
assert time.time()-locksfile.stat().st_mtime<1800
locks=json.loads(locksfile.read_text(encoding='utf-8'))
locks=locks if isinstance(locks,list) else locks['locks']
assert file.relative_to(root).as_posix() in {l['path'] for l in locks if l['owner']['name']=='XLingyyy'}
save=unreal.EditorLoadingAndSavingUtils
assert not save.get_dirty_content_packages() and not save.get_dirty_map_packages()
gt=unreal.load_asset(package)
varieties=list(gt.get_editor_property('grass_varieties'))
assert len(varieties)==1
v=varieties[0]
assert abs(v.grass_density.default-plan['before_density'])<.01
backup=root/'Saved/Task026/ReworkV2/backups/S1-grass-density-02/GT_S1_Meadow.uasset'
backup.parent.mkdir(parents=True,exist_ok=True)
assert not backup.exists()
shutil.copy2(file,backup);file.chmod(file.stat().st_mode|stat.S_IWRITE)
gt.modify()
v.set_editor_property('grass_density',unreal.PerPlatformFloat(default=plan['after_density']))
gt.set_editor_property('grass_varieties',varieties)
assert unreal.EditorAssetLibrary.save_asset(package,only_if_is_dirty=True)
(out/'apply.json').write_text(json.dumps({'status':'SAVED_PENDING_VISUAL_REVIEW',
    'plan':plan,'varieties':[str(v) for v in gt.get_editor_property('grass_varieties')]}),encoding='utf-8')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(world,'grass.FlushCachePIE')
