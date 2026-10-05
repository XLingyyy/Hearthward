# 现有蒙皮数据可行性与最窄补采样

现有 `Saved/Task055/stone-axe-palm-skin/actual-hand-skin.json` **不足以准确离线预测**“局部径向 .5 + 五个第二指节半幅展直 + 调整 Grip”组合候选。当前仅做 JSON 字段、169 个顶点 influence、205 张面索引和本机公开 API 的只读核对，没有运行接触遍历、UE、构建或候选 authoring。

## 事实

- 169 个顶点的全部非零权重、section→mesh bone index 映射均已记录。raw influence sum 与原 sum 一致，没有缺权重。
- 只有已蒙皮的 component/world/hand-local 位置，没有原 LOD0 PositionVertexBuffer 位置，没有 InvRef 或实际 RefToLocal 矩阵。
- 已记录 hand_r + 15 指骨的世界 TF，共16骨。实际169顶点用了19骨。30个参与205面几何的顶点还使用 `lowerarm_twist_01_r`，其 TF 没有记录。孤立顶点2184另使用 thigh_r/thigh_twist_01_r，这两骨同样未记录；该顶点不参与205面。
- 当前蒙皮结果是多个 bone contribution 的加权和，缺少原顶点和各贡献的 reference→pose 变换，不能按 dominant bone 旋转或由一个混合结果拆出全部贡献。
- 之前选定的 palm 面 `[4265,4268,4258]` 也**不能作为完全固定的掌面**：4265有 thumb02 权重771/65535，4258有9509/65535。本次 thumb02 候选会改变它。另两张实际 dominant-hand_r 交叉面只有手/前臂/01 influences，02调整不改变它们。

## 补丁范围

`stone-axe-palm-skin-inputs-proposal.patch` 只扩充现有 `EquipmentPresentationTests.cpp` 的 `-Task055PalmSkinCapture` 输出。不创建动画副本，不改骨轨迹、持握、网格或生产接口，使用原 source .55 的同一采样。所有新字段来自当前真实 LOD、pose 和权重。原 JSON 字段和169顶点/205面选择保持。

每个选中顶点增加 `source_reference_mesh_cm`，取原始 LOD0 `StaticVertexBuffers.PositionVertexBuffer.VertexPosition(VertexID)`。为全部真实非零 influences 收集 mesh bone index，并输出每个 used bone 的 parent index/name、local/component TF、InvRef和独立实际 RefToLocal。预计包含19骨，以实际当次使用集为准；不按手指名过滤，因此必含 lowerarm_twist_01_r。

公开 API 已核本机5.8：`USkeletalMeshComponent::GetBoneSpaceTransforms`（阻塞正在进行的评估后返回local TF副本）；`GetComponentSpaceTransforms`；`USkeletalMesh::GetRefBasesInvMatrix`；`USkinnedMeshComponent::GetCurrentRefToLocalMatrices`。最后一个 API 返回渲染蒙皮矩阵，见 [Epic GetCurrentRefToLocalMatrices](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/Components/USkinnedMeshComponent/GetCurrentRefToLocalMatrices?application_version=5.5)。网格的 reference 接口见 [Epic USkeletalMesh](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/USkeletalMesh?lang=en-US)。所需类型和接口已在现有 Engine headers/依赖中，无新增模块。

## 坐标和矩阵约定

- 原 LOD0 source positions 是参考网格坐标。当前 CPU skinned positions 是 mesh component 坐标；二者不能直接当作 bone local。
- 导出的 FMatrix44f 每项按 `M[row][column]` 展平，**行向量** `[x,y,z,1] * M`。UE5.8 `SkinnedMeshComponent.cpp:4291` 的组合为 `InvRef * ComponentTransform.ToMatrixWithScale()`。
- 独立实际 RefToLocal 按当前 LOD0 + 全 used bones 获取。先用真实 raw_uint16_weight/65535 和导出的原始位置、矩阵复算原 pose，量化与真实CPU位置的误差；该一致性建立后才能推演改指骨。不得靠归一化、抬高容差或改已有位置掩盖差异。
- 五个02旋转改变其自身和03的 component TF；01和其他参与骨保持当前实测 component TF。19骨数据完整覆盖这五组01→02→03；其余骨使用原矩阵，不需要修改或重建其他骨姿态。手骨root尺度、mesh组件1.839和实际Held .7分别处理。
- 原采样记录 asset morph target count=0；新采样仍记录实际值，不擅自忽略新增 morph/cloth 影响。

## 一个后续有限方向

补采样完成后，只评估局部径向 **.5**、原 .7 总尺度/原轴向长度/Blade/socket、既定五02半幅候选这一组。Grip 位置由真实**不受02/03影响的掌面**和改后真实柄表面支持距离确定，沿该掌面的掌外法线做一次有几何依据的平移；当前没有完整数据和支撑点实测，因此不填新数值、不声称候选可用。此前 palm 面含 thumb02 influence，应先用数据选择稳定掌面或明确采用改后实际掌面。

需要一次新实际只读 skin 采样。这次采样可以先于任何动画 author/候选渲染；它为离线预测提供缺失输入，避免构建和实拍已确定失败的旧绑定。Root当前模型矩阵结束后再运行，现阶段不执行CPU密集碰撞分析。

输出沿用已有 `actual-hand-skin.json` 路径；Root应先保留当前旧证据，再执行补采样。补丁未编译或在UE中执行。
