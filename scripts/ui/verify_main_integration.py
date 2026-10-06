"""Verify the integrated map/UI and main companion/dialogue changes in isolated UE fixtures."""
import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import uuid

if __name__ == '__main__' and not sys.flags.utf8_mode:
    os.execv(sys.executable, [sys.executable, '-X', 'utf8', __file__, *sys.argv[1:]])

GAME = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(GAME / 'scripts/animals'))
from paths import configure_factory, ue_root
configure_factory()
from engine_adapters.ue5 import UEClient

SCRIPTS = {
    'dialogue-final': ('docs/qa/TASK-081/verify_final.py', '.agent-local/qa/TASK-081/final-pie', '/Game/Hearthward/Bootstrap/L_Bootstrap'),
    'dialogue-visual': ('docs/qa/TASK-081/verify_visual.py', '.agent-local/qa/TASK-081/visual', '/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'),
    'quest-guidance': ('docs/qa/TASK-080/verify_quest_guidance_pie.py', '.agent-local/qa/TASK-080', '/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'),
}
NATIVE_FILTER = 'Hearthward.Inventory.Storage+Hearthward.Map078+Hearthward.Companion078+Hearthward.Companion079+Hearthward.Quest080+Hearthward.Dialogue081'
NATIVE_TESTS = {
    'Hearthward.Inventory.Storage.AtomicTransfer', 'Hearthward.Inventory.Storage.ReplayAndEpoch',
    'Hearthward.Map078.LocationKindsAndTravelFeedback', 'Hearthward.Map078.RectangularDragAndRelease',
    'Hearthward.Companion078.GatheringDepletionAndResume', 'Hearthward.Companion078.InspectSavedGathering',
    'Hearthward.Companion079.TaskContext', 'Hearthward.Companion079.TaskContract',
    'Hearthward.Quest080.MarkerPlacement', 'Hearthward.Dialogue081.Contract', 'Hearthward.Dialogue081.WorkParty',
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('suite', choices=['native', *SCRIPTS])
    args = parser.parse_args()
    out = GAME / '.agent-local/qa/TASK-082/integration' / (args.suite + '-' + uuid.uuid4().hex[:8])
    out.mkdir(parents=True)
    profile = GAME / 'TestClient/Runs' / out.name
    profile.mkdir(parents=True)
    sources = [p for p in (GAME / 'Source').rglob('*') if p.is_file()]
    sources += [GAME / p for p in ('Resources/Data/gameplay.json', 'Resources/UI/interface.json', 'Resources/UI/layout.json',
        'Resources/UI/Art/map-local-terrain.png', 'Binaries/Win64/UnrealEditor-Hearthward.dll', 'scripts/ui/verify_main_integration.py')]
    manifest = {'head': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=GAME, text=True).strip(),
        'fingerprints': {p.relative_to(GAME).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}}
    (out / 'source-manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    client = UEClient(project_path=GAME / 'Hearthward.uproject', ue_root=ue_root())
    flags = ['-RenderOffscreen', '-unattended', '-nosound', '-CoreLimit=4', '-UserDir=' + str(profile),
        '-HearthwardSaveTestPool=' + str(uuid.uuid4())]
    if args.suite == 'native':
        result = client.testing.run_automation_tests(NATIVE_FILTER, report_dir=str(out), extra_args=flags, timeout=300)
        (out / 'result.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
        index = json.loads((out / 'index.json').read_text('utf-8-sig'))
        actual = {test['fullTestPath'] for test in index['tests']}
        complete = actual == NATIVE_TESTS
        print(json.dumps({'ok': result['ok'] and complete, 'evidence': str(out), 'expected_test_set_matches': complete,
            'missing': sorted(NATIVE_TESTS - actual), 'unexpected': sorted(actual - NATIVE_TESTS),
            'counts': {k:v for k,v in result['payload'].items() if k.startswith('tests_')}}, ensure_ascii=False), flush=True)
        raise SystemExit(0 if result['ok'] and complete else 1)
    original, old_output, level = SCRIPTS[args.suite]
    source_path = GAME / original
    source = source_path.read_text('utf-8')
    assignments = [node for node in ast.parse(source).body if isinstance(node, ast.Assign)
        and any(isinstance(target, ast.Name) and target.id == 'out' for target in node.targets)]
    if len(assignments) != 1 or old_output not in ast.get_source_segment(source, assignments[0].value):
        raise RuntimeError('Expected unique original output directory in ' + original)
    value = assignments[0].value
    lines = source.splitlines(keepends=True)
    start = sum(map(len, lines[:value.lineno - 1])) + value.col_offset
    end = sum(map(len, lines[:value.end_lineno - 1])) + value.end_col_offset
    adapted = source[:start] + 'Path(' + repr(out.as_posix()) + ')' + source[end:]
    script = out / 'verify_pie.py'
    script.write_text(adapted, encoding='utf-8')
    (out / 'script-adaptation.json').write_text(json.dumps({'original': original,
        'original_sha256': hashlib.sha256(source_path.read_bytes()).hexdigest(),
        'adapted_sha256': hashlib.sha256(script.read_bytes()).hexdigest(),
        'change': 'Only redirect output to a unique TASK-082 run; original assertions and fixtures preserved',
        'launch_map': level,
        'map_note': 'Natural suites launch the current L_HearthwardWilds map required by EnableNaturalWorld; avoids stale objects after travel from the old L_NaturalWorld'}, indent=2), encoding='utf-8')
    launch = client.runtime.launch_editor(map_path=level, extra_args=flags + ['-ExecutePythonScript=' + str(script),
        '-HearthwardAIBackend=vulkan', '-HearthwardAIGpuLayers=32', '-abslog=' + str(out / 'runtime.log')])
    (out / 'launch.json').write_text(json.dumps(launch, indent=2), encoding='utf-8')
    print(json.dumps({'launch_ok': launch['ok'], 'evidence': str(out)}), flush=True)
    if not launch['ok']:
        raise SystemExit(1)
    try:
        deadline = time.monotonic() + (960 if args.suite == 'dialogue-final' else 360)
        result_path = out / 'results.json'
        while not result_path.is_file() and time.monotonic() < deadline:
            log = out / 'runtime.log'
            if log.is_file() and 'LogWindows: Error: Fatal error!' in log.read_text('utf-8', errors='replace'):
                raise RuntimeError('UE crashed; see ' + str(log))
            time.sleep(1)
        if not result_path.is_file():
            raise TimeoutError('Fresh PIE report was not written: ' + str(out))
        result = json.loads(result_path.read_text('utf-8'))
        print(json.dumps({'ok': result['ok'], 'checks': len(result['checks']),
            'failed': [k for k,v in result['checks'].items() if not v], 'evidence': str(out)}, ensure_ascii=False), flush=True)
        raise SystemExit(0 if result['ok'] else 1)
    finally:
        stopped = client.runtime.stop_editor(launch['payload']['process_id'])
        (out / 'stop.json').write_text(json.dumps(stopped, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
