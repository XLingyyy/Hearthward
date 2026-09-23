# 最短上手路径

## 当前本地准备阶段

先看 [TASK-000 交接](handoffs/TASK-000.md)与[首个工程任务 TASK-003](tasks/TASK-003.md)。
用户已授权阅读 GDD v0.3，并提交推送工作流启动基线；先前暂不读设计的限制已解除。
根 WORKFLOW.md 是工作流维护入口，docs 下中文工作流保留为交付快照。
本次按 WORKFLOW 第20.1节为原空仓库建立初始 main；后续改动使用任务分支与PR。
TASK-003 已完成本地实现和运行验证，见 [工程交接](handoffs/TASK-003.md)；流程因 Issue／独立评审缺项保持 Blocked。
[TASK-004](tasks/TASK-004.md) 为 UE 基础资产准备，Backlog；已有77个自然素材源文件入库，UE适配和整单验收未完成，见 [资源汇总](../resourceSummary.md)。

当前 main 的自然地图工作以 [TASK-026](tasks/TASK-026.md) 为准：UE 5.8.2 下的 4.032 km World Partition 地图已接入 Bootstrap 新游戏/存档/继续游戏，新营地与局部树石实例已落地；完整路线、流送、性能和 Owner 视觉验收仍未完成，详见[营地接入记录](world/TASK-026/CAMP_INTEGRATION.md)。

AI NPC vNext 最终统一按 **TASK-029 AI NPC 完整交付** 作为对外任务口径。当前最终候选已同步 `main@ba547c0` 并合入 PR #34 的自然营地 AI：world-authority safety、deterministic executor、suggestions、directives/combat、recovery、belief、initiative、episode、coordination、routine、componentization、bounded context、真实 Qwen guardrail、Save migration 与自然营地伙伴/有限资源/仓储/制作/兼容旧档共同存在。latest-main 核心验证为 Editor build PASS、Python 31/31、native 41/41、runtime smoke 23/23、Executor 49/49、Initiative 16/16、Tactical 16/16、Routine 26/26；repository validator 的 9 项问题在纯净 main 同样复现，属于 TASK-026/027/028 workflow metadata。开发与验收状态优先看 [TASK-029 交接](handoffs/TASK-029.md) 和 [latest-main finalization](qa/evidence/TASK-029/LATEST_MAIN_FINALIZATION.md)。
[TASK-005](tasks/TASK-005.md) 正常运行世界时钟已提交推送。
[TASK-006](tasks/TASK-006.md) 五秒持续动作基础已提交推送。
[TASK-007](tasks/TASK-007.md) 实时动作HUD已提交推送。
[TASK-008](tasks/TASK-008.md) 个人背包基础已提交推送。
[TASK-009](tasks/TASK-009.md) 背包查看界面与默认暂停已提交推送。
[TASK-010](tasks/TASK-010.md) 共享仓储与真实转移已提交推送。
[TASK-011](tasks/TASK-011.md) 距离交互与五秒动作已提交推送。
[TASK-012](tasks/TASK-012.md) 伙伴安全委托与多趟交付已提交推送。
[TASK-013](tasks/TASK-013.md) 接入本地Qwen3.5 4B，运行库与GGUF放入项目，由UE启动；轻量规则/RAG/可知状态过滤后做一次生成，仍通过世界执行器校验。模型版本见 `config/local-ai.lock.json`，验证状态见 [TASK-013交接](handoffs/TASK-013.md)。
用户批准当前设计原件同步修订，见 [DSGN-001](design/DSGN-001-local-inference.md)。原013存档范围由TASK-016继续实现；TASK-004继续由用户安排。

## 新成员第一次

读根 [WORKFLOW](../WORKFLOW.md) 的开工、隔离、PR与安全章节；看 [ENVIRONMENT](ENVIRONMENT.md)、
[OWNERSHIP](OWNERSHIP.md)；由集成人安排 [启动清单](planning/BOOTSTRAP.md)。
不要将文档存在误读为GitHub设置、UE编译或真实模型已经完成。

