"""Run inside Unreal Editor to import TASK-028's selected static FBX candidates.

Host launch must go through UEClient. Source files are read only; all UE packages
created by this script live below /Game/Hearthward/Assets/TASK-028/.
"""

import json
import traceback
from pathlib import Path

import unreal


ROOT = Path(unreal.Paths.project_dir()).resolve()
MANIFEST = ROOT / "docs/assets/TASK-028/asset-manifest.json"
REPORT = ROOT / "Saved/Task028/static-import-report.json"
BASE = "/Game/Hearthward/Assets/TASK-028/"
REPORT.parent.mkdir(parents=True, exist_ok=True)


def asset_paths(folder):
    return [str(path).split(".")[0] for path in
            unreal.EditorAssetLibrary.list_assets(folder, recursive=True, include_folder=False)]


def import_one(row, tools, static_mesh_editor):
    package = row["planned_ue_asset"]
    assert package and package.startswith(BASE), package
    assert not unreal.EditorAssetLibrary.does_asset_exist(package), package
    folder, name = package.rsplit("/", 1)
    source = ROOT / row["source"]
    assert source.is_file(), source
    before = set(asset_paths(folder))

    options = unreal.FbxImportUI()
    options.set_editor_properties(dict(
        import_mesh=True, import_as_skeletal=False,
        import_materials=True, import_textures=True,
        automated_import_should_detect_type=False,
        mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH,
    ))
    options.static_mesh_import_data.set_editor_properties(dict(
        combine_meshes=True, auto_generate_collision=False,
        import_mesh_lo_ds=False, generate_lightmap_u_vs=True,
    ))
    task = unreal.AssetImportTask()
    task.set_editor_properties(dict(
        filename=str(source), destination_path=folder,
        destination_name=name, automated=True, save=False,
        replace_existing=False, options=options,
    ))
    tools.import_asset_tasks([task])
    mesh = unreal.load_asset(package)
    assert mesh and isinstance(mesh, unreal.StaticMesh), (package, task.imported_object_paths)
    new = sorted(set(asset_paths(folder)) - before)
    assert package in new, (package, new)

    textures = []
    materials = []
    for path in new:
        asset = unreal.load_asset(path)
        assert asset, path
        if isinstance(asset, unreal.Texture2D):
            asset.set_editor_property("max_texture_size", 2048)
            if path.endswith("/Roughness"):
                asset.set_editor_properties(dict(
                    srgb=False,
                    compression_settings=unreal.TextureCompressionSettings.TC_MASKS,
                ))
            width = asset.blueprint_get_size_x() if hasattr(asset, "blueprint_get_size_x") else 0
            height = asset.blueprint_get_size_y() if hasattr(asset, "blueprint_get_size_y") else 0
            textures.append({
                "path": path,
                "size": [width, height],
                "srgb": bool(asset.get_editor_property("srgb")),
                "compression": str(asset.get_editor_property("compression_settings")),
                "max_size": 2048,
            })
        elif isinstance(asset, unreal.MaterialInterface):
            materials.append(path)

    slots = []
    for index, slot in enumerate(mesh.static_materials):
        material = slot.material_interface
        assert material, (package, index)
        slots.append({"slot": index, "name": str(slot.material_slot_name),
                      "material": material.get_path_name()})
    bounds = mesh.get_bounds().box_extent
    dimensions = [round(bounds.x * 2, 2), round(bounds.y * 2, 2), round(bounds.z * 2, 2)]
    assert all(value > 0 for value in dimensions), (package, dimensions)
    for path in new:
        assert unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=True), path
    return {
        "id": row["id"], "ue_asset": package, "packages": new,
        "dimensions_cm": dimensions,
        "material_slots": slots, "materials": materials, "textures": textures,
        "simple_collision_count": static_mesh_editor.get_simple_collision_count(mesh),
        "lod_count": static_mesh_editor.get_lod_count(mesh),
        "source": row["source"], "source_sha256": row["source_sha256"],
        "stage": "imported_pending_scale_collision_visual_review",
    }


def main():
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    selected = [row for row in manifest["assets"]
                if row["disposition"] in {"apply", "scene_prop"}
                and row["category"] in {"building_part", "furniture", "facility", "prop"}]
    assert len(selected) == 15, [row["id"] for row in selected]
    save = unreal.EditorLoadingAndSavingUtils
    assert not save.get_dirty_content_packages() and not save.get_dirty_map_packages(), "Editor has dirty packages"
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.SystemLibrary.execute_console_command(world, "Interchange.FeatureFlags.Import.FBX 0")
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    static_mesh_editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    results = []
    for row in selected:
        results.append(import_one(row, tools, static_mesh_editor))
    dirty = [p.get_path_name() for p in
             [*save.get_dirty_content_packages(), *save.get_dirty_map_packages()]]
    assert not dirty, dirty
    return {"ok": True, "baseline": manifest["baseline_commit"], "assets": results}


try:
    result = main()
except Exception:
    result = {"ok": False, "error": traceback.format_exc()}
REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
if not result["ok"]:
    raise RuntimeError(result["error"])
