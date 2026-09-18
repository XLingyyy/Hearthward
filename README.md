# Hearthward（归火）

基于 UE 5.8.1 的单人第三人称生存冒险游戏，设计基线为 GDD v0.3 与已批准的 DSGN-001 本地推理修订。当前工作分支已接通本地模型 UE → llama.cpp → GGUF → Qwen3.5 4B，并具备独立灰盒验证场、原生伙伴交流面板、TASK-016 开发存档闭环及 TASK-017 存档管理界面。

当前分支包含 TASK-012 伙伴委托、TASK-013 本地模型接入，TASK-014 独立灰盒验证场景、TASK-015 背包/伙伴UI、TASK-016 世界/知识快照及 TASK-017 存档管理/退出确认。017实现与验证已完成，按用户后续明确授权提交推送任务分支；此前成果已推送，均未合并到 `main`，完整游戏和打包验收未完成。
仓库：[XLingyyy/Hearthward](https://github.com/XLingyyy/Hearthward)。游戏工程根目录为本仓库，上层 GameFactory 为独立工具仓库。

## 当前实现

- 第三人称灰盒角色、WASD行走与鼠标视角。
- 世界时钟、五秒持续动作、暂停冻结、移动/受伤中断及实时HUD。
- 五种普通物品的个人背包、容量100、负重减速，以及Tab背包查看与默认暂停。
- 单世界共享仓储、整批真实存取、重复请求保护与旧时间线请求拒绝。
- 距离交互、目标失效/移远取消、完成条件复核及完成回调；E为灰盒临时交互键。
- 伙伴结构化安全采集委托原型：30米交流边界、三步计划、多趟实际入库、缺料/受阻返营、取消保留物资，以及旧回复和旧时间线拒绝。
- 本地自然语言入口、规则意图候选、初始知识检索与可知状态过滤、单次4B推理、JSON契约校验及NPC文字反馈。模型结果通过原有世界执行器后才产生动作。
- 原生伙伴交流面板：30米内按临时T键打开，Enter发送，Esc关闭；显示实际负重、目标/三步、入库进度、思考、回复、受阻与取消状态。
- 开发存档：同一节点保存角色/背包/仓储、伙伴任务/携带物资/有限资源、位置/时间/计时和交流记录；回档生成新epoch，废止旧推理与转移请求。全原型进度共用50点，保护手动和锁定档，支持自动保存、安全延后和完整性校验。
- 存档管理面板：F6打开，展示全局50点和进度/时间/地点/阶段/类型/锁定；可手动保存、回档、新进度、锁定/解锁、删除及设置1—60实玩分钟自动间隔。回档、新进度、删除、退出须确认；退出不额外保存。菜单暂停，与背包/交流互斥，关闭恢复输入并保留原有外部暂停。
- 独立灰盒验证场：60m×40m测试地面、距离刻度直道、碰撞墙、坡道平台、门洞及地标；新增1个基础网格、4个材质和1张地图，项目资产引用封闭在本单目录内。

共享仓储尚未接入正式营地解锁和操作界面；交互测试点的200cm距离、2木材消费仅为隔离测试参数，未形成正式配方或建造玩法。
伙伴目前仅通过独立开发夹具运行：有限16木材、每趟重量4、五秒采集、直线碰撞通路均为测试参数；正式弟弟容量、全地图寻路和危险识别未实现。
当前检索使用规则和词项评分，未安装独立Embedding模型；原始交流记录已随档恢复，近期最多8条（合计512字符预算）以不可信历史进入模型上下文；完整可编辑知识清单、长期约定索引与全局快捷建议生成尚未实现；建议区域明确显示不可用。原生自由输入、思考/台词及实际委托状态面板已接入同一伙伴开发夹具，完整NPC与生产地图仍待后续任务。
TASK-004基础资产准备仍只有任务单、未执行；正式美术、完整生存/战斗系统及覆盖全部GDD模块的存档尚未完成。当前存档仅用于显式启用的伙伴灰盒夹具，危险标志来自开发场景，R22未定规则保持开放。

## 运行与验证

工程入口：`Hearthward.uproject`；灰盒地图：`/Game/Hearthward/Bootstrap/L_Bootstrap`。
014独立测试地图：`/Game/Hearthward/Tests/Graybox/L_GrayboxValidation`。在Content Browser打开该地图后Play，或向UEClient的 `launch_editor` 传入此路径。默认启动地图保持原入口。
测试场仅用于尺度、移动和碰撞验证；尺寸不定义R24的正式攀越/地图参数，不自动生成伙伴、物资或加载模型。
本机工具链为 UE 5.8.1、MSVC14.44.35228、Windows SDK10.0.22621.0。
通过GameFactory的 `UEClient` 公开API构建和启动，具体命令见 [BUILD_AND_TEST](docs/qa/BUILD_AND_TEST.md)。

进入PIE并单击视口后，WASD行走、鼠标转向、Tab查看/关闭背包。
测试交互可在控制台执行 `Hearthward.Interaction.CreateTestTarget`，站定后按E开始五秒读条；该开发命令不生成建筑或材料。

首次克隆后执行 `python scripts/local_ai/prepare_bundle.py`，准备固定版本运行库与约2.74GB的Q4_K_M权重。开发准备需要联网；游戏推理不依赖Python、云端API或玩家另装模型管理器。权重和二进制实际放在项目 `Runtime/LocalAI/`，不提交到Git；发行资源由RuntimeDependencies按NonUFS收集。版本、散列、来源与许可证见 [本地推理包](Runtime/LocalAI/README.md) 和 [锁定记录](config/local-ai.lock.json)。

伙伴UI测试先在开发控制台执行 `Hearthward.Companion.CreateTest`，靠近30米内按T打开交流面板，输入后按Enter或点击发送。面板读取实际委托状态，世界继续运行；Esc关闭并恢复移动和视角。临时T键属于开发原型，正式键位仍待R23。
“取消本次回复”只取消推理；“取消当前委托”停止执行并保留实际物资。关闭面板沿用013开发行为，不自动撤销已发送请求；R20正式关闭/迟到体验仍未确定。Tab背包与交流面板互斥。
控制台自然语言入口 `Hearthward.AI.Say 帮我收集十份木材，分几趟运回营地仓库。` 继续保留。
`Hearthward.AI.CancelReply` 取消本次推理回复，已接受的采集继续。直接结构化开发入口 `Hearthward.Companion.Collect 10` 仍可用于执行器排查。
`Hearthward.Companion.Cancel` 在30米内取消，保留已采物资；Tab可暂停全过程。开发命令不在Shipping注册，也不修改地图。

存档界面使用：先执行 `Hearthward.Companion.CreateTest` 创建独立伙伴夹具，按 **F6** 打开菜单，首次点击“启用存档”，然后选择“新进度”或读取已有节点。Esc/F6返回（确认框中先取消），Tab切换背包。列表和详情可滚动；退出按钮先警告未保存进度丢失，确认后退出游戏。菜单仍为非Shipping开发入口，危险来源为显式夹具。

控制台存档入口仍保留：先创建伙伴夹具，依次执行 `Hearthward.Save enable`、`Hearthward.Save new`；`Hearthward.Save manual` 新建手动节点，`Hearthward.Save list` 在日志列出节点GUID。使用 `Hearthward.Save load <GUID>` 回档，`lock/unlock/delete <GUID>` 明确管理节点。默认池在 `Saved/SaveGames/HearthwardPrototype/pool.hws`；不额外建立退出档。重新进入PIE后先创建同一夹具并enable，再加载原节点；不会自动恢复或切图。初始节点采用enable时的夹具状态，正式出生位置仍未定。

TASK-013的Development Editor与Game构建通过，68项本地AI运行依赖已核对齐全；原生LocalAI/伙伴共4项、仓库工具31项通过。真实模型在UE中接受10木材委托并实际完成采集入库。最终Vulkan测试覆盖10类输入、两轮PIE共43项检查，CPU缺文件/进程退出恢复共9项通过。RTX4060 Laptop 8GB、32层GPU卸载，本轮首次请求含加载8.93秒，后续约2.0–3.0秒；CPU两次含加载响应约28–32秒。实现提交 4a75c7514474b7d901028a929482aa859c404d9b；测试覆盖与绑定见 [TASK-013交接](docs/handoffs/TASK-013.md)。
模型台词仍会出现多余追问和“已经在营地却说等回营”的措辞问题；结构化动作通过不等同于对话质量或认知系统全面验收。真实库存由UE保持，玩家虚报数量不会生成物品。
已知引擎启动期13条Condition failed诊断仍有记录；打包、两机验证和完整M0验收未运行。
TASK-014两轮PIE共28项检查通过：出生、直道移动、墙体阻挡、坡道上台、门洞通行、外边界、背包暂停/恢复及引用闭包。GitHub独立干净克隆取回6个LFS资产后，Editor源码构建与相同28项检查再次通过；该测试无需准备模型权重。物理键盘录像与克隆证据见 [TASK-014交接](docs/handoffs/TASK-014.md)；另一真人和双账号锁演练未运行。
TASK-015的Editor构建、两轮PIE31项状态/绑定检查通过；真实模型由UI提交两份木材委托并实际入库。中文输入、Enter/鼠标发送、Esc关闭和行走恢复已实测，最终输入框颜色修正已在独立窗口检查；实现提交 e0a83c54ed0d1ccea946c626da063de1c89c391d，覆盖绑定见 [TASK-015交接](docs/handoffs/TASK-015.md)。
TASK-016的Editor最终构建与原生2项通过，两轮PIE54项通过，仓库工具31项通过；覆盖采集中回档、物资/知识一致恢复、真实HTTP废止、全保护档池、危险延后、损坏拒绝和跨PIE磁盘恢复。最后的历史上下文预算修改由最终构建/原生测试覆盖。实现提交 `17f6008b55f102936aa6c6056b0bab5326088401`，验证与源码绑定见 [TASK-016交接](docs/handoffs/TASK-016.md)。当前格式为开发schema 1，保存小型快照时同步写盘；完整世界后台保存、正式开局/主菜单、自动切图与实体重建尚未实现。
TASK-017最终Editor构建通过，两轮PIE共48项UI/磁盘集成检查、仓库自检及31项工具测试通过；存档原生2项通过（后续仅调整UI显示，存档代码未变）。已实测鼠标保存、选择/锁定、回档确认/取消、F6开关、Tab背包切换、关闭后行走及独立游戏退出且不写盘，保留128秒实键操作录像。任务分支 `codex/TASK-017-save-menu`，证据和受测源码绑定见 [TASK-017交接](docs/handoffs/TASK-017.md)。
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
