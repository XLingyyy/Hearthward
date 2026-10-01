from pathlib import Path
import json
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[6]))
from engine_adapters.ue5 import UEClient
project=Path("G:/GameFactory/Hearthward/.agent-local/task051")
ue=UEClient(project_path=str(project/"Hearthward.uproject"),ue_root="G:/UnrealEngine/UE_5.8")
result=ue.build.project(target="HearthwardEditor",configuration="Development",timeout=1200)
(project/"docs/qa/TASK-051/build.json").write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
print(json.dumps({"ok":result["ok"],"diagnostics":result.get("diagnostics"),"errors":result.get("errors")},ensure_ascii=False))
raise SystemExit(0 if result["ok"] else 1)
