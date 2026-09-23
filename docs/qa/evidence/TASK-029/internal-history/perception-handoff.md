# TASK-027 交接

## 当前目标与实现

TASK-027 建立 AI NPC 的第一层真实世界感知与确定性安全策略，让 TASK-025 的规范目标在候选形成/确认和真实执行时都读取最新 UE 观察，而不是散落读取 `bSourceSafe`。

当前隔离分支已实现：

- 新增 `Source/Hearthward/AI/HearthwardNPCPerception.h/.cpp`。
- `Capture(companion)` 读取世界/暂停、战斗状态可用性与是否交战、营地/采集点可用性、已知安全点证据、导航重建、距离与执行阶段。
- `Evaluate(observation, goal)` 为纯安全策略：世界写入 fail-closed；采集需要安全来源+营地；bag 制作/维修不依赖采集点；当前 craft 因产物必须返营入库仍要求营地有效，repair 仅在 source=camp 时要求营地。
- `CompanionFixture` 的结构化采集与制作/维修共用 `AcceptGoal`，在接受前复核安全；采集、取料、制作/维修执行阶段继续复核。返营/已取得物资仍沿用既有安全恢复语义。
- `LocalAISubsystem` filtered context 改用同一权威观察快照，向模型暴露观察事实和 collection safety verdict；玩家/模型文本无安全写权。
- `NPCAgentTests` 新增 `Hearthward.NPCAgent.PerceptionSafety`，覆盖交战、未知安全状态、来源不可信、bag/camp 差异与暂停。

## 基线与隔离

- 基线：main `b1f85525697b79e6017455decab9d79a54977834`。
- 已核对该 main 包含 PR #23 merge `851d60e4dc8a9b2e31cea91e2c0fcce7ee5c326d`，因此旧文档中“025 v2 未合并”已在本分支修正。
- DevSpace 隔离 worktree 实施；源 checkout `D:\\Dev\\Hearthward` 的用户 `Hearthward.uproject` / `.codex/` 改动未触碰。
- 分支：`codex/TASK-027-npc-perception`。

## 验证

详见 [VALIDATION](../qa/evidence/TASK-027/VALIDATION.md)。

- `git diff --check`：PASS。
- `python scripts/validate_repo.py`：PASS，0 errors。
- Python 工具测试：31/31 PASS。
- 本机 Epic UE 5.8.2 首次构建受另一 UE 实例的 Live Coding mutex 阻止；未关闭用户的其他编辑器，改用 UBT `-NoHotReloadFromIDE` 后完成真实 C++ 编译与链接。第一次编译暴露并修复了一个 const 指针调用导航 API 的错误，随后 Editor build PASS。
- `Hearthward.NPCAgent` 原生自动化 5/5 PASS；全量 `Hearthward` 原生自动化 30/30 PASS。
- TASK-027 deterministic PIE：27/27 PASS，覆盖候选形成后安全变化、暂停、真实战斗进入/退出、采集中安全证据撤销、采集中进入战斗以及零副作用阻断。
- TASK-025 workshop deterministic 回归：129/129 PASS（三轮），确认本次安全分层没有破坏 own-bag/camp 制作维修、存档回执和规则版本行为。
- 隔离 worktree 不含 Qwen GGUF，因此本轮 real-model regression NOT_RUN；不影响确定性安全层验证。

## 流程状态

正式流程仍为 Blocked：本会话按用户要求再次调用 GitHub connector，但工具返回 `FORBIDDEN: This conversation is restricted to developer MCPs`，因此无法核验或创建 Issue/Reviewer，也未进行 commit/push/PR/merge。不要把本 worktree 成果描述成远端已交付。

## 设计边界与下一步

现有 `bSourceSafe` 只作为开发灰盒“已知安全采集点”的观测输入；本单没有实现完整视觉/听觉感知、EQS、开放世界危险评分或通用 Planner。

TASK-027 的本地实现与确定性验证已完成。下一工程步骤应是把本分支提交/推送并补真实 Issue/Reviewer；该远端流程当前受 GitHub connector 权限限制。远端基线落定后，下一开发单进入 TASK-028：把当前不断膨胀的 `EHearthwardCompanionPhase` 专用状态机收敛为通用 Goal → Plan → Action Executor，同时保持 TASK-027 的 Perception/Safety seam 作为执行前置条件。
