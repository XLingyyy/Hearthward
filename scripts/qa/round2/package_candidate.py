"""Build a local Shipping candidate without publishing or changing the release version."""
import argparse
import json
from pathlib import Path
import shutil
import sys

GAME = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(GAME / 'scripts/animals'))
from paths import configure_factory, ue_root
configure_factory()
from engine_adapters.ue5 import UEClient
from metrics import identity


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--stage', type=Path, help='Optional staging directory on another drive when archive space is limited.')
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    stage = args.stage.resolve() if args.stage else out / 'stage'
    metadata = {'candidate': out.name, 'configuration': 'Win64 Shipping', 'engine': '5.8.2',
                'status': 'EXPERIMENTAL_NOT_ACCEPTED', 'source': identity(),
                'limitations': 'Performance remains below the frozen threshold. Complete route, scene matrix, owner audiovisual acceptance and external human/device samples are not signed off.',
                'assets': json.loads((GAME / 'docs/qa/TASK-133/ASSET_REGISTER.json').read_text(encoding='utf-8'))}
    (out / 'build-info.json').write_text(json.dumps(metadata, ensure_ascii=False, indent=2), encoding='utf-8')
    ue = UEClient(project_path=GAME / 'Hearthward.uproject', ue_root=ue_root())
    result = ue.build.package(archive_dir=out / 'archive', log_path=out / 'package.log', configuration='Shipping',
        maps=('/Game/Hearthward/Bootstrap/L_Bootstrap', '/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'),
        extra_args=('-stagingdirectory=' + str(stage), '-nodebuginfo'), timeout=10800)
    (out / 'package-result.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    if result['ok']:
        archive = out / 'archive/Windows'
        shutil.copy2(out / 'build-info.json', archive / 'BUILD-INFO.json')
        shutil.copy2(GAME / 'docs/qa/TASK-133/PLAYTEST_GUIDE.md', archive / 'PLAYTEST_GUIDE.md')
        shutil.copy2(GAME / 'docs/qa/TASK-133/ASSET_REGISTER.json', archive / 'ASSET_REGISTER.json')
        (archive / 'VERIFICATION.md').write_text(
            '# Local experimental candidate\n\nBuild/Cook/Stage/Archive completed successfully. '
            'See BUILD-INFO.json for the build source snapshot and ASSET_REGISTER.json for asset provenance. '
            'Build success does not imply gameplay acceptance.\n\n' + metadata['limitations'] + '\n', encoding='utf-8')
        for backend in ('cpu', 'vulkan'):
            command = '@echo off\r\ncd /d "%~dp0"\r\nstart "" "Hearthward.exe" -HearthwardAIBackend=' + backend + ' -HearthwardAIGpuLayers=16 -UserDir="%~dp0CandidateProfile"\r\n'
            (archive / ('start-' + backend + '.cmd')).write_bytes(command.encode('ascii'))
    print(json.dumps({'ok': result['ok'], 'output': str(out), 'status': metadata['status']}, ensure_ascii=False), flush=True)
    return 0 if result['ok'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
