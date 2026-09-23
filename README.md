# Hearthward（归火）

UE 5.8.2 单人第三人称生存冒险项目。当前合并候选基于 `main@be9286f`，结合 TASK-030 自然场景采集/营地模型与 `codex/integrated-latest-20260923` 的 AI、UI、攻击和伙伴跟随修正。自然地图与资产底座中，[TASK-026：大地图自然场景底座](docs/tasks/TASK-026.md) 已形成可运行的 4032 m World Partition 自然地图与营地入口，但完整路线、流送、性能和 Owner 视觉验收仍未全部闭合。设计依据是 GDD v0.3、DSGN-001、[DSGN-002](docs/design/DSGN-002-ui-gameplay.md) 和 [025局部边界](docs/design/DSGN-025-agent-boundaries.md)。

AI NPC vNext 按 **TASK-029 AI NPC 完整交付** 验收。当前合并候选包含自然地图伙伴接入：标题页新游戏自动创建营地伙伴、仓储和有限木材点，支持对话确认、采集入库、跟随/等待/巡营、记忆、建造工作台及 NPC 制作；世界、委托与认知共同存读档。原有自然地图存档会补建缺失的伙伴状态，保留玩家物资。接入实现与本轮证据见[自然营地 AI 接入报告](docs/qa/evidence/TASK-029/natural-camp-integration/REPORT.md)。融合工作树的代码与复验证据见[双工作树集成报告](docs/qa/evidence/TASK-029/integrated-playable/REPORT.md)，已与 TASK-030 的 main 内容形成通过本地自动化复验的合并候选；当前尚待 PR 独立评审和 main 合并。接入前的真实模型矩阵与失败记录保留在[基线复验报告](docs/qa/evidence/TASK-029/revalidation-20260923/REPORT.md)。

[TASK-030 Demo 采集与营地交互](docs/tasks/TASK-030.md) 将自然场景采集、四种工作台配方、床休息、篝火烤肉接成一个小型营地循环，复用角色、木桌、床、箱子和篝火模型。

完整资产接线继续按 [TASK-028](docs/tasks/TASK-028.md) 推进。可见斧头模型与跳跃手臂动作仍未在本轮修复或验收。

## 运行

打开 `Hearthward.uproject`，运行 `/Game/Hearthward/Bootstrap/L_Bootstrap`。标题页“新游戏”进入自然地图的新营地并生成初始存档；营地中按 F6 打开存档页，暂停菜单也可手动保存。返回主菜单后“继续游戏”恢复最新节点；“载入存档”可选择自然地图节点。已有旧开发场景进度仍保留在同一50点档池。

自然地图已接通伙伴、仓储、建造、制作/维修和技能页面。靠近弟弟按 T，输入“帮我采集两份木材带回营地。”，检查任务卡并确认。伙伴原有木材点初始 16 份；玩家也可以靠近场景树木、石头和灌木按 E 采集木材、石材、草药，每次5秒获得2份。每处上限分别为12/8/4份，余量随存档保存，不自动刷新。营地仓储附近按 R 存取物资，按 B 建造工作台、床或篝火。地图和任务日志页面可从自然地图打开；旧开发场景敌人尚未布置到自然地图。

单独浏览自然地图时，在编辑器中打开 `/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds` 并运行PIE；地图级浏览GameMode只复用第三人称角色，不生成旧开发场景的敌人、伙伴或测试地标，也不启动本地模型。

| 操作 | 键位 |
|---|---|
| 移动、视角、冲刺、交互 | WASD、鼠标、Shift、E |
| 跳跃 | 空格；落地后可再次起跳，起跳中断采集或建造 |
| 建造 | B 打开目录；移动/转动视角摆放、Q 每次旋转15°、左键确认、右键取消 |
| 工作台制作 | 靠近已建工作台 E；选择配方、−/+调整批数、F 制作、返回按钮关闭 |
| 背包、技能 | Tab、K；M 地图、J 任务日志目前仅开发场景开放 |
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

