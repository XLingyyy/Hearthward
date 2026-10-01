"""Bind the final animal checks to the current local build and task scope."""
import hashlib
import json
from pathlib import Path
from paths import source_path

GAME = Path(__file__).resolve().parents[2]
OUT = GAME / 'docs/qa/TASK-051'


def read(name):
    return json.loads((OUT / name).read_text('utf-8'))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    runtime = read('runtime_result.json')
    binding = runtime['binding']
    changed = [name for name, digest in binding['source_sha256'].items()
               if not (GAME / name).is_file() or sha(GAME / name) != digest]
    current_build = sha(GAME / 'Binaries/Win64/UnrealEditor-Hearthward.dll') == binding['compiled_dll_sha256']
    level = read('world_report.json')['world']['payload']['world']['metadata']['level_path']
    current_map = sha(GAME / 'Content' / (level.removeprefix('/Game/') + '.umap')) == binding['map_sha256']
    changed_assets = [name for name, digest in binding.get('asset_sha256', {}).items()
                      if not (GAME / name).is_file() or sha(GAME / name) != digest]
    asset_binding = bool(binding.get('asset_sha256')) and not changed_assets
    imported = read('import_report.json')
    clips = [clip for animal in imported['animals'] for clip in animal['clips']]
    source_changes = []
    for animal in imported['animals']:
        mesh_source = source_path(animal['mesh']['source_path'])
        folder = mesh_source.parent
        expected_sources = [(mesh_source, animal['skeletal_sha256']),
                            (folder / ('AS_' + animal['slug'] + '.blend'), animal['source_blend_sha256'])]
        manifest = json.loads((folder / 'animation_manifest.json').read_text('utf-8'))
        imported_hashes = {clip['name']: clip['source_sha256'] for clip in animal['clips']}
        expected_sources += [(source_path(clip['file']), imported_hashes[clip['name']]) for clip in manifest['clips']]
        source_changes += [str(path) for path, digest in expected_sources
                           if not path.is_file() or sha(path) != digest]
    native = runtime['native']
    launch = json.loads((Path(runtime['evidence']) / 'launch.json').read_text('utf-8'))
    runtime_command = launch['payload']['command']
    failures = [check for check in native['checks'] if not check['pass']]
    screenshots = sorted((Path(runtime['evidence']) / 'native/screenshots').glob('*.png'))
    regression = read('regression_result.json')
    scope = read('scope_audit.json')
    physical = read('keyboard_check.json')
    result = {
        'head': binding['head'], 'branch': binding['branch'],
        'engine': native['engine'], 'evidence': runtime['evidence'],
        'current_source_matches_binding': not changed, 'changed_since_run': changed,
        'current_dll_matches_binding': current_build,
        'current_map_matches_binding': current_map,
        'current_animal_and_demo_assets_match_binding': asset_binding,
        'bound_assets': len(binding.get('asset_sha256', {})), 'changed_assets_since_run': changed_assets,
        'paired_original_sources_unchanged': not source_changes,
        'source_changes': source_changes,
        'imported_models': len(imported['animals']), 'imported_clips': len(clips),
        'import_checks_pass': imported['ok'] and all(animal['ok'] for animal in imported['animals']) and all(clip['ok'] for clip in clips),
        'maximum_clip_duration_difference_seconds': max(abs(clip['duration'] - clip['native_duration']) for clip in clips),
        'runtime_checks': len(native['checks']), 'runtime_passed': len(native['checks']) - len(failures),
        'runtime_failures': failures, 'runtime_screenshots': len(screenshots),
        'measured_player_sprint_cm_s': native.get('measured_player_sprint_cm_s'),
        'measured_player_skilled_sprint_cm_s': native.get('measured_player_skilled_sprint_cm_s'),
        'rendered_offscreen': '-RenderOffScreen' in runtime_command,
        'fixed_time_step': '-UseFixedTimeStep' in runtime_command,
        'runtime_command': runtime_command,
        'regression_found': regression['payload']['tests_found'],
        'regression_passed': regression['payload']['tests_passed'],
        'regression_failed': regression['payload']['tests_failed'],
        'automation_startup_diagnostics': regression['diagnostics'],
        'physical_keyboard_checks_pass': physical['pass'],
        'local_scope_audit_pass': scope['ok'],
        'prior_files_preserved': sum(row['unchanged'] for row in scope['prior_fingerprints']),
        'official_baseline_path_check': 'Cannot pass: TASK-051 has no approved snapshot in the real baseline commit; no commit was authorized or made.',
        'owner_visual_acceptance': 'Pending owner playthrough; implementation and automated checks do not claim human acceptance.'
    }
    result['ok'] = all([not changed, current_build, current_map, asset_binding, not source_changes, result['import_checks_pass'],
                        len(imported['animals']) == 14, len(clips) == 303, native['pass'],
                        len(native['checks']) == 199, not failures, len(screenshots) == 42,
                        regression['ok'], result['regression_found'] == 6,
                        result['regression_passed'] == 6, result['regression_failed'] == 0,
                        physical['pass'], scope['ok']])
    (OUT / 'verification_summary.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print('FINAL_ANIMAL_VERIFICATION', result['ok'], 'models', result['imported_models'],
          'clips', result['imported_clips'], 'runtime', result['runtime_passed'],
          'regression', result['regression_passed'], 'screenshots', result['runtime_screenshots'])
    if not result['ok']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
