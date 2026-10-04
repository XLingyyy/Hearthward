"""Build or verify the project-local standalone Development test client."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
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
    parser.add_argument('--build', action='store_true')
    parser.add_argument('--label', default='verification')
    args = parser.parse_args()
    if not args.label.replace('_', '').replace('-', '').isalnum():
        parser.error('Use a simple evidence label')
    out = GAME / 'docs/qa/TASK-053/input-lifecycle-fix' / (args.label + '_' + uuid.uuid4().hex[:8])
    out.mkdir(parents=True)
    client = UEClient(project_path=GAME / 'Hearthward.uproject', ue_root=ue_root())
    if args.build:
        result = client.build.project(target='HearthwardEditor', configuration='Development', timeout=1200)
        (out / 'build.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
        print(json.dumps({'build_ok': result['ok'], 'evidence': str(out), 'result': result}, ensure_ascii=False), flush=True)
        raise SystemExit(0 if result['ok'] else 1)
    profile = GAME / 'TestClient/Runs' / out.name
    profile.mkdir(parents=True)
    prefs = {p: p.read_bytes() for p in (GAME / 'Saved/Config').glob('*/GameUserSettings.ini')}
    sources = [p for p in (GAME / 'Source').rglob('*') if p.is_file()]
    sources += [GAME / 'Binaries/Win64/UnrealEditor-Hearthward.dll', Path(__file__)]
    metadata = dict(head=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=GAME, text=True).strip(),
                    fingerprints={p.relative_to(GAME).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources})
    (out / 'source-manifest.json').write_text(json.dumps(metadata, indent=2), encoding='utf-8')
    launch = client.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap', extra_args=[
        '-game', '-RenderOffscreen', '-unattended', '-nosound', '-CoreLimit=4', '-windowed', '-ResX=1600', '-ResY=1000',
        '-UserDir=' + str(profile),
        '-HearthwardSaveTestPool=' + str(uuid.uuid4()), '-HearthwardInputVerify=' + str(out / 'report.json'),
        '-abslog=' + str(out / 'runtime.log')])
    (out / 'launch.json').write_text(json.dumps(launch, indent=2), encoding='utf-8')
    print(json.dumps({'launch': launch, 'evidence': str(out)}, ensure_ascii=False), flush=True)
    if not launch['ok']:
        raise SystemExit(1)
    try:
        deadline = time.monotonic() + 300
        while not (out / 'report.json').is_file() and time.monotonic() < deadline:
            time.sleep(1)
        if not (out / 'report.json').is_file():
            raise TimeoutError('Standalone input client did not complete; preserve runtime log')
        report = json.loads((out / 'report.json').read_text(encoding='utf-8'))
        print(json.dumps({'passed': report['passed'], 'checks': len(report['checks']),
                          'failed': [k for k, v in report['checks'].items() if not v], 'evidence': str(out)}, ensure_ascii=False), flush=True)
        raise SystemExit(0 if report['passed'] else 1)
    finally:
        stopped = client.runtime.stop_editor(launch['payload']['process_id'])
        (out / 'stop.json').write_text(json.dumps(stopped, indent=2), encoding='utf-8')
        for p in set(prefs) | set((GAME / 'Saved/Config').glob('*/GameUserSettings.ini')):
            if p in prefs:
                p.write_bytes(prefs[p])
            else:
                p.unlink()


if __name__ == '__main__':
    main()
