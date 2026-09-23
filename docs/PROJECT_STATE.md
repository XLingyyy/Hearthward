# Hearthward 项目状态

2026-09-23局部更新：远端 main 为 `be9286f`（TASK-030）；`codex/integrated-latest-20260923` 已将该 main 合入本地候选，同时保留此前 TASK-029/041/042 选定代码。组合源码的构建、Python 31/31、原生 43/43、Demo 70/70、UI 62/62、自然路线 51/51、跟随 8/8、攻击 10/10 和 runtime smoke 23/23 已通过；见[组合复验](qa/evidence/TASK-029/main-integration-20260924/REPORT.md)。独立 PR 评审与 main 合并尚未完成。TASK-026 自然地图继续按用户新方向精修新营地，主菜单新游戏、存档和继续游戏已接入；全图质量、长路线、跨区流送、性能与 Owner 视觉验收仍未完成，见[营地接入记录](world/TASK-026/CAMP_INTEGRATION.md)。

AI NPC vNext 最终统一按 **TASK-029 AI NPC 完整交付** 对外验收。集成分支已把 `main@ba547c0` 的角色动画、latest-main AI 核心返工、PR #34 `25d53d8` 的自然营地 AI 与 TASK-041/042 的 UI/交互/攻击/跟随修正收口为同一树。latest-main AI 核心已验证 Python 31/31、Editor build PASS、native 41/41、runtime smoke 23/23、Executor 49/49、Initiative 16/16、Tactical 16/16、Routine 26/26；自然营地线此前验证 native 42/42、自然采集/存档 22/22、工作台闭环 23 项、旧自然档升级 12/12。旧 `main@ba547c0` 的 validator 有 9 项既有 workflow metadata 错误；本轮集成树只剩 canonical TASK-027 的 reviewer 与 Issue URL 两项。两次检查对应不同提交，均不冒充 0 errors。详见 [latest-main finalization](qa/evidence/TASK-029/LATEST_MAIN_FINALIZATION.md)。

历史基线：TASK-020 于2026-09-19通过用户验收；021伙伴导航、022自由建造、023即时制作、024维修已进入后续主干。原025经PR #22合并后被用户否决验收；增强版v2随后通过PR #23合并main。TASK-004已有77个自然素材源文件入库，UE适配未完成。

