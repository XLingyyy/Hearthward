# TASK-095 射手修补与箭显示

2026-10-08，分支 `codex/TASK-084-103-iteration`，修改基线 `eefa9a2d828fe0454eeb4c4136c4c4ca1824b605`。本轮范围为射手预览资产与投射物显示，任务仍为 Active。

## 制作结果

- [可编辑源](../../../../art_source/TASK-095/Archer-Repaired.blend)保留原22骨层级，移除连体盾牌、短刀和旧箭袋；从完整手臂镜像重建缺损手臂，映射变形权重并焊接肩部。原始 `Archer.blend` 保留。
- 长弓、箭袋和皮革背带已绑定到对应手臂与躯干骨骼。四个网格均无未赋权顶点，详细面数见[source-export.json](source-export.json)。肩部采用独立布料材质，细节仍需后续视觉打磨。
- 已导入并保存 `SK_Archer_Practical` 和两个新材质，共3个UE包。复用Guard骨架与身体材质；原骨架包未变脏。实际包围盒最低点约0 cm、最高点102 cm，见[UE记录](ue-import.json)。
- 导出检查发现原人体对象含约50 cm平移。已在临时导出副本上烘焙对象变换，修正模型下沉；制作源骨骼数据保持不变。
- 玩家与敌方箭投射物使用现有79.6 cm箭模型，箭尖保持为轨迹判定原点，朝向实际速度。石头、诱饵、重力、命中与伤害逻辑不变；落地存档箭恢复既有角度。

## 验证边界

Blender MCP实际制作和三视图渲染成功；UE MCP导入、材质绑定、资产渲染与保存成功。源导出JSON中的 `ue_import: NOT_RUN` 仅记录导出阶段，后续结果以上述UE记录为准。

Editor编译通过；原生2项射手/箭显示、5项敌箭命中/护甲、7项战斗音效最终均通过。5项旧命中夹具各有1条未调用EndPlay的清理警告，原样保留，未计为零警告。见[构建与验证汇总](validation.json)和各原始报告。

命中回归首次复现音效入口崩溃：测试世界设置 `AllowAudioPlayback(false)`，旧入口仍生成声音并读取空GameInstance。已在读取资源及创建组件前遵守该世界开关；依据[UE世界音频契约](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/FWorldInitializationValues)。5项命中回归与7项战斗音效回归验证该修复。音效读档测试首次漏传必需的隔离UUID档池参数，补齐后仅重跑该项并通过；初次失败记录保留。

仓库104份任务快照自检0错误；制作脚本Python语法通过。已移除本轮冗余 `.blend1` 与临时UV参考副本，保留原源文件和本轮完整可编辑资产。

范围检查以已提交的授权快照 `5ea1bedadada60e9f2b4cc06b97a24409e45624f` 为基线，25条改动路径全部通过，0错误。普通UE编辑器已通过UEClient重开，官方MCP连接和Bootstrap关卡查询通过。

射手仍为预览资产，尚未替换Campaign射手。拉弓/放箭动作、移动持弓、倒地/扶起、正式Cook、实机连续观感及Owner视觉验收仍未完成。原人体Tripo来源的授权核实保持待办；本轮原创弓箭和背带不能替代该核实。无新增待确认设计。

![源正面](front.png)
![源侧面](left.png)
![UE资产渲染](archer-ue.png)
