# -*- coding: utf-8 -*-
"""Import and inspect the ten Tripo FBX files in Unreal Engine 5.8.2."""

from __future__ import annotations

import json
import os
import traceback

import unreal


PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
REPORT_PATH = os.path.join(PROJECT_DIR, "ue582_import_report.json")
DESTINATION_ROOT = "/Game/TripoValidation"

ASSETS = [
    ("weapon_01_saber_a", r"E:\AiAgent\XLingGame\Resource\Tripo\武器\outputs\b39bb9c5-8ee6-484e-8ab1-e6c1ff5046e6\b39bb9c5-8ee6-484e-8ab1-e6c1ff5046e6_pbr.fbx"),
    ("weapon_02_saber_b", r"E:\AiAgent\XLingGame\Resource\Tripo\武器\outputs\8dd7cafa-fdd7-40cd-9724-a0fa113e72e9\8dd7cafa-fdd7-40cd-9724-a0fa113e72e9_pbr.fbx"),
    ("weapon_03_spear", r"E:\AiAgent\XLingGame\Resource\Tripo\武器\outputs\29322e3e-a4e3-4d17-9223-80d1f2d65f99\29322e3e-a4e3-4d17-9223-80d1f2d65f99_pbr.fbx"),
    ("weapon_04_mace", r"E:\AiAgent\XLingGame\Resource\Tripo\武器\outputs\c31fb95d-d93d-467b-bfa6-5e1e9a4bb2e3\c31fb95d-d93d-467b-bfa6-5e1e9a4bb2e3_pbr.fbx"),
    ("weapon_05_bow", r"E:\AiAgent\XLingGame\Resource\Tripo\武器\outputs\84cf4884-8d18-4737-a4f5-1b581e7eec13\84cf4884-8d18-4737-a4f5-1b581e7eec13_pbr.fbx"),
    ("weapon_06_crossbow", r"E:\AiAgent\XLingGame\Resource\Tripo\武器\outputs\c49a60bf-f3bf-4ee9-a854-145882c4ebdd\c49a60bf-f3bf-4ee9-a854-145882c4ebdd_pbr.fbx"),
    ("armor_07_hood", r"E:\AiAgent\XLingGame\Resource\Tripo\护具\outputs\a42d2caa-03d0-4d3e-b57b-88ae746453e2\a42d2caa-03d0-4d3e-b57b-88ae746453e2_pbr.fbx"),
    ("armor_08_leather_armor", r"E:\AiAgent\XLingGame\Resource\Tripo\护具\outputs\76f88975-3cc8-410c-9842-2ddd8765164b\76f88975-3cc8-410c-9842-2ddd8765164b_pbr.fbx"),
    ("armor_09_bracers", r"E:\AiAgent\XLingGame\Resource\Tripo\护具\outputs\afdffd2b-08be-462f-880b-e0c83454b953\afdffd2b-08be-462f-880b-e0c83454b953_pbr.fbx"),
    ("armor_10_boots", r"E:\AiAgent\XLingGame\Resource\Tripo\护具\outputs\64c15994-0225-4d07-96b4-7402c74acb94\64c15994-0225-4d07-96b4-7402c74acb94_pbr.fbx"),
]


def set_if_supported(obj, property_name, value):
    try:
        obj.set_editor_property(property_name, value)
        return True
    except Exception:
        return False


def safe_call(callable_obj, *args):
    try:
        return callable_obj(*args)
    except Exception:
        return None


def json_value(value):
    if value is None or isinstance(value, (str, int, float, bool)):
        return value
    if isinstance(value, (list, tuple)):
        return [json_value(item) for item in value]
    if isinstance(value, dict):
        return {str(key): json_value(item) for key, item in value.items()}
    if hasattr(value, "to_tuple"):
        return list(value.to_tuple())
    if hasattr(value, "get_path_name"):
        return value.get_path_name()
    return str(value)


