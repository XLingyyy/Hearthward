# TASK-095｜Blender MCP 弓箭装备首件

日期：2026-10-08。基线：`1a4160831ac8049cde85a0ad6cef200fdf16bf57`，分支 `codex/TASK-084-103-iteration`。本报告覆盖新增独立装备资源，TASK-095 仍为 Active。

## 已制作与保存

通过实际 Blender MCP `execute_blender_code` 调用运行[制作脚本](../../../../art_source/TASK-095/author_archery_kit.py)，产出[可编辑源文件](../../../../art_source/TASK-095/ArcheryKit.blend)。几何与材质均为本项目原创制作，无下载模型、付费生成或新增第三方贴图。

|资源|实际 UE 尺寸|LOD0 三角面|细节|
|---|---|---:|---|
|Longbow|高 150.301 cm|3776|木弓、弦、皮革握把、两端弦槽装饰；握把为挂接原点|
|Arrow|长 79.600 cm|1504|木杆、金属箭头、三片箭羽、绑线、开槽箭尾；箭尖为原点，朝 +X|
|Quiver|连箭束高 78.400 cm|10764|开口皮革筒、内壁、底盖、收边、缝线、两个背部挂环和五支箭|

各资源使用一张 1024×1024 颜色贴图和一个材质。颜色已烘焙并打包进源文件，FBX 内嵌贴图，不依赖 Blender 程序材质在 UE 中运行。当前交付材质统一粗糙度 0.82，尚未制作独立法线、金属度或粗糙度贴图。

保存位置：`/Game/Hearthward/Assets/TASK-095/Archery/{Longbow,Arrow,Quiver}/`。准确 9 个包及源文件锁在制作前取得，见 [PACKAGE_SCOPE](../../../assets/TASK-095/PACKAGE_SCOPE.json)。静态网格碰撞已移除；没有替换任何角色、伤害判定、动画或武器绑定。

## 实际验证

- Blender 5.2.0 LTS 源场景已渲染全套与箭袋细节，源尺寸、面数见 [source-results.json](source-results.json)。该文件的 `ue_import: NOT_RUN` 是源生成阶段状态；后续导入结果以本报告与下列 UE 记录为准。
- UE 5.8.2 官方 MCP 实际导入、网格包围盒、三角面数、材质贴图输入、粗糙度输入、材质编译和 9 个包保存成功，见 [ue-import.json](ue-import.json)。三件资源 UE 包围盒与源几何乘 100 的结果相差小于 0.05 cm，三角面数与源模型一致。
- 已检查 UE 实际资产渲染：[长弓](SM_Longbow_Practical-ue.png)、[箭](SM_Arrow_Practical-ue.png)、[箭袋](SM_Quiver_Practical-ue.png)。这些是编辑器资产渲染，不是角色实机持握证明。
- 首次导出依赖 FBX 单位元数据，MCP 导入得到 1/100 尺寸，并丢失细小三角面。已删除本轮新建的错误试导入包，采用项目 R3 已有的实际厘米坐标方式重新导入；未修改导入器。原始失败记录保留在 `.agent-local/qa/TASK-095/blender-20261008/ue-import-before-centimetre-fix.json`。
- 仓库自检104份任务快照、0错误；制作脚本语法与两处MCP TOML解析通过，Git diff无空白错误。
- 未改 C++，未重复构建或运行既有战斗测试。游戏持握、拉弓变形、碰撞行为、LOD 距离、最终 Cook 与 Owner 视觉签收均 **NOT_RUN**。

## 射手身体的已确认问题

原 `Archer.blend` 主体有 46063 个顶点，其中 45989 个属于同一连通网格。短刀和盾牌与人体连体；切除预览暴露出盾牌后手臂表面不完整，并会伤及袖口与手部。试验仅发生于内存与本地 QA 文件，未保存到原角色文件，也未导入 UE。下一步需要补建手臂与手部表面、恢复 UV/权重，再进行动作检查。不能把这组三件独立道具算作专用射手已完成。

原五角色制作源、骨架和游戏绑定保持原状。角色修补、短刃等武器的正式挂接、拉弓/倒地/扶起动作仍是 TASK-095 未完成项，无新增待定设计。

![弓箭装备源渲染](archery-kit.png)