菜单默认暂停，对话继续模拟世界，并停止绘制被全屏对话插画遮住的三维场景，关闭对话恢复渲染；输入框 Enter 发送，Esc 返回。技能选节点后 F 学习，可免费洗点。仓储鼠标选择一侧物品，−/+选数量，E或按钮执行真实转移。任务依据实际行为更新，领取奖励后只结算一次。

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
- 旧AI内部 TASK-030（历史现归档于 TASK-029）将战斗收敛成 `companion_order = hold / follow / assist` 高层能力；自然语言路径仍需任务卡确认，具体威胁、leash、导航、LOS、攻击冷却、命中和伤害由 UE 确定性处理。TASK-031 修正了 combat Tick 抢占 typed task 导航的问题：执行 collect/craft/repair 时战斗策略只让出控制权，不再停止 executor 的 MoveTo。
- TASK-032 新增纯 `HearthwardAgentRecovery` 策略模块：action failure 先区分 retry / rewind / return-to-camp / hold；Source 在 Gather 中移动会回退到 `MoveTo(Source)`，短暂 path failure 有界重试，成功 world effect 后恢复预算归零。LLM 不生成恢复步骤，也不能借恢复获得未知目标、坐标或额外权限。
- TASK-033 新增 `HearthwardNPCBelief`：World Truth 与 NPC Belief 在代码层分离。`inventory_report` 只更新弟弟认知，不修改真实仓库；模型上下文读取 provenance-aware `camp_beliefs`，执行器、库存、安全和结算继续只信 UE 权威状态。
- TASK-034 新增 event-driven Initiative：任务完成、自动重规划、受阻、belief 被亲见纠正时可主动提醒。触发、去重、队列、communication range 与 cooldown 全由 UE 确定；不定时轮询 LLM，也不远距离“心灵感应”。主动台词复用现有 HUD。
- TASK-035 新增 grounded Episode Memory：同一 command 的实际取得、交付、制作/维修、重规划、阻塞原因、完成/取消和 evidence IDs 从持久化 Events 即时聚合。过去行动相关回答只引用 episode evidence；Save/load 不保存第二份摘要，读档后重新派生相同 episode。
- TASK-036 扩展战斗协作策略：健康时维持 Assist；玩家低血且单个近身威胁时切 Protect；低血且多威胁时 Regroup；玩家倒地后不再追敌，回到玩家附近。具体目标、导航、LOS、攻击与伤害仍由 UE 权威处理，模型仍只发高层 `assist` 指令。
- TASK-037 增加 Coordination Prior：只从成功执行的 hold/follow/assist 真实指令事件学习近期协作习惯，达到样本与置信阈值后仅影响 contextual suggestion 和模型上下文；不会自动覆盖玩家当前指令。prior 由持久化 events 派生，可随 save/load 恢复，也会随着持续的新行为翻转。
- TASK-038 增加营地自主 Routine：没有 typed task、显式 follow/assist/hold、战斗或倒地时，伙伴按世界时间在营地附近休息、巡营、查看营地或回营，使用真实 UE 导航且不生产物资、不调用 LLM。Z/X/C 会显式关闭 Routine；确认 `companion_order=routine` 可重新授权；typed task 只暂时抢占导航，任务结束后自动恢复。
- TASK-039 完成组件化收口：`CompanionNavigationComponent` 统一导航与 retry，`NPCInitiativeQueue` 统一主动消息状态，`CompanionBehavior` 统一 Routine/Combat/任务导航仲裁，`LocalAIRuntime` 统一 llama.cpp 进程/端口/health/job 生命周期。玩法、存档与权限语义不变，热点类不再直接拥有这些底层实现。
- TASK-040（内部返工编号，对外并入 TASK-029）在 039 深模块边界上补齐上下文预算与认知完整性：`HearthwardNPCContextProjection` 从一次可信 snapshot 生成 full/compact/minimal 三档，并在实际 `/apply-template` + `/tokenize` 后只允许一个通过预算的 request 进入 generation；capability prompt 从 registry 派生，`routine` 不再漂移。Belief 将 semantic `RecordedAt` 与 `LastEvidenceAt` 分离；Episode 增加 `Complete/Truncated/Unknown` coverage，并随 Save schema 3 显式迁移。固定 3328 input-token 上限、256 output、UE 世界权威边界均未放宽；真实 CTX-03 验证可降到 compact，CTX-04 验证 minimal 超限时 generation=0。
- 工作台、床和篝火自由摆放，检查地面支撑、坡度、障碍与角色重叠；工作台和床限营地，篝火可在野外。五实玩秒后扣除背包材料并生成有碰撞的独立建筑；移动、跳跃、受伤、取消或切换菜单终止施工且不耗料。建造中自动保存延后。
- 已建工作台提供即时制作：普通箭矢/绳索/石斧/药膏四种配方、批量选择、实际材料/产出/负重预览。整批扣料并发放成品；材料不足、容量超限、超距、遮挡、持续动作中或旧时间线请求均不结算。制作结果和事件随库存一同回档。
- 工作台维修页列出持有的9类可维修装备，损坏优先，滚轮浏览；显示真实耐久、逐件材料与全修结果。满耐久、缺料、访问失效和过期请求拒绝结算；材料一次扣除、耐久恢复、维修事件在同一保存边界完成。背包H仅跳转页面，不直接收费。
- 同一保存节点恢复已建建筑的位置/旋转，以及库存、仓储、知识、委托、位置、时钟，以及020角色属性、装备耐久、技能、任务、地图和敌人状态。读档废止旧时间线请求。旧019档通过新界面加载时启用新增玩法，保留原库存/知识，初始装备与补给放入共享仓储；新节点再次加载不会重复发放。

