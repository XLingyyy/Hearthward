# 实际石斧柄部拓扑只读导出

2026-10-04，Root 使用 UEClient 公开启动／关闭接口，实际 UE 5.8.2 编辑器完成只读导出；CLI exit 0，sampling_complete=true，进程关闭成功，未保存或修改资产。

- 原失败独立保留 `Saved/Task055/stone-axe-handle-topology-api-red` 和 QA `*-api-red.json`。GetTriangleVertices 实测返回六项；源码 `MeshDescriptionBase.cpp` 先 SetNumUninitialized(3)，再由 Algo::Copy/Output.Add 追加三个真实顶点，前三项不能作为几何数据。
- 当前改用公开 GetTriangleVertexInstances → GetVertexInstanceVertex。首个实际 TriangleID=6，经该映射与逐角 GetTriangleVertexInstance 两条公开读取均得 [414,415,1]；只将旧 API 六项用于诊断，未作为接触输入。
- 实际 LOD0 源共有 47631 顶点／95278 三角形。选择与实测 BladeBase Z=10.305906295776367 cm 下方任一顶点相连的所有三角，包含跨区域边界面；得到 28091 个真实三角。输出保留实际三角／顶点 ID 和已保存 Grip／BladeBase／BladeTip socket。
- 第一次正确导出时 QA 分支漏设完成标记，CLI 仍 exit 1；`*-completion-flag-red.json` 保留该失败。完成标记修正后真实复跑 exit 0。未修改引擎。

原始数据：`Saved/Task055/stone-axe-handle-topology/stone-axe-handle-ue-local-topology.json`；API 对照：同目录 `stone-axe-handle-topology-api.json`；运行／关闭记录：本目录 `stone-axe-handle-topology-{launch,results,stop}.json`。

这些数据提供实际表面，尚未证明掌心握持合格。实际手皮肤子集存在开放边界；必须按真实变换分析三角接触并核看渲染，不能以点云或轴线未交叉替代接触验收。当前两候选仍未写入生产握点。
