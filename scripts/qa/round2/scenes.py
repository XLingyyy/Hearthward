"""Render real-map Round2 device fixtures, or restart their isolated saved pool."""
import argparse
import json
from pathlib import Path
import shutil
import sys
import time
import uuid

GAME = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(GAME / 'scripts/animals'))
from paths import configure_factory, ue_root
configure_factory()
from engine_adapters.ue5 import UEClient


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--reload-from', type=Path)
    parser.add_argument('--checkpoint', help='Existing save UUID from the source scene report; requires --reload-from.')
    args = parser.parse_args()
    if args.checkpoint:
        uuid.UUID(args.checkpoint)
        if not args.reload_from:
            parser.error('--checkpoint requires --reload-from')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    pool = str(uuid.uuid4())
    if args.reload_from:
        source = args.reload_from.resolve(strict=True)
        previous = json.loads((source / 'scenario.json').read_text(encoding='utf-8'))
        pool = previous['pool']
        shutil.copytree(source / 'profile/Saved/SaveGames', out / 'profile/Saved/SaveGames')
    scenario = {'fixture': True, 'pool': pool, 'reload_from': str(args.reload_from) if args.reload_from else None,
                'checkpoint': args.checkpoint,
                'scope': 'Relocated, pre-cleared real-map device fixture; no normal-route or human acceptance.'}
    (out / 'scenario.json').write_text(json.dumps(scenario, indent=2), encoding='utf-8')
    ue = UEClient(project_path=GAME / 'Hearthward.uproject', ue_root=ue_root(), port=30161, runtime_port=30162)
    launch = ue.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap', extra_args=[
        '-game', '-CoreLimit=4', '-windowed', '-ResX=1920', '-ResY=1080', '-RenderOffscreen', '-unattended', '-nosound', '-NoConsole',
        '-HearthwardRound2Reload' if args.reload_from else '-HearthwardHUDPreview',
        '-HearthwardSaveTestPool=' + pool, '-UserDir=' + str(out / 'profile'),
        '-abslog=' + str(out / 'runtime.log'), '-HearthwardRound2SceneVerify=' + str(out / 'report.json')]
        + (['-HearthwardRound2Checkpoint=' + args.checkpoint] if args.checkpoint else []))
    (out / 'launch.json').write_text(json.dumps(launch, indent=2), encoding='utf-8')
    if not launch['ok']:
        raise RuntimeError('Launch failed; see launch.json')
    try:
        deadline = time.monotonic() + 300
        while not (out / 'report.json').exists() and time.monotonic() < deadline:
            time.sleep(1)
        if not (out / 'report.json').exists():
            raise RuntimeError('Scene verification produced no report before the 300-second deadline; see runtime.log')
        report = json.loads((out / 'report.json').read_text(encoding='utf-8'))
        (out / 'result.json').write_text(json.dumps({'status': 'PASS' if report['passed'] else 'FAIL', 'report': str(out / 'report.json')}), encoding='utf-8')
        print(json.dumps(report, ensure_ascii=False), flush=True)
        return 0 if report['passed'] else 1
    except Exception as error:
        result = {'status': 'FAILED', 'error': str(error), 'report_present': (out / 'report.json').exists()}
        (out / 'result.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
        print(json.dumps(result, ensure_ascii=False), flush=True)
        return 1
    finally:
        stopped = ue.runtime.stop_editor(launch['payload']['process_id'])
        (out / 'stop.json').write_text(json.dumps(stopped, indent=2), encoding='utf-8')


if __name__ == '__main__':
    raise SystemExit(main())
