# TASK-093 实施入口与事实映射

受查基线：`codex/TASK-084-103-iteration`，HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`，工作区包含本轮已落盘但尚未提交的 085—092 产物。2026-10-07 开工调查；090、092 构建/定向验证由根代理统一执行。本文件记录修改前事实与拟验证条件，不代表原生、渲染或正常操作通过。

## 事实与提交入口

|显示内容|权威来源|边界|
|---|---|---|
|人口、获救人物|`Camp.State.Population()`、`State.Rescued`；`RecordRescue` 全部提交成功后才更新 State|人口为初始20人加已救稳定人物ID；同一人物不能重复加入|
|空闲/占岗族人|`State.Regions[].Workers` 的真实成员ID集合|人数全局计算一次；兄弟的岗位与普通族人分开|
|弟弟到岗|085 `ReadWorkParty`，以及区域 `BrotherEfficiency`|分配和真实到岗分别显示；移动、个人工作、睡眠不冒充有效劳动|
|营地等阶及效果|`State.Tier`、原 `campEconomy.camp_tiers` 的半径/属性/解锁|两营地共享一次；升级不会自动建造设施|
|本地设施|`State.CampAt(PlayerLocation)` 对应 `State.Facilities[].Camp`|设施列表限定当前营地；建筑等级各自独立|
|升阶条件|原下一阶 `conditions` 与 State 的救援/口粮/区域/故乡/设施状态|逐项显示真值与缺口；原 `UpgradeReason` 和提交保持不变|
|升阶材料|原下一阶 `cost`，共享 Storage 的 `Available(Item)`|预留材料不能使用；只读显示不预扣、不占用|
|升阶提交|原 `Camp.UpgradeCamp(Epoch)`|状态卡附带 epoch 与显示时 tier，执行瞬间核对后交原事务；旧卡不能升下一阶|
|弟弟真实入库|`LocalAI.GetEvents()` 中 `Kind=delivered`、`Reason` 为空的既有事件|Deposit 只有真实 Transfer 成功后才写事件；`Id` 为原操作GUID，`Count` 是实际交付量；其他目的地/救援/生产批次事件不混入|
|岗位模型|原 `CampaignWorld` 的 `PresentLabor(Region,Label,Working)`|现有稳定身份、区域、资源、批次与暂停状态驱动表现；不新增生产计时或搬运模拟|

## 已确认的显示缺口

修改前营地发展页仅显示下一阶材料总需求及 `UpgradeReason` 的第一个失败条件，未列共享仓储实际可用量、全部条件和下一阶解锁；升级按钮只有裸 `camp.upgrade`，未绑定当时等阶。设施管理展示全部营地设施。当前页面和 HUD 尚无以营地权威状态/入库事件组成的本次会话回营摘要。

首切片仍用原批准数据：人口20→21、工作台I、营地S2；S2需工作台I与至少一名获救族人，扣共享木材48、石材24，开放冶炼I/锻造I/治疗区。所有数字从原数据读，不另设经济规则。

## 最小实施及时间线

使用本单允许的新 `HearthwardCampFeedback` 值类型保存会话基线和既有ID；从当前 epoch、营地状态和 NPC 事件读取。首次读取或 epoch 变化时播种全部已有获救人物/等阶/事件ID并清空本次摘要，随后仅记录真实新状态转移和新入库回执。无本次会话来源时明确显示当前营地状态；不根据库存净变化归属交付，不发奖励，不新增 Save 字段。

共享 UI 写入窗口由根代理调度。拟写本单允许的 ScreenCamp、ScreenContent、ScreenWidget.h、新反馈 helper 与定向测试；若没有其它缺口，CampSubsystem、WorldPresentation、共享 UI JSON 保持原实现。营地通知复用现有 HUD 两秒反馈生命周期，有限摘要在营地页查看。

工程参考：[Epic 的 UI 事件更新说明](https://dev.epicgames.com/documentation/unreal-engine/driving-ui-updates-with-events-in-unreal-engine?lang=en-US)。沿既有0.2秒 UI 刷新读取有限事件/状态变化；本单不另建跨游戏系统更新服务。

## 定向验证入口

拟新增原生前缀 `Hearthward.Iteration.Task093.`，验证真实人口/升阶状态、旧卡不重复扣料、真实回执归属、同一事件不重放、epoch变化只建立当前状态基线、本营设施与真实劳工状态。生产经济已有 `Hearthward.Camp059.WorkerAssignmentPresentation` 覆盖稳定族人身份/派工/弟弟实际到岗；不复写劳工算法。

本单 C01—C07 的完整游戏、渲染、正常输入与读档切片仍需根代理实际运行和分层记录；原生夹具不替代这些项目。
