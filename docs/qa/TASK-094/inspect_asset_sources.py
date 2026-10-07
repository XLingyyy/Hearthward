"""Root-run read-only AssetRegistry/ImportData inspection of the exact TASK-094 manifest.

Use an owned NullRHI Editor and -Task094Run=<new-label>. This script never opens a
map, imports/saves a package, enumerates Content/source directories or starts PIE.
"""
from collections import Counter
from datetime import datetime, timezone
import json
from pathlib import Path
import re
from urllib.parse import urlsplit, urlunsplit
from uuid import uuid4


IMPORTABLE_CLASSES = {
    "StaticMesh", "SkeletalMesh", "Texture2D", "TextureCube", "Texture2DArray",
    "VolumeTexture", "AnimSequence",
}


def safe_url(value):
    parts = urlsplit(value)
    # Retain only origin/path. Credentials, signed query parameters and fragments
    # have no role in pairing a local imported source to its runtime package.
    hostname = parts.hostname or ""
    if ":" in hostname:
        hostname = "[" + hostname + "]"
    if parts.port:
        hostname += ":" + str(parts.port)
    segments = parts.path.split("/")
    for index, segment in enumerate(segments):
        if re.match(r"(?i)(api[-_]?key|access[-_]?token|token|secret|signature)=", segment):
            segments[index] = "REDACTED"
        elif segment.lower() in {"token", "api_key", "apikey", "access_token", "secret", "key"} and index+1 < len(segments):
            segments[index+1] = "REDACTED"
    return urlunsplit((parts.scheme, hostname, "/".join(segments), "", ""))


def safe_text(value):
    return re.sub(r"https?://[^\s\"'<>]+", lambda match: safe_url(match[0]), str(value))


def source_location(value, project):
    """Stat only an exact source inside art_source; external user paths stay opaque."""
    raw = str(value)
    if re.match(r"(?i)^https?://", raw):
        return {"source_url":safe_url(raw), "state":"URL_METADATA_ONLY", "bytes":None,
                "licence_state":"UNKNOWN"}
    path = Path(raw)
    if not path.is_absolute():
        return {"filename":path.name, "relative_filename":safe_text(raw),
                "state":"NOT_READ_UNRESOLVED_RELATIVE_PATH", "bytes":None}
    lexical_allowed = project/"art_source"
    if not path.is_relative_to(lexical_allowed) or any(part.lower() in {"fonts","runtime","saved"} for part in path.parts):
        return {"filename":path.name, "state":"NOT_READ_OUTSIDE_ART_SOURCE", "bytes":None}
    resolved = path.resolve()
    allowed = lexical_allowed.resolve()
    if not resolved.is_relative_to(allowed) or any(part.lower() in {"fonts","runtime","saved"} for part in resolved.parts):
        return {"filename":path.name, "state":"NOT_READ_OUTSIDE_ART_SOURCE", "bytes":None}
    relative = resolved.relative_to(project).as_posix()
    try:
        info = resolved.stat()
        if not resolved.is_file():
            return {"project_relative_source":relative, "state":"NOT_A_FILE", "bytes":None}
        return {"project_relative_source":relative, "state":"PRESENT", "bytes":info.st_size}
    except FileNotFoundError:
        return {"project_relative_source":relative, "state":"MISSING", "bytes":None}
    except OSError as exc:
        return {"project_relative_source":relative, "state":"NOT_READ", "bytes":None,
                "error_kind":type(exc).__name__}


def relative_name(value):
    raw = str(value)
    if re.match(r"(?i)^https?://", raw):
        return safe_url(raw)
    # Absolute external import names are deliberately reduced to a basename.
    return Path(raw).name if Path(raw).is_absolute() or any(part.lower() == "users" for part in Path(raw).parts) else safe_text(raw)


def dependency_read(registry, package, options, referencers=False):
    try:
        values = (registry.get_referencers(package, options) if referencers
                  else registry.get_dependencies(package, options))
        if values is None:
            return {"state":"NOT_READ", "count":None, "packages":None,
                    "reason":"AssetRegistry reported no readable dependency node"}
        if isinstance(values, (str, bytes, bool)):
            raise TypeError("Unexpected AssetRegistry array return type")
        result = sorted({str(value) for value in values})
        if any(not value.startswith("/") for value in result):
            raise ValueError("AssetRegistry returned a non-package dependency")
        return {"state":"READ", "count":len(result), "packages":result}
    except Exception as exc:
        return {"state":"NOT_READ", "count":None, "packages":None,
                "error_kind":type(exc).__name__}


