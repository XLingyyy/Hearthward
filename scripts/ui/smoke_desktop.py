"""Observe startup of the installed Shipping game, then stop only our own process."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

GAME = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(GAME / "scripts/animals"))
from paths import configure_factory, ue_root

configure_factory()
from engine_adapters.ue5 import UEClient


def player_hashes():
    root = Path(os.environ["LOCALAPPDATA"]) / "Hearthward/Saved"
    # CrashReportClient generates a new diagnostic ini on each launch; it is not a player setting.
    files = list((root / "SaveGames").rglob("*")) + list((root / "Config").glob("*/GameUserSettings.ini"))
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(files) if p.is_file()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence-root", required=True, type=Path)
    parser.add_argument("--attempt", default="", help="Suffix for a new, preserved startup report")
    parser.add_argument("--samples", type=int, choices=range(3,13), default=6)
    args = parser.parse_args()
    if args.attempt and not re.fullmatch(r"[a-zA-Z0-9_]+", args.attempt):
        parser.error("attempt must contain only letters, numbers or underscores")
    evidence = args.evidence_root.resolve(strict=True)
    result_path = evidence / ("shipping-startup" + ("-"+args.attempt if args.attempt else "") + ".json")
    if result_path.exists():
        raise FileExistsError("Preserve previous startup evidence")
    install = Path("C:/Users/22543/Desktop/Hearthward-20260929-9058ee2/Windows")
    executable = install / "Hearthward/Binaries/Win64/Hearthward-Win64-Shipping.exe"
    before = player_hashes()
    client = UEClient(project_path=GAME / "Hearthward.uproject", ue_root=ue_root())
    launch = client.runtime.launch_packaged(executable, extra_args=(
        "-HearthwardAIBackend=vulkan", "-HearthwardAIGpuLayers=16"))
    result = {"passed": False, "launch": launch, "samples": [], "player_before": before,
              "executable_sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
              "limitation": "Startup/process-window observation only; no physical input or Shipping UI screenshot."}
    started = time.monotonic()
    if launch["ok"]:
        process_id = int(launch["payload"]["process_id"])
        try:
            for _ in range(args.samples):
                time.sleep(5)
                command = (f"Get-Process -Id {process_id} -ErrorAction SilentlyContinue | "
                           "Select-Object Id,Responding,MainWindowTitle,WorkingSet64,"
                           "@{Name='HasWindow';Expression={[bool]$_.MainWindowHandle}} | ConvertTo-Json -Compress")
                probe = subprocess.run(["powershell.exe", "-NoProfile", "-Command", command],
                                       capture_output=True, text=True, encoding="utf-8", errors="replace")
                raw = probe.stdout.strip()
                sample = json.loads(raw) if raw else {"exited": True}
                if probe.returncode:
                    sample["probe_returncode"] = probe.returncode
                    sample["probe_error"] = probe.stderr.strip()
                sample["elapsed_seconds"] = round(time.monotonic()-started, 2)
                result["samples"].append(sample)
                if sample.get("exited"):
                    break
            result["passed"] = len(result["samples"]) == args.samples and all(
                s.get("Responding") and s.get("HasWindow") and s.get("MainWindowTitle", "").strip() == "Hearthward"
                for s in result["samples"][-3:])
        except Exception as error:
            result["error"] = f"{type(error).__name__}: {error}"
        finally:
            # UEClient also tracks launch_packaged ownership for this public stop API.
            result["stop"] = client.runtime.stop_editor(process_id)
    result["player_after"] = player_hashes()
    result["player_data_unchanged"] = before == result["player_after"]
    result_path.write_text(json.dumps(result, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    print(json.dumps({"passed": result["passed"], "player_data_unchanged": result["player_data_unchanged"],
                      "samples": len(result["samples"]), "evidence": str(result_path)}, ensure_ascii=False), flush=True)
    raise SystemExit(0 if result["passed"] else 1)


if __name__ == "__main__":
    main()
