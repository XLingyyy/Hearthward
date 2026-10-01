"""UE Python script builders for asset imports."""

from __future__ import annotations

import textwrap
from pathlib import Path


def _ue_import_helpers_script() -> str:
    return textwrap.dedent("""\
        import unreal

        def _set_property(obj, property_name, value):
            try:
                obj.set_editor_property(property_name, value)
                return True
            except Exception as exc:
                unreal.log_warning(f"[A3Game] 跳过属性 {property_name}: {exc}")
                return False

        def _new_import_task(source_path, dest_path, destination_name=""):
            task = unreal.AssetImportTask()
            task.set_editor_property("filename", source_path)
            task.set_editor_property("destination_path", dest_path)
            if destination_name:
                task.set_editor_property("destination_name", destination_name)
            task.set_editor_property("automated", True)
            task.set_editor_property("replace_existing", True)
            task.set_editor_property("save", True)
            return task

        def _run_import_task(task, label):
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            paths = task.get_editor_property("imported_object_paths") or []
            if not paths:
                raise RuntimeError(f"{label} 导入没有返回任何资产路径，请查看 UE Output Log")
            imported_paths = [str(path) for path in paths]
            for path in imported_paths:
                unreal.log(f"[A3Game] Imported {label}: {path}")
            _save_imported_assets(imported_paths, task.get_editor_property("destination_path"))
            return imported_paths

        def _save_imported_assets(imported_paths, dest_path):
            for asset_path in imported_paths:
                try:
                    asset = unreal.load_asset(asset_path)
                    if asset is not None:
                        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
                except Exception as exc:
                    unreal.log_warning(f"[A3Game] 保存导入资产失败 {asset_path}: {exc}")

            if dest_path:
                try:
                    unreal.EditorAssetLibrary.save_directory(dest_path, only_if_is_dirty=False, recursive=True)
                    unreal.log(f"[A3Game] Saved import directory: {dest_path}")
                except Exception as exc:
                    unreal.log_warning(f"[A3Game] 保存导入目录失败 {dest_path}: {exc}")

        def _static_mesh_collision(asset):
            body_setup = None
            simple_shape_count = 0
            trace_flag = ""
            try:
                body_setup = asset.get_editor_property("body_setup")
            except Exception:
                pass
            if body_setup is not None:
                try:
                    trace_flag = str(
                        body_setup.get_editor_property("collision_trace_flag")
                    )
                except Exception:
                    pass
                try:
                    aggregate_geometry = body_setup.get_editor_property("agg_geom")
                except Exception:
                    aggregate_geometry = None
                if aggregate_geometry is not None:
                    for property_name in (
                        "box_elems",
                        "sphere_elems",
                        "sphyl_elems",
                        "convex_elems",
                        "tapered_capsule_elems",
                        "level_set_elems",
                        "skinned_level_set_elems",
                    ):
                        try:
                            simple_shape_count += len(
                                aggregate_geometry.get_editor_property(property_name)
                            )
                        except Exception:
                            pass
            return {
                "body_setup": body_setup,
                "simple_shapes": int(simple_shape_count),
                "trace_flag": trace_flag,
                "usable": bool(
                    simple_shape_count > 0
                    or "COMPLEX_AS_SIMPLE" in trace_flag
                ),
            }

        def _ensure_static_mesh_collision(asset, force_complex=False):
            audit = _static_mesh_collision(asset)
            repaired = False
            repair_error = ""
            complex_enabled = "COMPLEX_AS_SIMPLE" in audit["trace_flag"]
            if (
                (not audit["usable"] or (force_complex and not complex_enabled))
                and audit["body_setup"] is not None
            ):
                collision_flag = getattr(
                    getattr(unreal, "CollisionTraceFlag", None),
                    "CTF_USE_COMPLEX_AS_SIMPLE",
                    None,
                )
                if collision_flag is not None:
                    try:
                        body_setup = audit["body_setup"]
                        body_setup.set_editor_property(
                            "collision_trace_flag",
                            collision_flag,
                        )
                        body_setup.invalidate_physics_data()
                        body_setup.create_physics_meshes()
                        unreal.EditorAssetLibrary.save_loaded_asset(
                            asset,
                            only_if_is_dirty=False,
                        )
                        repaired = True
                    except Exception as exc:
                        repair_error = str(exc)
            result = _static_mesh_collision(asset)
            result.pop("body_setup", None)
            result["repaired"] = repaired
            result["repair_error"] = repair_error
            result["force_complex"] = bool(force_complex)
            result["complex_as_simple"] = (
                "COMPLEX_AS_SIMPLE" in result["trace_flag"]
            )
            if (
                not result["usable"]
                or (force_complex and not result["complex_as_simple"])
            ):
                raise RuntimeError(
                    "StaticMesh 碰撞不可用: "
                    + asset.get_path_name()
                    + (": " + repair_error if repair_error else "")
                )
            return result
    """)


