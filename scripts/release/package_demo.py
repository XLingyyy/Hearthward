"""Package Hearthward with the prepared GameFactory Python environment."""
import json
from pathlib import Path
import argparse
import os
import sys
import shutil
import subprocess
import re
project=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("output",type=Path,nargs="?",default=Path("F:/HearthwardDemo/20261006-2"))
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
version_source=(project/"Source/Hearthward/Update/HearthwardUpdateSubsystem.h").read_text(encoding="utf-8")
version=re.search(r'Current=TEXT\("([^"]+)"\)',version_source).group(1)
metadata={"version":version,"configuration":"Win64 Shipping","engine":"5.8.2",
    "base_commit":subprocess.check_output(["git","rev-parse","HEAD"],cwd=project,text=True).strip(),
    "branch":subprocess.check_output(["git","branch","--show-current"],cwd=project,text=True).strip(),
    "workspace_changes":subprocess.check_output(["git","status","--short"],cwd=project,text=True,encoding="utf-8"),
    "scope":"TASK-078 through TASK-082 integrated main: companion work, dialogue, clan teams, quest guidance, map and storage UI; playable preview"}
(output/"build-info.json").write_text(json.dumps(metadata,ensure_ascii=False,indent=2),encoding="utf-8")
ue=UEClient(project_path=project/"Hearthward.uproject",ue_root="G:/UnrealEngine/UE_5.8")
result=ue.build.package(archive_dir=output,log_path=output/"package.log",
    maps=("/Game/Hearthward/Bootstrap/L_Bootstrap","/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds"),
    extra_args=("-stagingdirectory="+str(output/"Staged"),"-nodebuginfo"),timeout=10800)
(output/"package-result.json").write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
if result["ok"]:
    archive=output/"Windows"
    shutil.copy2(project/"scripts/release/README-DEMO.txt",archive/"README-DEMO.txt")
    shutil.copy2(project/"scripts/release/RELEASE-NOTES.txt",archive/"RELEASE-NOTES.txt")
    shutil.copy2(output/"build-info.json",archive/"BUILD-INFO.json")
    (archive/"启动游戏.cmd").write_bytes(b'@echo off\r\ncd /d "%~dp0"\r\nstart "" "Hearthward.exe" -HearthwardAIBackend=vulkan -HearthwardAIGpuLayers=32\r\n')
    (archive/"启动游戏_CPU.cmd").write_bytes(b'@echo off\r\ncd /d "%~dp0"\r\nstart "" "Hearthward.exe" -HearthwardAIBackend=cpu\r\n')
print(json.dumps(result,ensure_ascii=False,indent=2))
raise SystemExit(0 if result["ok"] else 1)
