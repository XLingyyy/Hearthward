from pathlib import Path
import argparse, json, sys, uuid
sys.path.insert(0,str(Path(__file__).resolve().parents[6]))
from engine_adapters.ue5 import UEClient
parser=argparse.ArgumentParser()
parser.add_argument("--filter",default="Hearthward.Time+Hearthward.Save+Hearthward.Camp+Hearthward.Nature+Hearthward.Campaign+Hearthward.Survival")
parser.add_argument("--label",default="final")
parser.add_argument("--render",action="store_true")
parser.add_argument("--pool",type=uuid.UUID,help="Reuse an isolated save pool across separate automation processes")
parser.add_argument("--extra-arg",action="append",default=[])
args=parser.parse_args()
root=Path(__file__).resolve().parents[3]
ue=UEClient(project_path=str(root/"Hearthward.uproject"),ue_root="G:/UnrealEngine/UE_5.8")
extra_args=(["-NoSound","-Culture=en"] if args.render else ["-NullRHI","-NoSound","-Culture=en"])+["-HearthwardSaveTestPool="+str(args.pool or uuid.uuid5(uuid.NAMESPACE_URL,"Hearthward/TASK053/"+args.label))]+args.extra_arg
r=ue.testing.run_automation_tests(args.filter,report_dir=str(root/"Saved/Task053"/args.label),extra_args=extra_args,timeout=600)
(root/"docs/qa/TASK-053"/(args.label+"-native.json")).write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding="utf-8")
print(json.dumps({"ok":r["ok"],"errors":r.get("errors"),"tests":{k:v for k,v in r.get("payload",{}).items() if k.startswith("tests_")}},ensure_ascii=False))
raise SystemExit(0 if r["ok"] else 1)
