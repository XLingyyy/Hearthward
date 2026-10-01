# -*- coding: utf-8 -*-
"""
Tripo 角色一键导入 Unreal Engine 5

使用前：
1. 在 UE 中启用插件：Python Editor Script Plugin
2. 打开：工具（Tools）> 执行 Python 脚本（Execute Python Script）
3. 选择本文件

脚本会：
- 将绑定后的角色主体导入为 Skeletal Mesh
- 自动取得该角色的 Skeleton
- 将动画 FBX 导入并绑定到同一 Skeleton
- 保存到 /Game/Characters/Hero/Tripo
"""

import os
import unreal


# ======================== 可调整配置 ========================
MODEL_FBX = r"E:\AiAgent\XLingGame\Resource\Tripo\主角\outputs\2a9cf835-4661-4ac7-962e-c40ed8608953\2a9cf835-4661-4ac7-962e-c40ed8608953_model.fbx"
ANIMATION_FBX = r"E:\AiAgent\XLingGame\Resource\Tripo\主角\outputs\4599d259-9a84-4382-b77f-63fe7f05c20d\hero_animations_idle_walk_run_slash_hurt.fbx"
DESTINATION_PATH = "/Game/Characters/Hero/Tripo"
SKELETAL_MESH_NAME = "SK_Hero_Tripo"
ANIMATION_ASSET_NAME = "A_Hero_Tripo_Actions"
REPLACE_EXISTING = False
# ===========================================================


def log(message):
    unreal.log("[Tripo Hero Import] " + str(message))


def warn(message):
    unreal.log_warning("[Tripo Hero Import] " + str(message))


def fail(message):
    unreal.log_error("[Tripo Hero Import] " + str(message))
    raise RuntimeError(message)


def set_if_supported(obj, property_name, value):
    """兼容不同 UE5 小版本：属性存在时设置，不存在时跳过。"""
    try:
        obj.set_editor_property(property_name, value)
        return True
    except Exception:
        return False


def validate_sources():
    missing = [path for path in (MODEL_FBX, ANIMATION_FBX) if not os.path.isfile(path)]
    if missing:
        fail("找不到源文件：\n" + "\n".join(missing))


def make_task(filename, destination_name, options):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filename)
    task.set_editor_property("destination_path", DESTINATION_PATH)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", REPLACE_EXISTING)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    return task


def make_mesh_options():
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("original_import_type", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)
    options.set_editor_property("create_physics_asset", True)

    mesh_data = options.get_editor_property("skeletal_mesh_import_data")
    set_if_supported(mesh_data, "convert_scene", True)
    set_if_supported(mesh_data, "convert_scene_unit", True)
    set_if_supported(mesh_data, "force_front_x_axis", False)
    set_if_supported(mesh_data, "import_morph_targets", False)
    set_if_supported(mesh_data, "update_skeleton_reference_pose", False)
    set_if_supported(mesh_data, "use_t0_as_ref_pose", False)
    set_if_supported(mesh_data, "preserve_smoothing_groups", True)
    set_if_supported(mesh_data, "import_mesh_lo_ds", False)
    return options


def make_animation_options(skeleton):
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("original_import_type", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("import_mesh", False)
    options.set_editor_property("import_animations", True)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("skeleton", skeleton)

    anim_data = options.get_editor_property("anim_sequence_import_data")
    set_if_supported(anim_data, "convert_scene", True)
    set_if_supported(anim_data, "convert_scene_unit", True)
    set_if_supported(anim_data, "force_front_x_axis", False)
    set_if_supported(anim_data, "import_bone_tracks", True)
    set_if_supported(anim_data, "remove_redundant_keys", True)
    set_if_supported(anim_data, "do_not_import_curve_with_zero", True)
    set_if_supported(anim_data, "import_custom_attribute", True)
    set_if_supported(anim_data, "animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    return options


def imported_assets(task):
    assets = []
    for object_path in task.get_editor_property("imported_object_paths"):
        asset = unreal.EditorAssetLibrary.load_asset(object_path)
        if asset:
            assets.append(asset)
    return assets


def find_skeletal_mesh(assets):
    for asset in assets:
        if isinstance(asset, unreal.SkeletalMesh):
            return asset
    return None


def get_skeleton(skeletal_mesh):
    try:
        return skeletal_mesh.get_editor_property("skeleton")
    except Exception:
        try:
            return skeletal_mesh.skeleton
        except Exception:
            return None


def run_import():
    validate_sources()
    unreal.EditorAssetLibrary.make_directory(DESTINATION_PATH)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    log("开始导入角色主体：" + MODEL_FBX)
    mesh_task = make_task(MODEL_FBX, SKELETAL_MESH_NAME, make_mesh_options())
    asset_tools.import_asset_tasks([mesh_task])

    mesh_assets = imported_assets(mesh_task)
    skeletal_mesh = find_skeletal_mesh(mesh_assets)
    if not skeletal_mesh:
        paths = mesh_task.get_editor_property("imported_object_paths")
        fail("未找到导入后的 Skeletal Mesh。导入结果：" + str(paths))

    skeleton = get_skeleton(skeletal_mesh)
    if not skeleton:
        fail("角色主体已导入，但没有取得 Skeleton。请确认 MODEL_FBX 是已绑定骨骼的模型。")

    log("角色主体完成：" + skeletal_mesh.get_path_name())
    log("使用 Skeleton：" + skeleton.get_path_name())

    log("开始导入动画：" + ANIMATION_FBX)
    animation_task = make_task(
        ANIMATION_FBX,
        ANIMATION_ASSET_NAME,
        make_animation_options(skeleton),
    )
    asset_tools.import_asset_tasks([animation_task])
    animation_assets = imported_assets(animation_task)

    animation_sequences = [
        asset for asset in animation_assets if isinstance(asset, unreal.AnimSequence)
    ]
    if not animation_sequences:
        warn("动画 FBX 已执行导入，但没有发现 AnimSequence。请检查 FBX 是否包含动画轨道。")
    else:
        log("已导入动画数量：{}".format(len(animation_sequences)))
        for sequence in animation_sequences:
            log("动画：" + sequence.get_path_name())

    unreal.EditorAssetLibrary.save_directory(DESTINATION_PATH, only_if_is_dirty=False, recursive=True)
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(
        [DESTINATION_PATH], force_rescan=True
    )

    log("导入完成。内容目录：" + DESTINATION_PATH)
    log("提示：若 FBX 内的 Idle/Walk/Run/Slash/Hurt 是一条连续时间轴，UE 会生成一个 AnimSequence，需要再按帧段拆分。")


if __name__ == "__main__":
    try:
        run_import()
    except Exception as exc:
        unreal.log_error("[Tripo Hero Import] 导入失败：{}".format(exc))
        raise
