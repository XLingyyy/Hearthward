"""Run the same UEClient build/regression gate locally and on a prepared Windows runner."""
import argparse
import json
import subprocess
import sys
import uuid
from pathlib import Path

GAME = Path(__file__).resolve().parents[2]
DEFAULT_TESTS = 'Hearthward.Engineering.Task105.+Hearthward.Content.M1.+Hearthward.Content.M2.+Hearthward.Content.M3.+Hearthward.Save.'


def save(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def check_report(path):
    report = json.loads(path.read_text(encoding='utf-8-sig'))
    tests = report.get('tests', [])
    if not tests or any(test.get('state') not in ('Success', 'SuccessWithWarnings') for test in tests):
        raise RuntimeError('Automation report has no tests or contains an incomplete/failed test: ' + str(path))
    return {'passed': len(tests), 'warnings': report.get('succeededWithWarnings', 0)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--tests', default=DEFAULT_TESTS)
    parser.add_argument('--render', action='store_true', help='Use offscreen rendering for UI tests.')
    parser.add_argument('--package', action='store_true', help='Also build/cook/stage/archive Shipping.')
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    summary = {'ok': False, 'project': str(GAME / 'Hearthward.uproject'), 'tests': args.tests,
               'rendered': args.render, 'shipping_requested': args.package}
    try:
        summary['commit'] = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=GAME, text=True).strip()
        summary['working_tree'] = subprocess.check_output(['git', '-c', 'core.quotepath=false', 'status', '--porcelain'], cwd=GAME, encoding='utf-8')
        sys.path.insert(0, str(GAME / 'scripts/animals'))
        from paths import configure_factory, ue_root
        configure_factory()
        from engine_adapters.ue5 import UEClient
        engine = ue_root()
        summary['engine_root'] = str(engine)
        ue = UEClient(project_path=GAME / 'Hearthward.uproject', ue_root=engine)
        build = ue.build.project(target='HearthwardEditor', configuration='Development', timeout=1200)
        save(out / 'build.json', build)
        if not build['ok']:
            raise RuntimeError('Editor build failed; see build.json')
        result = ue.testing.run_automation_tests(args.tests, report_dir=str(out / 'native'),
            extra_args=['-unattended', '-RenderOffscreen' if args.render else '-NullRHI', '-nosound', '-CoreLimit=4',
                        '-UserDir=' + str(out / 'profile'), '-HearthwardSaveTestPool=' + str(uuid.uuid4())], timeout=900)
        save(out / 'native-result.json', result)
        if not result['ok']:
            raise RuntimeError('UE automation failed; see native-result.json')
        summary['automation'] = check_report(out / 'native/index.json')
        if args.package:
            package = ue.build.package(configuration='Shipping', archive_dir=str(out / 'shipping'),
                                       maps=('/Game/Hearthward/Bootstrap/L_Bootstrap',
                                             '/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'),
                                       extra_args=('-stagingdirectory=' + str(out / 'stage'), '-nodebuginfo'),
                                       log_path=str(out / 'shipping.log'), timeout=3600)
            save(out / 'shipping.json', package)
            if not package['ok']:
                raise RuntimeError('Shipping build/cook failed; see shipping.json')
        summary['ok'] = True
    except Exception as error:
        summary['error'] = str(error)
    finally:
        save(out / 'summary.json', summary)
        print(json.dumps(summary, ensure_ascii=False), flush=True)
    return 0 if summary['ok'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
