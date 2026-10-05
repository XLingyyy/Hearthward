# TASK-070 首批两武器导入准备

2026-10-04。只在独立 task055-combat 的 QA070／.agent-local 准备目录写入；原制作源只读。没有运行 UE、build、付费服务或 Git，没有改生产、Content、任务状态。根负责逐包LFS锁、真实导入和正式验收。本文只给首批长矛／短刀的人物装备样板，不扩大批量；三组 Owner 视觉门与实际人物持握样图保留。

## 原身份与来源

- spear：`29322e3e-a4e3-4d17-9223-80d1f2d65f99`，task.json `created_at_utc=2026-09-22T09:56:35.187736+00:00`。
- shortblade：`8dd7cafa-fdd7-40cd-9724-a0fa113e72e9`，task.json `created_at_utc=2026-09-22T09:56:27.592895+00:00`。
- 两件实际源均为 `G:/GameFactory/Hearthward/.agent-local/task056/art_source/TASK-004/Tripo/武器/outputs/<UUID>/<UUID>_pbr.fbx`。对应 task.json 与四张 `<UUID>_pbr.fbm/*.jpg` 在相同真实目录，源预览已观察。
- 两份原记录 `type=image_to_model/status=success`、`v3.1-20260211`、PBR、quad、face_limit50000。输入是当地现存 `ChatGPT Image 2026年9月22日 17_47_13 (3).png`（长矛）及 `17_47_11 (2).png`（曲刃短刀），位于该武器根目录。task.json 中的旧 E 盘路径保留为原身份，不当当前文件路径。
- 两份 task.json 没有付费账户状态、输入图授权或许可证字段；本次 metadata 明确记为未记录。已有其他 props 的 SOURCE.md 不覆盖这两 UUID。Owner 已授权 D04 首批工程样板；TASK-074 来源许可台账与后续风格门仍未闭合。

## 已准备的标准暂存与公开参数

`first-weapon-import-manifest.json` 是完整公开 source descriptor／destination／options／逐包清单；`first-weapon-source-resolution.json` 保留真实 `UEClient.assets.resolve_source` 结果，两 mesh 与两件各缺失补图的相对源均解析成功。该调用只解析本机文件，没有启动或连接引擎进程。

暂存 root：`G:/GameFactory/Hearthward/.agent-local/task055-combat/.agent-local/task070-weapon-staging`。以 `pipeline.common.paths.task_output_dir/write_task_meta` 创建 `Hearthward/20261004_task070_existing_weapon/assets/3d_object/<UUID>/`。run_id 是导入暂存标识，原创建时间／UUID／生成请求另保留，不宣称本日重生成。复用真实 `20260923_demo/demo_chest/meta.json` 的五身份字段＋相邻 mesh_path 范式。

相同已有 FBX 的本地复制件分别命名 `SM_Spear.fbx`、`SM_ShortBlade.fbx`。原 FBX、模型节点、UV 和贴图内容没有编辑。四张既有 JPG 复制到 FBX 声明的 `tripo_pbr_model_<UUID>.fbm/`，解决源外部目录名和 RelativeFilename 不一致；没有下载或新烘焙。

根从 GameFactory cwd 的新 Python 进程读取 manifest，并在 import pipeline／UEClient **之前**设置这个进程的 `AAAGF_OUTPUT_ROOT`。不要在已 import paths 的进程晚设变量，也不改全局设置。正式引擎使用根的公开生命周期；以下只是导入调用参数：

```python
import os, json
from pathlib import Path
manifest = json.loads(Path(MANIFEST_ABSOLUTE_PATH).read_text(encoding="utf-8"))
os.environ["AAAGF_OUTPUT_ROOT"] = manifest["output_root"]
from engine_adapters.ue5 import UEClient
ue = UEClient(project_path=manifest["project_path"], ue_root=manifest["ue_root"])
# 根完成该件12个总包中对应6包的锁并启动引擎后执行。
entry = manifest["imports"][0]  # 先 spear，下一件是 shortblade
result = ue.assets.import_weapon(entry["source"], destination=entry["destination"], options=entry["options"])
```

options 只有 `as_skeletal=False/generate_collision=False`，没有虚构 mesh_name、normalize_scale、target_tris。普通静态 FBX builder 没有把 False 写入 auto_generate_collision；默认 `UFbxStaticMeshImportData` 是 True。导入后需要实查并以公开 StaticMeshEditorSubsystem.remove_collisions 清除，不把请求参数当零碰撞证明。

## 命名证据与准确锁清单

UE 5.8.2 本机默认 `BaseEditorPerProjectUserSettings.ini:641 bOverrideFullName=True`、684 `bCombineMeshes=False`、MaterialSearchLocation Local、BaseMaterialName 空；根 Config 及 Saved WindowsEditor 没有覆盖这几项。FBX 每件只有一个 Mesh Model、一个材质。原模型名是 `tripo_node_29322e3e/tripo_node_8dd7cafa`，几何名 `tripo_mesh_*`，材质 `tripo_mat_*`。纹理节点名和包名不同：base_color_texture 等节点的文件 leaf 是 Color／Normal／Roughness／Metallic.jpg。

