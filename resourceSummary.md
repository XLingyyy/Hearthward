# Resource 资产盘点与 UE 导入建议

更新日期：2026-09-20

项目：归火（Hearthward）

目标引擎：Unreal Engine 5.8.1

本地资源根目录：`E:\AiAgent\XLingGame\Resource`

## 1. 文档范围

本文记录仓库外 `Resource` 目录中现有第三方建模资产的文件清单、静态检查结果、推荐转换方式、UE 导入设置、性能风险与许可证状态。

本次盘点共发现 77 个文件，合计约 1.436 GB：

| 来源目录 | 文件数 | 磁盘占用 |
|---|---:|---:|
| `Resource/polyhaven` | 53 | 约 1.210 GB |
| `Resource/sketchfab` | 24 | 约 0.230 GB |

本分支同步完整清单和分析，并将许可已确认的 53 个 Poly Haven 源文件复制到 `art_source/TASK-004/polyhaven/`，通过 Git LFS 纳入 Hearthward 公开仓库。24 个 Sketchfab 文件因缺少确切许可证和公开再分发证明而没有上传。本次入库不代表这些资产已经导入 UE、完成性能验证或成为正式世界设定。

项目内 `docs/tasks/TASK-004.md` 要求首批基础资产具备来源可追溯、许可清楚、厘米尺度统一、碰撞和 LOD/Nanite 方案明确，并在独立展示地图验证。本盘点以这些要求为判断基准。

## 2. 总体结论

没有一整套资产属于“原样拖入 UE 即可作为正式游戏资产”。现有文件分为三类：

1. **格式可直接导入 UE，但仍需 UE 内设置**：解压后的 FBX、OBJ，以及 JPG、JPEG、PNG、EXR 贴图。
2. **建议或必须通过 Blender 整理**：所有本地 `.blend` 模型，以及缺法线、面数过高或材质引用不完整的 Sketchfab 岩石。
3. **主要通过其他方式处理**：三套 Poly Haven 地表材质、OpenGL 法线、EXR 数据贴图、透明遮罩、8K 贴图和来源许可证记录。

UE 支持 FBX、OBJ 静态网格导入；项目最终源格式优先使用 FBX 2020 或经验证的 glTF。`.blend` 不是项目稳定直导格式，应由 Blender 导出，或从 Poly Haven 重新下载 FBX/glTF 版本。

## 3. 逐资产结论

| 资产组 | 本地模型/主体 | 静态检查结果 | 推荐分类 | 处理结论 |
|---|---|---|---|---|
| Sketchfab 草 | ZIP 内 `rostlinka_07c_ske.FBX` | FBX 7.3；8 个网格，合计约 77,966 三角面；多个材质槽 | 可直导原型，量产前优化 | UE 能导入，但 FBX 内保留原作者 `D:\quixel\...` 路径，部分相对文件名与本地贴图不一致；需手工接材质、关闭草丛碰撞并补 LOD |
| Sketchfab 小岩石 | ZIP 内 `model.fbx` | FBX 7.7；约 500,075 顶点、1,000,006 三角面；含法线和 UV | 建议 Blender；单个 Nanite 高模可例外直导 | 面数不适合大量散布；只有漫反射和法线，缺粗糙度/ORM；需简单碰撞和纹理降级 |
| Sketchfab 中岩石 | ZIP 内 `stone assets.obj` | 150,000 顶点、299,992 面；无顶点法线；OBJ 引用的 `.mtl` 不在 ZIP 中 | 需要 Blender | 重算法线/切线、确认 UV、减面、统一比例和原点、重建材质后导出 FBX |
| Sketchfab 大岩石 | ZIP 内 `textured_output.obj` | 649,643 顶点、1,137,057 面；无顶点法线；单材质，仅 8K 漫反射 | 建议 Blender；大型 Nanite 景观石可例外直导 | 如果是少量远景巨石可让 UE 重算法线并启用 Nanite；一般用途应减面、补粗糙度/法线并制作简单碰撞 |
| Poly Haven 草 | `grass_medium_01_4k.blend` | 官方资产页标注约 200 万三角面，带 LOD/Geometry Nodes；本地为 Blender Zstandard 压缩文件 | Blender 或重新下载 FBX/glTF | 不可把完整高模直接作为高密度 Foliage；需选取/导出合适 LOD、保留 Alpha 叶片并控制实例密度 |
| Poly Haven 灌木 | `shrub_01_4k.blend` | 官方约 28.2 万三角面，带 LOD | Blender 或重新下载 FBX/glTF | 导出 LOD，设置 Masked 双面植被材质；普通灌木不启用阻挡碰撞 |
| Poly Haven 树桩 | `tree_stump_01_4k.blend` | 官方约 4.1 万三角面 | Blender 或重新下载 FBX/glTF | 当前 Poly Haven 模型中最接近游戏可用；导出 FBX、补简单碰撞、连接 PBR 材质即可进入展示场 |
| Poly Haven 蓝花楹 | `jacaranda_tree_4k.blend` | 官方约 31.2 万三角面；树干、枝条、叶片分别配图 | Blender 或重新下载 FBX/glTF | 分离树干/枝叶材质，导出 LOD，树干使用简单碰撞；树种具有明确气候含义，应标 `TEMP_VISUAL` |
| Poly Haven 岛树 | `island_tree_02_4k.blend` | 官方约 200 万三角面；包含树体、枝条和叶片多套贴图 | 必须重点优化 | 不适合直接大量种植；可保留高模树干作 Nanite 候选，但叶片仍需卡片/LOD/实例性能验证；标 `TEMP_VISUAL` |
| Poly Haven 草地 | `grass_ground_4k.blend` + 4K PBR 贴图 | Diffuse、Displacement、OpenGL Normal、Roughness | 不需 Blender，走 UE 材质流程 | 忽略 `.blend` 预览载体，直接用贴图创建 Landscape/地表材质实例 |
| Poly Haven 土地 | `dirt_4k.blend` + 4K PBR 贴图 | Diffuse、Displacement、OpenGL Normal、Roughness | 不需 Blender，走 UE 材质流程 | 作为营地/道路/裸土地表；设置实际平铺尺度，Displacement 先作为可选高度输入 |
| Poly Haven 岩面 | `rocky_terrain_4k.blend` + 4K PBR 贴图 | Diffuse、Displacement、OpenGL Normal、Roughness；官方覆盖约 90 米范围 | 不需 Blender，走 UE 材质流程 | 适合山坡/宏观岩面；不要与 2 米级土地材质使用相同 UV 平铺比例 |

