"""Root-run draft: public Unreal Python reads only; writes one QA JSON report."""

from datetime import datetime, timezone
import json
from pathlib import Path

import unreal


DEFAULT_MANIFEST = Path(__file__).with_name("first-weapon-import-manifest.json")
CHANNELS = {
    "BaseColor": ("Color", unreal.MaterialProperty.MP_BASE_COLOR),
    "Normal": ("Normal", unreal.MaterialProperty.MP_NORMAL),
    "Roughness": ("Roughness", unreal.MaterialProperty.MP_ROUGHNESS),
    "Metallic": ("Metallic", unreal.MaterialProperty.MP_METALLIC),
}


def object_info(obj):
    if obj is None:
        return None
    return {"path": obj.get_path_name(), "actual_asset_class": obj.get_class().get_name()}


def vector(v):
    return [float(v.x), float(v.y), float(v.z)]


def rotation(r):
    return {"pitch": float(r.pitch), "yaw": float(r.yaw), "roll": float(r.roll)}


def import_data_info(asset):
    data = asset.get_editor_property("asset_import_data")
    if data is None:
        return None
    result = object_info(data)
    result["source_filenames"] = list(data.extract_filenames())
    class_name = result["actual_asset_class"]
    if class_name == "FbxStaticMeshImportData":
        result["legacy_fbx"] = {
            "import_translation": vector(data.get_editor_property("import_translation")),
            "import_rotation": rotation(data.get_editor_property("import_rotation")),
            "import_uniform_scale": data.get_editor_property("import_uniform_scale"),
            "convert_scene": data.get_editor_property("convert_scene"),
            "force_front_x_axis": data.get_editor_property("force_front_x_axis"),
            "convert_scene_unit": data.get_editor_property("convert_scene_unit"),
            "transform_vertex_to_absolute": data.get_editor_property("transform_vertex_to_absolute"),
            "bake_pivot_in_vertex": data.get_editor_property("bake_pivot_in_vertex"),
        }
    elif class_name == "InterchangeAssetImportData":
        settings = data.get_translator_settings()
        result["translator_settings"] = object_info(settings)
        if settings and settings.get_class().get_name() == "InterchangeFbxTranslatorSettings":
            result["translator_settings"].update({
                name: settings.get_editor_property(name)
                for name in ("convert_scene", "force_front_x_axis", "convert_scene_unit")
            })
        result["pipelines"] = []
        for pipeline in data.get_pipelines():
            item = object_info(pipeline)
            if pipeline.get_class().get_name() == "InterchangeGenericAssetsPipeline":
                item.update({
                    "import_offset_translation": vector(pipeline.get_editor_property("import_offset_translation")),
                    "import_offset_rotation": rotation(pipeline.get_editor_property("import_offset_rotation")),
                    "import_offset_uniform_scale": pipeline.get_editor_property("import_offset_uniform_scale"),
                })
                common = pipeline.get_editor_property("common_meshes_properties")
                item["common_meshes"] = {
                    "bake_meshes": common.get_editor_property("bake_meshes"),
                    "bake_pivot_meshes": common.get_editor_property("bake_pivot_meshes"),
                }
                mesh_pipeline = pipeline.get_editor_property("mesh_pipeline")
                item["collision_import"] = mesh_pipeline.get_editor_property("collision")
            result["pipelines"].append(item)
    return result


