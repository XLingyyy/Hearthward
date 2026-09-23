"""Run inside Unreal Editor to correct Tripo roughness color-space imports."""

import json
import traceback
from pathlib import Path

import unreal


ROOT = Path(unreal.Paths.project_dir()).resolve()
REPORT = ROOT / "Saved/Task028/static-import-report.json"
OUT = ROOT / "Saved/Task028/texture-fix-report.json"


def main():
    imported = json.loads(REPORT.read_text(encoding="utf-8"))
    assert imported["ok"] and len(imported["assets"]) == 15
    result = []
    for row in imported["assets"]:
        rough = next(t for t in row["textures"] if t["path"].endswith("/Roughness"))
        texture = unreal.load_asset(rough["path"])
        assert isinstance(texture, unreal.Texture2D), rough["path"]
        texture.set_editor_properties(dict(
            srgb=False,
            compression_settings=unreal.TextureCompressionSettings.TC_MASKS,
            max_texture_size=2048,
        ))
        assert unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=True)
        result.append({"id": row["id"], "roughness": rough["path"],
                       "srgb": bool(texture.get_editor_property("srgb")),
                       "compression": str(texture.get_editor_property("compression_settings"))})
    return {"ok": True, "textures": result}


try:
    data = main()
except Exception:
    data = {"ok": False, "error": traceback.format_exc()}
OUT.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
if not data["ok"]:
    raise RuntimeError(data["error"])
