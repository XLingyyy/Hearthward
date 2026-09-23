# Hearthward（归火）

UE 5.8.1 单人第三人称生存冒险项目。TASK-025 增强版 v2 已通过 PR #23 合并 main（`851d60e`）；TASK-026 自然世界灰盒已通过 PR #25 合并 main（`4114556`），形成 4032 m World Partition 地图、独立浏览 GameMode 与局部 PIE 证据，但视觉、长路线、Standalone 流送、性能和 Owner 验收仍未完成。设计依据是 GDD v0.3、DSGN-001、[DSGN-002](docs/design/DSGN-002-ui-gameplay.md)和[025局部边界](docs/design/DSGN-025-agent-boundaries.md)。

当前 AI vNext 集成基线为 `codex/ai-npc-vnext-pr@04239f5`，已包含 TASK-032～039：adaptive recovery、provenance-aware Belief、event-driven Initiative、grounded Episode、Protect/Regroup、Coordination Prior、营地 Routine 与组件化收口。[TASK-040](docs/tasks/TASK-040.md) 在该最新“30 多号任务”基线上继续返工：新增三档 bounded ContextProjection、registry-driven capability prompt、Belief `LastEvidenceAt`、Episode `Complete/Truncated/Unknown` coverage 与 Save schema 3 迁移。当前任务分支实现 UE build PASS、native `Hearthward.*` 40/40 PASS；真实 Schema 2 文件迁移和相关 PIE 复验仍缺，真实 Qwen 当前源码 e2e 因当前工具环境缺少可用 Game WorldContext/项目 UEClient 入口保持 BLOCKED/NOT_RUN，未借用 TASK-039 历史 overflow 作为本轮结果。地图与 AI 两条车道保持隔离；不把 TASK-026 视为已完成视觉验收。见 [026世界状态](docs/world/TASK-026/CURRENT.md)、[TASK-040 交接](docs/handoffs/TASK-040.md) 与各 AI 任务交接。

## 运行

打开 `Hearthward.uproject`，运行 `/Game/Hearthward/Bootstrap/L_Bootstrap`，在标题页选择“新游戏”。020 自动建立现有开发场景所需的伙伴、资源、初始物品和初始存档节点，无需先输入控制台命令。继续游戏与载入存档读取已有的全局50点原型池。

TASK-026自然世界尚未接入标题页。要查看当前分支灰盒，在编辑器中直接打开 `/Game/Hearthward/World/Natural/L_NaturalWorld` 并运行PIE；地图级GameMode只复用第三人称角色，不生成营地、敌人、伙伴或仓储，也不启动本地模型。

| 操作 | 键位 |
|---|---|
| 移动、视角、冲刺、交互 | WASD、鼠标、Shift、E |
| 跳跃 | 空格；落地后可再次起跳，起跳中断采集或建造 |
| 建造 | B 打开目录；移动/转动视角摆放、Q 每次旋转15°、左键确认、右键取消 |
| 工作台制作 | 靠近已建工作台 E；选择配方、−/+调整批数、F 制作、返回按钮关闭 |
| 背包、技能、地图、任务 | Tab、K、M、J |
| 仓储、伙伴交流、存档 | 营地附近 R、弟弟30米内 T、F6 |
| 伙伴记忆 | T 对话 → 记忆与约定；填写陈述/偏好/文字约定，或设置指定物品的采集限制；选择已有记录可修改或撤销 |
| 暂停 | 独立运行 Esc；PIE 使用 P 避免编辑器停止Play快捷键 |
| 近战、弓箭、强力挥击 | 左键、右键、学习技能后 Q |
| 药品、食物、投掷物 | 1、2、4 |
| 伙伴等待、跟随、进攻 | Z、X、C；替换当前委托并保留携带物资 |
| 背包使用、丢弃、工作台维修 | F、R、H；H打开维修页，需靠近工作台，选装备后F确认修复 |
| 地图标记、缩放、平移 | 右键、滚轮、方向键 |
| UI 布局编辑（Development） | F10；拖动组件，右下角缩放，Ctrl+S 保存 |

标题页没有默认选中项；鼠标悬停与键盘导航控制高亮。HUD顶部方位及度数显示已移除。