def material_info(material):
    result = object_info(material)
    lib = unreal.MaterialEditingLibrary
    if isinstance(material, unreal.MaterialInstanceConstant):
        parent = material.get_editor_property("parent")
        result.update({
            "graph_status": "MIC instance parameters; parent graph is read-only",
            "parent": object_info(parent),
            "explicit_texture_overrides": [],
            "explicit_scalar_overrides": [],
            "explicit_vector_overrides": [],
            "effective_global_texture_parameters": {},
            "effective_global_scalar_parameters": {},
            "effective_global_vector_parameters": {},
            "effective_global_static_switch_parameters": {},
            "parameter_query_scope": "Public name-based getters use GlobalParameter. Explicit overrides retain association/index; layer-specific inherited effective values are not measured.",
        })
        for kind in ("texture", "scalar", "vector"):
            for override in material.get_editor_property(kind + "_parameter_values"):
                info = override.get_editor_property("parameter_info")
                value = override.get_editor_property("parameter_value")
                if kind == "texture":
                    value = object_info(value)
                elif kind == "vector":
                    value = [float(value.r), float(value.g), float(value.b), float(value.a)]
                else:
                    value = float(value)
                result["explicit_" + kind + "_overrides"].append({
                    "name": str(info.get_editor_property("name")),
                    "association": str(info.get_editor_property("association")),
                    "index": int(info.get_editor_property("index")),
                    "value": value,
                })
        for name in lib.get_texture_parameter_names(material):
            result["effective_global_texture_parameters"][str(name)] = object_info(lib.get_material_instance_texture_parameter_value(material, name))
        for name in lib.get_scalar_parameter_names(material):
            result["effective_global_scalar_parameters"][str(name)] = float(lib.get_material_instance_scalar_parameter_value(material, name))
        for name in lib.get_vector_parameter_names(material):
            value = lib.get_material_instance_vector_parameter_value(material, name)
            result["effective_global_vector_parameters"][str(name)] = [float(value.r), float(value.g), float(value.b), float(value.a)]
        for name in lib.get_static_switch_parameter_names(material):
            result["effective_global_static_switch_parameters"][str(name)] = bool(lib.get_material_instance_static_switch_parameter_value(material, name))
        result["parent_details_read_only"] = material_info(parent) if parent else None
        return result
    if not isinstance(material, unreal.Material):
        result["graph_status"] = "not_UMaterial; direct property graph not inspected or rewritten"
        return result
    expressions = list(lib.get_material_expressions(material))
    result.update({
        "blend_mode": str(material.get_editor_property("blend_mode")),
        "material_domain": str(material.get_editor_property("material_domain")),
        "two_sided": bool(material.get_editor_property("two_sided")),
        "use_material_attributes": bool(material.get_editor_property("use_material_attributes")),
        "expressions": [],
        "property_inputs": {},
    })
    for expression in expressions:
        node = object_info(expression)
        input_nodes = list(lib.get_inputs_for_material_expression(material, expression))
        node["input_names"] = list(lib.get_material_expression_input_names(expression))
        node["input_nodes"] = [object_info(other) for other in input_nodes]
        node["input_output_names"] = [
            lib.get_input_node_output_name_for_material_expression(expression, other) if other else None
            for other in input_nodes
        ]
        if isinstance(expression, unreal.MaterialExpressionTextureBase):
            node["texture"] = object_info(expression.get_editor_property("texture"))
            node["sampler_type"] = str(expression.get_editor_property("sampler_type"))
        if isinstance(expression, (unreal.MaterialExpressionParameter, unreal.MaterialExpressionTextureSampleParameter)):
            node["parameter_name"] = str(expression.get_editor_property("parameter_name"))
        if isinstance(expression, unreal.MaterialExpressionMaterialFunctionCall):
            node["material_function"] = object_info(expression.get_editor_property("material_function"))
            node["function_internals"] = "not expanded; external function reference retained"
        result["expressions"].append(node)
    # These are the reflected surface inputs supported by GetExpressionInputDescription.
    # MP_MAX/hidden derived-lightmass/deprecated properties have no valid input pointer.
    safe_inputs = (
        "MP_BASE_COLOR", "MP_NORMAL", "MP_ROUGHNESS", "MP_METALLIC", "MP_SPECULAR",
        "MP_EMISSIVE_COLOR", "MP_OPACITY", "MP_OPACITY_MASK", "MP_ANISOTROPY",
        "MP_TANGENT", "MP_WORLD_POSITION_OFFSET", "MP_SUBSURFACE_COLOR",
        "MP_AMBIENT_OCCLUSION", "MP_REFRACTION", "MP_FRONT_MATERIAL", "MP_MATERIAL_ATTRIBUTES",
    )
    for name in safe_inputs:
        if not hasattr(unreal.MaterialProperty, name):
            result["property_inputs"][name] = {"status": "not exposed by this Python enum"}
            continue
        prop = getattr(unreal.MaterialProperty, name)
        node = lib.get_material_property_input_node(material, prop)
        result["property_inputs"][name] = {
            "node": object_info(node),
            "output_name": lib.get_material_property_input_node_output_name(material, prop) if node else None,
            "status": "connected" if node else "no expression connection; default value not measured",
        }
    result["hidden_input_coverage"] = "Hidden/customized-UV/custom-data/pixel-depth/shading-model inputs are not exposed by MaterialProperty; no numeric-enum workaround."
    return result


def mesh_info(mesh, subsystem):
    result = object_info(mesh)
    bounds = mesh.get_bounds()
    result["bounds_ue_local_cm"] = {
        "origin": vector(bounds.origin),
        "box_extent": vector(bounds.box_extent),
        "size": vector(bounds.box_extent * 2.0),
        "sphere_radius": float(bounds.sphere_radius),
    }
    result["lods"] = []
    for lod in range(mesh.get_num_lods()):
        section_count = mesh.get_num_sections(lod)
        result["lods"].append({
            "index": lod,
            "triangles": mesh.get_num_triangles(lod),
            "vertices": mesh.get_num_vertices(lod),
            "uv_channels": subsystem.get_num_uv_channels(mesh, lod),
            "render_section_count": section_count,
            "render_section_material_slots": [subsystem.get_lod_material_slot(mesh, lod, section) for section in range(section_count)],
        })
    result["material_slots"] = [
        {"index": index,
         "slot_name": str(slot.get_editor_property("material_slot_name")),
         "imported_slot_name": str(slot.get_editor_property("imported_material_slot_name")),
         "material": object_info(slot.get_editor_property("material_interface"))}
        for index, slot in enumerate(mesh.get_editor_property("static_materials"))
    ]
    result["assigned_local_material_graphs"] = {
        str(index): material_info(slot.get_editor_property("material_interface"))
        for index, slot in enumerate(mesh.get_editor_property("static_materials"))
        if isinstance(slot.get_editor_property("material_interface"), unreal.Material)
    }
    result["simple_collision_count"] = subsystem.get_simple_collision_count(mesh)
    result["convex_collision_count"] = subsystem.get_convex_collision_count(mesh)
    result["component_collision_status"] = "not measured: no component instantiated; zero asset hulls alone does not prove component NoCollision"
    result["sockets"] = {}
    for name in ("Grip", "GripSocket", "BladeBase", "BladeTip", "Tip"):
        socket = mesh.find_socket(name)
        item = object_info(socket)
        if socket:
            item.update({
                "relative_location_ue_local_cm": vector(socket.get_editor_property("relative_location")),
                "relative_rotation": rotation(socket.get_editor_property("relative_rotation")),
                "relative_scale": vector(socket.get_editor_property("relative_scale")),
            })
        result["sockets"][name] = item
    result["import_data"] = import_data_info(mesh)
    return result