公开 builder 没有 destination_name 或 postrename 消费。实际入口需区分：

- Legacy：`FbxFactory.cpp:345` 使用显式 FbxImportUI，单 mesh 保留 FullName；`FbxMainImport.cpp:1906` 用 source filename basename 作网格名。材质 `FbxMaterialImport.cpp:439,468` 保留 tripo_mat 名称，纹理39–63用文件 basename。
- UE5.8 默认 Interchange：`AssetTools.cpp:3566,3954–3968` 把 FbxImportUI 经公开注册转换器转 pipeline；`InterchangeFbxAssetImportDataConverter.cpp:821` 将 FullName 转 UseSourceName；`InterchangeGenericAssetsPipeline.cpp:1941–1945` 单 StaticMesh 用 source basename。converter850–858在 BaseMaterialName 空时 ImportAsMaterials，材质不加 M_/MI_ 前缀。FBX TextureNode 按真实文件路径提取文件名，材质 factory 沿原 label；不是使用 Texture 节点的 base_color_texture 等名字。

两种入口的单 mesh 最终命名一致，因此暂存源文件名直接采用最终 mesh 名，可以省去 UUID 临时 Content 包、rename 和 redirector。下列是**源级确定的计划路径**，尚未实际创建；真实 imported_paths／AssetRegistry 必须匹配并记录，导入失败不能记完成：

```text
Content/Hearthward/Assets/TASK-070/Equipment/spear/SM_Spear.uasset
Content/Hearthward/Assets/TASK-070/Equipment/spear/tripo_mat_29322e3e.uasset
Content/Hearthward/Assets/TASK-070/Equipment/spear/Color.uasset
Content/Hearthward/Assets/TASK-070/Equipment/spear/Normal.uasset
Content/Hearthward/Assets/TASK-070/Equipment/spear/Roughness.uasset
Content/Hearthward/Assets/TASK-070/Equipment/spear/Metallic.uasset
Content/Hearthward/Assets/TASK-070/Equipment/shortblade/SM_ShortBlade.uasset
Content/Hearthward/Assets/TASK-070/Equipment/shortblade/tripo_mat_8dd7cafa.uasset
Content/Hearthward/Assets/TASK-070/Equipment/shortblade/Color.uasset
Content/Hearthward/Assets/TASK-070/Equipment/shortblade/Normal.uasset
Content/Hearthward/Assets/TASK-070/Equipment/shortblade/Roughness.uasset
Content/Hearthward/Assets/TASK-070/Equipment/shortblade/Metallic.uasset
```

每件6包，共12包。静态 socket、LOD、材质 graph 节点／ImportData 都存在这些包内，不另列人物 Skeleton／PhysicsAsset、Motion 或新材料包。此次不生产绑定、不 Cook、不批量做其他装备。

两图是否已生成与入口不同：legacy bImportMaterials=True 仅创建支持的连接（Color/Normal）；源 Roughness 在 ShininessExponent、Metallic 在 ReflectionFactor，不等于正确 UE PBR 输入。Interchange AddAllTextures 可创建更多纹理，也仍需核对实际连接。根先查 Roughness/Metallic 精确 package；只在缺失时按 manifest.additional_texture_imports_if_absent 用公开 `ue.assets.import_asset(source, "texture", destination=...)` 导入既有 JPG。四张相对 `_texture_path` 已注册，避免原路径绕 containment；不重复生成 `_1`。

当前首选无需 postrename。若根坚持使用原 UUID 文件名，初始网格将是 `<UUID>_pbr`，须另登记旧包和最终包，再用正常 EditorAssetLibrary.rename_asset。rename 会有旧路径 redirector 风险；不能 `load_asset(old)` 后强删，因为它可解析到新网格。安全收尾应通过旧路径 AssetData 明确 `is_redirector`／ObjectRedirector 类型，再仅删除该旧 redirector 实例并读 registry，确认新网格／依赖仍存在。本首批避免走这条路。

## 可读源坐标与语义边界

为补握点信息，只读取本批两个 Geometry 的真实 Vertices／PolygonVertexIndex／UV，复用已有缓存 PCA 基底，按实际 Color UV 给控制点着色；没有重做十件 FBX 审计。附 `spear-source-axis.png`、`shortblade-source-axis.png`，图中纵轴是控制点 PCA1 min→max 的实际比例，横轴 PCA2；来源坐标细节、实际极点 index、选区与最近控制点在 `weapon-source-axis-facts.json`。

