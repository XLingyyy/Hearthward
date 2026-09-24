"""Run with the GameFactory Python environment from the GameFactory root."""
import json
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from engine_adapters.ue5 import UEClient
project=Path(__file__).resolve().parents[2]
output=Path(sys.argv[1]) if len(sys.argv)>1 else Path("F:/HearthwardDemo/20260924")
output.mkdir(parents=True,exist_ok=True)
ue=UEClient(project_path=project/"Hearthward.uproject",ue_root="G:/UnrealEngine/UE_5.8")
result=ue.build.package(archive_dir=output,log_path=output/"package.log",
    maps=("/Game/Hearthward/Bootstrap/L_Bootstrap","/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds"),
    extra_args=("-stagingdirectory="+str(output/"Staged"),"-nodebuginfo"),timeout=10800)
(output/"package-result.json").write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
print(json.dumps(result,ensure_ascii=False,indent=2))
raise SystemExit(0 if result["ok"] else 1)
