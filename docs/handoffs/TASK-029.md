# TASK-029 交接｜AI NPC 完整交付

## 当前口径

AI NPC vNext 最终统一使用 **TASK-029**。早期 027～040 仅作为内部历史证据标签；项目 canonical TASK-027 为人物动作，canonical TASK-028 为 3D 资产任务。

目标分支：`codex/ai-npc-vnext-rework-01-fix`。GitHub PR：[#34](https://github.com/XLingyyy/Hearthward/pull/34)。**Agent 不直接 merge main。**

## 最终集成结构

最终候选把两条已经存在的开发线收口到一起：

1. `main@ba547c0`：包含 PR #35 主角模型与基础动作；
2. PR #34 原 head `25d53d8`：包含自然地图营地 AI 接入、有限资源、Navigation Invoker、UI 与兼容旧自然档；
3. latest-main AI 核心返工：保留 authoritative perception、typed executor、Recovery、Belief、Initiative、Episode、Tactical、Coordination、Routine、ContextProjection、Schema 3 migration。

核心原则仍是：LLM 做理解与表达；UE 做世界事实、安全、路径、战斗、结算和持久化权威。

## latest-main 核心验证

隔离 latest-main candidate 基于 `ba547c0`：

- Editor Development：PASS
- Python：31/31 PASS
- native：41/41 PASS
- Schema 2 → 3 real-file migration：1/1 PASS
- Executor：49/49
- Initiative：16/16
- Tactical：16/16
- Routine：26/26
- explicit runtime smoke：23/23

TASK-036 初次 tactical 回归失败来自 runner 的旧坐标假设：旧脚本以 camp 作为敌人原点，而当前 adventure 启用时 encounter 以玩家 Origin 创建。生产 tactical code 未改；runner 改为玩家 Origin 后恢复 16/16。

## 自然营地接入证据

PR #34 的自然地图接入此前已完成：

- Editor build PASS
- native 42/42
- Python 31/31
- 自然采集/存读档 22/22
- 工作台闭环 23 项
- 旧自然档升级 12/12
- 真实键鼠完成输入、任务卡确认、两份木材采集入库

详见 [natural-camp integration report](../qa/evidence/TASK-029/natural-camp-integration/REPORT.md)。

## repository validator 真实状态

全仓 validator **不是 0 errors**。当前 9 项错误全部来自 canonical TASK-026/027/028 的 reviewer / Issue URL / required_tests metadata。纯净 `main@ba547c0` 运行相同命令得到完全相同的 9 项，因此不把它们记为 TASK-029 回归，也不越权修改其他任务元数据。

task-scope validator 在隔离验证 worktree 额外记录 detached branch 与 TASK-029 baseline snapshot 两项流程约束。

详见 [latest-main finalization](../qa/evidence/TASK-029/LATEST_MAIN_FINALIZATION.md)。

## 真实模型边界

Qwen M01～M16 clean + pressure 为 32/32 safety PASS；M01～M10 core raw contract 20/20。CTX-03 降档到 compact 2832 tokens、generation=1；CTX-04 required-minimal 4020 tokens、CONTEXT_OVERFLOW、generation=0、无 candidate/world write。

部分安全样本 raw intent 可能偏宽；deterministic guardrail 正确拒绝/澄清。raw model 与 guardrail 结果继续分开报告。

## 下一步

1. 在最终合并树上重跑 build/native/runtime smoke 与 Python。
2. 记录 repository validator（预期仍包含 main 基线 9 项，除非对应 canonical tasks 被独立修复）。
3. commit/push 到 `codex/ai-npc-vnext-rework-01-fix`，更新 PR #34。
4. 等待独立 Reviewer / Owner 体验验收。
5. **不由 Agent 合并 main。**