纸张、插画、格子、分隔线与按钮已拆成独立组件，15页共62个组件组。`Resources/UI/layout.json` 保存位置、大小、显示状态和单独图层覆盖；编辑时子控件及点击区域跟随父组件。操作与扩展说明见 [布局编辑](Resources/UI/LAYOUT.md)。

界面主题、图集UV、静态布局、物品格与快捷栏位于 `Resources/UI/interface.json`；玩法内容和参数位于 `Resources/Data/gameplay.json`。建造目录、材料配方、营地范围、摆放尺寸及模型外形部件也位于同一玩法配置。当前工作台8木材、篝火4木材4石材、床6木材2绳索，营地半径12米为demo参数；现有场景可按E真实采集木材，也可从营地仓储取出。工作台即时配方位于 `craftingRecipes`：demo每1木材制作4箭矢或1绳索，2木材3石材制作1石斧，2草药制作1药膏，`crafting` 配置260cm访问距离和99批上限。普通配方默认掌握，不预留材料。维修费用位于 `repairRecipes`，使用可采集木材及可制作绳索；当前原型按整件全修收费，未将缺失的装备制作表推导为正式20%费用。耐久仍沿用现有按物品类型保存的模型，不区分同类装备实例；床靠近按E休息5秒，需至少15饱食，完成消耗10饱食、恢复30生命和全部耐力；满状态不消耗。篝火靠近按E，5秒消耗1鲜肉1木材得到1烤肉；新游戏补给含3鲜肉，烤肉可用快捷键2食用。高级蓝图、其他装备配方、熔炼和自动生产未接入。新增物品、技能、任务、建筑、制作配方与地点可按稳定ID扩展，存档以ID关联。中文正文为LXGW WenKai、大标题为Noto Serif CJK SC，OFL许可证随资产提供；生成美术的来源记录在 `Resources/UI/art-provenance.json`。

## 验证与限制

本轮合并候选在 UE 5.8.2 Editor Development 构建通过，Python 31/31、原生 43/43、TASK-030 Demo 70/70、跨页 UI 62/62、自然路线 51/51、疾跑跟随 8/8、攻击夹具 10/10、AI runtime smoke 23/23。原始结果与边界见[主干组合复验](docs/qa/evidence/TASK-029/main-integration-20260924/REPORT.md)。真人实键、可见斧头和跳跃手臂仍待验收。

TASK-030 当前实现通过 UE 5.8.2 Editor Development 构建、原生自动化43/43、工具测试31/31，以及自然地图采集—制作—建床—烹饪—跨地图读档70项检查。已录制并检查家具场景片段；修复箱子高差导致伙伴返营不结算的问题，真实NPC入库复验3/3通过。详情、失败尝试和复现脚本见[Demo验证报告](docs/qa/evidence/TASK-030/demo/REPORT.md)。这次交付为UE内可试玩版本，尚未生成独立Shipping安装包。

