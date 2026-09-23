"""Build the TASK-028 editor target via the GameFactory UEClient."""

import json
import sys
from pathlib import Path

project = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(project.parent / "GameFactory-3A"))
from engine_adapters.ue5 import UEClient  # noqa: E402

client = UEClient(project_path=project / "Hearthward.uproject", ue_root="E:/UE_5.8")
result = client.build.project(target="HearthwardEditor", configuration="Development", timeout=1200)
output = project / "Saved/Task028/integration-build.json"
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(json.dumps({"ok": result.get("ok"), "errors": result.get("errors", []),
                  "returncode": result.get("payload", {}).get("returncode")}, ensure_ascii=False))
if not result.get("ok"):
    raise SystemExit(1)