菜单默认暂停，对话继续运行；输入框 Enter 发送，Esc 返回。技能选节点后 F 学习，可免费洗点。仓储鼠标选择一侧物品，−/+选数量，E或按钮执行真实转移。任务依据实际行为更新，领取奖励后只结算一次。

## 当前能力

- 九页原生UE界面：标题、普通HUD、背包装备、营地仓储、暂停、伙伴对话、地图、技能树、任务日志；存档、设置和危险操作确认均接真实系统。
- 22项物品、10个装备槽、装备计重与耐久、100起始个人容量、无限共享仓储、食物和药品消费、材料修理。
- 生命/饱食/耐力与负重影响，经验与等级，四系29个技能节点及前置、点数、免费重置；近战、主动重击、弓箭和投掷消耗实际物品与体力。
- 8个主线步骤、3个支线目标；4处地点的发现、交互激活、站点间传送、地图标记、任务追踪与一次性奖励。
- 日志六类入口：主线、支线、世界见闻、人物、势力、收集。4条地点见闻、1个人物和2个势力按实际行为解锁；22物品的获得历史进入存档。
- 伙伴采集多趟入库、等待/跟随/协助进攻，统一使用UE原生导航和角色移动。TASK-032 后，资源点在采集中移动、短暂去程/工作台路径失败等可恢复情况会进入有界 `Recovery:Replan`，自动回退到对应 MoveTo 并继续原目标；已经携带真实任务物资时始终优先返营保货。资源耗尽、目标消失、安全失败、材料不足等硬阻塞仍不会凭空生成替代目标，返程不可达时继续安全持有并等待显式处理。取消命令或读档停止旧路径，恢复后重新求路。交流继续通过本地Qwen3.5 4B、规则/词项检索、可知状态过滤与UE结构化校验。
- AI NPC 的营地库存认知已升级为 typed Belief Store：每条记录包含数量、来源、游戏时间、revision 与 campaign。`firsthand` 来自亲眼观察，`player_report` 来自玩家明确报告且保持未核实标记，`receipt` 来自本人真实入库。离营后世界仓库变化不会自动同步 belief；回营后 firsthand 会纠正旧报告。库存询问只引用 belief，并明确来源/是否可能过期。
- “记忆与约定”提供玩家显式管理的长期记录，按相关性检索旧信息；回忆答复引用现存原话，陈述保持玩家来源，不改写世界或角色。文字约定持续进入上下文作为交流参考；指定物品的采集限制由UE在接受任务前强制检查。撤销/编辑清除旧澄清并取消进行中的回复，已有执行委托需单独取消。
- 未完成澄清保留原话、槽位和未解决限制；“帮我采些木材”→“三份”形成任务卡，核对后一次确认执行。关闭对话取消未确认卡片，已接受目标继续；改数量产生新卡片ID，旧确认失效。
- 弟弟可使用自身背包材料到真实工作台制作箭矢/绳索，或维修自己持有的唯一装备；明确授权后可实际到营领取缺料。制作产物真实交付，装备和剩余材料保留。事件来自已提交的扣料、采集、交付和维修，随世界同边界保存。
- 模型不可用时，对话页的手动任务卡、库存查询、取消和进度仍可用；不自动反复启动生成请求。快捷建议只在玩家显式刷新时生成最多3条，未选择内容不进入弟弟的 memory / model input / filtered context；点击后仍走正常 model → candidate → confirm → executor 边界。
- TASK-027 将世界事实和安全判定收敛到 UE authoritative perception；玩家或模型文本不能制造安全地点、库存或隐藏世界事实。
- TASK-028 将 collect/craft/repair 收敛成 deterministic typed plan/actions，运行时使用 plan cursor、真实 inventory/cargo、receipt 和存档重建；取消任务不会凭空删除已携带物资。
- TASK-030 将战斗收敛成 `companion_order = hold / follow / assist` 高层能力；自然语言路径仍需任务卡确认，具体威胁、leash、导航、LOS、攻击冷却、命中和伤害由 UE 确定性处理。TASK-031 修正了 combat Tick 抢占 typed task 导航的问题：执行 collect/craft/repair 时战斗策略只让出控制权，不再停止 executor 的 MoveTo。
- TASK-032 新增纯 `HearthwardAgentRecovery` 策略模块：action failure 先区分 retry / rewind / return-to-camp / hold；Source 在 Gather 中移动会回退到 `MoveTo(Source)`，短暂 path failure 有界重试，成功 world effect 后恢复预算归零。LLM 不生成恢复步骤，也不能借恢复获得未知目标、坐标或额外权限。
- TASK-033 新增 `HearthwardNPCBelief`：World Truth 与 NPC Belief 在代码层分离。`inventory_report` 只更新弟弟认知，不修改真实仓库；模型上下文读取 provenance-aware `camp_beliefs`，执行器、库存、安全和结算继续只信 UE 权威状态。
- TASK-034 新增 event-driven Initiative：任务完成、自动重规划、受阻、belief 被亲见纠正时可主动提醒。触发、去重、队列、communication range 与 cooldown 全由 UE 确定；不定时轮询 LLM，也不远距离“心灵感应”。主动台词复用现有 HUD。
- TASK-035 新增 grounded Episode Memory：同一 command 的实际取得、交付、制作/维修、重规划、阻塞原因、完成/取消和 evidence IDs 从持久化 Events 即时聚合。过去行动相关回答只引用 episode evidence；Save/load 不保存第二份摘要，读档后重新派生相同 episode。
- TASK-036 扩展战斗协作策略：健康时维持 Assist；玩家低血且单个近身威胁时切 Protect；低血且多威胁时 Regroup；玩家倒地后不再追敌，回到玩家附近。具体目标、导航、LOS、攻击与伤害仍由 UE 权威处理，模型仍只发高层 `assist` 指令。
- TASK-037 增加 Coordination Prior：只从成功执行的 hold/follow/assist 真实指令事件学习近期协作习惯，达到样本与置信阈值后仅影响 contextual suggestion 和模型上下文；不会自动覆盖玩家当前指令。prior 由持久化 events 派生，可随 save/load 恢复，也会随着持续的新行为翻转。
- TASK-038 增加营地自主 Routine：没有 typed task、显式 follow/assist/hold、战斗或倒地时，伙伴按世界时间在营地附近休息、巡营、查看营地或回营，使用真实 UE 导航且不生产物资、不调用 LLM。Z/X/C 会显式关闭 Routine；确认 `companion_order=routine` 可重新授权；typed task 只暂时抢占导航，任务结束后自动恢复。
- TASK-039 完成组件化收口：`CompanionNavigationComponent` 统一导航与 retry，`NPCInitiativeQueue` 统一主动消息状态，`CompanionBehavior` 统一 Routine/Combat/任务导航仲裁，`LocalAIRuntime` 统一 llama.cpp 进程/端口/health/job 生命周期。玩法、存档与权限语义不变，热点类不再直接拥有这些底层实现。
- TASK-040 在 039 深模块边界上补齐上下文预算与认知完整性：`HearthwardNPCContextProjection` 从一次可信 snapshot 生成 full/compact/minimal 三档，并在实际 `/apply-template` + `/tokenize` 后只允许一个通过预算的 request 进入 generation；capability prompt 从 registry 派生，`routine` 不再漂移。Belief 将 semantic `RecordedAt` 与 `LastEvidenceAt` 分离；Episode 增加 `Complete/Truncated/Unknown` coverage，并随 Save schema 3 显式迁移。固定 3328 input-token 上限、256 output、UE 世界权威边界均未放宽。
- 工作台和篝火自由摆放，检查地面支撑、坡度、障碍与角色重叠；工作台限营地，篝火可在野外。五实玩秒后扣除背包材料并生成有碰撞的独立建筑；移动、跳跃、受伤、取消或切换菜单终止施工且不耗料。建造中自动保存延后。
- 已建工作台提供即时制作：普通箭矢/绳索配方、批量选择、实际材料/产出/负重预览。整批扣料并发放成品；材料不足、容量超限、超距、遮挡、持续动作中或旧时间线请求均不结算。制作结果和事件随库存一同回档。
- 工作台维修页列出持有的9类可维修装备，损坏优先，滚轮浏览；显示真实耐久、逐件材料与全修结果。满耐久、缺料、访问失效和过期请求拒绝结算；材料一次扣除、耐久恢复、维修事件在同一保存边界完成。背包H仅跳转页面，不直接收费。
- 同一保存节点恢复已建建筑的位置/旋转，以及库存、仓储、知识、委托、位置、时钟，以及020角色属性、装备耐久、技能、任务、地图和敌人状态。读档废止旧时间线请求。旧019档通过新界面加载时启用新增玩法，保留原库存/知识，初始装备与补给放入共享仓储；新节点再次加载不会重复发放。