def probe(manifest_path=DEFAULT_MANIFEST, output_path=None):
    manifest = json.loads(Path(manifest_path).read_text(encoding="utf-8-sig"))
    output_path = Path(output_path) if output_path else Path(unreal.Paths.project_saved_dir()) / "Task070" / "equipment-import-probe.json"
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    texture_reads_before_wait = {}
    loaded_textures = []
    for entry in manifest["imports"]:
        for name in ("Color", "Normal", "Roughness", "Metallic"):
            package = entry["destination"] + "/" + name
            texture = unreal.load_asset(package)
            if isinstance(texture, unreal.Texture2D):
                loaded_textures.append(texture)
                texture_reads_before_wait[package] = [texture.blueprint_get_size_x(), texture.blueprint_get_size_y()]
    unreal.SystemLibrary.execute_console_command(world, "Editor.AsyncTextureCompilationFinishAll")
    report = {
        "captured_utc": datetime.now(timezone.utc).isoformat(),
        "engine_version": unreal.SystemLibrary.get_engine_version(),
        "project_dir": unreal.Paths.project_dir(),
        "method": "public Unreal Python read-only imported-asset probe; no PIE, input, hand attachment, render or Owner style acceptance",
        "manifest": str(Path(manifest_path).resolve()),
        "texture_compilation_wait": {
            "method": "Editor.AsyncTextureCompilationFinishAll after preloading all eight project Texture2D objects; no property changes or asset saves",
            "reads_before_wait": texture_reads_before_wait,
            "note": "Blueprint_GetSize reads platform data and can return the async default placeholder before this wait; Dimensions tag records imported source logical size, built size records platform target",
        },
        "pieces": [],
    }
    for entry in manifest["imports"]:
        result = {"piece": entry["piece"], "destination": entry["destination"], "assets": {}, "read_errors": []}
        expected = [("mesh", entry["expected_mesh_package"], unreal.StaticMesh),
                    ("material", entry["expected_material_package"], unreal.MaterialInterface)]
        expected += [(name, entry["destination"] + "/" + name, unreal.Texture2D) for name in ("Color", "Normal", "Roughness", "Metallic")]
        for label, package, expected_class in expected:
            asset = unreal.load_asset(package)
            record = {"expected_package": package, "loaded": object_info(asset)}
            result["assets"][label] = record
            if asset is None:
                record["status"] = "missing; import through public UEClient.assets only"
                continue
            if not isinstance(asset, expected_class):
                record["status"] = "unexpected actual asset class"
                continue
            try:
                if label == "mesh":
                    record["details"] = mesh_info(asset, subsystem)
                elif label == "material":
                    record["details"] = material_info(asset)
                else:
                    record["details"] = {
                        "size_pixels": [asset.blueprint_get_size_x(), asset.blueprint_get_size_y()],
                        "size_read_method": "Blueprint_GetSizeX/Y after registered async texture compilation finishes",
                        "source_dimensions_registry_tag": {str(key): str(value) for key, value in unreal.EditorAssetLibrary.get_tag_values(package).items()}.get("Dimensions"),
                        "built_size_pixels": vector(asset.blueprint_get_built_texture_size()),
                        "srgb": bool(asset.get_editor_property("srgb")),
                        "compression_settings": str(asset.get_editor_property("compression_settings")),
                        "flip_green_channel": bool(asset.get_editor_property("flip_green_channel")),
                        "virtual_texture_streaming": bool(asset.get_editor_property("virtual_texture_streaming")),
                        "import_data": import_data_info(asset),
                    }
                record["status"] = "read_complete"
            except Exception as error:
                record["status"] = "read_failed"
                result["read_errors"].append({"asset": package, "error": str(error)})
        result["actual_directory_asset_paths"] = list(unreal.EditorAssetLibrary.list_assets(entry["destination"], recursive=False, include_folder=False))
        report["pieces"].append(result)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.log("TASK070 imported-asset probe: " + str(output_path))
    return report


if __name__ == "__main__":
    probe()
