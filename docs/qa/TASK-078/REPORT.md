# TASK-078 采集委托与族人分工修复

> 集成更新：2026-10-06 用户明确要求“提交并推送到main吧”，授权 TASK-078—081 成果提交、推送及 main 集成；本次不更新 Release，不代填人工验收。 以下实现与验证记录保留开发时状态；源码随本次集成进入 main，历史“未提交／未推送”描述仅指验证当时。

2026-10-06，UE 5.8.2 / Win64 Development Editor，本地分支 `codex/TASK-078-companion-gathering`，基于 `ca21120cef4421d039c44248a0cfa5043c248feb` 加本单未提交补丁。未替换现有 Shipping Demo 或 GitHub Release。

## 诊断

先只读复制玩家 0.2.0 Demo 的存档池，再通过引擎原生 `HearthwardSave::Read` 解码副本。最新节点实际记录：wood，requested=32，delivered=14，acquired=14，carried=0，WaitingAtCamp，BlockReason=实际资源不足，SourceRef=S1，指定点木材=0。原件未写入，存档及完整原始日志不进入 Git。

`collect` 合同限定指定安全采集点，点位耗尽后返营等待是现有规则。新版 HUD 遗漏阶段与阻塞原因，且头像旁“进度”读取了玩家追踪任务的数量。委托移动另有硬编码 180 cm/s。以上均已从源码与实际存档确认。

## 改动

- 委托往返使用现有 sprintSpeed（600 cm/s）并乘背包负重系数；共享导航继续施加严重生存状态的 0.8 倍减速。
- HUD 显示弟弟真实委托进度、携带量、执行阶段；受阻返营／营地等待／安全停留时持续显示具体原因及处理路径，不在两秒后消失。
- 手动木材任务下单前显示指定点实际余量。指定点用尽时指向“营地管理 → 田野与牧场”另选点位；没有偷偷更换任务来源或生成资源。
- “重试返营”改为“继续未完成委托”。继续前检查当前采集点资源，不能恢复时保留进度并说明原因。工具不可用单独报告，避免误报成容量不足。
- 建造页营地入口标明族人分工；发展页可直接进入分工；工作页提供伐木区／采石区快捷按钮及分配、开工、入库说明。

## 验证

- Editor Development 构建通过；生命周期和原生测试均使用 GameFactory `UEClient` 公开 API。
- `Hearthward.Companion078.InspectSavedGathering`：只读诊断通过。它用于读取已指定的本地副本，不代表游戏行为回归。
- `Hearthward.Companion078.GatheringDepletionAndResume`：1/1 通过，包含真实动态导航、PathFollowing、角色移动和计时采集。复现 14/32 返营等待，空资源继续被拒绝，测试夹具补充 18 个真实库存后续做至 32；共享仓储恰好收到 32 个。跑步、负重及严重状态减速断言通过。引擎在夹具初始化时记录一条 CrowdManager 尚未找到 RecastNavMesh 的警告；之后完整导航探路与实际抵达断言通过。
- PIE 手动任务卡和族人分工：26 项检查通过。使用独立档池、明确的夹具货物和空来源，验证真实任务卡确认、14/32 状态、三秒后的持续 HUD、拒绝空资源恢复、伐木／采石快捷按钮命中、两名族人的分配及两个生产队列开启。此项使用 UI 动作 API，不冒充物理键鼠验收；全部采集往返由上述原生测试覆盖。
- 实际 Widget 渲染检查：HUD、任务卡、分工页。修正了初版余量提示与角色名字重叠的问题。截图是 Widget 单独渲染，HUD 透明背景显示为黑色，不表示游戏世界黑屏。
- 初次行为测试夹具缺少有效斧头，失败结果未用作采集行为结论；补齐真实工具后完成上述回归。原始失败记录保留在本机 `.agent-local/qa/TASK-078/`。

引擎行为证据见 [native-results.json](native-results.json)，UI 操作见 [pie-results.json](pie-results.json)，脚本见 [verify_gathering_ui.py](verify_gathering_ui.py)。截图：[持续停滞提示](blocked-hud.png)、[任务卡余量](task-card.png)、[族人分工](clan-work.png)。

任务路径校验 `validate_repo.py --task TASK-078 --base ca21120cef4421d039c44248a0cfa5043c248feb` 尚受仓库流程门槛限制：基线没有 TASK-078 的已批准任务快照。没有修改验证器绕过门槛，也没有代填人工审核或推送权限。普通 `validate_repo.py` 自检通过（79份任务快照，0错误），`git diff --check`无空白错误。

最终余量提示仅缩短文案后再次构建通过，并单独重做[任务卡画面检查](card-visual-results.json)；该文案修改不影响已通过的采集、导航和分工行为，未重复运行这些测试。

## 使用与边界

运行本仓库“启动测试版游戏.cmd”可使用已编译的本地修复。已发布的 0.2.0 包仍为旧版本。本轮不改存档格式，不改用户存档，不替换 Release。

目前手动木材委托仍限定一个指定点；资源不足时保留已交付数量并等待玩家处理，不自动跨点采集。玩家的 14 个木头已入库，剩余 18 个需要另选有资源的点位委托或安排族人生产。普通族人采用后台劳动规则，产物进入共享仓储；本轮没有新增实体族人行走 AI。

技术参考：Epic 官方 [CharacterMovementComponent](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent?lang=en-US) 与 [Move to Actor](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/AI/Navigation/MovetoActor)，沿用工程既有角色移动及导航组件。