图像直接显示长矛正端是尖刃、负端是包缠柄，短刀正端是握柄、负端是曲刃尖。Grip／BladeBase 是观察对应实物区域后，以实际局部截面中位数推导的**初始候选中心**；FBX 没有名为 Grip／BladeBase 的语义节点。BladeTip 是真实极值控制点，不能把这些未变换 raw 数字直接写入 UE socket。

|件|Grip raw XYZ 候选|BladeBase raw XYZ 候选|BladeTip raw XYZ 实点|
|---|---|---|---|
| spear | 0.015095, 0.343300, -0.365187 | -0.001363, -0.292386, 0.290158 | 0.002419, -0.481806, 0.498653 |
| shortblade | 0.047095, -0.353861, 0.266257 | 0.030392, -0.119195, 0.128942 | 0.053647, 0.426682, -0.499152 |

- spear 原始柄轴朝刃 `(0.044146,-0.698390,0.714355)`；刃基→尖轴 `(0.013426,-0.672378,0.740086)`。
- shortblade 原始柄轴朝刃 `(-0.091452,0.876985,-0.471736)`；刃基→尖轴 `(0.027935,0.655723,-0.754484)`。曲刃／柄并非相同直线，不能用一根主轴取代二者。

已审计的 Model Scale约100、Rotation spear约`(-90,0,0)`，shortblade约`(-90,-30.1344,0)`；节点尺度后的137.3／128.0cm只是源 PCA 长度估计，不冒称实际导入 Bounds 或批准武器尺寸。导入的场景转换／Model变换／轴反射与 ImportUniformScale 必须从真实 mesh／ImportData 读取；在 StaticMesh Editor 实物上校准 Grip、BladeBase、BladeTip，再和 Hero180cm／Brother160cm 身体、手掌姿态对照。此次没有 UE 握点、最终缩放或 Owner 视觉结论。

## 导入后公开 API（根独占执行）

1. `EditorAssetLibrary.list_assets/load_asset`／AssetRegistry 列该件实际包和唯一 StaticMesh，读取 mesh.get_bounds()、get_num_lods()、get_num_vertices(lod)／get_num_triangles(lod)／material slots；StaticMeshEditorSubsystem.get_num_uv_channels(mesh,lod)、get_simple_collision_count／get_convex_collision_count。实际输出匹配精确锁清单。remove_collisions(mesh) 后实读零碰撞，mesh has_navigation_data=False；实际持握组件仍 NoCollision／不影响导航。[Epic StaticMesh](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/StaticMesh?application_version=5.6)、[StaticMeshEditorSubsystem](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/StaticMeshEditorSubsystem?application_version=5.6)。本机5.8对应公开声明已读，在线文档为5.6。
2. socket：`mesh.find_socket("Grip")`；不存在时 `unreal.StaticMeshSocket(outer=mesh)`，以 `set_editor_property("socket_name", "Grip")` 设置名字（普通 .socket_name Python property 是只读，Editor Properties允许），设置真实 UE local 的 relative_location／relative_rotation／relative_scale，再 `mesh.add_socket(socket)`、save_loaded_asset。已有同名更新而不创建重复。BladeBase／BladeTip同法；原字段本机 StaticMeshSocket.h25–35、StaticMesh.h2308–2322均公开。手柄姿态没有实测，先保留候选，不能把源码数值当已贴手。[Epic StaticMeshSocket](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/StaticMeshSocket?application_version=5.6)。
3. 材质修正优先修改已锁 tripo_mat 包；`MaterialEditingLibrary.get_material_expressions` 找既有纹理 sample，缺少 Roughness/Metallic sample 才 create_material_expression(MaterialExpressionTextureSample)，set_editor_property texture／sampler_type，connect_material_property 的 R 输出到 MP_ROUGHNESS／MP_METALLIC，RGB到 MP_BASE_COLOR／MP_NORMAL。Color.srgb=True；Normal真实 TC_NORMALMAP／线性Normal sampler；Roughness、Metallic.srgb=False、Masks或线性采样，保持四原图／UV，不固定整件为金属。recompile_material 返回错误列表须为空，save_loaded_asset，仅同件新包。get_material_property_input_node 与 node.texture 读实际连接；本机 MaterialEditingLibrary.cpp1148 实现直接读Material输入，注释“active editor”不要求先开材质窗口。[Epic MaterialEditingLibrary](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialEditingLibrary?application_version=5.6)、[Epic官方Python材质例子](https://github.com/EpicGames/PythonSamples/blob/main/scripts/Create/Create_and_connect_material_parameter_expression.py)。

现有 LOD建议仍是待验参数，若根选择本批做LOD，SetLods/SetLodReductionSettings操作该同一mesh包后报告每LOD真实面数。此准备没有减面／纹理缩小，不把约96k／98k源三角面估计当20k预算通过。正式材质、持握和LOD近远景仍需实际渲染及Owner样板审阅。
