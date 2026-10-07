# Actual empty swing — 2026-10-07

用户本轮逐任务实现授权下，root批准了空挥音效的最小技术补充。源码范围为 `Source/Hearthward/Combat/HearthwardCombatComponent.cpp` 的既有 `Sweep` 入口，以及 `Source/Hearthward/Tests/CombatAudioFeedbackTests.cpp` 内一条 `Hearthward.Iteration.Task099.Combat.ActualEmptySwing`；Presentation消费与候选音源由独立写者负责。没有新公共API、保存字段、动画Notify或攻击规则。

## 真实 RED

[原始 index](swing-red-20261007-12/index.json)、[Editor build](swing-red-20261007-12/build.json)、[公开测试结果](swing-red-20261007-12/result.json)为完整原文件副本。私有源目录为 `.agent-local/qa/TASK-099/swing-red-20261007-12/`。该轮仅选中上述一条用例：Editor build SUCCESS，Native 1 Fail、0 Warning、5 Error；`reportCreatedOn=2026.10.07-00.23.04`。

五个错误均为已经接受并实际进入攻击窗口的动作没有发布 swing 回执：首次活动窗口0/1、同活动窗口后续Tick0/1、恢复阶段0/1、一次Tick跨整个窗口0/2、排除旧动作后新的合法攻击0/1。真实LocalPlayer、地面物理、公开库存与装备、攻击接受、活动时间、暂停、取消、epoch与实例变化前置没有选中用例错误。未用启动期Smoke作为此用例失败。

更早 `swing-red-20261007-10/build.json` 的C2512/指针类型推导错误与 `swing-red-20261007-11/build.json` 的AudioExtensions链接错误保留在私有原目录，均未运行Native；修复后第12轮完成编译和上述真实RED。这些结果优先于此前静态检查。

## 最小生产闭合

`Sweep` 保留已有 `B>A` 活动区间检查与石斧真实mesh、socket、动画clip、骨骼验证。在命中采样前，只有 `From<=Move.Windup && To>Move.Windup` 的活动入口发布现有 `OnCombatSucceeded`：新 `SuccessId`、当前 `ActionId`、`ActionEpoch`、`Kind=swing`、owner实时位置。每次新动作有自己的ID；同一活动窗口后续Tick不再满足该入口条件。回执不会把空挥计作命中，现有取消、旧epoch、武器实例变更检查继续在入口前拒绝。

该生产补丁之后，在同一注册用例追加真实producer→consumer检查：BeginPlay前注册Presentation，实际公开Attack推进后检查新注册AudioComponent、真实PCM波形、与实际knifeSlice候选文件一致的声道和时长、玩家声源位置；重复活动Tick/恢复/拒绝/暂停/取消/旧epoch/实例更换不增加声源，新epoch合法动作恢复播放。没有直接构造回执或调用私有播放方法。

新增音效断言在第12轮RED之后落盘，**没有独立的补丁前音效RED**。当前本条分母仍为1；生产和扩展断言已静态冻结、`git diff --check`通过，等待root统一Native。不能将第12轮的五个回执错误扩大描述为已实测音效失败，也不能借用此前19项通过说明本次新增路径通过。

当前候选资源映射为17个event ID / 12个独立运行WAV（原仓储1个+新增11个，新增原源为10个Kenney音源与1个Vistula音源）。`combat.swing`使用真实 `TASK-099/candidate-knifeSlice.wav`；水声独立记录见 [water/README](../water/README.md)。这些均为内部候选；设备实听、正常Hero动画路线、混音与Owner试听仍NOT_RUN。

## 最新实际GREEN13

Root第13轮Development Editor buildSUCCESS；本单23/23、联合28/28选中Native Success，0用例warnings/errors。此ActualEmptySwing同一case真实publicAttack→producer→Presentation→AudioComponent/PCM格式/时长/当前位置、重复/恢复/取消/暂停/旧epoch/换装/合法新epoch声源检查均通过。原RED12五个缺receipt错误保留；追加的audio断言没有独立补丁前RED，不能倒写历史。精确结果见 [13 raw index](../native-final-audio-20261007-13/index.json)、[23原对象子集](../native-final-audio-20261007-13/task099-subset.json)。

本轮-NoSound/-NullRHI，证明组件与算法，没有设备听声/正常Hero clip/Owner试听信用。原完整stdout仍留private run，13frame0 Smoke error与1MCP启动warning在结构化结果独立保留。当前17event/12WAV、Source/Resources冻结。