## 4. 可直接导入 UE 的本地文件

以下 ZIP 解压后的模型格式可以被 UE 读取，但导入后仍要完成材质、尺寸、碰撞和性能检查：

- `sketchfab/Grass/source/rostlinka_07c_ske.zip` → `rostlinka_07c_ske.FBX`
- `sketchfab/岩石/小岩石/source/finalized.zip` → `model.fbx`
- `sketchfab/岩石/中岩石/source/stone assets.zip` → `stone assets.obj`
- `sketchfab/岩石/大岩石/source/model.zip` → `textured_output.obj`

所有 JPG、JPEG、PNG 和 EXR 可以作为纹理导入 UE。颜色贴图与数据贴图必须使用不同设置，不能依靠自动材质生成判断正确性。

## 5. Blender 处理要求

### 5.1 通用整理

- 统一单位为厘米、Z 向上并应用缩放/旋转。
- 检查枢轴：树木和草位于地面中心，树桩和岩石位于可摆放底部。
- 对缺法线 OBJ 重新生成平滑法线与 MikkTSpace 切线。
- 清理不可见扫描碎片、重叠面、孤立点和不需要的地面底座。
- 将树干、枝条、叶片、地面碎片拆为明确材质槽。
- 生成至少 2–3 级 LOD；高模岩石可保留 LOD0 作为 Nanite 候选。
- 树干、树桩和岩石使用简单碰撞；草和普通灌木关闭不必要碰撞。
- 最终导出 FBX 2020 或经 UE 5.8.1 实测的 glTF，并记录导出参数。

### 5.2 推荐优先级

1. 树桩：低风险、较快形成正式展示资产。
2. Sketchfab 草：可快速建立 Foliage 原型，但先修材质引用。
3. Poly Haven 灌木和蓝花楹：中等优化量。
4. 小/中/大岩石：按 Nanite 用途决定是否减面。
5. Poly Haven 草和岛树：面数最高，必须先做实例密度和 LOD 方案。

## 6. UE 材质与纹理设置

### 6.1 Poly Haven OpenGL 法线

本地 Poly Haven 法线命名为 `_nor_gl_4k`，绿通道方向与 UE 常用 DirectX 法线相反。处理方式任选其一：

- 在 UE Texture 属性中启用 `Flip Green Channel`；或
- 从 Poly Haven 重新下载 `nor_dx` PNG/JPG；后者更适合长期资产流水线。

