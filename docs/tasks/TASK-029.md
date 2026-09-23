# TASK-029｜AI NPC 完整交付

状态：Blocked（代码与技术验证已完成；最终 PR 独立评审、Owner 体验验收和 main 合并尚未完成）。

## 统一口径

2026-09-23 Owner 指定：Hearthward AI NPC vNext 的最终任务、PR 与验收统一使用 **TASK-029**。

早期研发过程中使用过 TASK-027～040 等内部编号。它们不再作为对外任务口径，仅作为历史实现/证据标签保留在 `docs/qa/evidence/TASK-029/internal-history/` 或原始 raw evidence 中。项目当前 canonical TASK-028 是 3D 资产导入任务，与 AI executor 的历史内部编号无关。

## 目标

交付一个受 UE 权威世界约束、能够自然交流并执行真实游戏行动的 AI NPC：

```text
玩家输入 / 显式快捷建议
        ↓
本地 Qwen 结构化理解
        ↓
bounded context + capability contract
        ↓
deterministic guardrail / preflight
        ↓
候选任务卡 + 玩家确认
        ↓
UE Goal → Plan → Action executor
        ↓
导航 / 战斗 / 采集 / 制作 / 维修 / 结算
        ↓
事件、Belief、Episode、Coordination、Initiative
        ↓
下一轮可信上下文
```

LLM 负责理解与表达，不拥有世界写权限。

## 已交付能力

- UE authoritative perception / safety：玩家或模型文本不能制造安全地点、库存、敌人或隐藏世界事实。
- deterministic Goal → Plan → Action executor：collect / craft / repair 由 typed actions 执行，真实物资、receipt、timeline 和 save 恢复保持一致。
- contextual suggestions：最多三条，只有玩家显式刷新；未点击内容不进入 NPC cognition，点击后仍走正常推理/确认链。
- hold / follow / assist / routine 高层指令；具体目标、导航、LOS、伤害由 UE 决定。
- adaptive recovery / replanning：短暂路径/目标变化可有界恢复，不凭空生成未知替代目标。
- typed Belief / Knowledge State：firsthand / player_report / receipt provenance，世界真值与 NPC 认知分离。
- event-driven Initiative：完成、重规划、受阻、认知纠正可触发主动提醒，不轮询模型。
- grounded Episode Memory：从真实事件聚合过去行动，带 coverage/evidence。
- tactical cooperation：Assist / Protect / Regroup。
- Coordination Prior：只从已执行的真实协作事件学习近期偏好，不自动覆盖当前指令。
- Camp Routine：低权限巡营/查看营地/休息/回营，真实导航、不生产资源、不调用 LLM。
- componentization：Navigation / Behavior / Initiative Queue / Local AI Runtime 分层。
- bounded ContextProjection：full / compact / required-minimal 三档，真实 apply-template/tokenize 后最多一次 generation。
- Save schema 3 与 Schema 2 → 3 migration：Belief freshness 与 Episode coverage 有明确迁移语义。

## 权限边界

TASK-029 不允许模型直接决定：

- 世界坐标、具体敌人 ID、路径点；
- 命中、伤害、物资扣除/发放、库存结算；
- 未探索事实或安全真值；
- 任意工具调用或第二条世界写通路。

所有 side effect 必须通过 UE deterministic validation / executor。

## 最终技术验收

同步 main 前已完成：

- UE 5.8.2 default Unity Editor Development build：PASS
- full native `Hearthward.*`：41/41 PASS
- real Schema 2 → 3 disk migration：1/1 PASS
- real Qwen M01～M16 × clean/pressure：32 cases / 32 generation calls / 32/32 safety PASS
- M01～M10 core raw model contract：20/20 PASS
- CTX-03：full 超预算后降到 `compact_relevant`，2832 tokens，generation=1，限制保留
- CTX-04：`required_minimal` 4020 tokens，`CONTEXT_OVERFLOW`，generation=0，无候选/世界写
- executor runtime PIE：49/49 PASS
- Initiative PIE：16/16 PASS
- Tactical Cooperation PIE：16/16 PASS
- Camp Routine PIE：26/26 PASS
- repository validator：0 errors
- Python repository tests：31/31 PASS

最终 main 同步后还需重新执行集成 build/native/validator smoke，再创建 PR。

## Raw model 与 guardrail 的报告原则

真实 Qwen 的 raw JSON、normalized/applied result、deterministic guardrail 和最终 world state 分开记录。某些安全样本的 raw intent 可能偏宽，但只要 guardrail 正确阻断，也不会把它伪装成 raw model 本身成功。

## 证据入口

- 总验收：[TASK-029 FINAL_ACCEPTANCE](../qa/evidence/TASK-029/FINAL_ACCEPTANCE.md)
- 原始 suggestions 验证：[TASK-029 validation](../qa/evidence/TASK-029/VALIDATION.md)
- 历史实现索引：`docs/qa/evidence/TASK-029/internal-history/`
- 大型模型/上下文 raw 证据仍保留在内部来源目录，由 FINAL_ACCEPTANCE 统一索引。

## 剩余流程

1. 完成最新 main 集成与冲突解决。
2. 最终 build/native/repository validation。
3. push `codex/ai-npc-vnext-rework-01-fix`。
4. 创建 TASK-029 PR。
5. 独立 Reviewer / Owner 验收。
6. **不由 Agent 直接 merge**。