def fbx_binary_version(path):
    try:
        with open(path, "rb") as handle:
            header = handle.read(27)
        if not header.startswith(b"Kaydara FBX Binary") or len(header) < 27:
            return None
        return int.from_bytes(header[23:27], "little")
    except Exception:
        return None


def vector_value(value):
    if value is None:
        return None
    try:
        return [float(value.x), float(value.y), float(value.z)]
    except Exception:
        return json_value(value)


def material_metrics(material):
    library = unreal.MaterialEditingLibrary
    used_textures = safe_call(library.get_material_used_textures, material) or []
    parent = safe_call(material.get_editor_property, "parent")
    parameter_names = safe_call(library.get_texture_parameter_names, material) or []
    texture_parameters = []
    for parameter_name in parameter_names:
        texture = safe_call(
            library.get_material_instance_texture_parameter_value,
            material,
            parameter_name,
        )
        texture_parameters.append(
            {
                "name": str(parameter_name),
                "texture": texture.get_path_name() if texture else None,
            }
        )
    raw_overrides = safe_call(material.get_editor_property, "texture_parameter_values") or []
    texture_parameter_overrides = []
    for override in raw_overrides:
        parameter_info = safe_call(override.get_editor_property, "parameter_info")
        parameter_value = safe_call(override.get_editor_property, "parameter_value")
        texture_parameter_overrides.append(
            {
                "parameter_info": str(parameter_info),
                "texture": parameter_value.get_path_name() if parameter_value else None,
            }
        )
    property_inputs = {}
    for name, property_value in (
        ("base_color", unreal.MaterialProperty.MP_BASE_COLOR),
        ("metallic", unreal.MaterialProperty.MP_METALLIC),
        ("roughness", unreal.MaterialProperty.MP_ROUGHNESS),
        ("normal", unreal.MaterialProperty.MP_NORMAL),
    ):
        node = safe_call(library.get_material_property_input_node, material, property_value)
        property_inputs[name] = node.get_class().get_name() if node else None
    compile_errors = []
    if isinstance(material, unreal.Material):
        compile_errors = safe_call(library.recompile_material, material) or []
    return {
        "asset_path": material.get_path_name(),
        "class": material.get_class().get_name(),
        "parent": parent.get_path_name() if parent else None,
        "used_textures": [texture.get_path_name() for texture in used_textures if texture],
        "used_texture_count": len(used_textures),
        "texture_parameters": texture_parameters,
        "texture_parameter_overrides": texture_parameter_overrides,
        "referenced_project_textures": [
            item["texture"]
            for item in texture_parameters
            if item.get("texture") and item["texture"].startswith("/Game/")
        ],
        "property_input_nodes": property_inputs,
        "compile_errors": [str(item) for item in compile_errors],
    }


def texture_metrics(texture):
    width = safe_call(texture.blueprint_get_size_x)
    height = safe_call(texture.blueprint_get_size_y)
    return {
        "asset_path": texture.get_path_name(),
        "class": texture.get_class().get_name(),
        "width": width,
        "height": height,
        "srgb": safe_call(texture.get_editor_property, "srgb"),
        "compression_settings": json_value(
            safe_call(texture.get_editor_property, "compression_settings")
        ),
        "lod_group": json_value(safe_call(texture.get_editor_property, "lod_group")),
        "virtual_texture_streaming": safe_call(
            texture.get_editor_property, "virtual_texture_streaming"
        ),
    }