## 每个新开发 Agent 会话

1. 根 [AGENTS](../AGENTS.md)。
2. [PROJECT_STATE](PROJECT_STATE.md)，查看当前基线和已验证状态。
3. 指定 `docs/tasks/TASK-xxx.json`，从真实Issue核对最新归属。
4. **该任务分支**的交接 `docs/handoffs/TASK-xxx.md`；不能只在main找未合并任务的最新工作。
5. 相关契约、设计来源和允许路径内的实际代码；不全量读所有历史。
6. `python scripts/agent_context.py --task TASK-xxx`，只读回执后再写入。

缺任一步所需信息，先说缺什么，不靠自动记忆填补。

## 原件与检索

[当前设计](design/CURRENT.md)给出摘要和来源；[R项](design/OPEN_QUESTIONS.md)列出所有未定点。
当前Word位于docs根目录；归档和机械提取文保留历史内容，按章／Q号查阅时同时核对DSGN-001，不能用归档覆盖已批准修订。

[TASK-014](tasks/TASK-014.md)建立独立灰盒验证场，地图 /Game/Hearthward/Tests/Graybox/L_GrayboxValidation；现有玩法复用，测试尺寸不转为正式设计。用户授权实现、提交、推送，验证和克隆结果见[014交接](handoffs/TASK-014.md)。

[TASK-015](tasks/TASK-015.md)提供原生伙伴自由输入、模型思考/台词和真实委托进度；临时T键打开，Enter发送，Esc关闭。仍需开发命令生成独立伙伴夹具；不自动添加到地图。用户授权实现和提交推送，证据见[015交接](handoffs/TASK-015.md)。

[TASK-016](tasks/TASK-016.md)接入现有伙伴夹具的世界/库存/知识快照与全局50点原型档池；开发入口和验证边界见[016交接](handoffs/TASK-016.md)。危险检测、正式初始节点及完整世界模块仍待后续接入。

[TASK-017](tasks/TASK-017.md)由用户授权按设计自行拆解为存档管理/退出确认UI。F6打开，复用016快照；实现与验证已完成，已按授权提交推送任务分支，见[017交接](handoffs/TASK-017.md)。

[TASK-018](tasks/TASK-018.md)：伙伴夹具按E采集木材/营地入库，复用背包、交互和存档；用户授权自行拆解、实现及提交推送，见[018交接](handoffs/TASK-018.md)。

[TASK-019](tasks/TASK-019.md)：营地仓储数量选择及双向存取；用户授权自行实现并提交推送，见[019交接](handoffs/TASK-019.md)。

已验收[TASK-020](tasks/TASK-020.md)：九页参考UI与缺失玩法；新入口为标题页“新游戏”，详细操作和当前限制以根README与[020交接](handoffs/TASK-020.md)为准。

TASK-021伙伴原生导航已提交推送，见[021交接](handoffs/TASK-021.md)。[TASK-022](tasks/TASK-022.md)自由建造已提交推送，见[022交接](handoffs/TASK-022.md)。[TASK-023](tasks/TASK-023.md)即时制作已提交推送，见[023交接](handoffs/TASK-023.md)。当前[TASK-024](tasks/TASK-024.md)接入工作台装备维修与耐久回档，用户授权自主提交推送，见[024交接](handoffs/TASK-024.md)。

[TASK-025](tasks/TASK-025.md) 增强版 v2 已通过 PR #23 合并 main `851d60e`；[TASK-026](tasks/TASK-026.md) 已通过 PR #25 合并 main `4114556`。AI NPC 研发历史曾使用 027～040 等内部标签，但 canonical TASK-027 现为人物动作、TASK-028 为 3D 资产；Owner 指定 AI 最终只按 **TASK-029** 完整交付。当前 PR #34 分支正在完成最终合并树复验与 push；明确不得由 Agent 直接 merge main。
