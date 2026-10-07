# TASK-093｜营地准备与回营反馈

2026-10-07，mcp_setup 实施，根Agent统一构建/运行与README收尾。根目录 `G:/GameFactory/Hearthward`，分支 `codex/TASK-084-103-iteration`，完整基线HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058` 上的未提交工作区。用户本会话已授权逐单本地实现；无新玩法设计确认项。未更改 Content、Runtime、模型锁、Save、Camp生产计算或原升阶经济。

090投影已落盘且本轮Native 1/1 Success；092首轮诊断RED后由其写者修复，最终GREEN待根Agent。前置PASS不代表本单通过。

## 事实、源前缺口与实际实现

字段/提交入口见[实施入口与事实映射](IMPLEMENTATION_PLAN.md)。修改前发展页只列材料需求和第一个失败条件，设施列表未限定当前营地，裸升阶动作未绑定等阶，没有基于真实回执的会话回营摘要。该证据为源码条件复现；实际运行RED未执行。

新增 `HearthwardCampFeedback` 只读辅助，从原 conditions/cost/unlocks、共享 `Storage.Available` 读全部条件与缺口。人口/占岗/空闲按真实区域成员ID全局计数一次；设施列表按玩家所在Camp筛选。工作状态从启停/安全/设施/有限资源/真实兄弟效率读取。八阶表保留「八阶一览」入口。原S2木48/石24、工作台I及首救条件全部沿原配表，无新经济规则。

升阶按钮 `camp.upgrade:<epochDigits>:<当前tier>`，执行瞬间验证当前epoch与tier后调用原 `UpgradeCamp`。旧等阶卡不能再扣下一阶材料。原设施、配方、移动/拆除/睡眠路径继续走原API，没有自动造设施或调人。

回营来源为已验证营地状态转移及公开NPC事件：新增稳定获救PersonId、真实tier上升、`delivered`且空Reason的真实仓储成功事件。操作GUID为原Event.Id，其他目的地/救援/批次事件不混入。库存净变化从未用于推断归属。会话摘要最多3条，HUD沿既有两秒通知；显示不发奖励。

首次读取、epoch或Campaign改变时播种当前获救人物/等阶/已有事件ID，清空本次摘要与通知。Load后的已有事件不会补弹，同epoch过去事件不重放。没有会话来源时明确显示「当前营地状态 · 本次会话尚无新回营回执」。没有新增Save字段或跨系统订阅。

现有 `CampaignWorld::PresentLabor` 已用稳定族人身份、真实岗位/有限资源/批次与暂停条件表现，本单复用原模型并仅补状态文本，不新增演员/搬运模拟/生产时钟。工程参考Epic官方[UI事件更新说明](https://dev.epicgames.com/documentation/unreal-engine/driving-ui-updates-with-events-in-unreal-engine?lang=en-US)，沿原0.2秒UI刷新消费有限事件和状态，未引入全局服务。

## 验证与限制

新增 `CampLoopFeedbackTests.cpp`，过滤器 `Hearthward.Iteration.Task093.`。根Agent已实际发现并执行3项：ReceiptTimeline Success，LocalFacilitiesAndWork 与 UpgradePreviewAndReplay Fail。原始证据 `.agent-local/qa/TASK-088-101/native-integration-20261007/index.json`；本单完整子集见 `native-integration-red-20261007.json`。该批次30项27成功3失败，不将其余任务结果算作093。定向 `git diff --check` PASS。

两项失败原因已按真实路径定位：夹具CreateWidget后遗漏 `SetIsFocusable(true)`，直接TakeWidget/AddToViewport再OpenPage(camp)，原ApplyInputMode向非Focusable SObjectWidget设置UIOnly焦点产生LogPlayerController Error。LocalFacilities有2 errors/3 warnings，Upgrade有1 error/2 warnings；全部日志保留。该两项原生条目未出现业务断言失败，但总结果仍为FAIL，不能标PASS。ReceiptTimeline为0 errors/0 warnings。

生产HUD正常创建后调用InitializeScreen，其第69行已设置可聚焦，未发现相应生产初始化缺口。根Agent明确授权仅补夹具创建Screen后、TakeWidget前的SetIsFocusable(true)，已落盘为correctedfixture；没有修改生产或ExpectedError。根Agent随后实际构建并重跑这两项，均Success，合计0 errors/2 Standalone EnhancedInput warnings。原始证据 `.agent-local/qa/TASK-090-100/native-red-20261007/index.json`，本单子集 `native-correctedfixture-20261007.json`；该批次其余3项是090/100的预期RED，不混入093结果。ReceiptTimeline复用上一轮PASS，该夹具初始化行不影响其路径。首轮RED保留。

|原生测试|明确夹具与断言|结果|
|---|---|---|
|ReceiptTimeline|原CampState稳定人物/20→21去重；既有schema事件注入，只计正确仓储类型/量；重复/同epoch过去事件/新epoch基线不回弹。注入事件不称为实际伴侣采集成功|PASS，0 warnings/errors|
|UpgradePreviewAndReplay|真实Standalone Widget/Storage预留/原UpgradeCamp；提前准备S3材料及条件，让旧S1卡重复具有实际二升风险；预留拒绝、一次成本、旧tier/epoch拒绝|首轮FAIL（夹具焦点1 error）；correctedfixture PASS，1 warning/0 errors|
|LocalFacilitiesAndWork|两营地真实Widget visible components/action筛选；原CampState派工/暂停/有限源/兄弟效率状态只读，资源量不变|首轮FAIL（夹具焦点2 errors）；correctedfixture PASS，1 warning/0 errors|

既有 `Hearthward.Camp059.WorkerAssignmentPresentation` 在根Agent31项批次中Success并有Standalone EnhancedInput warning，原始证据 `.agent-local/qa/TASK-085-099/native-first-20261007/index.json`；只证明既有劳工路径，不包括本单新UI。

|任务用例|剩余最窄验证|实际结果|
|---|---|---|
|T093-C01 首救|真实首切片回营20→21一次；原CampState稳定人物去重已验|Native投影PASS；游戏NOT_RUN|
|T093-C02 升阶|Native真实预留/旧卡/原成本；正常工作台I及渲染另验|correctedfixture Native PASS；游戏/渲染NOT_RUN|
|T093-C03 交付|事件归属/量/去重投影已验；实际生产、弟弟入库、玩家制造并行另验|Native投影PASS；并行游戏NOT_RUN|
|T093-C04 岗位|Native真实状态文本；原模型工作/暂停/资源耗尽渲染另验|correctedfixture Native PASS；渲染NOT_RUN|
|T093-C05 两营地|Native本营设施/共享人数；正常往返仓储与建筑状态另验|correctedfixture Native PASS；游戏NOT_RUN|
|T093-C06 休息卸载|原时钟睡眠8小时/暂停/流送及结算快照待运行|NOT_RUN|
|T093-C07 Load|epoch与过去事件投影已验；101实际Snapshot Apply/Load，独立重启另验|Native投影PASS；实际Load/重启另验|

所有093源码窗口已释放给根Agent并进入构建冻结；本代理不启停UE。Windows11/UE5.8.2/RTX4060 Laptop环境由根Agent实际证据绑定。资产/模型未由本单修改，当前未提交，不作无意义hash。公共检查及含任务快照的scope检查待根Agent；旧HEAD未包含任务快照且无提交授权，不伪填基线、不改验证器。

任务Active，无正式review/Owner视觉/真人/二机/性能/发行验收。未提交、未推送、未合并、未发布。