def make_options():
    options = unreal.FbxImportUI()
    set_if_supported(options, "automated_import_should_detect_type", False)
    set_if_supported(options, "mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    set_if_supported(options, "original_import_type", unreal.FBXImportType.FBXIT_STATIC_MESH)
    set_if_supported(options, "import_mesh", True)
    set_if_supported(options, "import_animations", False)
    set_if_supported(options, "import_materials", True)
    set_if_supported(options, "import_textures", True)

    data = options.get_editor_property("static_mesh_import_data")
    applied = {
        "convert_scene": set_if_supported(data, "convert_scene", True),
        "convert_scene_unit": set_if_supported(data, "convert_scene_unit", True),
        "force_front_x_axis": set_if_supported(data, "force_front_x_axis", False),
        "combine_meshes": set_if_supported(data, "combine_meshes", True),
        "auto_generate_collision": set_if_supported(data, "auto_generate_collision", True),
        "generate_lightmap_u_vs": set_if_supported(data, "generate_lightmap_u_vs", True),
        "import_mesh_lods": set_if_supported(data, "import_mesh_lods", False),
        "remove_degenerates": set_if_supported(data, "remove_degenerates", True),
    }
    return options, applied


def static_mesh_metrics(mesh):
    bounds = safe_call(mesh.get_bounding_box)
    bounds_min = getattr(bounds, "min", None) if bounds else None
    bounds_max = getattr(bounds, "max", None) if bounds else None
    size = None
    if bounds_min is not None and bounds_max is not None:
        size = [
            float(bounds_max.x - bounds_min.x),
            float(bounds_max.y - bounds_min.y),
            float(bounds_max.z - bounds_min.z),
        ]
    materials = []
    try:
        for slot in mesh.get_editor_property("static_materials"):
            material = safe_call(slot.get_editor_property, "material_interface")
            materials.append(
                {
                    "slot_name": str(safe_call(slot.get_editor_property, "material_slot_name")),
                    "material": material.get_path_name() if material else None,
                }
            )
    except Exception:
        pass

    body_setup = None
    try:
        body_setup = mesh.get_editor_property("body_setup")
    except Exception:
        pass

    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)

    return {
        "asset_path": mesh.get_path_name(),
        "lod_count": safe_call(mesh.get_num_lods),
        "vertex_count_lod0": safe_call(mesh.get_num_vertices, 0),
        "triangle_count_lod0": safe_call(mesh.get_num_triangles, 0),
        "section_count_lod0": safe_call(mesh.get_num_sections, 0),
        "uv_channels_lod0": safe_call(mesh.get_num_tex_coords, 0),
        "material_slots": materials,
        "material_slot_count": len(materials),
        "bounds": {
            "min_cm": vector_value(bounds_min),
            "max_cm": vector_value(bounds_max),
            "size_cm": size,
        },
        "body_setup_present": body_setup is not None,
        "simple_collision_count": safe_call(subsystem.get_simple_collision_count, mesh),
        "convex_collision_count": safe_call(subsystem.get_convex_collision_count, mesh),
        "collision_complexity": json_value(safe_call(subsystem.get_collision_complexity, mesh)),
        "light_map_coordinate_index": safe_call(mesh.get_editor_property, "light_map_coordinate_index"),
    }


