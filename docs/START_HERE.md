# 最短上手路径

## 当前本地准备阶段

先看 [TASK-000 交接](handoffs/TASK-000.md)与[首个工程任务 TASK-003](tasks/TASK-003.md)。
用户已授权阅读 GDD v0.3，并提交推送工作流启动基线；先前暂不读设计的限制已解除。
根 WORKFLOW.md 是工作流维护入口，docs 下中文工作流保留为交付快照。
本次按 WORKFLOW 第20.1节为原空仓库建立初始 main；后续改动使用任务分支与PR。
TASK-003 已完成本地实现和运行验证，见 [工程交接](handoffs/TASK-003.md)；流程因 Issue／独立评审缺项保持 Blocked。
[TASK-004](tasks/TASK-004.md) 为 UE 基础资产准备，Backlog，仅任务单、未执行。
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

当前[TASK-017](tasks/TASK-017.md)由用户授权按设计自行拆解为存档管理/退出确认UI。F6打开，复用016快照；实现与验证已完成，已按授权提交推送任务分支，见[017交接](handoffs/TASK-017.md)。