| 项目 | 实际状态 |
|---|---|
| 开发根目录 | 独立 Hearthward 仓库，与上层 GameFactory 工具仓库隔离 |
| 工作流 | v1.0 已导入；根 WORKFLOW.md 为维护入口，团队采用仍为 DRAFT |
| GitHub | origin 为 XLingyyy/Hearthward，公开仓库；TASK-026 地图工作与 TASK-029 AI NPC 工作保持分支隔离 |
| 当前分支 | 本轮融合：`codex/integrated-latest-20260923`；对外统一 TASK-029。地图：`codex/TASK-026-natural-world-rebuild` 另行推进 |
| 游戏实现提交 | 远端 main 当前为 `be9286f`；本轮分支原基于 `ba547c0`，现已在 `1f8e4dd2c9c2e7b921c4aeb36e49ff5419f5ff6a` 合入 `main@be9286f` 并完成组合复验；旧 PR #34 不代表本轮分支；等待 review/Owner 验收，不自动 merge |
| Git LFS | 已启用；本次 AI NPC PR 不主动修改地图/资产 LFS 内容，main 资产更新仅作为同步基线继承 |
| 工具链 | UE 5.8.2、MSVC 19.44.35228.0、SDK 10.0.22621.0 |
| 工程 | 根 Hearthward.uproject；Source、Config、灰盒 Content 和本地框架插件源代码已提交 |
| 构建／操作 | TASK-013 Editor/Game构建通过，68项AI运行依赖齐全；真实模型Vulkan两轮PIE43项、CPU缺文件/进程退出恢复9项通过；伙伴通路仍为隔离夹具 |
| 仓库检查 | TASK-016基线范围与当前授权范围分别记录，结果见本单workflow-validation；不将扩展任务JSON冒充基线已审批 |
| 设计 | 当前v0.3原件按DSGN-001同步本机4B与轻量检索推理链；历史归档不变，R01—R25保持OPEN |
| TASK-005 | 正常运行世界时钟；构建通过，2项原生测试与两轮PIE共16项检查通过；见TASK-005交接 |
| TASK-006 | 五秒持续动作；构建、2项原生测试、两轮PIE共32项检查通过；未接领域结算 |
| TASK-007 | 实时动作HUD；构建通过，动作2项原生／32项PIE回归通过；10项HUD检查、两种尺寸12张截图已检查 |
| TASK-008 | 个人普通物品背包、容量100及负重减速；构建、2项原生测试、44项PIE检查通过 |
| TASK-009 | 背包查看面板与默认暂停；构建、2项原生测试、两轮PIE36项检查通过，4张截图已检查 |
| TASK-010 | 单世界共享仓储、整批真实转移、重试及epoch保护；构建、4项原生测试、42项PIE检查通过；正式营地交互未接入；存档由016接入现有夹具 |
| TASK-011 | 距离交互与目标生命周期，复用五秒计时/暂停/中断；构建、2项原生动作测试、46项PIE检查通过；测试距离/消费隔离，未接正式配方 |
| TASK-012 | 结构化委托、真实移动/多趟入库、缺料/受阻返营、取消及代次/epoch保护；Editor构建、6项原生、45项PIE及10项定向检查通过；独立夹具，真实模型NOT_RUN |
| TASK-014 | 独立地图、方块网格和四种材质；项目资产引用闭包六项，移动/碰撞/坡道/门洞/暂停28项PIE检查通过；GitHub独立克隆Editor构建及28项检查通过，第二真人尚未参与 |
| TASK-015 | 原生自由输入、思考/台词、目标/真实入库进度、受阻/取消/过期反馈；背包互斥与焦点恢复通过；快捷建议生成未接入，R20/R23保持OPEN |
| TASK-016 | 已实现原型池、世界/库存/知识同边界恢复、任务续作和旧回复隔离；危险标志显式夹具化，未接入GDD尚未实现的系统；最终证据见交接 |
| TASK-017 | 存档列表/详情、真实保存/回档/新进度/锁定/删除、间隔设置和退出确认；菜单暂停/互斥、输入恢复、48项PIE通过；证据见017交接 |
| TASK-018 | 玩家采集/营地入库、资源争用、满载/耗尽及回档一致性；原生2项、PIE39项、镜头4项通过，见018交接 |
| TASK-019 | 营地R键五物品存取、数量/容量反馈、会话与回档保护；验证见019交接 |
| TASK-020 | 九页UI、配套玩法、六类日志、旧档迁移；背景结构拆分及F10组件布局编辑已实现；功能验证与用户验收通过；详见020交接 |
| TASK-021 | 伙伴采集、返营、跟随及进攻接UE原生导航；绕障、不可达和回档验证见021交接 |
| TASK-022 | 独立工作台/篝火、真实五秒建造、中断不耗料、摆放校验及建筑回档；验证见022交接 |
| TASK-023 | 工作台E访问、配方/批量即时制作、原子库存交换及回档；Editor构建、58项PIE与2项原生通过，实键结果见023交接 |
| TASK-024 | 工作台装备维修、逐件原型费用、完整结算及耐久回档；Editor构建、59项PIE与1项原生通过，实键结果见024交接 |
| TASK-025 | v2 已通过 PR #23 合并 main `851d60e`；一次确认、规范目标/约束、事件回执、真实制作维修与迁移已进入主干 |
| TASK-004 | 部分交付：77个自然素材源文件及来源说明已入库，见resourceSummary.md；UE适配/展示验证未完成，人物与房屋缺项，整单尚未验收 |
| TASK-026 | 实施进度持续，但 workflow 状态为 Blocked：Rebuild 地图保留 4.032 km 底座，营地局部增加树石灌木并接入主菜单新游戏、存档/继续游戏；实机闭环通过，完整验收与正式 Issue/Reviewer 待完成，见[营地接入记录](world/TASK-026/CAMP_INTEGRATION.md) |
| TASK-028 | main canonical：建筑、物品、武器、防具等 3D 资产导入与游戏应用；当前 Backlog，仅规划，未执行 |
| TASK-029 | **AI NPC 完整交付**：统一包含 authoritative perception/safety、deterministic executor、contextual suggestions、hold/follow/assist/routine、adaptive recovery、typed Belief、event-driven Initiative、grounded Episode、tactical cooperation、Coordination Prior、componentization、bounded context、real Qwen guardrail、Schema 2→3 migration 与自然营地接入。latest-main 核心：Editor build PASS、Python 31/31、native 41/41、runtime smoke 23/23、Executor 49/49、Initiative 16/16、Tactical 16/16、Routine 26/26；真实 Qwen 32/32 safety、M01～M10 raw 20/20。双工作树集成树已完成 Editor build、Python 31/31、native 42/42、runtime smoke 23/23、UI 62/62、自然路线 50/50、疾跑跟随与攻击夹具验证；repository validator 仍有 canonical TASK-027 两项 workflow metadata 错误。 |
| Issue／评审 | TASK-029 将通过本次统一 PR 进入正式代码评审；Agent 不执行 main merge，Owner/独立 Reviewer 验收仍单列 |
| 打包／两机验证／完整 M0 | NOT_RUN |
| 本地模型 | 项目已含llama.cpp b10964及Qwen3.5-4B Q4_K_M；真实UE自然语言采集入库、澄清/拒绝与生命周期已验证；台词质量仍有记录限制，见TASK-013交接 |
| 付费资产生成 | NOT_RUN |