TASK-026本次重建记录见[REBUILD](docs/world/TASK-026/REBUILD.md)，历史检查不累计为新图验收。旧地图于2026-09-21完成27项定向PIE检查：地图重开、World Partition外部包、任务路径依赖闭包、浏览隔离、普通移动和一处浅滩通过，并保留5张观察点截图。截图同时显示悬空树冠、倾斜树干、重复形体和地表拼接，因此不构成视觉验收；主环线/两支路/第二浅滩、Standalone流送、目标硬件性能、干净克隆和Owner验收为NOT_RUN。详见[026证据](docs/qa/evidence/TASK-026/README.md)。

TASK-029 自然营地接入基线曾通过 UE 5.8.2 Editor Development 构建、原生自动化 42/42、Python 31/31、自然地图采集与存档 22/22、工作台制作与跨地图读档 23 项、旧自然存档升级 12/12。真实键鼠完成了输入、任务卡确认、两份木材采集入库。完整结果、失败尝试和验证边界见[本轮接入报告](docs/qa/evidence/TASK-029/natural-camp-integration/REPORT.md)。

接入前同一基线的真实 Qwen 32-case matrix：安全边界 32/32、核心 M01～M10 原始分类 20/20、全部原始分类 24/32；其余表达由确定性校验拒绝或澄清。CTX-03/04 及 executor/recovery/initiative/tactical/routine 复验记录见[基线复验](docs/qa/evidence/TASK-029/revalidation-20260923/REPORT.md)。这些结果分别记录，不能视为任意自然语言表达、全地图行为或发布版本的保证。构建/测试入口见 [BUILD_AND_TEST](docs/qa/BUILD_AND_TEST.md)。

当前自然地图已使用树木、植被和岩石资产，demo新增家具模型；伙伴和敌人仍使用碰撞形体，菜单插画不代表三维城寨已制作。主角已接入 Tripo 模型与八种基础动画，待机/行走/冲刺来自 UE 模板重定向；伙伴与敌人仍为灰盒。角色来源与限制见[027交接](docs/handoffs/TASK-027.md)。导航使用动态 Recast 与 Navigation Invokers，按玩家/伙伴附近已加载地形生成，支持障碍绕行；全图可达性、长距离跨流送回营和攀爬/跳跃连接仍未验收。完整十小时剧情、正式动作动画、营地生产/设施升级、重伤救援和Shipping打包尚未完成。TASK-004 自然素材源文件由此前 77 个增至 115 个，其中本批新增冷杉和松树的 38 个源文件。自然场景已接入部分树木模型与地表贴图，004其余模型适配与整单验收仍未完成，见[既有资源汇总](resourceSummary.md)与[新增树木来源](art_source/TASK-004/polyhaven/SOURCE.md)。020经验曲线、节点、任务、建筑、制作配方与战斗参数为独立内容配置，未替代GDD未决R项。

四件 Tripo 道具源资产（石骨斧、原始鱼竿、骨肉袋、陶罐）的 FBX、参考图和预览已存入 `art_source/TASK-004/Tripo/妙妙道具`，但尚未导入 UE Content 或完成游戏内验收，不视为已接入玩法。

十五件 Tripo 动物源资产（两只雄鹿、野兔、山羊、雉鸡、猪、狼、黑熊、公羊、赤狐、母鸡、鲤鱼、鲫鱼、鲶鱼、鳗鱼）的参考图、静态 FBX、预览和带蒙皮权重的骨骼 FBX 已存入 `art_source/TASK-004/Tripo/动物`。骨架包含四足、鸟类、水生和蛇形四类；尚未导入 UE Content，也未附加动画或完成游戏内验收，不视为正式动物系统。