### 6.2 颜色空间与压缩

| 贴图类型 | sRGB | 建议压缩/用途 |
|---|---|---|
| Diffuse、BaseColor、Albedo | 开 | 默认颜色压缩 |
| Normal | 关 | Normalmap；Poly Haven GL 法线翻转绿通道 |
| Roughness、AO、Occlusion、Concavity | 关 | Masks 或 Grayscale |
| Alpha、Alfa | 关 | 接入 Opacity Mask；优先使用无损 PNG/TGA |
| Displacement、Height | 关 | 高度混合、POM 或其他明确位移方案；不自动产生几何起伏 |

### 6.3 植被材质

- 叶片和草使用 `Masked`，不使用昂贵的普通半透明混合。
- 使用 Two Sided/Foliage 类材质设置，并单独调节透光与阴影。
- Sketchfab 的 Alpha 是独立灰度 JPEG，并不在 Diffuse 的 Alpha 通道内，必须手工连接。
- 草 FBX 自带的材质引用不完整；Normal、Occlusion 等本地贴图不会自动全部连接。

### 6.4 分辨率与显存

- Sketchfab 大、小岩石漫反射为 8192×8192；中岩石为 2048×2048。
- Poly Haven 现有贴图均为 4096×4096。
- 草/灌木原型优先限制为 1K–2K，树干/叶片通常从 2K 验证，英雄岩石最多先保留 4K。
- 可以先使用 UE 的 `Max Texture Size` 验证，再决定是否离线重采样。
- 当前目标机器为 RTX 4060 Laptop 8 GB；不能因为源贴图是 4K/8K 就默认全部常驻最高分辨率。

## 7. Nanite、LOD 与碰撞建议

- 高模岩石、树桩和树干是 Nanite 的优先候选。
- 小岩石 100 万面、大岩石约 114 万面，可以作为少量 Nanite 高模验证，但不应在未测量前大规模散布。
- 岛树和 Poly Haven 草约 200 万面；Masked 叶片/草片仍可能受过绘制影响，Nanite 不能代替植被 LOD 和实例密度设计。
- 草和灌木默认无阻挡碰撞；树干使用胶囊或简化凸包；岩石使用少量凸包或手工 UCX。
- 不使用高模网格本身作为复杂碰撞体。

## 8. 与“归火”设计的适配

这些资产整体属于偏写实自然风格，适合当前 GDD 已列出的森林、草原、河岸、山坡、山峰和洞穴环境。

需要保持以下边界：

- 正式美术风格、文化、地理原型和具体植物群尚未定案。
- 蓝花楹和岛树具有较强气候/地理暗示，只能作为 `TEMP_VISUAL` 候选。
- Sketchfab 中岩石的漫反射接近灰度，与 Poly Haven 偏暖的地表和树木混用时需要统一色调。
- 这些资源只覆盖 TASK-004 的树、草、灌木、树桩、岩石和地表；人物基模和房屋模块仍然缺失。
- 资产入库不代表自动替换 Bootstrap 灰盒角色、地图或正式 World 内容。

## 9. 许可证与公开仓库状态

### 9.1 Poly Haven

Poly Haven 官方声明全部资产采用 CC0，可用于商业项目、修改并重新分发。尽管不强制署名，项目台账仍应记录资产名、作者、原始页面、下载日期和 CC0 链接。

本次已经把本地 Poly Haven 集合的 53 个文件按原目录结构复制到 `art_source/TASK-004/polyhaven/`。源目录与仓库副本均为 1,300,014,678 字节，逐文件 SHA-256 比较结果为 0 个不匹配。

- 许可：https://polyhaven.com/license
- Grass Medium 01：https://polyhaven.com/a/grass_medium_01
- Shrub 01：https://polyhaven.com/a/shrub_01
- Tree Stump 01：https://polyhaven.com/a/tree_stump_01
- Jacaranda Tree：https://polyhaven.com/a/jacaranda_tree
- Island Tree 02：https://polyhaven.com/a/island_tree_02
- Dirt：https://polyhaven.com/a/dirt
- Rocky Terrain：https://polyhaven.com/a/rocky_terrain

### 9.2 Sketchfab

当前本地四套 Sketchfab 资产没有附带：

- 原始模型页面 URL/UID；
- 作者；
- 下载日期；
- Creative Commons、Standard 或 Editorial 的具体许可；
- 若为购买资产，对应的许可证或订单证明。

