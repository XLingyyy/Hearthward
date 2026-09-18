# Hearthward（归火）

基于 UE 5.8.1 的单人第三人称生存冒险游戏，设计基线为 GDD v0.3 与已批准的 DSGN-001 本地推理修订。当前工作分支已接通 TASK-013：UE → llama.cpp → GGUF → Qwen3.5 4B，模型在玩家电脑运行。

当前分支包含 TASK-012 伙伴委托与 TASK-013 本地模型接入成果；尚未合并到 `main`，完整游戏和打包验收未完成。
仓库：[XLingyyy/Hearthward](https://github.com/XLingyyy/Hearthward)。游戏工程根目录为本仓库，上层 GameFactory 为独立工具仓库。

## 当前实现

- 第三人称灰盒角色、WASD行走与鼠标视角。
- 世界时钟、五秒持续动作、暂停冻结、移动/受伤中断及实时HUD。
- 五种普通物品的个人背包、容量100、负重减速，以及Tab背包查看与默认暂停。
- 单世界共享仓储、整批真实存取、重复请求保护与旧时间线请求拒绝。
- 距离交互、目标失效/移远取消、完成条件复核及完成回调；E为灰盒临时交互键。
- 伙伴结构化安全采集委托原型：30米交流边界、三步计划、多趟实际入库、缺料/受阻返营、取消保留物资，以及旧回复和旧时间线拒绝。
- 本地自然语言入口、规则意图候选、初始知识检索与可知状态过滤、单次4B推理、JSON契约校验及NPC文字反馈。模型结果通过原有世界执行器后才产生动作。

共享仓储尚未接入正式营地解锁和操作界面；交互测试点的200cm距离、2木材消费仅为隔离测试参数，未形成正式配方或建造玩法。
伙伴目前仅通过独立开发夹具运行：有限16木材、每趟重量4、五秒采集、直线碰撞通路均为测试参数；正式弟弟容量、全地图寻路和危险识别未实现。
当前检索使用规则和词项评分，未安装独立Embedding模型；角色长期记忆、全局快捷建议与正式对话输入界面尚未实现。自然语言接在同一伙伴开发夹具上，完整NPC与生产地图仍待后续任务。
TASK-004基础资产准备仍只有任务单、未执行；正式美术、完整生存/战斗系统及存档尚未完成。

## 运行与验证

工程入口：`Hearthward.uproject`；灰盒地图：`/Game/Hearthward/Bootstrap/L_Bootstrap`。
本机工具链为 UE 5.8.1、MSVC14.44.35228、Windows SDK10.0.22621.0。
通过GameFactory的 `UEClient` 公开API构建和启动，具体命令见 [BUILD_AND_TEST](docs/qa/BUILD_AND_TEST.md)。

进入PIE并单击视口后，WASD行走、鼠标转向、Tab查看/关闭背包。
测试交互可在控制台执行 `Hearthward.Interaction.CreateTestTarget`，站定后按E开始五秒读条；该开发命令不生成建筑或材料。

首次克隆后执行 `python scripts/local_ai/prepare_bundle.py`，准备固定版本运行库与约2.74GB的Q4_K_M权重。开发准备需要联网；游戏推理不依赖Python、云端API或玩家另装模型管理器。权重和二进制实际放在项目 `Runtime/LocalAI/`，不提交到Git；发行资源由RuntimeDependencies按NonUFS收集。版本、散列、来源与许可证见 [本地推理包](Runtime/LocalAI/README.md) 和 [锁定记录](config/local-ai.lock.json)。

伙伴自然语言测试依次执行 `Hearthward.Companion.CreateTest`、`Hearthward.AI.Say 帮我收集十份木材，分几趟运回营地仓库。`；左上显示模型等待/回复，右上显示实际入库进度。
`Hearthward.AI.CancelReply` 取消本次推理回复，已接受的采集继续。直接结构化开发入口 `Hearthward.Companion.Collect 10` 仍可用于执行器排查。
`Hearthward.Companion.Cancel` 在30米内取消，保留已采物资；Tab可暂停全过程。开发命令不在Shipping注册，也不修改地图。

TASK-013的Development Editor与Game构建通过，68项本地AI运行依赖已核对齐全；原生LocalAI/伙伴共4项、仓库工具31项通过。真实模型在UE中接受10木材委托并实际完成采集入库。最终Vulkan测试覆盖10类输入、两轮PIE共43项检查，CPU缺文件/进程退出恢复共9项通过。RTX4060 Laptop 8GB、32层GPU卸载，本轮首次请求含加载8.93秒，后续约2.0–3.0秒；CPU两次含加载响应约28–32秒。完整结果见 [TASK-013交接](docs/handoffs/TASK-013.md)。
模型台词仍会出现多余追问和“已经在营地却说等回营”的措辞问题；结构化动作通过不等同于对话质量或认知系统全面验收。真实库存由UE保持，玩家虚报数量不会生成物品。
已知引擎启动期13条Condition failed诊断仍有记录；打包、两机验证和完整M0验收未运行。
CPU为兼容默认，可在 `Config/DefaultGame.ini` 切换Vulkan与GPU层数；本机性能不代表所有玩家硬件。其他模块证据见 [PROJECT_STATE](docs/PROJECT_STATE.md)。
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
