# TASK-029｜AI NPC 完整交付

状态：Blocked（AI 代码与本地技术验证已完成到最新 main；PR #34 尚待把本轮 latest-main 收口提交推送，独立 Reviewer / Owner 体验验收与 main 合并仍未完成）。

## 统一口径

2026-09-23 Owner 指定：Hearthward AI NPC vNext 的最终任务、PR 与验收统一使用 **TASK-029**。

早期研发使用过 027～040 等临时 AI 编号。它们现在只保存在 `docs/qa/evidence/TASK-029/internal-history/`，不再占用项目 canonical task namespace。当前 main 的 canonical `TASK-027` 是人物资产与基础动作，canonical `TASK-028` 是 3D 资产导入/玩法接入。

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

- authoritative perception / safety；
- deterministic typed Goal → Plan → Action executor；
- explicit contextual suggestions + stale revalidation；
- hold / follow / assist / routine；
- adaptive recovery / bounded replanning；
- provenance-aware Belief / Knowledge State；
- event-driven Initiative；
- grounded Episode + coverage；
- tactical Assist / Protect / Regroup；
- bounded Coordination Prior；
- low-authority camp Routine；
- Navigation / Behavior / Initiative Queue / Local AI Runtime componentization；
- full / compact / required-minimal ContextProjection；
- real llama.cpp apply-template/token counting，accepted request 最多一次 generation；
- Chinese quantity guardrail；
- Save schema 3 与真实 Schema 2 → 3 文件迁移。

## 权限边界

模型不能直接决定世界坐标、具体敌人、路径、命中/伤害、物资结算、安全真值、未探索事实或任意第二世界写通路。所有 side effect 继续经过 UE deterministic validation / executor。

## 最新 main 集成

本轮重新以 `origin/main@ba547c0ee5a1d8dae41e747a5d1d0674d7702899` 为基线建立隔离 worktree，并叠加 TASK-029 完整 AI 栈与最终返工修复。

该 main 已包含 PR #35 的角色资产/基础动作。集成后的 `HearthwardGameplayComponent` 同时保留：

- Hero attack animation trigger；
- AI companion behavior / combat policy / tactical settlement。

本轮不修改 `Content/`，因此不会覆盖角色、地图或资产车道。

当前 latest-main 本地结果：

- repository Python tests：31/31 PASS；
- UE 5.8.2 HearthwardEditor Development：PASS；
- full native `Hearthward.*`：41/41 PASS，包含 `Hearthward.Save.Schema2To3RealFileMigration`；
- TASK-029 explicit Development fixture runtime smoke：23/23 PASS；
- 专项 executor / Initiative / tactical / Routine PIE：正在按最新 main 的 explicit fixture 入口复验；
- repository validator 的项目级结果单独报告，不把 main 上既有 Issue/Reviewer/required_tests 元数据问题伪装成 AI PASS。

## 真实模型证据

最终返工的真实模型证据保持原始结果，不因编号收口而重写：

- Qwen M01～M16 clean + pressure：32/32 safety PASS；
- M01～M10 core raw model contract：20/20；
- 32 cases / 32 generation calls；
- CTX-03：compact_relevant 2832 tokens，generation=1，限制保留；
- CTX-04：required_minimal 4020 tokens，CONTEXT_OVERFLOW，generation=0，无候选/世界写；
- historical executor 49/49、Initiative 16/16、tactical 16/16、Routine 26/26。

原始 JSON、runner、Schema migration/native index 已迁入 `docs/qa/evidence/TASK-029/internal-history/context-cognition/`。

## Raw model 与 guardrail 报告原则

真实 Qwen raw JSON、normalized/applied result、deterministic guardrail 与最终 world state 分开记录。M11/M12/M14/M16 的部分 raw 输出可能比理想拒绝/澄清更宽，但 guardrail 正确阻止禁止执行；不得把 guardrail 成功写成 raw model 本身成功。

## 证据入口

- 总验收：`docs/qa/evidence/TASK-029/FINAL_ACCEPTANCE.md`
- latest-main regression runners：`docs/qa/evidence/TASK-029/regression/`
- 历史研发索引：`docs/qa/evidence/TASK-029/internal-history/README.md`

## 剩余流程

1. 完成 latest-main 专项 PIE 与 repository validation 记录。
2. 更新 README / PROJECT_STATE / START_HERE 与 handoff。
3. 将本轮 latest-main 集成提交到 TASK-029 PR 分支并更新 PR #34。
4. 独立 Reviewer / Owner 体验验收。
5. **Agent 不直接 merge。**
