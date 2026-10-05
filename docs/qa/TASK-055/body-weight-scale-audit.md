# TASK-055 身体碰撞生成：权重与尺度调查

2026-10-04。只读调查；未启动 UE、改写 Mesh、修改骨架或安装依赖。最初 PhysicsAsset 只有 pelvis／spine_03 两个 body 的运行结果由根 Agent 记录，后续生成与射线结果见末节。本文件分析源数据和本机生成算法，不替代实际 Mesh／PhysicsAsset 验收。

## 已直接读取的来源

TASK-031 首轮 `docs/qa/evidence/TASK-031/REPORT.md` 将当前 Hero 来源列为 `Resource/Tripo/主角/medieval+knight+3d+model.zip`，Brother 来源列为 `Resource/Tripo/弟弟/dark+fantasy+armor+3d+model.zip`。本轮从 F:/Download 下同名实际 ZIP 直接读其 FBX member，未解压改写源文件。当前集成树的归档 ZIP 为 LFS 指针，不把指针内容当作 FBX。

| 角色 | 实际 ZIP | FBX member | FBX 字节数 |
|---|---|---|---:|
| Hero | F:/Download/medieval+knight+3d+model.zip | tripo_convert_6490a05e-c4e5-474f-b1d3-0e5339e0c460.fbx | 845100 |
| Brother | F:/Download/dark+fantasy+armor+3d+model.zip | tripo_convert_e6f691e0-cf6d-4ab8-b136-4441a5f2a946.fbx | 4202876 |

两文件均为 Binary FBX 7400。使用 Python 标准库 struct／zlib 读取 Objects、Connections、Deformer Cluster 的 Indexes／Weights 和 TransformLink，使用现有 NumPy 计算范围；未新增解析框架或依赖。Cluster 与 Model 的 OO 连接确定实际骨名。Dominant 定义为每个控制点最大正权重对应骨，非最大骨计入 AnyWeight 的正权重点数。

## 源 FBX 权重事实

两角色均有 61 个 Cluster，其中 60 骨有正权重，58 骨拥有 dominant 控制点。未发现“仅两骨有权重”的源数据情况。

| 角色 | 骨 | 正权重点数 | dominant 控制点数 | 最大权重 |
|---|---|---:|---:|---:|
| Hero | pelvis | 1398 | 466 | 1.0 |
| Hero | spine_03 | 727 | 62 | 0.54902 |
| Hero | head | 869 | 631 | 0.99216 |
| Hero | calf_l | 926 | 520 | 1.0 |
| Hero | foot_l | 782 | 429 | 1.0 |
| Brother | pelvis | 1348 | 133 | 0.48235 |
| Brother | spine_03 | 863 | 121 | 0.54902 |
| Brother | head | 912 | 739 | 1.0 |
| Brother | calf_l | 1186 | 485 | 1.0 |
| Brother | foot_l | 316 | 117 | 1.0 |

Hero 几何有 9729 控制点，原始 XYZ 跨度约 (0.41196, 0.23032, 0.97870)；Brother 9269 控制点，跨度约 (0.39642, 0.21256, 0.97864)。原始 Model root／pelvis 等局部 Scale 约 1，Cluster TransformLink 的各轴长度约 1。

将每个骨的正权重控制点用其源 TransformLink 逆矩阵变换，得到下列骨局部包围盒对角线长度。这些是源 FBX 数据；没有假称为重新读取后的 UE render buffer 结果。

| 角色 | head | neck_01 | spine_03 | pelvis | thigh_l | calf_l | foot_l | ball_l |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Hero | 0.284596 | 0.264913 | 0.315938 | 0.421885 | 0.571487 | 0.354481 | 0.225887 | 0.188563 |
| Brother | 0.233350 | 0.273114 | 0.364402 | 0.433706 | 0.604595 | 0.438402 | 0.231210 | 0.192186 |

## 本机 UE 5.8 算法事实

`G:/UnrealEngine/UE_5.8/Engine/Source/Developer/MeshUtilitiesEngine/Private/MeshUtilitiesEngine.cpp:27` 的 CalcBoneVertInfos 读取 LOD 的真实 SkinWeightVertexBuffer；Dominant 分支在 69 行、AnyWeight 分支在 84 行均用 GetRefBasesInvMatrix()[BoneIndex].TransformPosition(LOD vertex) 得到骨局部点。

