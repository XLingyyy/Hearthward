"""Create plain backing materials for holes in the source floor/roof meshes."""

import json
import traceback
from pathlib import Path

import unreal


ROOT = Path(unreal.Paths.project_dir()).resolve()
OUT = ROOT / "Saved/Task028/backing-materials.json"
PACKAGE = "/Game/Hearthward/Assets/TASK-028/house/backing"
definitions = {
    "M_FloorBacking": (0.28, 0.25, 0.22),
    "M_RoofBacking": (0.18, 0.15, 0.12),
}
try:
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    rows = []
    for name, rgb in definitions.items():
        path = PACKAGE + "/" + name
        assert not unreal.EditorAssetLibrary.does_asset_exist(path), path
        material = tools.create_asset(name, PACKAGE, unreal.Material, unreal.MaterialFactoryNew())
        assert material, path
        color = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionConstant3Vector, -300, 0)
        color.set_editor_property("constant", unreal.LinearColor(*rgb))
        unreal.MaterialEditingLibrary.connect_material_property(
            color, "", unreal.MaterialProperty.MP_BASE_COLOR)
        roughness = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionConstant, -300, 150)
        roughness.set_editor_property("r", 0.9)
        unreal.MaterialEditingLibrary.connect_material_property(
            roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
        unreal.MaterialEditingLibrary.recompile_material(material)
        assert unreal.EditorAssetLibrary.save_loaded_asset(material), path
        rows.append({"asset": path, "base_color_linear": rgb, "roughness": 0.9})
    result = {"ok": True, "assets": rows,
              "reason": "Only non-colliding backing cubes use these materials; source meshes retain imported materials"}
except Exception:
    result = {"ok": False, "error": traceback.format_exc()}
OUT.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
if not result["ok"]:
    raise RuntimeError(result["error"])