纸张、插画、格子、分隔线与按钮已拆成独立组件，15页共62个组件组。`Resources/UI/layout.json` 保存位置、大小、显示状态和单独图层覆盖；编辑时子控件及点击区域跟随父组件。操作与扩展说明见 [布局编辑](Resources/UI/LAYOUT.md)。

界面主题、图集UV、静态布局、物品格与快捷栏位于 `Resources/UI/interface.json`；玩法内容和参数位于 `Resources/Data/gameplay.json`。建造目录、材料配方、营地范围、摆放尺寸及灰盒外形部件也位于同一玩法配置。当前工作台8木材、篝火4木材，营地半径12米为 PROTOTYPE_ONLY 参数；现有场景可按E真实采集木材，也可从营地仓储取出。工作台即时配方位于 `craftingRecipes`：原型每1木材制作4箭矢或1绳索，`crafting` 配置260cm访问距离和99批上限。普通配方默认掌握，不预留材料。维修费用位于 `repairRecipes`，使用可采集木材及可制作绳索；当前原型按整件全修收费，未将缺失的装备制作表推导为正式20%费用。耐久仍沿用现有按物品类型保存的模型，不区分同类装备实例；高级蓝图、装备制作、熔炼烹饪、休息和生产暂未接入。新增物品、技能、任务、建筑、制作配方与地点可按稳定ID扩展，存档以ID关联。中文正文为LXGW WenKai、大标题为Noto Serif CJK SC，OFL许可证随资产提供；生成美术的来源记录在 `Resources/UI/art-provenance.json`。