因此这些文件目前只能作为本地技术候选，**不得提交到 Hearthward 公开仓库，也不能判定为可用于商业发行**。应找回原始页面并逐项确认：商业使用权、署名要求、修改权、源文件公开再分发权。Sketchfab 的 Standard/Editorial 与 Creative Commons 许可不可混为一谈。

许可入口：https://sketchfab.com/licenses

## 10. 完整本地文件清单

### 10.1 Poly Haven：草

```text
polyhaven/草/草2/grass_medium_01_4k.blend
polyhaven/草/草2/textures/grass_medium_01_alpha_4k.png
polyhaven/草/草2/textures/grass_medium_01_diff_4k.jpg
polyhaven/草/草2/textures/grass_medium_01_dry_diff_4k.png
polyhaven/草/草2/textures/grass_medium_01_nor_gl_4k.exr
polyhaven/草/草2/textures/grass_medium_01_rough_4k.exr
```

### 10.2 Poly Haven：灌木

```text
polyhaven/灌木/shrub_01_4k.blend
polyhaven/灌木/textures/shrub_01_alpha_4k.png
polyhaven/灌木/textures/shrub_01_diff_4k.jpg
polyhaven/灌木/textures/shrub_01_disp_4k.png
polyhaven/灌木/textures/shrub_01_nor_gl_4k.exr
polyhaven/灌木/textures/shrub_01_rough_4k.exr
```

### 10.3 Poly Haven：树桩

```text
polyhaven/树桩/tree_stump_01_4k.blend
polyhaven/树桩/textures/tree_stump_01_diff_4k.jpg
polyhaven/树桩/textures/tree_stump_01_nor_gl_4k.exr
polyhaven/树桩/textures/tree_stump_01_rough_4k.exr
```

### 10.4 Poly Haven：蓝花楹树

```text
polyhaven/树木/蓝花楹树/jacaranda_tree_4k.blend
polyhaven/树木/蓝花楹树/textures/jacaranda_tree_branches_diff_4k.png
polyhaven/树木/蓝花楹树/textures/jacaranda_tree_branches_nor_gl_4k.png
polyhaven/树木/蓝花楹树/textures/jacaranda_tree_branches_rough_4k.png
polyhaven/树木/蓝花楹树/textures/jacaranda_tree_leaves_alpha_4k.png
polyhaven/树木/蓝花楹树/textures/jacaranda_tree_leaves_diff_4k.png
polyhaven/树木/蓝花楹树/textures/jacaranda_tree_leaves_nor_gl_4k.png
polyhaven/树木/蓝花楹树/textures/jacaranda_tree_leaves_rough_4k.png
polyhaven/树木/蓝花楹树/textures/jacaranda_tree_trunk_diff_4k.png
polyhaven/树木/蓝花楹树/textures/jacaranda_tree_trunk_nor_gl_4k.png
polyhaven/树木/蓝花楹树/textures/jacaranda_tree_trunk_rough_4k.png
```

### 10.5 Poly Haven：岛树

```text
polyhaven/树木/岛树/island_tree_02_4k.blend
polyhaven/树木/岛树/textures/island_tree_02_branches_diff_4k.png
polyhaven/树木/岛树/textures/island_tree_02_branches_nor_gl_4k.png
polyhaven/树木/岛树/textures/island_tree_02_branches_rough_4k.png
polyhaven/树木/岛树/textures/island_tree_02_diff_4k.jpg
polyhaven/树木/岛树/textures/island_tree_02_leaves_alpha_4k.png
polyhaven/树木/岛树/textures/island_tree_02_leaves_diff_4k.png
polyhaven/树木/岛树/textures/island_tree_02_leaves_nor_gl_4k.png
polyhaven/树木/岛树/textures/island_tree_02_leaves_rough_4k.png
polyhaven/树木/岛树/textures/island_tree_02_nor_gl_4k.exr
polyhaven/树木/岛树/textures/island_tree_02_rough_4k.exr
```

### 10.6 Poly Haven：地表

```text
polyhaven/地表/草地/grass_ground_4k.blend
polyhaven/地表/草地/textures/grass_ground_diff_4k.jpg
polyhaven/地表/草地/textures/grass_ground_disp_4k.png
polyhaven/地表/草地/textures/grass_ground_nor_gl_4k.exr
polyhaven/地表/草地/textures/grass_ground_rough_4k.exr

polyhaven/地表/土/dirt_4k.blend
polyhaven/地表/土/textures/dirt_diff_4k.jpg
polyhaven/地表/土/textures/dirt_disp_4k.png
polyhaven/地表/土/textures/dirt_nor_gl_4k.exr
polyhaven/地表/土/textures/dirt_rough_4k.exr

polyhaven/地表/岩面/rocky_terrain_4k.blend
polyhaven/地表/岩面/textures/rocky_terrain_diff_4k.jpg
polyhaven/地表/岩面/textures/rocky_terrain_disp_4k.png
polyhaven/地表/岩面/textures/rocky_terrain_nor_gl_4k.exr
polyhaven/地表/岩面/textures/rocky_terrain_rough_4k.exr
```

