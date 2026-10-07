# TASK-086｜本地实现交接

[任务单](../tasks/TASK-086.md) · [元数据](../tasks/TASK-086.json) · [报告](../qa/TASK-086/REPORT.md)

2026-10-07。根目录 `G:/GameFactory/Hearthward`；实际分支 `codex/TASK-084-103-iteration`；基线完整 HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`。本单使用当前未提交的085显示适配产物，实现仍为dirty工作树；未提交、未推送、未发布。用户当前会话授权本地执行084—103，根Agent委派mcp_setup实施086，UE生命周期、统一构建/运行和README由根Agent负责。

## 可用产物

个人状态页由 `ReadCompanion` 的权威快照显示已交付/目标、携带、尚未交付、完整phase、已知容器角色与实际停工原因。完成和取消记录保留可见，缺失量保持未知。营地队伍独立显示族人人数、弟弟另占名额、实际到岗/移动/受阻、暂停和累计入库；产量沿现有每批2份口径，不显示完成百分比。

`GetCurrentWorkContent` 每次读取085适配器；Screen与旧HUD中的task_status回复在展示时重读当前事实，避免仅显示推理完成时的旧数量。个人委托与队伍动作各自显示，操作许可仍使用现有权威Preview/ResumeBlocked/CancelExecution/候选确认入口。详细数量集中在工作页，主HUD保留工作入口、阶段和受阻原因。

状态卡动作携带值快照标识：个人为epoch/CommandId，队伍为epoch/Region/worker IDs。执行瞬间重读权威视图、核对标识和当前许可；队伍继续使用已有成员数量，不借继续动作补招。旧个人卡和旧队伍成员/时间线卡拒绝执行。提案确认仍沿085以前的CandidateId契约，本单没有新增提案状态机。

新增 `CompanionStatusViewTests.cpp` 注册三个原生用例：`Hearthward.Iteration.Task086.PersonalStatus`、`PartyStatus`、`StatusActionBinding`。它们覆盖投影计数/终态/未知、队伍累计与到岗文案、动作标识变更，不替代真实采集、入库、OS输入、模型或保存恢复用例。

## 写入窗口与校验

所有086源码窗口已释放给根Agent：ScreenWidget.h、ScreenActions.cpp、ScreenContent.cpp、ScreenDialogue.cpp、HUDDialogue.cpp、PresentationReadModels.h/.cpp与新测试。后续088可接入共享文件。mcp_setup不再写这些文件；本交接及QA为086独立文档收尾。

定向 `git diff --check` 已通过。修改前的源码条件复现记录位于 `.agent-local/qa/TASK-086/20261007-source-before/reproduction.json`，确认旧终态被Active过滤、个人/队伍按钮互斥和模型回复缓存路径；未宣称运行时RED。task_audit做只读内部核对后由根Agent实际构建/执行原生，结果见下段。核对发现的085 fish目的地错误已经根Agent修正为弟弟背包，沿实际MoveTo Nature/CommitNature收获路径。

## 交接后的最窄验证

2026-10-07根Agent通过公开UEClient统一构建成功；过滤器 `Hearthward.Iteration.Task086.` 实际发现并执行3项，全部Success，零警告/错误。构建与Native原始报告分别为`.agent-local/qa/TASK-085-088/build-repair2-20261007/result.json`、`native-baseline-20261007/index.json`，086子集见QA中的`native-baseline-20261007.json`。本代理未启动/关闭引擎。公共仓库检查按任务指南由根Agent统一执行。

真实端到端优先复用078耗尽14/32隔离夹具、079查询与续接、081队伍到岗/暂停/继续脚本；旧脚本结果和宽松按钮断言不能代替本次断言。C01另建立carried6并实际入库；C06用旧状态卡实际换Command/Load后点击；C07在大字号与4:3中渲染；C08独立重启验证携货与暂停队伍恢复。全部尚待执行，详见报告逐项限制。

未发现需要人工确定的新玩法设计。README由根Agent收尾。Owner视觉、真人体验、二机、性能和发行未验；无正式验收结论。范围基线检查尚未执行，旧HEAD未包含本批任务快照；未获提交授权，不修改验证器或伪造基线。