def _build_fbx_import_script(
    local_path: str,
    dest_path: str,
    as_skeletal: bool,
    label: str,
    import_animations: bool | None = None,
) -> str:
    should_import_animations = bool(as_skeletal) if import_animations is None else bool(import_animations)
    return _ue_import_helpers_script() + textwrap.dedent(f"""\
        source_path = {local_path!r}
        dest_path = {dest_path!r}
        task = _new_import_task(source_path, dest_path)
        task.set_editor_property("factory", unreal.FbxFactory())
        options = unreal.FbxImportUI()
        _set_property(options, "automated_import_should_detect_type", False)
        _set_property(
            options,
            "mesh_type_to_import",
            unreal.FBXImportType.FBXIT_SKELETAL_MESH
            if {bool(as_skeletal)!r}
            else unreal.FBXImportType.FBXIT_STATIC_MESH,
        )
        _set_property(
            options,
            "original_import_type",
            unreal.FBXImportType.FBXIT_SKELETAL_MESH
            if {bool(as_skeletal)!r}
            else unreal.FBXImportType.FBXIT_STATIC_MESH,
        )
        _set_property(options, "import_mesh", True)
        _set_property(options, "import_as_skeletal", {bool(as_skeletal)!r})
        _set_property(options, "create_physics_asset", {bool(as_skeletal)!r})
        _set_property(options, "import_materials", True)
        _set_property(options, "import_textures", True)
        _set_property(options, "import_animations", {should_import_animations!r})
        if {bool(as_skeletal)!r}:
            if not _set_property(options.skeletal_mesh_import_data, "convert_scene_unit", True):
                raise RuntimeError("Unable to preserve FBX physical units")
            _set_property(options.skeletal_mesh_import_data, "import_uniform_scale", 1.0)
        task.set_editor_property("options", options)
        imported_paths = _run_import_task(task, {label!r})
        if {bool(as_skeletal)!r}:
            # Small meshes can be below the FBX importer's 20 cm body cutoff.
            # The optional adapter editor plugin exposes the native fitter with
            # a smaller cutoff. Otherwise use Unreal's public editor subsystem.
            for asset_path in list(imported_paths):
                skeletal = unreal.load_asset(asset_path)
                if not isinstance(skeletal, unreal.SkeletalMesh):
                    continue
                physics = skeletal.get_editor_property("physics_asset")
                if physics is None:
                    helper = getattr(unreal, "A3GameAssetImportLibrary", None)
                    if helper is not None:
                        physics = helper.create_physics_asset(skeletal, 1.0)
                    else:
                        physics = unreal.SkeletalMeshEditorSubsystem.create_physics_asset(skeletal, True, 0)
                    if physics is None:
                        raise RuntimeError("Unable to create native PhysicsAsset")
                    skeletal.set_editor_property("physics_asset", physics)
                if physics.get_path_name() not in imported_paths:
                    imported_paths.append(physics.get_path_name())
            _save_imported_assets(imported_paths, dest_path)
        print("A3GAME_IMPORTED:" + repr(imported_paths))
    """)


def _build_scene_import_script(
    local_path: str,
    dest_path: str,
    label: str,
    generate_collision: bool = True,
    combine_meshes: bool = True,
    force_complex_collision: bool = False,
) -> str:
    """Import a static scene mesh and make its collision usable in Play mode."""
    return _ue_import_helpers_script() + textwrap.dedent(f"""\
        source_path = {local_path!r}
        dest_path = {dest_path!r}
        task = _new_import_task(source_path, dest_path)
        options = unreal.FbxImportUI()
        _set_property(options, "automated_import_should_detect_type", False)
        _set_property(options, "mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
        _set_property(options, "original_import_type", unreal.FBXImportType.FBXIT_STATIC_MESH)
        _set_property(options, "import_mesh", True)
        _set_property(options, "import_as_skeletal", False)
        _set_property(options, "import_materials", True)
        _set_property(options, "import_textures", True)
        _set_property(options, "import_animations", False)
        try:
            _set_property(options.static_mesh_import_data, "auto_generate_collision", {bool(generate_collision)!r})
            _set_property(options.static_mesh_import_data, "combine_meshes", {bool(combine_meshes)!r})
        except Exception as exc:
            unreal.log_warning(f"[A3Game] 设置 FBX StaticMesh 导入选项失败: {{exc}}")
        task.set_editor_property("options", options)
        imported_paths = _run_import_task(task, {label!r})

        collision_audit = []
        if {bool(generate_collision)!r}:
            for asset_path in imported_paths:
                asset = unreal.load_asset(asset_path)
                if not isinstance(asset, unreal.StaticMesh):
                    continue
                audit = _ensure_static_mesh_collision(
                    asset,
                    force_complex={bool(force_complex_collision)!r},
                )
                audit["path"] = asset_path
                collision_audit.append(audit)

        print("A3GAME_COLLISION_AUDIT:" + repr(collision_audit))
        print("A3GAME_IMPORTED:" + repr(imported_paths))
    """)