def source_tag_read(data):
    try:
        text = data.get_tag_value("SourceFile")
        if text is None:
            return {"state":"NO_SOURCEFILE_TAG", "count":0, "files":[]}
        rows = json.loads(text)
        if not isinstance(rows, list) or any(not isinstance(row, dict) or "RelativeFilename" not in row or not isinstance(row["RelativeFilename"], str) for row in rows):
            raise ValueError("SourceFile is not the documented source-file array")
        return {"state":"READ", "count":len(rows),
                "files":[{"relative_filename":relative_name(row["RelativeFilename"])} for row in rows]}
    except Exception as exc:
        return {"state":"NOT_READ", "count":None, "files":None,
                "error_kind":type(exc).__name__}


def import_read(data, class_name, project):
    if class_name not in IMPORTABLE_CLASSES:
        return {"state":"NOT_APPLICABLE_TO_CLASS", "class_name":class_name,
                "count":None, "files":None, "pairing_state":"UNKNOWN"}
    try:
        asset = data.get_asset()
        if asset is None:
            raise RuntimeError("Exact importable AssetData did not load")
        import_data = asset.get_editor_property("asset_import_data")
        if import_data is None:
            return {"state":"NO_IMPORT_DATA_RECORDED", "count":0, "files":[],
                    "pairing_state":"UNKNOWN"}
        values = import_data.extract_filenames()
        if values is None or isinstance(values, (str, bytes, bool)):
            raise TypeError("Unexpected ImportData filenames array return type")
        if any(not isinstance(value, str) for value in values):
            raise TypeError("ImportData returned a non-string source filename")
        files = [source_location(value, project) for value in values]
        return {"state":"READ", "class_name":import_data.get_class().get_name(),
                "count":len(files), "files":files,
                "pairing_state":"IMPORTED_SOURCE_PATHS_RECORDED" if files else "UNKNOWN",
                "licence_inference":"NONE; import paths and file presence do not establish author/terms"}
    except Exception as exc:
        return {"state":"NOT_READ", "count":None, "files":None,
                "pairing_state":"UNKNOWN", "error_kind":type(exc).__name__}


def inspect_package(unreal, registry, target, options, project, registry_ready):
    package = target["runtime_package"]
    result = {"runtime_package":package, "content_path":target["content_path"],
              "asset_id":target["asset_id"], "ledger_source_pairing":target["source_pairing"],
              "ledger_licence_state":target["licence_state"], "ledger_release_blocker":target["release_blocker"],
              "registry_state":"NOT_READ", "assets":None, "asset_count":None,
              "dependencies":{"state":"NOT_READ","count":None,"packages":None},
              "referencers":{"state":"NOT_READ","count":None,"packages":None},
              "map_runtime_use":"NOT_EVALUATED; serialized package references only"}
    if not registry_ready:
        result["reason"] = "AssetRegistry is still loading; no zero-reference inference"
        return result
    try:
        assets = registry.get_assets_by_package_name(package, include_only_on_disk_assets=True)
        if assets is None or isinstance(assets, (str, bytes, bool)):
            raise TypeError("Unexpected on-disk AssetData array return type")
        assets = list(assets)
        if not assets:
            result["reason"] = "No exact on-disk AssetData; no zero-reference inference"
            return result
        result["registry_state"] = "READ"
        result["asset_count"] = len(assets)
        result["assets"] = []
        for data in assets:
            if not data.is_valid() or str(data.package_name) != package:
                raise ValueError("Exact AssetData identity is invalid or mismatched")
            class_name = str(data.asset_class_path.asset_name)
            result["assets"].append({"asset_name":str(data.asset_name), "class_name":class_name,
                "source_file_tag":source_tag_read(data), "import_data":import_read(data,class_name,project)})
    except Exception as exc:
        result.update(registry_state="NOT_READ", asset_count=None, assets=None, error_kind=type(exc).__name__)
        return result
    result["dependencies"] = dependency_read(registry,package,options)
    result["referencers"] = dependency_read(registry,package,options,referencers=True)
    return result


