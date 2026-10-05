# TASK071 公共 UI 动作跨读档 runner

状态：QA 脚本已完成；两文件 AST PASS。子代理未启动 UE、模型、HTTP 或构建，运行结论 NOTRUN。Root 在完整 60 项模型矩阵关闭后串行执行。

文件：`verify_ui_loadpoint_pie.py`（233 行）与 `run_ui_loadpoint_pie.py`（48 行）。Host 使用 GUID 独立 SaveTestPool，输出 `Saved/Task071/ui-<label>-<pool>/`。仅复用 TASK068 现有真实 Ground、已建工作台、营地物资、实际 SavePoint 夹具；不运行其模型矩阵与 boundaries。

## 最窄实际路线

1. 记录 baseline A 的真实 SaveId、公开 Goal/count/Phase/CommandId。要求无材料目标且 requested/delivered/acquired/carried=0；Phase 原样记录，不要求 Idle，CommandId 不要求 invalid。
2. 走 `ExecuteAction(page:dialogue)` 与正常 `agentCollectCard`，公开 GetCandidate 核对 collect/wood/1/additional_acquired/S1、无 limits/unresolved。从公开 DescribeLayout 找唯一实际确认元素，使用其真实 rect 中心调用 ActionAt，核对返回的 GUID 与 GetCandidateId 一致。
3. 保持 dialogue 和候选存在，直接真实 LoadPoint(A)。先验禁止使用原 fixture 的 restore()，该 helper 会先 OpenPage(hud)，提前 CancelPending。这里必须实际 Load 改 epoch、HUD snapshot 回调自动回到 hud、清候选。
4. 重放捕获的原确认 Action：HUD 拒绝；正常重开 dialogue 后仍拒绝；公开库存与 Goal/count 无变化。再生成正常新卡，新 GUID 必须不同；原 Action 在新卡旁仍拒绝且不破坏新卡。
5. 新卡实际 ActionAt → ExecuteAction 确认，等待真实 COMPLETED，核对 Delivered/Requested/Acquired=1、Carried=0，camp wood+1/source wood-1，弟弟/玩家木材不变及所有物品总量守恒；2 秒 quiet 无额外结算。
6. 恢复 A，公开 TryAdd 一份玩家 wood，正常 save 页的实际保存按钮写 B，须只有一个新实际 SaveId 且 B 与 A 不同。通过正常菜单真实 `ask:load:A` 元素进入确认，捕获实际 confirm；外部真实 LoadPoint(B) 自动清模态，原 confirm 拒绝，B 库存保持。
7. 正常重开 save 页 → 实际 ask:load:A → 实际 confirm，须真正经过 normal OpenSavePoint 分支并恢复 A；epoch 改变、HUD 回到 hud、2 秒 quiet 无额外库存/任务量。
8. 初始和最终 GetGenerationCalls/GetServerProcessId 均 0；新脚本没有 SubmitPlayerText、ConfirmCandidate、SetStructuredGoal、get_editor_property 或 set_editor_property 调用。正常手动卡由真实 UI handler 内部调用公开生产 SetStructuredGoal。

## 公开接口和原路径依据

- ScreenWidget.h 的 OpenPage/ExecuteAction/GetPage/DescribeLayout/ActionAt 均公开 UFUNCTION。ScreenLayout172–188 的 DescribeLayout 导出实际元素 Action 与布局 rect；ActionAt 调用同一个 Hit。脚本不读 Elements 或私人索引，不搜索一条能通过的坐标。
- ScreenContent528–534 构造实际候选 GUID 的 agentConfirm；ScreenActions217–234 是正常手动任务卡。源默认注册能力第一项为 collect，第一材料 wood，来源 S1；脚本核对实际卡，失败时报告 fixture 前置，不写私有菜单状态纠正。
- ScreenActions190–193 页面与 GUID handler；AgentInteraction309–314 候选确认检查；ScreenWidget174–175 离开 dialogue 提前 CancelPending，205 清 ConfirmAction。
- Save Restore 的 OnSnapshotRestored → HUDDialogue SnapshotRestored → Screen.OpenPage(hud) 是实际跨 Load 链路。
- ScreenContent827–844 在正常 save 页构造 ask:load:<实际SaveId>；Resources/UI/interface.json save 页已有正常 save 按钮。ScreenActions284、286–297 与349–355 处理真实 ask/confirm/load。OpenSavePoint62–87 在已启用 prototype 时走 PrepareSession + LoadPoint，不读取 Python protected Point.World。
- CompanionFixture.h43–49 的量/Goal/CommandId 及 AI GetCandidate/GetCandidateId/GetGenerationCalls/GetServerProcessId 均公开 Blueprint API；初始 CANCELLED 是已有 HTTP runner 的实际 baseline，不假设 enum Idle。

## 隔离和信用边界

`runpy` 创建 dormant generator 后，在返回到 Slate 之前将其函数 globals 的 out 改为 Task071 独立输出、注销原 tick；然后 selected_cases=[]、case_limit=1，才 advance setup。原 fixture 仅 mkdir 旧 Task068/backend 目录，其 setup 日志与空 cases/boundaries jsonl 落新 Task071。冻结 dataset 与现有模型结果不改写。

信用仅公共 UI handler replay、真实 HUD snapshot reset、正常菜单 OpenSavePoint 和真实物资结算。**Slate event replay、physical input、model HTTP、NPC receipt/operation Python 检查均 NOTRUN**。没有按钮 UButton 本体的公开接口；当前界面是自绘 Elements，脚本捕获实际 Action 后复用同一 ExecuteAction，不称为旧 Slate 回调验证。Save13 Native 既有 receipt 幂等信用保持独立。

Root 执行例（示例命令未运行）：

```powershell
& G:/GameFactory/.venv/Scripts/python.exe G:/GameFactory/Hearthward/.agent-local/task067/docs/qa/TASK-071/run_ui_loadpoint_pie.py --label public-ui-action-replay
```

AST 检查：两个文件可解析；QA verifier 中无直接模型/私有/生产确认 API 调用；Host 无 AI backend、bundle、GPU 或模型启动参数。未运行 UE，因此未声明业务 PASS 或零 warning。
