# TASK-055 固定足部射线只读诊断

2026-10-04。依据根 `Saved/Task053/fitted055-convex/index.json` 与 UE 5.8.2 实际源码；未执行 UE、修改资产或修改测试。

实际验收五项中四项通过，仅 HeroArmor 的 feet 射线首命中 `calf_l`。Hero foot_l 的世界包围盒中心为 (8.952,-6.798,111.943)，测试固定从此点 X-300 射到 X+300；此 Y/Z 同时落入 calf_l 的世界 AABB：Y[-23.329,3.203]、Z[107.800,165.958]。这证明射线经过两个宽相包围盒，尚未证明实际凸包是否遮挡、渲染皮肤是否遮挡或 cook 扩大几何。Brother 同用拟合方案且四部位已通过，不能据此排除 Hero 的实际姿态差异。

`Author055 hullPoints` 是 `FKConvexElem.VertexData.Num()`，并非 Chaos 实际简化凸包的顶点数。`BodySetup.cpp:1903–1940` 的 SetConvexMeshObject/ComputeChaosConvexIndices 在已有 VertexData 上建立 IndexData，保持输入点；不能用 842/526 点断言实际 cook 保留全部细节。`KAggregateGeom.cpp:474–486` 的公开 `FKConvexElem::GetPlanes` 则读取实际 Chaos 凸包 planes，无需引入 Chaos 模块。`CalcAABB:466–471` 使用原始 ElemBox，仍不能代表 exact cooked hull。

`Chaos/Convex.h:133–140` 采用 MergeFaces 距离阈值 1.0；`CollisionConvexMesh.h:521–545` 先要求两面 normal dot > .9999，再检查距离。存在局部小尺度敏感性，但尚无证据确认它导致此次失败。直接 cook 不经过 V-HACD 的 MinSize=1 过滤；已有非空 hull 和实际箭命中可确认该路径有效。

当前 fixture 运行 NotifyBeginPlay 后 RefreshBoneTransforms，没有世界时间 tick。Hero 原生 proxy 初始 GroundSpeed/ActionState 都为 0，绑定 Idle clip。`SkeletalMeshComponent.cpp:2958–2975` 在首次刷新时会确保 TickAnimation(0) 并评估。应把当前姿态作为实际初始 Idle 图姿态，不能未经对比认定为 RefPose。

建议最小新增只读诊断，继续保持同一条已失败射线与 boots 预期：

1. 对真实 calf_l 和 foot_l 的 `FBodyInstance::LineTrace` 分别输出命中、Distance、ImpactPoint；同时保留原 World.LineTraceSingleByChannel 结果。该调用仅回答两真实 body 的遮挡顺序，不跳过首命中。
2. 通过 GetPlanes 输出两凸包实际 planeCount；对每 plane 记录原 VertexData 的最大正向超出、最小/最大 PlaneDot；将固定射线变换至 element/bone local 后裁剪 cooked plane 半空间，记录交点区间。输入点 outside 或有效 supporting plane 丢失才支持 cook 形状误差推断，数量减少本身不够。
3. 输出当前 calf/foot bone transform 与 RefSkeleton 累积 ref transform 的差；若仍需区分真实皮肤遮挡，使用现有 Engine ComputeSkinnedPositions 与 LOD0 实际 index buffer，对同一条射线计算实际三角形相交，记录首个表面的实际 dominant/control body。不得采样寻找通过线。

阶段 1、2 优先；若 exact cooked hull 本身遮挡，需要阶段 3 才能区分合理凸包包络与渲染皮肤穿插。几何证据收齐前不收缩 PA、不改变 slot 映射、不改 expectedSlot，也不把诊断或 author 测试算验收。