### 10.7 Sketchfab：草

```text
sketchfab/Grass/source/rostlinka_07c_ske.zip
sketchfab/Grass/textures/ground_close_04_basecolor.jpeg
sketchfab/Grass/textures/ground_close_04_normal.jpeg
sketchfab/Grass/textures/ground_close_04_oclusion.jpeg
sketchfab/Grass/textures/rostlinka_07_ground_albedo.jpeg
sketchfab/Grass/textures/rostlinka_07_ground_alfa.jpeg
sketchfab/Grass/textures/rostlinka_07_ground_concavity.jpeg
sketchfab/Grass/textures/rostlinka_07_ground_NormalsMap.jpeg
sketchfab/Grass/textures/rostlinka_07_ground_occlusion.jpeg
sketchfab/Grass/textures/rostlinka_07c_alfa.jpeg
sketchfab/Grass/textures/rostlinka_07c_diffuse.jpeg
sketchfab/Grass/textures/rostlinka_07c_normal.jpeg
sketchfab/Grass/textures/rostlinka_07c_oclusion.jpeg
sketchfab/Grass/textures/rostlinka12_2k_alfa.jpeg
sketchfab/Grass/textures/rostlinka12_2k_difuse.jpeg
sketchfab/Grass/textures/rostlinka12_2k_normal.jpeg
```

ZIP 内另含 `ground_close_04_high.jpg` 和 `rostlinka_07c_ske.FBX`；其余贴图与外层目录基本重复，但扩展名为 JPG。

### 10.8 Sketchfab：大中小岩石

```text
sketchfab/岩石/大岩石/source/model.zip
sketchfab/岩石/大岩石/textures/textured_output.jpeg

sketchfab/岩石/中岩石/source/stone assets.zip
sketchfab/岩石/中岩石/textures/d_m.jpg
sketchfab/岩石/中岩石/textures/nm.jpg

sketchfab/岩石/小岩石/source/finalized.zip
sketchfab/岩石/小岩石/textures/internal_ground_ao_texture.jpeg
sketchfab/岩石/小岩石/textures/tex_u1_v1_diffuse.jpeg
```

ZIP 内补充内容：

- 大岩石：`textured_output.obj`、`textured_output.mtl`、`textured_output.jpg`。
- 中岩石：仅 `stone assets.obj`；OBJ 引用的 `stone assets.mtl` 缺失。
- 小岩石：`model.fbx`、`tex_u1_v1_diffuse.jpg`、`tex_u1_v1_normal.jpg`；外层目录没有单独保存 Normal，但 ZIP 内存在。

## 11. 后续执行清单

1. 找回四套 Sketchfab 资产的原始 URL、作者和确切许可；许可未闭合前不提交源文件。
2. 从 Poly Haven 重新下载 FBX/glTF、DirectX Normal 和适当分辨率贴图，可减少 Blender 与 EXR 转换工作。
3. 优先制作三套地表材质实例、树桩和草原型。
4. 对树木、灌木和岩石建立统一 Blender 导出预设、LOD、碰撞和命名规则。
5. 资产导入路径遵循 TASK-004：`/Game/Hearthward/Assets/`；展示地图使用 `/Game/Hearthward/AssetReview/L_AssetReview`。
6. 在目标 RTX 4060 Laptop 8 GB 环境记录实例数量、近远景效果、帧耗时、显存、LOD/Nanite 和阴影表现。
7. 每件实际入库资产建立来源台账；本文件是初次盘点，不替代逐资产许可和导入记录。

## 12. 本次未执行事项

- 未解压或修改原始资源；Poly Haven 文件按原字节复制到仓库，Sketchfab 文件未复制。
- 未安装或运行 Blender。
- 未在 UE 5.8.1 中实际导入、保存或重开资产。
- 未创建材质、碰撞、LOD、Nanite 设置或展示地图。
- 未进行 UE 构建、PIE、帧率或显存验证。
- 未提交许可证不明确的 Sketchfab 模型或贴图。
