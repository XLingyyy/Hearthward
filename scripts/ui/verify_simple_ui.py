"""Build or check simplified storage and remaining legacy UI in the project-local test client."""
import argparse
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', action='store_true')
    parser.add_argument('--suite', choices=['simple-ui'], default='simple-ui')
    args = parser.parse_args()
    out = GAME / '.agent-local/qa/TASK-078' / args.suite / (('build_' if args.build else 'verify_') + uuid.uuid4().hex[:8])
    out.mkdir(parents=True)
    client = UEClient(project_path=GAME / 'Hearthward.uproject', ue_root=ue_root())
    if args.build:
        result = client.build.project(target='HearthwardEditor', configuration='Development', timeout=1200)
        (out / 'build.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
        print(json.dumps(dict(ok=result['ok'], diagnostics=result['diagnostics'], evidence=str(out))), flush=True)
        raise SystemExit(0 if result['ok'] else 1)
    profile = GAME / 'TestClient/Runs' / out.name
    profile.mkdir(parents=True)
    sources = [p for p in (GAME / 'Source').rglob('*') if p.is_file()]
    sources += [GAME / 'Resources/Data/gameplay.json', GAME / 'Resources/UI/interface.json', GAME / 'Resources/UI/layout.json', GAME / 'Binaries/Win64/UnrealEditor-Hearthward.dll', Path(__file__)]
    manifest = dict(head=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=GAME, text=True).strip(),
                    fingerprints={p.relative_to(GAME).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources})
    (out / 'source-manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    launch = client.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap', extra_args=[
        '-game', '-RenderOffscreen', '-unattended', '-nosound', '-CoreLimit=4', '-windowed', '-ResX=1600', '-ResY=1000',
        '-HearthwardHUDPreview', '-UserDir=' + str(profile), '-HearthwardSaveTestPool=' + str(uuid.uuid4()),
        '-HearthwardStorageVerify=' + str(out / 'report.json'), '-abslog=' + str(out / 'runtime.log')])
    (out / 'launch.json').write_text(json.dumps(launch, indent=2), encoding='utf-8')
    print(json.dumps(dict(launch_ok=launch['ok'], evidence=str(out))), flush=True)
    if not launch['ok']:
        raise SystemExit(1)
    try:
        deadline = time.monotonic() + 180
        while not (out / 'report.json').is_file() and time.monotonic() < deadline:
            log = out / 'runtime.log'
            if log.is_file() and 'LogWindows: Error: Fatal error!' in log.read_text(encoding='utf-8', errors='replace'):
                raise RuntimeError('Standalone client crashed; see ' + str(log))
            time.sleep(1)
        report = json.loads((out / 'report.json').read_text(encoding='utf-8'))
        print(json.dumps(dict(passed=report['passed'], checks=len(report['checks']),
                             failed=[k for k, v in report['checks'].items() if not v], evidence=str(out))), flush=True)
        raise SystemExit(0 if report['passed'] else 1)
    finally:
        stopped = client.runtime.stop_editor(launch['payload']['process_id'])
        (out / 'stop.json').write_text(json.dumps(stopped, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
