# Tripo 武器与护具：Unreal Engine 5.8.2 兼容性检查报告

检查日期：2026-09-22

## 结论

- **导入兼容性：通过（10/10）**。全部 FBX 已在本机 `Unreal Engine 5.8.2-56702186` 隔离工程中实际导入，均成功创建 Static Mesh、材质实例和贴图资产，导入日志没有 FBX、Interchange、Static Mesh 或 AssetTools 错误。
- **可直接投产：暂不通过（0/10）**。网格本体完整，但所有资产都有相同的材质、LOD、尺度/枢轴和 FBX 版本问题；护具还需要根据使用方式决定是否蒙皮。
- 原始 Tripo 模型未被修改。实际导入验证位于 `.ue582_asset_validation` 隔离工程。

## 已通过的项目

- 10 个 FBX 文件均可被 UE 5.8.2 读取并保存为 `.uasset`。
- 每个模型均包含有效顶点、三角形和 1 个材质槽。
- 每个模型均有 2 套 UV；UE 使用 UV1 作为 Lightmap UV。
- 每个模型均生成 Body Setup 和 1 个凸包碰撞体。
- 每个模型均成功导入 4 张 4096×4096 贴图：Color、Metallic、Normal、Roughness。
- Normal 贴图被正确识别为 Normal Map、关闭 sRGB 并使用法线压缩。

## 必须处理的问题

### 1. PBR 材质未完整接线

UE 的 FBX Legacy Phong 导入器为每个模型创建了材质实例，并连接：

- Color → `DiffuseColorMap`
- Normal → `NormalMap`
- Roughness → `ShininessMap`

但 **Metallic 贴图没有被任何材质参数引用**。此外 Metallic 与 Roughness 当前都以 `sRGB=true`、默认颜色压缩导入；作为数据贴图，应关闭 sRGB，并使用适合 Mask/灰度数据的压缩。Roughness 通过旧 Phong 的 Shininess 参数传递，还需要在最终 UE 主材质中确认是否做了正确的粗糙度语义转换。

建议创建统一的 UE PBR 主材质，显式连接 Base Color、Metallic、Roughness、Normal，再为 10 个模型建立材质实例。

### 2. 只有 LOD0

每个模型只有一个 LOD，三角形约 90,000–102,563。对小型武器和穿戴物来说偏高。建议二选一：

- 启用并验证 Nanite；或
- 生成至少 LOD1/LOD2/LOD3，并按目标平台制定三角形预算。

### 3. 尺度和枢轴未按游戏语义整理

Tripo 将资产大致归一化到约 1 米包围盒，所有枢轴都接近几何中心。因此长矛长度仅约 1 米，靴子组合包围盒超过 1 米，不能直接视为真实游戏尺寸。

建议为每个资产设定真实厘米尺寸，并调整枢轴：武器枢轴放在握持点；靴子/护臂按角色骨骼或插槽定位；护甲和兜帽按角色原点与参考姿态对齐。

### 4. 护具目前是 Static Mesh

兜帽、皮甲、护臂、靴子可以作为 Static Mesh 导入，也可以挂到角色 Socket；但若需要随身体自然变形，必须转换为 Skeletal Mesh，并绑定到目标角色 Skeleton、补充蒙皮权重后再验证穿插。

### 5. FBX 版本不是 Epic 推荐版本

这些文件的二进制 FBX 版本是 `7400`。Epic 的 UE 5.8 文档建议使用 FBX 2020.2（`7700`）。本次在 UE 5.8.2 中实测全部导入成功，所以这不是当前阻塞项，但在最终资产管线中建议经 Blender、Maya 或 3ds Max 重新导出为 FBX 2020.2。

## 逐项实测结果

| 资产 | 顶点 | 三角形 | UE 包围盒（cm） | UV | LOD | 凸包碰撞 | 4K 贴图 | UE 5.8.2 导入 |
|---|---:|---:|---|---:|---:|---:|---:|---|
| 弯刀 A | 66,077 | 102,563 | 48.5 × 68.0 × 99.9 | 2 | 1 | 1 | 4 | 通过 |
| 弯刀 B | 62,498 | 98,299 | 54.3 × 71.2 × 100.0 | 2 | 1 | 1 | 4 | 通过 |
| 长矛 | 65,868 | 96,818 | 7.2 × 96.5 × 99.9 | 2 | 1 | 1 | 4 | 通过 |
| 狼牙棒 | 63,710 | 90,000 | 106.5 × 71.8 × 98.0 | 2 | 1 | 1 | 4 | 通过 |
| 弓 | 72,813 | 99,796 | 54.7 × 28.3 × 99.9 | 2 | 1 | 1 | 4 | 通过 |
| 弩 | 66,639 | 96,836 | 84.6 × 80.3 × 59.0 | 2 | 1 | 1 | 4 | 通过 |
| 兜帽 | 61,837 | 92,260 | 67.1 × 74.0 × 99.8 | 2 | 1 | 1 | 4 | 通过 |
| 皮甲 | 69,427 | 96,410 | 57.5 × 75.0 × 99.1 | 2 | 1 | 1 | 4 | 通过 |
| 护臂 | 65,860 | 95,105 | 85.4 × 99.8 × 72.9 | 2 | 1 | 1 | 4 | 通过 |
| 靴子 | 68,421 | 94,596 | 107.3 × 110.1 × 97.2 | 2 | 1 | 1 | 4 | 通过 |

## 验证证据

- [机器可读导入报告](../../../docs/qa/project-progress-20261002/model-import-history/ue582_import_report.json)
- [UE 5.8.2 导入日志](../../../docs/qa/project-progress-20261002/model-import-history/ue582_import.txt)
- [隔离验证工程](../../../docs/qa/project-progress-20261002/model-import-history/TripoAssetValidation.uproject)
- [验证脚本](../../../docs/qa/project-progress-20261002/model-import-history/validate_tripo_assets.py)

官方参考：

- [UE 5.8 FBX Static Mesh Pipeline](https://dev.epicgames.com/documentation/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine)
- [UE 5.8 FBX Import Options](https://dev.epicgames.com/documentation/unreal-engine/fbx-import-options-reference-in-unreal-engine)
- [UE 5.8 FBX Material Pipeline](https://dev.epicgames.com/documentation/unreal-engine/fbx-material-pipeline-in-unreal-engine)
