# TASK-085｜本地实现交接

[任务单](../tasks/TASK-085.md) · [元数据](../tasks/TASK-085.json) · [报告](../qa/TASK-085/REPORT.md)

2026-10-07。根目录`G:/GameFactory/Hearthward`；实际分支`codex/TASK-084-103-iteration`；基线`6fcf5c22e965f0f7409438f19bc7b09e96ffb058`。本单为dirty实现，未提交/未推送。用户本轮要求逐项执行084—103；根Agent指定task_audit实施085，README及UE构建/运行由根Agent串行执行。

新增ReadModels.h/.cpp提供`FHearthwardCompanionWorkView`、`FHearthwardCampWorkView`，复用既有Command/Phase/CampRegion/Storage epoch；无源和无任务数量为Unknown，保留终态记录，不以采得量或携货宣称完成。个人Resume/Cancel在ScreenActions.cpp先做当前投影/阶段/交流检查，执行仍使用ResumeBlocked/CancelExecution。未修改Save格式、玩法数值、Content、字体、UI主题或刷新框架。

ScreenActions写窗口已释放；ScreenWidget.h、ScreenLayout.cpp及UI两个JSON未改。086可消费ReadCompanion/ReadWorkParty/CompanionPhaseText，并用旧View的epoch/CommandId接CanApplyPersonalAction；085现值重读不能独自代表新状态卡的旧快照绑定已完成。队伍CompletedBatches为批数，原份数沿DescribeWorkParty。

定向原生过滤器：`Hearthward.Iteration.Task085.`，四项覆盖个人计数守恒/不提前完成、提案与执行分离/epoch、Missing/异常数量、真实世界来源销毁/队伍只读。主代理实际Development Editor构建成功，原生4/4成功，无警告/错误；证据见`docs/qa/TASK-085/native-baseline-20261007.json`及源联合index。渲染、真实输入、模型：NOT_RUN。联合轮其他任务失败另行修复，085不据此宣称整轮或人工验收通过。

README同步：由根Agent待执行。范围基线检查：旧HEAD不含本批任务快照，NOT_RUN，不修改验证器；本次未取得提交授权。Owner/Reviewer体验签收、真人和二机不代填。本单源码缺口、接口采用与验证边界见报告。