## 验证与限制

TASK-026于2026-09-21完成27项定向PIE检查：地图重开、World Partition外部包、任务路径依赖闭包、浏览隔离、普通移动和一处浅滩通过，并保留5张观察点截图。截图同时显示悬空树冠、倾斜树干、重复形体和地表拼接，因此不构成视觉验收；主环线/两支路/第二浅滩、Standalone流送、目标硬件性能、干净克隆和Owner验收为NOT_RUN。详见[026证据](docs/qa/evidence/TASK-026/README.md)。

025 v2实现与验证见 [TASK-025交接](docs/handoffs/TASK-025.md)、[设计决定](docs/decisions/ADR-TASK-025-npc-cognition.md)和[rev2证据](docs/qa/evidence/TASK-025/rev2/)。TASK-040 当前源码额外通过 repo validator、Python 31/31、UE Editor Development build 与全量 native `Hearthward.*` 40/40；构建机器实际为 UE 5.8.2，而仓库锁定仍为 5.8.1。当前源码真实 Qwen CTX e2e 保持 BLOCKED/NOT_RUN，详见 [TASK-040 验证](docs/qa/evidence/TASK-040/VALIDATION.md)。此前 AI NPC 栈本地/PR前验证包括：repo tests 31/31、UE Editor build PASS、全量原生 `Hearthward` 33/33、TASK-027 Safety PIE 27/27、TASK-028 Executor PIE 49/49、TASK-029 Modern suggestions 40/40 + Legacy 8/8、TASK-030 deterministic combat PIE 29/29、真实 Qwen3.5-4B combat directive PIE 14/14、用户反馈回归 PIE 11/11，以及用户原句“帮我采集两份木材带回营地。”真实 Qwen 链 11/11；确认后实际移动并完成2/2交付。TASK-032 额外完成 `Hearthward.NPCAgent` 9/9 与 adaptive-recovery runtime PIE 11/11：Gather 中移动真实 Source 后观察到 `Recovery:Replan->MoveTo:Source`，最终仍真实完成2/2入库。TASK-033 将 NPCAgent 扩展为 10/10，Belief runtime PIE 25/25：离营时真实仓库变化不会同步认知，回营 firsthand 自动纠正 player report，且 World Truth / stale belief 可独立保存恢复。TASK-034 将 NPCAgent 扩展为 11/11，Initiative PIE 16/16：完成任务可无新 prompt 主动提醒，超出30m时重规划消息排队、重新靠近后才显示，且 `is_busy=false`。TASK-035 将 NPCAgent 扩展为 12/12，Episode PIE 21/21：真实 collect2 + replan 被聚合为 acquired=2/delivered=2/replans=1/completed=true，并保留4个 evidence IDs；save/load 后 projection 与 grounded recall 完全一致。TASK-036 保持 NPCAgent 12/12，Tactical PIE 16/16：Assist/Protect 走真实伤害结算，Regroup 约500→89.5cm，玩家倒地回援约500→142cm。TASK-037 将 NPCAgent 扩展为 13/13，Coordination PIE 28/28：3次 follow 建立 stable prior，一次 hold 不越权改回 follow，持续6次 assist 可将 rolling prior 翻转，读档恢复保存时 prior。TASK-038 将 NPCAgent 扩展为 14/14，Routine PIE 26/26，并重跑 TASK-028 Executor 49/49：Routine 真实移动、显式等待/跟随可关闭、typed task 抢占导航后可恢复、save/load 保留授权。TASK-026 的27项地图定向 PIE 结果和 AI 验证彼此独立记录。构建/测试入口见 [BUILD_AND_TEST](docs/qa/BUILD_AND_TEST.md)。