用户已验收 TASK-020。TASK-026 当前营地实机闭环与局部视觉结果不代表完整自然场景验收通过。AI NPC 本轮统一按 **TASK-029 完整交付** 收口；latest-main 核心与自然营地接入各自已有验证证据，本轮集成分支已完成 build/native/Python/runtime/UI/自然路线/跟随/攻击夹具复验；旧 PR #34 不代表新分支。repository validator 的 canonical TASK-027 两项流程缺失单独记录，不由 TASK-029 越权修复；Agent 不直接合并 main。
[TASK-018交接](handoffs/TASK-018.md)记录当前玩家采集入库、回档、录像及边界。
[TASK-017交接](handoffs/TASK-017.md)记录存档管理UI、实键录像、受测源码快照和限制。
[TASK-013交接](handoffs/TASK-013.md)记录本地模型与设计修订的当前结果。
[TASK-012 交接](handoffs/TASK-012.md)记录当前伙伴委托原型、测试和真实模型缺项。
[TASK-011 交接](handoffs/TASK-011.md)记录已提交距离交互与隔离测试。
[TASK-010 交接](handoffs/TASK-010.md)记录已提交共享仓储与真实转移。
[TASK-009 交接](handoffs/TASK-009.md)记录已提交背包面板、暂停与验证。
[TASK-008 交接](handoffs/TASK-008.md)记录已提交个人背包与验证。
[TASK-007 交接](handoffs/TASK-007.md)记录已提交HUD与验证。
[TASK-006 交接](handoffs/TASK-006.md)记录已提交的动作机制与验证。
[TASK-005 交接](handoffs/TASK-005.md)记录已提交的世界时钟验证。
[工程交接](handoffs/TASK-003.md)记录证据及初始化日志中的未解决提示；
[资产任务单](tasks/TASK-004.md)定义首批范围、设计边界、许可和导入验收。
运行测试发生于实现提交前工作树，提交绑定说明见交接；该条仅描述 TASK-003 的提交绑定；TASK-005 有独立构建和运行测试证据。

启动来源见 [TASK-000 交接](handoffs/TASK-000.md)及 [STARTER_VALIDATION](qa/STARTER_VALIDATION.md)，不替代游戏证据。

[TASK-014交接](handoffs/TASK-014.md)：本单地图、资产来源、LFS锁、通行录像和干净克隆验证；用户授权直接提交推送，不授权合并。

[TASK-015交接](handoffs/TASK-015.md)：原生自由输入与真实伙伴/背包状态UI；无全局建议生成，R20/R23保持OPEN。

[TASK-016交接](handoffs/TASK-016.md)：50点保护、原子文件、同边界恢复、真实模型请求废止及跨PIE磁盘恢复；正式全世界存档仍待系统接入。

[TASK-019交接](handoffs/TASK-019.md)：当前营地仓储界面、验证和提交绑定。

[TASK-020交接](handoffs/TASK-020.md)：九页参考UI、数据驱动内容与真实配套玩法、验证状态及视觉差异。
