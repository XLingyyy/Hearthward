"""Inspect and export the TASK-028 floor texture without editing assets."""

import json
import traceback
from pathlib import Path

import unreal


ROOT = Path(unreal.Paths.project_dir()).resolve()
OUT = ROOT / "Saved/Task028/material-diagnostic.json"
try:
    stem = "/Game/Hearthward/Assets/TASK-028/house/wood_floor_panel/"
    mesh = unreal.load_asset(stem + "SM_wood_floor_panel")
    material = unreal.load_asset(stem + "tripo_mat_91c736a2")
    texture = unreal.load_asset(stem + "Color")
    task = unreal.AssetExportTask()
    task.object = texture
    task.filename = str(ROOT / "Saved/Task028/floor-color.png")
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    task.exporter = unreal.TextureExporterPNG()
    exported = unreal.Exporter.run_asset_export_task(task)
    result = {
        "ok": True,
        "mesh_material": mesh.static_materials[0].material_interface.get_path_name(),
        "material": material.get_path_name(),
        "material_class": material.get_class().get_name(),
        "texture": texture.get_path_name(),
        "exported": bool(exported),
    }
except Exception:
    result = {"ok": False, "error": traceback.format_exc()}
OUT.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
