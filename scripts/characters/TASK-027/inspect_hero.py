"""Read existing Hero assets in the editor without saving packages."""
import json
from pathlib import Path
import unreal

report = {}
root = '/Game/Characters/Hero/Tripo'
for path in unreal.EditorAssetLibrary.list_assets(root, recursive=True):
    asset = unreal.load_asset(path)
    info = {'class': asset.get_class().get_name()}
    if isinstance(asset, unreal.AnimSequence):
        info['length'] = asset.get_play_length()
        info['skeleton'] = asset.get_editor_property('skeleton').get_path_name()
        info['frames'] = unreal.AnimationLibrary.get_num_frames(asset)
        info['bones'] = [str(x) for x in unreal.AnimationLibrary.get_animation_track_names(asset)]
    if isinstance(asset, unreal.SkeletalMesh):
        info['skeleton'] = asset.get_editor_property('skeleton').get_path_name()
        bounds = asset.get_bounds()
        info['bounds'] = {'origin': str(bounds.origin), 'extent': str(bounds.box_extent)}
        info['materials'] = [str(m.material_interface) for m in asset.get_editor_property('materials')]
    report[path] = info
out = Path(unreal.Paths.project_saved_dir()) / 'Task027/asset-inspection.json'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.SystemLibrary.quit_editor()
