# TASK-040 Validation
> 2026-09-23 clean committed-tree verification supersedes the PASS/NOT_RUN summary below. See [CLEAN_TREE_REVIEW.md](CLEAN_TREE_REVIEW.md); the earlier 40/40 native run is historical worktree evidence.


基线：`04239f542ee99c1a735a380a66faa6ec99dc614b`

工作分支：`codex/ai-npc-vnext-rework-01`

日期：2026-09-23

## R0 原始工作树环境与锁（历史记录）

- HEAD / AI vNext 基线：`04239f542ee99c1a735a380a66faa6ec99dc614b`
- `config/local-ai.lock.json` SHA256：`9910343014e0159e5e18d6603fdb2303c131b2f53b633488297d3fbe0814c4b2`
- `config/toolchain.lock.json` SHA256：`ecce1cdde269a1130f189ec9ba56bc31f8f51a7b4425972885fd6ef6d92ed12b`
- `Hearthward.uproject` SHA256：`357a6365d8c22035c528b1c9a5288cbee1bed7db6a3b1cd891dc15e552bf3f8c`
- 锁定工具链：UE 5.8.1 / MSVC 19.44.35228 / Windows SDK 10.0.22621.0
- 本机实际 UE：5.8.2-56702186，`D:\UE5.8\UE_5.8`
- 本机模型：`D:\Dev\Hearthward\Runtime\LocalAI\models\Qwen3.5-4B-Q4_K_M.gguf`
- GGUF SHA256：`00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4`，与 lock 一致

环境偏差：本机可编译 UE 5.8.2，但不是仓库锁定的 5.8.1，因此构建结果是当前机器诊断/回归信号，不替代锁定工具链复验。

## 自动验证（历史工作树；当前结果见 CLEAN_TREE_REVIEW.md）

| 项目 | 状态 | 证据 |
|---|---|---|
| repository validator | PASS | `python scripts/validate_repo.py` → 0 errors |
| repository Python tests | PASS | 31/31 |
| Editor Development build | PASS | 当前 worktree 源码在 UE 5.8.2 下编译、链接 `UnrealEditor-Hearthward.dll` 成功 |
| native `Hearthward.*` | PASS | 40/40 Success；无 Fail/Error/Assertion/Fatal/Ensure |
| task baseline scope validator | BLOCKED | TASK-040 是本轮新任务，不存在于 `04239f5` 基线，validator 正确拒绝把当前 JSON 冒充 baseline-approved snapshot |
| locked UE 5.8.1 build | NOT_RUN | 当前机器仅发现 5.8.2 |
| current-source real Qwen CTX e2e | BLOCKED | 见“真实模型验证” |
| 16 类样本 × 干净/压力进度 | NOT_RUN | 至少 32 次真实请求尚无当前源码结果 |
| real pre-TASK-040 Schema 2 file migration | NOT_RUN | 现有测试使用更早真实旧档及新结构构造；缺真实 Schema 2 文件或旧版序列化路径 |
| related TASK-028/034/036/038 runtime PIE | NOT_RUN | 本轮尚未逐套重跑 |

原生完整结果保存在运行时日志 `Saved/Logs/Hearthward-backup-2026.09.23-03.51.07.log`；该日志不是 Git 交付物。本轮复核检查到 40 条 `Test Completed. Result={Success}`、0 条失败/崩溃标记，并重新执行 repo validator 0 errors、Python 31/31。基线工作树共有 28 个改动文件，全部在 TASK-040 allowed_paths，Content/ 零修改。

关键新回归均 PASS：

- `Hearthward.NPCAgent.CapabilitiesAndLimits`
- `Hearthward.NPCAgent.BeliefStateProvenance`
- `Hearthward.NPCAgent.BoundedContextProjection`
- `Hearthward.NPCAgent.GroundedEpisodeProjection`
- `Hearthward.Save.FileIntegrityAndSnapshot`
- `Hearthward.Save.NPCMemoryCompatibility`
- `Hearthward.Save.PoolProtectionAndSafety`

## R1 ContextProjection

PASS（原生/静态）：

