# TASK-055：真实蒙皮 PhysicsAsset 拟合 author 技术依据

2026-10-04。子代理只写 author 源码，未启动 UE、构建、执行 Git 或改写 uasset。没有实际 author 或箭矢运行结果。

授权文件：现有 Source/Hearthward/Tests/ProjectileBodyProtectionTests.cpp。补丁为根当前文件的纯追加增量，保留原五项真实 Combat055 测试和 body center / Bounds 诊断。

独立 complex author 名称：Hearthward.AssetAuthoring055.FitCombatPhysicsFromActualSkin.HeroAndBrother。仅当命令行包含 -Hearthward055AuthorPhysics 时，GetTests 才枚举案例；常规测试不会得到 author 案例或资产写入。

作者只加载 /Game/Characters/Hero/UE5/SK_Hero 和 /Game/Characters/Brother/UE5/SK_Brother 当前 Mesh 已分配的 PA_HeroCombat / PA_BrotherCombat，并校验准确 PA 路径。根持有 owner 锁 53714421 / 53714422；锁检查和单次 UE 执行由根负责。

使用当前 LOD0 PositionVertexBuffer、SkinWeightVertexBuffer 的最大正权重，经 RenderSection.BoneMap 得到真实骨，用现有 PhysicsAsset.FindControllingBodyIndex 将无独立 body 的子骨关联到真实控制 body。顶点通过目标 body 所属骨 GetRefBasesInvMatrix 转换为 BoneLocal；每 body 只拟合实际顶点 AABB，尺寸采用 1% 相对留量，无固定最小尺寸。两份 PA 的全部 body 候选须先验证非空、有限且三轴尺寸为正，然后替换 AggGeom。保留所有现有 body 骨名、约束、profile 和 Mesh 关联；只保存两个现有 PA，不保存 Mesh。

本机 UE 5.8 源码依据：
- Developer/MeshUtilitiesEngine/Private/MeshUtilitiesEngine.cpp:27–94：Epic 使用同样的 LOD0 dominant 权重、section BoneMap 和 InvRef 变换。这里只使用 Runtime/Engine 公共 API，不依赖 MeshUtilitiesEngine。
- Runtime/Engine/Public/Rendering/SkinWeightVertexBuffer.h:388：GetSkinWeights 为 ENGINE_API；PositionVertexBuffer.h:85–104 提供公开顶点和 CPU 数据检查。
- Runtime/Engine/Private/PhysicsEngine/PhysicsAsset.cpp:404–423：FindControllingBodyIndex 按真实骨架父链匹配已有 body。
- Developer/PhysicsUtilities/Private/PhysicsAssetUtils.cpp:889：primitive 工厂把骨局部每轴 extent 提升至 .5；ConvexDecompTool.cpp:163 过滤最大跨度小于 1 的 hull 输入。
- Runtime/Engine/Private/PhysicsEngine/BodySetup.cpp:2048–2064：直接 FKBoxElem 只按实际骨缩放变换，没有工厂 .5 下限。
- Runtime/Engine/Classes/PhysicsEngine/BodySetup.h:338、426、436：CreatePhysicsMeshes / RemoveSimpleCollision / InvalidatePhysicsData 公共导出；Runtime/CoreUObject/Public/UObject/Package.h:1204 提供 SavePackage。

根执行 author：现有原生 runner --filter Hearthward.AssetAuthoring055 --extra-arg=-Hearthward055AuthorPhysics。之后在新进程运行五项 Hearthward.Combat055 实际箭矢测试。Author 成功只证明拟合资产写入，不计入碰撞或装备验收。

已知边界：骨局部 AABB 是真实点簇的保守包络，邻接部位可能仍重叠；若世界第一命中或实际 BoneName 跨部位，保留原测试失败。CPU 数据缺失、无真实控制 body 或退化包络直接失败，不改 Mesh 或加入替代碰撞盒。两个文件保存若遇到 I/O 失败可出现先保存一份的状态，根需依据具体结果处理；本轮没有增加跨资产持久化事务。
