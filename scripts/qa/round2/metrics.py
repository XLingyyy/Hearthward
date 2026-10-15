"""Round2 evidence ingestion; reuse TASK-102's full-frame parser and timing definitions."""
import argparse
import importlib.util
import json
import subprocess
from pathlib import Path

GAME = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('hearthward_joint_metrics', GAME / 'docs/qa/TASK-102/run_standalone_joint.py')
legacy = importlib.util.module_from_spec(spec)
spec.loader.exec_module(legacy)

SCENES = ('camp_day', 'camp_night', 'combined_combat', 'cooperation_streaming', 'camp_management', 'rain_stealth')
BACKENDS = ('cpu', 'vulkan')
THRESHOLDS = {'p99_ms_max': 16.67, 'one_percent_low_fps_min': 60, 'over_50ms_fraction_max': .001}


def identity():
    def git(*args):
        return subprocess.check_output(['git', *args], cwd=GAME, encoding='utf-8')
    # Retain the actual patch and untracked source contents, not just a dirty boolean.
    untracked = git('ls-files', '--others', '--exclude-standard', 'Source', 'scripts', 'Resources', 'Config').splitlines()
    return {'commit': git('rev-parse', 'HEAD').strip(), 'branch': git('branch', '--show-current').strip(),
            'patch': git('diff', 'HEAD', '--', 'Source', 'scripts', 'Resources', 'Config', 'Hearthward.uproject',
                         'docs/qa/TASK-102/run_standalone_joint.py'),
            'untracked_text': {name: (GAME / name).read_text(encoding='utf-8') for name in untracked
                               if Path(name).suffix in ('.h', '.cpp', '.inl', '.cs', '.py', '.json', '.ini')}}


def summarize(csv_path, log_path):
    timings, metadata = legacy.parse_frame_timings(Path(csv_path).read_text(encoding='utf-8-sig'))
    frames = legacy.frame_statistics(timings)
    model = legacy.analyze_model_log(Path(log_path).read_text(encoding='utf-8-sig'))
    gates = {'p99': frames['p99_ms'] <= THRESHOLDS['p99_ms_max'],
             'one_percent_low': frames['one_percent_low_fps'] >= THRESHOLDS['one_percent_low_fps_min'],
             'long_frames': frames['over_50ms_fraction'] <= THRESHOLDS['over_50ms_fraction_max']}
    # Transport completion alone never proves visible feedback or semantic success.
    overlap = model['capture_contains_whole_observed_request'] is True
    return {'frames': frames, 'csv_metadata': metadata, 'thresholds': THRESHOLDS, 'frame_gates': gates,
            'frame_status': 'PASS' if all(gates.values()) else 'FAIL', 'model': model,
            'joint_status': 'NOT_RUN' if not overlap else 'NEEDS_UI_AND_EXECUTION_EVIDENCE',
            'dropped_frames': 0, 'frame_exclusions': []}


def matrix(evidence_root):
    """Each slot must supply its own recorded identity; never stamp historical frames as today's build."""
    root = Path(evidence_root)
    results = []
    for scene in SCENES:
        for backend in BACKENDS:
            directory = root / scene / backend
            entry = {'scene': scene, 'backend': backend, 'status': 'NOT_RUN'}
            required = ('identity.json', 'configuration.json', 'scene.json', 'frames.csv', 'runtime.log')
            missing = [name for name in required if not (directory / name).is_file()]
            if missing:
                entry['missing'] = missing
            else:
                try:
                    run_identity = json.loads((directory / 'identity.json').read_text(encoding='utf-8'))
                    config = json.loads((directory / 'configuration.json').read_text(encoding='utf-8'))
                    route = json.loads((directory / 'scene.json').read_text(encoding='utf-8'))
                    if not run_identity.get('commit') or 'patch' not in run_identity or 'untracked_text' not in run_identity:
                        raise ValueError('Missing exact source identity')
                    if config.get('backend') != backend or not all(config.get(key) for key in ('hardware', 'engine', 'resolution', 'quality', 'model')):
                        raise ValueError('Missing/mismatched frozen configuration')
                    if route.get('scene') != scene or not route.get('checkpoint') or not route.get('operations'):
                        raise ValueError('Missing actual scene route/checkpoint')
                    entry.update(summarize(directory / 'frames.csv', directory / 'runtime.log'))
                    entry['identity'] = run_identity
                    entry['configuration'] = config
                    entry['route'] = route
                    entry['status'] = 'FAIL' if entry['frame_status'] == 'FAIL' else 'MEASURED'
                except (ValueError, OSError) as error:
                    entry.update(status='INVALID_EVIDENCE', error=str(error))
            results.append(entry)
    return {'slots': results, 'final_acceptance': 'NOT_RUN; TASK-133 owns complete scene and human acceptance'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--evidence-root', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--identity-only', action='store_true')
    args = parser.parse_args()
    if not args.identity_only and args.evidence_root is None:
        parser.error('--evidence-root is required for a matrix')
    value = identity() if args.identity_only else matrix(args.evidence_root)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open('x', encoding='utf-8') as stream:
        json.dump(value, stream, ensure_ascii=False, indent=2)
        stream.write('\n')


if __name__ == '__main__':
    main()
