# TASK-090 本轮实现与验证

2026-10-07，根目录 `G:/GameFactory/Hearthward`，实际分支 `codex/TASK-084-103-iteration`。受测状态为完整HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058` 加本轮未提交工作区。root实施生产修复、统一公开UEClient构建/原生执行；mcp_setup补定向急救回归并同步证据。状态Active，正式渲染、正常输入及Owner/真人签收未完成。

## 实际行为与边界

只读PreparationView已接入HUD和日志，按原Campaign条件/阶段给出下一步。日志开场地点使用正式QuestLocation，修复D084-001；满足条件仍要求玩家在原日志手动领奖。没有自动奖励、改主线条件或强制阶段。

材料目标与主线各自保持原ID和暂态状态。玩家或弟弟任一倒地时，HUD抑制普通准备目标、材料行及详情动作，保留实际急救提示和材料tracker；恢复后可继续原目标。未清空材料计划来掩盖布局问题。

main_03等待中的获救者使用实际Campaign.Actor位置；未加载且已Located时使用原Position。waiting不显示旧出发fork。日志显示「等待中的获救者」，其地图定位与HUD读取同一Resolver.World；日志可显式查询所选任务而不改变TrackedQuest。未知支线目标的Gate只作用于side，保留原序章主线规则。只读显示未新增Save字段或改变Campaign事实/奖励。

## 构建与原生证据

实际构建 `.agent-local/qa/TASK-090-100/build-green-repair-20261007/result.json`：`ok=true`。过程中的构建报告保留；原生受测DLL来自该修复构建。

早期AuthoritativeStagesAndReadOnly已有1项Success证据，原导出 `native-first-20261007.json` 保留。后来新增150%字号、实际伤害倒地的两项Widget语义回归，红轮 `.agent-local/qa/TASK-090-100/native-red-20261007/index.json` 两项均Fail，合计6 errors/2 warnings：普通材料行、详情动作与另一入口仍可见。原样子集 `native-urgent-red-20261007.json` 保留；没有吞Error或改验收断言。

最新实际原生 `.agent-local/qa/TASK-090-100/native-green-20261007/index.json` 整批15/15 Success；本单仅取3项。原样子集 `native-green-20261007.json` 保留设备、报告原时间、所有entries及warnings。

|原生测试|实际结果|覆盖与限制|
|---|---|---|
|AuthoritativeStagesAndReadOnly|Success，0 warnings/errors|原阶段/手动领奖/只读无副作用；新增真实Widget日志开场地点、waiting当前目标文案、地图目标随Located Position移动、显式查询不修改TrackedQuest。非真人路线或渲染检查|
|BrotherDownedSuppressesMaterials|Success，1 warning/0 errors|150%字号，原ReceiveDamage使弟弟真正Downed；实际visible components保留急救文本、移除材料行/详情/可见制作入口、tracker保留|
|PlayerDownedSuppressesMaterials|Success，1 warning/0 errors|150%字号，原ReceiveDamage使玩家真正Downed；同一急救优先与tracker断言|

两项warning均为Standalone LocalPlayer缺有效PlayerInput的EnhancedInput设置加载警告，日志原样保留。原生components/动作检查未测量最终Slate像素或真实OS按键。

## 任务用例层级

|用例|已验证子范围|剩余|
|---|---|---|
|T090-C01 新档阶段|Native按原事实读取阶段/地点，以及waiting日志与地图同步PASS|正常游戏按序完成遗物/跟随/撤离/准备、真实截图NOT_RUN|
|T090-C02 旧档恢复|本单无独立兼容旧档实际运行证据|中期/首救兼容档正常加载与不重复引导另验；101证据独立统计|
|T090-C03 两类追踪|088目标跨页及本单急救tracker保留Native PASS，独立分母|同时主线/材料分别取消的正常输入与渲染NOT_RUN|
|T090-C04 待领取目标|Native待领取文案/原日志入口、不自动领奖PASS|实际J领奖及领取后旧绘制标记消失NOT_RUN|
|T090-C05 页面链路|实际Widget HUD→日志入口及不自动领奖Native PASS|正常OS快捷键、焦点/IME/Load全过程NOT_RUN|
|T090-C06 紧急状态|玩家/弟弟倒地150%语义RED→GREEN|最终截图、长原因、投影标记与其他反馈避让NOT_RUN|
|T090-C07 可读性|150%真实Widget语义分层已验|100%/150%、多比例及低对比最终绘制/真人NOT_RUN|

Source写窗口已释放且保持冻结；README/PROJECT_STATE由root统一收尾。未提交、未推送、未合并、未发布。正常输入、Owner视觉、真人、二机、性能和发行未由本报告标为PASS。
