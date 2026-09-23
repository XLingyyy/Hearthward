# TASK-029 交接｜AI NPC 完整交付

## 当前口径

AI NPC vNext 最终统一使用 **TASK-029**。早期 027～040 仅作为内部历史证据标签；项目 canonical TASK-027 为人物动作，canonical TASK-028 为 3D 资产任务。

本轮工作分支：`codex/integrated-latest-20260923`，本地 checkpoint `66d0107`。旧 AI 分支的 GitHub PR [#34](https://github.com/XLingyyy/Hearthward/pull/34) 仍指向旧 head `25d53d8`，不是本轮集成分支。**Agent 不直接 merge main。**

## 2026-09-23 双工作树集成

在 checkpoint 上选择性吸收 TASK-041/042 的 UI 返回栈、快捷键与确认框独占、焦点恢复、实景树木采集绑定、左键单一 Enhanced Input 入口、近战空挥与真实命中结算、30 米伙伴指令反馈、疾跑跟随速度和稳定交互目标排序。没有覆盖 TASK-029 的模型、Schema 3 旧档升级，也没有修改 Content 动画或 Config 自动生成项。完整当轮证据见 [integrated-playable/REPORT](../qa/evidence/TASK-029/integrated-playable/REPORT.md)。

该分支最初继承 `main@ba547c0`；随后已合入 TASK-030 的 `main@be9286f`，本轮再引入 TASK-028 的 `main@97af300`。这段是分阶段历史，不代表当前仍缺 TASK-030/028。真实键鼠、石骨斧手部动作及跳跃手臂动画还需单独验收。

本轮范围提交 `4b3489aba85eac5dd2960200056a4b4302f36553` 和已验证源码提交 `4dae443094346f7d1ed8e4a62ba11370dc2165d7` 已推送至 GitHub 分支 `codex/integrated-latest-20260923`。本分支没有创建 PR 或合并 main；独立评审与真人验收仍未完成。

## 2026-09-24 当前 main 组合候选

用户明确要求将本轮分支按最佳结果合入 main。已将 `main@be9286fdc09beac8cf10ec21126472b0ce5bfbcb` 合入专用分支，解决 README 和 NaturalCamp 两处冲突，源码提交为 `1f8e4dd2c9c2e7b921c4aeb36e49ff5419f5ff6a`。该组合通过构建、Python 31/31、原生 43/43、TASK-030 Demo 70/70、UI 62/62、自然路线 51/51、跟随 8/8、攻击 10/10、runtime smoke 23/23，见[前轮报告](../qa/evidence/TASK-029/main-integration-20260924/REPORT.md)。

2026-09-24 再将 `main@97af300bd801917ece85c21b87d5ce482bf50052` 的 TASK-028 房屋、家具和石骨斧合入该分支。README 冲突按当前主干状态解决；源码自动融合。本轮重新通过 UE Editor Development、Python 31/31、native 43/43、TASK-030 Demo 70/70、无授物自然路线 51/51 与 TASK-028 隔离资产路线 49/49，见[最新主干报告](../qa/evidence/TASK-029/main-task028-integration-20260924/REPORT.md)。新主干新增 TASK-028 reviewer/Issue URL 两项流程错误，仓库 validator 合计 4 项。此前 UI/跟随/攻击/runtime smoke 的 PASS 与本轮源码不同，不作为新源码的重跑结果。下一步推送更新分支、建立 PR、接受非作者真人评审；合并后按 main 的新 SHA 再记集成验收。

GitHub 连接器尝试创建 TASK-029 集成 Issue 返回 `403 Resource not accessible by integration`；未创建 Issue，不编造 URL。canonical TASK-027 reviewer / Issue URL 两项既有 validator 错误仍在。

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

全仓 validator **不是 0 errors**。本轮工作树当前只剩 canonical TASK-027 的 reviewer 与 Issue URL 两项；旧 `main@ba547c0` 曾有 TASK-026/027/028 共 9 项。两次结果对应不同提交，均不是 TASK-029 新引入的代码错误，也不越权修改其他任务元数据。

task-scope validator 在隔离验证 worktree 额外记录 detached branch 与 TASK-029 baseline snapshot 两项流程约束。

详见 [latest-main finalization](../qa/evidence/TASK-029/LATEST_MAIN_FINALIZATION.md)。

## 真实模型边界

Qwen M01～M16 clean + pressure 为 32/32 safety PASS；M01～M10 core raw contract 20/20。CTX-03 降档到 compact 2832 tokens、generation=1；CTX-04 required-minimal 4020 tokens、CONTEXT_OVERFLOW、generation=0、无 candidate/world write。

部分安全样本 raw intent 可能偏宽；deterministic guardrail 正确拒绝/澄清。raw model 与 guardrail 结果继续分开报告。

## 下一步

1. 本轮统一分支的 build/native/Python/runtime/UI/自然路线/跟随/攻击夹具复验已完成，结果见上文集成报告。
2. 新分支已推送；旧 PR #34 保持原 head，需另行评审新分支。
3. 等待真实键鼠、可见武器/跳跃动画、独立 Reviewer / Owner 体验验收。
4. **不由 Agent 合并 main。**