def _build_scene_generic_import_script(
    local_path: str,
    dest_path: str,
    label: str,
    generate_collision: bool = True,
    force_complex_collision: bool = False,
) -> str:
    """Import non-FBX scene assets and configure generated StaticMesh collision."""
    return _ue_import_helpers_script() + textwrap.dedent(f"""\
        source_path = {local_path!r}
        dest_path = {dest_path!r}
        task = _new_import_task(source_path, dest_path)
        imported_paths = _run_import_task(task, {label!r})
        collision_audit = []
        if {bool(generate_collision)!r}:
            for asset_path in imported_paths:
                asset = unreal.load_asset(asset_path)
                if not isinstance(asset, unreal.StaticMesh):
                    continue
                audit = _ensure_static_mesh_collision(
                    asset,
                    force_complex={bool(force_complex_collision)!r},
                )
                audit["path"] = asset_path
                collision_audit.append(audit)
        print("A3GAME_COLLISION_AUDIT:" + repr(collision_audit))
        print("A3GAME_IMPORTED:" + repr(imported_paths))
    """)


def _build_generic_import_script(local_path: str, dest_path: str, label: str) -> str:
    return _ue_import_helpers_script() + textwrap.dedent(f"""\
        source_path = {local_path!r}
        dest_path = {dest_path!r}
        task = _new_import_task(source_path, dest_path)
        imported_paths = _run_import_task(task, {label!r})
        print("A3GAME_IMPORTED:" + repr(imported_paths))
    """)


def _safe_asset_name_part(value: str) -> str:
    cleaned = "".join(ch if ch.isalnum() or ch == "_" else "_" for ch in value.strip())
    return "_".join(part for part in cleaned.split("_") if part)


def _motion_import_name(local_path: str, skeleton_asset_path: str) -> str:
    motion_name = _safe_asset_name_part(Path(local_path).stem) or "Motion"
    skeleton_package_path = (skeleton_asset_path or "").split(".", 1)[0]
    skeleton_name = _safe_asset_name_part(skeleton_package_path.rsplit("/", 1)[-1])
    if skeleton_name:
        return f"{motion_name}_{skeleton_name}_Anim"
    return f"{motion_name}_Anim"


def _build_motion_import_script(local_path: str, dest_path: str, skeleton_asset_path: str, avatar_name: str, *, sample_rate: int = 0) -> str:
    if sample_rate < 0 or sample_rate > 240:
        raise ValueError("custom_sample_rate must be between 0 and 240")
    motion_name = _motion_import_name(local_path, skeleton_asset_path)
    return _ue_import_helpers_script() + textwrap.dedent(f"""\
        source_path = {local_path!r}
        dest_path = {dest_path!r}
        skeleton_asset_path = {skeleton_asset_path!r}
        avatar_name = {avatar_name!r}
        if avatar_name:
            unreal.log("[A3Game] Motion avatar_name hint: " + avatar_name)

        task = _new_import_task(source_path, dest_path, {motion_name!r})
        task.set_editor_property("factory", unreal.FbxFactory())
        options = unreal.FbxImportUI()
        for property_name, value in (
            ("automated_import_should_detect_type", False),
            ("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION),
            ("original_import_type", unreal.FBXImportType.FBXIT_ANIMATION),
            ("override_animation_name", {motion_name!r}),
            ("import_mesh", False),
            ("import_as_skeletal", False),
            ("import_materials", False),
            ("import_textures", False),
            ("import_animations", True),
        ):
            _set_property(options, property_name, value)

        if skeleton_asset_path:
            skeleton = unreal.load_asset(skeleton_asset_path)
            if skeleton is None:
                raise RuntimeError(f"找不到 Skeleton 资产: {skeleton_asset_path}")
            _set_property(options, "skeleton", skeleton)

        try:
            _set_property(options.anim_sequence_import_data, "import_custom_attribute", True)
            if not _set_property(options.anim_sequence_import_data, "convert_scene_unit", True):
                raise RuntimeError("Unable to preserve FBX animation physical units")
            _set_property(options.anim_sequence_import_data, "import_uniform_scale", 1.0)
            if {sample_rate!r}:
                if not _set_property(options.anim_sequence_import_data, "use_default_sample_rate", False):
                    raise RuntimeError("Unable to disable default animation sampling")
                if not _set_property(options.anim_sequence_import_data, "custom_sample_rate", {sample_rate!r}):
                    raise RuntimeError("Unable to preserve requested animation sample rate")
        except Exception:
            if {sample_rate!r}:
                raise

        task.set_editor_property("options", options)
        imported_paths = _run_import_task(task, "Motion FBX")
        print("A3GAME_IMPORTED_MOTION:" + repr(imported_paths))
    """)
