# TASK-093｜营地反馈本地实现交接

[任务单](../tasks/TASK-093.md) · [元数据](../tasks/TASK-093.json) · [本批指南](../planning/TASK-084-103/EXECUTION_GUIDE.md)

## 当前状态

2026-10-07，mcp_setup实施、根Agent统一构建/验证/README。实际分支`codex/TASK-084-103-iteration`，根目录`G:/GameFactory/Hearthward`，完整HEAD`6fcf5c22e965f0f7409438f19bc7b09e96ffb058`上的dirty实现。093首轮实际Native3项：ReceiptTimeline Success（0 warnings/errors），其余2项因夹具焦点初始化遗漏Fail。纠正夹具后根Agent实际重跑2项均Success、合计2 warnings/0 errors；ReceiptTimeline复用前轮成功。原始RED子集保留。

## 实际产物与窗口

只写允许的ScreenCamp.cpp、ScreenContent.cpp、ScreenWidget.h、新CampFeedback.h/.cpp与CampLoopFeedbackTests.cpp及自身文档。所有源码写锁已明确释放给root并冻结。ScreenActions未修改；CampSubsystem/WorldPresentation/Save/Content/配表未扩范围。

发展页读原条件/材料与Available缺口/批准解锁，八阶表保留。共享人口/占岗计数一次，本营设施按玩家所在Camp筛选。升阶卡绑定当前tier+epoch后走原UpgradeCamp，旧卡拒绝，不重复扣料。回营按真实稳定人物、等阶转移与原NPC入库操作GUID去重；首次/新epoch播种旧ID并清空会话摘要，最多3条，HUD既有2秒通知。没有来源时显示当前营地状态，没有奖励/净库存归属推断/新Save字段。演员模型复用原PresentLabor，只补真实状态文本。

## 实施与证据

过滤器`Hearthward.Iteration.Task093.`，实际执行3项。ReceiptTimeline是既有schema事件注入投影夹具；其余2项含真实Standalone Widget/Storage/原升阶与两营地设施列表。首轮失败日志仅见夹具漏SetIsFocusable使OpenPage→ApplyInputMode尝试非Focusable SObjectWidget的3条Error，共5 warnings，未出现业务断言失败，总结果仍FAIL。生产InitializeScreen已设置Focus，根Agent授权补夹具创建Screen后、TakeWidget前的SetIsFocusable(true)，没有生产修改/ExpectedError屏蔽。correctedfixture两项实际Success，各1条Standalone EnhancedInput warning、0 errors，原始证据`.agent-local/qa/TASK-090-100/native-red-20261007/index.json`，导出`docs/qa/TASK-093/native-correctedfixture-20261007.json`。首轮RED导出与REPORT继续保留；diff --check PASS。

101可复用Screen->DescribeLayout实际visible components：`camp.return.summary` text、`hud.camp.return` text、`camp.growth.upgrade` action。真实Apply/Load后提交旧action应拒绝，营地摘要回到当前状态且不补弹。没有新增公开持久接口。

## 交付与权限

README归根Agent。任务Active，无新设计确认项；3项原生均有成功证据（ReceiptTimeline复用前轮），correctedfixture两项已有实际重跑；渲染/真实输入/Owner视觉/真人/二机/发行尚未验。未提交、未推送、未合并、未发布。

## 下一位Agent

本单定向原生已建立合理信心，未出现新相关变动时不重复同一测试。后续实际首救/工作台I/S2、两营地、生产并行、睡眠/Load/渲染切片逐层登记。当前不得覆盖root共享UI写入；后续源修正统一协调。
