# TASK-040 交接

## 基线与分支

- AI NPC vNext 基线：`04239f542ee99c1a735a380a66faa6ec99dc614b`
- 来源分支：`codex/ai-npc-vnext-pr`
- 当前本地分支：`codex/ai-npc-vnext-rework-01`
- 基线已包含 TASK-032～039；本任务没有回退到 TASK-029 脏工作区。
- 原始 `D:\Dev\Hearthward` checkout 的未知未提交改动没有被清理、覆盖或纳入当前 worktree。

## 完成内容

### R1｜ContextProjection

新增 `HearthwardNPCContextProjection` 深模块。

- UE 只捕获一次可信快照。
- full / compact / minimal 三档都对同一 snapshot 做纯投影。
- 当前能力相关 belief、episode、记录优先；硬规则、玩家原话和 unresolved 限制不进入普通 Top-K。
- 不再把 `observed_camp_inventory / last_seen_camp / camp_knowledge` 作为并行模型事实源重复注入。
- 每档都通过实际 llama.cpp `/apply-template` → `/tokenize` 计真实模板 token。
- 保持 `max_input_tokens=3328`、输出 256。
- 超限只尝试下一档，不生成；三档都超限时 `CONTEXT_OVERFLOW` 且 generation=0。
- 只有通过计数的同一个 Body 才进入唯一一次 `/v1/chat/completions`。
- 诊断保留 `context tier / dropped fields / input tokens / generation calls`。

### R2｜能力契约单一来源

- `Capabilities()` 继续作为能力枚举唯一事实来源。
- `CompanionOrderPrompt()` 从 registry 自动生成指令集合。
- prompt 不再硬编码 hold/follow/assist 而漏掉 routine。
- schema 测试改为结构化逐 branch 对照 registry。
- hold/follow/assist/routine 全部走相同 `Validate` 边界。
- `inventory` 与 `inventory_report` 的 read-only / cognition-write 语义仍严格分离。

### R3｜Belief 时效

`FHearthwardNPCBelief` 新增 `LastEvidenceAt`。

- `RecordedAt`：最后一次 value/source 语义变化。
- `LastEvidenceAt`：当前 value/source 最近一次有效证据。
- 同值同来源刷新只推进 evidence time，不推进 `Memory.Revision`，因此不会仅因重新观察相同事实使待确认卡失效。
- 来源/数值变化继续视为语义变化。
- 旧档缺字段时保守继承 `RecordedAt`。

### R4｜Episode 完整性

新增 persistent bounded command coverage：

- `Complete`
- `Truncated`
- `Unknown`

新任务只在真实 `SubmitGoal -> Accepted` 后登记 Complete。事件从 128 ring 被淘汰时相应 command 降为 Truncated；后续 completed/cancelled 不能恢复完整性。旧档没有 coverage 证据时迁移为 Unknown。

Episode 的终态与 coverage 分开表达：任务可以“已完成但记录已截断”。

### R5｜Save / 恢复

- Save schema：3
- NPC cognition state version：3
- 明确的 Schema 2 → 3 迁移：
  - `LastEvidenceAt = RecordedAt`
  - coverage = Unknown
- current-format 损坏字段严格拒绝，不自动伪装成旧档迁移。
- migration / validation 在 Restore 修改世界状态前完成。
- `RestoreMemory` 不再二次猜测迁移。

## 当前验证

PASS：

- `python scripts/validate_repo.py`：0 errors
- repository Python tests：31/31
- UE Editor Development build：PASS
- full native `Hearthward.*`：40/40 Success
- 关键新回归：
  - `Hearthward.NPCAgent.CapabilitiesAndLimits`
  - `Hearthward.NPCAgent.BeliefStateProvenance`
  - `Hearthward.NPCAgent.BoundedContextProjection`
  - `Hearthward.NPCAgent.GroundedEpisodeProjection`
  - `Hearthward.Save.FileIntegrityAndSnapshot`
  - `Hearthward.Save.NPCMemoryCompatibility`
  - `Hearthward.Save.PoolProtectionAndSafety`

构建环境说明：

- 仓库锁定：UE 5.8.1
- 本机实际：UE 5.8.2
- 因此当前 build/native 结果是强回归信号，但锁定 5.8.1 复验仍为 NOT_RUN。
- 本轮复核重跑 repo validator 0 errors、Python 31/31；核对原生日志 40 条 Success、0 条失败/崩溃标记。

真实模型当前源码：

- 模型 bundle 存在于 `D:\Dev\Hearthward\Runtime\LocalAI`。
- GGUF SHA256 与 `config/local-ai.lock.json` 一致。
- Unreal built-in Python Remote Execution 能发现当前 worktree 的 UE process。
- 当前工具环境缺少项目约定的 GameFactory `engine_adapters.ue5` UEClient；命令行 `-game` process 的 remote Python 又没有可用 Game WorldContext，无法可靠触发 world-scoped `Hearthward.AI.Say`。
- 因此当前源码真实 Qwen CTX-01～CTX-04：**BLOCKED / NOT_RUN**。
- TASK-039 的历史 `CONTEXT_OVERFLOW` 不作为 TASK-040 当前源码结果复用。

详见 [TASK-040 验证](../qa/evidence/TASK-040/VALIDATION.md) 与 [ADR](../decisions/ADR-TASK-040-npc-context-and-cognition.md)。

## 尚未验收完毕

- 当前源码真实 Qwen CTX-01～04 与 16 类样本 × 干净/压力进度至少 32 次请求尚未执行；现有 PIE 脚本只准备了 collect 正常/压力和 routine 局部场景，不能替代整套验收。
- TASK-028/034/036/038 的本轮相关 runtime PIE 回归尚未逐套重跑。
- Save 测试读取了更早的真实旧档，但尚无真实 pre-TASK-040 Schema 2 文件或对应旧版序列化路径的迁移证明。
- 仓库锁定 UE 5.8.1 构建/测试、独立 Reviewer、Owner 体验验收和技术合并条件仍未闭合。
- 本轮范围核对为 28 个改动文件，全部位于 TASK-040 allowed_paths，Content/ 为零修改。

## 流程状态

TASK-040 保持 `Blocked`：

- 真实 GitHub Issue：未登记
- 独立 Reviewer：未登记
- commit：用户已于本轮授权提交到独立任务分支；提交标识以 Git 历史为准
- push：用户已于本轮授权推送独立任务分支；远端状态以 GitHub 分支为准
- PR：未执行
- merge：明确未授权，未执行

本地代码已经实现并通过当前可执行回归，但不把本地验证冒充正式集成完成。
