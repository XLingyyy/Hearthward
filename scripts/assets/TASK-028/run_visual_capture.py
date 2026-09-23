"""Launch TASK-028 DX12 visual capture through UEClient."""

import json
import sys
import time
import uuid
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(PROJECT.parent / "GameFactory-3A"))
from engine_adapters.ue5 import UEClient  # noqa: E402

report = PROJECT / "Saved/Task028/visual-capture.json"
assert not report.exists(), "Preserve prior visual report before retrying"
pool = str(uuid.uuid4())
client = UEClient(project_path=PROJECT / "Hearthward.uproject", ue_root="E:/UE_5.8")
script = Path(__file__).with_name("capture_visual.py")
launch = client.runtime.launch_editor(
    map_path="/Engine/Maps/Entry",
    extra_args=["-ExecutePythonScript=" + str(script), "-HearthwardSaveTestPool=" + pool,
                "-unattended", "-NoSplash", "-d3d12", "-sm6",
                "-ini:Engine:[/Script/WindowsTargetPlatform.WindowsTargetSettings]:D3D12TargetedShaderFormats=PCD3D_SM6"],
)
assert launch.get("ok"), launch
pid = launch["payload"]["process_id"]
try:
    deadline = time.monotonic() + 1500
    while not report.exists() and time.monotonic() < deadline:
        time.sleep(5)
    assert report.exists(), "Timed out waiting for Unreal visual capture"
    result = json.loads(report.read_text(encoding="utf-8"))
    result["isolated_pool_id"] = pool
    report.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"ok": result["ok"], "captures": result["captures"],
                      "error": result.get("error")}, ensure_ascii=False))
    assert result["ok"], result.get("error")
finally:
    print(json.dumps(client.runtime.stop_editor(pid), ensure_ascii=False))