`G:/UnrealEngine/UE_5.8/Engine/Source/Developer/PhysicsUtilities/Private/PhysicsAssetUtils.cpp:160` 的 CalcBoneInfoLength 返回骨局部点包围盒 GetExtent().Size()，即上述完整对角线长度的一半。327—335 行按这个长度累计 MergedSizes，小于 MinBoneSize 的骨向父骨合并；386 行比较生成 body。该阈值直接作用于骨局部长度，不根据角色组件显示高度换算。

TASK-031 记录原 FBX 导入后 UE 根骨具有 100 倍单位 Scale；retarget.json 记录实际 Mesh bounds 高度 Hero 97.869893、Brother 97.863766 cm。归一化只用于 retarget 代理，原始运行 Mesh 未作破坏性替换。

## 推断与下一步检查

源数据证明头／腿／脚具有有效权重。仅两个 PhysicsBody 无法推出 rig 或 skin 损坏。

若当前 UE InvRef 矩阵仍携带记录中的 100 倍根单位 Scale，骨局部包络将接近源数据尺度；MinBoneSize=5 或 1 都可能继续吞并独立头脚。阈值应由实际生成结果验证，禁止按命中点高度补造部位。源 AnyWeight 头／脚的半对角线约 0.11—0.14；Dominant 包络可能更小。

最小资产诊断可在现有根 Python QA 中只读 SkeletonModifier.get_bone_transform(bone,true).scale3d，记录 root／head／calf_l／foot_l 的累计 Scale 与 Mesh bounds；如需要更进一步，再单次读取实际 LOD 权重和骨局部范围，避免引入新依赖或框架。

AnyWeight 包含所有正权重，微小权重远点会扩大包络；它并不自动保证更贴合身体。当前按根授权由根试验 MinBoneSize=0.1／Dominant，然后用实际 head／spine／calf／foot 箭线与真实 BoneName 判定四部位覆盖。本文未记录该后续试验为已通过。

## 后续真实生成结果及最小形状尺寸根因

根 Agent 的 UE 运行结果：MinBoneSize=0.1／Dominant 后，Hero、Brother 均生成 21 个 body；两项实际箭碰身体测试通过。四部位护甲测试仍失败。先将骨关节 pivot 的瞄准点改为对应真实 PhysicsAsset body 的 world AABB center，仍出现跨部位首命中。根记录当前 rig 根累计尺度约 100，Mesh 组件额外显示尺度约 1.8，head body 的 world AABB extent 约 X=134、Y=102、Z=111 cm。这里的尺寸是根的真实运行证据，未由本次源 FBX 解析代替。

本次直接复核本机 `PhysicsAssetUtils.cpp:39`：静态 MinPrimSize=0.5f。889 行将骨局部 BoxExtent 的每轴强制提升至至少 0.5；Box／Sphere／Sphyl 分支使用此结果。该下限和累计约 180 倍缩放组合，会把局部小顶点簇扩大为约 90 cm 以上的世界半尺寸，并叠加形状朝向投影；根记录的 world AABB 与此机制一致。调整 MinBoneSize 只改变哪些骨保留 body，不能消除形状创建时的 MinPrimSize 下限。

920—927 行的 SingleConvexHull／MultiConvexHull 分支将真实 `VertsToUse` 与 `TriangleCacheIndices` 传入 DecomposeMeshToHulls，不使用提升后的 BoxExtent 创建形状；SingleConvexHull 的 HullCount=1。根决定保持 MinBoneSize=0.1／Dominant，并改用 SingleConvexHull。该决策使用既有引擎工厂，未修改引擎下限、骨架尺度或命中部位映射。此处只确认绕过所见 primitive extent 下限的调用路径；最终 hull 范围及四部位射线仍须由根实际运行验证，不能预记为通过。

## 四部位射线采样口径

head／calf／foot 的 socket pivot 分别可能落在颈根、膝、踝关节边界，关节位置不能稳定代表对应部位内部的可命中位置。四部位装备回归使用按准确 BoneName 找到的真实 PhysicsAsset body world AABB center 作为目标，保持整世界射线的第一命中、当前 SkeletalMesh 命中组件断言，以及实际 Hit.BoneName 必须属于目标部位的断言。禁止回退到父骨、整体 Mesh bounds、高度部位推定，禁止跳过覆盖该中心的其他 body。

这个采样方式验证每个部位有一个真实可达命中点并驱动对应装备 GUID 的伤害／耐久链。若首命中仍属于其他部位，测试继续失败并要求修整 PhysicsAsset。它不宣称完整轮廓、关节边界或所有动画姿态已经通过资产 QA。