当前可运行内容仍基于开发灰盒，伙伴和敌人使用碰撞形体；TASK-026也只使用基础形体表达树木、草、岩石和地标，菜单插画不代表三维城寨、正式森林或角色资产已制作。导航覆盖现有开发场景的可行走表面，支持静态障碍绕行和动态障碍重建；正式大世界伙伴导航、攀爬/跳跃导航连接仍未制作。完整十小时剧情、正式动作动画、营地生产/设施升级、重伤救援和Shipping打包尚未完成。TASK-004已有77个自然素材源文件入库，026只复用了其中三张地表贴图，004模型适配与整单验收仍未完成，人物和房屋仍缺，见[资源汇总](resourceSummary.md)。020经验曲线、节点、任务与战斗参数为独立内容配置，未替代GDD未决R项。

旧界面及此前定向验证可使用 `-HearthwardLegacyUI`。正式流程仍缺Issue归属与独立评审；任务分支成果与main集成状态分别记录。

025记忆由玩家显式维护，每条最多120字、64条活跃记录、4条文字约定；撤销回收容量，单次相关检索最多3条。本人真实事件视图最多128条，与副作用去重回执独立。四类可执行约束为禁采、已知来源、禁用材料和累计耗料上限，长期规则需确认；普通文字约定仅供交流参考。澄清保留原话与限制，容量耗尽明确反馈。真实token预算为3328输入+256输出+512预留。语言理解及词项召回仍可能失败，任意表达可靠性、角色自然度、Shipping和第二机器尚未验收。R18/R20/R21其他内容未闭合。

## 本地推理与工程约定

首次克隆运行 `python scripts/local_ai/prepare_bundle.py`，准备锁定版本llama.cpp和Qwen3.5-4B Q4_K_M。模型/运行库保存在 `Runtime/LocalAI/`，不提交Git；玩家推理不依赖Python或云API。参见 [本地推理包](Runtime/LocalAI/README.md)。

游戏为独立Git仓库，上层GameFactory是工具仓库。引擎生命周期和构建通过GameFactory的UEClient公开API进行。新会话先读 [AGENTS.md](AGENTS.md)、[START_HERE](docs/START_HERE.md) 和当前任务交接。

伙伴导航复用现有操作：X 跟随、Z 等待、C 协助进攻，T 下达采集委托。开发场景在创建伙伴时按碰撞几何生成临时导航边界；关卡已有 `NavMeshBoundsVolume` 时使用关卡设置，不改写地图资产。

025 v2本机GPU复验使用 `-HearthwardAIBackend=vulkan -HearthwardAIGpuLayers=32`；CPU兼容路径与Vulkan耗时分别记录，默认CPU策略未更改。Development Editor 还支持 `-HearthwardAIBundlePath=<已有Runtime/LocalAI>`，便于隔离 worktree 复用本机模型包；Shipping 不接受该覆盖。最新 AI NPC 手动验收步骤见 [TASK-030 UE验证指南](docs/qa/evidence/TASK-030/MANUAL_UE_VALIDATION.md)。
