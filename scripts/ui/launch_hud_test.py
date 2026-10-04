"""Launch the HUD and inventory preview in an isolated new game, or capture the real prologue world."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
import uuid

GAME = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(GAME / "scripts/animals"))
from paths import configure_factory, ue_root

configure_factory()
from engine_adapters.ue5 import UEClient


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture", action="store_true")
    parser.add_argument("--smoke", action="store_true", help="Observe the actual -game preview startup, then stop our own process")
    args = parser.parse_args()
    if args.capture and args.smoke:
        parser.error("capture and smoke are separate runs")
    run_id = "hud_" + uuid.uuid4().hex[:12]
    evidence = GAME / "docs/qa/TASK-053" / run_id
    evidence.mkdir(parents=True, exist_ok=True)
    os.environ["HEARTHWARD_HUD_RUN"] = run_id
    client = UEClient(project_path=GAME / "Hearthward.uproject", ue_root=ue_root(),
                      port=30081, runtime_port=30082)
    preference_root = GAME / "Saved/Config"
    preferences = {p: p.read_bytes() for p in preference_root.glob("*/GameUserSettings.ini")}
    extra = ["-HearthwardSaveTestPool=" + str(uuid.uuid4()),
             "-abslog=" + str(evidence / "runtime.log"),
             "-CoreLimit=4",
             "-windowed", "-ResX=1600", "-ResY=1000", "-WinX=80", "-WinY=80"]
    if args.capture:
        extra += ["-RenderOffscreen", "-unattended", "-nosound",
                  "-ExecutePythonScript=" + str(GAME / "docs/qa/TASK-053/capture_hud_world.py")]
    else:
        extra += ["-game", "-HearthwardHUDPreview"]
    launch = client.runtime.launch_editor(map_path="/Game/Hearthward/Bootstrap/L_Bootstrap", extra_args=extra)
    (evidence / "launch.json").write_text(json.dumps(launch, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({"run_id": run_id, "launch": launch}, ensure_ascii=False, indent=2), flush=True)
    if not launch["ok"]:
        raise SystemExit(1)
    sources = ["Resources/UI/interface.json", "Resources/UI/layout.json", "Source/Hearthward/HearthwardCharacter.cpp",
               "Binaries/Win64/UnrealEditor-Hearthward.dll"]
    sources += [str(p.relative_to(GAME)).replace("\\", "/") for p in (GAME / "Source/Hearthward/UI").glob("HearthwardScreen*.*")]
    sources += ["Resources/Data/gameplay.json", "Source/Hearthward/Gameplay/HearthwardGameplayComponent.cpp",
                "Source/Hearthward/Gameplay/HearthwardProgression.cpp", "Source/Hearthward/Gameplay/HearthwardProgression.h"]
    fingerprints = {name: hashlib.sha256((GAME / name).read_bytes()).hexdigest() for name in sources}
    (evidence / "fingerprints.json").write_text(json.dumps(fingerprints, indent=2), encoding="utf-8")
    report_path = GAME / "Saved/HUDPreview" / run_id / "report.json"
    try:
        if args.smoke:
            process_id = int(launch["payload"]["process_id"])
            samples = []
            deadline = time.monotonic() + 120
            ready = False
            steady_samples = 0
            while time.monotonic() < deadline:
                time.sleep(5)
                command = (f"Get-Process -Id {process_id} -ErrorAction SilentlyContinue | "
                           "Select-Object Id,Responding,MainWindowTitle,"
                           "@{Name='HasWindow';Expression={[bool]$_.MainWindowHandle}} | ConvertTo-Json -Compress")
                probe = subprocess.run(["powershell.exe", "-NoProfile", "-Command", command],
                                       capture_output=True, text=True, encoding="utf-8", errors="replace")
                sample = json.loads(probe.stdout.strip()) if probe.stdout.strip() else {"exited": True}
                log = (evidence / "runtime.log").read_text(encoding="utf-8", errors="replace") if (evidence / "runtime.log").is_file() else ""
                map_ready_index = log.rfind("Load map complete /Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds")
                ready = map_ready_index >= 0 and log.rfind("Hearthward loading end") > map_ready_index
                sample["natural_map_ready"] = ready
                samples.append(sample)
                steady_samples = steady_samples + 1 if ready and sample.get("Responding") and sample.get("HasWindow") else 0
                if sample.get("exited") or steady_samples >= 3:
                    break
            passed = ready and steady_samples >= 3
            report = {"passed": passed, "auto_new_game_loaded": ready, "samples": samples,
                      "ready_responsive_samples": steady_samples,
                      "limitation": "Observed actual Development -game launch and three responsive windows after Natural loading ended; no physical input."}
            (evidence / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
            print(json.dumps({"run_id": run_id, "passed": passed, "auto_new_game_loaded": ready}, ensure_ascii=False), flush=True)
            if not passed:
                raise SystemExit(1)
            return
        if not args.capture:
            print("主界面、背包与技能 UI 测试已启动。按 K 打开技能，按 Tab 打开背包，顶部切换装备／材料／食物／工具；滚轮切换HUD道具，原有使用快捷键保留。使用独立临时存档池，桌面安装版不变。", flush=True)
            return
        deadline = time.monotonic() + 480
        while not report_path.is_file() and time.monotonic() < deadline:
            time.sleep(1)
        if not report_path.is_file():
            raise TimeoutError("Natural-world HUD capture did not return a report")
        report = json.loads(report_path.read_text(encoding="utf-8"))
        for filename in report.get("captures", []):
            shutil.copy2(report_path.parent / filename, evidence / filename)
        for filename in report.get("attachments", []):
            shutil.copy2(report_path.parent / filename, evidence / filename)
        shutil.copy2(report_path, evidence / "report.json")
        print(json.dumps({"run_id": run_id, "passed": report["passed"], "checks": report["checks"],
                          "error": report.get("error"), "evidence": str(evidence)}, ensure_ascii=False, indent=2), flush=True)
        if not report["passed"]:
            raise SystemExit(1)
    finally:
        if args.capture or args.smoke:
            result = client.runtime.stop_editor(launch["payload"]["process_id"])
            (evidence / "stop.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
            for path in set(preferences) | set(preference_root.glob("*/GameUserSettings.ini")):
                if path in preferences:
                    path.write_bytes(preferences[path])
                else:
                    path.unlink()


if __name__ == "__main__":
    main()
