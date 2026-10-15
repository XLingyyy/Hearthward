# 依据与核对边界

核对日期：2026-10-13。读取基线：`dbf6acd4df31ed1f7154464ac9b635e52e173806`。项目事实来自连接的GitHub；所有新功能、数量和方案为本任务包提案。

## S01

**当前实现范围**

`README.md`

https://github.com/XLingyyy/Hearthward/blob/dbf6acd4df31ed1f7154464ac9b635e52e173806/README.md

已接入 M0—M3、物品图、地图/移动攻击；正式发布包与开发主干区分。

## S02

**有效游戏设计边界**

`docs/design/CURRENT.md`

https://github.com/XLingyyy/Hearthward/blob/dbf6acd4df31ed1f7154464ac9b635e52e173806/docs/design/CURRENT.md

人口/驻军/两营地/兄弟安全任务/死亡存档与明确排除项。

## S03

**任务 JSON 模板**

`docs/templates/task.json`

https://github.com/XLingyyy/Hearthward/blob/dbf6acd4df31ed1f7154464ac9b635e52e173806/docs/templates/task.json

schema_version=1、Backlog、权限与路径字段。

## S04

**最近任务格式**

`docs/tasks/TASK-111.json`

https://github.com/XLingyyy/Hearthward/blob/dbf6acd4df31ed1f7154464ac9b635e52e173806/docs/tasks/TASK-111.json

现有任务111与共用测试ID、分支/来源/范围格式。

## S05

**当前战斗公共接口**

`Source/Hearthward/Combat/HearthwardCombatComponent.h`

https://github.com/XLingyyy/Hearthward/blob/dbf6acd4df31ed1f7154464ac9b635e52e173806/Source/Hearthward/Combat/HearthwardCombatComponent.h

已有轻重击、闪避、格挡、锁定、执行与动作边界。

## S06

**当前战斗参数规则**

`Source/Hearthward/Combat/HearthwardCombatRules.h`

https://github.com/XLingyyy/Hearthward/blob/dbf6acd4df31ed1f7154464ac9b635e52e173806/Source/Hearthward/Combat/HearthwardCombatRules.h

已有前摇/有效段/后摇、消耗、距离和防御参数。

## S07

**上轮补全与未闭合项**

`docs/planning/TASK-109/COMPLETION.md`

https://github.com/XLingyyy/Hearthward/blob/dbf6acd4df31ed1f7154464ac9b635e52e173806/docs/planning/TASK-109/COMPLETION.md

本轮不能重复或误判上轮 UI、模型、LOD、坐姿、声音和测试状态。

## S08

**既有测试编号与边界**

`docs/qa/TEST_MATRIX.md`

https://github.com/XLingyyy/Hearthward/blob/dbf6acd4df31ed1f7154464ac9b635e52e173806/docs/qa/TEST_MATRIX.md

仅引用测试含义，不把初始矩阵中的旧参数取代后续批准规则。

## S09

**既有联合性能口径**

`docs/qa/TASK-102/REPORT.md`

https://github.com/XLingyyy/Hearthward/blob/dbf6acd4df31ed1f7154464ac9b635e52e173806/docs/qa/TASK-102/REPORT.md

固定配置、p99/1%Low/长帧与样本边界，旧成绩不是本轮成绩。

## E01

**Epic：Report Noise Event**

https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/AI/Perception/ReportNoiseEvent

声源事件参考；是否复用原感知由工程设计决定，不强制重写。 官方文档查询日期：2026-10-13；具体API在项目锁定UE版本中编译核验，不预设安装。

## E02

**Epic：Motion Warping Target**

https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/MotionWarping/MakeMotionWarpingTarget

目标位置/旋转对齐参考，不代替碰撞、导航与动画审看。 官方文档查询日期：2026-10-13；具体API在项目锁定UE版本中编译核验，不预设安装。

## E03

**Epic：Saving and Loading Your Game**

https://dev.epicgames.com/documentation/en-us/unreal-engine/saving-and-loading-your-game-in-unreal-engine

保存/恢复职责参考，项目仍沿用现有存档实现。 官方文档查询日期：2026-10-13；具体API在项目锁定UE版本中编译核验，不预设安装。

## E04

**Epic：Automation Test Framework**

https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-test-framework-in-unreal-engine

区分单元/功能/内容与截图测试；不能把工具通过等同实际玩法通过。 官方文档查询日期：2026-10-13；具体API在项目锁定UE版本中编译核验，不预设安装。

## 编号与远端状态

本次读取main引用为dbf6acd4df31ed1f7154464ac9b635e52e173806，并核对任务目录及TASK-1前缀分支分页；最新已有编号111。112—133只在本包安排，尚未写入远端。导入前必须再次核对；本次快照不能保证后续没有并行新任务。

PR #76已于2026-10-12合并，作为本轮既有基础；正式发行版本与主干不等同。https://github.com/XLingyyy/Hearthward/pull/76
