# TASK-055：由保守 box 改为真实点凸包的 author 增量

2026-10-04。根实际验证 fitted box 后：两个身体箭线和拒绝磨损测试绿；Hero / Brother 的脚部 body-center 箭线第一命中 calf_l，保守 box 留有相邻部位重叠。这里保留原世界第一命中及装备部位断言，只提高真实 PhysicsAsset 几何精度。

补丁 physics-convex-authoring.patch 基于根当前已集成 author 的增量，只改现有 ProjectileBodyProtectionTests.cpp 的候选类型、真实点聚合、预检 cooking 及 AggGeom 写入。仍由 -Hearthward055AuthorPhysics opt-in 枚举同一双资产 complex 案例。

每个 FKConvexElem.VertexData 直接保存已批准 dominant/control bone-local 真实顶点，去除相同坐标重复，无盒体角点或相对 padding。UpdateElemBox 更新包络。每个候选先在 transient USkeletalBodySetup 中沿相同 CollisionTraceFlag 调已有 Engine CreatePhysicsMeshes；只有 GetChaosConvexMesh 非空才接受候选。Engine FinishCreatingPhysicsMeshes_Chaos 仅对 IsValidGeometry 的 hull 安装该指针，因此这项预检验证实际 Chaos 接受候选，且两份所有候选全部通过后才改持久 PA。保留所有骨名、约束、profile 和 Mesh 关联，不保存 Mesh。

本机 UE 5.8 源码依据：
- Runtime/Engine/Classes/PhysicsEngine/ConvexElem.h:35–44、78–79、103–106：真实 VertexData、ElemBox、UpdateElemBox 和公开 cooked-hull getter；构造/拷贝/API 均由 Engine 导出，源码无需直接调用 Chaos API 或新增 module。
- Runtime/Engine/Private/PhysicsEngine/BodySetup.cpp:258–285：GetCookInfo 将 ConvexElems.VertexData 放入标准 convex cooking 输入，不进入 V-HACD。
- Runtime/Engine/Private/PhysicsEngine/Experimental/ChaosCooking.cpp:165–197：只跳过空顶点数组，直接构建 FConvex(vertices, margin0)，没有 V-HACD largest<1 / smallest<.1 过滤。
- Runtime/Experimental/Chaos/Private/Chaos/CollisionConvexMesh.cpp:16–19、172–181：默认启用 TConvexHull3；后备 legacy horizon epsilon 通过 SuggestEpsilon 按输入坐标尺度生成（Public/Chaos/CollisionConvexMesh.h:61–90）。标准路径能接收局部 .1–.4 的非退化真实点簇，最终由 author 的实际 cooking 判断。
- Runtime/Experimental/Chaos/Public/Chaos/Convex.h:133–140：默认合面 DistanceTolerance=1；Public/Chaos/CollisionConvexMesh.h:506–564 先要求法线 dot>1-1e-4 再比较距离，故该数值不是整体尺寸过滤。默认 planar 检测使用约1e-4（.h:116–141），这仍是数值精度边界；不修改全局 cvar 或引擎。
- Runtime/Engine/Private/PhysicsEngine/BodySetup.cpp:621–636：仅 IsValidGeometry 成功时安装 cooked convex 指针。

子代理没有启动 UE/build/Git或修改 uasset。根应再执行一次 opt-in author，随后新进程仅重验两项真实护甲箭线。凸包 author 不计入装备验收，不能按实际 hit bone 改 expected slot。
