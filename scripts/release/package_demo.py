"""Package Hearthward with the prepared GameFactory Python environment."""
import json
from pathlib import Path
import argparse
import os
import sys
project=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("output",type=Path,nargs="?",default=Path("F:/HearthwardDemo/20260924"))
args=parser.parse_args()
configured=os.environ.get("HEARTHWARD_FACTORY_ROOT")
candidates=[Path(configured)] if configured else []
candidates.extend(project.parents)
factory=next((candidate for candidate in candidates if (candidate/"engine_adapters/ue5/__init__.py").is_file()),None)
if factory is None:
    parser.error("Set HEARTHWARD_FACTORY_ROOT to the prepared GameFactory checkout.")
sys.path.insert(0,str(factory.resolve()))
from engine_adapters.ue5 import UEClient
output=args.output
output.mkdir(parents=True,exist_ok=True)
ue=UEClient(project_path=project/"Hearthward.uproject",ue_root="G:/UnrealEngine/UE_5.8")
result=ue.build.package(archive_dir=output,log_path=output/"package.log",
    maps=("/Game/Hearthward/Bootstrap/L_Bootstrap","/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds"),
    extra_args=("-stagingdirectory="+str(output/"Staged"),"-nodebuginfo"),timeout=10800)
(output/"package-result.json").write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
print(json.dumps(result,ensure_ascii=False,indent=2))
raise SystemExit(0 if result["ok"] else 1)
