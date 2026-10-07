"""Install candidate-3 documents, then close its verified runtime tree and ZIP."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath, PureWindowsPath
import re
import shutil
import stat
import uuid
import zipfile

PROJECT = Path(__file__).resolve().parents[3]
CANDIDATE = "iteration-084-103-20261007-3"
VERSION = "0.2.0-preview.20261007.2"
WINDOWS = Path("F:/HearthwardDemo") / CANDIDATE / "Windows"
ZIP = WINDOWS.parent.with_suffix(".zip")
EVIDENCE = PROJECT / ".agent-local/qa/TASK-103/package-20261007-3"
RELEASE = PROJECT / "docs/releases/iteration-084-103-rc"
DOCUMENTS = {"README-DEMO.txt": "README-DEMO.txt", "RELEASE-NOTES.txt": "RELEASE-NOTES.txt",
             "CANDIDATE3_BUILD_INFO.json": "BUILD-INFO.json",
             "Start-Hearthward-CPU.cmd": "Start-Hearthward-CPU.cmd",
             "Start-Hearthward-Vulkan.cmd": "Start-Hearthward-Vulkan.cmd"}
EXCLUDED = {"Hearthward/Resources/UI/items-clean.prompt.txt",
            "Hearthward/Resources/UI/LAYOUT.md", "Hearthward/Resources/Audio/README.md"}
LICENSES = ["NOTICES.txt", "Hearthward/Runtime/LocalAI/LICENSE-Qwen.txt",
            "Hearthward/Runtime/LocalAI/LICENSE-llama.cpp.txt",
            "Hearthward/Runtime/LocalAI/bin/cpu/LICENSE-LLVM-OpenMP",
            "Hearthward/Runtime/LocalAI/bin/vulkan/LICENSE-LLVM-OpenMP",
            "Hearthward/Resources/UI/Fonts/OFL.txt",
            "Hearthward/Resources/UI/Fonts/LXGW-OFL.txt",
             "Hearthward/Resources/Audio/TASK-099/License-Kenney-RPG-Audio.txt",
             "Hearthward/Resources/Audio/TASK-099/License-RandomMind-Vistula.txt"]
WATER_DATA = "Hearthward/Resources/Data/TASK-099-water-audio.json"
WATER_MESH = "/Game/Hearthward/Assets/NaturalWorld/Rebuild/Meshes/Lake_0/StaticMeshes/SM_Lake_0.SM_Lake_0"
WATER_SOURCE_REPORT = PROJECT / "docs/qa/TASK-099/water/second-read-water-source-20261006T235439Z-44d9531a.json"
EXPECTED_EVENTS, EXPECTED_WAVS = 17, 12
FINAL_LAUNCHER_STATE = "SCRIPT_LAUNCH_AND_OS_GAME_INPUT_VERIFIED"
REQUIRED = ["Hearthward.exe", "Hearthward/Binaries/Win64/Hearthward-Win64-Shipping.exe",
            "Hearthward/Runtime/LocalAI/models/Qwen3.5-4B-Q4_K_M.gguf",
            "Hearthward/Runtime/LocalAI/bin/cpu/llama-server.exe",
            "Hearthward/Runtime/LocalAI/bin/vulkan/llama-server.exe",
            "Hearthward/Content/Paks/Hearthward-Windows.pak",
            "Hearthward/Content/Paks/Hearthward-Windows.ucas",
            "Hearthward/Content/Paks/Hearthward-Windows.utoc",
            "Hearthward/Content/Paks/global.ucas", "Hearthward/Content/Paks/global.utoc", WATER_DATA]
TEXT_SUFFIXES = {".txt", ".md", ".json", ".ini", ".cfg", ".conf", ".toml", ".yaml",
                 ".yml", ".xml", ".csv", ".cmd", ".bat", ".ps1", ".html", ".license"}
FORBIDDEN_PARTS = {"source", "saved", "profile", "profiles", "cache", "deriveddatacache",
                   "intermediate", "qa", ".agent-local", ".git", ".github", "__pycache__"}
FORBIDDEN_SUFFIXES = {".hws", ".sav", ".py", ".pyc", ".uproject", ".pem", ".key", ".pfx", ".p12"}
PATTERNS = {"private_key": re.compile(r"-----BEGIN (?:RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----"),
            "service_token": re.compile(r"\b(?:sk-[A-Za-z0-9_-]{20,}|ghp_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,}|(?:AKIA|ASIA)[A-Z0-9]{16})\b"),
            "credential_assignment": re.compile(r"(?im)\b(?:api[_-]?key|secret[_-]?key|access[_-]?token|authorization|password|credential)\b[\"']?\s*[:=]\s*[\"']?(?:Bearer\s+)?([^\s\"',;}\]]+)")}
NON_SECRETS = {"null", "none", "true", "false", "not_required", "not_configured", "not_granted",
               "unknown", "pending", "environment", "<api_key>", "<token>"}


def require(condition, code):
    if not condition:
        raise RuntimeError(code)


def exact_windows_path(value, expected):
    p = PureWindowsPath(value)
    require(p.is_absolute() and p.drive.upper() == "F:" and ".." not in p.parts
            and str(p).casefold() == str(PureWindowsPath(expected)).casefold(), "WRONG_CANDIDATE_PATH")


def json_read(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def json_new(path, value):
    with path.open("x", encoding="utf-8", newline="\n") as f:
        json.dump(value, f, ensure_ascii=False, indent=2)
        f.write("\n")


def metadata(path):
    s = path.stat()
    return {"bytes": s.st_size, "mtime_ns": s.st_mtime_ns}


def project_reference(value):
    require(isinstance(value, str) and bool(value), "MISSING_EVIDENCE_REFERENCE")
    p = Path(value)
    p = p if p.is_absolute() else PROJECT / p
    resolved = p.resolve(strict=True)
    require(resolved.is_relative_to(PROJECT.resolve()) and p.is_file(), "EVIDENCE_OUTSIDE_PROJECT")
    return p


def check_build(final):
    info = json_read(RELEASE / "CANDIDATE3_BUILD_INFO.json")
    require(info.get("candidate_id") == CANDIDATE and info.get("proposed_version") == VERSION,
            "WRONG_BUILD_INFO_CANDIDATE")
    exact_windows_path(info.get("candidate_directory", ""), WINDOWS.parent)
    require(str(info.get("state", "")).startswith("BUILD_SUCCESS")
            and info.get("build_cook_stage_archive") == "SUCCESS", "PACKAGE_NOT_VERIFIED")
    freeze = str(info.get("source_freeze", "")).upper()
    require("FROZEN" in freeze and "UNFROZEN" not in freeze
            and not freeze.startswith(("NOT", "PENDING", "PLANNED", "FAIL")), "SOURCE_NOT_FROZEN")
    refs = info.get("source_freeze_evidence")
    require(isinstance(refs, list) and bool(refs), "MISSING_SOURCE_FREEZE_EVIDENCE")
    for ref in refs:
        project_reference(ref)
    package = json_read(EVIDENCE / "result.json")
    require(package.get("ok") is True and package.get("payload", {}).get("returncode") == 0
            and package.get("payload", {}).get("dry_run") is False, "PACKAGE_RESULT_NOT_SUCCESS")
    exact_windows_path(package["payload"].get("archive_dir", ""), WINDOWS.parent)
    require(info.get("published") is False, "INTERNAL_CANDIDATE_ONLY")
    if final:
        require(info.get("actual_embedded_version") == VERSION, "EMBEDDED_VERSION_NOT_OBSERVED")
        require(str(info.get("shipping_normal_input", "")).upper() in {"PARTIAL_NORMAL_INPUT_VERIFIED",
                "NORMAL_INPUT_VERIFIED", "FULL_NORMAL_INPUT_VERIFIED"}, "OS_EVIDENCE_NOT_VERIFIED")
        project_reference(info.get("shipping_os_evidence"))
        launcher = str(info.get("launchers", {}).get("state", "")).upper()
        require(launcher == FINAL_LAUNCHER_STATE, "LAUNCHER_OS_NOT_VERIFIED")
    return info


def tree():
    exact_windows_path(WINDOWS, "F:/HearthwardDemo/" + CANDIDATE + "/Windows")
    require(WINDOWS.is_dir() and WINDOWS.resolve() == WINDOWS.absolute(), "WINDOWS_MISSING_OR_REDIRECTED")
    files = {}
    for directory, children, names in os.walk(WINDOWS, followlinks=False):
        for name in children + names:
            p = Path(directory) / name
            s = p.lstat()
            require(not p.is_symlink() and not (getattr(s, "st_file_attributes", 0)
                                              & stat.FILE_ATTRIBUTE_REPARSE_POINT), "REPARSE_POINT_NOT_ALLOWED")
            rel = p.relative_to(WINDOWS).as_posix()
            require(not any(x.casefold() in FORBIDDEN_PARTS for x in PurePosixPath(rel).parts)
                    and p.suffix.lower() not in FORBIDDEN_SUFFIXES and not p.name.lower().startswith(".env")
                    and p.name.lower() not in {"credentials.json", "secrets.json", "id_rsa", "id_ed25519"},
                    "FORBIDDEN_RUNTIME_TREE_ENTRY")
            if p.is_file():
                files[rel] = metadata(p)
    return files


def scan_text(path):
    raw = path.read_bytes()
    if raw.startswith((b"\xff\xfe", b"\xfe\xff")):
        text = raw.decode("utf-16")
    else:
        try:
            text = raw.decode("utf-8-sig")
        except UnicodeDecodeError:
            text = raw.decode("gb18030")
    require("\x00" not in text, "TEXT_HAS_BINARY_CONTROL_BYTES")
    found = {}
    for name, pattern in PATTERNS.items():
        matches = list(pattern.finditer(text))
        if name == "credential_assignment":
            matches = [m for m in matches if m.group(1).lower() not in NON_SECRETS
                       and not m.group(1).startswith(("${", "%"))]
        if matches:
            found[name] = len(matches)
    return found


def secret_scan(files):
    scanned, findings, not_read, binary = [], [], [], []
    candidates = [(rel, WINDOWS / rel, row["bytes"]) for rel, row in files.items()
                  if rel not in EXCLUDED and rel not in DOCUMENTS.values()]
    candidates += [(target, RELEASE / source, (RELEASE / source).stat().st_size)
                   for source, target in DOCUMENTS.items()]
    total = 0
    for rel, path, size in candidates:
        is_text = path.suffix.lower() in TEXT_SUFFIXES or path.name.upper().startswith(("LICENSE", "NOTICE", "OFL"))
        if not is_text:
            binary.append(rel)
            continue
        if size > 1024 ** 2 or total + size > 8 * 1024 ** 2:
            not_read.append({"path": rel, "state": "NOT_READ_SIZE_BOUND"})
            continue
        total += size
        try:
            hit = scan_text(path)
        except (UnicodeError, RuntimeError):
            not_read.append({"path": rel, "state": "NOT_READ_TEXT_DECODE"})
            continue
        scanned.append(rel)
        if hit:
            findings.append({"path": rel, "rules_and_counts": hit})
    return {"status": "BOUNDED_TEXT_CLEAN" if not findings and not not_read else "FAILED_OR_INCOMPLETE",
            "scanned": scanned, "bytes_read": total, "findings": findings, "not_read": not_read,
            "binary_metadata_only": binary, "matched_values_recorded": False, "binary_secret_scan": "NOT_RUN"}


def water_geometry():
    data = json_read(WINDOWS / WATER_DATA)
    source = json_read(WATER_SOURCE_REPORT)
    geometry = source.get("geometry", {})
    require(data.get("schema_version") == 1 and data.get("source_id") == "lake_west"
            and data.get("mesh") == WATER_MESH and data.get("required_actor_tag") == "water"
            and data.get("event_id") == "environment.water.lake_west", "WATER_GEOMETRY_IDENTITY_MISMATCH")
    require(source.get("status") == "READ_SAVED_METADATA_GAMEPLAY_ACTIVITY_NOT_PROVEN"
            and source.get("mesh", {}).get("path") == WATER_MESH
            and source.get("mesh", {}).get("class") == "/Script/Engine.StaticMesh"
            and geometry.get("state") == geometry.get("vertices_state") == geometry.get("triangles_state") == "READ"
            and source.get("asset_saves_requested") == 0 and source.get("asset_setters_called") == []
            and source.get("map_open_requested") is False and source.get("pie_start_requested") is False,
            "WATER_SOURCE_REPORT_NOT_ACTUAL_READ")
    require(geometry.get("vertex_count") == 98 and geometry.get("triangle_count") == 96
            and len(data.get("vertices_local_cm", [])) == 98 and len(data.get("triangles", [])) == 96
            and data.get("vertices_local_cm") == geometry.get("vertices_local_cm")
            and data.get("triangles") == geometry.get("triangles")
            and data.get("geometry_read_engine_version") == source.get("engine_version"),
            "WATER_GEOMETRY_SOURCE_PAIRING_MISMATCH")
    return {"state": "ACTUAL_READ_REPORT_PAIRED", "runtime_data": WATER_DATA, "mesh": WATER_MESH,
            "vertices": 98, "triangles": 96,
            "source_report": WATER_SOURCE_REPORT.relative_to(PROJECT).as_posix(),
            "source_report_bundled": False, "hashes_computed": 0,
            "licence_or_gameplay_activity_proven": False}


def check_closure(files):
    for rel in REQUIRED + LICENSES:
        require(rel in files and files[rel]["bytes"] > 0, "REQUIRED_RUNTIME_OR_LICENCE_MISSING")
    events = json_read(WINDOWS / "Hearthward/Resources/Data/experience.json")["sound_events"]
    wavs = sorted({"Hearthward/Resources/Audio/" + e["file"] for e in events})
    ids = {e["event_id"]: e["file"] for e in events}
    require(len(wavs) == EXPECTED_WAVS and len(events) == len(ids) == EXPECTED_EVENTS,
            "ACTUAL_AUDIO_EVENT_SCOPE_CHANGED")
    require(ids.get("environment.water.lake_west") == "TASK-099/candidate-water-west.wav"
            and ids.get("combat.swing") == "TASK-099/candidate-knifeSlice.wav", "FINAL_AUDIO_BINDING_MISMATCH")
    for rel in wavs:
        require(".." not in PurePosixPath(rel).parts and rel in files and files[rel]["bytes"] > 0,
                "ACTUAL_AUDIO_DEPENDENCY_MISSING")
    water = water_geometry()
    return {"required_licences": LICENSES, "actual_sound_events": len(events), "actual_unique_wavs": wavs,
            "water_geometry": water,
            "licence_file_presence": "PRESENT", "content_provenance": "UNKNOWN_GAPS_REMAIN_TASK_094",
            "full_licence_acceptance": "NOT_PASSED", "owner_audio_audition": "NOT_RUN"}


def documents():
    for source, target in DOCUMENTS.items():
        p = RELEASE / source
        require(p.is_file() and p.stat().st_size <= 1024 ** 2, "DOCUMENT_MISSING_OR_TOO_LARGE")
        if target != "BUILD-INFO.json":
            text = p.read_text(encoding="utf-8-sig")
            require(CANDIDATE in text and (target.endswith(".cmd") or VERSION in text),
                    "DOCUMENT_CANDIDATE_MISMATCH")
            if target.endswith(".cmd"):
                backend = "cpu" if "CPU" in target else "vulkan"
                require("-HearthwardAIBackend=" + backend in text and "-HearthwardAIGpuLayers=16" in text
                        and "%LOCALAPPDATA%" in text and "python" not in text.lower(), "LAUNCHER_ARGUMENT_MISMATCH")


def install(record_path, refresh=False):
    if refresh:
        record = json_read(record_path)
        require(record.get("candidate_id") == CANDIDATE, "INSTALL_RECORD_CANDIDATE_MISMATCH")
        for target in DOCUMENTS.values():
            require(record["files"].get(target) == metadata(WINDOWS / target), "INSTALLED_DOCUMENT_CHANGED")
    else:
        require(not record_path.exists() and all(not (WINDOWS / t).exists() for t in DOCUMENTS.values()),
                "UNOWNED_INSTALL_TARGET_EXISTS")
    installed = {}
    for source, target in DOCUMENTS.items():
        with (WINDOWS / target).open("wb" if refresh else "xb") as f:
            f.write((RELEASE / source).read_bytes())
        installed[target] = metadata(WINDOWS / target)
    record = {"candidate_id": CANDIDATE,
              "owned_install": record["owned_install"] if refresh else str(uuid.uuid4()), "files": installed,
              "source_documents": {source: metadata(RELEASE / source) for source in DOCUMENTS},
              "hashes_computed": 0, "state": "REFRESHED_FROM_FINAL_SOURCE_DOCS" if refresh else "INSTALLED_FOR_OS_VALIDATION"}
    if refresh:
        record_path.write_text(json.dumps(record, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    else:
        json_new(record_path, record)


def final_sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(8 * 1024 ** 2), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--install", action="store_true")
    modes.add_argument("--execute", action="store_true")
    args = parser.parse_args()
    if not (args.install or args.execute):
        print(json.dumps({"status": "PREPARED_NOT_EXECUTED", "windows": str(WINDOWS), "zip": str(ZIP),
                          "evidence": str(EVIDENCE), "modes": ["--install", "--execute"]}))
        return
    check_build(args.execute)
    documents()
    files = tree()
    closure = check_closure(files)
    scan = secret_scan(files)
    if scan["status"] != "BOUNDED_TEXT_CLEAN":
        print(json.dumps({"status": scan["status"], "findings": scan["findings"], "not_read": scan["not_read"]}))
    require(scan["status"] == "BOUNDED_TEXT_CLEAN", "SECRET_SCAN_FAILED_OR_INCOMPLETE")
    record = EVIDENCE / "document-install.json"
    if args.install:
        install(record)
        print(json.dumps({"status": "INSTALLED_FOR_ROOT_OS_VALIDATION", "candidate": CANDIDATE,
                          "copied": list(DOCUMENTS.values()), "zip_created": False}))
        return
    outputs = [EVIDENCE / "finalization-result.json", EVIDENCE / "final-file-manifest.json",
               EVIDENCE / "bounded-secret-scan.json", WINDOWS / "FILE-MANIFEST.json", ZIP,
               Path(str(ZIP) + ".sha256")]
    require(all(not p.exists() for p in outputs), "FINAL_OUTPUT_ALREADY_EXISTS")
    exact_windows_path(ZIP, "F:/HearthwardDemo/" + CANDIDATE + ".zip")
    result = {"candidate_id": CANDIDATE, "status": "STARTED", "published": False,
              "source_qa_profiles_included": False, "sha256_computations": 0}
    json_new(outputs[0], result)
    try:
        install(record, refresh=record.exists())
        removed = []
        for rel in sorted(EXCLUDED):
            p = WINDOWS / rel
            require(p.resolve().is_relative_to(WINDOWS.resolve()), "EXCLUSION_ESCAPED_WINDOWS")
            if p.exists():
                p.unlink()
                removed.append(rel)
        payload = tree()
        manifest = {"candidate_id": CANDIDATE, "scope": "Runtime files excluding this manifest and external ZIP/hash",
                    "files": [{"path": rel, **row} for rel, row in sorted(payload.items())],
                    "count": len(payload), "bytes": sum(x["bytes"] for x in payload.values()),
                    "removed_exact_production_notes": removed, "hashes_computed": 0, "closure": closure}
        json_new(outputs[1], manifest)
        json_new(outputs[2], scan)
        json_new(outputs[3], manifest)
        zipped = tree()
        with zipfile.ZipFile(ZIP, "x", allowZip64=True, compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
            for rel in sorted(zipped):
                compression = zipfile.ZIP_STORED if Path(rel).suffix.lower() in {".gguf", ".pak", ".ucas", ".utoc"} else zipfile.ZIP_DEFLATED
                archive.write(WINDOWS / rel, arcname=rel, compress_type=compression)
        require(tree() == zipped, "RUNTIME_TREE_CHANGED_DURING_ZIP")
        with zipfile.ZipFile(ZIP, "r") as archive:
            actual = {entry.filename: entry.file_size for entry in archive.infolist()}
        require(actual == {rel: row["bytes"] for rel, row in zipped.items()}, "ZIP_METADATA_MISMATCH")
        result["sha256_computations"] = 1
        digest = final_sha256(ZIP)
        with outputs[5].open("x", encoding="ascii") as f:
            f.write(digest + "  " + ZIP.name + "\n")
        result.update({"status": "INTERNAL_CANDIDATE_ZIP_CREATED", "zip": str(ZIP), "zip_bytes": ZIP.stat().st_size,
                       "sha256": digest, "runtime_files": len(zipped), "closure": closure,
                       "formal_acceptance": "NOT_PASSED", "zip_payload_hashes": 0})
    except Exception as error:
        result.update({"status": "FAILED_PARTIAL_ARTIFACTS_PRESERVED", "error_type": type(error).__name__,
                       "error_code": str(error) if isinstance(error, RuntimeError) else "IO_OR_FORMAT_ERROR"})
        raise
    finally:
        outputs[0].write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": result["status"], "zip_bytes": result["zip_bytes"],
                      "runtime_files": result["runtime_files"], "sha256_computations": 1}))


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(json.dumps({"status": "FAILED", "error_type": type(error).__name__,
                          "error_code": str(error) if isinstance(error, RuntimeError) else "IO_OR_FORMAT_ERROR"}))
        raise SystemExit(1)
