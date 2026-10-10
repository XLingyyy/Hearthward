#!/usr/bin/env python3
"""Run TASK-104 and existing presentation regressions with the project's UEClient.

Run from the separately installed GameFactory Python environment on the authorized
Windows UE 5.8.2 machine. This runner does not download/install an engine, edit maps,
start a normal save, or claim that NullRHI tests validate appearance or listening.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import subprocess
import uuid


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", required=True, type=Path)
    parser.add_argument("--ue-root", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    project = args.project.resolve()
    if not project.is_file() or project.suffix != ".uproject":
        parser.error("--project must be the actual Hearthward.uproject")
    if not args.ue_root.is_dir():
        parser.error("--ue-root must be an existing UE 5.8.2 installation")
    args.output.mkdir(parents=True, exist_ok=True)
    from engine_adapters.ue5 import UEClient
    client = UEClient(project_path=str(project), ue_root=str(args.ue_root.resolve()))
    sha = subprocess.check_output(["git", "-C", str(project.parent), "rev-parse", "HEAD"], text=True).strip()
    dirty = subprocess.check_output(["git", "-C", str(project.parent), "status", "--porcelain"], text=True)
    results = {"commit": sha, "working_tree_status": dirty, "visual_and_listening": "NOT_RUN", "runs": []}
    def save():
        (args.output / "results.json").write_text(json.dumps(results, ensure_ascii=False, indent=2, default=str) + "\n", encoding="utf-8")
    build = client.build.project(target="HearthwardEditor", configuration="Development", timeout=1200)
    results["build"] = build
    save()
    if not build.get("ok"):
        return 1
    # Existing producer/consumer lifecycle coverage is retained, not replaced by
    # new narrow helper tests. Nonzero actual discovery is required for every run.
    for group in ("Hearthward.Iteration.Task104.", "Hearthward.Iteration.Task099."):
        # The integrated TASK-099 fire tests require an active rendered Niagara system.
        extra_args = ["-UserDir=" + str((args.output / "profile").resolve()),
                      "-HearthwardSaveTestPool=" + str(uuid.uuid4())]
        result = client.testing.run_automation_tests(group, report_dir=str((args.output / group.rstrip(".")).resolve()), extra_args=extra_args, timeout=600)
        results["runs"].append({"filter": group, "result": result})
        save()
        if not result.get("ok") or result.get("payload", {}).get("tests_found", 0) <= 0:
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