def import_one(label, source):
    destination = f"{DESTINATION_ROOT}/{label}"
    unreal.EditorAssetLibrary.make_directory(destination)
    options, applied_options = make_options()
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source)
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", label)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    object_paths = list(task.get_editor_property("imported_object_paths"))
    directory_paths = list(unreal.EditorAssetLibrary.list_assets(destination, recursive=True, include_folder=False))
    loaded = []
    static_meshes = []
    materials = []
    textures = []
    for object_path in directory_paths:
        asset = unreal.EditorAssetLibrary.load_asset(object_path)
        if not asset:
            continue
        asset_class = asset.get_class().get_name()
        loaded.append({"path": object_path, "class": asset_class})
        if isinstance(asset, unreal.StaticMesh):
            static_meshes.append(static_mesh_metrics(asset))
        elif isinstance(asset, unreal.MaterialInterface):
            materials.append(material_metrics(asset))
        elif isinstance(asset, unreal.Texture):
            textures.append(texture_metrics(asset))

    unreal.EditorAssetLibrary.save_directory(destination, only_if_is_dirty=False, recursive=True)
    import_issues = []
    readiness_warnings = []
    if not static_meshes:
        import_issues.append("No StaticMesh asset was imported")
    for mesh in static_meshes:
        if not mesh.get("vertex_count_lod0"):
            import_issues.append(f"{mesh['asset_path']}: no vertices reported")
        if not mesh.get("triangle_count_lod0"):
            import_issues.append(f"{mesh['asset_path']}: no triangles reported")
        if not mesh.get("uv_channels_lod0"):
            import_issues.append(f"{mesh['asset_path']}: no UV channels reported")
        if not mesh.get("material_slot_count"):
            import_issues.append(f"{mesh['asset_path']}: no material slots reported")
        if not mesh.get("body_setup_present"):
            import_issues.append(f"{mesh['asset_path']}: no body setup reported")
        if mesh.get("lod_count") == 1:
            readiness_warnings.append(f"{mesh['asset_path']}: only LOD0 is present")
    if not materials:
        import_issues.append("No Material asset was imported")
    if len(textures) < 4:
        import_issues.append(f"Expected four PBR textures, imported {len(textures)}")
    imported_texture_paths = {item["asset_path"] for item in textures}
    referenced_texture_paths = {
        path
        for material in materials
        for path in material.get("referenced_project_textures", [])
    }
    unreferenced_textures = sorted(imported_texture_paths - referenced_texture_paths)
    for material in materials:
        if material.get("compile_errors"):
            import_issues.append(f"{material['asset_path']}: material compile errors")
    if unreferenced_textures:
        readiness_warnings.append(
            "Imported textures not referenced by the generated material: "
            + ", ".join(unreferenced_textures)
        )
    source_fbx_version = fbx_binary_version(source)
    if source_fbx_version != 7700:
        readiness_warnings.append(
            f"FBX binary version is {source_fbx_version}; UE 5.8 documentation recommends FBX 2020.2 (7700)"
        )
    import_passed = bool(static_meshes) and not import_issues
    production_ready = import_passed and not readiness_warnings

    return {
        "label": label,
        "source": source,
        "source_exists": os.path.isfile(source),
        "source_bytes": os.path.getsize(source) if os.path.isfile(source) else 0,
        "fbx_binary_version": source_fbx_version,
        "destination": destination,
        "import_options_applied": applied_options,
        "imported_object_paths": object_paths,
        "imported_assets": loaded,
        "static_meshes": static_meshes,
        "materials": materials,
        "textures": textures,
        "unreferenced_textures": unreferenced_textures,
        "import_issues": import_issues,
        "readiness_warnings": readiness_warnings,
        "import_passed": import_passed,
        "production_ready": production_ready,
        "passed": import_passed,
    }


def main():
    report = {
        "schema": "tripo.ue582.static_mesh_import_validation.v1",
        "engine_version": unreal.SystemLibrary.get_engine_version(),
        "project": unreal.Paths.get_project_file_path(),
        "assets": [],
        "passed": False,
    }
    try:
        for label, source in ASSETS:
            unreal.log(f"[TripoValidation] Importing {label}: {source}")
            try:
                report["assets"].append(import_one(label, source))
            except Exception as exc:
                report["assets"].append(
                    {
                        "label": label,
                        "source": source,
                        "passed": False,
                        "error": f"{type(exc).__name__}: {exc}",
                        "traceback": traceback.format_exc(),
                    }
                )
        report["passed"] = len(report["assets"]) == len(ASSETS) and all(
            item.get("passed") for item in report["assets"]
        )
        report["production_ready"] = len(report["assets"]) == len(ASSETS) and all(
            item.get("production_ready") for item in report["assets"]
        )
    except Exception as exc:
        report["fatal_error"] = f"{type(exc).__name__}: {exc}"
        report["fatal_traceback"] = traceback.format_exc()
    finally:
        with open(REPORT_PATH, "w", encoding="utf-8") as handle:
            json.dump(report, handle, ensure_ascii=False, indent=2)
        unreal.log(f"[TripoValidation] Report: {REPORT_PATH}")
        unreal.log(f"[TripoValidation] Passed: {report['passed']}")


if __name__ == "__main__":
    main()
