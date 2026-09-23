# TASK-029 交接｜AI NPC 完整交付

## 当前口径

AI NPC vNext 最终统一使用 **TASK-029**。

早期研发的 027～040 编号只作为内部历史来源；其中历史 “AI TASK-028 executor” 已归档到 `docs/qa/evidence/TASK-029/internal-history/TASK-028-agent-executor/`，因为项目 main 当前 canonical TASK-028 已被用于 3D 资产任务。

当前候选分支：`codex/ai-npc-vnext-rework-01-fix`。

历史候选：`037628f`；本次自然地图接入提交在 `bf5fb97` 基线上继续。GitHub PR：[#34](https://github.com/XLingyyy/Hearthward/pull/34)，目标 `main`，当前保持 open，Agent 不执行 merge。

## 架构

```text
Player / Suggestions
        ↓
Local Qwen
        ↓
ContextProjection + Capability Contract
        ↓
Deterministic Guardrail
        ↓
Candidate + Confirm
        ↓
Goal → Plan → Action
        ↓
UE authoritative world systems
        ↓
Receipts / Events
        ↓
Belief / Episode / Coordination / Initiative
```

核心原则：LLM 做理解与表达；UE 做世界事实、安全、路径、战斗、结算和持久化权威。

## 已完成内容

- authoritative perception / safety
- typed Goal → Plan → Action executor
- contextual suggestion cache + stale revalidation
- hold / follow / assist / routine
- deterministic combat policy and tactical Assist/Protect/Regroup
- adaptive recovery
- typed Belief with provenance + freshness
- event-driven Initiative
- grounded Episode + coverage
- Coordination Prior
- camp autonomous Routine
- Navigation / Behavior / Initiative / Local AI Runtime componentization
- bounded ContextProjection with real template token count
- capability registry/prompt alignment
- Chinese numeric quantity handling
- Schema 2 → 3 real-file migration

## 本轮返工修复

1. 修复 Unity build 中匿名 helper `Json` collision，并审计/前缀化其它通用 helper。
2. 修复 CTX-02 压力下无关 Belief 与配方数量污染 prompt，导致“新采四份木材”被误判缺数量的问题。
3. 修复 `inventory_report` 只识别阿拉伯数字、无法接受“十份木材”的 guardrail 缺陷。
4. 验证 full → compact 降档时 unresolved 安全限制不会丢失。
5. 验证 required-minimal 真正超预算时明确失败且 generation=0。
6. 新增真实 Schema 2 `.hws` → Schema 3 disk migration automation。
7. 最终重跑 executor / Initiative / tactical / routine runtime PIE。

## 同步 main 前证据

- Editor Development default Unity build：PASS
- native：41/41 PASS
- Schema migration：1/1 PASS
- real Qwen matrix：32 cases / 32 generations / 32/32 safety PASS
- core raw M01～M10：20/20 PASS
- CTX-03：compact 2832 tokens / generation=1 / restriction preserved
- CTX-04：required-minimal 4020 tokens / generation=0 / no candidate / no world write
- executor PIE：49/49
- Initiative PIE：16/16
- Tactical PIE：16/16
- Routine PIE：26/26
- repo validator：0 errors
- Python：31/31

## 真实模型说明

M11/M12/M14/M16 的 raw JSON 在部分 clean/pressure run 中仍可能给出偏宽的 collect/repair intent；deterministic guardrail 均正确拒绝/澄清并阻止世界写。raw model 与 guardrail 成功分开记录，不混为一谈。

## Evidence

统一入口：[FINAL_ACCEPTANCE](../qa/evidence/TASK-029/FINAL_ACCEPTANCE.md)。

历史建议层证据仍在 `docs/qa/evidence/TASK-029/VALIDATION.md`。更早内部任务证据由 `internal-history/` 索引；上下文/Qwen/Schema 的原始大文件保留原来源路径并由 FINAL_ACCEPTANCE 链接。

## Release

最新 `origin/main@28e7c52` 已同步到候选，保留 main 的自然地图/资产更新并合入 TASK-029 AI NPC 栈。最终 post-merge 验证已经完成：repo validator 0 errors、Python 31/31、Editor build PASS、native 41/41、TASK-029 runtime smoke 23/23。

分支已 push，PR #34 已创建并保持 open。后续只需要独立 Reviewer / Owner 审查与体验验收；**Agent 不直接 merge**。

## 2026-09-23 本机重新拉取与复验

用户要求拉取并测试指定分支；独立工作树为 G:/GameFactory/Hearthward-ai-npc-fix，受测提交 `bf5fb97eb466d38aeef7b6f092a2c6822926e6e1`。原 Hearthward 角色动画工作区保留。当前源码重新构建、原生41/41、Python31/31、模型安全32/32及核心raw20/20、CTX-03/04均通过；PIE smoke23、executor50、recovery13、initiative17、tactical17、routine27项检查通过，另完成真实键鼠输入/确认/交付闭环。

复现并修正模型/上下文测试的过时新游戏入口；恢复脚本的旧空状态断言与当前巡营状态冲突，保留失败记录后按当前语义复测通过。原始模型24/32理想分类，其余8项由确定性校验阻止执行。没有修改游戏C++、模型或资产。该次基线复验时 AI 仍限显式 Development 夹具；后续自然地图接入见下节。

详见[完整复验报告](../qa/evidence/TASK-029/revalidation-20260923/REPORT.md)。本轮脚本修订、README和证据仅在本地，未提交、未推送、未合并。

## 2026-09-23 自然地图玩法接入（当前工作树）

根据用户“先把已有的AI玩法融入游戏中”指令，标题新游戏已自动创建自然营地伙伴、有限木材点及仓储；接通对话、记忆、任务确认/执行、跟随等待巡营、工作台制作与完整存读档。旧自然地图档自动补建伙伴并保留玩家物资。共享导航改用局部 invoker 生成；未修改地图二进制资产。修正当前测试的旧新游戏入口、巡营状态误报和对话遮挡场景仍满负载绘制的问题。

构建 PASS，native 42/42，Python 31/31，自然采集存读档 22/22，工作台闭环 23 项通过，旧自然存档升级 12/12。工作台脚本随后旧档测试代码报错和可见 UI 初次 GPU 推理超时均保留原证据，详见[接入报告](../qa/evidence/TASK-029/natural-camp-integration/REPORT.md)。当前实现位于 `G:/GameFactory/Hearthward-ai-npc-fix`，原角色动画工作区不受影响。本轮代码、文档及复验证据按用户后续指令一并提交推送到当前分支；未合并 main，先前 Release 段落描述历史分支发布。

最终对话 UI 定向复测：无手动帧率限制，真实输入至任务卡 11.234 秒；保持对话运行完成 2 份木材入库，关闭后恢复自然场景。仓库通用自检 0 errors；基线任务范围自检保留 12 项授权范围扩展报告，见接入报告。

用户后续明确授权提交推送，并确认 main 已使用角色模型。fetch 确认 `origin/main@ba547c0` 已合入 PR #35；当前 AI 分支的灰盒外观属于预期版本差异。本次推送不引入 main 角色更新，避免将尚未联合验证的角色改动混入现有 AI 测试结论。
