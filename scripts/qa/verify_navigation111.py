"""Render TASK-111 in a standalone new game with an isolated save pool."""
import argparse
import json
from pathlib import Path
import sys
import time
import uuid

GAME = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(GAME / 'scripts/animals'))
from paths import configure_factory, ue_root
configure_factory()
from engine_adapters.ue5 import UEClient


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    client = UEClient(project_path=GAME / 'Hearthward.uproject', ue_root=ue_root(), port=30161, runtime_port=30162)
    launch = client.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap', extra_args=[
        '-game', '-CoreLimit=4', '-windowed', '-ResX=1920', '-ResY=1080', '-RenderOffscreen', '-unattended', '-nosound',
        '-HearthwardHUDPreview', '-HearthwardSaveTestPool=' + str(uuid.uuid4()),
        '-UserDir=' + str(out / 'profile'), '-abslog=' + str(out / 'runtime.log'),
        '-HearthwardNavigationVerify=' + str(out / 'report.json')])
    (out / 'launch.json').write_text(json.dumps(launch, indent=2), encoding='utf-8')
    if not launch['ok']:
        raise RuntimeError('Launch failed; see launch.json')
    try:
        deadline = time.monotonic() + 300
        while not (out / 'report.json').exists() and time.monotonic() < deadline:
            time.sleep(1)
        report = json.loads((out / 'report.json').read_text(encoding='utf-8'))
        print(json.dumps(report, ensure_ascii=False), flush=True)
        return 0 if report['passed'] else 1
    finally:
        stop = client.runtime.stop_editor(launch['payload']['process_id'])
        (out / 'stop.json').write_text(json.dumps(stop, indent=2), encoding='utf-8')


if __name__ == '__main__':
    raise SystemExit(main())
