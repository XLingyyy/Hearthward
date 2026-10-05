"""Root-run sample correction after exact TASK070 asset registration and locks.

Creates only M_Spear_PBR/M_ShortBlade_PBR, reuses all four actual imported
textures, changes each Roughness/Metallic texture to linear masks, binds slot 0,
and removes the confirmed asset hulls. Never edits the imported MIC or parent.
"""
from datetime import datetime, timezone
import json
from pathlib import Path
import sys
import traceback

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
from probe_imported_weapons import material_info, object_info

HERE = Path(__file__).resolve().parent
MANIFEST = json.loads((HERE / "first-weapon-import-manifest.json").read_text(encoding="utf-8-sig"))
OUTPUT = Path(unreal.Paths.project_saved_dir()) / "Task070" / "equipment-sample-pbr-fix.json"
NAMES = {"spear": "M_Spear_PBR", "shortblade": "M_ShortBlade_PBR"}
CHANNELS = (
    ("BaseColor", "Color", unreal.MaterialProperty.MP_BASE_COLOR, "RGB", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR),
    ("Normal", "Normal", unreal.MaterialProperty.MP_NORMAL, "RGB", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL),
    ("Roughness", "Roughness", unreal.MaterialProperty.MP_ROUGHNESS, "R", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS),
    ("Metallic", "Metallic", unreal.MaterialProperty.MP_METALLIC, "R", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS),
)


def apply_samples():
    report = {"captured_utc": datetime.now(timezone.utc).isoformat(), "engine_version": unreal.SystemLibrary.get_engine_version(),
              "method": "Public material graph API, existing four textures, local sample materials only; slot0 and confirmed mesh collision removal; exact asset saves",
              "ok": False, "saved_assets": [], "pieces": [], "old_mic_and_parent_edited": False,
              "Owner_style": "pending", "hand_grip": "not measured"}
    save_assets = []
    try:
        prepared = []
        for entry in MANIFEST["imports"]:
            piece = entry["piece"]
            if piece not in NAMES:
                raise RuntimeError("Sample correction only permits spear and shortblade")
            material_path = entry["destination"] + "/" + NAMES[piece]
            if unreal.EditorAssetLibrary.does_asset_exist(material_path):
                raise RuntimeError("New material already exists; inspect that asset before retry: " + material_path)
            mesh = unreal.load_asset(entry["expected_mesh_package"])
            if not isinstance(mesh, unreal.StaticMesh) or len(mesh.get_editor_property("static_materials")) != 1:
                raise RuntimeError("Actual sample mesh must have exactly one material slot")
            textures = {name: unreal.load_asset(entry["destination"] + "/" + name) for name in ("Color", "Normal", "Roughness", "Metallic")}
            if any(not isinstance(texture, unreal.Texture2D) for texture in textures.values()):
                raise RuntimeError("Required texture missing; resolve through public import_texture")
            if not textures["Color"].get_editor_property("srgb"):
                raise RuntimeError("Color texture has changed since the actual import probe")
            if textures["Normal"].get_editor_property("srgb") or textures["Normal"].get_editor_property("compression_settings") != unreal.TextureCompressionSettings.TC_NORMALMAP:
                raise RuntimeError("Normal texture has changed since the actual import probe")
            prepared.append((entry, mesh, textures, material_path))
        if len(prepared) != 2:
            raise RuntimeError("Exactly the two approved samples are required")
        subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        lib = unreal.MaterialEditingLibrary
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        for entry, mesh, textures, material_path in prepared:
            piece = entry["piece"]
            item = {"piece": piece, "mesh": object_info(mesh), "old_slot0": object_info(mesh.get_material(0)),
                    "collision_before": {"simple": subsystem.get_simple_collision_count(mesh), "convex": subsystem.get_convex_collision_count(mesh)},
                    "texture_changes": []}
            report["pieces"].append(item)
            for name in ("Roughness", "Metallic"):
                texture = textures[name]
                before = {"srgb": bool(texture.get_editor_property("srgb")), "compression_settings": str(texture.get_editor_property("compression_settings"))}
                texture.set_editor_property("srgb", False)
                texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
                after = {"srgb": bool(texture.get_editor_property("srgb")), "compression_settings": str(texture.get_editor_property("compression_settings"))}
                if after["srgb"] or texture.get_editor_property("compression_settings") != unreal.TextureCompressionSettings.TC_MASKS:
                    raise RuntimeError("Linear mask texture settings did not persist in memory")
                item["texture_changes"].append({"texture": object_info(texture), "before": before, "after": after})
                save_assets.append(texture)
            material = tools.create_asset(NAMES[piece], entry["destination"], unreal.Material, unreal.MaterialFactoryNew())
            if not isinstance(material, unreal.Material) or material.get_path_name().split(".")[0] != material_path:
                raise RuntimeError("Failed to create exact local material package")
            material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
            item["new_material"] = object_info(material)
            item["connections"] = []
            for index, (parameter, name, prop, output, sampler) in enumerate(CHANNELS):
                expression = lib.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -650, index * 180)
                if expression is None:
                    raise RuntimeError("Failed to create texture parameter " + parameter)
                expression.set_editor_property("parameter_name", parameter)
                expression.set_editor_property("texture", textures[name])
                expression.set_editor_property("sampler_type", sampler)
                if not lib.connect_material_property(expression, output, prop):
                    raise RuntimeError("Failed material connection " + parameter)
                connected = lib.get_material_property_input_node(material, prop)
                connected_output = lib.get_material_property_input_node_output_name(material, prop)
                if connected != expression or connected_output != output or expression.get_editor_property("texture") != textures[name]:
                    raise RuntimeError("Material connection readback mismatch " + parameter)
                item["connections"].append({"parameter": parameter, "texture": object_info(textures[name]), "property": str(prop), "output": connected_output, "sampler": str(expression.get_editor_property("sampler_type"))})
            item["compile_errors"] = list(lib.recompile_material(material))
            if item["compile_errors"]:
                raise RuntimeError("Dedicated sample material compilation failed")
            mesh.set_material(0, material)
            if mesh.get_material(0) != material:
                raise RuntimeError("Actual mesh slot0 assignment mismatch")
            if item["collision_before"]["simple"] or item["collision_before"]["convex"]:
                item["remove_collisions_result"] = bool(subsystem.remove_collisions(mesh))
            item["collision_after"] = {"simple": subsystem.get_simple_collision_count(mesh), "convex": subsystem.get_convex_collision_count(mesh)}
            if item["collision_after"]["simple"] or item["collision_after"]["convex"]:
                raise RuntimeError("Actual mesh collision removal did not reach zero")
            item["actual_material_graph"] = material_info(material)
            save_assets.extend((material, mesh))
        unreal.SystemLibrary.execute_console_command(world, "Editor.AsyncTextureCompilationFinishAll")
        for asset in save_assets:
            if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=True):
                raise RuntimeError("Failed exact asset save: " + asset.get_path_name())
            report["saved_assets"].append(object_info(asset))
        report["ok"] = True
    except Exception:
        report["error"] = traceback.format_exc()
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.log("TASK070 dedicated sample material correction: " + str(OUTPUT))
    return report


if __name__ == "__main__":
    apply_samples()
    unreal.SystemLibrary.quit_editor()
