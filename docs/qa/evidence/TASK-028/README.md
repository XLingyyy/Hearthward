# TASK-028 验证记录

工作树基线：`28e7c52e10e3988a19ca1e4b7e4fcf86ab2f3b93`；任务分支：`codex/TASK-028-3d-assets-integration`。环境：Windows、UE 5.8.2、Win64 Development Editor。这里记录的是本任务分支的定向验证，不是main或Owner验收结果。

| 验收项 | 状态 | 本轮证据与边界 |
|---|---|---|
| A1 清单 | PARTIAL | [逐件清单](../../../assets/TASK-028/asset-manifest.json)：35件源FBX候选，15件已导入、当前13件已应用，20件暂缓，9个装备模型缺源。木桌与篝火当前由TASK-030同源Demo资产接管；核心防具缺项。 |
| A2 导入 | PARTIAL | [导入](static-import-report.json)、[贴图](texture-fix-report.json)、[重开](reopen-inspection.json)证实15件的包、材质槽、贴图设置；房屋拼接仍是候选视觉，未做全部类别导入。 |
| A3 建筑 | PARTIAL | [最新运行路径](runtime-qa-after-task030.json)在 `be9286f` 基线验证8类部件、门与台阶真实行走、TASK-030篝火/工作台预览、建成、扣料和读档；隔离测试档仍注入20木材和4石材。TASK-030已接通自然图采集，无注入联合回归待测。 |
| A4 物品/武器 | PARTIAL | 同一运行路径在TASK-027合入前验证斧头装备、卸下、再次装备、手动存读档和继续游戏恢复；TASK-027主角骨架现已合入main，最终手部socket及攻击/采集动画联动仍未验。 |
| A5 防具 | BLOCKED | 用户确认没有可穿戴防具FBX；TASK-027骨架现已合入main，但未以静态模型或灰盒冒充穿戴成果。 |
| A6 场景兼容 | PARTIAL | TASK-029/030新基线下正常标题页新游戏与继续游戏、自然图房屋生成、两种设施和斧头恢复49/49项通过；旧档全量兼容须复验。 |
| A7 开销交接 | NOT_RUN | 缺同画质导入前后固定路线帧时间/显存、Standalone长路线及Owner视觉验收。当前三张DX12截图仅供自查。 |

## 证据文件

- `baseline-build-summary.json`：已有代码基线构建结果；不可代替本轮最终构建。
- `integration-build-summary.json`：本轮最终C++代码的Editor目标构建命令、结果和退出码。
- `static-import-report.json`、`texture-fix-report.json`、`reopen-inspection.json`：15件资产导入与新编辑器实例检查。
- `material-diagnostic.json`、`backing-materials.json`：原地板图集诊断与2个任务专属补缝材质；最终截图报告记录实际使用的内衬材质路径。
- `runtime-qa-full-route.json`：全路径定向测试；`test_only_material_injection` 和 `isolated_pool_id` 标明测试隔离条件。
- `runtime-qa-after-task027.json`：合入main `ba547c0` 后再次执行同一隔离路径，48/48项通过；仍使用测试材料注入，不代表正式自然图资源循环。
- `task027-merge-build-summary.json`：合入TASK-027后Editor目标构建退出码0，8个构建动作，73.66秒。
- `task030-merge-build-summary.json`、`runtime-qa-after-task030.json`：合入main `be9286f` 的TASK-029/030后，Editor构建退出码0（58动作、231.01秒），隔离存档路径49/49项通过。测试注入20木材、4石材；新游戏已自带石斧，因此未重复注入。
- `runtime-qa-after-task030-attempt1.json` 至 `attempt4.json`：合并后失败尝试依次定位到新增石材费用、工作台预览地点、新测试脚本不可反射的C++方法、初始石斧重复注入；均已在最终脚本修正，失败记录保留。
- `visual-capture.json`、`house-exterior.png`、`house-entry.png`、`house-interior.png`：PIE DX12 SM6、默认工程画质、固定观察相机，截图未经后处理；报告记录相机位置。

运行命令：`python -X utf8 scripts/assets/TASK-028/inventory.py --check`；`python -X utf8 scripts/assets/TASK-028/run_runtime_qa.py`；`python -X utf8 scripts/assets/TASK-028/run_visual_capture.py`。UE操作通过上层GameFactory `UEClient` 执行，测试存档池与用户正常存档隔离。最终构建命令与结果见JSON摘要。录像及真实人类视觉验收均未取得，因此不标PASS。

仓库范围门禁已运行但未通过：TASK-028的main基线JSON未预列两处必须修改的UI实现文件，且缺真实Issue URL和独立Reviewer；全仓还报告既有任务字段错误。用户现已明确批准两处UI文件纳入TASK-028，并记入工作树JSON；路径门禁核对的仍是main旧快照，尚不能通过本地工作树修改识别该批准。最初的运行路径和截图来自合入TASK-027之前的实现快照；含TASK-029/030的新基线已单独重测构建与49项路径，视觉截图仍未重拍。完整错误及后续处理见[单任务交接](../../../handoffs/TASK-028.md)。