def main():
    import unreal
    project = Path(unreal.Paths.project_dir()).resolve()
    manifest_path = project/"docs/qa/TASK-094/ASSET_INSPECTION_TARGETS.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    selected = manifest["packages"]
    packages = {row["runtime_package"] for row in selected}
    if len(selected) != 150 or len(packages) != 150 or len(manifest["unresolved_requirements"]) != 10:
        raise ValueError("The frozen 150-package/10-unresolved inspection denominator changed")
    if any(not name.startswith("/Game/") or any(part.lower() in {"runtime","saved","fonts"} for part in name.split("/")) for name in packages):
        raise ValueError("An inspection package is outside the approved scope")
    maps = {row["runtime_package"] for row in manifest["entry_maps"]}
    if len(maps) != 2 or not maps.issubset(packages):
        raise ValueError("Exact entry maps are not included in the approved package manifest")
    command_line = unreal.SystemLibrary.get_command_line()
    match = re.search(r"(?i)(?:^|\s)-Task094Run=([^\s]+)", command_line)
    run = match[1].strip('"') if match else "asset-inspection-"+str(uuid4())
    if Path(run).name != run or run in {".",".."}:
        raise ValueError("Task094Run must be one new output directory name")
    out = project/".agent-local/qa/TASK-094"/run
    out.mkdir(parents=True,exist_ok=True)
    output = out/"asset-sources.json"
    if output.exists():
        raise FileExistsError("This run already contains an inspection result; choose a new Task094Run")
    report = {"task":"TASK-094", "run":run, "updated_at":datetime.now(timezone.utc).isoformat(),
              "status":"RUNNING", "project":str(project), "manifest":str(manifest_path),
              "selection":manifest["selection"], "unresolved_requirements":manifest["unresolved_requirements"],
              "package_writes":False, "maps_loaded_by_script":False, "directory_scans":False,
              "hashes_computed":0, "network_downloads":False, "licence_state_changes":False,
              "owner_visual_acceptance":"NOT_RUN", "runtime_visibility":"NOT_RUN", "packages":[]}
    try:
        registry = unreal.AssetRegistryHelpers.get_asset_registry()
        # Wait only for the editor's already-running discovery. No explicit search,
        # scan_paths or recursive get_assets_by_path is requested by this script.
        registry.wait_for_completion()
        registry_ready = not registry.is_loading_assets()
        report["registry_ready"] = registry_ready
        options = unreal.AssetRegistryDependencyOptions()
        for name, value in {"include_soft_package_references":True,"include_hard_package_references":True,
                            "include_game_package_references":True,"include_editor_only_package_references":True,
                            "include_searchable_names":False,"include_soft_management_references":False,
                            "include_hard_management_references":False}.items():
            options.set_editor_property(name,value)
        for target in selected:
            report["packages"].append(inspect_package(unreal,registry,target,options,project,registry_ready))
            output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
        by_package = {row["runtime_package"]:row for row in report["packages"]}
        report["entry_maps"] = []
        for name in sorted(maps):
            dependencies = by_package[name]["dependencies"]
            report["entry_maps"].append({"package":name,"dependencies":dependencies,
                "selected_direct_dependencies":sorted(packages.intersection(dependencies["packages"])) if dependencies["state"] == "READ" else None,
                "scope":"On-disk direct hard/soft package references only; no World load or transitive/WorldPartition/live visibility claim"})
        for row in report["packages"]:
            dependencies = row["dependencies"]
            row["selected_dependency_imports"] = None if dependencies["state"] != "READ" else [
                {"dependency":name,"registry_state":by_package[name]["registry_state"],
                 "import_data":None if by_package[name]["registry_state"] != "READ" else [asset["import_data"] for asset in by_package[name]["assets"]]}
                for name in dependencies["packages"] if name in by_package]
            row["dependency_pairing_scope"] = "Read-only dependency import evidence; not proof of geometry authorship or licence"
        report["registry_state_counts"] = dict(Counter(row["registry_state"] for row in report["packages"]))
        not_read = any(row["registry_state"] != "READ" or row["dependencies"]["state"] != "READ"
            or row["referencers"]["state"] != "READ" or any(asset["source_file_tag"]["state"] == "NOT_READ"
            or asset["import_data"]["state"] == "NOT_READ" for asset in row["assets"] or []) for row in report["packages"])
        report["status"] = "PARTIAL_NOT_READ" if not_read else "READ_COMPLETE_WITH_UNKNOWN_PROVENANCE"
    except Exception as exc:
        report.update(status="NOT_READ",error_kind=type(exc).__name__)
    finally:
        output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
        unreal.log("TASK094_INSPECTION "+json.dumps({"status":report["status"],"output":str(output),
            "packages_attempted":len(report["packages"]),"licence_approval":"NOT_EVALUATED"}))
    return report


if __name__ == "__main__":
    main()
