"""Launch a TASK-028 Unreal Python asset script via the public UEClient."""

import json
import sys
import time
import argparse
from pathlib import Path


PROJECT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(PROJECT.parent / "GameFactory-3A"))
from engine_adapters.ue5 import UEClient  # noqa: E402


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("step", choices=["import", "textures", "inspect", "diagnose", "backing"])
    args = parser.parse_args()
    scripts = {
        "import": ("import_static_assets.py", "static-import-report.json"),
        "textures": ("fix_imported_textures.py", "texture-fix-report.json"),
        "inspect": ("inspect_imported_assets.py", "reopen-inspection.json"),
        "diagnose": ("diagnose_material.py", "material-diagnostic.json"),
        "backing": ("create_backing_materials.py", "backing-materials.json"),
    }
    script_name, report_name = scripts[args.step]
    report = PROJECT / "Saved/Task028" / report_name
    assert not report.exists(), "Preserve the previous import report before retrying"
    script = Path(__file__).with_name(script_name)
    client = UEClient(project_path=PROJECT / "Hearthward.uproject", ue_root="E:/UE_5.8")
    launch = client.runtime.launch_editor(
        map_path="/Engine/Maps/Entry",
        extra_args=["-ExecutePythonScript=" + str(script), "-unattended", "-NoSplash", "-NullRHI"],
    )
    assert launch.get("ok"), launch
    pid = launch["payload"]["process_id"]
    try:
        deadline = time.monotonic() + 1200
        while not report.exists() and time.monotonic() < deadline:
            time.sleep(5)
        assert report.exists(), "Timed out waiting for Unreal import report"
        result = json.loads(report.read_text(encoding="utf-8"))
        print(json.dumps({"ok": result["ok"], "rows": len(result.get("assets", result.get("textures", []))),
                          "error": result.get("error")}, ensure_ascii=False))
        assert result["ok"], result["error"]
    finally:
        print(json.dumps(client.runtime.stop_editor(pid), ensure_ascii=False))


if __name__ == "__main__":
    main()