已推送的 [PR #33](https://github.com/XLingyyy/Hearthward/pull/33) 在 `art_source/TASK-004/Tripo/敌人` 收录短刀兵、重甲兵两件候选的参考图、静态 FBX、预览和带双足骨骼的 FBX；[逐件索引](art_source/TASK-004/Tripo/敌人/敌人模型与骨骼索引.md)记录任务 ID 与文件。它们尚未导入 UE，武器、盾牌、护甲权重和动作表现未在引擎中验证；现有玩法敌人仍使用碰撞形体，不视为正式敌人美术。

同一 PR 在 `art_source/TASK-004/Tripo/篝火` 收录一件[静态候选模型](art_source/TASK-004/Tripo/篝火/篝火模型索引.md)的参考图、FBX 与预览。TASK-030已将该石圈、木柴和静态火焰模型接入可建篝火，完成尺寸、碰撞及烹饪验证；动态火焰和局部照明尚未制作。

已通过 [PR #29](https://github.com/XLingyyy/Hearthward/pull/29) 合入 main 的 8 件 [Tripo 房屋建筑部件制作源资产](art_source/TASK-004/Tripo/房屋建筑部件/房屋建筑部件模型索引.md)，每件均有参考图、静态 FBX 和 PNG 预览。模型已完成文件格式核验，尚未导入 UE、统一吸附尺度或完成碰撞及拼接验证；它们是 `TEMP_VISUAL` 候选，不代表正式建筑风格。

资产库已有5件 [Tripo 室内家具制作源资产](art_source/TASK-004/Tripo/室内家具/室内家具模型索引.md)：绳网木床、带锁木箱、木桌、木椅和金属提灯，每件均有参考图、静态 FBX 和 PNG 预览。TASK-030已导入床、箱子和木桌，分别用于休息、营地仓储和工作台；木椅及提灯尚未接入。专用工作台模型暂缺，持握工具挂接、专用采集音效和躺卧动画未实现。这些模型已随 TASK-030 合入 main。

旧界面及此前定向验证可使用 `-HearthwardLegacyUI`。正式流程仍缺Issue归属与独立评审；任务分支成果与main集成状态分别记录。

025记忆由玩家显式维护，每条最多120字、64条活跃记录、4条文字约定；撤销回收容量，单次相关检索最多3条。本人真实事件视图最多128条，与副作用去重回执独立。四类可执行约束为禁采、已知来源、禁用材料和累计耗料上限，长期规则需确认；普通文字约定仅供交流参考。澄清保留原话与限制，容量耗尽明确反馈。真实token预算为3328输入+256输出+512预留。语言理解及词项召回仍可能失败，任意表达可靠性、角色自然度、Shipping和第二机器尚未验收。R18/R20/R21其他内容未闭合。

## 本地推理与工程约定

首次克隆运行 `python scripts/local_ai/prepare_bundle.py`，准备锁定版本llama.cpp和Qwen3.5-4B Q4_K_M。模型/运行库保存在 `Runtime/LocalAI/`，不提交Git；玩家推理不依赖Python或云API。参见 [本地推理包](Runtime/LocalAI/README.md)。

游戏为独立Git仓库，上层GameFactory是工具仓库。引擎生命周期和构建通过GameFactory的UEClient公开API进行。新会话先读 [AGENTS.md](AGENTS.md)、[START_HERE](docs/START_HERE.md) 和当前任务交接。

伙伴导航操作：X 跟随、Z 等待、C 协助有效威胁，T 下达委托；自然地图目前没有布置战斗敌人。自然地图运行时创建覆盖已知地图尺寸的导航边界，通过 invoker 只生成附近 tiles，不改写地图资产。开发场景保留显式 `Hearthward.Companion.CreateTest` 夹具。

025 v2本机GPU复验使用 `-HearthwardAIBackend=vulkan -HearthwardAIGpuLayers=32`；CPU兼容路径与Vulkan耗时分别记录，默认CPU策略未更改。Development Editor 还支持 `-HearthwardAIBundlePath=<已有Runtime/LocalAI>`，便于隔离 worktree 复用本机模型包；Shipping 不接受该覆盖。本机隔离工作树启动还需 `-HearthwardAIBundlePath=G:/GameFactory/Hearthward/Runtime/LocalAI`，或按锁定版本准备此工作树自己的模型包；直接双击尚未准备模型包的工作树工程只能使用手动任务卡。当前手动入口见上方“运行”。TASK-029 及内部 032—040 的当前回归脚本使用显式夹具初始化；更早的历史脚本需按其记录版本运行，不能用其旧“新游戏创建夹具”假设测试当前入口。
