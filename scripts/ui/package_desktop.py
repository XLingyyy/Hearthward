"""Build the approved UI revision as a local Windows Shipping archive."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import time
import uuid

GAME = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(GAME / "scripts/animals"))
from paths import configure_factory, ue_root

configure_factory()
from engine_adapters.ue5 import UEClient


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def write_json(path, data):
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run-id", default="desktop_" + uuid.uuid4().hex[:12])
    args = parser.parse_args()
    if not re.fullmatch(r"desktop_[a-zA-Z0-9_]+", args.run_id):
        parser.error("run-id must be desktop_ followed by letters, numbers or underscores")
    evidence = GAME / ".agent-local/qa/TASK-076" / args.run_id
    generated = GAME / "Saved/UIShipping/TASK-053" / args.run_id
    if evidence.exists() or generated.exists():
        raise FileExistsError("Use a new run-id; existing build evidence is preserved")
    evidence.mkdir(parents=True)
    generated.mkdir(parents=True)
    sources = [GAME / "Hearthward.uproject"]
    for root in ("Source", "Config", "Resources", "Plugins/A3GamePlayable/Source"):
        sources.extend(p for p in (GAME / root).rglob("*") if p.is_file())
    manifest = {p.relative_to(GAME).as_posix(): sha256(p) for p in sorted(sources)}
    write_json(evidence / "source-manifest.json", manifest)
    version_source = (GAME / "Source/Hearthward/Update/HearthwardUpdateSubsystem.h").read_text(encoding="utf-8")
    version = re.search(r'Current=TEXT\("([^\"]+)"\)', version_source).group(1)
    metadata = {
        "run_id": args.run_id,
        "version": version,
        "base_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=GAME, text=True).strip(),
        "branch": subprocess.check_output(["git", "branch", "--show-current"], cwd=GAME, text=True).strip(),
        "configuration": "Win64 Shipping",
        "engine": "5.8.2",
        "archive": str(generated / "Archive/Windows"),
        "source_manifest_sha256": sha256(evidence / "source-manifest.json"),
        "local_changes": "TASK-053 approved UI; uncommitted; local deployment only",
    }
    write_json(evidence / "build-info.json", metadata)
    print(json.dumps(metadata, ensure_ascii=False), flush=True)
    client = UEClient(project_path=GAME / "Hearthward.uproject", ue_root=ue_root())
    started = time.monotonic()
    result = client.build.package(
        archive_dir=generated / "Archive", configuration="Shipping",
        maps=("/Game/Hearthward/Bootstrap/L_Bootstrap", "/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds"),
        extra_args=("-stagingdirectory=" + str(generated / "Staged"), "-nodebuginfo"),
        log_path=evidence / "package.log", timeout=10800,
    )
    write_json(evidence / "package-result.json", result)
    metadata["elapsed_seconds"] = round(time.monotonic() - started, 2)
    metadata["package_result"] = "PASS" if result["ok"] else "FAIL"
    if result["ok"]:
        archive = generated / "Archive/Windows"
        metadata["executable_sha256"] = sha256(archive / "Hearthward/Binaries/Win64/Hearthward-Win64-Shipping.exe")
        metadata["ui_files_match"] = all(
            sha256(archive / "Hearthward" / name) == manifest[name]
            for name in ("Resources/UI/interface.json", "Resources/UI/layout.json")
        )
        if not metadata["ui_files_match"]:
            raise RuntimeError("Archived UI does not match the approved revision")
        write_json(archive / "BUILD-INFO.json", metadata)
    write_json(evidence / "build-info.json", metadata)
    print(json.dumps({"ok": result["ok"], "evidence": str(evidence), **metadata}, ensure_ascii=False), flush=True)
    raise SystemExit(0 if result["ok"] else 1)


if __name__ == "__main__":
    main()
