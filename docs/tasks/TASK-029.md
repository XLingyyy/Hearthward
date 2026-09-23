# TASK-029｜AI NPC 完整交付

状态：Blocked（实现与 latest-main 技术收口已完成；PR #34 仍待独立 Reviewer / Owner 体验验收，Agent 不合并 main）。

## 统一口径

2026-09-23 Owner 指定：Hearthward AI NPC vNext 的最终任务、PR 与验收统一使用 **TASK-029**。

早期研发使用过 027～040 等临时 AI 编号。它们只作为历史实现/证据标签保留，不再占用项目 canonical task namespace。当前 canonical `TASK-027` 是人物资产与基础动作，canonical `TASK-028` 是 3D 资产导入/玩法接入。

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
- Save schema 3 与真实 Schema 2 → 3 文件迁移；
- 自然地图营地接入：伙伴、有限木材点、仓储、对话确认、采集入库、跟随/等待/巡营、记忆、工作台制作与兼容旧自然档。

## 权限边界

模型不能直接决定世界坐标、具体敌人、路径、命中/伤害、物资结算、安全真值、未探索事实或任意第二世界写通路。所有 side effect 继续经过 UE deterministic validation / executor。

## latest-main 集成

最终候选以 `origin/main@ba547c0ee5a1d8dae41e747a5d1d0674d7702899` 为 latest-main 基线，并合入 PR #34 已有的自然营地 AI 接入。main 的 TASK-027 主角/动画能力与 TASK-029 companion behavior / combat policy / tactical settlement 同时保留。

latest-main AI 核心快照：

- repository Python tests：31/31 PASS；
- UE 5.8.2 HearthwardEditor Development：PASS；
- full native `Hearthward.*`：41/41 PASS，包含 `Hearthward.Save.Schema2To3RealFileMigration`；
- explicit Development runtime smoke：23/23 PASS；
- executor / Initiative / Tactical / Routine：49/49、16/16、16/16、26/26 PASS；
- TASK-036 初次 latest-main 回归暴露的是测试脚本坐标基准过时：敌人按 `EnableAdventure` 捕获的玩家 Origin 创建，旧 runner 却按 camp 推导；生产战术代码未改，runner 修正后恢复 16/16。

自然营地接入分支证据：

- Editor Development build：PASS；
- native：42/42；
- Python：31/31；
- 自然采集与存档：22/22；
- 工作台制作与跨地图读档：23 项；
- 旧自然存档升级：12/12。

## repository validator

`python -X utf8 scripts/validate_repo.py` 当前不能记录为全仓 PASS：候选报 9 项错误，全部属于 canonical TASK-026/027/028 的 Issue/Reviewer/required_tests workflow metadata；在纯净 `main@ba547c0` 上运行同一命令复现完全相同的 9 项。因此它们不归因于 TASK-029，也不能被 TASK-029 越权修改或伪装成 0 errors。

task-scope validator 在隔离验证树还会额外报告：worktree 为 detached、TASK-029 在 `ba547c0` 基线没有 approved snapshot。两项均按真实结果记录。

## 真实模型证据

- Qwen M01～M16 clean + pressure：32/32 safety PASS；
- M01～M10 core raw model contract：20/20；
- 32 cases / 32 generation calls；
- CTX-03：compact_relevant 2832 tokens，generation=1，限制保留；
- CTX-04：required_minimal 4020 tokens，CONTEXT_OVERFLOW，generation=0，无候选/世界写。

raw model、normalized/applied result、deterministic guardrail 与最终 world state 分开记录；不把 guardrail 成功伪装成 raw model 本身成功。

## 证据入口

- 总验收：`docs/qa/evidence/TASK-029/FINAL_ACCEPTANCE.md`
- latest-main 最终收口：`docs/qa/evidence/TASK-029/LATEST_MAIN_FINALIZATION.md`
- 自然营地接入：`docs/qa/evidence/TASK-029/natural-camp-integration/REPORT.md`
- 本轮基线复验：`docs/qa/evidence/TASK-029/revalidation-20260923/REPORT.md`
- 历史研发索引：`docs/qa/evidence/TASK-029/internal-history/`

## 剩余流程

1. 对最终合并树运行最后一轮 build/native/runtime smoke 与 repository record。
2. 更新 PR #34 到最终提交。
3. 独立 Reviewer / Owner 体验验收。
4. **Agent 不直接 merge。**
