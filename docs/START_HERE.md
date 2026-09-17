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
当前用户授权执行并提交推送 [TASK-008](tasks/TASK-008.md) 个人背包基础，不依赖 TASK-004。

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
完整Word在归档区，机械提取文供按章／Q号查阅，不代表新的设计版本。
