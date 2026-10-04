"""Launch the compiled title UI prototype, or run its isolated PIE verification."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
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
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    run_id = ("verify_" if args.verify else "preview_") + uuid.uuid4().hex[:12]
    evidence = GAME / "docs/qa/TASK-053" / run_id
    evidence.mkdir(parents=True, exist_ok=True)
    preference_root = GAME / "Saved/Config"
    preference_snapshot = {p: p.read_bytes() for p in preference_root.glob("*/GameUserSettings.ini")} if args.verify else {}
    os.environ["HEARTHWARD_TITLE_RUN"] = run_id
    client = UEClient(project_path=GAME / "Hearthward.uproject", ue_root=ue_root(),
                      port=30071, runtime_port=30072)
    extra = ["-HearthwardSaveTestPool=" + str(uuid.uuid4()),
             "-abslog=" + str(evidence / "runtime.log")]
    if args.verify:
        extra += ["-RenderOffscreen", "-unattended", "-nosound",
                  "-ExecutePythonScript=" + str(GAME / "docs/qa/TASK-053/verify_title.py")]
    else:
        extra += ["-game", "-windowed", "-ResX=1600", "-ResY=1000", "-WinX=80", "-WinY=80"]
    launch = client.runtime.launch_editor(map_path="/Game/Hearthward/Bootstrap/L_Bootstrap",
                                          extra_args=extra)
    (evidence / "launch.json").write_text(json.dumps(launch, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({"run_id": run_id, "launch": launch}, ensure_ascii=False, indent=2), flush=True)
    if not launch["ok"]:
        raise SystemExit(1)
    sources = ["Resources/UI/interface.json", "Resources/UI/layout.json",
               "Binaries/Win64/UnrealEditor-Hearthward.dll", "Source/Hearthward/HearthwardCharacter.cpp"]
    sources += [str(p.relative_to(GAME)).replace("\\", "/")
                for p in (GAME / "Source/Hearthward/UI").glob("HearthwardScreen*.*")]
    sources += ["Resources/Data/gameplay.json", "Source/Hearthward/Gameplay/HearthwardGameplayComponent.cpp",
                "Source/Hearthward/Gameplay/HearthwardProgression.cpp", "Source/Hearthward/Gameplay/HearthwardProgression.h"]
    hashes = {name: hashlib.sha256((GAME / name).read_bytes()).hexdigest() for name in sources}
    (evidence / "fingerprints.json").write_text(json.dumps(hashes, indent=2), encoding="utf-8")
    if not args.verify:
        print("标题 UI 测试窗口已启动。滚轮或上下键选择，Enter确认；本次使用独立临时存档池。", flush=True)
        return
    report_path = GAME / "Saved/TitleWheel" / run_id / "report.json"
    try:
        deadline = time.monotonic() + 240
        while not report_path.is_file() and time.monotonic() < deadline:
            time.sleep(1)
        if not report_path.is_file():
            raise TimeoutError("PIE title UI verification did not return a report")
        report = json.loads(report_path.read_text(encoding="utf-8"))
        for filename in report.get("captures", []):
            shutil.copy2(GAME / "Saved/Task020" / filename, evidence / filename)
        shutil.copy2(report_path, evidence / "report.json")
        if report_path.with_name("modal-focus.json").is_file():
            shutil.copy2(report_path.with_name("modal-focus.json"), evidence / "modal-focus.json")
        if report_path.with_name("hud-preview.json").is_file():
            shutil.copy2(report_path.with_name("hud-preview.json"), evidence / "hud-preview.json")
        if report_path.with_name("inventory-preview.json").is_file():
            shutil.copy2(report_path.with_name("inventory-preview.json"), evidence / "inventory-preview.json")
        if report_path.with_name("skills-preview.json").is_file():
            shutil.copy2(report_path.with_name("skills-preview.json"), evidence / "skills-preview.json")
        if report_path.with_name("journal-preview.json").is_file():
            shutil.copy2(report_path.with_name("journal-preview.json"), evidence / "journal-preview.json")
        print(json.dumps({"passed": report["passed"], "run_id": run_id,
                          "checks": report["checks"], "error": report.get("error"),
                          "evidence": str(evidence)}, ensure_ascii=False, indent=2), flush=True)
        if not report["passed"]:
            raise SystemExit(1)
    finally:
        result = client.runtime.stop_editor(launch["payload"]["process_id"])
        (evidence / "stop.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
        restored = []
        for path in set(preference_snapshot) | set(preference_root.glob("*/GameUserSettings.ini")):
            if path in preference_snapshot:
                path.write_bytes(preference_snapshot[path])
            else:
                path.unlink()
            restored.append(str(path.relative_to(GAME)))
        (evidence / "preference-restore.json").write_text(json.dumps({"restored": restored}, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
