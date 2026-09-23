# TASK-029 AI NPC 完整交付｜PR 交接

日期：2026-09-23
候选分支：`codex/ai-npc-vnext-rework-01-fix`
目标：`main`
PR：[#34](https://github.com/XLingyyy/Hearthward/pull/34)

## 统一口径

本次 AI NPC vNext 最终只使用 **TASK-029 AI NPC 完整交付** 作为对外任务和验收口径。早期 027～040 仅作为历史 evidence 标签；canonical TASK-027 为人物动作，canonical TASK-028 为 3D 资产任务。

## 最终候选组成

最终树收口三部分：

- `main@ba547c0`：PR #35 主角模型与基础动作；
- PR #34 原 head `25d53d8`：自然营地 AI 接入、有限木材点、仓储、Navigation Invoker、UI、兼容旧自然档；
- latest-main AI 核心：authoritative perception/safety、typed Goal→Plan→Action、Recovery、Belief、Initiative、Episode、Tactical、Coordination、Routine、ContextProjection、Schema 3 migration。

LLM 只负责理解与表达；坐标、目标、导航、战斗、结算、安全真值与存档世界状态仍由 UE 权威系统决定。

## 已绑定验证

latest-main AI 核心（基于 `main@ba547c0`）：

- UE 5.8.2 Editor Development：PASS
- Python：31/31 PASS
- full native `Hearthward.*`：41/41 PASS
- Schema 2→3 real-file migration：1/1 PASS
- Executor：49/49 PASS
- Initiative：16/16 PASS
- Tactical：16/16 PASS
- Routine：26/26 PASS
- explicit runtime smoke：23/23 PASS
- real Qwen M01～M16 clean + pressure：32/32 safety PASS
- M01～M10 core raw contract：20/20 PASS
- CTX-03：compact 2832 tokens / generation=1
- CTX-04：required-minimal 4020 tokens / generation=0 / no candidate/world write

自然营地线此前独立验证：

- Editor Development：PASS
- native：42/42
- Python：31/31
- 自然采集/存档：22/22
- 工作台与跨地图读档：23 项
- 旧自然档升级：12/12
- 真实键鼠完成输入、任务卡确认与两份木材交付

## TASK-036 runner 修正

latest-main tactical 首轮失败来自测试脚本旧坐标假设：旧 runner 按 camp 推导敌人位置，当前 encounter 在 adventure 启用时按玩家 Origin 创建。生产 tactical code 未修改；runner 改为玩家 Origin 后恢复 16/16。

## Repository validator

项目级 validator **不记录为 0 errors**。当前 9 项错误全部属于 canonical TASK-026/027/028 的 reviewer / Issue URL / required_tests workflow metadata；在纯净 `main@ba547c0` 上运行相同命令得到完全相同的 9 项，因此不归因于 TASK-029，也不在本 PR 越权修复。

## Evidence

- `docs/qa/evidence/TASK-029/FINAL_ACCEPTANCE.md`
- `docs/qa/evidence/TASK-029/LATEST_MAIN_FINALIZATION.md`
- `docs/qa/evidence/TASK-029/natural-camp-integration/REPORT.md`
- `docs/qa/evidence/TASK-029/revalidation-20260923/REPORT.md`
- `docs/handoffs/TASK-029.md`

## Release state

最终合并树正在做最后一轮 build/native/runtime smoke 与 repository record；完成后 fast-forward push 到 PR #34 分支。PR 保持 open，等待独立 Reviewer / Owner 体验验收。

**Agent 不直接 merge main。**
