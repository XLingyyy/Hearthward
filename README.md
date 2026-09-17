# Hearthward（归火）

基于 UE 5.8.1 的单人第三人称生存冒险游戏，设计基线为 GDD v0.3。当前仓库已包含可构建的 C++ 工程和可运行的灰盒地图，开发进度到 TASK-011。

当前分支包含已提交推送的任务成果；这些功能尚未合并到 `main`，也未完成完整游戏或打包验收。
仓库：[XLingyyy/Hearthward](https://github.com/XLingyyy/Hearthward)。游戏工程根目录为本仓库，上层 GameFactory 为独立工具仓库。

## 当前实现

- 第三人称灰盒角色、WASD行走与鼠标视角。
- 世界时钟、五秒持续动作、暂停冻结、移动/受伤中断及实时HUD。
- 五种普通物品的个人背包、容量100、负重减速，以及Tab背包查看与默认暂停。
- 单世界共享仓储、整批真实存取、重复请求保护与旧时间线请求拒绝。
- 距离交互、目标失效/移远取消、完成条件复核及完成回调；E为灰盒临时交互键。

共享仓储尚未接入正式营地解锁和操作界面；交互测试点的200cm距离、2木材消费仅为隔离测试参数，未形成正式配方或建造玩法。
TASK-004基础资产准备仍只有任务单、未执行；正式美术、完整生存/战斗/伙伴系统及存档尚未完成。

## 运行与验证

工程入口：`Hearthward.uproject`；灰盒地图：`/Game/Hearthward/Bootstrap/L_Bootstrap`。
本机工具链为 UE 5.8.1、MSVC14.44.35228、Windows SDK10.0.22621.0。
通过GameFactory的 `UEClient` 公开API构建和启动，具体命令见 [BUILD_AND_TEST](docs/qa/BUILD_AND_TEST.md)。

进入PIE并单击视口后，WASD行走、鼠标转向、Tab查看/关闭背包。
测试交互可在控制台执行 `Hearthward.Interaction.CreateTestTarget`，站定后按E开始五秒读条；该开发命令不生成建筑或材料。

最近功能验证：TASK-011 Editor构建、2项原生动作测试、两轮PIE共46项检查及2项HUD定向检查通过，证据绑定实现提交 `00443867e430f8e4647aa589c0638240cc75aa4c`。
已知引擎启动期13条Condition failed诊断仍有记录；打包、两机验证和完整M0验收未运行。
详细结果、测试边界和截图见 [TASK-011交接](docs/handoffs/TASK-011.md)，其他模块证据见 [PROJECT_STATE](docs/PROJECT_STATE.md)。
Issue、独立评审和部分正式审批尚未落实，任务流程状态不等同于本地实现完成。

## 开发与文档维护

从 [START_HERE](docs/START_HERE.md) 开始；开发Agent先读 [AGENTS](AGENTS.md)，完整规则见 [WORKFLOW](WORKFLOW.md)。
**每次任务完成，必须在提交前同步更新本README，替换或删除过时描述，并随任务成果提交。** README保留当前有效说明；历史进展查任务交接和Git记录。

| 内容 | 入口 |
|---|---|
| 当前分支实现、集成状态与证据 | [PROJECT_STATE](docs/PROJECT_STATE.md) |
| 环境与构建操作 | [ENVIRONMENT](docs/ENVIRONMENT.md) / [BUILD_AND_TEST](docs/qa/BUILD_AND_TEST.md) |
| 设计基线与未定项 | [CURRENT](docs/design/CURRENT.md) / [OPEN_QUESTIONS](docs/design/OPEN_QUESTIONS.md) |
| 开发者上手与职责 | [CONTRIBUTING](CONTRIBUTING.md) / [OWNERSHIP](docs/OWNERSHIP.md) |
| 任务规划与验收矩阵 | [BACKLOG](docs/planning/BACKLOG.md) / [TEST_MATRIX](docs/qa/TEST_MATRIX.md) |

本仓库文档不向游戏素材、第三方模型或商业插件授予新许可；资产引入需保留来源及许可记录。
