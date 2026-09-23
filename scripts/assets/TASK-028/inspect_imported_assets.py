"""Fresh-editor structural QA for TASK-028 imported static assets."""

import json
import traceback
from pathlib import Path

import unreal


ROOT = Path(unreal.Paths.project_dir()).resolve()
SOURCE = ROOT / "Saved/Task028/static-import-report.json"
OUT = ROOT / "Saved/Task028/reopen-inspection.json"


def main():
    imported = json.loads(SOURCE.read_text(encoding="utf-8"))
    assert imported["ok"] and len(imported["assets"]) == 15
    rows = []
    for row in imported["assets"]:
        for package in row["packages"]:
            assert unreal.EditorAssetLibrary.does_asset_exist(package), package
        mesh = unreal.load_asset(row["ue_asset"])
        assert isinstance(mesh, unreal.StaticMesh), row["ue_asset"]
        bounds = mesh.get_bounds()
        slots = mesh.static_materials
        assert slots and all(slot.material_interface for slot in slots), row["id"]
        inspected_textures = []
        for entry in row["textures"]:
            texture = unreal.load_asset(entry["path"])
            assert isinstance(texture, unreal.Texture2D), entry["path"]
            name = entry["path"].rsplit("/", 1)[-1]
            srgb = bool(texture.get_editor_property("srgb"))
            compression = str(texture.get_editor_property("compression_settings"))
            if name == "Normal":
                assert not srgb and "TC_NORMALMAP" in compression, entry["path"]
            if name == "Roughness":
                assert not srgb and "TC_MASKS" in compression, entry["path"]
            assert texture.get_editor_property("max_texture_size") == 2048
            inspected_textures.append({"path": entry["path"], "srgb": srgb,
                                       "compression": compression})
        rows.append({
            "id": row["id"], "ue_asset": row["ue_asset"],
            "bounds_origin_cm": [round(bounds.origin.x, 2), round(bounds.origin.y, 2),
                                 round(bounds.origin.z, 2)],
            "dimensions_cm": [round(bounds.box_extent.x * 2, 2),
                              round(bounds.box_extent.y * 2, 2),
                              round(bounds.box_extent.z * 2, 2)],
            "material_count": len(slots), "textures": inspected_textures,
        })
    return {"ok": True, "engine": "UE 5.8.2", "reopened_assets": len(rows), "assets": rows}


try:
    result = main()
except Exception:
    result = {"ok": False, "error": traceback.format_exc()}
OUT.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
if not result["ok"]:
    raise RuntimeError(result["error"])
