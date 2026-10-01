from pathlib import Path
import json,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[6]))
from engine_adapters.ue5 import UEClient
root=Path(__file__).resolve().parents[3]
ue=UEClient(project_path=str(root/"Hearthward.uproject"),ue_root="G:/UnrealEngine/UE_5.8")
r=ue.testing.run_automation_tests("Hearthward.Experience+Hearthward.Survival+Hearthward.Combat",report_dir=str(root/"Saved/Task051/native"),extra_args=["-NullRHI","-HearthwardSaveTestPool=TASK051-native"],timeout=300)
(root/"docs/qa/TASK-051/native.json").write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding="utf-8")
print(json.dumps({"ok":r["ok"],"errors":r.get("errors"),"tests":{k:v for k,v in r.get("payload",{}).items() if k.startswith("tests_")}},ensure_ascii=False))
raise SystemExit(0 if r["ok"] else 1)
