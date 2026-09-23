# Hearthward 项目状态

2026-09-23局部更新：remote main 当前为 `73bb10e`（PR #30 后）；AI NPC 修复候选分支为 `codex/ai-npc-vnext-rework-01-fix`。Owner 指定最终对外验收/PR 统一使用 **TASK-029 AI NPC 完整交付**；仓库内部 TASK-027～040 继续保留为可追溯实现与证据。当前返工源码已完成 default Unity Editor build、native 41/41、真实 Qwen 32-case matrix、CTX-03/04、Schema 2→3 real-file migration 与 TASK-028/034/036/038 最终 PIE 回归；尚待同步 main、最终集成 validate、PR review 与 Owner 验收。

历史基线：TASK-020 于2026-09-19通过用户验收；021伙伴导航、022自由建造、023即时制作、024维修已进入后续主干。原025经PR #22合并后被用户否决验收；增强版v2随后通过PR #23合并main。TASK-004已有77个自然素材源文件入库，UE适配未完成。

| 项目 | 实际状态 |
|---|---|
| 开发根目录 | 独立 Hearthward 仓库，与上层 GameFactory 工具仓库隔离 |
| 工作流 | v1.0 已导入；根 WORKFLOW.md 为维护入口，团队采用仍为 DRAFT |
| GitHub | origin 为 XLingyyy/Hearthward，公开仓库；当前按授权推送任务分支 |
| 当前分支 | `codex/ai-npc-vnext-rework-01-fix`；内部承接 TASK-027～040，对外统一作为 TASK-029 AI NPC 完整交付；已授权 commit / push / 创建 PR，明确不直接 merge |
| 游戏实现提交 | main 当前为 `73bb10e`，包含 TASK-025 v2、TASK-026 与后续资产/地图提交；AI NPC 完整栈仍留在候选任务分支，等待 PR |
| Git LFS | 已启用；TASK-026 地图/资产由 main 继承，本 PR 不新增或修改其 LFS 资产 |
| 工具链 | 当前统一锁定 UE 5.8.2 CL 56702186、MSVC 19.44.35228.0、SDK 10.0.22621.0；目标引擎安装与锁定版本一致 |
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
| TASK-026 | 已通过 PR #25 合并 main `4114556`；4032 m World Partition灰盒、026材质/贴图、独立浏览GameMode与27项定向PIE已生成。视觉、长路线、Standalone、性能和Owner验收仍未完成 |
| TASK-027 | 本 PR：authoritative NPC perception/safety。候选形成、确认和执行阶段共享 UE 权威观察；玩家/模型文本不能写入安全或隐藏世界事实。Safety PIE 27/27 PASS |
| TASK-028 | 本 PR：deterministic Goal→Plan→Action executor。typed actions、plan cursor、retained cargo、receipt、存档计划重建；Executor PIE 49/49 PASS |
| TASK-029 | **对外总交付口径**：AI NPC 完整交付。原 029 contextual suggestions 仍保留为内部能力之一；总交付实际由 TASK-027～040 共同构成，包含 perception/safety、executor、suggestions、combat/directives、recovery、belief、initiative、episode、coordination、routine、componentization、bounded context、real Qwen guardrail 与 save migration。 |
| TASK-030 | 本 PR：`companion_order=hold/follow/assist` 与 player-centered deterministic combat policy；UE负责目标、导航、LOS、cooldown与伤害。Combat PIE 29/29、真实Qwen directive 14/14 PASS |
| TASK-031 | 本 PR：用户 UE 复验修正。修复 combat Tick 每帧停止 typed task 导航、整理对话页工具栏、过滤手动任务卡能力；用户反馈 PIE 11/11、真实Qwen“采两份木材”链 11/11 PASS |
| TASK-032 | 本地增量：deterministic adaptive recovery。Source relocation / transient route failure 进入 bounded retry/rewind；真实 cargo 优先返营。UE build PASS、NPCAgent 9/9、runtime PIE 11/11 PASS，未 push/merge |
| TASK-033 | 本地增量：typed Belief / Knowledge State。firsthand/player_report/receipt provenance；离营不偷看仓库真值，回营亲见纠正报告；World 与 Belief 同快照但独立。UE build PASS、NPCAgent 10/10、runtime PIE 25/25 PASS，未 push/merge |
| TASK-034 | 本地增量：event-driven Initiative。完成/重规划/受阻/认知纠正触发确定性主动提醒；超距排队、靠近再显示，无LLM轮询。UE build PASS、NPCAgent 11/11、runtime PIE 16/16 PASS，未 push/merge |
| TASK-035 | 本地增量：grounded Episode Memory。command-scoped 聚合真实取得/交付/replan/原因/evidence；past-action recall只引用事件证据。UE build PASS、NPCAgent 12/12、runtime PIE 21/21 PASS，未 push/merge |
| TASK-036 | 本地增量：战斗协作 Agent。健康 Assist；低血单威胁 Protect；低血多威胁 / 玩家倒地 Regroup；UE 继续负责真实目标与伤害。UE build PASS、NPCAgent 12/12、runtime PIE 16/16 PASS，未 push/merge |
| TASK-037 | 本地增量：Coordination Prior。真实已确认 hold/follow/assist 形成滚动行为 prior，只影响建议/上下文，不自动执行；save/load 从 events 重建。UE build PASS、NPCAgent 13/13、runtime PIE 28/28 PASS，未 push/merge |
| TASK-038 | 本地增量：营地自主 Routine。无任务/显式指令/战斗时按世界时间低权限巡营、查看营地、休息/回营；真实导航、不生产、不调用LLM。UE build PASS、NPCAgent 14/14、runtime PIE 26/26、TASK-028 executor 49/49 PASS，未 push/merge |
| TASK-039 | 本地增量：AI NPC 组件化收口。导航、Initiative队列、伙伴Behavior编排、Local AI Runtime抽成深模块；不改玩法/存档/权限。UE build PASS、全量native 39/39、Executor 49/49、Initiative 16/16、Tactical 16/16、Routine 26/26 PASS，未 push/merge |
| TASK-040 | 内部返工已技术闭合：Unity helper collision 修复；bounded ContextProjection；registry-driven prompt；Belief `LastEvidenceAt`；Episode coverage；Schema 3 migration。Editor build PASS、native 41/41、Schema2→3 real-file 1/1、真实 Qwen 32/32 safety、M01～M10 raw 20/20、CTX-03 compact 2832/1 generation、CTX-04 minimal 4020/0 generation；最终 TASK-028/034/036/038 PIE 分别 49/49、16/16、16/16、26/26 PASS。对外并入 TASK-029 总交付。 |
| Issue／评审 | 任务期内 GitHub Issue 创建曾返回403，因此027→031任务快照缺独立远端Issue/Reviewer；本次统一 PR 用于正式代码评审，不在 Agent 侧执行合并 |
| 打包／两机验证／完整 M0 | NOT_RUN |
| 本地模型 | 项目已含llama.cpp b10964及Qwen3.5-4B Q4_K_M；真实UE自然语言采集入库、澄清/拒绝与生命周期已验证；台词质量仍有记录限制，见TASK-013交接 |
| 付费资产生成 | NOT_RUN |

用户已验收TASK-020；TASK-025 v2与TASK-026已进入main。AI NPC 当前对外按 **TASK-029 完整交付** 收口，内部 TASK-027～040 保留完整历史。返工分支已闭合 Unity build、41/41 native、32-case real Qwen、CTX-03/04、Schema 2→3 real-file migration 及 028/034/036/038 当前源码 PIE；详细结论见 [TASK-040 clean-tree review](qa/evidence/TASK-040/CLEAN_TREE_REVIEW.md) 与 [validation](qa/evidence/TASK-040/VALIDATION.md)。尚未完成的是 main 同步后的最终集成验证、独立 Reviewer / Owner 验收和 main merge。
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
