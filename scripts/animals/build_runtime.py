"""Native project build and focused execution-authority automation through UEClient."""
import sys,json,argparse,datetime
from pathlib import Path
GAME=Path(__file__).resolve().parents[2]
from paths import configure_factory,ue_root
configure_factory()
from engine_adapters.ue5 import UEClient
OUT=GAME/'docs/qa/TASK-051'
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--tests',action='store_true');parser.add_argument('--filter',default='Hearthward.Animals');parser.add_argument('--report-dir',type=Path,default=OUT);args=parser.parse_args()
    output=args.report_dir;output.mkdir(parents=True,exist_ok=True)
    client=UEClient(project_path=GAME/'Hearthward.uproject',ue_root=ue_root(),port=30031,runtime_port=30032)
    if args.tests:
        stamp=datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
        result=client.testing.run_automation_tests(args.filter,report_dir=str(output/'automation'/stamp),extra_args=['-NullRHI'],timeout=240)
        name='automation_result.json' if args.filter=='Hearthward.Animals' else 'regression_result.json'
    else:
        result=client.build.project(target='HearthwardEditor',configuration='Development',timeout=1200)
        name='build_result.json'
    (output/name).write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(result,ensure_ascii=False,indent=2),flush=True)
    if not result['ok']:raise SystemExit(1)
if __name__=='__main__':main()
