"""Create the two approved query bodies with Unreal's PhysicsAssetFactory."""
import json
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
selected_avatar = 'Brother' if 'Task055Avatar=Brother' in unreal.SystemLibrary.get_command_line() else 'Hero'
out = Path(unreal.Paths.project_dir()) / ('Saved/Task055/physics-' + selected_avatar.lower())
out.mkdir(parents=True, exist_ok=True)
report = {'ok': False, 'assets': {}, 'scope': 'real skeletal physics asset generation'}
def create_assets(delta):
    unreal.unregister_slate_post_tick_callback(handle)
    try:
        unreal.EditorAssetLibrary.make_directory('/Game/Hearthward/Assets/TASK-055/Physics')
        for avatar in [selected_avatar]:
            mesh_path = f'/Game/Characters/{avatar}/UE5/SK_{avatar}'
            asset_path = f'/Game/Hearthward/Assets/TASK-055/Physics/PA_{avatar}Combat'
            mesh = unreal.load_asset(mesh_path)
            if mesh is None:
                raise RuntimeError('Missing actual mesh: ' + mesh_path)
            asset = unreal.load_asset(asset_path)
            # Replace only the owned generated candidate whose part rays failed.
            if asset is not None:
                if not unreal.EditorAssetLibrary.delete_asset(asset_path):
                    raise RuntimeError('Failed to replace generated candidate: ' + avatar)
                asset = None
            if asset is None:
                factory = unreal.PhysicsAssetFactory()
                asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset_with_dialog(
                    f'PA_{avatar}Combat', '/Game/Hearthward/Assets/TASK-055/Physics',
                    unreal.PhysicsAsset, factory)
            if asset is None:
                raise RuntimeError('Physics asset creation cancelled: ' + avatar)
            mesh.set_editor_property('physics_asset', asset)
            report['assets'][avatar] = {'mesh': mesh.get_path_name(), 'asset': asset.get_path_name(),
                                        'body_validation': 'pending real native projectile queries'}
            if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
                raise RuntimeError('Failed to save physics asset: ' + avatar)
            if not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
                raise RuntimeError('Failed to save actual mesh binding: ' + avatar)
        report['ok'] = True
    except Exception:
        report['exception'] = traceback.format_exc()
    finally:
        (out / 'results.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        unreal.log(json.dumps(report, ensure_ascii=False))

handle = unreal.register_slate_post_tick_callback(create_assets)
