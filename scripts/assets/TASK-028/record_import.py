"""Bind the Unreal import reports to the reviewed TASK-028 asset manifest."""

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
SAVED = ROOT / "Saved/Task028"
EVIDENCE = ROOT / "docs/qa/evidence/TASK-028"
MANIFEST = ROOT / "docs/assets/TASK-028/asset-manifest.json"


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def main():
    manifest = read(MANIFEST)
    report = read(SAVED / "static-import-report.json")
    fixes = read(SAVED / "texture-fix-report.json")
    reopened = read(SAVED / "reopen-inspection.json")
    assert report["ok"] and fixes["ok"] and reopened["ok"]
    assert len(report["assets"]) == len(fixes["textures"]) == len(reopened["assets"]) == 15
    assets = {row["id"]: row for row in manifest["assets"]}
    fixed = {row["id"]: row for row in fixes["textures"]}
    inspected = {row["id"]: row for row in reopened["assets"]}
    for imported in report["assets"]:
        row = assets[imported["id"]]
        assert row["source"] == imported["source"]
        assert row["source_sha256"] == imported["source_sha256"]
        assert row["planned_ue_asset"] == imported["ue_asset"]
        assert row["status"] in {"source_available", "imported"}
        assert fixed[row["id"]]["srgb"] is False
        assert "TC_MASKS" in fixed[row["id"]]["compression"]
        assert inspected[row["id"]]["ue_asset"] == imported["ue_asset"]
        for package in imported["packages"]:
            path = ROOT / ("Content/" + package.removeprefix("/Game/") + ".uasset")
            assert path.is_file(), path
        row["ue_asset"] = imported["ue_asset"]
        row["materials_textures"] = {
            "materials": imported["materials"],
            "textures": [t["path"] for t in imported["textures"]],
            "texture_max_size": 2048,
            "roughness_linear": True,
        }
        row["scale_axis_pivot"] = {
            "unscaled_bounds_cm": imported["dimensions_cm"],
            "bounds_origin_cm": inspected[row["id"]]["bounds_origin_cm"],
            "scale": "Unverified for game application",
            "axis": "Unverified",
            "pivot": "Unverified",
        }
        row["collision"] = {
            "simple_count": imported["simple_collision_count"],
            "strategy": "Pending assembly and actual walk-through",
        }
        row["lod_nanite"] = {
            "lod_count": imported["lod_count"],
            "nanite": "Unverified; no blanket enablement",
        }
        row["status"] = "imported"
        row["verification"] = [
            "UE 5.8.2 static FBX import succeeded and packages saved",
            "Fresh UE Editor reopen verified all packages, material slots, normal and roughness texture settings",
            "Material slots and package closure recorded in static-import-report.json",
            "Roughness textures changed to linear TC_MASKS; normal textures imported as TC_NORMALMAP",
            "Scale, collision, gameplay and visual acceptance remain open",
        ]
    write(MANIFEST, manifest)
    write(EVIDENCE / "static-import-report.json", report)
    write(EVIDENCE / "texture-fix-report.json", fixes)
    write(EVIDENCE / "reopen-inspection.json", reopened)
    build = read(SAVED / "baseline-build.json")
    assert build["ok"], build["errors"]
    write(EVIDENCE / "baseline-build-summary.json", {
        "ok": build["ok"], "operation": build["operation"],
        "target": build["payload"]["target"],
        "configuration": build["payload"]["configuration"],
        "returncode": build["payload"]["returncode"],
        "baseline_commit": manifest["baseline_commit"],
        "ue_root": "E:/UE_5.8", "engine_version": "5.8.2",
    })
    print("Recorded 15 imported assets and UE 5.8.2 build evidence")


if __name__ == "__main__":
    main()