- full / compact / minimal 只有三档。
- 三档从同一 `FHearthwardNPCContextSnapshot` 纯投影，测试确认同一 snapshot 输出 deterministic。
- minimal 在压力数据下小于 full。
- 硬规则和 unresolved 原始限制不会被普通 Top-K 裁剪。
- 旧的 `observed_camp_inventory / last_seen_camp / camp_knowledge` 不再作为新的并行模型事实通道。
- 真实 request 流使用 llama.cpp `/apply-template` + `/tokenize`，超限只降档，不调用 generation。
- 只有通过 token count 的同一 Body 才进入 `Generate`；`GenerationCalls` 在真实 `/v1/chat/completions` 前递增。

BLOCKED（真实模型 e2e）：

- CTX-01 正常输入实际 token/tier/generation=1
- CTX-02 压力输入实际自动降档
- CTX-03 必保内容保持不丢
- CTX-04 三档均超限时实际 generation=0

原因不是模型文件缺失。bundle 与 GGUF hash 已确认，且 Unreal built-in Python Remote Execution 可以发现当前 worktree 的 UE 5.8.2 process；但当前 `-game` 实例中 remote Python 无可用 Game WorldContext，项目 world-scoped debug commands 无法可靠触发。当前 Python 环境也没有仓库约定的 GameFactory `engine_adapters.ue5` UEClient。因此不把 TASK-039 的历史 overflow 当作本轮当前源码 PASS/FAIL。

## R2 Capability Contract

PASS：

- schema 从 registry 生成。
- system prompt 的 companion directive 列表从 registry 生成。
- `routine` 与 hold/follow/assist 走相同 Validate/preflight。
- `inventory` 与 `inventory_report` 保持不同 mode/source/authority。
- 未扩张能力集合或世界写权限。

## R3 Belief Freshness

PASS：

- 同值、同 source 新证据刷新 `LastEvidenceAt`。
- `RecordedAt` 保持 semantic change time。
- `Memory.Revision` 不因为 freshness-only 更新增长。
- 旧时间证据不能倒退 freshness。
- source/value 变化仍走 semantic revision。
- Schema 2 旧档缺失字段保守迁移为 `LastEvidenceAt = RecordedAt`。

## R4 Episode Coverage

PASS：

- current accepted command 完整事件时为 `Complete`。
- event-only compatibility projection 为 `Unknown`。
- 活动 command 早期事件被 128 ring 全部挤出后仍保留 metadata 并变为 `Truncated`。
- 后续 delivered/completed 不会把 Truncated 恢复为 Complete。
- terminal state 与 coverage 分离。
- legacy event history 迁移为 Unknown。
- metadata 按活动 command + 当前 buffered episode command 有界。

## R5 Save / Regression

PASS（原生）：

- Save schema / cognition state 显式升级到 3。
- Schema 2 旧 vNext 的迁移路径已实现；真实 Schema 2 文件级迁移证据仍缺。
- current Schema 3 损坏 `LastEvidenceAt` 直接拒绝，不走 legacy repair。
- existing Save file integrity / snapshot / NPC memory compatibility tests 全绿。
- Local AI cancel / pending / structured boundary tests 全绿。
- full `Hearthward.*` 40/40 覆盖现有 inventory、gameplay、companion、save 等原生回归。

NOT_RUN：

- TASK-028/034/036/038 的既有专门 runtime PIE 脚本本轮未逐套重新执行。
- 当前源码真实模型 end-to-end 未完成，原因见上。

## 流程状态

- commit：本轮用户已授权在独立任务分支提交；实际提交以 Git 历史为准
- push：本轮用户已授权推送独立任务分支；实际发布以 GitHub 分支为准
- PR：NOT_RUN / 未授权创建
- merge：NOT_RUN / explicitly not authorized
- real GitHub Issue / reviewer：BLOCKED / 未登记

该文件保存此前工作树的详细实现与测试记录。当前分支结论以 [CLEAN_TREE_REVIEW.md](CLEAN_TREE_REVIEW.md) 为准：默认 UE 构建失败，当前干净提交原生测试未运行，TASK-040 保持 `Blocked`。
